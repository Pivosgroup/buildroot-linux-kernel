/*
 * Amlogic M1 
 * frame buffer driver-----------HDMI_TX
 * Copyright (C) 2010 Amlogic, Inc.
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
 */


#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
//#include <linux/amports/canvas.h>
#include <asm/uaccess.h>
#include <mach/am_regs.h>

#include "../hdmi_tx_module.h"
#include "../hdmi_info_global.h"
#include "hdmi_tx_reg.h"
#define VFIFO2VD_TO_HDMI_LATENCY    3   // Latency in pixel clock from VFIFO2VD request to data ready to HDMI
#define XTAL_24MHZ
#define Wr(reg,val) WRITE_MPEG_REG(reg,val)
#define Rd(reg)   READ_MPEG_REG(reg)
 
#define HSYNC_POLARITY      1                       // HSYNC polarity: active high 
#define VSYNC_POLARITY      1                       // VSYNC polarity: active high
#define TX_INPUT_COLOR_DEPTH    0                   // Pixel bit width: 0=24-bit; 1=30-bit; 2=36-bit; 3=48-bit.
#define TX_INPUT_COLOR_FORMAT   1                   // Pixel format: 0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
#define TX_INPUT_COLOR_RANGE    0                   // Pixel range: 0=16-235/240; 1=16-240; 2=1-254; 3=0-255.


#define TX_OUTPUT_COLOR_RANGE   0                   // Pixel range: 0=16-235/240; 1=16-240; 2=1-254; 3=0-255.

#if 1
//spdif
#define TX_I2S_SPDIF        0                       // 0=SPDIF; 1=I2S.
#define TX_I2S_8_CHANNEL    0                       // 0=I2S 2-channel; 1=I2S 4 x 2-channel.
#else
//i2s 8 channel
#define TX_I2S_SPDIF        1                       // 0=SPDIF; 1=I2S.
#define TX_I2S_8_CHANNEL    1                       // 0=I2S 2-channel; 1=I2S 4 x 2-channel.
#endif

//static struct tasklet_struct EDID_tasklet;

static unsigned long modulo(unsigned long a, unsigned long b)
{
    if (a >= b) {
        return(a-b);
    } else {
        return(a);
    }
}
        
static signed int to_signed(unsigned int a)
{
    if (a <= 7) {
        return(a);
    } else {
        return(a-16);
    }
}

static void delay_us (int us)
{
    Wr(ISA_TIMERE,0);
    while(Rd(ISA_TIMERE)<us){}
} /* delay_us */

static irqreturn_t intr_handler(int irq, void *dev_instance)
{
    unsigned long data32;
    hdmitx_dev_t* hdmitx_device = (hdmitx_dev_t*)dev_instance;
    
    data32 = hdmi_rd_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_STAT); 
    printk("HDMI irq %x\n",data32);
    if (data32 & (1 << 0)) {  //HPD rising 
        hdmi_wr_only_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_STAT_CLR,  1 << 0); //clear HPD rising interrupt in hdmi module
        // If HPD asserts, then start DDC transaction
        if (hdmi_rd_reg(TX_HDCP_ST_EDID_STATUS) & (1<<1)) {
            // Start DDC transaction
            hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) | (1<<6)); // Assert sys_trigger_config
            hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) & ~(1<<1)); // Release sys_trigger_config_semi_manu
            hdmitx_device->hpd_event = 1;
        // Error if HPD deasserts
        } else {
            printk("HDMI Error: HDMI HPD deasserts!\n");
        }
    } else if (data32 & (1 << 1)) { //HPD falling
        hdmi_wr_only_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_STAT_CLR,  1 << 1); //clear HPD falling interrupt in hdmi module 
        hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) & ~(1<<6)); // Release sys_trigger_config
        hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) | (1<<1)); // Assert sys_trigger_config_semi_manu
        hdmitx_device->hpd_event = 2;
    } else if (data32 & (1 << 2)) { //TX EDID interrupt
        /*walkaround: manually clear EDID interrupt*/
        hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) | (1<<1)); 
        hdmi_wr_reg(TX_HDCP_EDID_CONFIG, hdmi_rd_reg(TX_HDCP_EDID_CONFIG) & ~(1<<1)); 
        /**/
        //tasklet_schedule(&EDID_tasklet);
        hdmi_wr_only_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_STAT_CLR,  1 << 2); //clear EDID rising interrupt in hdmi module 
    } else {
        printk("HDMI Error: Unkown HDMI Interrupt source Process_Irq\n");
        hdmi_wr_only_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_STAT_CLR,  data32); //clear unkown interrupt in hdmi module 
    }
    return IRQ_HANDLED;
}

