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


#ifndef __TVIN_VDIN_H
#define __TVIN_VDIN_H

#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/timer.h>

#include <linux/amports/vframe.h>
#include <linux/tvin/tvin.h>


//*** vdin device structure
typedef struct tvin_dec_ops_s {
    //int (*dec_start) (void);
    //int (*dec_stop) (void);
    int (*dec_run) (struct vframe_s *vf);
} tvin_dec_ops_t;

typedef struct vdin_dev_s {
    int                         index;
    unsigned int                flags;
    unsigned int                mem_start;
    unsigned int                mem_size;
    unsigned int                irq;
    unsigned int                addr_offset;  //address offset(vdin0/vdin1/...)
    unsigned int                hdmi_rgb;
    unsigned int                meas_th;
    unsigned int                meas_tv;
    struct tvin_para_s          para;

    struct cdev                 cdev;
    irqreturn_t (*vdin_isr) (int irq, void *dev_id);
    struct timer_list           timer;

    spinlock_t                  declock;
    struct tvin_dec_ops_s       *decop;
} vdin_dev_t;


extern void tvin_dec_register(struct vdin_dev_s *devp, struct tvin_dec_ops_s *op);
extern void tvin_dec_unregister(struct vdin_dev_s *devp);
extern void vdin_info_update(struct vdin_dev_s *devp, struct tvin_para_s *para);



#endif // __TVIN_VDIN_H

