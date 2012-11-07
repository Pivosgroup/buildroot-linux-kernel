/*
 * AMLOGIC TCON controller driver.
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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/vout/tcon.h>
#include <linux/vout/vinfo.h>
#include <linux/vout/vout_notify.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/logo/logo.h>
#include <mach/am_regs.h>
#include <asm/fiq.h>

#define FIQ_VSYNC

#define BL_MAX_LEVEL 0x100
#define PANEL_NAME	"panel"

typedef struct {
	tcon_conf_t conf;
	vinfo_t lcd_info;
} tcon_dev_t;

static tcon_dev_t *pDev = NULL;

static void _tcon_init(tcon_conf_t *pConf) ;
static void set_lcd_gamma_table(u16 *data, u32 rgb_mask)
{
    int i;

    while (!(READ_MPEG_REG(GAMMA_CNTL_PORT) & (0x1 << ADR_RDY)));
    WRITE_MPEG_REG(GAMMA_ADDR_PORT, (0x1 << H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x0 << HADR));
    for (i=0;i<256;i++) {
        while (!( READ_MPEG_REG(GAMMA_CNTL_PORT) & (0x1 << WR_RDY) )) ;
        WRITE_MPEG_REG(GAMMA_DATA_PORT, data[i]);
    }
    while (!(READ_MPEG_REG(GAMMA_CNTL_PORT) & (0x1 << ADR_RDY)));
    WRITE_MPEG_REG(GAMMA_ADDR_PORT, (0x1 << H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x23 << HADR));
}

static inline void _init_tcon(tcon_conf_t *pConf)
{
    set_lcd_gamma_table(pConf->GammaTableR, H_SEL_R);
    set_lcd_gamma_table(pConf->GammaTableG, H_SEL_G);
    set_lcd_gamma_table(pConf->GammaTableB, H_SEL_B);

    WRITE_MPEG_REG(GAMMA_CNTL_PORT, pConf->gamma_cntl_port);
    WRITE_MPEG_REG(GAMMA_VCOM_HSWITCH_ADDR, pConf->gamma_vcom_hswitch_addr);

    WRITE_MPEG_REG(RGB_BASE_ADDR,   pConf->rgb_base_addr);
    WRITE_MPEG_REG(RGB_COEFF_ADDR,  pConf->rgb_coeff_addr);
    WRITE_MPEG_REG(POL_CNTL_ADDR,   pConf->pol_cntl_addr);
    WRITE_MPEG_REG(DITH_CNTL_ADDR,  pConf->dith_cntl_addr);

    WRITE_MPEG_REG(STH1_HS_ADDR,    pConf->sth1_hs_addr);
    WRITE_MPEG_REG(STH1_HE_ADDR,    pConf->sth1_he_addr);
    WRITE_MPEG_REG(STH1_VS_ADDR,    pConf->sth1_vs_addr);
    WRITE_MPEG_REG(STH1_VE_ADDR,    pConf->sth1_ve_addr);

    WRITE_MPEG_REG(STH2_HS_ADDR,    pConf->sth2_hs_addr);
    WRITE_MPEG_REG(STH2_HE_ADDR,    pConf->sth2_he_addr);
    WRITE_MPEG_REG(STH2_VS_ADDR,    pConf->sth2_vs_addr);
    WRITE_MPEG_REG(STH2_VE_ADDR,    pConf->sth2_ve_addr);

    WRITE_MPEG_REG(OEH_HS_ADDR,     pConf->oeh_hs_addr);
    WRITE_MPEG_REG(OEH_HE_ADDR,     pConf->oeh_he_addr);
    WRITE_MPEG_REG(OEH_VS_ADDR,     pConf->oeh_vs_addr);
    WRITE_MPEG_REG(OEH_VE_ADDR,     pConf->oeh_ve_addr);

    WRITE_MPEG_REG(VCOM_HSWITCH_ADDR, pConf->vcom_hswitch_addr);
    WRITE_MPEG_REG(VCOM_VS_ADDR,    pConf->vcom_vs_addr);
    WRITE_MPEG_REG(VCOM_VE_ADDR,    pConf->vcom_ve_addr);

    WRITE_MPEG_REG(CPV1_HS_ADDR,    pConf->cpv1_hs_addr);
    WRITE_MPEG_REG(CPV1_HE_ADDR,    pConf->cpv1_he_addr);
    WRITE_MPEG_REG(CPV1_VS_ADDR,    pConf->cpv1_vs_addr);
    WRITE_MPEG_REG(CPV1_VE_ADDR,    pConf->cpv1_ve_addr);

    WRITE_MPEG_REG(CPV2_HS_ADDR,    pConf->cpv2_hs_addr);
    WRITE_MPEG_REG(CPV2_HE_ADDR,    pConf->cpv2_he_addr);
    WRITE_MPEG_REG(CPV2_VS_ADDR,    pConf->cpv2_vs_addr);
    WRITE_MPEG_REG(CPV2_VE_ADDR,    pConf->cpv2_ve_addr);

    WRITE_MPEG_REG(STV1_HS_ADDR,    pConf->stv1_hs_addr);
    WRITE_MPEG_REG(STV1_HE_ADDR,    pConf->stv1_he_addr);
    WRITE_MPEG_REG(STV1_VS_ADDR,    pConf->stv1_vs_addr);
    WRITE_MPEG_REG(STV1_VE_ADDR,    pConf->stv1_ve_addr);

    WRITE_MPEG_REG(STV2_HS_ADDR,    pConf->stv2_hs_addr);
    WRITE_MPEG_REG(STV2_HE_ADDR,    pConf->stv2_he_addr);
    WRITE_MPEG_REG(STV2_VS_ADDR,    pConf->stv2_vs_addr);
    WRITE_MPEG_REG(STV2_VE_ADDR,    pConf->stv2_ve_addr);

    WRITE_MPEG_REG(OEV1_HS_ADDR,    pConf->oev1_hs_addr);
    WRITE_MPEG_REG(OEV1_HE_ADDR,    pConf->oev1_he_addr);
    WRITE_MPEG_REG(OEV1_VS_ADDR,    pConf->oev1_vs_addr);
    WRITE_MPEG_REG(OEV1_VE_ADDR,    pConf->oev1_ve_addr);

    WRITE_MPEG_REG(OEV2_HS_ADDR,    pConf->oev2_hs_addr);
    WRITE_MPEG_REG(OEV2_HE_ADDR,    pConf->oev2_he_addr);
    WRITE_MPEG_REG(OEV2_VS_ADDR,    pConf->oev2_vs_addr);
    WRITE_MPEG_REG(OEV2_VE_ADDR,    pConf->oev2_ve_addr);

    WRITE_MPEG_REG(OEV3_HS_ADDR,    pConf->oev3_hs_addr);
    WRITE_MPEG_REG(OEV3_HE_ADDR,    pConf->oev3_he_addr);
    WRITE_MPEG_REG(OEV3_VS_ADDR,    pConf->oev3_vs_addr);
    WRITE_MPEG_REG(OEV3_VE_ADDR,    pConf->oev3_ve_addr);

    WRITE_MPEG_REG(INV_CNT_ADDR,    pConf->inv_cnt_addr);
    WRITE_MPEG_REG(TCON_MISC_SEL_ADDR, 	pConf->tcon_misc_sel_addr);
    WRITE_MPEG_REG(DUAL_PORT_CNTL_ADDR, pConf->dual_port_cntl_addr);

    CLEAR_MPEG_REG_MASK(VPP_MISC, VPP_OUT_SATURATE);
}

static inline void _init_tvenc(tcon_conf_t *pConf)
{
    WRITE_MPEG_REG(ENCP_VIDEO_FILT_CTRL,    0x1000);
    WRITE_MPEG_REG(VENC_DVI_SETTING,        0x11);
  
    WRITE_MPEG_REG(HHI_VID_PLL_CNTL, pConf->pll_ctrl);
    WRITE_MPEG_REG(HHI_VID_CLK_CNTL, pConf->clk_ctrl);
    WRITE_MPEG_REG(HHI_VID_CLK_DIV, (pConf->clk_ctrl)&0xf);
    WRITE_MPEG_REG(HHI_MPEG_CLK_CNTL, READ_MPEG_REG(HHI_MPEG_CLK_CNTL)|(0<<11));  //[11]=1:aud clk mux to cph 

    WRITE_MPEG_REG(ENCP_VIDEO_MODE,         0x0040);
    WRITE_MPEG_REG(ENCP_VIDEO_MODE_ADV,     0x418);

    WRITE_MPEG_REG(ENCP_VIDEO_YFP1_HTIME,   64);
    WRITE_MPEG_REG(ENCP_VIDEO_YFP2_HTIME,   1056);
    WRITE_MPEG_REG(ENCP_VIDEO_MAX_PXCNT,    pConf->max_width - 1);
    WRITE_MPEG_REG(ENCP_VIDEO_MAX_LNCNT,    pConf->max_height - 1);

    WRITE_MPEG_REG(ENCP_VIDEO_HSPULS_BEGIN, 3245);
    WRITE_MPEG_REG(ENCP_VIDEO_HSPULS_END,   79);
    WRITE_MPEG_REG(ENCP_VIDEO_HSPULS_SWITCH, 80);
    WRITE_MPEG_REG(ENCP_VIDEO_VSPULS_BEGIN, 0);
    WRITE_MPEG_REG(ENCP_VIDEO_VSPULS_END,   3079);
    WRITE_MPEG_REG(ENCP_VIDEO_VSPULS_BLINE, 0);
    WRITE_MPEG_REG(ENCP_VIDEO_VSPULS_ELINE, 4);

    WRITE_MPEG_REG(ENCP_VIDEO_HAVON_BEGIN,  48);
    WRITE_MPEG_REG(ENCP_VIDEO_HAVON_END,    pConf->width - 1 + 48 );
    WRITE_MPEG_REG(ENCP_VIDEO_VAVON_BLINE,  pConf->video_on_line);
    WRITE_MPEG_REG(ENCP_VIDEO_VAVON_ELINE,  pConf->height + 3  + pConf->video_on_line);

    WRITE_MPEG_REG(ENCP_VIDEO_HSO_BEGIN,    15);
    WRITE_MPEG_REG(ENCP_VIDEO_HSO_END,      31);
    WRITE_MPEG_REG(ENCP_VIDEO_VSO_BEGIN,    15);
    WRITE_MPEG_REG(ENCP_VIDEO_VSO_END,      31);

    WRITE_MPEG_REG(ENCP_VIDEO_VSO_BLINE,    0);
    WRITE_MPEG_REG(ENCP_VIDEO_VSO_ELINE,    2);
    WRITE_MPEG_REG(ENCP_VIDEO_EN,         1);

    WRITE_MPEG_REG(VENC_VIDEO_PROG_MODE,    0x100);
    WRITE_MPEG_REG(ENCP_VIDEO_EN, 1);

//    WRITE_MPEG_REG(VPP_POSTBLEND_VD1_H_START_END,		pConf->width);
}

static inline void _enable_vsync_interrupt(void)
{
    if (READ_MPEG_REG(ENCP_VIDEO_EN) & 1) {
        WRITE_MPEG_REG(VENC_INTCTRL, 0x200);
#if 0
        while ((READ_MPEG_REG(VENC_INTFLAG) & 0x200) == 0) {
            u32 line1, line2;

            line1 = line2 = READ_MPEG_REG(VENC_ENCP_LINE);

            while (line1 >= line2) {
                line2 = line1;
                line1 = READ_MPEG_REG(VENC_ENCP_LINE);
            }

            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            if (READ_MPEG_REG(VENC_INTFLAG) & 0x200) {
                break;
            }

            WRITE_MPEG_REG(ENCP_VIDEO_EN, 0);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);

            WRITE_MPEG_REG(ENCP_VIDEO_EN, 1);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
            READ_MPEG_REG(VENC_INTFLAG);
        }
#endif
    }
    else{
        WRITE_MPEG_REG(VENC_INTCTRL, 0x2);
    }
}
static void _enable_backlight(u32 brightness_level)
{
    pDev->conf.backlight_on?pDev->conf.backlight_on():0;
}
static void _disable_backlight(void)
{
    pDev->conf.backlight_off?pDev->conf.backlight_off():0;
}
static void _lcd_module_enable(void)
{
    BUG_ON(pDev==NULL);
    pDev->conf.power_on?pDev->conf.power_on():0;
    _init_tvenc(&pDev->conf);
    	_init_tcon(&pDev->conf);
    	_enable_vsync_interrupt();
}

static const vinfo_t *lcd_get_current_info(void)
{
	return &pDev->lcd_info;
}

static int lcd_set_current_vmode(vmode_t mode)
{
	if (mode != VMODE_LCD)
        return -EINVAL;
    WRITE_MPEG_REG(VPP_POSTBLEND_H_SIZE, pDev->lcd_info.width);
    _lcd_module_enable();
    if (VMODE_INIT_NULL == pDev->lcd_info.mode)
        pDev->lcd_info.mode = VMODE_LCD;
    else
        _enable_backlight(BL_MAX_LEVEL);
	return 0;
}

static vmode_t lcd_validate_vmode(char *mode)
{
	if ((strncmp(mode, PANEL_NAME, strlen(PANEL_NAME))) == 0)
		return VMODE_LCD;
	
	return VMODE_MAX;
}
static int lcd_vmode_is_supported(vmode_t mode)
{
	mode&=VMODE_MODE_BIT_MASK;
	if(mode == VMODE_LCD )
	return true;
	return false;
}
static int lcd_module_disable(vmode_t cur_vmod)
{
	BUG_ON(pDev==NULL);
    _disable_backlight();
	pDev->conf.power_off?pDev->conf.power_off():0;
	return 0;
}
#ifdef  CONFIG_PM
static int lcd_suspend(void)
{
	BUG_ON(pDev==NULL);
    printk("lcd_suspend \n");
    _disable_backlight();
	pDev->conf.power_off?pDev->conf.power_off():0;

	return 0;
}
static int lcd_resume(void)
{
	printk("lcd_resume\n");
	_lcd_module_enable();
    _enable_backlight(BL_MAX_LEVEL);
	return 0;
}
#endif
static vout_server_t lcd_vout_server={
	.name = "lcd_vout_server",
	.op = {	
		.get_vinfo = lcd_get_current_info,
		.set_vmode = lcd_set_current_vmode,
		.validate_vmode = lcd_validate_vmode,
		.vmode_is_supported=lcd_vmode_is_supported,
		.disable=lcd_module_disable,
#ifdef  CONFIG_PM  
		.vout_suspend=lcd_suspend,
		.vout_resume=lcd_resume,
#endif
	},
};

static void _init_vout(tcon_dev_t *pDev)
{
	pDev->lcd_info.name = PANEL_NAME;
    pDev->lcd_info.mode = VMODE_INIT_NULL;
	pDev->lcd_info.width = pDev->conf.width;
	pDev->lcd_info.height = pDev->conf.height;
	pDev->lcd_info.field_height = pDev->conf.height;
	pDev->lcd_info.aspect_ratio_num = pDev->conf.screen_width;
	pDev->lcd_info.aspect_ratio_den = pDev->conf.screen_height;
	pDev->lcd_info.sync_duration_num = pDev->conf.sync_duration_num;
	pDev->lcd_info.sync_duration_den = pDev->conf.sync_duration_den;
       pDev->lcd_info.screen_real_width= pDev->conf.screen_actual_width;
       pDev->lcd_info.screen_real_height= pDev->conf.screen_actual_height;
	
       
	//add lcd actual active area size
	printk("lcd actual active area size: %d %d (mm).\n",pDev->conf.screen_actual_width, pDev->conf.screen_actual_height);

	vout_register_server(&lcd_vout_server);
}

static void _tcon_init(tcon_conf_t *pConf)
{
	logo_object_t  *init_logo_obj=NULL;

	
	_init_vout(pDev);
	init_logo_obj = get_current_logo_obj();	
	if(NULL==init_logo_obj ||!init_logo_obj->para.loaded)
    	_lcd_module_enable();
}

static int tcon_probe(struct platform_device *pdev)
{
    struct resource *s;

    pDev = (tcon_dev_t *)kmalloc(sizeof(tcon_dev_t), GFP_KERNEL);
    if (!pDev) {
        printk("[tcon]: Not enough memory.\n");
        return -ENOMEM;
    }

    s = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!s) {
        printk("[tcon]: No tcon configuration data specified.\n");
        kfree(pDev);
        return -EINVAL;
    }

	pDev->conf = *(tcon_conf_t *)(s->start);

    _tcon_init(&pDev->conf);
    return 0;
}

static int tcon_remove(struct platform_device *pdev)
{
    kfree(pDev);
    
    return 0;
}

static struct platform_driver tcon_driver = {
    .probe      = tcon_probe,
    .remove     = tcon_remove,
    .driver     = {
        .name   = "tcon-dev",
    }
};

static int __init tcon_init(void)
{
    if (platform_driver_register(&tcon_driver)) {
        printk("failed to register tcon driver module\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit tcon_exit(void)
{
    platform_driver_unregister(&tcon_driver);
}

subsys_initcall(tcon_init);
module_exit(tcon_exit);

MODULE_DESCRIPTION("AMLOGIC TCON controller driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");

