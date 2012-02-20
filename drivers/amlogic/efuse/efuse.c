/*
 * E-FUSE char device driver.
 *
 * Author: Bo Yang <bo.yang@amlogic.com>
 *
 * Copyright (c) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the smems of the GNU General Public License as published by
  * the Free Software Foundation; version 2 of the License.
  *
  */

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include "mach/am_regs.h"

#include <linux/efuse.h>
#include "efuse_regs.h"

#define EFUSE_MODULE_NAME   "efuse"
#define EFUSE_DRIVER_NAME   "efuse"
#define EFUSE_DEVICE_NAME   "efuse"
#define EFUSE_CLASS_NAME    "efuse"

#define EFUSE_BITS		3072
#define EFUSE_BYTES		384  //(EFUSE_BITS/8)
#define EFUSE_DWORDS		96  //(EFUSE_BITS/32)

#define DOUBLE_WORD_BYTES	4
//#define hemingdebug	printk("heming add %s %d",__FUNCTION__,__LINE__);
#define EFUSE_READ_ONLY     

/* efuse layout
http://wiki-sh.amlogic.com/index.php/How_To_burn_the_info_into_E-Fuse
0~3			licence				1 check byte			4 bytes(in total)
4~10			mac				1 check byte			7 bytes(in total)
12~322			hdcp				10 check byte			310 bytes(in total)
322~328 		mac_bt				1 check byte			7 bytes(in total)
330~336 		mac_wifi			1 check byte			7 bytes(in total)
337~384  		usid				2 check byte		 	48 bytes(in total)
*/
#define LICENSE_POS		0
#define MAC_POS			4
#define HDCP_POS		12
#define MAC_BT_POS		322
#define MAC_WIFI_POS		330
#define USERDATA_POS		337
 
#define EFUSE_LICENSE_DATA_LEN	3
#define EFUSE_MAC_DATA_LEN	6
#define EFUSE_HDCP_DATA_LEN	300
#define EFUSE_BTMAC_DATA_LEN	6
#define EFUSE_WIFIMAC_DATA_LEN	6
#define EFUSE_USER_DATA_LEN	48

#define EFUSE_LICENSE_TOTAL	4
#define EFUSE_MAC_TOTAL		7
#define EFUSE_HDCP_TOTAL	310
#define EFUSE_BTMAC_TOTAL	7
#define EFUSE_WIFIMAC_TOTAL	7
#define EFUSE_USER_TOTAL	62

#define USR_LICENCE		1
#define USR_MACADDR		2
#define USR_HDMIHDCP		3
#define USR_USERIDF		4

static unsigned long efuse_status;
#define EFUSE_IS_OPEN           (0x01)

//#define EFUSE_DEBUG                                       
                                                                                      
typedef struct efuse_dev_s {
	struct cdev         cdev;
	unsigned int        flags;
} efuse_dev_t;

static efuse_dev_t *efuse_devp;
//static struct class *efuse_clsp;
static dev_t efuse_devno;


unsigned char usid[EFUSE_USERIDF_BYTES] = {0};	 //48

#ifdef EFUSE_DEBUG

static unsigned long efuse_test_buf_32[EFUSE_DWORDS];
static unsigned char* efuse_test_buf_8 = (unsigned char*)efuse_test_buf_32;

static void __efuse_write_byte_debug(unsigned long addr, unsigned char data)
{
	efuse_test_buf_8[addr] = data;	
}

static void __efuse_read_dword_debug(unsigned long addr, unsigned long *data)    
{
	*data = efuse_test_buf_32[addr >> 2];
}

#endif


static void __efuse_write_byte( unsigned long addr, unsigned long data );
static void __efuse_read_dword( unsigned long addr, unsigned long *data);


static void __efuse_write_byte( unsigned long addr, unsigned long data )
{
	unsigned long auto_wr_is_enabled = 0;

	if (READ_CBUS_REG( EFUSE_CNTL1) & (1 << CNTL1_AUTO_WR_ENABLE_BIT)) {                                                                                
		auto_wr_is_enabled = 1;
	} else {                                                                                
		/* temporarily enable Write mode */
		WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_ON,
		CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

	/* write the address */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, addr,
	CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );
	/* set starting byte address */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
	CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
	CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

	/* write the byte */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, data,
	CNTL1_BYTE_WR_DATA_BIT, CNTL1_BYTE_WR_DATA_SIZE );
	/* start the write process */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
	CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
	CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	/* dummy read */
	READ_CBUS_REG( EFUSE_CNTL1 );

	while ( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_BUSY_BIT ) ) {                                                                                
		udelay(1);
	}

	/* if auto write wasn't enabled and we enabled it, then disable it upon exit */
	if (auto_wr_is_enabled == 0 ) {                                                                                
		WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_OFF,
		CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

	//printk(KERN_INFO "__efuse_write_byte: addr=%ld, data=0x%ld\n", addr, data);
}

