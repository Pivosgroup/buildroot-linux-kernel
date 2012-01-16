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


#ifndef __TVIN_TUNER_FQ1216ME_H
#define __TVIN_TUNER_FQ1216ME_H

/* Standard Liniux Headers */
#include <linux/module.h>
#include <linux/i2c.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Local Headers */
#include "tvin_tuner_device.h"

typedef struct fq1216me_device_s {
    struct class            *clsp;
    dev_t                   devno;
    struct cdev             cdev;

	struct i2c_adapter      *adap;
    struct i2c_client       *tuner;
    struct i2c_client       *demod;

    /* reserved for futuer abstract */
    struct tvin_tuner_ops_s *tops;
} fq1216me_device_s;


#endif //__TVIN_TUNER_FQ1216ME_H

