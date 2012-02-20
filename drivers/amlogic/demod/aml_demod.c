#include <linux/init.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/am_regs.h>

#include <asm/fiq.h>
#include <asm/uaccess.h>
#include "aml_demod.h"
#include "demod_func.h"

#include <linux/slab.h>

#define DRIVER_NAME "aml_demod"
#define MODULE_NAME "aml_demod"
#define DEVICE_NAME "aml_demod"

const char aml_demod_dev_id[] = "aml_demod";

#ifndef CONFIG_AM_DEMOD_DVBAPI
static struct aml_demod_i2c demod_i2c;
static struct aml_demod_sta demod_sta;
#else 
  extern struct aml_demod_i2c demod_i2c;
  extern struct aml_demod_sta demod_sta;
#endif

static DECLARE_WAIT_QUEUE_HEAD(lock_wq);

static irqreturn_t aml_demod_isr(int irq, void *dev_id)
{
    if (demod_sta.dvb_mode == 0) {
	//dvbc_isr(&demod_sta);
	/*if(dvbc_isr_islock())*/{
		//printk("sync4\n");
		if(waitqueue_active(&lock_wq))
			wake_up_interruptible(&lock_wq);
	}
    }
    else {
	dvbt_isr(&demod_sta);
    }

    return IRQ_HANDLED;
}

static int aml_demod_open(struct inode *inode, struct file *file)
{
    printk("Amlogic Demod DVB-T/C Open\n");    
    return 0;
}

static int aml_demod_release(struct inode *inode, struct file *file)

{
    printk("Amlogic Demod DVB-T/C Release\n");    
    return 0;
}

static int amdemod_islock(void)
{
	struct aml_demod_sts demod_sts;
	if(demod_sta.dvb_mode == 0) {
		dvbc_status(&demod_sta, &demod_i2c, &demod_sts);
		return demod_sts.ch_sts&0x1;
	} else if(demod_sta.dvb_mode == 1) {
		dvbt_status(&demod_sta, &demod_i2c, &demod_sts);
		return demod_sts.ch_sts>>12&0x1;
	}
	return 0;
}