static void __efuse_read_dword( unsigned long addr, unsigned long *data )
{
	unsigned long auto_rd_is_enabled = 0;

	if( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ) ){                                                                               
		auto_rd_is_enabled = 1;
	} else {                                                                               
		/* temporarily enable Read mode */
		WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
		CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	}

	/* write the address */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, addr,
	CNTL1_BYTE_ADDR_BIT,  CNTL1_BYTE_ADDR_SIZE );
	/* set starting byte address */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
	CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
	CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

	/* start the read process */
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
	CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
	WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
	CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
	/* dummy read */
	READ_CBUS_REG( EFUSE_CNTL1 );

	while ( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) ) {                                                                               
		udelay(1);
	}
	/* read the 32-bits value */
	( *data ) = READ_CBUS_REG( EFUSE_CNTL2 );

	/* if auto read wasn't enabled and we enabled it, then disable it upon exit */
	if ( auto_rd_is_enabled == 0 ){                                                                               
		WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
		CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	}

	//printk(KERN_INFO "__efuse_read_dword: addr=%ld, data=0x%lx\n", addr, *data);
}

static int efuse_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	efuse_dev_t *devp;

	devp = container_of(inode->i_cdev, efuse_dev_t, cdev);
	file->private_data = devp;

	return ret;
}

static int efuse_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	efuse_dev_t *devp;

	devp = file->private_data;
	efuse_status &= ~EFUSE_IS_OPEN;
	return ret;
}

static ssize_t __efuse_read( char *buf,
	size_t count, loff_t *ppos )
{
	unsigned long* contents = (unsigned long*)kzalloc(sizeof(unsigned long)*EFUSE_DWORDS, GFP_KERNEL);
	unsigned pos = *ppos;
	unsigned long *pdw;
	char* tmp_p;
	
	/*pos may not align to 4*/
	unsigned int dwsize = (count + 3 +  pos%4) >> 2;

	if (!contents) {
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}
	if (pos >= EFUSE_BYTES)
		return 0;

	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;

	for (pdw = contents + pos/4; dwsize-- > 0 && pos < EFUSE_BYTES; pos += 4, ++pdw) {
		#ifdef EFUSE_DEBUG     
		__efuse_read_dword_debug(pos, pdw);
		#else
		/*if pos does not align to 4,  __efuse_read_dword read from next dword, so, discount this un-aligned partition*/
		__efuse_read_dword((pos - pos%4), pdw);
		#endif
	}     
	
	tmp_p = (char*)contents;
       tmp_p += *ppos;                           

	memcpy(buf, tmp_p, count);

	*ppos += count;
	
	if (contents)
		kfree(contents);
	return count;
}

ssize_t efuse_read_kernel( char *buf,	size_t count, loff_t *ppos )
{
    return __efuse_read(buf, count, ppos);
}
EXPORT_SYMBOL(efuse_read_kernel);

static ssize_t efuse_read( struct file *file, char __user *buf,
	size_t count, loff_t *ppos )
{
	int ret;
	loff_t local_ppos = *ppos;
	size_t local_count = 0;
	unsigned char* local_buf = (unsigned char*)kzalloc(sizeof(char)*count, GFP_KERNEL);
	
	if (!local_buf) {
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}
	
	local_count = __efuse_read(local_buf, count, &local_ppos);
	
	if (local_count <= 0) {
		ret =  -EFAULT;
		goto error_exit;
	}

	if (copy_to_user((void*)buf, (void*)local_buf, local_count)) {                                  
		ret =  -EFAULT;
		goto error_exit;
	}

error_exit:
	if (local_buf) 
		kfree(local_buf);
	return local_count;
}