static void hdmi_tvenc_set(Hdmi_tx_video_para_t *param)
{
    unsigned long TOTAL_PIXELS, PIXEL_REPEAT_HDMI, PIXEL_REPEAT_VENC, ACTIVE_PIXELS;
    unsigned FRONT_PORCH, HSYNC_PIXELS, ACTIVE_LINES, INTERLACE_MODE, TOTAL_LINES, SOF_LINES, VSYNC_LINES;
    unsigned LINES_F0, LINES_F1,BACK_PORCH, EOF_LINES, TOTAL_FRAMES;

    unsigned long total_pixels_venc ;
    unsigned long active_pixels_venc;
    unsigned long front_porch_venc  ;
    unsigned long hsync_pixels_venc ;

    unsigned long de_h_begin, de_h_end;
    unsigned long de_v_begin_even, de_v_end_even, de_v_begin_odd, de_v_end_odd;
    unsigned long hs_begin, hs_end;
    unsigned long vs_adjust;
    unsigned long vs_bline_evn, vs_eline_evn, vs_bline_odd, vs_eline_odd;
    unsigned long vso_begin_evn, vso_begin_odd;
    if(param->VIC==HDMI_480p60){
         INTERLACE_MODE     = 0;                   
         PIXEL_REPEAT_VENC  = 1;                   
         PIXEL_REPEAT_HDMI  = 0;                   
         ACTIVE_PIXELS      = (720*(1+PIXEL_REPEAT_HDMI)); // Number of active pixels per line.
         ACTIVE_LINES       = (480/(1+INTERLACE_MODE));    // Number of active lines per field.
         LINES_F0           = 525;                 
         LINES_F1           = 525;                 
         FRONT_PORCH        = 16;                  
         HSYNC_PIXELS       = 62;                  
         BACK_PORCH         = 60;                  
         EOF_LINES          = 9;                   
         VSYNC_LINES        = 6;                   
         SOF_LINES          = 30;                  
         TOTAL_FRAMES       = 4;                   
    }
    else{
         INTERLACE_MODE      =0;              
         PIXEL_REPEAT_VENC   =0;              
         PIXEL_REPEAT_HDMI   =0;              
         ACTIVE_PIXELS       =(1920*(1+PIXEL_REPEAT_HDMI)); // Number of active pixels per line.
         ACTIVE_LINES        =(1080/(1+INTERLACE_MODE));    // Number of active lines per field.
         LINES_F0            =1125;           
         LINES_F1            =1125;           
         FRONT_PORCH         =88;             
         HSYNC_PIXELS        =44;             
         BACK_PORCH          =148;            
         EOF_LINES           =4;              
         VSYNC_LINES         =5;              
         SOF_LINES           =36;             
         TOTAL_FRAMES        =4;              
    }
    TOTAL_PIXELS       = (FRONT_PORCH+HSYNC_PIXELS+BACK_PORCH+ACTIVE_PIXELS); // Number of total pixels per line.
    TOTAL_LINES        = (LINES_F0+(LINES_F1*INTERLACE_MODE));                // Number of total lines per frame.

    total_pixels_venc = (TOTAL_PIXELS  / (1+PIXEL_REPEAT_HDMI)) * (1+PIXEL_REPEAT_VENC); // 858 / 1 * 2 = 1716
    active_pixels_venc= (ACTIVE_PIXELS / (1+PIXEL_REPEAT_HDMI)) * (1+PIXEL_REPEAT_VENC); // 720 / 1 * 2 = 1440
    front_porch_venc  = (FRONT_PORCH   / (1+PIXEL_REPEAT_HDMI)) * (1+PIXEL_REPEAT_VENC); // 16   / 1 * 2 = 32
    hsync_pixels_venc = (HSYNC_PIXELS  / (1+PIXEL_REPEAT_HDMI)) * (1+PIXEL_REPEAT_VENC); // 62   / 1 * 2 = 124

    if(param->VIC==HDMI_480p60){    
        Wr(ENCP_VIDEO_MODE,          1<<14); // cfg_de_v = 1
        // Program DE timing
        de_h_begin = modulo(Rd(ENCP_VIDEO_HAVON_BEGIN) + VFIFO2VD_TO_HDMI_LATENCY,  total_pixels_venc); // (217 + 3) % 1716 = 220
        de_h_end   = modulo(de_h_begin + active_pixels_venc,                        total_pixels_venc); // (220 + 1440) % 1716 = 1660
        Wr(ENCP_DE_H_BEGIN, de_h_begin);    // 220
        Wr(ENCP_DE_H_END,   de_h_end);      // 1660
        // Program DE timing for even field
        de_v_begin_even = Rd(ENCP_VIDEO_VAVON_BLINE);       // 42
        de_v_end_even   = de_v_begin_even + ACTIVE_LINES;   // 42 + 480 = 522
        Wr(ENCP_DE_V_BEGIN_EVEN,de_v_begin_even);   // 42
        Wr(ENCP_DE_V_END_EVEN,  de_v_end_even);     // 522
        // Program DE timing for odd field if needed
        if (INTERLACE_MODE) {
            // Calculate de_v_begin_odd according to enc480p_timing.v:
            //wire[10:0]    cfg_ofld_vavon_bline    = {{7{ofld_vavon_ofst1 [3]}},ofld_vavon_ofst1 [3:0]} + cfg_video_vavon_bline    + ofld_line;
            de_v_begin_odd  = to_signed((Rd(ENCP_VIDEO_OFLD_VOAV_OFST) & 0xf0)>>4) + de_v_begin_even + (TOTAL_LINES-1)/2;
            de_v_end_odd    = de_v_begin_odd + ACTIVE_LINES;
            Wr(ENCP_DE_V_BEGIN_ODD, de_v_begin_odd);
            Wr(ENCP_DE_V_END_ODD,   de_v_end_odd);
        }
    
        // Program Hsync timing
        if (de_h_end + front_porch_venc >= total_pixels_venc) {
            hs_begin    = de_h_end + front_porch_venc - total_pixels_venc;
            vs_adjust   = 1;
        } else {
            hs_begin    = de_h_end + front_porch_venc; // 1660 + 32 = 1692
            vs_adjust   = 0;
        }
        hs_end  = modulo(hs_begin + hsync_pixels_venc,   total_pixels_venc); // (1692 + 124) % 1716 = 100
        Wr(ENCP_DVI_HSO_BEGIN,  hs_begin);  // 1692
        Wr(ENCP_DVI_HSO_END,    hs_end);    // 100
        
        // Program Vsync timing for even field
        if (de_v_begin_even >= SOF_LINES + VSYNC_LINES + (1-vs_adjust)) {
            vs_bline_evn = de_v_begin_even - SOF_LINES - VSYNC_LINES - (1-vs_adjust); // 42 - 30 - 6 - 1 = 5
        } else {
            vs_bline_evn = TOTAL_LINES + de_v_begin_even - SOF_LINES - VSYNC_LINES - (1-vs_adjust);
        }
        vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES); // (5 + 6) % 525 = 11
        Wr(ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);   // 5
        Wr(ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);   // 11
        vso_begin_evn = hs_begin; // 1692
        Wr(ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);  // 1692
        Wr(ENCP_DVI_VSO_END_EVN,   vso_begin_evn);  // 1692
        // Program Vsync timing for odd field if needed
        if (INTERLACE_MODE) {
            vs_bline_odd = de_v_begin_odd-1 - SOF_LINES - VSYNC_LINES;
            vs_eline_odd = de_v_begin_odd-1 - SOF_LINES;
            vso_begin_odd   = modulo(hs_begin + (total_pixels_venc>>1), total_pixels_venc);
            Wr(ENCP_DVI_VSO_BLINE_ODD, vs_bline_odd);
            Wr(ENCP_DVI_VSO_ELINE_ODD, vs_eline_odd);
            Wr(ENCP_DVI_VSO_BEGIN_ODD, vso_begin_odd);
            Wr(ENCP_DVI_VSO_END_ODD,   vso_begin_odd);
        }
        Wr(VENC_DVI_SETTING, (1 << 0)               | //0=select enci hs/vs; 1=select encp hs/vs
                             (0 << 1)               | //select vso/hso as hsync vsync
                             (HSYNC_POLARITY << 2)  | //invert hs
                             (VSYNC_POLARITY << 3)  | //invert vs
                             (2 << 4)               | //select encp_dvi_clk = clk54/2
                             (1 << 7)               | //0=sel external dvi; 1= sel internal hdmi
                             (0 << 8)               | //no invert clk
                             (0 << 13)              | //cfg_dvi_mode_gamma_en
                             (1 << 15)                //select encp_vs_dvi, encp_hs_dvi and encp_de as timing signal
        );
        Wr(VENC_DVI_SETTING_MORE, (TX_INPUT_COLOR_FORMAT==0)? 1 : 0); // [0] 0=Map data pins from Venc to Hdmi Tx as CrYCb mode;
        
    }
    else if(param->VIC==HDMI_1080p60){
        Wr(ENCP_VIDEO_MODE,          1<<14); // cfg_de_v = 1
        // Program DE timing
        de_h_begin = modulo(Rd(ENCP_VIDEO_HAVON_BEGIN) + VFIFO2VD_TO_HDMI_LATENCY,  total_pixels_venc); // (148 + 3) % 2200 = 151
        de_h_end   = modulo(de_h_begin + active_pixels_venc,                        total_pixels_venc); // (151 + 1920) % 2200 = 2071
        Wr(ENCP_DE_H_BEGIN, de_h_begin);    // 151
        Wr(ENCP_DE_H_END,   de_h_end);      // 2071
        // Program DE timing for even field
        de_v_begin_even = Rd(ENCP_VIDEO_VAVON_BLINE);       // 41
        de_v_end_even   = de_v_begin_even + ACTIVE_LINES;   // 41 + 1080 = 1121
        Wr(ENCP_DE_V_BEGIN_EVEN,de_v_begin_even);   // 41
        Wr(ENCP_DE_V_END_EVEN,  de_v_end_even);     // 1121
        // Program DE timing for odd field if needed
        if (INTERLACE_MODE) {
            // Calculate de_v_begin_odd according to enc480p_timing.v:
            //wire[10:0]    cfg_ofld_vavon_bline    = {{7{ofld_vavon_ofst1 [3]}},ofld_vavon_ofst1 [3:0]} + cfg_video_vavon_bline    + ofld_line;
            de_v_begin_odd  = to_signed((Rd(ENCP_VIDEO_OFLD_VOAV_OFST) & 0xf0)>>4) + de_v_begin_even + (TOTAL_LINES-1)/2;
            de_v_end_odd    = de_v_begin_odd + ACTIVE_LINES;
            Wr(ENCP_DE_V_BEGIN_ODD, de_v_begin_odd);
            Wr(ENCP_DE_V_END_ODD,   de_v_end_odd);
        }
    
        // Program Hsync timing
        if (de_h_end + front_porch_venc >= total_pixels_venc) {
            hs_begin    = de_h_end + front_porch_venc - total_pixels_venc;
            vs_adjust   = 1;
        } else {
            hs_begin    = de_h_end + front_porch_venc; // 2071 + 88 = 2159
            vs_adjust   = 0;
        }
        hs_end  = modulo(hs_begin + hsync_pixels_venc,   total_pixels_venc); // (2159 + 44) % 2200 = 3
        Wr(ENCP_DVI_HSO_BEGIN,  hs_begin);  // 2159
        Wr(ENCP_DVI_HSO_END,    hs_end);    // 3
        
        // Program Vsync timing for even field
        if (de_v_begin_even >= SOF_LINES + VSYNC_LINES + (1-vs_adjust)) {
            vs_bline_evn = de_v_begin_even - SOF_LINES - VSYNC_LINES - (1-vs_adjust);
        } else {
            vs_bline_evn = TOTAL_LINES + de_v_begin_even - SOF_LINES - VSYNC_LINES - (1-vs_adjust); // 1125 + 41 - 36 - 5 - 1 = 1124
        }
        vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES); // (1124 + 5) % 1125 = 4
        Wr(ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);   // 1124
        Wr(ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);   // 4
        vso_begin_evn = hs_begin; // 2159
        Wr(ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);  // 2159
        Wr(ENCP_DVI_VSO_END_EVN,   vso_begin_evn);  // 2159
        // Program Vsync timing for odd field if needed
        if (INTERLACE_MODE) {
            vs_bline_odd = de_v_begin_odd-1 - SOF_LINES - VSYNC_LINES;
            vs_eline_odd = de_v_begin_odd-1 - SOF_LINES;
            vso_begin_odd   = modulo(hs_begin + (total_pixels_venc>>1), total_pixels_venc);
            Wr(ENCP_DVI_VSO_BLINE_ODD, vs_bline_odd);
            Wr(ENCP_DVI_VSO_ELINE_ODD, vs_eline_odd);
            Wr(ENCP_DVI_VSO_BEGIN_ODD, vso_begin_odd);
            Wr(ENCP_DVI_VSO_END_ODD,   vso_begin_odd);
        }
    
        Wr(VENC_DVI_SETTING, (1 << 0)               | //0=select enci hs/vs; 1=select encp hs/vs
                             (0 << 1)               | //select vso/hso as hsync vsync
                             (HSYNC_POLARITY << 2)  | //invert hs
                             (VSYNC_POLARITY << 3)  | //invert vs
                             (1 << 4)               | //select clk54 as clk
                             (1 << 7)               | //0=sel external dvi; 1= sel internal hdmi
                             (0 << 8)               | //no invert clk
                             (0 << 13)              | //cfg_dvi_mode_gamma_en
                             (1 << 15)                //select encp_vs_dvi, encp_hs_dvi and encp_de as timing signal
        );
        Wr(VENC_DVI_SETTING_MORE, (TX_INPUT_COLOR_FORMAT==0)? 1 : 0); // [0] 0=Map data pins from Venc to Hdmi Tx as CrYCb mode;
                                                                      //     1=Map data pins from Venc to Hdmi Tx as RGB mode.
    }        

}    
 