static int aml_demod_ioctl(struct inode *inode, struct file *file,
                        unsigned int cmd, ulong arg)
{
    // printk("AML_DEMOD_SET_SYS     0x%x\n", AML_DEMOD_SET_SYS    );
    // printk("AML_DEMOD_GET_SYS     0x%x\n", AML_DEMOD_GET_SYS    );
    // printk("AML_DEMOD_TEST        0x%x\n", AML_DEMOD_TEST       );
    // printk("AML_DEMOD_DVBC_SET_CH 0x%x\n", AML_DEMOD_DVBC_SET_CH);
    // printk("AML_DEMOD_DVBC_GET_CH 0x%x\n", AML_DEMOD_DVBC_GET_CH);
    // printk("AML_DEMOD_DVBC_TEST   0x%x\n", AML_DEMOD_DVBC_TEST  );
    // printk("AML_DEMOD_DVBT_SET_CH 0x%x\n", AML_DEMOD_DVBT_SET_CH);
    // printk("AML_DEMOD_DVBT_GET_CH 0x%x\n", AML_DEMOD_DVBT_GET_CH);
    // printk("AML_DEMOD_DVBT_TEST   0x%x\n", AML_DEMOD_DVBT_TEST  );
    // printk("AML_DEMOD_SET_REG     0x%x\n", AML_DEMOD_SET_REG    );
    // printk("AML_DEMOD_GET_REG     0x%x\n", AML_DEMOD_GET_REG    );

    switch (cmd) {
    case AML_DEMOD_SET_SYS : 
	// printk("Ioctl Demod Set System\n");    	
	demod_set_sys(&demod_sta, &demod_i2c, (struct aml_demod_sys *)arg);
	break;
	
    case AML_DEMOD_GET_SYS :
	// printk("Ioctl Demod Get System\n");    	
	demod_get_sys(&demod_i2c, (struct aml_demod_sys *)arg);
	break;

    case AML_DEMOD_TEST :
	// printk("Ioctl Demod Test\n");    	
	demod_msr_clk(13);
	demod_msr_clk(14);
	demod_calc_clk(&demod_sta);
	break;

    case AML_DEMOD_TURN_ON :
	demod_turn_on(&demod_sta, (struct aml_demod_sys *)arg);
	break;

    case AML_DEMOD_TURN_OFF :
	demod_turn_off(&demod_sta, (struct aml_demod_sys *)arg);
	break;
	
    case AML_DEMOD_DVBC_SET_CH :
	// printk("Ioctl DVB-C Set Channel\n");    	
	dvbc_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_dvbc *)arg);
	{
		int ret;
		ret = wait_event_interruptible_timeout(lock_wq, amdemod_islock(), 2*HZ);
		if(!ret)	printk(">>> wait lock timeout.\n");
	}
	break;

    case AML_DEMOD_DVBC_GET_CH :
	// printk("Ioctl DVB-C Get Channel\n");
	dvbc_status(&demod_sta, &demod_i2c, (struct aml_demod_sts *)arg);
	break;

    case AML_DEMOD_DVBC_TEST :
	// printk("Ioctl DVB-C Test\n");    	
	dvbc_get_test_out(0xb, 1000, (u32 *)arg);
	break;

    case AML_DEMOD_DVBT_SET_CH :
	// printk("Ioctl DVB-T Set Channel\n");    	
	dvbt_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_dvbt *)arg);
	break;

    case AML_DEMOD_DVBT_GET_CH :
	// printk("Ioctl DVB-T Get Channel\n");
	dvbt_status(&demod_sta, &demod_i2c, (struct aml_demod_sts *)arg);
	break;

    case AML_DEMOD_DVBT_TEST :
	// printk("Ioctl DVB-T Test\n"); 
	dvbt_get_test_out(0x1e, 1000, (u32 *)arg);
	break;

    case AML_DEMOD_SET_REG :
	// printk("Ioctl Set Register\n"); 
   	demod_set_reg((struct aml_demod_reg *)arg);
	break;

    case AML_DEMOD_GET_REG :
	// printk("Ioctl Get Register\n");    	
   	demod_get_reg((struct aml_demod_reg *)arg);
	break;
            
    case AML_DEMOD_SET_REGS : {
		int i;
		struct aml_demod_regs *regs = ((struct aml_demod_regs *)arg);
		// printk("Ioctl Set Registers\n"); 

		if(regs->mode==10) { /*i2c*/
		    struct i2c_msg msg;
		    u8 v[8], *val=v;
		    int ret=0;
		    if(regs->addr_len>4)
				return -EINVAL;
			
		    if((regs->n + regs->addr_len)>8)
			val = kmalloc((regs->n + regs->addr_len), GFP_KERNEL);
			
		    for(i=regs->addr_len-1; i>=0; i--)
			    val[i] = (regs->addr>>(i*8))&0xff;
		    for(i=0;i<regs->n;i++)
			    val[regs->addr_len+i] = regs->vals[i];

		    msg.addr = demod_i2c.addr;
		    msg.flags = 0;
		    msg.len = (regs->n + regs->addr_len);
		    msg.buf = val;

		    printk(">> i2c[%d][0x%x] W[", demod_i2c.i2c_id, demod_i2c.addr);
		    for(i=0;i<(regs->n + regs->addr_len);i++)
			    printk("0x%x ", val[i]);
		    printk("\n");

		    //ret = am_demod_i2c_xfer(&demod_i2c, &msg, 1);

		    if(val!=v)
			kfree(val);

		    return ret;
			
		} else {

			for(i=0;i<regs->n;i++) {
				struct aml_demod_reg reg;
				reg.mode = regs->mode;
				reg.addr = regs->addr+i;
				reg.rw = regs->rw;
				reg.val = regs->vals[i];
				printk("m:%d a:0x%08x rw:%d val:0x%08x\n", reg.mode, reg.addr, reg.rw, reg.val);
			   	//demod_set_reg(&reg);
			}
		}
    	}
	break;

    case AML_DEMOD_GET_REGS :{
		int i, j;
		struct aml_demod_regs *regs = ((struct aml_demod_regs *)arg);
		// printk("Ioctl Get Registers\n");    	
		if(regs->mode==10) {
		    struct i2c_msg msg[2];
		    u8 a[4], *addr=a;
		    u8 *val=(u8 *)&regs->vals;
		    int ret=0;
		    if(regs->addr_len>4)
				return -EINVAL;
			
		    for(j=0,i=regs->addr_len-1; i>=0; i--,j++)
			    addr[j] = (regs->addr>>(i*8))&0xff;

		    msg[0].addr = demod_i2c.addr;
		    msg[0].flags = 0;
		    msg[0].len = regs->addr_len;
		    msg[0].buf = addr;

		    msg[1].addr = demod_i2c.addr;
		    msg[1].flags = I2C_M_RD;
		    msg[1].len = regs->n;
		    msg[1].buf = val;
			
		    printk(">> i2c[%d][0x%x] R[", demod_i2c.i2c_id, demod_i2c.addr);
		    for(i=0;i<regs->addr_len;i++)
			    printk("0x%x ", addr[i]);
		    printk("]\n");

		    ret = am_demod_i2c_xfer(&demod_i2c, msg, 2);

		    printk(">> i2c[%d][0x%x] R=", demod_i2c.i2c_id, demod_i2c.addr);
		    for(i=0;i<regs->n ;i++)
			    printk("0x%x ", val[i]);
		    printk("\n");
			
		    return ret;

		} else {
			printk("m:%d a:0x%08x al:%d n:%d rw:%d\n", regs->mode, regs->addr, regs->addr_len, regs->n, regs->rw);
			for(i=0;i<regs->n;i++) {
				struct aml_demod_reg reg;
				reg.mode = regs->mode;
				reg.addr = regs->addr+i;
				reg.rw = regs->rw;
				reg.val = 0;
				demod_get_reg(&reg);
				printk("v:0x%08x\n", reg.val);
				regs->vals[i] = reg.val;
			}
		}
    	}
	break;
	
    default:
		printk("???? 0x%X\n", cmd);
		printk("AML_DEMOD_GET_REGS=0x%08X\n", AML_DEMOD_GET_REGS);
		printk("AML_DEMOD_SET_REGS=0x%08X\n", AML_DEMOD_SET_REGS);
	return -EINVAL;
    }

    return 0;
}