static int check_if_efused(loff_t pos, size_t count)
{
	loff_t local_pos = pos;
	int i;
	unsigned char* buf = (unsigned char*)kzalloc(sizeof(char)*count, GFP_KERNEL);
	if (buf) {
		if (__efuse_read(buf, count, &local_pos) == count) {
			for (i = 0; i < count; i++) {
				if (buf[i]) {
					printk("pos %d value is %d", pos + i, buf[i]);
					return 1;
				}
			}
		}
	} else {
		printk("no memory \n");
		return -ENOMEM;
	}
	kfree(buf);
	buf = NULL;
	return 0;
}
                                                                                     
static ssize_t efuse_write( struct file *file, const char __user *buf,
	size_t count, loff_t *ppos )
{
	unsigned char* contents = (char*)kzalloc(sizeof(char)*EFUSE_BYTES, GFP_KERNEL);                                        
	unsigned pos = *ppos;
	unsigned char *pc;
	int ret;                                                         

	if (!contents) {
		printk(KERN_INFO "memory not enough\n");
		return -ENOMEM;
	}                              
	if (pos >= EFUSE_BYTES)
                 return 0;       /* Past EOF */

	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;

	//printk(KERN_INFO "\nefuse_write: f_pos: %lld, ppos: %lld\n", file->f_pos, *ppos);

	if (ret = check_if_efused(pos, count)) {
		printk(KERN_INFO "the chip has been efused\n");
		if (ret == 1)
			return -EROFS;
		else if (ret < 0)
			return ret;
	}

	if (copy_from_user(contents, buf, count))                                   
		return -EFAULT;                                                     
                                                                                     
	for (pc = contents; count--; ++pos, ++pc) {
		#ifdef EFUSE_DEBUG    
         	__efuse_write_byte_debug(pos, *pc);  
         	#else                             
                __efuse_write_byte(pos, *pc);   
                #endif
	}                                                                                                                         
                                                                                     
	*ppos = pos;                  	
	
	if (contents)
		kfree(contents);    
	return pc - contents;                                                       
}                    

#ifndef EFUSE_READ_ONLY
static ssize_t __efuse_write(const char *buf,
	size_t count, loff_t *ppos )
{
	unsigned pos = *ppos;
	loff_t *readppos = ppos;
	unsigned char *pc;
	char efuse_data[EFUSE_USERIDF_BYTES],null_data[EFUSE_USERIDF_BYTES];

	if (pos >= EFUSE_BYTES)
		return 0;       /* Past EOF */

	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;

	__efuse_read(efuse_data, count, readppos);
	memset(null_data,0,count);
	if(strncmp(efuse_data,null_data,count) != 0){
		printk(" Data had written ,the block is not clean!!!\n");
		return -EFAULT;
	}
	for (pc = buf; count--; ++pos, ++pc)
		__efuse_write_byte(pos, *pc);

	*ppos = pos;

	return (const char *)pc - buf;
}
#endif

static int efuse_ioctl( struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg )
{
	switch (cmd)
	{
		case EFUSE_ENCRYPT_ENABLE:
			WRITE_CBUS_REG_BITS( EFUSE_CNTL4, CNTL4_ENCRYPT_ENABLE_ON,
			CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE);
			break;

		case EFUSE_ENCRYPT_DISABLE:
			WRITE_CBUS_REG_BITS( EFUSE_CNTL4, CNTL4_ENCRYPT_ENABLE_OFF,
			CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE);
			break;

		case EFUSE_ENCRYPT_RESET:
			WRITE_CBUS_REG_BITS( EFUSE_CNTL4, CNTL4_ENCRYPT_RESET_ON,
			CNTL4_ENCRYPT_RESET_BIT, CNTL4_ENCRYPT_RESET_SIZE);
			break;

		default:
			return -ENOTTY;
	}
	return 0;
}

loff_t efuse_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;

	switch(whence) {
		case 0: /* SEEK_SET */
			newpos = off;
			break;

		case 1: /* SEEK_CUR */
			newpos = filp->f_pos + off;
			break;

		case 2: /* SEEK_END */
			newpos = EFUSE_BYTES + off;
			break;

		default: /* can't happen */
			return -EINVAL;
	}

	if (newpos < 0)
		return -EINVAL;                                             
	filp->f_pos = newpos;
		return newpos;
}

static const struct file_operations efuse_fops = {
	.owner      = THIS_MODULE,
	.llseek     = efuse_llseek,
	.open       = efuse_open,
	.release    = efuse_release,
	.read       = efuse_read,
	.write      = efuse_write,
	.ioctl      = efuse_ioctl,
};