static void hdmi_hw_set_480p(Hdmi_tx_video_para_t *param)
{
    unsigned int tmp_add_data;
    unsigned long TX_OUTPUT_COLOR_FORMAT;
    if(param->color==COLOR_SPACE_YUV444){
        TX_OUTPUT_COLOR_FORMAT=1;
    }
    else if(param->color==COLOR_SPACE_YUV422){
        TX_OUTPUT_COLOR_FORMAT=3;
    }
    else{
        TX_OUTPUT_COLOR_FORMAT=0;
    }

    // Configure HDMI PLL
//    Wr(HHI_HDMI_PLL_CNTL, 0x000a1B16);
#ifdef XTAL_24MHZ
    Wr(HHI_HDMI_PLL_CNTL, 0x03040905); // For xtal=24MHz: PREDIV=5, POSTDIV=9, N=4, 0D=3, to get phy_clk=270MHz, tmds_clk=27MHz.
#else
    Wr(HHI_HDMI_PLL_CNTL, 0x03050906); // PREDIV=6, POSTDIV=9, N=5, 0D=3, to get phy_clk=270MHz, tmds_clk=27MHz.
#endif
    Wr(HHI_HDMI_PLL_CNTL2, 0x50e8);
    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003);
    Wr(HHI_HDMI_AFC_CNTL, Rd(HHI_HDMI_AFC_CNTL) | 0x3);
    //Wr(HHI_HDMI_PLL_CNTL1, 0x014000c1);
    //Wr(OTHER_PLL_CNTL, 0x1021e);

    // Configure HDMI TX serializer:
    hdmi_wr_reg(0x011, 0x0f);   //Channels Power Up Setting ,"1" for Power-up ,"0" for Power-down,Bit[3:0]=CK,Data2,data1,data1,data0 Channels ;
    hdmi_wr_reg(0x015, 0x03);   //slew rate
    hdmi_wr_reg(0x017, 0x1d);   //1d for power-up Band-gap and main-bias ,00 is power down 
    hdmi_wr_reg(0x018, 0x24);   //Serializer Internal clock setting ,please fix to vaue 24 ,other setting is only for debug  
    hdmi_wr_reg(0x01a, 0xfb);   //bit[2:0]=011 ,CK channel output TMDS CLOCK ,bit[2:0]=101 ,ck channel output PHYCLCK 
    hdmi_wr_reg(0x016, 0x06);   // Bit[3:0] is HDMI-PHY's output swing control register
    hdmi_wr_reg(0x0F7, 0x0F);   // Termination resistor calib value
  //hdmi_wr_reg(0x014, 0x07);   // This register is for pre-emphasis control ,we need test different TMDS Clcok speed then write down the suggested     value for each one ;
    hdmi_wr_reg(0x014, 0x01);   // This is a sample for Pre-emphasis setting ,recommended for 225MHz's TMDS Setting & ** meters HDMI Cable  
    
    // delay 1000uS, then check HPLL_LOCK
    delay_us(1000);
    //while ( (Rd(HHI_HDMI_PLL_CNTL2) & (1<<31)) != (1<<31) );

    // --------------------------------------------------------
    // Program core_pin_mux to enable HDMI pins
    // --------------------------------------------------------
    //wire            pm_hdmi_cec_en              = pin_mux_reg0[2];
    //wire            pm_hdmi_hpd_5v_en           = pin_mux_reg0[1];
    //wire            pm_hdmi_i2c_5v_en           = pin_mux_reg0[0];
    //(*P_PERIPHS_PIN_MUX_0) |= ((1 << 2) | // pm_hdmi_cec_en
    //                           (1 << 1) | // pm_hdmi_hpd_5v_en
    //                           (1 << 0)); // pm_hdmi_i2c_5v_en
    Wr(PERIPHS_PIN_MUX_0, Rd(PERIPHS_PIN_MUX_0)|((1 << 2) | // pm_hdmi_cec_en
                               (1 << 1) | // pm_hdmi_hpd_5v_en
                               (1 << 0))); // pm_hdmi_i2c_5v_en

/////////////////reset

    // Enable these interrupts: [2] tx_edit_int_rise [1] tx_hpd_int_fall [0] tx_hpd_int_rise
    hdmi_wr_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_MASKN, 0x7);
    
    // HPD glitch filter
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_L, 0x00);
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_H, 0xa0);

    // Keep TX (except register I/F) in reset, while programming the registers:
    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // tx_pixel_rstn
    tmp_add_data |= 1 << 6; // tx_tmds_rstn
    tmp_add_data |= 1 << 5; // tx_audio_master_rstn
    tmp_add_data |= 1 << 4; // tx_audio_sample_rstn
    tmp_add_data |= 1 << 3; // tx_i2s_reset_rstn
    tmp_add_data |= 1 << 2; // tx_dig_reset_n_ch2
    tmp_add_data |= 1 << 1; // tx_dig_reset_n_ch1
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch0
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, tmp_add_data);

    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // HDMI_CH3_RST_IN
    tmp_add_data |= 1 << 6; // HDMI_CH2_RST_IN
    tmp_add_data |= 1 << 5; // HDMI_CH1_RST_IN
    tmp_add_data |= 1 << 4; // HDMI_CH0_RST_IN
    tmp_add_data |= 1 << 3; // HDMI_SR_RST
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch3
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, tmp_add_data);

    // Enable software controlled DDC transaction
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0 ;  // forced_sys_trigger
    //tmp_add_data[6]   = 1'b0 ;  // sys_trigger_config
    //tmp_add_data[5]   = 1'b0 ;  // mem_acc_seq_mode
    //tmp_add_data[4]   = 1'b0 ;  // mem_acc_seq_start
    //tmp_add_data[3]   = 1'b1 ;  // forced_mem_copy_done
    //tmp_add_data[2]   = 1'b1 ;  // mem_copy_done_config
    //tmp_add_data[1]   = 1'b1 ;  // sys_trigger_config_semi_manu
    //tmp_add_data[0]   = 1'b0 ;  // Rsrv
    tmp_add_data = 0x0e;
    hdmi_wr_reg(TX_HDCP_EDID_CONFIG, tmp_add_data);
    
    hdmi_wr_reg(TX_HDCP_CONFIG0,      1<<3);  //set TX rom_encrypt_off=1
    hdmi_wr_reg(TX_HDCP_MEM_CONFIG,   0<<3);  //set TX read_decrypt=0
    hdmi_wr_reg(TX_HDCP_ENCRYPT_BYTE, 0);     //set TX encrypt_byte=0x00
    
    // Setting the keys & EDID
    //task_tx_key_setting();
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;      // Force DTV timing (Auto)
    //tmp_add_data[6] = 1'b0;      // Force Video Scan, only if [7]is set
    //tmp_add_data[5] = 1'b0 ;     // Force Video field, only if [7]is set
    //tmp_add_data[4:0] = 5'b00 ;  // Rsrv
    tmp_add_data = 0;
    hdmi_wr_reg(TX_VIDEO_DTV_TIMING, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 7; // [7]   forced_default_phase
    tmp_add_data |= 0                       << 2; // [6:2] Rsrv
    tmp_add_data |= param->color_depth   << 0; // [1:0] Color_depth:0=24-bit pixel; 1=30-bit pixel; 2=36-bit pixel; 3=48-bit pixel
    hdmi_wr_reg(TX_VIDEO_DTV_MODE, tmp_add_data); // 0x00
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;       // Force packet timing
    //tmp_add_data[6] = 1'b0;       // PACKET ALLOC MODE
    //tmp_add_data[5:0] = 6'd47 ;   // PACKET_START_LATENCY
    tmp_add_data = 47;
    hdmi_wr_reg(TX_PACKET_CONTROL_1, tmp_add_data);

    // For debug: disable packets of audio_request, acr_request, deep_color_request, and avmute_request
    //hdmi_wr_reg(TX_PACKET_CONTROL_2, hdmi_rd_reg(TX_PACKET_CONTROL_2) | 0x0f);

    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:6] = 2'b0;     // audio_source_select[1:0]
    //tmp_add_data[5] = 1'b0;       // external_packet_enable
    //tmp_add_data[4] = 1'b1 ;      // internal_packet_enable
    //tmp_add_data[3:2] = 2'b0;     // afe_fifo_source_select_lane_1[1:0]
    //tmp_add_data[1:0] = 2'b0 ;    // afe_fifo_source_select_lane_0[1:0]
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_CORE_DATA_CAPTURE_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0;     // monitor_lane_1
    //tmp_add_data[6:4] = 3'd0;     // monitor_select_lane_1[2:0]
    //tmp_add_data[3]   = 1'b1 ;    // monitor_lane_0
    //tmp_add_data[2:0] = 3'd7;     // monitor_select_lane_0[2:0]
    tmp_add_data = 0xf;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_1, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:3] = 5'b0;     // Rsrv
    //tmp_add_data[2:0] = 3'd2;     // monitor_select[2:0]
    tmp_add_data = 0x2;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b1;     // forced_hdmi
    //tmp_add_data[6] = 1'b1;     // hdmi_config
    //tmp_add_data[5:4] = 2'b0;   // Rsrv
    //tmp_add_data[3] = 1'b0;     // bit_swap.
    //tmp_add_data[2:0] = 3'd0;   // channel_swap[2:0]
    tmp_add_data = 0xc0;
    hdmi_wr_reg(TX_TMDS_MODE, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_CONNECT_SEL: 0=use lower channel data[29:0]; 1=use upper channel data[59:30]
    //tmp_add_data[5:0] = 'h0;  // Rsrv
    tmp_add_data = 0x0;
    hdmi_wr_reg(TX_SYS4_CONNECT_SEL_1, tmp_add_data);
    
    // Normally it makes sense to synch 3 channel output with clock channel's rising edge,
    // as HDMI's serializer is LSB out first, invert tmds_clk pattern from "1111100000" to
    // "0000011111" actually enable data synch with clock rising edge.
    tmp_add_data = 1 << 4; // Set tmds_clk pattern to be "0000011111" before being sent to AFE clock channel
    hdmi_wr_reg(TX_SYS4_CK_INV_VIDEO, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_AFE_FIFO channel 2 bypass=0
    //tmp_add_data[5] = 1'b0;  // TX_AFE_FIFO channel 1 bypass=0
    //tmp_add_data[4] = 1'b0;  // TX_AFE_FIFO channel 0 bypass=0
    //tmp_add_data[3] = 1'b1;  // output enable of clk channel (channel 3)
    //tmp_add_data[2] = 1'b1;  // TX_AFE_FIFO channel 2 enable
    //tmp_add_data[1] = 1'b1;  // TX_AFE_FIFO channel 1 enable
    //tmp_add_data[0] = 1'b1;  // TX_AFE_FIFO channel 0 enable
    tmp_add_data = 0x0f;
    hdmi_wr_reg(TX_SYS5_FIFO_CONFIG, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= TX_OUTPUT_COLOR_FORMAT  << 6; // [7:6] output_color_format: 0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= TX_INPUT_COLOR_FORMAT   << 4; // [5:4] input_color_format:  0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= param->color_depth<< 2; // [3:2] output_color_depth:  0=24-b; 1=30-b; 2=36-b; 3=48-b.
    tmp_add_data |= TX_INPUT_COLOR_DEPTH    << 0; // [1:0] input_color_depth:   0=24-b; 1=30-b; 2=36-b; 3=48-b.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_L, tmp_add_data); // 0x50
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 4; // [7:4] Rsrv
    tmp_add_data |= TX_OUTPUT_COLOR_RANGE   << 2; // [3:2] output_color_range:  0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    tmp_add_data |= TX_INPUT_COLOR_RANGE    << 0; // [1:0] input_color_range:   0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_H, tmp_add_data); // 0x00

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_SPDIF    << 7; // [7]    I2S or SPDIF
    tmp_add_data |= TX_I2S_8_CHANNEL<< 6; // [6]    8 or 2ch
    tmp_add_data |= 2               << 4; // [5:4]  Serial Format: I2S format
    tmp_add_data |= 3               << 2; // [3:2]  Bit Width: 24-bit
    tmp_add_data |= 1               << 1; // [1]    WS Polarity: 1=WS high is left
    tmp_add_data |= 1               << 0; // [0]    For I2S: 0=one-bit audio; 1=I2S;
                                          //        For SPDIF: 0= channel status from input data; 1=from register
    hdmi_wr_reg(TX_AUDIO_FORMAT, tmp_add_data); // 0x2f

    tmp_add_data  = 0;
    tmp_add_data |= 0x4 << 4; // [7:4]  FIFO Depth=512
    tmp_add_data |= 0x2 << 2; // [3:2]  Critical threshold=Depth/16
    tmp_add_data |= 0x1 << 0; // [1:0]  Normal threshold=Depth/8
    hdmi_wr_reg(TX_AUDIO_FIFO, tmp_add_data); // 0x49
    hdmi_wr_reg(TX_AUDIO_LIPSYNC, 0); // [7:0] Normalized lip-sync param: 0 means S(lipsync) = S(total)/2
    tmp_add_data  = 0;
    tmp_add_data |= 0   << 7; // [7]    forced_audio_fifo_clear
    tmp_add_data |= 1   << 6; // [6]    auto_audio_fifo_clear
    tmp_add_data |= 0x0 << 4; // [5:4]  audio_packet_type: 0=audio sample packet; 1=one bit audio; 2=HBR audio packet; 3=DST audio packet.
    tmp_add_data |= 0   << 3; // [3]    Rsrv
    tmp_add_data |= 0   << 2; // [2]    Audio sample packet's valid bit: 0=valid bit is 0 for I2S, is input data for SPDIF; 1=valid bit from register
    tmp_add_data |= 0   << 1; // [1]    Audio sample packet's user bit: 0=user bit is 0 for I2S, is input data for SPDIF; 1=user bit from register
    tmp_add_data |= 0   << 0; // [0]    0=Audio sample packet's sample_flat bit is 1; 1=sample_flat is 0.
    hdmi_wr_reg(TX_AUDIO_CONTROL, tmp_add_data); // 0x40

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_8_CHANNEL<< 7; // [7]    Audio sample packet's header layout bit: 0=layout0; 1=layout1
    tmp_add_data |= 0               << 6; // [6]    Set normal_double bit in DST packet header.
    tmp_add_data |= 0               << 0; // [5:0]  Rsrv
    hdmi_wr_reg(TX_AUDIO_HEADER, tmp_add_data); // 0x00

    tmp_add_data  = TX_I2S_8_CHANNEL ? 0x00ff : 0x0003;
    hdmi_wr_reg(TX_AUDIO_SAMPLE, tmp_add_data); // Channel valid for up to 8 channels, 1 bit per channel.

    hdmi_wr_reg(TX_AUDIO_PACK, 0x0001); // Enable audio sample packets

    // Set N = 4096 (N is not measured, N must be configured so as to be a reference to clock_meter)
    hdmi_wr_reg(TX_SYS1_ACR_N_0, 0x0000); // N[7:0]
    hdmi_wr_reg(TX_SYS1_ACR_N_1, 0x0010); // N[15:8]

    tmp_add_data  = 0;
    tmp_add_data |= 0xa << 4;    // [7:4] Meas Tolerance
    tmp_add_data |= 0x0 << 0;    // [3:0] N[19:16]
    hdmi_wr_reg(TX_SYS1_ACR_N_2, tmp_add_data); // 0xa0

    //tmp_add_data[7] = 1'b0;      // cp_desired
    //tmp_add_data[6] = 1'b0;      // ess_config
    //tmp_add_data[5] = 1'b0;      // set_avmute
    //tmp_add_data[4] = 1'b1;      // clear_avmute
    //tmp_add_data[3] = 1'b0;      // hdcp_1_1
    //tmp_add_data[2] = 1'b0;      // Vsync/Hsync forced_polarity_select
    //tmp_add_data[1] = 1'b0;      // forced_vsync_polarity
    //tmp_add_data[0] = 1'b0;      // forced_hsync_polarity
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);

    //config_hdmi(1);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 8'b1 ;  //cp_desired 
    //tmp_add_data[6]   = 8'b1 ;  //ess_config 
    //tmp_add_data[5]   = 8'b0 ;  //set_avmute 
    //tmp_add_data[4]   = 8'b0 ;  //clear_avmute 
    //tmp_add_data[3]   = 8'b1 ;  //hdcp_1_1 
    //tmp_add_data[2]   = 8'b0 ;  //forced_polarity 
    //tmp_add_data[1]   = 8'b0 ;  //forced_vsync_polarity 
    //tmp_add_data[0]   = 8'b0 ;  //forced_hsync_polarity
    tmp_add_data = 0x8; //0xc8;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);
    


    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:0]   = 0xa ; // time_divider[7:0] for DDC I2C bus clock
    tmp_add_data = 0xa;
    hdmi_wr_reg(TX_HDCP_CONFIG3, tmp_add_data);
    
/////////////////////////    
    //hdmi_wr_reg(TX_AUDIO_I2S,   1); // TX AUDIO I2S Enable
/////////////////////////    
    // --------------------------------------------------------
    // Release TX out of reset
    // --------------------------------------------------------

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040000); // turn off phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x01); // Release serializer resets
    delay_us(10);

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003); // turn on phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x00); // Release reset on TX digital clock channel
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 1<<6); // Release resets all other TX digital clock domain, except tmds_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 0x00); // Final release reset on tmds_clk domain
}

