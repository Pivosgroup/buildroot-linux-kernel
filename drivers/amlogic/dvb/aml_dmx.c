/*
 * AMLOGIC Smart card driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include <mach/am_regs.h>
#endif
#include "aml_dvb.h"

#define USE_AHB_MODE

#if 1
#define pr_dbg(fmt, args...) printk(KERN_DEBUG "DVB: " fmt, ## args)
#define pr_dbg(fmt, args...) printk( "DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "DVB: " fmt, ## args)

#define DMX_READ_REG(i,r)\
	((i)?((i==1)?READ_MPEG_REG(r##_2):READ_MPEG_REG(r##_3)):READ_MPEG_REG(r))

#define DMX_WRITE_REG(i,r,d)\
	do{\
	if(i==1) {\
		WRITE_MPEG_REG(r##_2,d);\
	} else if(i==2) {\
		WRITE_MPEG_REG(r##_3,d);\
	}\
	else {\
		WRITE_MPEG_REG(r,d);\
	}\
	}while(0)





#define NO_SUB

#define SYS_CHAN_COUNT    (4)
#define SEC_GRP_LEN_0     (8*1024)
#define SEC_GRP_LEN_1     (10*1024)
#define SEC_GRP_LEN_2     (12*1024)
#define SEC_GRP_LEN_3     (13*1024)

/*Reset the demux device*/
void dmx_reset_hw(struct aml_dvb *dvb);

static irqreturn_t dmx_irq_handler(int irq_number, void *para)
{
	struct tasklet_struct *tasklet=(struct tasklet_struct*)para;
	tasklet_schedule(tasklet);

	return  IRQ_HANDLED;
}

static inline int sec_filter_match(struct aml_filter *f, u8 *p)
{
	int b;
	u8 neq = 0;
	
	if(!f->used)
		return 0;
	
	for(b=0; b<FILTER_LEN; b++) {
		u8 xor = p[b]^f->value[b];
		
		if(xor&f->maskandmode[b])
			return 0;
		
		if(xor&f->maskandnotmode[b])
			neq = 1;
	}
	
	if(f->neq && !neq)
		return 0;
	
	return 1;
}

static void sec_data_notify(struct aml_dmx *dmx, u16 sec_num, u16 buf_num)
{
	int i;
	u8 *p = (u8*)dmx->sec_buf[buf_num].addr;
	int sec_len;
	struct aml_filter *f;
	
	for(i=0; i<FILTER_COUNT; i++) {
		f = &dmx->filter[i];
		
		if(sec_filter_match(f, p))
			break;
	}
	
	if(i>=FILTER_COUNT)
		return;
	sec_len = (((p[1]&0xF)<<8)|p[2])+3;
	
	if(f->feed && f->feed->cb.sec)
		f->feed->cb.sec(p, sec_len, NULL, 0, f->filter, DMX_OK);
}

static void process_section(struct aml_dmx *dmx)
{
	u32 ready, i;
	u16 sec_num;
	
	//pr_dbg("section\n");
	ready = DMX_READ_REG(dmx->id, SEC_BUFF_READY);
	
	if(ready) {
#ifdef USE_AHB_MODE
	//	WRITE_ISA_REG(AHB_BRIDGE_CTRL1, READ_ISA_REG (AHB_BRIDGE_CTRL1) | (1 << 31));
	//	WRITE_ISA_REG(AHB_BRIDGE_CTRL1, READ_ISA_REG (AHB_BRIDGE_CTRL1) & (~ (1 << 31)));
#endif
		for(i=0; i<32; i++) {
			if(!(ready & (1 << i)))
				continue;
			
			sec_num = DMX_READ_REG(dmx->id, SEC_BUFF_NUMBER) >> 8;
			//flush_and_inv_dcache_all(); //removed by whh
			sec_data_notify(dmx, sec_num, i);
			
			DMX_WRITE_REG(dmx->id, SEC_BUFF_READY, (1 << i));
		}
	}
}

#ifdef NO_SUB
static void process_sub(struct aml_dmx *dmx)
{
}
#endif

static void process_pes(struct aml_dmx *dmx)
{
}

static void process_om_read(struct aml_dmx *dmx)
{
	unsigned i;
	unsigned short om_cmd_status_data_0 = 0;
	unsigned short om_cmd_status_data_1 = 0;
//	unsigned short om_cmd_status_data_2 = 0;
	unsigned short om_cmd_data_out = 0;
	
	om_cmd_status_data_0 = DMX_READ_REG(dmx->id, OM_CMD_STATUS);
	om_cmd_status_data_1 = DMX_READ_REG(dmx->id, OM_CMD_DATA);
//	om_cmd_status_data_2 = DMX_READ_REG(dmx->id, OM_CMD_DATA2);
	
	if(om_cmd_status_data_0 & 1) {
		DMX_WRITE_REG(dmx->id, OM_DATA_RD_ADDR, (1<<15) | ((om_cmd_status_data_1 & 0xff) << 2));
		for(i = 0; i<(((om_cmd_status_data_1 >> 7) & 0x1fc) >> 1); i++) {
			om_cmd_data_out = DMX_READ_REG(dmx->id, OM_DATA_RD); 
		}
	
		om_cmd_data_out = DMX_READ_REG(dmx->id, OM_DATA_RD_ADDR); 
		DMX_WRITE_REG(dmx->id, OM_DATA_RD_ADDR, 0);
		DMX_WRITE_REG(dmx->id, OM_CMD_STATUS, 1);
	}
}