/* Sysfs Files */
static ssize_t mac_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char buf_mac[8] = {0};
	char dec_mac[6] = {0};
	loff_t ppos = MAC_POS;
	__efuse_read(buf_mac, sizeof(buf_mac), &ppos);
	efuse_bch_dec(buf_mac, 7, dec_mac);
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			dec_mac[0],dec_mac[1],dec_mac[2],dec_mac[3],dec_mac[4],dec_mac[5]);
}

static ssize_t mac_wifi_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char buf_mac[8] = {0};
	char dec_mac[6] = {0};
	loff_t ppos = MAC_WIFI_POS;
	__efuse_read(buf_mac, sizeof(buf_mac), &ppos);
	efuse_bch_dec(buf_mac, 7, dec_mac);
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		dec_mac[0],dec_mac[1],dec_mac[2],dec_mac[3],dec_mac[4],dec_mac[5]);
}

static ssize_t mac_bt_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char buf_mac[8] = {0};
	char dec_mac[6] = {0};
	loff_t ppos = MAC_BT_POS;
	__efuse_read(buf_mac, sizeof(buf_mac), &ppos);
	efuse_bch_dec(buf_mac, 7, dec_mac);
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		dec_mac[0],dec_mac[1],dec_mac[2],dec_mac[3],dec_mac[4],dec_mac[5]);
}



#ifdef CONFIG_MACH_MESON_8726M_REFB09


extern int get_board_version(void);

static ssize_t board_version_show(struct class *cla, struct class_attribute *attr, char *buf)

{
	
	int board_version=0;

	
	board_version = get_board_version();

	
	return sprintf(buf, "%01d\n", board_version);

}



extern int get_uboot_version(void);

static ssize_t uboot_version_show(struct class *cla, struct class_attribute *attr, char *buf)

{
	
	int uboot_version=0;

	
	uboot_version = get_uboot_version();

	
	return sprintf(buf, "%01d\n", uboot_version);

}


#endif /* CONFIG_MACH_MESON_8726M_REFB09 */



static inline int cm(int p, int x)
{
	int i, tmp;

	tmp = x;
	for (i=0; i<p; i++) {
		tmp <<= 1;
		if (tmp&(1<<8)) tmp ^= 0x11d;
	}

	return tmp;
}

static inline int gm(int x, int y)
{
	int i, tmp;

	tmp = 0;
	for (i=0; i<8; i++) {
		if (y&(1<<i)) tmp ^= cm(i, x);
	}

	return tmp;
}

static void bch_enc(int c[255], int n, int t)
{
	int i, j, r, gate;
	int b[20], g[20];
	char gs1[] = "101110001";
	char gs2[] = "11000110111101101";

	memset(g, 0, 20*sizeof(int));
	memset(b, 0, 20*sizeof(int));
	r = t*8;

	for (i=0; i<r+1; i++) {
		g[i] = (t==1 ? gs1[i] : gs2[i]) - '0';
	}

	for (i=0; i<n; i++) {
		c[i] = i<n-r ? c[i] : b[r-1];
		gate = i<n-r ? c[i]^b[r-1] : 0;
		for (j=r-1; j>=0; j--)
		b[j] = gate*g[j] ^ (j==0?0:b[j-1]);
	}
}