static void hdmi_hw_set_1080p(Hdmi_tx_video_para_t *param)
{
    unsigned int tmp_add_data;
    unsigned long TX_OUTPUT_COLOR_FORMAT;
    if(param->color==COLOR_SPACE_YUV444){
        TX_OUTPUT_COLOR_FORMAT=1;
    }
    else if(param->color==COLOR_SPACE_YUV422){
        TX_OUTPUT_COLOR_FORMAT=3;
    }
    else{
        TX_OUTPUT_COLOR_FORMAT=0;
    }
    // Configure HDMI PLL
#ifdef XTAL_24MHZ
    Wr(HHI_HDMI_PLL_CNTL, 0x0008210f); // For 24MHz xtal: PREDIV=15, POSTDIV=33, N=8, 0D=0, to get phy_clk=1485MHz, tmds_clk=148.5MHz.
#else
    Wr(HHI_HDMI_PLL_CNTL, 0x00081913); // PREDIV=19, POSTDIV=25, N=8, 0D=0, to get phy_clk=1484.375MHz, tmds_clk=148.4375MHz.
#endif    
    Wr(HHI_HDMI_PLL_CNTL2, 0x50e8);
    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003);
    Wr(HHI_HDMI_AFC_CNTL, Rd(HHI_HDMI_AFC_CNTL) | 0x3);

    // Configure HDMI TX serializer:
    hdmi_wr_reg(0x011, 0x0f);   //Channels Power Up Setting ,"1" for Power-up ,"0" for Power-down,Bit[3:0]=CK,Data2,data1,data1,data0 Channels ;
  //hdmi_wr_reg(0x015, 0x03);   //slew rate
    hdmi_wr_reg(0x017, 0x1d);   //1d for power-up Band-gap and main-bias ,00 is power down 
    hdmi_wr_reg(0x018, 0x24);   //Serializer Internal clock setting ,please fix to vaue 24 ,other setting is only for debug  
    hdmi_wr_reg(0x01a, 0xfb);   //bit[2:0]=011 ,CK channel output TMDS CLOCK ,bit[2:0]=101 ,ck channel output PHYCLCK 
    hdmi_wr_reg(0x016, 0x04);   // Bit[3:0] is HDMI-PHY's output swing control register
    hdmi_wr_reg(0x0F7, 0x0F);   // Termination resistor calib value
  //hdmi_wr_reg(0x014, 0x07);   // This register is for pre-emphasis control ,we need test different TMDS Clcok speed then write down the suggested     value for each one ;
  //hdmi_wr_reg(0x014, 0x01);   // This is a sample for Pre-emphasis setting ,recommended for 225MHz's TMDS Setting & ** meters HDMI Cable  
    
    // delay 1000uS, then check HPLL_LOCK
    delay_us(1000);
    //while ( (Rd(HHI_HDMI_PLL_CNTL2) & (1<<31)) != (1<<31) );
 
    // --------------------------------------------------------
    // Program core_pin_mux to enable HDMI pins
    // --------------------------------------------------------
    //wire            pm_hdmi_cec_en              = pin_mux_reg0[2];
    //wire            pm_hdmi_hpd_5v_en           = pin_mux_reg0[1];
    //wire            pm_hdmi_i2c_5v_en           = pin_mux_reg0[0];
    Wr(PERIPHS_PIN_MUX_0, Rd(PERIPHS_PIN_MUX_0)|((1 << 2) | // pm_hdmi_cec_en
                               (1 << 1) | // pm_hdmi_hpd_5v_en
                               (1 << 0))); // pm_hdmi_i2c_5v_en


