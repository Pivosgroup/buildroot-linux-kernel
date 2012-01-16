/*
 * VDIN driver
 *
 * Author: Lin Xu <lin.xu@amlogic.com>
 *         Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Linux headers */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
//#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

/* Amlogic headers */
#include <linux/amports/canvas.h>
#include <mach/am_regs.h>
#include <linux/amports/vframe.h>
#include <linux/tvin/tvin.h>

/* TVIN headers */
#include "tvin_global.h"
#include "tvin_format_table.h"
#include "tvin_notifier.h"
#include "vdin_regs.h"
#include "vdin.h"
#include "vdin_vf.h"
#include "vdin_ctl.h"
#include "tvin_debug.h"


#define VDIN_NAME               "vdin"
#define VDIN_DRIVER_NAME        "vdin"
#define VDIN_MODULE_NAME        "vdin"
#define VDIN_DEVICE_NAME        "vdin"
#define VDIN_CLASS_NAME         "vdin"

#if defined(CONFIG_ARCH_MESON)
#define VDIN_COUNT              1
#elif defined(CONFIG_ARCH_MESON2)
#define VDIN_COUNT              2
#endif

#define VDIN_PUT_INTERVAL       1    //(HZ/100)   //10ms, #define HZ 100

#define INVALID_VDIN_INPUT      0xffffffff

static dev_t vdin_devno;
static struct class *vdin_clsp;

static vdin_dev_t *vdin_devp[VDIN_COUNT];

extern ssize_t vdin_dbg_store(struct device *dev, struct device_attribute *attr, const char * buf, size_t count)

/* create debug attribute sys file used to debug registers */
DEVICE_ATTR(debug, S_IWUSR | S_IRUGO, NULL, vdin_dbg_store);



void tvin_dec_register(struct vdin_dev_s *devp, struct tvin_dec_ops_s *op)
{
    ulong flags;

    if (devp->decop)
        tvin_dec_unregister(devp);

    spin_lock_irqsave(&devp->declock, flags);

    devp->decop = op;

    spin_unlock_irqrestore(&devp->declock, flags);
}

void tvin_dec_unregister(struct vdin_dev_s *devp)
{
    ulong flags;
    spin_lock_irqsave(&devp->declock, flags);

    devp->decop = NULL;

    spin_unlock_irqrestore(&devp->declock, flags);
}

void vdin_info_update(struct vdin_dev_s *devp, struct tvin_para_s *para)
{
    //check decoder signal status
    if((para->status != TVIN_SIG_STATUS_STABLE) || (para->fmt == TVIN_SIG_FMT_NULL))
        return;

    devp->para.status = para->status;
    devp->para.fmt = para->fmt;

    //write vdin registers
    vdin_set_all_regs(devp);

}

EXPORT_SYMBOL(vdin_info_update);

static void vdin_put_timer_func(unsigned long arg)
{
    struct timer_list *timer = (struct timer_list *)arg;

    while (!vfq_empty_recycle()) {
        vframe_t *vf = vfq_pop_recycle();
        vfq_push_newframe(vf);
    }

    tvin_check_notifier_call(TVIN_EVENT_INFO_CHECK, NULL);

    timer->expires = jiffies + VDIN_PUT_INTERVAL;
    add_timer(timer);
}

static void vdin_start_dec(struct vdin_dev_s *devp)
{
    vdin_vf_init();
    vdin_reg_vf_provider();


    vdin_set_default_regmap(devp);

    tvin_dec_notifier_call(TVIN_EVENT_DEC_START, devp);

    devp->timer.data = (ulong) &devp->timer;
    devp->timer.function = vdin_put_timer_func;
    devp->timer.expires = jiffies + VDIN_PUT_INTERVAL * 5;
    add_timer(&devp->timer);
}

static void vdin_stop_dec(struct vdin_dev_s *devp)
{
    vdin_unreg_vf_provider();
    //load default setting for vdin
    vdin_set_default_regmap(devp);
    tvin_dec_notifier_call(TVIN_EVENT_DEC_STOP, devp);
}