static void dmx_irq_bh_handler(unsigned long arg)
{
	struct aml_dmx *dmx = (struct aml_dmx*)arg;
	u32 status;

	status = DMX_READ_REG(dmx->id, STB_INT_STATUS);
	
	if(!status) return;
	//printk("demux int\n");
	if(status & (1<<SECTION_BUFFER_READY)) {
		process_section(dmx);
	}
#ifdef NO_SUB
        if(status & (1<<SUB_PES_READY)) {
		process_sub(dmx);
	}
#endif
	if(status & (1<<OTHER_PES_READY)) {
		process_pes(dmx);
	}
	if(status & (1<<OM_CMD_READ_PENDING)) {
		process_om_read(dmx);
	}
	if(status & (1<<DUPLICATED_PACKET)) {
	}
	if(status & (1<<DIS_CONTINUITY_PACKET)) {
	}
	if(status & (1<<VIDEO_SPLICING_POINT)) {
	}
	if(status & (1<<AUDIO_SPLICING_POINT)) {
	}
	if(status & (1<<TS_ERROR_PIN)) {
	}
	
	if(dmx->irq_handler) {
		dmx->irq_handler(dmx->dmx_irq, (void*)dmx->id);
	}
	
	DMX_WRITE_REG(dmx->id, STB_INT_STATUS, status);
        
	return;
}

static void dvr_irq_bh_handler(unsigned long arg)
{
	struct aml_dmx *dmx = (struct aml_dmx*)arg;
	return;
}

/*Enable the STB*/
static void stb_enable(struct aml_dvb *dvb)
{
	int out_src, des_in, en_des, invert_clk, fec_s, fec_clk, hiu;
	
	switch(dvb->stb_source) {
		case AM_TS_SRC_TS0:
			out_src = 0;
			des_in  = 0;
			en_des  = 1;
			invert_clk = 1;
			fec_s   = 0;
			fec_clk = 4;
			hiu     = 0;
		break;
		case AM_TS_SRC_TS1:
			out_src = 1;
			des_in  = 1;
			en_des  = 1;
			invert_clk = 1;
			fec_s   = 1;
			fec_clk = 4;
			hiu     = 0;
		break;
		case AM_TS_SRC_S2P0:
			out_src = 6;
			des_in  = 0;
			en_des  = 1;
			invert_clk = 1;
			fec_s   = 0;
			fec_clk = 4;
			hiu     = 0;
		break;
		case AM_TS_SRC_S2P1:
			out_src = 6;
			des_in  = 1;
			en_des  = 1;
			invert_clk = 1;
			fec_s   = 1;
			fec_clk = 4;
			hiu     = 0;
		break;
		case AM_TS_SRC_HIU:
			out_src = 0;
			des_in  = 0;
			en_des  = 0;
			invert_clk = 0;
			fec_s   = 0;
			fec_clk = 4;
			hiu     = 1;
		break;
		default:
			out_src = 0;
			des_in  = 0;
			en_des  = 0;
			invert_clk = 0;
			fec_s   = 0;
			fec_clk = 0;
			hiu     = 0;
		break;
	}
	
	WRITE_MPEG_REG(STB_TOP_CONFIG, 
		(out_src<<TS_OUTPUT_SOURCE) |
		(des_in<<DES_INPUT_SEL)     |
		(en_des<<ENABLE_DES_PL)     |
		(0<<INVERT_S2P0_FEC_ERROR)    |
		(0<<INVERT_S2P0_FEC_DATA)     |
		(0<<INVERT_S2P0_FEC_SYNC)     |
		(0<<INVERT_S2P0_FEC_VALID)    |
		(invert_clk<<INVERT_S2P0_FEC_CLK)|
		(fec_s<<S2P0_FEC_SERIAL_SEL));

	WRITE_MPEG_REG(TS_FILE_CONFIG,
		(6<<DES_OUT_DLY)                      |
		(3<<TRANSPORT_SCRAMBLING_CONTROL_ODD) |
		(hiu<<TS_HIU_ENABLE)                  |
		(fec_clk<<FEC_FILE_CLK_DIV));
}

int dsc_set_pid(struct aml_dsc *dsc, int pid)
{
	u32 data;
	
	WRITE_MPEG_REG(TS_PL_PID_INDEX, (dsc->id & 0x0f)>>1);
	data = READ_MPEG_REG(TS_PL_PID_DATA);
	if(dsc->id&1) {
		data &= 0xFFFF0000;
		data |= pid;
	} else {
		data &= 0xFFFF;
		data |= (pid<<16);
	}
	WRITE_MPEG_REG(TS_PL_PID_INDEX, (dsc->id & 0x0f)>>1);
	WRITE_MPEG_REG(TS_PL_PID_DATA, data);
	WRITE_MPEG_REG(TS_PL_PID_INDEX, 0);
	
	return 0;
}

int dsc_set_key(struct aml_dsc *dsc, int type, u8 *key)
{
	u32 key0, key1;
	
	key0 = (key[7]<<24)|(key[6]<<16)|(key[5]<<8)|key[4];
	key1 = (key[3]<<24)|(key[2]<<16)|(key[1]<<8)|key[0];
	WRITE_MPEG_REG(COMM_DESC_KEY0, key0);
	WRITE_MPEG_REG(COMM_DESC_KEY1, key1);
	WRITE_MPEG_REG(COMM_DESC_KEY_RW, (dsc->id + type*DSC_COUNT));
	
	return 0;
}

int dsc_release(struct aml_dsc *dsc)
{
	u32 data;
	
	WRITE_MPEG_REG(TS_PL_PID_INDEX, (dsc->id & 0x0f)>>1);
	data = READ_MPEG_REG(TS_PL_PID_DATA);
	if(dsc->id&1) {
		data |= 1<<PID_MATCH_DISABLE_LOW;
	} else {
		data |= 1<<PID_MATCH_DISABLE_HIGH;
	}
	WRITE_MPEG_REG(TS_PL_PID_INDEX, (dsc->id & 0x0f)>>1);
	WRITE_MPEG_REG(TS_PL_PID_DATA, data);
	
	return 0;
}

