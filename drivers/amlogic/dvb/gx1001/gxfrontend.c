/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : gxfrontend.c
**
**  comment:
**        Driver for GX1001 demodulator
**  author :
**	    jianfeng_wang@amlogic
**  version :
**	    v1.0	 09/03/04
*****************************************************************/

/*
    Driver for GX1001 demodulator
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include <mach/am_regs.h>
#endif
#include <linux/i2c.h>
#include <linux/gpio.h>
#include "gx1001.h"

#if 1
#define pr_dbg(fmt, args...) printk(KERN_DEBUG "DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "DVB: " fmt, ## args)

static int gx1001_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct gx1001_state *state = fe->demodulator_priv;

	return 0;
}

static int gx1001_init(struct dvb_frontend *fe)
{
	struct gx1001_state *state = fe->demodulator_priv;
	
	//GX_Init_Chip();
	pr_dbg("=========================demod init\r\n");
	demod_init(state);
	msleep(200);
	return 0;
}

static int gx1001_sleep(struct dvb_frontend *fe)
{
	struct gx1001_state *state = fe->demodulator_priv;

	GX_Set_Sleep(state, 1);

	return 0;
}

static int gx1001_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct gx1001_state *state = fe->demodulator_priv;
	unsigned char s=0;
	
	demod_check_locked(state, &s);
	if(s==1)
	{
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		*status = FE_TIMEDOUT;
	}
	
	return  0;
}

static int gx1001_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct gx1001_state *state = fe->demodulator_priv;
	
	demod_get_signal_errorate(state, ber);
	return 0;
}

static int gx1001_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct gx1001_state *state = fe->demodulator_priv;
	uint   rec_strength;
	
	demod_get_signal_strength(state, &rec_strength);
	*strength=rec_strength;
	return 0;
}

static int gx1001_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct gx1001_state *state = fe->demodulator_priv;
	uint   rec_snr ;
	
	demod_get_signal_quality(state, &rec_snr) ;
	*snr = rec_snr;//>>16;		
	return 0;
}

static int gx1001_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int gx1001_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct gx1001_state *state = fe->demodulator_priv;
	
	demod_connect(state, p->frequency,p->u.qam.modulation,p->u.qam.symbol_rate);
	state->freq=p->frequency;
	state->mode=p->u.qam.modulation ;
	state->symbol_rate=p->u.qam.symbol_rate; //these data will be writed to eeprom
	
	pr_dbg("gx1001=>frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;
}

static int gx1001_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct gx1001_state *state = fe->demodulator_priv;
	
	p->frequency=state->freq;
	p->u.qam.modulation=state->mode;
	p->u.qam.symbol_rate=state->symbol_rate;
	
	return 0;
}

static void gx1001_release(struct dvb_frontend *fe)
{
	struct gx1001_state *state = fe->demodulator_priv;
	
	demod_deinit(state);
	kfree(state);
}

static struct dvb_frontend_ops gx1001_ops;

struct dvb_frontend *gx1001_attach(const struct aml_fe_config *config)
{
	struct gx1001_state *state = NULL;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct gx1001_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;
	
	/* create dvb_frontend */
	memcpy(&state->fe.ops, &gx1001_ops, sizeof(struct dvb_frontend_ops));
	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(gx1001_attach);
static struct dvb_frontend_ops gx1001_ops = {

	.info = {
		 .name = "AMLOGIC GX1001  DVB-C",
		 .type = FE_QAM,
		 .frequency_min = 1000,
		 .frequency_max = 1000000,
		 .frequency_stepsize = 62500,
		 .symbol_rate_min = 870000,
		 .symbol_rate_max = 11700000,
		 .caps = FE_CAN_QAM_16 | FE_CAN_QAM_32 | FE_CAN_QAM_64 |
		 FE_CAN_QAM_128 | FE_CAN_QAM_256 | FE_CAN_FEC_AUTO},

	.release = gx1001_release,

	.init = gx1001_init,
	.sleep = gx1001_sleep,
	.i2c_gate_ctrl = gx1001_i2c_gate_ctrl,

	.set_frontend = gx1001_set_frontend,
	.get_frontend = gx1001_get_frontend,

	.read_status = gx1001_read_status,
	.read_ber = gx1001_read_ber,
	.read_signal_strength =gx1001_read_signal_strength,
	.read_snr = gx1001_read_snr,
	.read_ucblocks = gx1001_read_ucblocks,
};

static int __init gxfrontend_init(void)
{

	pr_dbg("gxfrontend_init\n");
	return 0;
}


static void __exit gxfrontend_exit(void)
{
	pr_dbg("gxfrontend_exit");
}

module_init(gxfrontend_init);
module_exit(gxfrontend_exit);


MODULE_DESCRIPTION("GX1001 DVB-C Demodulator driver");
MODULE_AUTHOR("Dennis Noermann and Andrew de Quincey");
MODULE_LICENSE("GPL");


