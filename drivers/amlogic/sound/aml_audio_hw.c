#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <mach/am_regs.h>
#include <linux/clk.h>

#include "aml_audio_hw.h"

#ifndef MREG_AIU_958_chstat0
#define AIU_958_chstat0	AIU_958_CHSTAT_L0
#endif

#ifndef MREG_AIU_958_chstat1
#define AIU_958_chstat1	AIU_958_CHSTAT_L1
#endif
unsigned ENABLE_IEC958 = 0;

int audio_clock_config_table[][5][2]=
{
	{
	//24M
		{(71 <<  0) |(4   <<  9) | (1   << 14), (26-1)},//32K	
		{(143 <<  0) |(8   <<  9) | (1   << 14), (19-1)},//44.1K	
		{(128 <<  0) |(5   <<  9) | (1   << 14), (25-1)},//48K	
		{(128 <<  0) |(5   <<  9) | (0   << 14), (25-1)},//96K	
		{(213 <<  0) |(8   <<  9) | (0   << 14), (13-1)}//192K	    
	},
	{
	//25M
		{(19 <<  0) |(1   <<  9) | (1   << 14), (29-1)},//32K	
		{(28 <<  0) |(1   <<  9) | (1   << 14), (31-1)},//44.1K	
		{(173 <<  0) |(8   <<  9) | (1   << 14), (22-1)},//48K	
		{(173 <<  0) |(8   <<  9) | (1   << 14), (11-1)},//96K	
		{(118 <<  0) |(5   <<  9) | (1   << 14), (6-1)}//192K	               
	}
};


void audio_set_aiubuf(u32 addr, u32 size)
{
    WRITE_MPEG_REG(AIU_MEM_I2S_START_PTR, addr & 0xffffffc0);
    WRITE_MPEG_REG(AIU_MEM_I2S_RD_PTR, addr & 0xffffffc0);
    WRITE_MPEG_REG(AIU_MEM_I2S_END_PTR, (addr & 0xffffffc0) + (size & 0xffffffc0) - 64);   //this is for 16bit 2 channel
#if 0
    WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 1, 0, 1);
    WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 0, 0, 1);

    WRITE_MPEG_REG(AIU_MEM_I2S_BUF_CNTL, 1 | (0 << 1));
    WRITE_MPEG_REG(AIU_MEM_I2S_BUF_CNTL, 0 | (0 << 1));
#else
	WRITE_MPEG_REG(AIU_I2S_MISC,		0x0004);	// Hold I2S
	WRITE_MPEG_REG(AIU_I2S_MUTE_SWAP,	0x0000);	// No mute, no swap
	WRITE_MPEG_REG(AIU_I2S_DAC_CFG, 	0x000f);	// Payload 24-bit, Msb first, alrclk = aoclk/64
	WRITE_MPEG_REG(AIU_I2S_SOURCE_DESC, 0x0001);	// four 2-channel
	// As the default amclk is 24.576MHz, set i2s and iec958 divisor appropriately so as not to exceed the maximum sample rate.
	WRITE_MPEG_REG(AIU_I2S_MISC,		0x0010 );	// Release hold and force audio data to left or right
	
	WRITE_MPEG_REG(AIU_MEM_I2S_MASKS,		(24 << 16) |	// [31:16] IRQ block.
								(0x3 << 8) |	// [15: 8] chan_mem_mask. Each bit indicates which channels exist in memory
								(0x3 << 0));	// [ 7: 0] chan_rd_mask.  Each bit indicates which channels are READ from memory
	// Set init high then low to initilize the I2S memory logic
	WRITE_MPEG_REG(AIU_MEM_I2S_CONTROL, 	1 );
	WRITE_MPEG_REG(AIU_MEM_I2S_CONTROL, 	0 );
	// Enable the I2S FIFO (enable both the empty and fill modules)
	WRITE_MPEG_REG(AIU_MEM_I2S_CONTROL, 	6|(1<<6) );

#endif
}