/*Set section buffer*/
static int dmx_alloc_sec_buffer(struct aml_dmx *dmx)
{
	unsigned long base;
	unsigned long grp_addr[SEC_BUF_GRP_COUNT];
	int grp_len[SEC_BUF_GRP_COUNT];
	int i;
	
	if(dmx->sec_pages)
		return 0;
	
	grp_len[0] = SEC_GRP_LEN_0;
	grp_len[1] = SEC_GRP_LEN_1;
	grp_len[2] = SEC_GRP_LEN_2;
	grp_len[3] = SEC_GRP_LEN_3;

	dmx->sec_total_len = grp_len[0]+grp_len[1]+grp_len[2]+grp_len[3];
	dmx->sec_pages = __get_free_pages(GFP_KERNEL, get_order(dmx->sec_total_len));
	if(!dmx->sec_pages) {
		pr_error("cannot allocate section buffer %d bytes %d order\n", dmx->sec_total_len, get_order(dmx->sec_total_len));
		return -1;
	}
	
	grp_addr[0] = virt_to_phys((void*)dmx->sec_pages);
	grp_addr[1] = grp_addr[0]+grp_len[0];
	grp_addr[2] = grp_addr[1]+grp_len[1];
	grp_addr[3] = grp_addr[2]+grp_len[2];
	
	dmx->sec_buf[0].addr = dmx->sec_pages;
	dmx->sec_buf[0].len  = grp_len[0]/8;
	
	for(i=1; i<SEC_BUF_COUNT; i++) {
		dmx->sec_buf[i].addr = dmx->sec_buf[i-1].addr+dmx->sec_buf[i-1].len;
		dmx->sec_buf[i].len  = grp_len[i/8]/8;
	}
	
	base = grp_addr[0]&0xFFFF0000;
	DMX_WRITE_REG(dmx->id, SEC_BUFF_BASE, base>>16);
	DMX_WRITE_REG(dmx->id, SEC_BUFF_01_START, (((grp_addr[0]-base)>>8)<<16)|((grp_addr[1]-base)>>8));
	DMX_WRITE_REG(dmx->id, SEC_BUFF_23_START, (((grp_addr[2]-base)>>8)<<16)|((grp_addr[3]-base)>>8));
	DMX_WRITE_REG(dmx->id, SEC_BUFF_SIZE, (grp_len[0]>>10)|
		((grp_len[1]>>10)<<4)|
		((grp_len[2]>>10)<<8)|
		((grp_len[3]>>10)<<12));
	
	return 0;
}

#ifdef NO_SUB
/*Set subtitle buffer*/
static int dmx_alloc_sub_buffer(struct aml_dmx *dmx)
{
	unsigned long addr;
	
	if(dmx->sub_pages)
		return 0;
	
	dmx->sub_buf_len = 64*1024;
	dmx->sub_pages = __get_free_pages(GFP_KERNEL, get_order(dmx->sub_buf_len));
	if(!dmx->sub_pages) {
		pr_error("cannot allocate subtitle buffer\n");
		return -1;
	}
	
	addr = virt_to_phys((void*)dmx->sub_pages);
	DMX_WRITE_REG(dmx->id, SB_START, addr>>12);
	DMX_WRITE_REG(dmx->id, SB_LAST_ADDR, (dmx->sub_buf_len>>3));
	return 0;	
}
#endif /*NO_SUB*/

/*Set PES buffer*/
static int dmx_alloc_pes_buffer(struct aml_dmx *dmx)
{
	unsigned long addr;
	
	if(dmx->pes_pages)
		return 0;
	
	dmx->pes_buf_len = 64*1024;
	dmx->pes_pages = __get_free_pages(GFP_KERNEL, get_order(dmx->pes_buf_len));
	if(!dmx->pes_pages) {
		pr_error("cannot allocate pes buffer\n");
		return -1;
	}
	
	addr = virt_to_phys((void*)dmx->pes_pages);
	DMX_WRITE_REG(dmx->id, OB_START, addr>>12);
	DMX_WRITE_REG(dmx->id, OB_LAST_ADDR, (dmx->pes_buf_len>>3));
	return 0;	
}

static void dmx_set_mux(struct aml_dvb *dvb)
{
#define PREG_PIN_MUX_REG5 PERIPHS_PIN_MUX_5
#define PINMUX5_GPIOD12_FECB_DVALID  1<<22
#define PINMUX5_GPIOD13_FECB_FAIL       1<<21
#define PINMUX5_GPIOD14_FECB_SOP         1<<20
#define PINMUX5_GPIOD15_FECB_CLK         1<<19
#define PINMUX5_GPIOD16_FECB_D0           1<<18
#define PINMUX1_GPIOD17_24_FECB_D1_7 1<<17

#if 0
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC4_FEC_A_D0);
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC5_11_FEC_D1_7);
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC0_FEC_A_CLK);
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC1_FEC_A_SOP);
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC2_FEC_A_FAIL);
	SET_PERI_REG_MASK(PREG_PIN_MUX_REG1,PINMUX1_GPIOC3_FEC_A_VALID);
#endif
	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX5_GPIOD16_FECB_D0);
	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX1_GPIOD17_24_FECB_D1_7);
 	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX5_GPIOD15_FECB_CLK);
	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX5_GPIOD14_FECB_SOP);
	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX5_GPIOD12_FECB_DVALID);
	SET_CBUS_REG_MASK(PREG_PIN_MUX_REG5,PINMUX5_GPIOD13_FECB_FAIL);


}

