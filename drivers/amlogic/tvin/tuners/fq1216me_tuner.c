/*
 * PHILIPS FQ1216ME-MK3 Tuner Device Driver
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


/* Standard Liniux Headers */
#include <linux/module.h>
#include <linux/i2c.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Local Headers */
#include "fq1216me_demod.h"


int fn(struct i2c_adapter adap, struct i2c_client tuner)
{
}
EXPORT_SYMBOL_GPL(fn);

static int fq1216me_tuner_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	//unsigned char data[16] = {0};
	int res = 0;

    pr_info("i2c client namne %s is probed at 0x%x\n", id->name, client->addr);
    pr_info("%s driver probed ok.\n", client->name);
	return 0;
}

static int fq1216me_tuner_remove(struct i2c_client *client)
{
    pr_info("%s driver removed ok.\n", client->name);
	return 0;
}

static const struct i2c_device_id fq1216me_tuner_id[] = {
	{ "fq1216me-tuner", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fq1216me_tuner_id);


static struct i2c_driver fq1216me_tuner_driver = {
	.driver = {
		.name = "fq1216me-tuner",
	},
	.probe		= fq1216me_tuner_probe,
	.remove		= fq1216me_tuner_remove,
	.id_table	= fq1216me_tuner_id,
};



static int __init fq1216me_tuner_init(void)
{
	return i2c_add_driver(&fq1216me_tuner_driver);
}

static void __exit f11216me_tuner_exit(void)
{
	i2c_del_driver(&fq1216me_tuner_driver);
}

MODULE_AUTHOR("Bobby Yang <bo.yang@amlogic.com>");
MODULE_DESCRIPTION("Philips FQ1216ME-MK3 tuner i2c device driver");
MODULE_LICENSE("GPL");

module_init(fq1216me_tuner_init);
module_exit(f11216me_tuner_exit);