void audio_set_958outbuf(u32 addr, u32 size)
{
    if (ENABLE_IEC958) {
        WRITE_MPEG_REG(AIU_MEM_IEC958_START_PTR, addr & 0xffffffc0);
        WRITE_MPEG_REG(AIU_MEM_IEC958_RD_PTR, addr & 0xffffffc0);
        WRITE_MPEG_REG(AIU_MEM_IEC958_END_PTR, (addr & 0xffffffc0) + (size & 0xffffffc0) - 64);    // this is for 16bit 2 channel

        WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 1, 0, 1);
        WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 0, 1);

        WRITE_MPEG_REG(AIU_MEM_IEC958_BUF_CNTL, 1 | (0 << 1));
        WRITE_MPEG_REG(AIU_MEM_IEC958_BUF_CNTL, 0 | (0 << 1));
    }
}

void audio_set_i2s_mode(u32 mode)
{
    const unsigned short control[4] = {
        0x14,                   /* AIU_I2S_MODE_2x16 */
        0x30,                   /* AIU_I2S_MODE_2x24 */
        0x11                    /* AIU_I2S_MODE_8x24 */
    };

    const unsigned short mask[4] = {
        0x303,                  /*2 ch in, 2ch out */
        0x303,                  /*2ch in, 2ch out */
        0xffff                  /*8ch in, 8ch out */
    };

    if (mode < sizeof(control) / sizeof(unsigned short)) {
        WRITE_MPEG_REG(AIU_I2S_SOURCE_DESC, 1);

        if (mode == 0) {
            WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 1, 6, 1);
        } else {
            WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 0, 6, 1);
        }
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_MASKS, mask[mode], 0, 16);

        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 1, 0, 1);
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 0, 0, 1);

        if (ENABLE_IEC958) {
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_MASKS, mask[mode], 0,
                                16);
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 1, 0, 1);
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 0, 1);
        }
    }

}

const _aiu_clk_setting_t freq_tab_384fs[7] = {
    {AUDIO_384FS_PLL_192K, AUDIO_384FS_PLL_192K_MUX, AUDIO_384FS_CLK_192K},
    {AUDIO_384FS_PLL_176K, AUDIO_384FS_PLL_176K_MUX, AUDIO_384FS_CLK_176K},
    {AUDIO_384FS_PLL_96K, AUDIO_384FS_PLL_96K_MUX, AUDIO_384FS_CLK_96K},
    {AUDIO_384FS_PLL_88K, AUDIO_384FS_PLL_88K_MUX, AUDIO_384FS_CLK_88K},
    {AUDIO_384FS_PLL_48K, AUDIO_384FS_PLL_48K_MUX, AUDIO_384FS_CLK_48K},
    {AUDIO_384FS_PLL_44K, AUDIO_384FS_PLL_44K_MUX, AUDIO_384FS_CLK_44K},
    {AUDIO_384FS_PLL_32K, AUDIO_384FS_PLL_32K_MUX, AUDIO_384FS_CLK_32K}
};

const _aiu_clk_setting_t freq_tab_256fs[7] = {
    {AUDIO_256FS_PLL_192K, AUDIO_256FS_PLL_192K_MUX, AUDIO_256FS_CLK_192K},
    {AUDIO_256FS_PLL_176K, AUDIO_256FS_PLL_176K_MUX, AUDIO_256FS_CLK_176K},
    {AUDIO_256FS_PLL_96K, AUDIO_256FS_PLL_96K_MUX, AUDIO_256FS_CLK_96K},
    {AUDIO_256FS_PLL_88K, AUDIO_256FS_PLL_88K_MUX, AUDIO_256FS_CLK_88K},
    {AUDIO_256FS_PLL_48K, AUDIO_256FS_PLL_48K_MUX, AUDIO_256FS_CLK_48K},
    {AUDIO_256FS_PLL_44K, AUDIO_256FS_PLL_44K_MUX, AUDIO_256FS_CLK_44K},
    {AUDIO_256FS_PLL_32K, AUDIO_256FS_PLL_32K_MUX, AUDIO_256FS_CLK_32K}
};

void audio_util_set_dac_format(unsigned format)
{
    if (format == AUDIO_ALGOUT_DAC_FORMAT_DSP) {
        WRITE_MPEG_REG_BITS(AIU_CLK_CTRL, 1, 8, 2);
    } else if (format == AUDIO_ALGOUT_DAC_FORMAT_LEFT_JUSTIFY) {
        WRITE_MPEG_REG_BITS(AIU_CLK_CTRL, 0, 8, 2);
    }

}
void latch (void)
{
    int latch;
    latch = 1;
    WRITE_APB_REG(ADAC_LATCH, latch);
    latch = 0;
    WRITE_APB_REG(ADAC_LATCH, latch);
}