static int bch_dec(int c[255], int n, int t)
{
	int i, j, tmp;
	int s[4], a[6], b[6], temp[6], deg_a, deg_b;
	int sig[5], deg_sig;
	int errloc[4], errcnt;

	memset(s, 0, 4*sizeof(int));
	for (j=0; j<n; j++)
		for (i=0; i<t*2; i++)
			s[i] = cm(i+1, s[i]) ^ (c[j]&1);

	memset(a, 0, 6*sizeof(int));
	memset(b, 0, 6*sizeof(int));
	a[0] = 1;
	deg_a = t*2;
	for (i=0; i<t*2; i++) b[i] = s[t*2-1-i];
		deg_b = t*2-1;

	a[t*2+1] = 1;
	b[t*2+1] = 0;
	while (deg_b >= t) {
		if (b[0] == 0) {
			memmove(b, b+1, 5*sizeof(int));
			b[5] = 0;
			deg_b--;
        }
        else {
			for (i=t*2+1; i>deg_a; i--)
				b[i] = gm(b[i], b[0]) ^ gm(a[i], a[0]);
			for (i=deg_a; i>deg_b; i--) {
				b[i] = gm(b[i], b[0]);
				a[i] = gm(a[i], b[0]);
			}
			for (; i>0; i--)
				a[i] = gm(a[i], b[0]) ^ gm(b[i], a[0]);
				memmove(a, a+1, 5*sizeof(int));
				a[5] = 0;
				deg_a--;

			if (deg_a < deg_b) {
				memcpy(temp, a, 6*sizeof(int));
				memcpy(a, b, 6*sizeof(int));
				memcpy(b, temp, 6*sizeof(int));

				tmp = deg_a;
				deg_a = deg_b;
				deg_b = tmp;
			}
		}
	}

	deg_sig = t*2 - deg_a;
	memcpy(sig, a+deg_a+1, (deg_sig+1)*sizeof(int));

	errcnt = 0;
	for (j=0; j<255; j++) {
		tmp = 0;
		for (i=0; i<=deg_sig; i++) {
			sig[i] = cm(i, sig[i]);
			tmp ^= sig[i];
		}

		if (tmp == 0) {
			errloc[errcnt] = j - (255-n);
			if (errloc[errcnt] >= 0)
			errcnt++;
		}
	}

	if (errcnt<deg_sig) {
		return -1;
	}

	for (i=0; i<errcnt; i++) {
		c[errloc[i]] ^= 1;
		__D("fix error at %4d\n", errloc[i]);
	}

	return errcnt;
}


void efuse_bch_enc(const char *ibuf, int isize, char *obuf)
{
	int i, j;
	int cnt, tmp;
	int errnum, errbit;
	char info;
	int c[255];

	int t = BCH_T;
	int n = isize*8 + t*8;

	for (i = 0; i < isize; ++i) {
		info = ibuf[i];
		info = ~info;
		for (j = 0; j < 8; ++j) {
			c[i*8 + j] = info >> (7 - j)&1;
		}
	}

	bch_enc(c, n, t);

#ifdef __ADDERR
	/* add error */
	errnum = t;
	for ( i = 0; i < errnum; ++i) {
		errbit = rand()%n;
		c[errbit] ^= 1;
		__D("add error #%d at %d\n", i, errbit );
	}
#endif

	for (i = 0; i < n/8; ++i) {
		tmp = 0;
		for (j = 0; j < 8; ++j) {
			tmp += c[i*8 + j]<<(7-j);
		}

		obuf[i] = ~tmp;
	}
}

void efuse_bch_dec(const char *ibuf, int isize, char *obuf)
{
	int i, j;
	int cnt, tmp;
	char info;
	int c[255];

	int t = BCH_T;
	int n = isize*8;


	for (i = 0; i < isize; ++i) {
		info = ibuf[i];
		info = ~info;
		for (j = 0; j < 8; ++j) {
			c[i*8 + j] = info >> (7 - j)&1;
		}
	}

	bch_dec(c, n, t);

	for (i = 0; i < (n/8 - t); ++i) {
		tmp = 0;
		for (j = 0; j < 8; ++j) {
			tmp += c[i*8 + j]<<(7-j);
		}

		obuf[i] = ~tmp;
	}
}

static int efuse_device_match(struct device *dev, void *data)
{
	return (!strcmp(dev->kobj.name,(const char*)data));
}

struct device *efuse_class_to_device(struct class *cla)
{
	struct device		*dev;

	dev = class_find_device(cla, NULL, cla->name,
				efuse_device_match);
	if (!dev)
		printk("%s no matched device found!/n",__FUNCTION__);
	return dev;
}

#if 0
unsigned char re_usid[EFUSE_USERIDF_BYTES+2] = {0};		//64

unsigned char *efuse_read_usr(int usr_type)
{
	unsigned char buf[EFUSE_BYTES];
	unsigned char buf_dec[EFUSE_BYTES];
	loff_t ppos;
	size_t count;
	int dec_len,i;
	char *op;
	ppos =320; count=64;dec_len=62;op=re_usid;

	memset(op,0,count);
	memset(buf,0,sizeof(buf));
	memset(buf_dec,0,sizeof(buf_dec));
	__efuse_read(buf, count, &ppos);


	memcpy(buf_dec,buf+2,dec_len);


	if(dec_len>30)
		for(i=0;i*31<dec_len;i++)
			efuse_bch_dec(buf_dec+i*31, 31, op+i*30);
	else
		efuse_bch_dec(buf_dec, dec_len, op);

	return op;

}
unsigned char *efuse_read_usr_workaround(int usr_type)
{

	unsigned char buf[EFUSE_BYTES];
	unsigned char buf_dec[EFUSE_BYTES];
	loff_t ppos;
	size_t count;
	int dec_len,i;
	char *op;
	ppos =324; count=24;dec_len=21;op=re_usid;

	memset(op,0,count);
	memset(buf,0,sizeof(buf));
	memset(buf_dec,0,sizeof(buf_dec));
	__efuse_read(buf, count, &ppos);

	/*
	if(usr_type==USR_USERIDF)
		memcpy(buf_dec,buf+2,dec_len);
	else*/
		memcpy(buf_dec,buf,dec_len);

	if(dec_len>30)
		for(i=0;i*31<dec_len;i++)
			efuse_bch_dec(buf_dec+i*31, 31, op+i*30);
	else
		efuse_bch_dec(buf_dec, dec_len, op);

	return op;


}

