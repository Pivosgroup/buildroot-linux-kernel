
#ifndef _RDA5880_adp_H_
#define _RDA5880_adp_H_

#include <linux/kernel.h>
#include <linux/i2c.h>

#include "../../aml_demod.h"
#include "../../demod_func.h"

#undef MS_U8
#undef MS_U16
#undef MS_U32
#define MS_U8   u8
#define MS_U16 u16
#define MS_U32 u32

#include "RDA5880.h"

/*low level*/
int RDA5880e_Write_single(unsigned char reg, unsigned char high, unsigned char low);

unsigned short RDA5880e_Read(unsigned char reg);

void MsOS_DelayTask(int delay_ms);



/*high level*/
int set_tuner_RDA5880(struct aml_demod_sta *demod_sta, 
			     struct aml_demod_i2c *adap);

int init_tuner_RDA5880(struct aml_demod_sta *demod_sta, 
		     struct aml_demod_i2c *adap);

int set_tuner_RDA5880_suspend(struct aml_demod_i2c *adap);

#endif/*_RDA5880_adp_H_*/