void wr_adac_regbank (unsigned long rstdpz,
                  unsigned long mclksel,
                  unsigned long i2sfsdac,
                  unsigned long i2ssplit,
                  unsigned long i2smode,
                  unsigned long pdauxdrvrz,
                  unsigned long pdauxdrvlz,
                  unsigned long pdhsdrvrz,
                  unsigned long pdhsdrvlz,
                  unsigned long pddacrz,
                  unsigned long pddaclz,
                  unsigned long pdz,
                  unsigned long hsmute,
                  unsigned long lmmute,
                  unsigned long lmmix,
                  unsigned long ctr,
                  unsigned long lmvol,
                  unsigned long hsvol)
{
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    WRITE_APB_REG(ADAC_RESET, (rstdpz<<1));
    
    WRITE_APB_REG(ADAC_CLOCK, (mclksel<<0));
   
    WRITE_APB_REG(ADAC_I2S_CONFIG_REG1, (i2sfsdac<<0));
   
    WRITE_APB_REG(ADAC_I2S_CONFIG_REG2, (i2ssplit<<3) | (i2smode<<0));
    WRITE_APB_REG(ADAC_POWER_CTRL_REG1, (pdauxdrvrz<<7) | (pdauxdrvlz<<6) | (pdhsdrvrz<<5) | (pdhsdrvlz<<4) | (pddacrz<<1) | (pddaclz<<0));

    WRITE_APB_REG(ADAC_POWER_CTRL_REG2, (pdz<<7));
    WRITE_APB_REG(ADAC_MUTE_CTRL_REG1, (hsmute<<6) | (lmmute<<0));
    WRITE_APB_REG(ADAC_DAC_ADC_MIXER, (lmmix<<5) | (ctr<<1));
    WRITE_APB_REG(ADAC_PLAYBACK_VOL_CTRL_LSB, (lmvol&0xff));
    WRITE_APB_REG(ADAC_PLAYBACK_VOL_CTRL_MSB, (lmvol>>8));
    WRITE_APB_REG(ADAC_STEREO_HS_VOL_CTRL_LSB, (hsvol&0xff));
    WRITE_APB_REG(ADAC_STEREO_HS_VOL_CTRL_MSB, (hsvol>>8));    
} /* wr_regbank */