/*Initalize the registers*/
static int dmx_init(struct aml_dmx *dmx)
{
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	int irq;
	
	if(dmx->init) return 0;
	
	pr_dbg("demux init\n");
	
	/*Register irq handlers*/
	if(dmx->dmx_irq!=-1) {
		pr_dbg("request irq\n");
		tasklet_init(&dmx->dmx_tasklet, dmx_irq_bh_handler, (unsigned long)dmx);
		irq = request_irq(dmx->dmx_irq, dmx_irq_handler, IRQF_SHARED, "dmx irq", &dmx->dmx_tasklet);
	}
	
	if(dmx->dvr_irq!=-1) {
		tasklet_init(&dmx->dvr_tasklet, dvr_irq_bh_handler, (unsigned long)dmx);
		irq = request_irq(dmx->dvr_irq, dmx_irq_handler, IRQF_SHARED, "dvr irq", &dmx->dvr_tasklet);
	}
	
	/*Allocate buffer*/
	if(dmx_alloc_sec_buffer(dmx)<0)
		return -1;
#ifdef NO_SUB
	if(dmx_alloc_sub_buffer(dmx)<0)
		return -1;
#endif
	if(dmx_alloc_pes_buffer(dmx)<0)
		return -1;
	
	/*Reset the hardware*/
	if (!dvb->dmx_init) {
		pr_dbg("demux reset\n");
		dmx_set_mux(dvb);
		dmx_reset_hw(dvb);
	}
	
	dvb->dmx_init++;
	
	dmx->init = 1;
	
	return 0;
}

/*Release the resource*/
static int dmx_deinit(struct aml_dmx *dmx)
{
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	
	if(!dmx->init) return 0;
	
	DMX_WRITE_REG(dmx->id, DEMUX_CONTROL, 0);
	
	dvb->dmx_init--;
	
	/*Reset the hardware*/
	if(!dvb->dmx_init) {
		dmx_reset_hw(dvb);
	}
	
	if(dmx->sec_pages) {
		free_pages(dmx->sec_pages, get_order(dmx->sec_total_len));
		dmx->sec_pages = 0;
	}
#ifdef NO_SUB
	if(dmx->sub_pages) {
		free_pages(dmx->sub_pages, get_order(dmx->sub_buf_len));
		dmx->sub_pages = 0;
	}
#endif
	if(dmx->pes_pages) {
		free_pages(dmx->pes_pages, get_order(dmx->pes_buf_len));
		dmx->pes_pages = 0;
	}
	
	if(dmx->dmx_irq!=-1) {
		free_irq(dmx->dmx_irq, dmx);
		tasklet_kill(&dmx->dmx_tasklet);
	}
	if(dmx->dvr_irq!=-1) {
		free_irq(dmx->dvr_irq, dmx);
		tasklet_kill(&dmx->dvr_tasklet);
	}
	
	dmx->init = 0;

	return 0;
}

/*Enable the demux device*/
static int dmx_enable(struct aml_dmx *dmx)
{
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	int fec_sel, hi_bsf, fec_clk, record;
	pr_dbg("demux enable\n");
	
	switch(dmx->source) {
		case AM_TS_SRC_TS0:
			fec_sel = 0;
			fec_clk = 1;
			record  = dmx->record?1:0;
			break;
		case AM_TS_SRC_TS1:
			fec_sel = 1;
			fec_clk = 1;
			record  = dmx->record?1:0;
		break;
		case AM_TS_SRC_S2P0:
		case AM_TS_SRC_S2P1:
			fec_sel = 6;
			fec_clk = 1;
			record  = dmx->record?1:0;
		break;
		case AM_TS_SRC_HIU:
			fec_sel = 7;
			fec_clk = 0;
			record  = 0;
		break;
		default:
			fec_sel = 0;
			fec_clk = 0;
			record  = 0;
		break;
	}
	
	if(dmx->channel[0].used || dmx->channel[1].used)
		hi_bsf = 1;
	else
		hi_bsf = 0;
	
	if(dmx->chan_count) {
		/*Initialize the registers*/
		DMX_WRITE_REG(dmx->id, STB_INT_MASK,
			(0<<(AUDIO_SPLICING_POINT))     |
			(0<<(VIDEO_SPLICING_POINT))     |
			(0<<(OTHER_PES_READY))          |
			(1<<(SUB_PES_READY))            |
			(1<<(SECTION_BUFFER_READY))     |
			(0<<(OM_CMD_READ_PENDING))      |
			(0<<(TS_ERROR_PIN))             |
			(1<<(NEW_PDTS_READY))           | 
			(0<<(DUPLICATED_PACKET))        | 
			(0<<(DIS_CONTINUITY_PACKET)));
		DMX_WRITE_REG(dmx->id, DEMUX_MEM_REQ_EN, 
#ifdef USE_AHB_MODE
			(1<<SECTION_AHB_DMA_EN)         |
#endif
			(1<<SECTION_PACKET)             |
			(1<<VIDEO_PACKET)               |
			(1<<AUDIO_PACKET)               |
			(1<<SUB_PACKET)                 |
			(0<<OTHER_PES_PACKET));
	 	DMX_WRITE_REG(dmx->id, PES_STRONG_SYNC, 0x1234);
 		DMX_WRITE_REG(dmx->id, DEMUX_ENDIAN,
			(7<<OTHER_ENDIAN)               |
			(7<<BYPASS_ENDIAN)              |
			(0<<SECTION_ENDIAN));
		DMX_WRITE_REG(dmx->id, TS_HIU_CTL,
			(0<<LAST_BURST_THRESHOLD)       |
			(hi_bsf<<USE_HI_BSF_INTERFACE));

		
		DMX_WRITE_REG(dmx->id, FEC_INPUT_CONTROL,
			(0<<FEC_CORE_SEL)               |
			(fec_sel<<FEC_SEL)              |
			(fec_clk<<FEC_INPUT_FEC_CLK)    |
			(0<<FEC_INPUT_SOP)              |
			(0<<FEC_INPUT_D_VALID)          |
			(0<<FEC_INPUT_D_FAIL));
		DMX_WRITE_REG(dmx->id, STB_OM_CTL, 
			(0x20<<MAX_OM_DMA_COUNT)        |
			(0x7f<<LAST_OM_ADDR));
		DMX_WRITE_REG(dmx->id, DEMUX_CONTROL,
			(0<<BYPASS_USE_RECODER_PATH)          |
			(0<<INSERT_AUDIO_PES_STRONG_SYNC)     |
			(0<<INSERT_VIDEO_PES_STRONG_SYNC)     |    
			(0<<OTHER_INT_AT_PES_BEGINING)        |
			(0<<DISCARD_AV_PACKAGE)               |
			(0<<TS_RECORDER_SELECT)               |
			(record<<TS_RECORDER_ENABLE)          |
			(1<<KEEP_DUPLICATE_PACKAGE)           |
			(1<<SECTION_END_WITH_TABLE_ID)        |
			(1<<STB_DEMUX_ENABLE));
	} else {
		DMX_WRITE_REG(dmx->id, STB_INT_MASK, 0);
		DMX_WRITE_REG(dmx->id, FEC_INPUT_CONTROL, 0);
		DMX_WRITE_REG(dmx->id, DEMUX_CONTROL, 0);
	}
	
	
	return 0;
}

