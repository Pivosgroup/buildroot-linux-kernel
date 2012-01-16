/*
 * TVIN Tuner Bridge Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Linux Headers */
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
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Driver Headers */
#include "fq1216me.h"
#include "fq1216me_tuner.h"
#include "fq1216me_demod.h"
#include "tvin_tuner_device.h"


#define DEVICE_NAME "fq1216me"
#define DRIVER_NAME "fq1216me"
#define MODULE_NAME "fq1216me"
#define CLASS_NAME  "fq1216me"
#define DEVICE_COUNT    1



static int fq1216me_open(struct inode *inode, struct file *file)
{
    struct fq1216me_device_s *devp;

    /* Get the per-device structure that contains this cdev */
    devp = container_of(inode->i_cdev, struct fq1216me_device_s, cdev);
    file->private_data = devp;

    /* @todo */
    //fn(devp->adap, devp->tuenr);

    printk(KERN_INFO "device opened ok.\n");
    return 0;
}



static int fq1216me_release(struct inode *inode, struct file *file)
{
    /* @todo */
    printk(KERN_INFO "device released ok.\n");
    return 0;
}



static int fq1216me_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    /* @todo */

    return 0;
}

static struct file_operations fq1216me_fops = {
    .owner   = THIS_MODULE,
    .open    = fq1216me_open,
    .release = fq1216me_release,
    .ioctl   = fq1216me_ioctl,
};


static struct fq1216me_device_s * fq1216me_device_init(void)
{
    int ret = 0;
    struct fq1216me_device_s *devp;

    devp = kmalloc(sizeof(struct fq1216me_device_s), GFP_KERNEL);
    if (!devp)
    {
        printk(KERN_ERR "failed to allocate memory\n");
        return NULL;
    }

    ret = alloc_chrdev_region(&devp->devno, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ERR "failed to allocate major number\n");
        kfree(devp);
		return NULL;
	}

    devp->clsp = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(devp->clsp))
    {
        unregister_chrdev_region(devp->devno, 1);
        kfree(devp);
        ret = (int)PTR_ERR(devp->clsp);
        return NULL;
    }
    cdev_init(&devp->cdev, &fq1216me_fops);
    devp->cdev.owner = THIS_MODULE;
    ret = cdev_add(&devp->cdev, devp->devno, 1);
    if (ret) {
        printk(KERN_ERR "failed to add device\n");
        class_destroy(devp->clsp);
        unregister_chrdev_region(devp->devno, 1);
        kfree(devp);
        return NULL;
    }
    device_create(devp->clsp, NULL, devp->devno, NULL, DEVICE_NAME);
    return devp;
}

static void fq1216me_device_release(struct fq1216me_device_s *devp)
{
    cdev_del(&devp->cdev);
    class_destroy(devp->clsp);
    unregister_chrdev_region(devp->devno, 1);
    kfree(devp);
}

static struct i2c_board_info fq1216me_tuner_info = {
    .type = "fq1216me-tuner"
};

static const unsigned short fq1216me_tuner_addrs[] = {
    0xC0, 0xC2, 0xC4, 0xC6, I2C_CLIENT_END
};

static struct i2c_board_info fq1216me_demod_info = {
    I2C_BOARD_INFO("fq1216me-demod", 0x84)
};

static int fq1216me_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct fq1216me_device_s * devp;
    struct i2c_adapter *adapp;
    int i2c_nr = 0;

    devp = fq1216me_device_init();
    if (!devp)
    {
        pr_info("device init failed\n");
        return ret;
    }

    /* @todo: adapter id can be got from platfrom_data by bsp */

    adapp = i2c_get_adapter(i2c_nr);
    if (!adapp){
        printk(KERN_ERR "can't get i2c adapter %d.\n", i2c_nr);
        fq1216me_device_release(devp);
        return -1;
    }

    /* the tuner address may be one of the address list */
    devp->tuner = i2c_new_probed_device(adapp, &fq1216me_tuner_info, fq1216me_tuner_addrs);
    /* the demodulation only has fix address 0x84 in fq1216me */
    devp->demod = i2c_new_device(adapp, &fq1216me_demod_info);
    i2c_put_adapter(adapp);

    /* set the platform data so used later */
    platform_set_drvdata(pdev, devp);
    printk(KERN_ERR "driver probed ok.\n");
    return ret;
}

static int fq1216me_remove(struct platform_device *pdev)
{
    struct fq1216me_device_s *devp;

    devp = platform_get_drvdata(pdev);
    i2c_unregister_device(devp->demod);
    i2c_unregister_device(devp->tuner);
    fq1216me_device_release(devp);
    printk(KERN_ERR "driver removed ok.\n");

    return 0;
}

static struct platform_driver fq1216me_driver = {
    .probe      = fq1216me_probe,
    .remove     = fq1216me_remove,
    .driver     = {
        .name   = DRIVER_NAME,
    }
};


static int __init fq1216me_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&fq1216me_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register module, error %d\n", ret);
        return -ENODEV;
    }
    return ret;
}

static void __exit fq1216me_exit(void)
{
    platform_driver_unregister(&fq1216me_driver);
}

module_init(fq1216me_init);
module_exit(fq1216me_exit);

MODULE_DESCRIPTION("Amlogic FQ1216ME Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bo Yang <bo.yang@amlogic.com>");