int audio_dac_set(unsigned freq)
{

	unsigned long   data32;             
   	int i=0;
      // Configure audio DAC control interface
    data32  = 0;
    data32 |= 0 << 15;  // [15]     audac_soft_reset_n
    data32 |= 0 << 14;  // [14]     audac_reset_ctrl: 0=use audac_reset_n pulse from reset module; 1=use audac_soft_reset_n.
    data32 |= 0 << 9;   // [9]      delay_rd_en
    data32 |= 0 << 8;   // [8]      audac_reg_clk_inv
    data32 |= 0 << 1;   // [7:1]    audac_i2caddr
    data32 |= 0 << 0;   // [0]      audac_intfsel: 0=use host bus; 1=use I2C.
    WRITE_MPEG_REG(AIU_AUDAC_CTRL0, data32);
    
     // Enable APB3 fail on error
    data32  = 0;
    data32 |= 1     << 15;  // [15]     err_en
    data32 |= 255   << 0;   // [11:0]   max_err
    WRITE_MPEG_REG(AIU_AUDAC_CTRL1, data32);
	switch(freq)
	{
		case AUDIO_CLK_FREQ_192:
			data32=11;
			break;
		case AUDIO_CLK_FREQ_96:
			data32=10;
			break;
		case AUDIO_CLK_FREQ_48:
			data32=8;
			break;
		case AUDIO_CLK_FREQ_441:
			data32=7;
			break;
		case AUDIO_CLK_FREQ_32:
			data32=6;
			break;
		default:
			data32=6;
			break;
	};
    wr_adac_regbank(0,          // rstdpz: active low.
	                0,          // mclksel[3:0]: master clock freq sel. 0=256Fs, 1=384Fs, ... 
	                data32,         // i2sfsdac[3:0]: sample freq sel. 0=8kHz, 1=11.025k, 2=12k, 3=16k, 4=22.05k, 5=24k, 6=32k, 7=44.1k, 8=48k, 9=88.2k, 10=96k, 11=192k, >11=Rsrv.
	                0,          // i2ssplit
	                1,          // i2smode[2:0]: Data format sel. 0=Right justify, 1=I2S, 2=Left justify, 3=Burst1, 4=Burst2, 5=Mono burst1, 6=Mono burst2, 7=Rsrv.
	                1,          // pdauxdrvrz: active low.
	                1,          // pdauxdrvlz: active low.
	                1,          // pdhsdrvrz: active low.
	                1,          // pdhsdrvlz: active low.
	                1,          // pddacrz: active low.
	                1,          // pddaclz: active low.
	                0,          // pdz: active low.
	                0,          // hsmute[1:0]: bit[1] Analog playback right channel mute, bit[0] Analog playback left channel mute.
	                0,          // lmmute[1:0]: bit[1] Digital playback right channel mute, bit[0] Digital playback left channel mute.
	                0,          // lmmix: Digital mixer sel.
	                0,          // ctr[1:0]: test mode sel. 0=normal mode, 2=Digital filter bypass, 1 or 3=Rsrv.
	                0x5454,     // lmvol[15:0]: Digital volumn control, [15:8] control right channel, [7:0] control left channel.
	                            // 0=-126dB, 1=-124.5dB, ..., 0x53=-1.5dB, 0x54=0dB, >=0x55 Rsrv.
	                0x2828);    // hsvol[15:0]: Analog headset volumn control, [15:8] control right channel, [7:0] control left channel.
	                            // 0=-40dB, 1=-39dB, ..., 0x28=0dB, ..., 0x2e=6dB, >=0x2f Rsrv.

    latch();
	WRITE_APB_REG(ADAC_POWER_CTRL_REG2, (0<<7));
    latch();
	WRITE_APB_REG(ADAC_POWER_CTRL_REG2, (1<<7));
    latch();

	WRITE_APB_REG(ADAC_RESET, (0<<1));
    latch();
	WRITE_APB_REG(ADAC_RESET, (1<<1));
    latch();
	for (i = 0; i < 1500000; i++) ;
	for (i = 0; i < 1500000; i++) ;
	for (i = 0; i < 1500000; i++) ;
	for (i = 0; i < 1500000; i++) ;
	
	WRITE_MPEG_REG(AIU_CLK_CTRL_MORE,	 0x0000);	 // i2s_divisor_more does not take effect
	WRITE_MPEG_REG(AIU_CLK_CTRL,		 (0 << 12) | // 958 divisor more, if true, divided by 2, 4, 6, 8.
							(1 <<  8) | // alrclk skew: 1=alrclk transitions on the cycle before msb is sent
							(1 <<  6) | // invert aoclk
							(1 <<  4) | // 958 divisor: 0=no div; 1=div by 2; 2=div by 3; 3=div by 4.
							(2 <<  2) | // i2s divisor: 0=no div; 1=div by 2; 2=div by 4; 3=div by 8.
							(1 <<  0)); // enable I2S clock
    //delay_us(1500000); // The IP's behavioral model needs 1.5s to power-up.
    return 0;
}