/*Get the channel's ID by its PID*/
static int dmx_get_chan(struct aml_dmx *dmx, int pid)
{
	int id;
	
	for(id=0; id<CHANNEL_COUNT; id++) {
		if(dmx->channel[id].used && dmx->channel[id].pid==pid)
			return id;
	}
	
	return -1;
}

/*Get the channel's target*/
static u32 dmx_get_chan_target(struct aml_dmx *dmx, int cid)
{
	u32 type;
	
	if(!dmx->channel[cid].used) {
		return 0xFFFF;
	}
	
	if(dmx->channel[cid].type==DMX_TYPE_SEC) {
		type = SECTION_PACKET;
	} else {
		switch(dmx->channel[cid].pes_type) {
			case DMX_TS_PES_AUDIO:
				type = AUDIO_PACKET;
			break;
			case DMX_TS_PES_VIDEO:
				type = VIDEO_PACKET;
			break;
			case DMX_TS_PES_SUBTITLE:
				type = SUB_PACKET;
			break;
			default:
				type = OTHER_PES_PACKET;
			break;
		}
	}
	
	pr_dbg("chan target: %x %x\n", type, dmx->channel[cid].pid);
	return (type<<PID_TYPE)|dmx->channel[cid].pid;
}

/*Set the channel registers*/
static int dmx_set_chan_regs(struct aml_dmx *dmx, int cid)
{
	u32 data, addr, advance=0, max;
	
	pr_dbg("set channel (id:%d PID:0x%x) registers\n", cid, dmx->channel[cid].pid);
	
	while(DMX_READ_REG(dmx->id, FM_WR_ADDR) & 0x8000) {
		udelay(100);
	}
	
	if(cid&1) {
		data = (dmx_get_chan_target(dmx, cid-1)<<16) | dmx_get_chan_target(dmx, cid);
	} else {
		data = (dmx_get_chan_target(dmx, cid)<<16) | dmx_get_chan_target(dmx, cid+1);
	}
	addr = cid>>1;
	DMX_WRITE_REG(dmx->id, FM_WR_DATA, data);
	DMX_WRITE_REG(dmx->id, FM_WR_ADDR, (advance<<16)|0x8000|addr);
	
	pr_dbg("write fm %x:%x\n", (advance<<16)|0x8000|addr, data);
	
	for(max=CHANNEL_COUNT-1; max>0; max--) {
		if(dmx->channel[max].used)
			break;
	}
	
	data = DMX_READ_REG(dmx->id, MAX_FM_COMP_ADDR)&0xF0;
	DMX_WRITE_REG(dmx->id, MAX_FM_COMP_ADDR, data|(max>>1));
	
	pr_dbg("write fm comp %x\n", data|(max>>1));
	
	if(DMX_READ_REG(dmx->id, OM_CMD_STATUS)&0x8e00) {
		pr_error("error send cmd %x\n", DMX_READ_REG(dmx->id, OM_CMD_STATUS));
	}
	
	return 0;
}

/*Get the filter target*/
static int dmx_get_filter_target(struct aml_dmx *dmx, int fid, u32 target[FILTER_LEN])
{
	struct dmx_section_filter *filter;
	struct aml_filter *f;
	int i, cid;
	int neq_bytes = 0;
	
	fid = fid&0xFFFF;
	f = &dmx->filter[fid];
	
	if(!f->used) {
		target[0] = 0x1fff;
		for(i=1; i<FILTER_LEN; i++) {
			target[i] = 0x9fff;
		}
		return 0;
	}
	
	cid = f->chan_id;
	filter = f->filter;
	
	if(filter->filter_mode[0]!=0xFF) {
		neq_bytes = 2;
	} else {
		for(i=0; i<FILTER_LEN; i++) {
			if(filter->filter_mode[i]!=0xFF)
				neq_bytes++;
		}
	}
	
	f->neq = 0;
	
	for(i=0; i<FILTER_LEN; i++) {
		u8 value = filter->filter_value[i];
		u8 mask  = filter->filter_mask[i];
		u8 mode  = filter->filter_mode[i];
		u8 mb, mb1, nb, v;
		
		if(!i) {
			mb = 1;
			mb1= 1;
			v  = 0;
			if(mode==0xFF) {
				if((mask&0xF0)==0xF0) {
					mb1 = 0;
					v  |= value&0xF0;
				}
				if((mask&0x0F)==0x0F) {
					mb = 0;
					v  |= value&0x0F;
				}
			}
			
			target[i] = (mb<<SECTION_FIRSTBYTE_MASKLOW)         |
				(mb1<<SECTION_FIRSTBYTE_MASKHIGH)           |
				(0<<SECTION_FIRSTBYTE_DISABLE_PID_CHECK)    |
				(cid<<SECTION_FIRSTBYTE_PID_INDEX)          |
				v;
		} else {
			mb = 1;
			nb = 0;
			v = 0;
			
			if(mode==0xFF) {
				if(mask==0xFF) {
					mb = 0;
					nb = 0;
					v  = value;
				}
			} else {
				if(neq_bytes==1) {
					mb = 0;
					nb = 1;
					v = value;
				}
			}
			
			target[i] = (mb<<SECTION_RESTBYTE_MASK)             |
				(nb<<SECTION_RESTBYTE_MASK_EQ)              |
				(0<<SECTION_RESTBYTE_DISABLE_PID_CHECK)     |
				(cid<<SECTION_RESTBYTE_PID_INDEX)           |
				v;
		}
		
		f->value[i] = value;
		f->maskandmode[i] = mask&mode;
		f->maskandnotmode[i] = mask&~mode;
		
		if(f->maskandnotmode[i])
			f->neq = 1;
	}
	
	return 0;
}

