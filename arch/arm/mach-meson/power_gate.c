#include <linux/module.h>
#include <mach/am_regs.h>

#include <mach/power_gate.h>

unsigned char GCLK_ref[GCLK_IDX_MAX];

EXPORT_SYMBOL(GCLK_ref);

int  video_dac_enable(unsigned char enable_mask)
{
    CLEAR_CBUS_REG_MASK(VENC_VDAC_SETTING, enable_mask&0x1f);
	  return 0;
}
EXPORT_SYMBOL(video_dac_enable);

int  video_dac_disable()
{
    SET_CBUS_REG_MASK(VENC_VDAC_SETTING, 0x1f);
    return 0;    
}    
EXPORT_SYMBOL(video_dac_disable);

static void turn_off_audio_DAC (void)
{
    int wr_val;
    wr_val = 0;
    WRITE_APB_REG(ADAC_RESET, wr_val);
    WRITE_APB_REG(ADAC_POWER_CTRL_REG1, wr_val);
    WRITE_APB_REG(ADAC_POWER_CTRL_REG2, wr_val);

    wr_val = 1;
    WRITE_APB_REG(ADAC_LATCH, wr_val);
    wr_val = 0;
    WRITE_APB_REG(ADAC_LATCH, wr_val);
} /* turn_off_audio_DAC */

int audio_internal_dac_disable()
{
    turn_off_audio_DAC();
    return 0;    
}    
EXPORT_SYMBOL(audio_internal_dac_disable);

#define HDMI_ADDR_PORT 0x2000
#define HDMI_DATA_PORT 0x2004
static void hdmi_wr_only_reg_(unsigned long addr, unsigned long data)
{
    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
    
    WRITE_APB_REG(HDMI_DATA_PORT, data);
}

int turn_off_hdmi(void)
{
    WRITE_MPEG_REG( HHI_HDMI_CLK_CNTL,  ((1 << 9)  |   // select "other" PLL
                             (1 << 8)  |   // Enable gated clock
                             (5 << 0)) );  // Divide the "other" PLL output by 6

    hdmi_wr_only_reg_(0xf4, 0x8);
    hdmi_wr_only_reg_(0x11, 0);
    hdmi_wr_only_reg_(0x12, 0);
    hdmi_wr_only_reg_(0x17, 0);
    /**/
    WRITE_MPEG_REG(HHI_HDMI_PLL_CNTL, READ_MPEG_REG(HHI_HDMI_PLL_CNTL)|(1<<30)); //disable HDMI PLL

    WRITE_MPEG_REG( HHI_HDMI_CLK_CNTL,  0);
    printk("turn off hdmi\n");
    return 0;
}    

EXPORT_SYMBOL(turn_off_hdmi);