//////////////////////////////reset    
    // Enable these interrupts: [2] tx_edit_int_rise [1] tx_hpd_int_fall [0] tx_hpd_int_rise
    hdmi_wr_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_MASKN, 0x7);
    
    // HPD glitch filter
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_L, 0x00);
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_H, 0xa0);

    // Keep TX (except register I/F) in reset, while programming the registers:
    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // tx_pixel_rstn
    tmp_add_data |= 1 << 6; // tx_tmds_rstn
    tmp_add_data |= 1 << 5; // tx_audio_master_rstn
    tmp_add_data |= 1 << 4; // tx_audio_sample_rstn
    tmp_add_data |= 1 << 3; // tx_i2s_reset_rstn
    tmp_add_data |= 1 << 2; // tx_dig_reset_n_ch2
    tmp_add_data |= 1 << 1; // tx_dig_reset_n_ch1
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch0
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, tmp_add_data);

    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // HDMI_CH3_RST_IN
    tmp_add_data |= 1 << 6; // HDMI_CH2_RST_IN
    tmp_add_data |= 1 << 5; // HDMI_CH1_RST_IN
    tmp_add_data |= 1 << 4; // HDMI_CH0_RST_IN
    tmp_add_data |= 1 << 3; // HDMI_SR_RST
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch3
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, tmp_add_data);

    // Enable software controlled DDC transaction
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0 ;  // forced_sys_trigger
    //tmp_add_data[6]   = 1'b0 ;  // sys_trigger_config
    //tmp_add_data[5]   = 1'b0 ;  // mem_acc_seq_mode
    //tmp_add_data[4]   = 1'b0 ;  // mem_acc_seq_start
    //tmp_add_data[3]   = 1'b1 ;  // forced_mem_copy_done
    //tmp_add_data[2]   = 1'b1 ;  // mem_copy_done_config
    //tmp_add_data[1]   = 1'b1 ;  // sys_trigger_config_semi_manu
    //tmp_add_data[0]   = 1'b0 ;  // Rsrv
    tmp_add_data = 0x0e;
    hdmi_wr_reg(TX_HDCP_EDID_CONFIG, tmp_add_data);
    
    hdmi_wr_reg(TX_HDCP_CONFIG0,      1<<3);  //set TX rom_encrypt_off=1
    hdmi_wr_reg(TX_HDCP_MEM_CONFIG,   0<<3);  //set TX read_decrypt=0
    hdmi_wr_reg(TX_HDCP_ENCRYPT_BYTE, 0);     //set TX encrypt_byte=0x00
    
    // Setting the keys & EDID
    //task_tx_key_setting();
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;      // Force DTV timing (Auto)
    //tmp_add_data[6] = 1'b0;      // Force Video Scan, only if [7]is set
    //tmp_add_data[5] = 1'b0 ;     // Force Video field, only if [7]is set
    //tmp_add_data[4:0] = 5'b00 ;  // Rsrv
    tmp_add_data = 0;
    hdmi_wr_reg(TX_VIDEO_DTV_TIMING, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 7; // [7]   forced_default_phase
    tmp_add_data |= 0                       << 2; // [6:2] Rsrv
    tmp_add_data |= param->color_depth      << 0; // [1:0] Color_depth:0=24-bit pixel; 1=30-bit pixel; 2=36-bit pixel; 3=48-bit pixel
    hdmi_wr_reg(TX_VIDEO_DTV_MODE, tmp_add_data); // 0x00
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;       // Force packet timing
    //tmp_add_data[6] = 1'b0;       // PACKET ALLOC MODE
    //tmp_add_data[5:0] = 6'd47 ;   // PACKET_START_LATENCY
    tmp_add_data = 47;
    hdmi_wr_reg(TX_PACKET_CONTROL_1, tmp_add_data);

    // For debug: disable packets of audio_request, acr_request, deep_color_request, and avmute_request
    //hdmi_wr_reg(TX_PACKET_CONTROL_2, hdmi_rd_reg(TX_PACKET_CONTROL_2) | 0x0f);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:6] = 2'b0;     // audio_source_select[1:0]
    //tmp_add_data[5] = 1'b0;       // external_packet_enable
    //tmp_add_data[4] = 1'b1 ;      // internal_packet_enable
    //tmp_add_data[3:2] = 2'b0;     // afe_fifo_source_select_lane_1[1:0]
    //tmp_add_data[1:0] = 2'b0 ;    // afe_fifo_source_select_lane_0[1:0]
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_CORE_DATA_CAPTURE_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0;     // monitor_lane_1
    //tmp_add_data[6:4] = 3'd0;     // monitor_select_lane_1[2:0]
    //tmp_add_data[3]   = 1'b1 ;    // monitor_lane_0
    //tmp_add_data[2:0] = 3'd7;     // monitor_select_lane_0[2:0]
    tmp_add_data = 0xf;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_1, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:3] = 5'b0;     // Rsrv
    //tmp_add_data[2:0] = 3'd2;     // monitor_select[2:0]
    tmp_add_data = 0x2;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b1;     // forced_hdmi
    //tmp_add_data[6] = 1'b1;     // hdmi_config
    //tmp_add_data[5:4] = 2'b0;   // Rsrv
    //tmp_add_data[3] = 1'b0;     // bit_swap.
    //tmp_add_data[2:0] = 3'd0;   // channel_swap[2:0]
    tmp_add_data = 0xc0;
    hdmi_wr_reg(TX_TMDS_MODE, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_CONNECT_SEL: 0=use lower channel data[29:0]; 1=use upper channel data[59:30]
    //tmp_add_data[5:0] = 'h0;  // Rsrv
    tmp_add_data = 0x0;
    hdmi_wr_reg(TX_SYS4_CONNECT_SEL_1, tmp_add_data);
    
    // Normally it makes sense to synch 3 channel output with clock channel's rising edge,
    // as HDMI's serializer is LSB out first, invert tmds_clk pattern from "1111100000" to
    // "0000011111" actually enable data synch with clock rising edge.
    tmp_add_data = 1 << 4; // Set tmds_clk pattern to be "0000011111" before being sent to AFE clock channel
    hdmi_wr_reg(TX_SYS4_CK_INV_VIDEO, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_AFE_FIFO channel 2 bypass=0
    //tmp_add_data[5] = 1'b0;  // TX_AFE_FIFO channel 1 bypass=0
    //tmp_add_data[4] = 1'b0;  // TX_AFE_FIFO channel 0 bypass=0
    //tmp_add_data[3] = 1'b1;  // output enable of clk channel (channel 3)
    //tmp_add_data[2] = 1'b1;  // TX_AFE_FIFO channel 2 enable
    //tmp_add_data[1] = 1'b1;  // TX_AFE_FIFO channel 1 enable
    //tmp_add_data[0] = 1'b1;  // TX_AFE_FIFO channel 0 enable
    tmp_add_data = 0x0f;
    hdmi_wr_reg(TX_SYS5_FIFO_CONFIG, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= TX_OUTPUT_COLOR_FORMAT  << 6; // [7:6] output_color_format: 0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= TX_INPUT_COLOR_FORMAT   << 4; // [5:4] input_color_format:  0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= param->color_depth   << 2; // [3:2] output_color_depth:  0=24-b; 1=30-b; 2=36-b; 3=48-b.
    tmp_add_data |= TX_INPUT_COLOR_DEPTH    << 0; // [1:0] input_color_depth:   0=24-b; 1=30-b; 2=36-b; 3=48-b.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_L, tmp_add_data); // 0x50
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 4; // [7:4] Rsrv
    tmp_add_data |= TX_OUTPUT_COLOR_RANGE   << 2; // [3:2] output_color_range:  0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    tmp_add_data |= TX_INPUT_COLOR_RANGE    << 0; // [1:0] input_color_range:   0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_H, tmp_add_data); // 0x00

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_SPDIF    << 7; // [7]    I2S or SPDIF
    tmp_add_data |= TX_I2S_8_CHANNEL<< 6; // [6]    8 or 2ch
    tmp_add_data |= 2               << 4; // [5:4]  Serial Format: I2S format
    tmp_add_data |= 3               << 2; // [3:2]  Bit Width: 24-bit
    tmp_add_data |= 1               << 1; // [1]    WS Polarity: 1=WS high is left
    tmp_add_data |= 1               << 0; // [0]    For I2S: 0=one-bit audio; 1=I2S;
                                          //        For SPDIF: 0= channel status from input data; 1=from register
    hdmi_wr_reg(TX_AUDIO_FORMAT, tmp_add_data); // 0x2f

    tmp_add_data  = 0;
    tmp_add_data |= 0x4 << 4; // [7:4]  FIFO Depth=512
    tmp_add_data |= 0x2 << 2; // [3:2]  Critical threshold=Depth/16
    tmp_add_data |= 0x1 << 0; // [1:0]  Normal threshold=Depth/8
    hdmi_wr_reg(TX_AUDIO_FIFO, tmp_add_data); // 0x49

    hdmi_wr_reg(TX_AUDIO_LIPSYNC, 0); // [7:0] Normalized lip-sync param: 0 means S(lipsync) = S(total)/2

    tmp_add_data  = 0;
    tmp_add_data |= 0   << 7; // [7]    forced_audio_fifo_clear
    tmp_add_data |= 1   << 6; // [6]    auto_audio_fifo_clear
    tmp_add_data |= 0x0 << 4; // [5:4]  audio_packet_type: 0=audio sample packet; 1=one bit audio; 2=HBR audio packet; 3=DST audio packet.
    tmp_add_data |= 0   << 3; // [3]    Rsrv
    tmp_add_data |= 0   << 2; // [2]    Audio sample packet's valid bit: 0=valid bit is 0 for I2S, is input data for SPDIF; 1=valid bit from register
    tmp_add_data |= 0   << 1; // [1]    Audio sample packet's user bit: 0=user bit is 0 for I2S, is input data for SPDIF; 1=user bit from register
    tmp_add_data |= 0   << 0; // [0]    0=Audio sample packet's sample_flat bit is 1; 1=sample_flat is 0.
    hdmi_wr_reg(TX_AUDIO_CONTROL, tmp_add_data); // 0x40

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_8_CHANNEL<< 7; // [7]    Audio sample packet's header layout bit: 0=layout0; 1=layout1
    tmp_add_data |= 0               << 6; // [6]    Set normal_double bit in DST packet header.
    tmp_add_data |= 0               << 0; // [5:0]  Rsrv
    hdmi_wr_reg(TX_AUDIO_HEADER, tmp_add_data); // 0x00

    tmp_add_data  = TX_I2S_8_CHANNEL ? 0xff : 0x03;
    hdmi_wr_reg(TX_AUDIO_SAMPLE, tmp_add_data); // Channel valid for up to 8 channels, 1 bit per channel.

    hdmi_wr_reg(TX_AUDIO_PACK, 0x01); // Enable audio sample packets

    // Set N = 4096 (N is not measured, N must be configured so as to be a reference to clock_meter)
    hdmi_wr_reg(TX_SYS1_ACR_N_0, 0x00); // N[7:0]
    hdmi_wr_reg(TX_SYS1_ACR_N_1, 0x18 /*0x10*/); // N[15:8]

    tmp_add_data  = 0;
    tmp_add_data |= 0xa << 4;    // [7:4] Meas Tolerance
    tmp_add_data |= 0x0 << 0;    // [3:0] N[19:16]
    hdmi_wr_reg(TX_SYS1_ACR_N_2, tmp_add_data); // 0xa0

    //tmp_add_data[7] = 1'b0;      // cp_desired
    //tmp_add_data[6] = 1'b0;      // ess_config
    //tmp_add_data[5] = 1'b0;      // set_avmute
    //tmp_add_data[4] = 1'b1;      // clear_avmute
    //tmp_add_data[3] = 1'b0;      // hdcp_1_1
    //tmp_add_data[2] = 1'b0;      // Vsync/Hsync forced_polarity_select
    //tmp_add_data[1] = 1'b0;      // forced_vsync_polarity
    //tmp_add_data[0] = 1'b0;      // forced_hsync_polarity
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);
    //config_hdmi(1);

    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:0]   = 0xa ; // time_divider[7:0] for DDC I2C bus clock
    tmp_add_data = 0xa;
    hdmi_wr_reg(TX_HDCP_CONFIG3, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 8'b1 ;  //cp_desired 
    //tmp_add_data[6]   = 8'b1 ;  //ess_config 
    //tmp_add_data[5]   = 8'b0 ;  //set_avmute 
    //tmp_add_data[4]   = 8'b0 ;  //clear_avmute 
    //tmp_add_data[3]   = 8'b1 ;  //hdcp_1_1 
    //tmp_add_data[2]   = 8'b0 ;  //forced_polarity 
    //tmp_add_data[1]   = 8'b0 ;  //forced_vsync_polarity 
    //tmp_add_data[0]   = 8'b0 ;  //forced_hsync_polarity
    tmp_add_data = 0xc8;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);
    
    // --------------------------------------------------------
    // Release TX out of reset
    // --------------------------------------------------------

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040000); // turn off phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x01); // Release serializer resets
    delay_us(10);

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003); // turn on phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x00); // Release reset on TX digital clock channel
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 1<<6); // Release resets all other TX digital clock domain, except tmds_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 0x00); // Final release reset on tmds_clk domain
}