/*Set the filter registers*/
static int dmx_set_filter_regs(struct aml_dmx *dmx, int fid)
{
	u32 t1[FILTER_LEN], t2[FILTER_LEN];
	u32 addr, data, advance = 0, max;
	int i;
	
	fid = fid&0xFFFF;
	
	pr_dbg("set filter (id:%d) registers\n", fid);
	
	if(fid&1) {
		dmx_get_filter_target(dmx, fid-1, t1);
		dmx_get_filter_target(dmx, fid, t2);
	} else {
		dmx_get_filter_target(dmx, fid, t1);
		dmx_get_filter_target(dmx, fid+1, t2);
	}
	
	for(i=0; i<FILTER_LEN; i++) {
		while(DMX_READ_REG(dmx->id, FM_WR_ADDR) & 0x8000) {
			udelay(100);
		}
		
		data = (t1[i]<<16)|t2[i];
		addr = (fid>>1)|((i+1)<<4);
		
		DMX_WRITE_REG(dmx->id, FM_WR_DATA, data);
		DMX_WRITE_REG(dmx->id, FM_WR_ADDR, (advance<<16)|0x8000|addr);
		
		pr_dbg("write fm %x:%x\n", (advance<<16)|0x8000|addr, data);
	}
	
	for(max=FILTER_COUNT-1; max>0; max--) {
		if(dmx->filter[max].used)
			break;
	}
	
	data = DMX_READ_REG(dmx->id, MAX_FM_COMP_ADDR)&0xF;
	DMX_WRITE_REG(dmx->id, MAX_FM_COMP_ADDR, data|((max>>1)<<4));
	
	pr_dbg("write fm comp %x\n", data|((max>>1)<<4));
	
	if(DMX_READ_REG(dmx->id, OM_CMD_STATUS)&0x8e00) {
		pr_error("error send cmd %x\n",DMX_READ_REG(dmx->id, OM_CMD_STATUS));
	}
	
	return 0;
}

/*Reset the demux device*/
void dmx_reset_hw(struct aml_dvb *dvb)
{
	int id, times;
	
	for (id=0; id<DMX_DEV_COUNT; id++) {
		if(dvb->dmx[id].dmx_irq!=-1) {
			disable_irq(dvb->dmx[id].dmx_irq);
		}
		if(dvb->dmx[id].dvr_irq!=-1) {
			disable_irq(dvb->dmx[id].dvr_irq);
		}
	}
	
	WRITE_MPEG_REG(RESET1_REGISTER, RESET_DEMUXSTB);
	
	for (id=0; id<DMX_DEV_COUNT; id++) {
		times = 0;
		while(times++ < 1000000) {
			if (!(DMX_READ_REG(id, OM_CMD_STATUS)&0x01))
				break;
		}
	}
	
	WRITE_MPEG_REG(STB_TOP_CONFIG, 0);
	
	for (id=0; id<DMX_DEV_COUNT; id++) {
		u32 version, data;
		
		if(dvb->dmx[id].dmx_irq!=-1) {
			enable_irq(dvb->dmx[id].dmx_irq);
		}
		if(dvb->dmx[id].dvr_irq!=-1) {
			enable_irq(dvb->dmx[id].dvr_irq);
		}
		DMX_WRITE_REG(id, DEMUX_CONTROL, 0x0000);
		version = DMX_READ_REG(id, STB_VERSION);
		DMX_WRITE_REG(id, STB_TEST_REG, version);
		pr_dbg("STB %d hardware version : %d\n", id, version);
		DMX_WRITE_REG(id, STB_TEST_REG, 0x5550);
		data = DMX_READ_REG(id, STB_TEST_REG);
		if(data!=0x5550)
			pr_error("STB %d register access failed\n", id);
		DMX_WRITE_REG(id, STB_TEST_REG, 0xaaa0);
		data = DMX_READ_REG(id, STB_TEST_REG);
		if(data!=0xaaa0)
			pr_error("STB %d register access failed\n", id);
		DMX_WRITE_REG(id, MAX_FM_COMP_ADDR, 0x0000);
		DMX_WRITE_REG(id, STB_INT_MASK, 0);
		DMX_WRITE_REG(id, STB_INT_STATUS, 0xffff);
		DMX_WRITE_REG(id, FEC_INPUT_CONTROL, 0);
	}
	
	stb_enable(dvb);
	
	for(id=0; id<DMX_DEV_COUNT; id++)
	{
		struct aml_dmx *dmx = &dvb->dmx[id];
		int n;
		unsigned long addr;
		unsigned long base;
		unsigned long grp_addr[SEC_BUF_GRP_COUNT];
		int grp_len[SEC_BUF_GRP_COUNT];
		int i;
		
		if(dmx->sec_pages) {
			grp_len[0] = SEC_GRP_LEN_0;
			grp_len[1] = SEC_GRP_LEN_1;
			grp_len[2] = SEC_GRP_LEN_2;
			grp_len[3] = SEC_GRP_LEN_3;
				
			grp_addr[0] = virt_to_phys((void*)dmx->sec_pages);
			grp_addr[1] = grp_addr[0]+grp_len[0];
			grp_addr[2] = grp_addr[1]+grp_len[1];
			grp_addr[3] = grp_addr[2]+grp_len[2];
			
			base = grp_addr[0]&0xFFFF0000;
			DMX_WRITE_REG(dmx->id, SEC_BUFF_BASE, base>>16);
			DMX_WRITE_REG(dmx->id, SEC_BUFF_01_START, (((grp_addr[0]-base)>>8)<<16)|((grp_addr[1]-base)>>8));
			DMX_WRITE_REG(dmx->id, SEC_BUFF_23_START, (((grp_addr[2]-base)>>8)<<16)|((grp_addr[3]-base)>>8));
			DMX_WRITE_REG(dmx->id, SEC_BUFF_SIZE, (grp_len[0]>>10)|
					((grp_len[1]>>10)<<4)|
					((grp_len[2]>>10)<<8)|
					((grp_len[3]>>10)<<12));
		}
		
		if(dmx->sub_pages) {	
			addr = virt_to_phys((void*)dmx->sub_pages);
			DMX_WRITE_REG(dmx->id, SB_START, addr>>12);
			DMX_WRITE_REG(dmx->id, SB_LAST_ADDR, (dmx->sub_buf_len>>3));
		}
		
		if(dmx->pes_pages) {
			addr = virt_to_phys((void*)dmx->pes_pages);
			DMX_WRITE_REG(dmx->id, OB_START, addr>>12);
			DMX_WRITE_REG(dmx->id, OB_LAST_ADDR, (dmx->pes_buf_len>>3));
		}
		
		for(n=0; n<CHANNEL_COUNT; n++)
		{
			struct aml_channel *chan = &dmx->channel[n];
			
			if(chan->used)
			{
				dmx_set_chan_regs(dmx, n);
			}
		}
		
		for(n=0; n<FILTER_COUNT; n++)
		{
			struct aml_filter *filter = &dmx->filter[n];
			
			if(filter->used)
			{
				dmx_set_filter_regs(dmx, n);
			}
		}
				
		dmx_enable(&dvb->dmx[id]);
	}
	
	for(id=0; id<DSC_COUNT; id++)
	{
		struct aml_dsc *dsc = &dvb->dsc[id];
		
		if(dsc->used)
		{
			dsc_set_pid(dsc, dsc->pid);
			
			if(dsc->set&1)
				dsc_set_key(dsc, 0, dsc->even);
			if(dsc->set&2)
				dsc_set_key(dsc, 1, dsc->odd);
		}
	}
}