void audio_set_clk(unsigned freq, unsigned fs_config)
{
    int i;
    struct clk *clk;
    int xtal = 0;
    
    int (*audio_clock_config)[2];
    
   // if (fs_config == AUDIO_CLK_256FS) {
   if(1){
#if 0
        //WRITE_MPEG_REG_BITS(MREG_AUDIO_CLK_CTRL, 0, 8, 1);
        WRITE_MPEG_REG(HHI_AUD_PLL_CNTL, freq_tab_256fs[freq].pll);
        for (i = 0; i < 100000; i++) ;
        WRITE_MPEG_REG(HHI_AUD_CLK_CNTL, freq_tab_256fs[freq].mux);
        WRITE_MPEG_REG_BITS(HHI_AUD_CLK_CNTL, 1, 8, 1);

        WRITE_MPEG_REG_BITS(AIU_CLK_CTRL,
                            freq_tab_256fs[freq].devisor, 0, 8);
        WRITE_MPEG_REG(AIU_I2S_DAC_CFG, AUDIO_256FS_DAC_CFG);
#else
#define AUDIO_CLK_FREQ_192  0
#define AUDIO_CLK_FREQ_1764 1
#define AUDIO_CLK_FREQ_96   2
#define AUDIO_CLK_FREQ_882  3
#define AUDIO_CLK_FREQ_48   4
#define AUDIO_CLK_FREQ_441  5
#define AUDIO_CLK_FREQ_32   6

	int index=0;
	switch(freq)
	{
		case AUDIO_CLK_FREQ_192:
			index=4;
			break;
		case AUDIO_CLK_FREQ_96:
			index=3;
			break;
		case AUDIO_CLK_FREQ_48:
			index=2;
			break;
		case AUDIO_CLK_FREQ_441:
			index=1;
			break;
		case AUDIO_CLK_FREQ_32:
			index=0;
			break;
		default:
			index=0;
			break;
	};
	// get system crystal freq
		clk=clk_get_sys("clk_xtal", NULL);
		if(!clk)
		{
			printk(KERN_ERR "can't find clk %s for AUDIO PLL SETTING!\n\n","clk_xtal");
			//return -1;
		}
		else
		{
			xtal=clk_get_rate(clk);
			xtal=xtal/1000000;
			if(xtal>=24 && xtal <=25)/*current only support 24,25*/
			{
				xtal-=24;
			}
			else
			{
				printk(KERN_WARNING "UNsupport xtal setting for audio xtal=%d,default to 24M\n",xtal);	
				xtal=0;
			}
		}
		
		audio_clock_config = audio_clock_config_table[xtal];
	
    // gate the clock off
    WRITE_MPEG_REG( HHI_AUD_CLK_CNTL, READ_MPEG_REG(HHI_AUD_CLK_CNTL) & ~(1 << 8));

    // Put the PLL to sleep
    WRITE_MPEG_REG( HHI_AUD_PLL_CNTL, READ_MPEG_REG(HHI_AUD_PLL_CNTL) | (1 << 15));

    // Bring out of reset but keep bypassed to allow to stablize
    //Wr( HHI_AUD_PLL_CNTL, (1 << 15) | (0 << 14) | (hiu_reg & 0x3FFF) );
    WRITE_MPEG_REG( HHI_AUD_PLL_CNTL, (1 << 15) | (audio_clock_config[index][0] & 0x7FFF) );
    // Set the XD value
    WRITE_MPEG_REG( HHI_AUD_CLK_CNTL, (READ_MPEG_REG(HHI_AUD_CLK_CNTL) & ~(0xff << 0)) | audio_clock_config[index][1]);
    // delay 5uS
	//udelay(5);
	for (i = 0; i < 500000; i++) ;
    // Bring the PLL out of sleep
    WRITE_MPEG_REG( HHI_AUD_PLL_CNTL, READ_MPEG_REG(HHI_AUD_PLL_CNTL) & ~(1 << 15));

    // gate the clock on
    WRITE_MPEG_REG( HHI_AUD_CLK_CNTL, READ_MPEG_REG(HHI_AUD_CLK_CNTL) | (1 << 8)|(1<<23));
    
    // delay 2uS
	//udelay(2);
	for (i = 0; i < 200000; i++) ;

#endif
		
    } else if (fs_config == AUDIO_CLK_384FS) {
        //WRITE_MPEG_REG_BITS(MREG_AUDIO_CLK_CTRL, 0, 8, 1);
        WRITE_MPEG_REG(HHI_AUD_PLL_CNTL, freq_tab_384fs[freq].pll);
        for (i = 0; i < 100000; i++) ;
        WRITE_MPEG_REG(HHI_AUD_CLK_CNTL, freq_tab_384fs[freq].mux);
        WRITE_MPEG_REG_BITS(HHI_AUD_CLK_CNTL, 1, 8, 1);
        WRITE_MPEG_REG_BITS(AIU_CLK_CTRL,
                            freq_tab_384fs[freq].devisor, 0, 8);
        WRITE_MPEG_REG(AIU_I2S_DAC_CFG, AUDIO_384FS_DAC_CFG);
    }
}