static irqreturn_t vdin_isr(int irq, void *dev_id)
{
    struct vdin_dev_s *devp = (struct vdin_dev_s *)dev_id;
    vframe_t *vf;

    /* pop out a new frame to be displayed */
    vf = vfq_pop_newframe();
    if(vf == NULL)
    {
        pr_dbg("vdin_isr: don't get newframe \n");
    }
    else
    {
        vf->type = INVALID_VDIN_INPUT;
        devp->decop->dec_run(vf);
        vdin_set_vframe_prop_info(vf, devp);

        //If vf->type ( --reture value )is INVALID_VDIN_INPUT, the current field is error
        if(vf->type == INVALID_VDIN_INPUT)
        {
            /* mpeg12 used spin_lock_irqsave(), @todo... */
            vfq_push_recycle(vf);
            pr_dbg("decode data is error, skip the feild data \n");
        }
        else    //do buffer managerment, and send info into video display, please refer to vh264 decode
        {
            vfq_push_display(vf); /* push to display */
        }
    }

    return IRQ_HANDLED;
}


static int vdin_open(struct inode *inode, struct file *file)
{
    vdin_dev_t *devp;

    /* Get the per-device structure that contains this cdev */
    devp = container_of(inode->i_cdev, vdin_dev_t, cdev);
    file->private_data = devp;

	if (devp->index >= VDIN_COUNT)
        return -ENXIO;

    return 0;
}

static int vdin_release(struct inode *inode, struct file *file)
{
    vdin_dev_t *devp = file->private_data;
    file->private_data = NULL;

    printk(KERN_INFO "vdin: device %d release ok.\n", devp->index);
    return 0;
}

static inline bool vdin_port_valid(enum tvin_port_e port)
{
    bool ret = false;

#if defined(CONFIG_ARCH_MESON)
    switch (port>>8)
    {
        case 0x01: // mpeg
        case 0x02: // 656
        case 0x80: // dvin
            ret = true;
            break;
        default:
            break;
    }
#elif defined(CONFIG_ARCH_MESON2)
    switch (port>>8)
    {
        case 0x01: // mpeg
        case 0x02: // 656
        case 0x04: // VGA
        case 0x08: // COMPONENT
        case 0x10: // CVBS
        case 0x20: // SVIDEO
        case 0x40: // hdmi
        case 0x80: // dvin
            ret = true;
        default:
            break;
    }
#endif
    return ret;
}

static int vdin_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    vdin_dev_t *devp;
    void __user *argp = (void __user *)arg;

	if (_IOC_TYPE(cmd) != VDIN_IOC_MAGIC) {
		return -EINVAL;
	}

    devp = container_of(inode->i_cdev, vdin_dev_t, cdev);

    switch (cmd)
    {
        case VDIN_IOC_START_DEC:
        {
            struct tvin_para_s para = {TVIN_PORT_BT656, TVIN_SIG_FMT_BT656IN_576I, TVIN_SIG_STATUS_NULL, 0x85100000, 0};
 //           pr_dbg(" command is VDIN_IOC_START_DEC. \n");
            if (copy_from_user(&para, argp, sizeof(struct tvin_para_s)))
            {
                pr_err(" vdin start decode para is error. \n ");
                ret = -EFAULT;
                break;
            }

            if (!vdin_port_valid(para.port))
            {
                ret = -EFAULT;
                break;
            }

            //init vdin signal info
            devp->para.port = para.port;
            devp->para.fmt = TVIN_SIG_FMT_NULL;
            devp->para.status = TVIN_SIG_STATUS_NULL;
            devp->para.cap_addr = 0x85100000;
            devp->para.cap_flag = 0;
            vdin_start_dec(devp);
#if defined(CONFIG_ARCH_MESON2)
            vdin_set_meas_mux(devp);
#endif
            break;
        }

        case VDIN_IOC_STOP_DEC:
        {
            vdin_stop_dec(devp);
            break;
        }

        case VDIN_IOC_G_PARA:
        {
		    if (copy_to_user((void __user *)arg, &devp->para, sizeof(struct tvin_para_s)))
		    {
               ret = -EFAULT;
		    }
            break;
        }

        case VDIN_IOC_S_PARA:
        {
            struct tvin_para_s *para = NULL;
            if (copy_from_user(para, argp, sizeof(struct tvin_para_s)))
		    {
                ret = -EFAULT;
                break;
            }
            //get tvin port selection and other setting
            devp->para.cap_flag = para->cap_flag;
            if(devp->para.port != para->port)
            {
                //to do

		    }
            break;
        }

#if 1
        case VDIN_IOC_DEBUG:
        {
            struct vdin_regs_s data;
            if (copy_from_user(&data, argp, sizeof(struct vdin_regs_s)))
            {
                ret = -EFAULT;
            }
            else
            {
                vdin_set_regs(&data, devp->addr_offset);

                if (!(data.mode)) // read
                {
                    if (copy_to_user(argp, &data, sizeof(struct vdin_regs_s)))
                    {
                        ret = -EFAULT;
                    }
                }
            }
            break;
        }
#endif

        default:
            ret = -ENOIOCTLCMD;
            break;
    }

    return ret;
}