/*Allocate a new channel*/
int dmx_alloc_chan(struct aml_dmx *dmx, int type, int pes_type, int pid)
{
	int id = CHANNEL_COUNT;
	
	if(type==DMX_TYPE_TS) {
		switch(pes_type) {
			case DMX_TS_PES_VIDEO:
				if(!dmx->channel[0].used)
					id = 0;
			break;
			case DMX_TS_PES_AUDIO:
				if(!dmx->channel[1].used)
					id = 1;
			break;
			case DMX_TS_PES_SUBTITLE:
			case DMX_TS_PES_TELETEXT:
				if(!dmx->channel[2].used)
					id = 2;
			break;
			case DMX_TS_PES_PCR:
				if(!dmx->channel[3].used)
					id = 3;
			break;
			default:
			break;
		}
	} else {
		for(id=SYS_CHAN_COUNT; id<CHANNEL_COUNT; id++) {
			if(!dmx->channel[id].used)
				break;
		}
	}
	
	if(id>=CHANNEL_COUNT) {
		pr_error("too many channels\n");
		return -1;
	}
	
	pr_dbg("allocate channel(id:%d PID:0x%x)\n", id, pid);
	
	dmx->channel[id].type = type;
	dmx->channel[id].pes_type = pes_type;
	dmx->channel[id].pid  = pid;
	dmx->channel[id].used = 1;
	dmx->channel[id].filter_count = 0;
	
	dmx_set_chan_regs(dmx, id);
	
	dmx->chan_count++;
	
	dmx_enable(dmx);
	
	return id;
}

/*Free a channel*/
void dmx_free_chan(struct aml_dmx *dmx, int cid)
{
	pr_dbg("free channel(id:%d PID:0x%x)\n", cid, dmx->channel[cid].pid);
	
	dmx->channel[cid].used = 0;
	dmx_set_chan_regs(dmx, cid);
	
	dmx->chan_count--;
	
	dmx_enable(dmx);
}

/*Add a section*/
static int dmx_chan_add_filter(struct aml_dmx *dmx, int cid, struct dvb_demux_feed *feed)
{
	int id;
	
	for(id=0; id<FILTER_COUNT; id++) {
		if(!dmx->filter[id].used)
			break;
	}
	
	if(id>=FILTER_COUNT) {
		pr_error("too many filters\n");
		return -1;
	}
	
	pr_dbg("channel(id:%d PID:0x%x) add filter(id:%d)\n", cid, feed->pid, id);
	
	dmx->filter[id].chan_id = cid;
	dmx->filter[id].used = 1;
	dmx->filter[id].filter = &feed->filter->filter;
	dmx->filter[id].feed = feed;
	dmx->channel[cid].filter_count++;
	
	id = (cid<<16)|id;
	feed->priv = (void*)id;
	
	dmx_set_filter_regs(dmx, id);
	
	return id;
}

static void dmx_remove_filter(struct aml_dmx *dmx, struct dvb_demux_feed *feed)
{
	int id = (int)feed->priv;
	int cid = id>>16;
	int fid = id&0xFFFF;
	
	pr_dbg("channel(id:%d PID:0x%x) remove filter(id:%d)\n", cid, feed->pid, fid);
	
	dmx->filter[fid].used = 0;
	dmx->channel[cid].filter_count--;
	
	dmx_set_filter_regs(dmx, id);
	
	if(dmx->channel[cid].filter_count<=0) {
		dmx_free_chan(dmx, cid);
	}
}

