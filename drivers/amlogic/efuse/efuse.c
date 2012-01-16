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

#include "mach/am_regs.h"

#include "efuse.h"
#include "efuse_regs.h"

#define EFUSE_MODULE_NAME   "efuse"
#define EFUSE_DRIVER_NAME   "efuse"
#define EFUSE_DEVICE_NAME   "efuse"
#define EFUSE_CLASS_NAME    "efuse"

#define EFUSE_BITS             3072
#define EFUSE_BYTES            384  //(EFUSE_BITS/8)
#define EFUSE_DWORDS            96  //(EFUSE_BITS/32)

#define DOUBLE_WORD_BYTES        4

static unsigned long efuse_status;
#define EFUSE_IS_OPEN           (0x01)

typedef struct efuse_dev_s {
    struct cdev         cdev;
    unsigned int        flags;
} efuse_dev_t;

static efuse_dev_t *efuse_devp;
static struct class *efuse_clsp;
static dev_t efuse_devno;


static void __efuse_write_byte( unsigned long addr, unsigned long data );
static void __efuse_read_dword( unsigned long addr, unsigned long *data);


static void __efuse_write_byte( unsigned long addr, unsigned long data )
{
    unsigned long auto_wr_is_enabled = 0;

    if ( READ_CBUS_REG( EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_ENABLE_BIT ) )
    {
        auto_wr_is_enabled = 1;
    }
    else
    {
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

    while ( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_BUSY_BIT ) )
    {
        udelay(1);
    }

    /* if auto write wasn't enabled and we enabled it, then disable it upon exit */
    if (auto_wr_is_enabled == 0 )
    {
        WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_OFF,
            CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
    }
}

static void __efuse_read_dword( unsigned long addr, unsigned long *data )
{
    unsigned long auto_rd_is_enabled = 0;

    if( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ) )
    {
        auto_rd_is_enabled = 1;
    }
    else
    {
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

    while ( READ_CBUS_REG(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) )
    {
        udelay(1);
    }
    /* read the 32-bits value */
    ( *data ) = READ_CBUS_REG( EFUSE_CNTL2 );

    /* if auto read wasn't enabled and we enabled it, then disable it upon exit */
    if ( auto_rd_is_enabled == 0 )
    {
        WRITE_CBUS_REG_BITS( EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
            CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
    }

    printk(KERN_INFO "__efuse_read_dword: addr=%ld, data=0x%lx\n", addr, *data);
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


static ssize_t efuse_read( struct file *file, char __user *buf,
    size_t count, loff_t *ppos )
{
    unsigned long contents[EFUSE_DWORDS];
	unsigned pos = *ppos;
    unsigned long *pdw;
    unsigned int dwsize = (count + 3)/4;

	if (pos >= EFUSE_BYTES)
		return 0;

	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;

    printk( KERN_INFO "efuse_read: f_pos: %lld, ppos: %lld\n", file->f_pos, *ppos);

    memset(contents, 0, sizeof(contents));

	for (pdw = contents; dwsize-- > 0 && pos < EFUSE_BYTES; pos += 4, ++pdw)
		__efuse_read_dword(pos, pdw);

    if (copy_to_user(buf, contents, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

static ssize_t efuse_write( struct file *file, const char __user *buf,
    size_t count, loff_t *ppos )
{
 	unsigned char contents[EFUSE_BYTES];
	unsigned pos = *ppos;
	unsigned char *pc;

	if (pos >= EFUSE_BYTES)
		return 0;	/* Past EOF */

	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;

    printk( KERN_INFO "efuse_write: f_pos: %lld, ppos: %lld\n", file->f_pos, *ppos);

	if (copy_from_user(contents, buf, count))
		return -EFAULT;

	for (pc = contents; count--; ++pos, ++pc)
		__efuse_write_byte(pos, *pc);

	*ppos = pos;

	return pc - contents;
}

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

	if (newpos < 0) return -EINVAL;
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

static int __init efuse_init(void)
{
    int ret;
    struct device *devp;

    ret = alloc_chrdev_region(&efuse_devno, 0, 1, EFUSE_DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ERR "efuse: failed to allocate major number\n");
        ret = -ENODEV;
        goto out;
	}

    efuse_clsp = class_create(THIS_MODULE, EFUSE_CLASS_NAME);
    if (IS_ERR(efuse_clsp)) {
        ret = PTR_ERR(efuse_clsp);
        goto error1;
    }

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

    devp = device_create(efuse_clsp, NULL, efuse_devno, NULL, "efuse");
    if (IS_ERR(devp)) {
        printk(KERN_ERR "efuse: failed to create device node\n");
        ret = PTR_ERR(devp);
        goto error4;
    }
    printk(KERN_INFO "efuse: device %s created\n", EFUSE_DEVICE_NAME);


    /* disable efuse encryption */
    WRITE_CBUS_REG_BITS( EFUSE_CNTL4, CNTL1_AUTO_WR_ENABLE_OFF,
        CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE );

    return 0;

error4:
    cdev_del(&efuse_devp->cdev);
error3:
    kfree(efuse_devp);
error2:
    class_destroy(efuse_clsp);
error1:
    unregister_chrdev_region(efuse_devno, 1);
out:
    return ret;
}

static void __exit efuse_exit(void)
{
    unregister_chrdev_region(efuse_devno, 1);
    device_destroy(efuse_clsp, efuse_devno);
    cdev_del(&efuse_devp->cdev);
    kfree(efuse_devp);
    class_destroy(efuse_clsp);
    return;
}

module_init(efuse_init);
module_exit(efuse_exit);

MODULE_DESCRIPTION("AMLOGIC eFuse driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bo Yang <bo.yang@amlogic.com>");