static ssize_t store_dbg(struct device * dev, struct device_attribute *attr,
                        const char * buf, size_t count) {
    char tmpbuf[128];
    int i=0;
    unsigned int adr;
    unsigned int value=0;
    while((buf[i])&&(buf[i]!=',')&&(buf[i]!=' ')){
        tmpbuf[i]=buf[i];
        i++;
    }
    tmpbuf[i]=0;
    if(tmpbuf[0]=='w'){
        adr=simple_strtoul(tmpbuf+2, NULL, 16);
        value=simple_strtoul(buf+i+1, NULL, 16);
        if(buf[1]=='c'){
            WRITE_MPEG_REG(adr, value);
            printk("write %x to %s reg[%x]\n",value, "CBUS", adr);
        }
    }
    else if(tmpbuf[0]=='r'){
        adr=simple_strtoul(tmpbuf+2, NULL, 16);
        if(buf[1]=='c'){
            value = READ_MPEG_REG(adr);
            printk("%s reg[%x]=%x\n", "CBUS", adr, value);
        }

    }
    return 16;
}

static DEVICE_ATTR(debug, S_IWUSR | S_IRUGO, NULL, store_dbg);


static struct file_operations vdin_fops = {
    .owner   = THIS_MODULE,
    .open    = vdin_open,
    .release = vdin_release,
    .ioctl   = vdin_ioctl,
};