static void hdmi_m1a_init()
{
    unsigned int tmp_add_data;
    // Configure HDMI PLL
//    Wr(HHI_HDMI_PLL_CNTL, 0x000a1B16);
#ifdef XTAL_24MHZ
    Wr(HHI_HDMI_PLL_CNTL, 0x03040905); // For xtal=24MHz: PREDIV=5, POSTDIV=9, N=4, 0D=3, to get phy_clk=270MHz, tmds_clk=27MHz.
#else
    Wr(HHI_HDMI_PLL_CNTL, 0x03050906); // PREDIV=6, POSTDIV=9, N=5, 0D=3, to get phy_clk=270MHz, tmds_clk=27MHz.
#endif
    Wr(HHI_HDMI_PLL_CNTL2, 0x50e8);
    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003);
    Wr(HHI_HDMI_AFC_CNTL, Rd(HHI_HDMI_AFC_CNTL) | 0x3);
    //Wr(HHI_HDMI_PLL_CNTL1, 0x014000c1);
    //Wr(OTHER_PLL_CNTL, 0x1021e);

    // Configure HDMI TX serializer:
    hdmi_wr_reg(0x011, 0x0f);   //Channels Power Up Setting ,"1" for Power-up ,"0" for Power-down,Bit[3:0]=CK,Data2,data1,data1,data0 Channels ;
    hdmi_wr_reg(0x015, 0x03);   //slew rate
    hdmi_wr_reg(0x017, 0x1d);   //1d for power-up Band-gap and main-bias ,00 is power down 
    hdmi_wr_reg(0x018, 0x24);   //Serializer Internal clock setting ,please fix to vaue 24 ,other setting is only for debug  
    hdmi_wr_reg(0x01a, 0xfb);   //bit[2:0]=011 ,CK channel output TMDS CLOCK ,bit[2:0]=101 ,ck channel output PHYCLCK 
    hdmi_wr_reg(0x016, 0x06);   // Bit[3:0] is HDMI-PHY's output swing control register
    hdmi_wr_reg(0x0F7, 0x0F);   // Termination resistor calib value
  //hdmi_wr_reg(0x014, 0x07);   // This register is for pre-emphasis control ,we need test different TMDS Clcok speed then write down the suggested     value for each one ;
    hdmi_wr_reg(0x014, 0x01);   // This is a sample for Pre-emphasis setting ,recommended for 225MHz's TMDS Setting & ** meters HDMI Cable  
    
    // delay 1000uS, then check HPLL_LOCK
    delay_us(1000);
    //while ( (Rd(HHI_HDMI_PLL_CNTL2) & (1<<31)) != (1<<31) );

    // --------------------------------------------------------
    // Program core_pin_mux to enable HDMI pins
    // --------------------------------------------------------
    //wire            pm_hdmi_cec_en              = pin_mux_reg0[2];
    //wire            pm_hdmi_hpd_5v_en           = pin_mux_reg0[1];
    //wire            pm_hdmi_i2c_5v_en           = pin_mux_reg0[0];
    //(*P_PERIPHS_PIN_MUX_0) |= ((1 << 2) | // pm_hdmi_cec_en
    //                           (1 << 1) | // pm_hdmi_hpd_5v_en
    //                           (1 << 0)); // pm_hdmi_i2c_5v_en
    Wr(PERIPHS_PIN_MUX_0, Rd(PERIPHS_PIN_MUX_0)|((1 << 2) | // pm_hdmi_cec_en
                               (1 << 1) | // pm_hdmi_hpd_5v_en
                               (1 << 0))); // pm_hdmi_i2c_5v_en