static int dmx_add_feed(struct aml_dmx *dmx, struct dvb_demux_feed *feed)
{
	int id, ret;
	
	switch(feed->type)
	{
		case DMX_TYPE_TS:
			if((ret=dmx_get_chan(dmx, feed->pid))>=0) {
				pr_error("PID %d already used\n", feed->pid);
				return -EBUSY;
			}
			if((ret=dmx_alloc_chan(dmx, feed->type, feed->pes_type, feed->pid))<0) {
				return ret;
			}
			feed->priv = (void*)ret;
		break;
		case DMX_TYPE_SEC:
			if((id=dmx_get_chan(dmx, feed->pid))<0) {
				if((id=dmx_alloc_chan(dmx, feed->type, feed->pes_type, feed->pid))<0) {
					return id;
				}
			} else {
				if(id<SYS_CHAN_COUNT) {
					pr_error("pid 0x%x is not a section\n", feed->pid);
					return -EINVAL;
				}
			}
			if((ret=dmx_chan_add_filter(dmx, id, feed))<0) {
				if(!dmx->channel[id].filter_count) {
					dmx_free_chan(dmx, id);
				}
			}
		break;
		default:
			return -EINVAL;
		break;
	}
	
	dmx->feed_count++;
	
	return 0;
}

static int dmx_remove_feed(struct aml_dmx *dmx, struct dvb_demux_feed *feed)
{
	switch(feed->type)
	{
		case DMX_TYPE_TS:
			dmx_free_chan(dmx, (int)feed->priv);
		break;
		case DMX_TYPE_SEC:
			dmx_remove_filter(dmx, feed);
		break;
		default:
			return -EINVAL;
		break;
	}
	
	dmx->feed_count--;
	return 0;
}

int aml_dmx_hw_init(struct aml_dmx *dmx)
{
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	unsigned long flags;
	int ret;
	
	/*Demux initialize*/
	spin_lock_irqsave(&dmx->slock, flags);
	ret = dmx_init(dmx);
	spin_unlock_irqrestore(&dmx->slock, flags);
	
	return ret;
}

int aml_dmx_hw_deinit(struct aml_dmx *dmx)
{
	unsigned long flags;
	int ret;
	
	spin_lock_irqsave(&dmx->slock, flags);
	ret = dmx_deinit(dmx);
	spin_unlock_irqrestore(&dmx->slock, flags);
	
	return ret;
}

int aml_dmx_hw_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct aml_dmx *dmx = (struct aml_dmx*)dvbdmxfeed->demux;
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	unsigned long flags;
	int ret = 0;
	
	spin_lock_irqsave(&dmx->slock, flags);
	ret = dmx_add_feed(dmx, dvbdmxfeed);
	spin_unlock_irqrestore(&dmx->slock, flags);
	
	return ret;
}

int aml_dmx_hw_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct aml_dmx *dmx = (struct aml_dmx*)dvbdmxfeed->demux;
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	unsigned long flags;
	
	spin_lock_irqsave(&dmx->slock, flags);
	dmx_remove_feed(dmx, dvbdmxfeed);
	spin_unlock_irqrestore(&dmx->slock, flags);
	
	return 0;
}

int aml_dmx_hw_set_source(struct dmx_demux* demux, dmx_source_t src)
{
	struct aml_dmx *dmx = (struct aml_dmx*)demux;
	struct aml_dvb *dvb = (struct aml_dvb*)dmx->demux.priv;
	int ret = 0;
	unsigned long flags;
	
	spin_lock_irqsave(&dmx->slock, flags);
	switch(src) {
		case DMX_SOURCE_FRONT0:
#ifndef CONFIG_AMLOGIC_S2P_TS0
			if(dmx->source!=AM_TS_SRC_TS0) {
				dmx->source = AM_TS_SRC_TS0;
				ret = 1;
			}
#else
			if(dmx->source!=AM_TS_SRC_S2P0) {
				dmx->source = AM_TS_SRC_S2P0;
				ret = 1;
			}
#endif
		break;
		case DMX_SOURCE_FRONT1:
#ifndef CONFIG_AMLOGIC_S2P_TS1
			if(dmx->source!=AM_TS_SRC_TS1) {
				dmx->source = AM_TS_SRC_TS1;
				ret = 1;
			}
#else
			if(dmx->source!=AM_TS_SRC_S2P1) {
				dmx->source = AM_TS_SRC_S2P1;
				ret = 1;
			}
#endif
		break;
		case DMX_SOURCE_DVR0:
			if(dmx->source!=AM_TS_SRC_HIU) {
				dmx->source = AM_TS_SRC_HIU;
				ret = 1;
			}
		break;
		default:
			pr_error("illegal demux source %d\n", src);
			ret = -EINVAL;
		break;
	}
	
	if(ret>0) {
		dmx_reset_hw(dvb);
	}
	
	spin_unlock_irqrestore(&dmx->slock, flags);
	
	return ret;
}

int aml_stb_hw_set_source(struct aml_dvb *dvb, dmx_source_t src)
{
	int ret = 0;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);

	switch(src) {
		case DMX_SOURCE_FRONT0:
#ifndef CONFIG_AMLOGIC_S2P_TS0
			dvb->stb_source = AM_TS_SRC_TS0;
#else
			dvb->stb_source = AM_TS_SRC_S2P0;
#endif
		break;
		case DMX_SOURCE_FRONT1:
#ifndef CONFIG_AMLOGIC_S2P_TS1
			dvb->stb_source = AM_TS_SRC_TS1;
#else
			dvb->stb_source = AM_TS_SRC_S2P1;
#endif
		break;
		case DMX_SOURCE_DVR0:
			dvb->stb_source = AM_TS_SRC_HIU;
		break;
		default:
			pr_error("illegal demux source %d\n", src);
			ret = -EINVAL;
		break;
	}
	
	if(ret==0)
		dmx_reset_hw(dvb);
	
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return ret;
}

