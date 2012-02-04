/*
 *
 *
 * Author: Peter Lin <peter.lin@amlogic.com>
 *
 * Copyright (C) 2011 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Linux headers */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>

/* Local headers */
#include <linux/aml_uevent_msg.h>

static void msg_dev_release(struct device *dev)
{
	kfree(dev);
}

static struct class msg_class = {
	.name = "ueventmsg",
	.dev_release = msg_dev_release,
};

static struct device *dev;

static int __init
msg_init(void)
{
	class_register(&msg_class);
	
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -EINVAL;
		
	dev_set_name(dev, "%s", "ueventmsg");
	dev->class = &msg_class;
	if (device_register(dev))
	{
		kfree(dev);
		return -EINVAL;
	}

	return 0;
}

static void __exit
msg_exit(void)
{
	device_unregister(dev);
	class_unregister(&msg_class);
}

int aml_send_msg(char *error_name, int error_flag)
{
	char name[256];
	char flag[256];
	char *envp[] = {name, flag, NULL};
						
	sprintf(name, "ERROR_NAME=%s", error_name);
	sprintf(flag, "ERROR_FLAG=%d", error_flag);

	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
	return 0;
}

subsys_initcall(msg_init);
module_exit(msg_exit);

MODULE_DESCRIPTION("Amlogic uevent message");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Lin <peter.lin@amlogic.com>");

