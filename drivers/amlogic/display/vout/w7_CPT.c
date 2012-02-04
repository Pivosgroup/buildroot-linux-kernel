/*
 * AMLOGIC T13 LCD panel driver.
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
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/vout/tcon.h>

#ifdef CONFIG_SN7325
#include <linux/sn7325.h>
#endif

#include <mach/gpio.h>
#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <mach/power_gate.h>

#define LCD_WIDTH       800 
#define LCD_HEIGHT      600
#define MAX_WIDTH       1010
#define MAX_HEIGHT      660
#define VIDEO_ON_LINE   22

static void t13_power_on(void);
static void t13_power_off(void);
void power_on_backlight(void);
void power_off_backlight(void);
    
static tcon_conf_t tcon_config =
{
    .width      = LCD_WIDTH,
    .height     = LCD_HEIGHT,
    .max_width  = MAX_WIDTH,
    .max_height = MAX_HEIGHT,
    .video_on_line = VIDEO_ON_LINE,
    .pll_ctrl = 0x063c,
    .clk_ctrl = 0x1fc1,
    .gamma_cntl_port = (1 << LCD_GAMMA_EN) | (0 << LCD_GAMMA_RVS_OUT) | (1 << LCD_GAMMA_VCOM_POL),
    .gamma_vcom_hswitch_addr = 0,
    .rgb_base_addr = 0xf0,
    .rgb_coeff_addr = 0x74a,
    .pol_cntl_addr = (0x0 << LCD_CPH1_POL) |(0x1 << LCD_HS_POL) | (0x1 << LCD_VS_POL),
    .dith_cntl_addr = 0x400,
    .sth1_hs_addr = 16,
    .sth1_he_addr = 986,
    .sth1_vs_addr = 0,
    .sth1_ve_addr = MAX_HEIGHT - 1,
    .sth2_hs_addr = 0,
    .sth2_he_addr = 0,
    .sth2_vs_addr = 0,
    .sth2_ve_addr = 0,
    .oeh_hs_addr = 67,
    .oeh_he_addr = 67+LCD_WIDTH,
    .oeh_vs_addr = VIDEO_ON_LINE,
    .oeh_ve_addr = VIDEO_ON_LINE+LCD_HEIGHT-1,
    .vcom_hswitch_addr = 0,
    .vcom_vs_addr = 0,
    .vcom_ve_addr = 0,
    .cpv1_hs_addr = 0,
    .cpv1_he_addr = 0,
    .cpv1_vs_addr = 0,
    .cpv1_ve_addr = 0,
    .cpv2_hs_addr = 0,
    .cpv2_he_addr = 0,
    .cpv2_vs_addr = 0,
    .cpv2_ve_addr = 0,
    .stv1_hs_addr = 0,
    .stv1_he_addr = MAX_WIDTH - 1,
    .stv1_vs_addr = 646,
    .stv1_ve_addr = 643,
    .stv2_hs_addr = 0,
    .stv2_he_addr = 0,
    .stv2_vs_addr = 0,
    .stv2_ve_addr = 0,
    .oev1_hs_addr = 0,
    .oev1_he_addr = 0,
    .oev1_vs_addr = 0,
    .oev1_ve_addr = 0,
    .oev2_hs_addr = 0,
    .oev2_he_addr = 0,
    .oev2_vs_addr = 0,
    .oev2_ve_addr = 0,
    .oev3_hs_addr = 0,
    .oev3_he_addr = 0,
    .oev3_vs_addr = 0,
    .oev3_ve_addr = 0,
    .inv_cnt_addr = (0<<LCD_INV_EN) | (0<<LCD_INV_CNT),
    .tcon_misc_sel_addr = (1<<LCD_STV1_SEL) | (1<<LCD_STV2_SEL),
    .dual_port_cntl_addr = (1<<LCD_TTL_SEL) | (1<<LCD_ANALOG_SEL_CPH3) | (1<<LCD_ANALOG_3PHI_CLK_SEL) | (1<<1) | (1<<0),
    .flags = 0,
    .screen_width = 4,
    .screen_height = 3,
    .sync_duration_num = 60,
    .sync_duration_den = 1,
    .power_on=t13_power_on,
    .power_off=t13_power_off,
    .backlight_on = power_on_backlight,
    .backlight_off = power_off_backlight,
};
static struct resource tcon_resources[] = {
    [0] = {
        .start = (ulong)&tcon_config,
        .end   = (ulong)&tcon_config + sizeof(tcon_conf_t) - 1,
        .flags = IORESOURCE_MEM,
    },
};

static void t13_setup_gama_table(tcon_conf_t *pConf)
{

    int i;
    const unsigned short gamma_adjust_R[256] = {
        0,1,2,3,4,5,6,7,9,10,11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,30,31,32,
		33,34,35,35,36,37,38,39,39,40,41,41,42,43,43,44,45,45,46,47,48,48,49,49,50,51,52,52,53,53,54,55,
		56,56,57,58,59,59,60,61,62,62,63,64,65,65,66,67,68,69,70,71,71,72,73,74,74,75,76,77,78,79,80,80,
		81,82,83,83,84,85,86,87,88,88,89,90,91,92,93,94,95,97,97,99,100,101,102,103,104,105,106,107,108,109,110,111,
		113,114,115,117,118,120,122,123,126,128,130,132,134,135,137,139,140,141,142,143,144,146,147,148,149,150,151,152,154,155,156,158,
		159,160,161,163,164,165,166,167,168,169,170,171,172,173,174,175,176,178,179,180,182,183,185,187,189,190,192,194,195,196,198,199,
		200,201,201,202,203,204,205,206,206,207,208,208,209,209,210,211,212,212,213,214,215,215,216,217,217,218,219,219,220,221,222,222,
		223,223,224,224,225,226,226,227,227,228,228,229,229,230,230,231,231,232,233,234,235,236,237,238,239,241,243,245,247,249,252,255
    };

	const unsigned short gamma_adjust_G[256] = {
        0,1,2,3,4,5,6,7,9,10,11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,30,31,32,
		33,34,35,35,36,37,38,39,39,40,41,41,42,43,43,44,45,45,46,47,48,48,49,49,50,51,52,52,53,53,54,55,
		56,56,57,58,59,59,60,61,62,62,63,64,65,65,66,67,68,69,70,71,71,72,73,74,74,75,76,77,78,79,80,80,
		81,82,83,83,84,85,86,87,88,88,89,90,91,92,93,94,95,97,97,99,100,101,102,103,104,105,106,107,108,109,110,111,
		113,114,115,117,118,120,122,123,126,128,130,132,134,135,137,139,140,141,142,143,144,146,147,148,149,150,151,152,154,155,156,158,
		159,160,161,163,164,165,166,167,168,169,170,171,172,173,174,175,176,178,179,180,182,183,185,187,189,190,192,194,195,196,198,199,
		200,201,201,202,203,204,205,206,206,207,208,208,209,209,210,211,212,212,213,214,215,215,216,217,217,218,219,219,220,221,222,222,
		223,223,224,224,225,226,226,227,227,228,228,229,229,230,230,231,231,232,233,234,235,236,237,238,239,241,243,245,247,249,252,255
    };
	
	const unsigned short gamma_adjust_B[256] = {
        0,0,1,2,3,4,5,6,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,28,29,30,
		31,31,32,32,33,34,35,36,36,37,38,38,39,40,40,41,42,42,43,44,45,45,46,46,47,47,48,48,49,49,50,51,
		52,52,53,54,55,55,56,57,58,58,59,60,61,61,62,62,63,64,65,66,66,67,68,69,69,70,71,72,73,74,75,75,
		76,77,78,78,78,79,80,81,82,82,83,84,85,86,87,88,89,91,91,93,94,94,95,96,97,98,99,100,101,102,103,104,
		106,107,108,109,110,112,114,115,118,120,122,124,125,126,128,130,131,132,133,134,135,137,138,139,140,141,141,142,144,145,146,148,
		149,150,151,153,154,155,156,156,157,158,159,160,161,162,163,164,165,167,168,169,171,172,173,175,177,178,180,182,183,184,186,187,
		188,188,188,189,190,191,192,193,193,194,195,195,196,196,197,198,199,199,200,201,202,202,203,203,203,204,205,205,206,207,208,208,
		209,209,210,210,211,212,212,213,213,214,214,215,215,216,216,217,217,218,219,219,220,221,222,223,224,226,228,230,232,234,236,239
    }; //0.94

    for (i=0; i<256; i++) {
        pConf->GammaTableR[i] = gamma_adjust_R[i] << 2;
        pConf->GammaTableG[i] = gamma_adjust_G[i] << 2;
        pConf->GammaTableB[i] = gamma_adjust_B[i] << 2;
    }
}

void power_on_backlight(void)
{
    //BL_PWM -> GPIOA_7: 1
    set_gpio_val(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), 1);
    set_gpio_mode(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), GPIO_OUTPUT_MODE);
}

void power_off_backlight(void)
{
    //BL_PWM -> GPIOA_7: 0
    set_gpio_val(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), 0);
    set_gpio_mode(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), GPIO_OUTPUT_MODE);
    
		clear_mio_mux(0, ((1<<11)|(1<<14)));
    clear_mio_mux(4,(3<<0)|(3<<2)|(3<<4));   //For 8bits
        
}
#define CONFIG_AMLOGIC_ENABLE_SPREAD
#ifdef CONFIG_AMLOGIC_ENABLE_SPREAD
static void Enable_Spread_Spectrum(void)
{
//0xc110413c[7:4]: PLL_CMOD_CTR
//0xc1104160[3:0]: CMOD_CLK_SEL
//0xc1104160[9]: SSEN

#if 0  //3.4%
   WRITE_CBUS_REG(HHI_VID_PLL_CNTL2,0x60);
  printk("\n\nEnable spread 3.4\n\n"); 
#elif 1  //2.6%
  WRITE_CBUS_REG(HHI_VID_PLL_CNTL2,0x40);
  printk("\n\nEnable spread 2.6\%\n\n");
#else  //1.3
  WRITE_CBUS_REG(HHI_VID_PLL_CNTL2,0x20);
  printk("\n\nEnable spread 1.3\n\n");
#endif

   WRITE_CBUS_REG(HHI_VID_PLL_CNTL3,0x02);
   WRITE_CBUS_REG(HHI_VID_PLL_CNTL3,READ_CBUS_REG(HHI_VID_PLL_CNTL3)|(1<<8));  
}

static void disable_Spread_Spectrum(void)
{
//0xc110413c[7:4]: PLL_CMOD_CTR
//0xc1104160[3:0]: CMOD_CLK_SEL
//0xc1104160[9]: SSEN


   WRITE_CBUS_REG(HHI_VID_PLL_CNTL2,0x10);

   CLEAR_CBUS_REG_MASK(HHI_VID_PLL_CNTL3, (1<<8));

}
#endif

static void power_on_lcd(void)
{
#ifdef CONFIG_AMLOGIC_ENABLE_SPREAD    
    udelay(20); //delay 2us
		Enable_Spread_Spectrum();
		udelay(20); //delay 2us
#endif


    //EIO -> OD0: 0  lcd 3.3v
#ifdef CONFIG_SN7325
    configIO(0, 0);
    setIO_level(0, 0, 0);
#endif

    msleep(10);
    
    //EIO -> OD4: 1  VCCx3_EN
#ifdef CONFIG_SN7325
    configIO(0, 0);
    setIO_level(0, 1, 4);
#endif
    msleep(100);
    
    //VCCx2_EN D17 GPIOA_6 --> H
    set_gpio_val(GPIOA_bank_bit(6), GPIOA_bit_bit0_14(6), 1);
    set_gpio_mode(GPIOA_bank_bit(6), GPIOA_bit_bit0_14(6), GPIO_OUTPUT_MODE);
}

static void power_off_lcd(void)
{
#ifdef CONFIG_AMLOGIC_ENABLE_SPREAD    
    udelay(20); //delay 2us
		disable_Spread_Spectrum();
		udelay(20); //delay 2us
#endif

    power_off_backlight();
    
    //EIO -> OD4: 0  VCCx3_EN
#ifdef CONFIG_SN7325
    configIO(0, 0);
    setIO_level(0, 0, 4);
#endif
    msleep(100);
    
    //EIO -> OD0: 1  lcd 3.3v
#ifdef CONFIG_SN7325
    configIO(0, 0);
    setIO_level(0, 1, 0);
#endif

    msleep(10);    

//    //VCCx2_EN D17 GPIOA_6 --> H
//    set_gpio_val(GPIOA_bank_bit(6), GPIOA_bit_bit0_14(6), 0);
//    set_gpio_mode(GPIOA_bank_bit(6), GPIOA_bit_bit0_14(6), GPIO_OUTPUT_MODE);
 //   msleep(10);
   //EIO -> OD7: 1
//#ifdef CONFIG_SN7325
//    configIO(0, 0);
//    setIO_level(0, 1, 7);
//#endif
}

static void set_tcon_pinmux(void)
{
    /* TCON control pins pinmux */
    /* GPIOA_5 -> LCD_Clk, GPIOA_0 -> TCON_STH1, GPIOA_1 -> TCON_STV1, GPIOA_2 -> TCON_OEH, */
    set_mio_mux(0, ((1<<11)|(1<<14)));
    set_mio_mux(4,(3<<0)|(3<<2)|(3<<4));   //For 8bits
    //PP1 -> UPDN:0, PP2 -> SHLR:1
#ifdef CONFIG_SN7325
    configIO(1, 0);
    setIO_level(1, 0, 1);
    setIO_level(1, 1, 2);
#endif
}
static void t13_power_on(void)
{
    video_dac_disable();
	set_tcon_pinmux();
	power_on_lcd();
    printk("\n\nt13_power_on...\n\n");
}
static void t13_power_off(void)
{
    	power_off_lcd();
}

static void t13_io_init(void)
{
    printk("\n\nT13 LCD Init.\n\n");

    set_tcon_pinmux();
    power_on_lcd();
}

static struct platform_device tcon_dev = {
    .name = "tcon-dev",
    .id   = 0,
    .num_resources = ARRAY_SIZE(tcon_resources),
    .resource      = tcon_resources,
};

static int __init t13_init(void)
{
    t13_setup_gama_table(&tcon_config);
    t13_io_init();

    platform_device_register(&tcon_dev);

    return 0;
}

static void __exit t13_exit(void)
{
    power_off_backlight();
    power_off_lcd();

    platform_device_unregister(&tcon_dev);
}

subsys_initcall(t13_init);
//module_init(t13_init);
module_exit(t13_exit);

MODULE_DESCRIPTION("AMLOGIC T13 LCD panel driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");