/////////////////reset

    // Enable these interrupts: [2] tx_edit_int_rise [1] tx_hpd_int_fall [0] tx_hpd_int_rise
    hdmi_wr_reg(OTHER_BASE_ADDR + HDMI_OTHER_INTR_MASKN, 0x7);
    
    // HPD glitch filter
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_L, 0x00);
    hdmi_wr_reg(TX_HDCP_HPD_FILTER_H, 0xa0);

    // Keep TX (except register I/F) in reset, while programming the registers:
    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // tx_pixel_rstn
    tmp_add_data |= 1 << 6; // tx_tmds_rstn
    tmp_add_data |= 1 << 5; // tx_audio_master_rstn
    tmp_add_data |= 1 << 4; // tx_audio_sample_rstn
    tmp_add_data |= 1 << 3; // tx_i2s_reset_rstn
    tmp_add_data |= 1 << 2; // tx_dig_reset_n_ch2
    tmp_add_data |= 1 << 1; // tx_dig_reset_n_ch1
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch0
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, tmp_add_data);

    tmp_add_data  = 0;
    tmp_add_data |= 1 << 7; // HDMI_CH3_RST_IN
    tmp_add_data |= 1 << 6; // HDMI_CH2_RST_IN
    tmp_add_data |= 1 << 5; // HDMI_CH1_RST_IN
    tmp_add_data |= 1 << 4; // HDMI_CH0_RST_IN
    tmp_add_data |= 1 << 3; // HDMI_SR_RST
    tmp_add_data |= 1 << 0; // tx_dig_reset_n_ch3
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, tmp_add_data);

    // Enable software controlled DDC transaction
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0 ;  // forced_sys_trigger
    //tmp_add_data[6]   = 1'b0 ;  // sys_trigger_config
    //tmp_add_data[5]   = 1'b0 ;  // mem_acc_seq_mode
    //tmp_add_data[4]   = 1'b0 ;  // mem_acc_seq_start
    //tmp_add_data[3]   = 1'b1 ;  // forced_mem_copy_done
    //tmp_add_data[2]   = 1'b1 ;  // mem_copy_done_config
    //tmp_add_data[1]   = 1'b1 ;  // sys_trigger_config_semi_manu
    //tmp_add_data[0]   = 1'b0 ;  // Rsrv
    tmp_add_data = 0x0e;
    hdmi_wr_reg(TX_HDCP_EDID_CONFIG, tmp_add_data);
    
    hdmi_wr_reg(TX_HDCP_CONFIG0,      1<<3);  //set TX rom_encrypt_off=1
    hdmi_wr_reg(TX_HDCP_MEM_CONFIG,   0<<3);  //set TX read_decrypt=0
    hdmi_wr_reg(TX_HDCP_ENCRYPT_BYTE, 0);     //set TX encrypt_byte=0x00
    
    // Setting the keys & EDID
    //task_tx_key_setting();
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;      // Force DTV timing (Auto)
    //tmp_add_data[6] = 1'b0;      // Force Video Scan, only if [7]is set
    //tmp_add_data[5] = 1'b0 ;     // Force Video field, only if [7]is set
    //tmp_add_data[4:0] = 5'b00 ;  // Rsrv
    tmp_add_data = 0;
    hdmi_wr_reg(TX_VIDEO_DTV_TIMING, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 7; // [7]   forced_default_phase
    tmp_add_data |= 0                       << 2; // [6:2] Rsrv
    tmp_add_data |= 0   << 0; // [1:0] Color_depth:0=24-bit pixel; 1=30-bit pixel; 2=36-bit pixel; 3=48-bit pixel
    hdmi_wr_reg(TX_VIDEO_DTV_MODE, tmp_add_data); // 0x00
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;       // Force packet timing
    //tmp_add_data[6] = 1'b0;       // PACKET ALLOC MODE
    //tmp_add_data[5:0] = 6'd47 ;   // PACKET_START_LATENCY
    tmp_add_data = 47;
    hdmi_wr_reg(TX_PACKET_CONTROL_1, tmp_add_data);

    // For debug: disable packets of audio_request, acr_request, deep_color_request, and avmute_request
    //hdmi_wr_reg(TX_PACKET_CONTROL_2, hdmi_rd_reg(TX_PACKET_CONTROL_2) | 0x0f);

    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:6] = 2'b0;     // audio_source_select[1:0]
    //tmp_add_data[5] = 1'b0;       // external_packet_enable
    //tmp_add_data[4] = 1'b1 ;      // internal_packet_enable
    //tmp_add_data[3:2] = 2'b0;     // afe_fifo_source_select_lane_1[1:0]
    //tmp_add_data[1:0] = 2'b0 ;    // afe_fifo_source_select_lane_0[1:0]
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_CORE_DATA_CAPTURE_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 1'b0;     // monitor_lane_1
    //tmp_add_data[6:4] = 3'd0;     // monitor_select_lane_1[2:0]
    //tmp_add_data[3]   = 1'b1 ;    // monitor_lane_0
    //tmp_add_data[2:0] = 3'd7;     // monitor_select_lane_0[2:0]
    tmp_add_data = 0xf;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_1, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:3] = 5'b0;     // Rsrv
    //tmp_add_data[2:0] = 3'd2;     // monitor_select[2:0]
    tmp_add_data = 0x2;
    hdmi_wr_reg(TX_CORE_DATA_MONITOR_2, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b1;     // forced_hdmi
    //tmp_add_data[6] = 1'b1;     // hdmi_config
    //tmp_add_data[5:4] = 2'b0;   // Rsrv
    //tmp_add_data[3] = 1'b0;     // bit_swap.
    //tmp_add_data[2:0] = 3'd0;   // channel_swap[2:0]
    tmp_add_data = 0xc0;
    hdmi_wr_reg(TX_TMDS_MODE, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_CONNECT_SEL: 0=use lower channel data[29:0]; 1=use upper channel data[59:30]
    //tmp_add_data[5:0] = 'h0;  // Rsrv
    tmp_add_data = 0x0;
    hdmi_wr_reg(TX_SYS4_CONNECT_SEL_1, tmp_add_data);
    
    // Normally it makes sense to synch 3 channel output with clock channel's rising edge,
    // as HDMI's serializer is LSB out first, invert tmds_clk pattern from "1111100000" to
    // "0000011111" actually enable data synch with clock rising edge.
    tmp_add_data = 1 << 4; // Set tmds_clk pattern to be "0000011111" before being sent to AFE clock channel
    hdmi_wr_reg(TX_SYS4_CK_INV_VIDEO, tmp_add_data);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7] = 1'b0;  // Rsrv
    //tmp_add_data[6] = 1'b0;  // TX_AFE_FIFO channel 2 bypass=0
    //tmp_add_data[5] = 1'b0;  // TX_AFE_FIFO channel 1 bypass=0
    //tmp_add_data[4] = 1'b0;  // TX_AFE_FIFO channel 0 bypass=0
    //tmp_add_data[3] = 1'b1;  // output enable of clk channel (channel 3)
    //tmp_add_data[2] = 1'b1;  // TX_AFE_FIFO channel 2 enable
    //tmp_add_data[1] = 1'b1;  // TX_AFE_FIFO channel 1 enable
    //tmp_add_data[0] = 1'b1;  // TX_AFE_FIFO channel 0 enable
    tmp_add_data = 0x0f;
    hdmi_wr_reg(TX_SYS5_FIFO_CONFIG, tmp_add_data);
    
    tmp_add_data  = 0;
    tmp_add_data |= 0  << 6; // [7:6] output_color_format: 0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= TX_INPUT_COLOR_FORMAT   << 4; // [5:4] input_color_format:  0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422.
    tmp_add_data |= 0<< 2; // [3:2] output_color_depth:  0=24-b; 1=30-b; 2=36-b; 3=48-b.
    tmp_add_data |= TX_INPUT_COLOR_DEPTH    << 0; // [1:0] input_color_depth:   0=24-b; 1=30-b; 2=36-b; 3=48-b.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_L, tmp_add_data); // 0x50
    
    tmp_add_data  = 0;
    tmp_add_data |= 0                       << 4; // [7:4] Rsrv
    tmp_add_data |= TX_OUTPUT_COLOR_RANGE   << 2; // [3:2] output_color_range:  0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    tmp_add_data |= TX_INPUT_COLOR_RANGE    << 0; // [1:0] input_color_range:   0=16-235/240; 1=16-240; 2=1-254; 3=0-255.
    hdmi_wr_reg(TX_VIDEO_DTV_OPTION_H, tmp_add_data); // 0x00

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_SPDIF    << 7; // [7]    I2S or SPDIF
    tmp_add_data |= TX_I2S_8_CHANNEL<< 6; // [6]    8 or 2ch
    tmp_add_data |= 2               << 4; // [5:4]  Serial Format: I2S format
    tmp_add_data |= 3               << 2; // [3:2]  Bit Width: 24-bit
    tmp_add_data |= 1               << 1; // [1]    WS Polarity: 1=WS high is left
    tmp_add_data |= 1               << 0; // [0]    For I2S: 0=one-bit audio; 1=I2S;
                                          //        For SPDIF: 0= channel status from input data; 1=from register
    hdmi_wr_reg(TX_AUDIO_FORMAT, tmp_add_data); // 0x2f

    tmp_add_data  = 0;
    tmp_add_data |= 0x4 << 4; // [7:4]  FIFO Depth=512
    tmp_add_data |= 0x2 << 2; // [3:2]  Critical threshold=Depth/16
    tmp_add_data |= 0x1 << 0; // [1:0]  Normal threshold=Depth/8
    hdmi_wr_reg(TX_AUDIO_FIFO, tmp_add_data); // 0x49
    hdmi_wr_reg(TX_AUDIO_LIPSYNC, 0); // [7:0] Normalized lip-sync param: 0 means S(lipsync) = S(total)/2
    tmp_add_data  = 0;
    tmp_add_data |= 0   << 7; // [7]    forced_audio_fifo_clear
    tmp_add_data |= 1   << 6; // [6]    auto_audio_fifo_clear
    tmp_add_data |= 0x0 << 4; // [5:4]  audio_packet_type: 0=audio sample packet; 1=one bit audio; 2=HBR audio packet; 3=DST audio packet.
    tmp_add_data |= 0   << 3; // [3]    Rsrv
    tmp_add_data |= 0   << 2; // [2]    Audio sample packet's valid bit: 0=valid bit is 0 for I2S, is input data for SPDIF; 1=valid bit from register
    tmp_add_data |= 0   << 1; // [1]    Audio sample packet's user bit: 0=user bit is 0 for I2S, is input data for SPDIF; 1=user bit from register
    tmp_add_data |= 0   << 0; // [0]    0=Audio sample packet's sample_flat bit is 1; 1=sample_flat is 0.
    hdmi_wr_reg(TX_AUDIO_CONTROL, tmp_add_data); // 0x40

    tmp_add_data  = 0;
    tmp_add_data |= TX_I2S_8_CHANNEL<< 7; // [7]    Audio sample packet's header layout bit: 0=layout0; 1=layout1
    tmp_add_data |= 0               << 6; // [6]    Set normal_double bit in DST packet header.
    tmp_add_data |= 0               << 0; // [5:0]  Rsrv
    hdmi_wr_reg(TX_AUDIO_HEADER, tmp_add_data); // 0x00

    tmp_add_data  = TX_I2S_8_CHANNEL ? 0x00ff : 0x0003;
    hdmi_wr_reg(TX_AUDIO_SAMPLE, tmp_add_data); // Channel valid for up to 8 channels, 1 bit per channel.

    hdmi_wr_reg(TX_AUDIO_PACK, 0x0001); // Enable audio sample packets

    // Set N = 4096 (N is not measured, N must be configured so as to be a reference to clock_meter)
    hdmi_wr_reg(TX_SYS1_ACR_N_0, 0x0000); // N[7:0]
    hdmi_wr_reg(TX_SYS1_ACR_N_1, 0x0010); // N[15:8]

    tmp_add_data  = 0;
    tmp_add_data |= 0xa << 4;    // [7:4] Meas Tolerance
    tmp_add_data |= 0x0 << 0;    // [3:0] N[19:16]
    hdmi_wr_reg(TX_SYS1_ACR_N_2, tmp_add_data); // 0xa0

    //tmp_add_data[7] = 1'b0;      // cp_desired
    //tmp_add_data[6] = 1'b0;      // ess_config
    //tmp_add_data[5] = 1'b0;      // set_avmute
    //tmp_add_data[4] = 1'b1;      // clear_avmute
    //tmp_add_data[3] = 1'b0;      // hdcp_1_1
    //tmp_add_data[2] = 1'b0;      // Vsync/Hsync forced_polarity_select
    //tmp_add_data[1] = 1'b0;      // forced_vsync_polarity
    //tmp_add_data[0] = 1'b0;      // forced_hsync_polarity
    tmp_add_data = 0x10;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);

    //config_hdmi(1);
    
    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7]   = 8'b1 ;  //cp_desired 
    //tmp_add_data[6]   = 8'b1 ;  //ess_config 
    //tmp_add_data[5]   = 8'b0 ;  //set_avmute 
    //tmp_add_data[4]   = 8'b0 ;  //clear_avmute 
    //tmp_add_data[3]   = 8'b1 ;  //hdcp_1_1 
    //tmp_add_data[2]   = 8'b0 ;  //forced_polarity 
    //tmp_add_data[1]   = 8'b0 ;  //forced_vsync_polarity 
    //tmp_add_data[0]   = 8'b0 ;  //forced_hsync_polarity
    tmp_add_data = 0x8; //0xc8;
    hdmi_wr_reg(TX_HDCP_MODE, tmp_add_data);
    


    //tmp_add_data[15:8] = 0;
    //tmp_add_data[7:0]   = 0xa ; // time_divider[7:0] for DDC I2C bus clock
    tmp_add_data = 0xa;
    hdmi_wr_reg(TX_HDCP_CONFIG3, tmp_add_data);
    
/////////////////////////    
    //hdmi_wr_reg(TX_AUDIO_I2S,   1); // TX AUDIO I2S Enable
/////////////////////////    
    // --------------------------------------------------------
    // Release TX out of reset
    // --------------------------------------------------------

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040000); // turn off phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x01); // Release serializer resets
    delay_us(10);

    Wr(HHI_HDMI_PLL_CNTL1, 0x00040003); // turn on phy_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_2, 0x00); // Release reset on TX digital clock channel
    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 1<<6); // Release resets all other TX digital clock domain, except tmds_clk
    delay_us(10);

    hdmi_wr_reg(TX_SYS5_TX_SOFT_RESET_1, 0x00); // Final release reset on tmds_clk domain
}

static void hdmi_audio_init()
{
}