static ssize_t userdata_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char *op;
	op=efuse_read_usr(4);
	if((op[0]==7)&&(op[1]==0)&&(op[2]==1)&&(op[3]==3)&&(op[4]==0)&&(op[5]==2)){
		printk( KERN_INFO"read usid ok\n");
	}
	else{
		op=efuse_read_usr_workaround(4);
		if((op[0]==7)&&(op[1]==0)&&(op[2]==1)&&(op[3]==3)&&(op[4]==0)&&(op[5]==2)){
			printk( KERN_INFO"read usid ok\n");
		}
		else{
			printk( KERN_INFO"read usid error\n");
			return -1;
		}
	}
	return sprintf(buf, "%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d%01d\n",
    			   op[0],op[1],op[2],op[3],op[4],op[5],
    			   op[6],op[7],op[8],op[9],op[10],op[11],
    			   op[12],op[13],op[14],op[15],op[16],op[17],
    			   op[18],op[19]
    			   );
}
#else
unsigned char *efuse_read_usr(struct efuse_platform_data *data)
{
	loff_t ppos;
	char *op;
	ppos = data->pos;
	op=usid;
	memset(op,0,sizeof(op));
	__efuse_read(op, data->count, &ppos);
	return op;
}

static ssize_t userdata_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char *op;
	bool ret = true;
	int i;
	struct efuse_platform_data *data = NULL;
	struct device	*dev = efuse_class_to_device(cla);
	data = dev->platform_data;
	if(!data){
		printk( KERN_ERR"%s error!no platform_data!\n",__FUNCTION__);
		return -1;
	}
	op=efuse_read_usr(data);
	if(data->data_verify)
		ret = data->data_verify(op);
	if(!ret){
		printk("%s error!data_verify failed!\n",__FUNCTION__);
		return -1;
	}
	/*return sprintf(buf, "%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c\n",
    			   op[0],op[1],op[2],op[3],op[4],op[5],
    			   op[6],op[7],op[8],op[9],op[10],op[11],
    			   op[12],op[13],op[14],op[15],op[16],op[17],
    			   op[18],op[19]);*/
	memcpy(buf,op,strlen(op));
	printk("buf is ");
    	for(i=0;i<data->count;i++)
	    	printk("%c",buf[i]);
	return strlen(op);
}

#ifndef EFUSE_READ_ONLY
static ssize_t efuse_write_usr(struct efuse_platform_data *data,const unsigned char *buf)
{
	loff_t ppos;
	ppos = data->pos;
	return __efuse_write(buf, data->count, &ppos)+1;//add \0,size+1
}
#endif

#ifndef EFUSE_READ_ONLY
static ssize_t userdata_write(struct class *cla, struct class_attribute *attr, char *buf,size_t count)
{
	struct efuse_platform_data *data = NULL;
	struct device	*dev = NULL;
	bool ret = true;
	dev = efuse_class_to_device(cla);
	data = dev->platform_data;
	if(!data){
		printk( KERN_ERR"%s error!no platform_data!\n",__FUNCTION__);
		return -1;
	}
	if(data->data_verify)
		ret = data->data_verify(buf);
	if(!ret){
		printk("%s error!data_verify failed!\n",__FUNCTION__);
		return -1;
	}
	efuse_write_usr(data,buf);
	return count;
}
#endif
#endif


static struct class_attribute efuse_class_attrs[] = {
	
#ifdef CONFIG_MACH_MESON_8726M_REFB09
    
	__ATTR_RO(board_version),
    
	__ATTR_RO(uboot_version),
	
#endif /* CONFIG_MACH_MESON_8726M_REFB09 */
    