const static struct file_operations aml_demod_fops = {
    .owner    = THIS_MODULE,
    .open     = aml_demod_open,
    .release  = aml_demod_release,
    .ioctl    = aml_demod_ioctl,
};

static ssize_t aml_demod_info(struct class *cla, 
			      struct class_attribute *attr, 
			      char *buf)
{
    return 0;
}

static struct class_attribute aml_demod_class_attrs[] = {
    __ATTR(info,       
	   S_IRUGO | S_IWUSR, 
	   aml_demod_info,    
	   NULL),
    __ATTR_NULL
};

static struct class aml_demod_class = {
    .name = "aml_demod",
    .class_attrs = aml_demod_class_attrs,
};

static struct device *aml_demod_dev;

#ifdef CONFIG_AM_DEMOD_DVBAPI
int aml_demod_init(void)
#else
static int __init aml_demod_init(void)
#endif
{
    int r = 0;

    printk("Amlogic Demod DVB-T/C DebugIF Init\n");    

    init_waitqueue_head(&lock_wq);

    /* hook demod isr */
    r = request_irq(INT_DEMOD, &aml_demod_isr,
                    IRQF_SHARED, "aml_demod",
                    (void *)aml_demod_dev_id);

    if (r) {
        printk("aml_demod irq register error.\n");
        r = -ENOENT;
        goto err0;
    }

    /* sysfs node creation */
    r = class_register(&aml_demod_class);
    if (r) {
        printk("create aml_demod class fail\r\n");
        goto err1;
    }

    /* create video device */
    r = register_chrdev(AML_DEMOD_MAJOR, "aml_demod", &aml_demod_fops);
    if (r < 0) {
        printk("Can't register major for aml_demod device\n");
        goto err2;
    }

    aml_demod_dev = device_create(&aml_demod_class, NULL,
				  MKDEV(AML_DEMOD_MAJOR, 0), NULL,
				  DEVICE_NAME);

    if (IS_ERR(aml_demod_dev)) {
        printk("Can't create aml_demod device\n");
        goto err3;
    }

    return (0);

  err3:
    unregister_chrdev(AML_DEMOD_MAJOR, DEVICE_NAME);

  err2:
    free_irq(INT_DEMOD, (void *)aml_demod_dev_id);

  err1:
    class_unregister(&aml_demod_class);
    
  err0:
    return r;
}

#ifdef CONFIG_AM_DEMOD_DVBAPI
void aml_demod_exit(void)
#else
static void __exit aml_demod_exit(void)
#endif
{
    printk("Amlogic Demod DVB-T/C DebugIF Exit\n");    

    device_destroy(&aml_demod_class, MKDEV(AML_DEMOD_MAJOR, 0));

    unregister_chrdev(AML_DEMOD_MAJOR, DEVICE_NAME);

    free_irq(INT_DEMOD, (void *)aml_demod_dev_id);

    class_unregister(&aml_demod_class);
}

#ifndef CONFIG_AM_DEMOD_DVBAPI
module_init(aml_demod_init);
module_exit(aml_demod_exit);

MODULE_LICENSE("GPL");
//MODULE_AUTHOR(DRV_AUTHOR);
//MODULE_DESCRIPTION(DRV_DESC);
#endif