static void enable_audio_spdif(unsigned char* audio_adr, int audio_data_size)
{
    // Audio clock source use xtal clock
    Wr(HHI_AUD_CLK_CNTL,    Rd(HHI_AUD_CLK_CNTL) | (1<<11));
    // As amclk is 25MHz, set i2s and iec958 divisor appropriately to get 48KHz sample rate.
    Wr(AIU_CLK_CTRL_MORE,   0x0000); // i2s_divisor_more does not take effect
    Wr(AIU_CLK_CTRL,        (0 << 12) | // 958 divisor more, if true, divided by 2, 4, 6, 8.
                            (1 <<  8) | // alrclk skew: 1=alrclk transitions on the cycle before msb is sent
                            (1 <<  6) | // invert aoclk
                            (3 <<  4) | // 958 divisor: 0=no div; 1=div by 2; 2=div by 3; 3=div by 4.
                            (3 <<  2)   // i2s divisor: 0=no div; 1=div by 2; 2=div by 4; 3=div by 8.
      );
        Wr( AIU_958_MISC, 0x204a ); // // Program the IEC958 Module in the AIU
        Wr( AIU_958_FORCE_LEFT, 0x0000 );
        Wr( AIU_958_CTRL, 0x0240 );
        Wr( AIU_MEM_IEC958_START_PTR, ((unsigned)audio_adr)&0x7fffffff  /*0x00110000*/ ); // Set the IEC958 Start pointer into DDR memory to 0x00110000
        Wr( AIU_MEM_IEC958_RD_PTR,    ((unsigned)audio_adr)&0x7fffffff /*0x00110000*/ ); // Set the IEC958 current pointer into DDR memory to 0x00110000
        Wr( AIU_MEM_IEC958_END_PTR,   (((unsigned)audio_adr)+audio_data_size)&0x7fffffff /*0x00112000*/ ); // Set the IEC958 end pointer into DDR memory to 0x00112000
        Wr( AIU_MEM_IEC958_MASKS, 0xFFFF ); // Set the number of channels in memory to 8 and the number of channels to read to 8

/* enable audio*/        
        hdmi_wr_reg(TX_AUDIO_SPDIF, 1); // TX AUDIO SPDIF Enable

        Wr(AIU_CLK_CTRL,        Rd(AIU_CLK_CTRL) | 2); // enable iec958 clock which is audio_master_clk
        Wr( AIU_958_BPF, 0x0100 ); // Set the PCM frame size to 256 bytes
        Wr( AIU_958_DCU_FF_CTRL, 0x0001 );
        // Set init high then low to initilize the IEC958 memory logic
        // bit 30 : ch_always_8
        Wr( AIU_MEM_IEC958_CONTROL, 1  | (1 << 30));
        Wr( AIU_MEM_IEC958_CONTROL, 0  | (1 << 30));
        // Enable the IEC958 FIFO (both the empty and fill modules)
        Wr( AIU_MEM_IEC958_CONTROL, Rd( AIU_MEM_IEC958_CONTROL) | ((1 << 1) | (1 << 2)) );
        
} 
#if 0
#include "logon_spdif.dat"
static int prepare_audio_data(unsigned int* audio_adr)
{
    int i;
    int audio_data_size=sizeof(audio_data);
    for(i=0;i<(audio_data_size>>2);i++){
        audio_adr[i]=audio_data[i];
    }
    audio_data_size=(audio_data_size*7/8)&0xffff0000;
    return audio_data_size;
}
#endif
/************************************
*    hdmitx hardware level interface
*************************************/
static unsigned char hdmitx_m1a_getediddata(hdmitx_dev_t* hdmitx_device)
{
    if(hdmi_rd_reg(TX_HDCP_ST_EDID_STATUS) & (1<<4)){
        int block,i;
        for(block=0;block<4;block++){
            for(i=0;i<128;i++){
                hdmitx_device->EDID_buf[block*128+i]=hdmi_rd_reg(0x600+block*128+i);
            }
        }
        return 1;
    }
    else{
        return 0;
    }    
}    


static int hdmitx_m1a_set_dispmode(Hdmi_tx_video_para_t *param)
{
    int ret=0;
    if((param->VIC==HDMI_480p60)||(param->VIC==HDMI_480p60_16x9)){
        hdmi_hw_set_480p(param);    
        hdmi_tvenc_set(param);
    }
    else if(param->VIC==HDMI_1080p60){
        hdmi_hw_set_1080p(param);    
        hdmi_tvenc_set(param);
    }
    else{
        ret = -1;
    }
    return ret;
}    


static void hdmitx_m1a_setavi(unsigned char* AVI_DB, unsigned char* AVI_HB)
{
    // AVI frame
    int i ;
    unsigned char ucData ;

    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x01, AVI_DB[0]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x02, AVI_DB[1]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x03, AVI_DB[2]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x04, AVI_DB[3]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x05, AVI_DB[4]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x06, AVI_DB[5]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x07, AVI_DB[6]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x08, AVI_DB[7]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x09, AVI_DB[8]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x010, AVI_DB[9]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x011, AVI_DB[10]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x012, AVI_DB[11]);  
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x013, AVI_DB[12]);  

    for(i = 0,ucData = 0; i < 13 ; i++)
    {
        ucData -= AVI_DB[i] ;
    }
    for(i=0; i<3; i++){
        ucData -= AVI_HB[i];
    }
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x00, ucData);  

    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x1C, AVI_HB[0]);        // HB0: packet type=0x82
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x1D, AVI_HB[1]);        // HB1: packet version =0x02
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x1E, AVI_HB[2]);        // HB2: payload bytes=13
    hdmi_wr_reg(TX_PKT_REG_AVI_INFO_BASE_ADDR+0x1F, 0x00ff);        // Enable AVI packet generation
}


static void hdmitx_m1a_setaudioinfoframe(unsigned char bEnable, unsigned char* AUD_DB)
{

    if(!bEnable){
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x1F, 0x0); // disable audio info frame
    }
    else{
        int i ;
        unsigned char ucData ;
    
        // Audio Info frame
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x01, AUD_DB[0]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x02, AUD_DB[1]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x03, AUD_DB[2]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x04, AUD_DB[3]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x05, AUD_DB[4]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x06, AUD_DB[5]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x07, AUD_DB[6]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x08, AUD_DB[7]);  
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x09, AUD_DB[8]);  
        //hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x0a, AUD_DB[9]);  
    
    
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x1B, 0x0001); //AUDIO_INFOFRAME_VER // PB27: Rsrv.
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x1C, 0x0084); //0x80+AUDIO_INFOFRAME_TYPE // HB0: Packet Type = 0x84
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x1E, 0x000a); //AUDIO_INFOFRAME_LEN // HB2: Payload length in bytes
    
        for(i = 0,ucData = 0 ; i< 9 ; i++)
        {
            ucData -= AUD_DB[i] ;
        }
        ucData -= (0x80+0x1+0x4+0xa) ;
    
        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR, ucData); // PB0: Checksum

#if 0
        for(i=0;i<24;i++){
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+i, 0);        
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+i, 0);
        }
        hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x3, 0x2);
        hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x3, 0x2);
#else
        for(i=0;i<24;i++){
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+i, 0);        
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+i, 0);
        }
        hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x2, 0x11);
        hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x2, 0x11);
        hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x3, 0x2);
        hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x3, 0x2);
        hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x4, 0xd2);
        hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x4, 0xd2);
        
#endif        

        hdmi_wr_reg(TX_PKT_REG_AUDIO_INFO_BASE_ADDR+0x1F, 0x00ff); // Enable audio info frame
        if (TX_I2S_SPDIF == 0) {
            // TX Channel Status
            //0xB0 - 00000000;     //0xC8 - 00000000;
            //0xB1 - 00000000;     //0xC9 - 00000000;
            //0xB2 - 00011000;     //0xCA - 00101000;
            //0xB3 - 00000000;     //0xCB - 00000000;
            //0xB4 - 11111011;     //0xCC - 11111011;
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x00, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x01, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x02, 0x0018);
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x03, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB1_OFFSET+0x04, 0x00fb);
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x00, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x01, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x02, 0x0028);
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x03, 0x0000);
            hdmi_wr_reg(TX_IEC60958_SUB2_OFFSET+0x04, 0x00fb);
        }
    }
}

static void hdmitx_m1a_set_audmode(hdmitx_dev_t* hdmitx_device)
{
    //int size = prepare_audio_data((unsigned char*)0x81100000);
    //hdmi_audio_init();
    //enable_audio_spdif((unsigned char*)0x81100000, size);
}    
    
static void hdmitx_m1a_setupirq(hdmitx_dev_t* hdmitx_device)
{
   int r;

   //tasklet_init(&EDID_tasklet, edid_tasklet_fun, hdmitx_device);

   r = request_irq(INT_HDMI_TX, &intr_handler,
                    IRQF_SHARED, "amhdmitx",
                    (void *)hdmitx_device);


    Rd(A9_0_IRQ_IN1_INTR_STAT_CLR);
    Wr(A9_0_IRQ_IN1_INTR_MASK, Rd(A9_0_IRQ_IN1_INTR_MASK)|(1 << 25));
    
}    

void HDMITX_M1A_Init(hdmitx_dev_t* hdmitx_device)
{
    hdmitx_device->HWOp.SetAVI = hdmitx_m1a_setavi;
    hdmitx_device->HWOp.SetAudioInfoFrame = hdmitx_m1a_setaudioinfoframe;
    hdmitx_device->HWOp.GetEDIDData = hdmitx_m1a_getediddata;
    hdmitx_device->HWOp.SetDispMode = hdmitx_m1a_set_dispmode;
    hdmitx_device->HWOp.SetAudMode = hdmitx_m1a_set_audmode;
    hdmitx_device->HWOp.SetupIRQ = hdmitx_m1a_setupirq;
    
    // -----------------------------------------
    // HDMI (90Mhz)
    // -----------------------------------------
    //         .clk_div            ( hi_hdmi_clk_cntl[6:0] ),
    //         .clk_en             ( hi_hdmi_clk_cntl[8]   ),
    //         .clk_sel            ( hi_hdmi_clk_cntl[11:9]),
    Wr( HHI_HDMI_CLK_CNTL,  ((1 << 9)  |   // select "other" PLL
                             (1 << 8)  |   // Enable gated clock
                             (5 << 0)) );  // Divide the "other" PLL output by 6

                                                                  //     1=Map data pins from Venc to Hdmi Tx as RGB mode.
    // --------------------------------------------------------
    // Configure HDMI TX analog, and use HDMI PLL to generate TMDS clock
    // --------------------------------------------------------

    // Enable APB3 fail on error
    //*((volatile unsigned long *) HDMI_CTRL_PORT) |= (1 << 15);        //APB3 err_en
    WRITE_APB_REG(0x2008, READ_APB_REG(0x2008)|(1<<15));
    
    hdmi_m1a_init();

}    
    