void audio_enable_ouput(int flag)
{
    if (flag) {

        WRITE_MPEG_REG(AIU_RST_SOFT, 0x05);
        READ_MPEG_REG(AIU_I2S_SYNC);
		//what about this reg mean?
       // WRITE_MPEG_REG_BITS(DDR_TOP_CTL2, 3, 3, 2);
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 3, 1, 2);

        if (ENABLE_IEC958) {
            READ_MPEG_REG(AIU_I2S_SYNC);
            WRITE_MPEG_REG(AIU_958_FORCE_LEFT, 0);
            WRITE_MPEG_REG(AIU_958_DCU_FF_CTRL, 1);

            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 3, 1, 2);
        }
    } else {
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 0, 1, 2);
//              WRITE_MPEG_REG_BITS(MREG_AIU_MEM_I2S_CONTROL, 1, 0, 1);

        if (ENABLE_IEC958) {
            WRITE_MPEG_REG(AIU_958_DCU_FF_CTRL, 0);
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 1, 2);
//              WRITE_MPEG_REG_BITS(MREG_AIU_MEM_IEC958_CONTROL, 1, 0, 1);
        }
    }
}

unsigned int read_i2s_rd_ptr(void)
{
    unsigned int val;
    val = READ_MPEG_REG(AIU_MEM_I2S_RD_PTR);
    return val;
}

void audio_i2s_unmute(void)
{
    WRITE_MPEG_REG_BITS(AIU_I2S_MUTE_SWAP, 0, 8, 8);
    WRITE_MPEG_REG_BITS(AIU_958_CTRL, 0, 3, 2);
}

void audio_i2s_mute(void)
{
    WRITE_MPEG_REG_BITS(AIU_I2S_MUTE_SWAP, 0xff, 8, 8);
    WRITE_MPEG_REG_BITS(AIU_958_CTRL, 3, 3, 2);
}

void audio_hw_958_reset(unsigned slow_domain, unsigned fast_domain)
{
    WRITE_MPEG_REG(AIU_RST_SOFT,
                   (slow_domain << 3) | (fast_domain << 2));
}

void set_958_channel_status(_aiu_958_channel_status_t * set)
{
    if (set) {
        WRITE_MPEG_REG(AIU_958_chstat0, set->chstat0_l);
        WRITE_MPEG_REG(AIU_958_chstat1, set->chstat1_l);
        WRITE_MPEG_REG(AIU_958_CHSTAT_L0, set->chstat0_r);
        WRITE_MPEG_REG(AIU_958_CHSTAT_L1, set->chstat1_r);
    }
}

void audio_hw_set_958_pcm24(_aiu_958_raw_setting_t * set)
{
    WRITE_MPEG_REG(AIU_958_BPF, 0x80); /* in pcm mode, set bpf to 128 */
    set_958_channel_status(set->chan_stat);
}

void audio_hw_set_958_mode(unsigned mode, _aiu_958_raw_setting_t * set)
{
    if (mode == AIU_958_MODE_RAW) {
        // audio_hw_set_958_raw(set);
        if (ENABLE_IEC958) {
            WRITE_MPEG_REG(AIU_958_MISC, 1);
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 1, 8, 1);  // raw
        }
    } else if (mode == AIU_958_MODE_PCM24) {
        audio_hw_set_958_pcm24(set);
        if (ENABLE_IEC958) {
            WRITE_MPEG_REG(AIU_958_MISC, 0x2020 | (1 << 7));
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 8, 1);  // pcm
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 7, 1);  // 16bit
        }
    } else if (mode == AIU_958_MODE_PCM16) {
        audio_hw_set_958_pcm24(set);
        if (ENABLE_IEC958) {
            WRITE_MPEG_REG(AIU_958_MISC, 0x2042);
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 0, 8, 1);  // pcm
            WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, 1, 7, 1);  // 16bit
        }
    }

    audio_hw_958_reset(0, 1);

    WRITE_MPEG_REG(AIU_958_FORCE_LEFT, 1);
}

void audio_hw_958_enable(unsigned flag)
{
    if (ENABLE_IEC958) {
        WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, flag, 2, 1);
        WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, flag, 1, 1);
        WRITE_MPEG_REG_BITS(AIU_MEM_IEC958_CONTROL, flag, 0, 1);
    }
}

unsigned int read_i2s_mute_swap_reg(void)
{
	unsigned int val;
    	val = READ_MPEG_REG(AIU_I2S_MUTE_SWAP);
    	return val;
}

void audio_i2s_swap_left_right(unsigned int flag)
{
	WRITE_MPEG_REG_BITS(AIU_I2S_MUTE_SWAP, flag, 0, 2);
}