static int vdin_probe(struct platform_device *pdev)
{
    int ret;
    int i;
    struct device *devp;
    struct resource *res;
    char name[12];

    ret = alloc_chrdev_region(&vdin_devno, 0, VDIN_COUNT, VDIN_NAME);
	if (ret < 0) {
		printk(KERN_ERR "vdin: failed to allocate major number\n");
		return 0;
	}

    vdin_clsp = class_create(THIS_MODULE, VDIN_NAME);
    if (IS_ERR(vdin_clsp))
    {
        unregister_chrdev_region(vdin_devno, VDIN_COUNT);
        return PTR_ERR(vdin_clsp);
    }

    for (i = 0; i < VDIN_COUNT; ++i)
    {
        /* allocate memory for the per-device structure */
        vdin_devp[i] = kmalloc(sizeof(struct vdin_dev_s), GFP_KERNEL);
        if (!vdin_devp[i])
        {
            printk(KERN_ERR "vdin: failed to allocate memory for vdin device\n");
            return -ENOMEM;
        }
        vdin_devp[i]->index = i;

        vdin_devp[i]->declock = SPIN_LOCK_UNLOCKED;
        vdin_devp[i]->decop = NULL;

        /* connect the file operations with cdev */
        cdev_init(&vdin_devp[i]->cdev, &vdin_fops);
        vdin_devp[i]->cdev.owner = THIS_MODULE;
        /* connect the major/minor number to the cdev */
        ret = cdev_add(&vdin_devp[i]->cdev, (vdin_devno + i), 1);
    	if (ret) {
    		printk(KERN_ERR "vdin: failed to add device\n");
            /* @todo do with error */
    		return ret;
    	}
        /* create /dev nodes */
        devp = device_create(vdin_clsp, NULL, MKDEV(MAJOR(vdin_devno), i),
                            NULL, "vdin%d", i);
        if (IS_ERR(devp)) {
            printk(KERN_ERR "vdin: failed to create device node\n");
            class_destroy(vdin_clsp);
            /* @todo do with error */
            return PTR_ERR(devp);;
    	}


        /* get device memory */
        res = platform_get_resource(pdev, IORESOURCE_MEM, i);
        if (!res) {
            printk(KERN_ERR "vdin: can't get memory resource\n");
            return -EFAULT;
        }
        vdin_devp[i]->mem_start = res->start;
        vdin_devp[i]->mem_size  = res->end - res->start + 1;
        pr_dbg(" vdin[%d] memory start addr is %x, mem_size is %x . \n",i,
            vdin_devp[i]->mem_start,vdin_devp[i]->mem_size);

        /* get device irq */
        res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
        if (!res) {
            printk(KERN_ERR "vdin: can't get memory resource\n");
            return -EFAULT;
        }
        vdin_devp[i]->irq = res->start;

        vdin_devp[i]->addr_offset = 0;

        sprintf(name, "vdin%d-irq", i);
        /* register vdin irq */
        ret = request_irq(vdin_devp[i]->irq, vdin_isr, IRQF_SHARED, name, (void *)vdin_devp[i]);
        if (ret) {
            printk(KERN_ERR "vdin: irq regist error.\n");
            return -ENOENT;
        }

        /* init timer */
        init_timer(&vdin_devp[i]->timer);
        vdin_devp[i]->timer.data = (ulong) &vdin_devp[i]->timer;
        vdin_devp[i]->timer.function = vdin_put_timer_func;
        vdin_devp[i]->timer.expires = jiffies + VDIN_PUT_INTERVAL;
        add_timer(&vdin_devp[i]->timer);
    }

    device_create_file(pdev->dev, &dev_attr_debug);

    printk(KERN_INFO "vdin: driver initialized ok\n");
    return 0;
}

static int vdin_remove(struct platform_device *pdev)
{
    int i = 0;

    device_remove_file(pdev->dev, &dev_attr_debug);

    unregister_chrdev_region(vdin_devno, VDIN_COUNT);
    for (i = 0; i < VDIN_COUNT; ++i)
    {
        del_timer_sync(&vdin_devp[i]->timer);
        free_irq(vdin_devp[i]->irq,(void *)vdin_devp[i]);
        device_destroy(vdin_clsp, MKDEV(MAJOR(vdin_devno), i));
        cdev_del(&vdin_devp[i]->cdev);
        kfree(vdin_devp[i]);
    }
    class_destroy(vdin_clsp);

    printk(KERN_ERR "vdin: driver removed ok.\n");
    return 0;
}

static struct platform_driver vdin_driver = {
    .probe      = vdin_probe,
    .remove     = vdin_remove,
    .driver     = {
        .name   = VDIN_DRIVER_NAME,
    }
};

static int __init vdin_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&vdin_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register vdin module, error %d\n", ret);
        return -ENODEV;
    }
    return ret;
}

static void __exit vdin_exit(void)
{
    platform_driver_unregister(&vdin_driver);
}

module_init(vdin_init);
module_exit(vdin_exit);

MODULE_DESCRIPTION("AMLOGIC VDIN driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xu Lin <lin.xu@amlogic.com>");