	__ATTR_RO(mac),
    
	__ATTR_RO(mac_wifi),
    
	__ATTR_RO(mac_bt),
  
	#ifndef EFUSE_READ_ONLY		/*make the efuse can not be write through sysfs */
	__ATTR(userdata, S_IRWXU, userdata_show, userdata_write),
    
	#else
	__ATTR_RO(userdata),
 
	#endif
	__ATTR_NULL

};



static struct class efuse_class = {
    
	.name = EFUSE_CLASS_NAME,
    
	.class_attrs = efuse_class_attrs,

};



static int efuse_probe(struct platform_device *pdev)
{
	 int ret;
	 struct device *devp;

	 ret = alloc_chrdev_region(&efuse_devno, 0, 1, EFUSE_DEVICE_NAME);
	 if (ret < 0) {
			 printk(KERN_ERR "efuse: failed to allocate major number\n");
	 ret = -ENODEV;
	 goto out;
	 }

//	   efuse_clsp = class_create(THIS_MODULE, EFUSE_CLASS_NAME);
//	   if (IS_ERR(efuse_clsp)) {
//		   ret = PTR_ERR(efuse_clsp);
//		   goto error1;
//	   }
	 ret = class_register(&efuse_class);
	 if (ret)
		 goto error1;

	 efuse_devp = kmalloc(sizeof(efuse_dev_t), GFP_KERNEL);
	 if ( !efuse_devp ) {
		 printk(KERN_ERR "efuse: failed to allocate memory\n");
		 ret = -ENOMEM;
		 goto error2;
	 }

	 /* connect the file operations with cdev */
	 cdev_init(&efuse_devp->cdev, &efuse_fops);
	 efuse_devp->cdev.owner = THIS_MODULE;
	 /* connect the major/minor number to the cdev */
	 ret = cdev_add(&efuse_devp->cdev, efuse_devno, 1);
	 if (ret) {
		 printk(KERN_ERR "efuse: failed to add device\n");
		 goto error3;
	 }

	 //devp = device_create(efuse_clsp, NULL, efuse_devno, NULL, "efuse");
	 devp = device_create(&efuse_class, NULL, efuse_devno, NULL, "efuse");
	 if (IS_ERR(devp)) {
		 printk(KERN_ERR "efuse: failed to create device node\n");
		 ret = PTR_ERR(devp);
		 goto error4;
	 }
	 printk(KERN_INFO "efuse: device %s created\n", EFUSE_DEVICE_NAME);
	 if(pdev->dev.platform_data)
		 devp->platform_data = pdev->dev.platform_data;
	 else
	 	devp->platform_data = NULL;
	 /* disable efuse encryption */
	 WRITE_CBUS_REG_BITS( EFUSE_CNTL4, CNTL1_AUTO_WR_ENABLE_OFF,
		 CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE );

	 return 0;

 error4:
	 cdev_del(&efuse_devp->cdev);
 error3:
	 kfree(efuse_devp);
 error2:
	 //class_destroy(efuse_clsp);
	 class_unregister(&efuse_class);
 error1:
	 unregister_chrdev_region(efuse_devno, 1);
 out:
	 return ret;
}

static int efuse_remove(struct platform_device *pdev)
{
	unregister_chrdev_region(efuse_devno, 1);
	//device_destroy(efuse_clsp, efuse_devno);
	device_destroy(&efuse_class, efuse_devno);
	cdev_del(&efuse_devp->cdev);
	kfree(efuse_devp);
	//class_destroy(efuse_clsp);
	class_unregister(&efuse_class);
	return 0;
}

static struct platform_driver efuse_driver = {
	.probe = efuse_probe,
	.remove = efuse_remove,
	.driver = {
		.name = EFUSE_DEVICE_NAME,
	.owner = THIS_MODULE,
	},
};

static int __init efuse_init(void)
{
	int ret = -1;
	ret = platform_driver_register(&efuse_driver);
	if (ret != 0) {
		printk(KERN_ERR "failed to register efuse driver, error %d\n", ret);
		return -ENODEV;
	}
	return ret;
}

static void __exit efuse_exit(void)
{
	platform_driver_unregister(&efuse_driver);
}

module_init(efuse_init);
module_exit(efuse_exit);

MODULE_DESCRIPTION("AMLOGIC eFuse driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bo Yang <bo.yang@amlogic.com>");

