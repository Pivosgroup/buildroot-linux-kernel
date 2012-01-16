/*
 * Amlogic M1 & M2
 * frame buffer driver  -------bt656 & 601 input
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <asm/delay.h>
#include <asm/atomic.h>

#include <linux/amports/amstream.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/tvin/tvin.h>
#include <mach/am_regs.h>

#include "tvin_global.h"
#include "bt656_601_in.h"
#include "vdin_regs.h"
#include "tvin_notifier.h"
#include "vdin.h"
#include "vdin_ctl.h"


#define DEVICE_NAME "amvdec_656in"
#define MODULE_NAME "amvdec_656in"
#define BT656IN_IRQ_NAME "amvdec_656in-irq"

//#define HANDLE_BT656IN_IRQ

//#define BT656IN_COUNT             32
//#define BT656IN_ANCI_DATA_SIZE        0x4000



/* Per-device (per-bank) structure */
//typedef struct bt656_in_dev_s {
    /* ... */
//    struct cdev         cdev;             /* The cdev structure */
    //wait_queue_head_t   wait_queue;            /* wait queues */
//}am656in_dev_t;

//static struct am656in_dev_t am656in_dev_;
typedef struct {
    unsigned char       rd_canvas_index;
    unsigned char       wr_canvas_index;
    unsigned char       buff_flag[BT656IN_VF_POOL_SIZE];

    unsigned char       fmt_check_cnt;

//    tvin_port_t         port;
//    enum tvin_sig_fmt_e    fmt;
//    enum tvin_sig_status_e status;


    unsigned        pbufAddr;
    unsigned        decbuf_size;

    unsigned        pin_mux_reg1;   //for bt656 input
    unsigned        pin_mux_mask1;

    unsigned        pin_mux_reg2;   //for bt601 input
    unsigned        pin_mux_mask2;

    unsigned        pin_mux_reg3;   //for camera input
    unsigned        pin_mux_mask3;

    unsigned        active_pixel;
    unsigned        active_line;
    unsigned        dec_status : 1;
    unsigned        canvas_total_count : 4;
    struct tvin_para_s para ;



}am656in_t;


static struct vdin_dev_s *vdin_devp_bt656 = NULL;


am656in_t am656in_dec_info = {
    .pbufAddr = 0x81000000,
    .decbuf_size = 0x70000,
    .pin_mux_reg1 = 0,      //for bt656 input
    .pin_mux_mask1 = 0,
    .pin_mux_reg2 = 0,      //for bt656 input
    .pin_mux_mask2 = 0,
    .pin_mux_reg3 = 0,      //for camera input
    .pin_mux_mask3 = 0,
    .active_pixel = 720,
    .active_line = 288,
    .para = {
        .port       = TVIN_PORT_NULL,
        .fmt        = TVIN_SIG_FMT_NULL,
        .status     = TVIN_SIG_STATUS_NULL,
        .cap_addr   = 0x81000000,
        .cap_flag   = 0,
     },

    .rd_canvas_index = 0xff - (BT656IN_VF_POOL_SIZE + 2),
    .wr_canvas_index =  BT656IN_VF_POOL_SIZE,

    .buff_flag = {
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP},

    .fmt_check_cnt = 0,
    .dec_status = 0,
    .canvas_total_count = BT656IN_VF_POOL_SIZE,
};

extern struct tvin_format_s tvin_fmt_tbl[TVIN_SIG_FMT_MAX + 1];

#ifdef HANDLE_BT656IN_IRQ
static const char bt656in_dec_id[] = "bt656in-dev";
#endif

static u8 bt656_canvas_tab[BT656IN_VF_POOL_SIZE] = {
    VDIN_START_CANVAS,
    VDIN_START_CANVAS + 1,
    VDIN_START_CANVAS + 2,
    VDIN_START_CANVAS + 3,
    VDIN_START_CANVAS + 4,
    VDIN_START_CANVAS + 5,
    VDIN_START_CANVAS + 6,
    VDIN_START_CANVAS + 7,
};

static void init_656in_dec_parameter(tvin_sig_fmt_t input_mode)
{
    am656in_dec_info.para.fmt = input_mode;
    switch(input_mode)
    {
        case TVIN_SIG_FMT_BT656IN_576I:     //656--PAL(interlace)
            am656in_dec_info.active_pixel = 720;
            am656in_dec_info.active_line = 288;
            pr_dbg("PAL input mode is selected for bt656, \n");
            break;

        case TVIN_SIG_FMT_BT656IN_480I:     //656--NTSC(interlace)
            am656in_dec_info.active_pixel = 720;
            am656in_dec_info.active_line = 240;
            pr_dbg("NTSC input mode is selected for bt656, \n");
            break;

        case TVIN_SIG_FMT_BT601IN_576I:     //601--PAL(interlace)
            am656in_dec_info.active_pixel = 720;
            am656in_dec_info.active_line = 288;
            pr_dbg("PAL input mode is selected for bt601, \n");
            break;

        case TVIN_SIG_FMT_BT601IN_480I:     //601--NTSC(interlace)
            am656in_dec_info.active_pixel = 720;
            am656in_dec_info.active_line = 240;
            pr_dbg("NTSC input mode is selected for bt601, \n");
            break;

        case TVIN_SIG_FMT_CAMERA_640X480P_30Hz:     //640x480 camera inout(progressive)
            am656in_dec_info.active_pixel = 640;
            am656in_dec_info.active_line = 480;
            pr_dbg("640x480 input mode is selected for camera, \n");
            break;

        case TVIN_SIG_FMT_CAMERA_800X600P_30Hz:     //800x600 camera inout(progressive)
            am656in_dec_info.active_pixel = 800;
            am656in_dec_info.active_line = 600;
            pr_dbg("800x600 input mode is selected for camera, \n");
            break;

        case TVIN_SIG_FMT_CAMERA_1024X768P_30Hz:     //1024x768 camera inout(progressive)
            am656in_dec_info.active_pixel = 1024;
            am656in_dec_info.active_line = 768;
            pr_dbg("1024x768 input mode is selected for camera, \n");
            break;

        case TVIN_SIG_FMT_CAMERA_1920X1080P_30Hz:     //1920x1080 camera inout(progressive)
            am656in_dec_info.active_pixel = 1920;
            am656in_dec_info.active_line = 1080;
            pr_dbg("1920x1080 input mode is selected for camera, \n");
            break;

        case TVIN_SIG_FMT_NULL:
            pr_dbg("bt656_601 input decode is not start, do nothing \n");
            break;

        default:
            pr_dbg("bt656_601 input mode is not defined, do nothing \n");
            break;

    }

    return;
}

static void bt656_656in_canvas_init(unsigned int mem_start, unsigned int mem_size)
{
    unsigned i = 0;
    unsigned int canvas_width  = 1440;
    unsigned int canvas_height = 288;
    unsigned decbuf_start = mem_start + BT656IN_ANCI_DATA_SIZE;
    am656in_dec_info.decbuf_size   = 0x70000;
    am656in_dec_info.pbufAddr  = mem_start ;
    i = (unsigned)((mem_size - BT656IN_ANCI_DATA_SIZE) / am656in_dec_info.decbuf_size);
    if(i % 2)
        i -= 1;     //sure the counter is even

    am656in_dec_info.canvas_total_count = (BT656IN_VF_POOL_SIZE > i)? i : BT656IN_VF_POOL_SIZE;
    for ( i = 0; i < am656in_dec_info.canvas_total_count; i++)
    {
        canvas_config(VDIN_START_CANVAS + i, decbuf_start + i * am656in_dec_info.decbuf_size,
            canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
    }
}

static void bt656_camera_in_canvas_init(unsigned int mem_start, unsigned int mem_size)
{
    int i = 0;
    unsigned int canvas_width  = 1920 << 1;
    unsigned int canvas_height = 1080;
    unsigned decbuf_start = mem_start + BT656IN_ANCI_DATA_SIZE;
    am656in_dec_info.decbuf_size   = 0x400000;

    i = (unsigned)((mem_size - BT656IN_ANCI_DATA_SIZE) / am656in_dec_info.decbuf_size);

    am656in_dec_info.canvas_total_count = (BT656IN_VF_POOL_SIZE > i)? i : BT656IN_VF_POOL_SIZE;

    am656in_dec_info.pbufAddr  = mem_start;
    for ( i = 0; i < am656in_dec_info.canvas_total_count; i++)
    {
        canvas_config(VDIN_START_CANVAS + i, decbuf_start + i * am656in_dec_info.decbuf_size,
            canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
    }
}


static void bt656_release_canvas(void)
{
    int i = 0;

//    for(;i < am656in_dec_info.canvas_total_count; i++)
//    {
//        canvas_release(VDIN_START_CANVAS + i);
//    }
    return;
}


static inline u32 bt656_index2canvas(u32 index)
{
    if(index < am656in_dec_info.canvas_total_count)
        return bt656_canvas_tab[index];
    else
        return 0xff;
}

/*
    NTSC or PAL input(interlace mode): CLOCK + D0~D7(with SAV + EAV )
*/
static void reinit_bt656in_dec(void)
{
    int temp_data;

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_EN_BIT );
    WRITE_CBUS_REG(BT_CTRL, temp_data); //disable BT656 input

        // reset BT656in module.
    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data |= ( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    WRITE_CBUS_REG(BT_FIELDSADR, (4 << 16) | 4);    // field 0/1 start lcnt: default value
// configuration the BT PORT control
// For standaREAD_CBUS_REG bt656 in stream, there's no HSYNC VSYNC pins.
// So we don't need to configure the port.
    WRITE_CBUS_REG(BT_PORT_CTRL, 1 << BT_D8B);  // data itself is 8 bits.

    WRITE_CBUS_REG(BT_SWAP_CTRL,    ( 4 << 0 ) |        //POS_Y1_IN
                        ( 5 << 4 ) |        //POS_Cr0_IN
                        ( 6 << 8 ) |        //POS_Y0_IN
                        ( 7 << 12 ));       //POS_CB0_IN

    WRITE_CBUS_REG(BT_LINECTRL , 0)  ;
// ANCI is the field blanking data, like close caption. If it connected to digital camara interface, the jpeg bitstream also use this ANCI FIFO.
    WRITE_CBUS_REG(BT_ANCISADR, am656in_dec_info.pbufAddr);
    WRITE_CBUS_REG(BT_ANCIEADR, am656in_dec_info.pbufAddr + BT656IN_ANCI_DATA_SIZE);

    WRITE_CBUS_REG(BT_AFIFO_CTRL,   (1 <<31) |     // load start and end address to afifo.
                        (1 << 6) |     // fill _en;
                        (1 << 3)) ;     // urgent


    WRITE_CBUS_REG(BT_INT_CTRL ,   // (1 << 5) |    //ancififo done int.
//                      (1 << 4) |    //SOF interrupt enable.
//                      (1 << 3) |      //EOF interrupt enable.
                        (1 << 1)); // |      //input overflow interrupt enable.
//                      (1 << 0));      //bt656 controller error interrupt enable.

    WRITE_CBUS_REG(BT_ERR_CNT, (626 << 16) | (1760));

    if(am656in_dec_info.para.fmt  == TVIN_SIG_FMT_BT656IN_576I ) //input is PAL
    {
        WRITE_CBUS_REG(BT_VBIEND,   22 | (22 << 16));       //field 0/1 VBI last line number
        WRITE_CBUS_REG(BT_VIDEOSTART,   23 | (23 << 16));   //Line number of the first video start line in field 0/1.
        WRITE_CBUS_REG(BT_VIDEOEND ,    312 |          //  Line number of the last video line in field 1. added video end for avoid overflow.
                                (312 <<16));                    // Line number of the last video line in field 0
        WRITE_CBUS_REG(BT_CTRL ,    (0 << BT_UPDATE_ST_SEL) |  //Update bt656 status register when end of frame.
                                (1 << BT_COLOR_REPEAT) | //Repeated the color data when do 4:2:2 -> 4:4:4 data transfer.
                                (1 << BT_AUTO_FMT ) |           //use haREAD_CBUS_REGware to check the PAL/NTSC format input format if it's standaREAD_CBUS_REG BT656 input format.
                                (1 << BT_MODE_BIT     ) | // BT656 standaREAD_CBUS_REG interface.
                                (1 << BT_EN_BIT       ) |    // enable BT moduale.
                                (1 << BT_REF_MODE_BIT ) |    // timing reference is from bit stream.
                                (1 << BT_CLK27_SEL_BIT) |    // use external xclk27.
                                (1 << BT_XCLK27_EN_BIT)) ;    // xclk27 is input.
        WRITE_CBUS_REG(VDIN_WR_V_START_END, 287 |     //v end
                                        (0 << 16) );   // v start

    }
    else //if(am656in_dec_info.para.fmt  == TVIN_SIG_FMT_BT656IN_480I) //input is PAL   //input is NTSC
    {
        WRITE_CBUS_REG(BT_VBIEND,   21 | (21 << 16));       //field 0/1 VBI last line number
        WRITE_CBUS_REG(BT_VIDEOSTART,   18 | (18 << 16));   //Line number of the first video start line in field 0/1.
        WRITE_CBUS_REG(BT_VIDEOEND ,    257 |          //  Line number of the last video line in field 1. added video end for avoid overflow.
                                (257 <<16));                    // Line number of the last video line in field 0
        WRITE_CBUS_REG(BT_CTRL ,    (0 << BT_UPDATE_ST_SEL) |  //Update bt656 status register when end of frame.
                                (1 << BT_COLOR_REPEAT) | //Repeated the color data when do 4:2:2 -> 4:4:4 data transfer.
                                (1 << BT_AUTO_FMT ) |       //use haREAD_CBUS_REGware to check the PAL/NTSC format input format if it's standaREAD_CBUS_REG BT656 input format.
                                (1 << BT_MODE_BIT     ) | // BT656 standaREAD_CBUS_REG interface.
                                (1 << BT_EN_BIT       ) |    // enable BT moduale.
                                (1 << BT_REF_MODE_BIT ) |    // timing reference is from bit stream.
                                (1 << BT_CLK27_SEL_BIT) |    // use external xclk27.
                                (1 << BT_XCLK27_EN_BIT) |       // xclk27 is input.
                                (1 << BT_FMT_MODE_BIT));   //input format is NTSC
        WRITE_CBUS_REG(VDIN_WR_V_START_END, 239 |     //v end
                                        (0 << 16) );   // v start

    }

    return;
}

//NTSC or PAL input(interlace mode): CLOCK + D0~D7 + HSYNC + VSYNC + FID
static void reinit_bt601in_dec(void)
{
    int temp_data;

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_EN_BIT );
    WRITE_CBUS_REG(BT_CTRL, temp_data); //disable BT656 input

        // reset BT656in module.
    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data |= ( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    WRITE_CBUS_REG(BT_PORT_CTRL,    (0 << BT_IDQ_EN )   |     // use external idq pin.
                                    (1 << BT_IDQ_PHASE )   |
                                    ( 1 << BT_FID_HSVS ) |         // FID came from HS VS.
                                    ( 1 << BT_HSYNC_PHASE ) |
                                    (1 << BT_D8B )     |
                                    (4 << BT_FID_DELAY ) |
                                    (5 << BT_VSYNC_DELAY) |
                                    (5 << BT_HSYNC_DELAY));

    WRITE_CBUS_REG(BT_601_CTRL2 , ( 10 << 16));     // FID field check done point.

    WRITE_CBUS_REG(BT_SWAP_CTRL,    ( 4 << 0 ) | // suppose the input bitstream format is Cb0 Y0 Cr0 Y1.
                            ( 5 << 4 ) |
                            ( 6 << 8 ) |
                            ( 7 << 13 ) );

    WRITE_CBUS_REG(BT_LINECTRL , ( 1 << 31 ) |   //software line ctrl enable.
                                    (1644 << 16 ) |    //1440 + 204
                                    220 )  ;

    // ANCI is the field blanking data, like close caption. If it connected to digital camara interface, the jpeg bitstream also use this ANCI FIFO.
    WRITE_CBUS_REG(BT_ANCISADR, am656in_dec_info.pbufAddr);
    WRITE_CBUS_REG(BT_ANCIEADR, am656in_dec_info.pbufAddr + BT656IN_ANCI_DATA_SIZE);

    WRITE_CBUS_REG(BT_AFIFO_CTRL,   (1 <<31) |     // load start and end address to afifo.
                                        (1 << 6) |     // fill _en;
                                        (1 << 3)) ;     // urgent

    WRITE_CBUS_REG(BT_INT_CTRL ,   // (1 << 5) |    //ancififo done int.
//                      (1 << 4) |    //SOF interrupt enable.
//                      (1 << 3) |      //EOF interrupt enable.
                        (1 << 1)); // |      //input overflow interrupt enable.
//                      (1 << 0));      //bt656 controller error interrupt enable.
    WRITE_CBUS_REG(BT_ERR_CNT, (626 << 16) | (2000));
                                                                    //otherwise there is always error flag,
                                                                    //because the camera input use HREF ont HSYNC,
                                                                    //there are some lines without HREF sometime
    WRITE_CBUS_REG(BT_FIELDSADR, (1 << 16) | 1);    // field 0/1 start lcnt

    if(am656in_dec_info.para.fmt == TVIN_SIG_FMT_BT601IN_576I) //input is PAL
    {
        WRITE_CBUS_REG(BT_VBIEND, 22 | (22 << 16));     //field 0/1 VBI last line number
        WRITE_CBUS_REG(BT_VIDEOSTART, 23 | (23 << 16)); //Line number of the first video start line in field 0/1.
        WRITE_CBUS_REG(BT_VIDEOEND , 312 |          //  Line number of the last video line in field 1. added video end for avoid overflow.
                                                (312 <<16));                    // Line number of the last video line in field 0
        WRITE_CBUS_REG(BT_CTRL ,    (0 << BT_MODE_BIT     ) |    // BT656 standaREAD_CBUS_REG interface.
                                    (1 << BT_AUTO_FMT )     |
                                    (1 << BT_EN_BIT       ) |    // enable BT moduale.
                                    (0 << BT_REF_MODE_BIT ) |    // timing reference is from bit stream.
                                    (0 << BT_FMT_MODE_BIT ) |     //PAL
                                    (1 << BT_SLICE_MODE_BIT )|    // no ancillay flag.
                                    (0 << BT_FID_EN_BIT )   |     // use external fid pin.
                                    (1 << BT_CLK27_SEL_BIT) |  // use external xclk27.
                                    (1 << BT_XCLK27_EN_BIT) );   // xclk27 is input.
        WRITE_CBUS_REG(VDIN_WR_V_START_END, 287 |     //v end
                                        (0 << 16) );   // v start

     }

    else //if(am656in_dec_info.para.fmt == TVIN_SIG_FMT_BT601IN_480I)   //input is NTSC
    {
        WRITE_CBUS_REG(BT_VBIEND, 21 | (21 << 16));     //field 0/1 VBI last line number
        WRITE_CBUS_REG(BT_VIDEOSTART, 18 | (18 << 16)); //Line number of the first video start line in field 0/1.
        WRITE_CBUS_REG(BT_VIDEOEND , 257 |          //  Line number of the last video line in field 1. added video end for avoid overflow.
                                        (257 <<16));        // Line number of the last video line in field 0
        WRITE_CBUS_REG(BT_CTRL ,(0 << BT_MODE_BIT     ) |    // BT656 standaREAD_CBUS_REG interface.
                                (1 << BT_AUTO_FMT )     |
                                (1 << BT_EN_BIT       ) |    // enable BT moduale.
                                (0 << BT_REF_MODE_BIT ) |    // timing reference is from bit stream.
                                (1 << BT_FMT_MODE_BIT ) |     // NTSC
                                (1 << BT_SLICE_MODE_BIT )|    // no ancillay flag.
                                (0 << BT_FID_EN_BIT )   |     // use external fid pin.
                                (1 << BT_CLK27_SEL_BIT) |  // use external xclk27.
                                (1 << BT_XCLK27_EN_BIT) );   // xclk27 is input.
        WRITE_CBUS_REG(VDIN_WR_V_START_END, 239 |     //v end
                                        (0 << 16) );   // v start

    }

    return;
}

//CAMERA input(progressive mode): CLOCK + D0~D7 + HREF + VSYNC
static void reinit_camera_dec(void)
{
    int temp_data;

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_EN_BIT );
    WRITE_CBUS_REG(BT_CTRL, temp_data); //disable BT656 input

    // reset BT656in module.
    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data |= ( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    temp_data = READ_CBUS_REG(BT_CTRL);
    temp_data &= ~( 1 << BT_SOFT_RESET );
    WRITE_CBUS_REG(BT_CTRL, temp_data);

    WRITE_CBUS_REG(BT_PORT_CTRL,    (0 << BT_IDQ_EN )   |     // use external idq pin.
                                        (0 << BT_IDQ_PHASE )   |
                                        ( 0 << BT_FID_HSVS ) |         // FID came from HS VS.
                                        ( 1 << BT_VSYNC_PHASE ) |
                                        (0 << BT_D8B )     |
                                        (4 << BT_FID_DELAY ) |
                                        (0 << BT_VSYNC_DELAY) |
                                        (0 << BT_HSYNC_DELAY));

    WRITE_CBUS_REG(BT_601_CTRL2 , ( 10 << 16));     // FID field check done point.

    WRITE_CBUS_REG(BT_SWAP_CTRL,    ( 7 << 0 ) | // suppose the input bitstream format is Cb0 Y0 Cr0 Y1.
                            ( 6 << 4 ) |
                            ( 5 << 8 ) |
                            ( 4 << 12 ) );

    WRITE_CBUS_REG(BT_LINECTRL , ( 1 << 31 ) |   //software line ctrl enable.
                                    ((am656in_dec_info.active_pixel << 1)<< 16 ) |    //the number of active data per line
                                    0 )  ;

    // ANCI is the field blanking data, like close caption. If it connected to digital camara interface, the jpeg bitstream also use this ANCI FIFO.
    WRITE_CBUS_REG(BT_ANCISADR, am656in_dec_info.pbufAddr);
    WRITE_CBUS_REG(BT_ANCIEADR, am656in_dec_info.pbufAddr + BT656IN_ANCI_DATA_SIZE);

    WRITE_CBUS_REG(BT_AFIFO_CTRL,   (1 <<31) |     // load start and end address to afifo.
                                        (1 << 6) |     // fill _en;
                                        (1 << 3)) ;     // urgent

    WRITE_CBUS_REG(BT_INT_CTRL ,   // (1 << 5) |    //ancififo done int.
//                      (1 << 4) |    //SOF interrupt enable.
//                      (1 << 3) |      //EOF interrupt enable.
                        (1 << 1)); // |      //input overflow interrupt enable.
//                      (1 << 0));      //bt656 controller error interrupt enable.
    WRITE_CBUS_REG(BT_ERR_CNT, ((2000) << 16) | (2000 * 10));   //total lines per frame and total pixel per line
                                                                    //otherwise there is always error flag,
                                                                    //because the camera input use HREF ont HSYNC,
                                                                    //there are some lines without HREF sometime

    WRITE_CBUS_REG(BT_FIELDSADR, (1 << 16) | 1);    // field 0/1 start lcnt
    WRITE_CBUS_REG(BT_VBIEND, 1 | (1 << 16));       //field 0/1 VBI last line number
    WRITE_CBUS_REG(BT_VIDEOSTART, 1 | (1 << 16));   //Line number of the first video start line in field 0/1.
    WRITE_CBUS_REG(BT_VIDEOEND , am656in_dec_info.active_line|          //  Line number of the last video line in field 1. added video end for avoid overflow.
                                    (am656in_dec_info.active_line <<16));                   // Line number of the last video line in field 0
    WRITE_CBUS_REG(BT_CTRL ,    (1 << BT_EN_BIT       ) |    // enable BT moduale.
                                (0 << BT_REF_MODE_BIT ) |    // timing reference is from bit stream.
                                (0 << BT_FMT_MODE_BIT ) |     //PAL
                                (1 << BT_SLICE_MODE_BIT )|    // no ancillay flag.
                                (0 << BT_MODE_BIT     ) |    // BT656 standard interface.
                                (1 << BT_CLK27_SEL_BIT) |  // use external xclk27.
                                (0 << BT_FID_EN_BIT )   |     // use external fid pin.
                                (1 << BT_XCLK27_EN_BIT) |   // xclk27 is input.
                                (1 << BT_PROG_MODE  )   |
                                (0 << BT_AUTO_FMT    ) );

    return;
}


void set_next_field_656_601_camera_in_anci_address(unsigned char index)
{
    unsigned pbufAddr;
    pbufAddr = am656in_dec_info.pbufAddr + index * 0x200;
//      //set next field ANCI.
    WRITE_CBUS_REG(BT_ANCISADR, pbufAddr);

    WRITE_CBUS_REG(BT_AFIFO_CTRL,   (1 <<31) |     // load start and end address to afifo.
                                    (1 << 6) |     // fill _en;
                                    (1 << 3)) ;     // urgent
}

static void start_amvdec_656_601_camera_in(unsigned int mem_start, unsigned int mem_size,
                        tvin_port_t port, tvin_sig_fmt_t input_mode)
{

    if(am656in_dec_info.dec_status != 0)
    {
        pr_dbg("656_601_camera_in is processing, don't do starting operation \n");
        return;
    }

    pr_dbg("start ");
    if(port == TVIN_PORT_BT656)  //NTSC or PAL input(interlace mode): D0~D7(with SAV + EAV )
    {
        pr_dbg("bt656in decode. \n");
        bt656_656in_canvas_init(mem_start, mem_size);
        init_656in_dec_parameter(TVIN_SIG_FMT_BT656IN_576I);
        reinit_bt656in_dec();
        am656in_dec_info.rd_canvas_index = 0xff - (am656in_dec_info.canvas_total_count + 2);
        am656in_dec_info.wr_canvas_index =  0;
        am656in_dec_info.para.port = TVIN_PORT_BT656;
        am656in_dec_info.dec_status = 1;
    }
    else if(port == TVIN_PORT_BT601)
    {
        pr_dbg("bt601in decode. \n");
        bt656_656in_canvas_init(mem_start, mem_size);
        init_656in_dec_parameter(TVIN_SIG_FMT_BT601IN_576I);
        reinit_bt601in_dec();
        am656in_dec_info.rd_canvas_index = 0xff - (am656in_dec_info.canvas_total_count + 2);
        am656in_dec_info.wr_canvas_index =  0;
        am656in_dec_info.para.port = TVIN_PORT_BT601;
        am656in_dec_info.dec_status = 1;
        if(am656in_dec_info.pin_mux_reg2 != 0)
            SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg2, am656in_dec_info.pin_mux_mask2);  //set the related pin mux
        if(am656in_dec_info.pin_mux_reg3 != 0)
            SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg3, am656in_dec_info.pin_mux_mask3);  //set the related pin mux

    }
    else if(port == TVIN_PORT_CAMERA)
    {
        pr_dbg("camera in decode. \n");
        bt656_camera_in_canvas_init(mem_start, mem_size);
        init_656in_dec_parameter(TVIN_SIG_FMT_CAMERA_1920X1080P_30Hz);
        am656in_dec_info.rd_canvas_index = 0xff - (am656in_dec_info.canvas_total_count + 2);
        am656in_dec_info.wr_canvas_index =  0;
        am656in_dec_info.para.port = TVIN_PORT_CAMERA;
        reinit_camera_dec();
        am656in_dec_info.dec_status = 1;
        if(am656in_dec_info.pin_mux_reg2 != 0)
            SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg2, am656in_dec_info.pin_mux_mask2);  //set the related pin mux
    }
    else
    {
        am656in_dec_info.para.fmt = TVIN_SIG_FMT_NULL;
        am656in_dec_info.para.port = TVIN_PORT_NULL;
        pr_dbg("bt656/bt601/camera input is not selected, please try again. \n");
        return;
    }
    if(am656in_dec_info.pin_mux_reg1 != 0)
        SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg1, am656in_dec_info.pin_mux_mask1);  //set the related pin mux
#if 0
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, 0x3e07fe);
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_7, 0x000fc000);
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_8, 0xffc00000);
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_10, 0xe0000000);
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_12, 0xfff80000);
#endif

    return;
}

static void stop_amvdec_656_601_camera_in(void)
{
    unsigned temp_data;


    if(am656in_dec_info.dec_status)
    {
        pr_dbg("stop 656_601_camera_in decode. \n");
        if(am656in_dec_info.pin_mux_reg1 != 0)
            CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg1, am656in_dec_info.pin_mux_mask1);

        if(am656in_dec_info.pin_mux_reg2 != 0)
            CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg2, am656in_dec_info.pin_mux_mask2);

        if(am656in_dec_info.pin_mux_reg3 != 0)
            CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg3, am656in_dec_info.pin_mux_mask3);  //set the related pin mux

        temp_data = READ_CBUS_REG(BT_CTRL);
        temp_data &= ~( 1 << BT_EN_BIT );
        WRITE_CBUS_REG(BT_CTRL, temp_data); //disable BT656 input

        //reset 656 input module
        temp_data = READ_CBUS_REG(BT_CTRL);
        temp_data |= ( 1 << BT_SOFT_RESET );
        WRITE_CBUS_REG(BT_CTRL, temp_data);
        temp_data = READ_CBUS_REG(BT_CTRL);
        temp_data &= ~( 1 << BT_SOFT_RESET );
        WRITE_CBUS_REG(BT_CTRL, temp_data);

        bt656_release_canvas();

        am656in_dec_info.para.fmt = TVIN_SIG_FMT_NULL;
        am656in_dec_info.dec_status = 0;

    }
    else
    {
         pr_dbg("656_601_camera_in is not started yet. \n");
    }

    return;
}

static void check_amvdec_656_601_camera_fromat( void )
{
    enum tvin_sig_fmt_e format ;
    unsigned  active_line, active_pixel;
    if((vdin_devp_bt656 != NULL) && (am656in_dec_info.dec_status))
    {

        am656in_dec_info.para.cap_flag = vdin_devp_bt656->para.cap_flag;
//        field_total_line = READ_CBUS_REG(BT_VLINE_STATUS);
//        field_total_line &= 0xfff;
        active_line = vdin_get_active_v();
        active_pixel = vdin_get_active_h();
        switch(am656in_dec_info.para.port)
        {
            case TVIN_PORT_CAMERA:
                for(format = (TVIN_SIG_FMT_BT601IN_480I + 1); format < TVIN_SIG_FMT_MAX; format++)
                {
                    if((active_line >= (tvin_fmt_tbl[format].v_active- tvin_fmt_tbl[format].v_cnt_offset)) &&
                        (active_line <= (tvin_fmt_tbl[format].v_active + tvin_fmt_tbl[format].v_cnt_offset)))
                    {
                        if((active_pixel >= (tvin_fmt_tbl[format].h_active- tvin_fmt_tbl[format].h_cnt_offset)) &&
                            (active_pixel <= (tvin_fmt_tbl[format].h_active+ tvin_fmt_tbl[format].h_cnt_offset)))
                           break;
                    }

                }
                if(format >= TVIN_SIG_FMT_MAX)
                {
                   format =  TVIN_SIG_FMT_NULL;
                }
                break;

             case  TVIN_PORT_BT601:
                if((active_pixel > 640) && (active_pixel < 800))
                {
                      if((active_line < 200) || (active_line > 320))  //skip current field
                     {
                         format =  TVIN_SIG_FMT_NULL;
                     }

                     else if(active_line < 264)  //current field total line number is 240
                     {
                         format = TVIN_SIG_FMT_BT601IN_480I;
                     }

                     else
                     {
                         format = TVIN_SIG_FMT_BT601IN_576I;
                     }
                }
                else
                {
                    format = TVIN_SIG_FMT_NULL;
                }
                break;

             case  TVIN_PORT_BT656:
                if((active_pixel > 640) && (active_pixel < 800))
                {
                     if((active_line < 200) || (active_line > 320))  //skip current field
                     {
                         format =  TVIN_SIG_FMT_NULL;
                     }

                     else if(active_line < 264)  //current field total line number is 240
                     {
                         format = TVIN_SIG_FMT_BT656IN_480I;
                     }

                     else
                     {
                         format = TVIN_SIG_FMT_BT656IN_576I;
                     }
                }
                else
                {
                    format = TVIN_SIG_FMT_NULL;
                }
                break;

             default:
                break;
        }

        if(am656in_dec_info.para.fmt != format)
        {
            if(am656in_dec_info.fmt_check_cnt >= STATUS_ANTI_SHOCKING)
            {
                am656in_dec_info.para.fmt = format;
                switch(am656in_dec_info.para.port)
                {
                    case TVIN_PORT_CAMERA:
                        if(format ==  TVIN_SIG_FMT_NULL)
                        {
                            if(active_line == 0)
                                am656in_dec_info.para.status = TVIN_SIG_STATUS_NOSIG;
                            else if(vdin_devp_bt656->meas_th >= MINIMUM_H_CNT)
                                am656in_dec_info.para.status = TVIN_SIG_STATUS_UNSTABLE;
                            else
                                am656in_dec_info.para.status = TVIN_SIG_STATUS_NOTSUP;
                        }
                        else
                        {
                            am656in_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
                            init_656in_dec_parameter(format);
                            reinit_camera_dec();
                        }
                        break;

                     case  TVIN_PORT_BT656:
                         if(format ==  TVIN_SIG_FMT_NULL)
                         {
                             am656in_dec_info.para.status = TVIN_SIG_STATUS_UNSTABLE;
//                             pr_dbg("active_line = %d, active_pixel = %d. \n", active_line, active_pixel);
                         }
                         else
                         {
                             am656in_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
                             init_656in_dec_parameter(format);
                             reinit_bt656in_dec();
//                             pr_dbg("update bt656 input format, cap_flag = %d, format = %d, status = %d, vdin_devp_bt656->para.fmt = %d .\n",
//                                     am656in_dec_info.para.cap_flag, am656in_dec_info.para.fmt, am656in_dec_info.para.status, vdin_devp_bt656->para.fmt);
                         }
                        break;

                     case  TVIN_PORT_BT601:
                         if(format ==  TVIN_SIG_FMT_NULL)
                         {
                             am656in_dec_info.para.status = TVIN_SIG_STATUS_UNSTABLE;
                         }
                         else
                         {
                             am656in_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
                             init_656in_dec_parameter(format);
                             reinit_bt601in_dec();
                         }

                        break;

                     default:
                        break;
                }

            }
            else
               am656in_dec_info.fmt_check_cnt++;
        }
        else
           am656in_dec_info.fmt_check_cnt = 0;

    }
    else
    {
        pr_err("check_amvdec_656_601_camera_fromat: vdin_devp_bt656 is null. \n");
    }
    return ;
}


#ifdef HANDLE_BT656IN_IRQ
static irqreturn_t bt656in_dec_irq(int irq, void *dev_id)
{

}
#endif

//bt656 flag error = (pixel counter error) | (line counter error) | (input FIFO over flow)
static void bt656_in_dec_run(vframe_t * info)
{
    unsigned ccir656_status;
    unsigned char last_receiver_buff_index, canvas_id;

    ccir656_status = READ_CBUS_REG(BT_STATUS);

    if(ccir656_status & 0x80)
    {
        pr_dbg("bt656 input FIFO over flow, reinit \n");
        reinit_bt656in_dec();
        return ;
    }

    else
    {
        if(am656in_dec_info.para.cap_flag)
        {
            if(am656in_dec_info.wr_canvas_index == 0)
                  last_receiver_buff_index = am656in_dec_info.canvas_total_count - 1;
            else
                  last_receiver_buff_index = am656in_dec_info.wr_canvas_index - 1;

            if(last_receiver_buff_index != am656in_dec_info.rd_canvas_index)
                canvas_id = last_receiver_buff_index;
            else
            {
                if(am656in_dec_info.wr_canvas_index == am656in_dec_info.canvas_total_count - 1)
                      canvas_id = 0;
                else
                      canvas_id = am656in_dec_info.wr_canvas_index + 1;
            }

            vdin_devp_bt656->para.cap_addr = am656in_dec_info.pbufAddr +
                    (am656in_dec_info.decbuf_size * canvas_id) + BT656IN_ANCI_DATA_SIZE ;
        }

        else
        {
            if(am656in_dec_info.wr_canvas_index == 0)
                  last_receiver_buff_index = am656in_dec_info.canvas_total_count - 1;
            else
                  last_receiver_buff_index = am656in_dec_info.wr_canvas_index - 1;

            if(ccir656_status & 0x1000)  // previous field is field 1.
            {
                am656in_dec_info.buff_flag[am656in_dec_info.wr_canvas_index] = VIDTYPE_INTERLACE_BOTTOM;
                //make sure that the data type received ..../top/bottom/top/buttom/top/buttom/....
                if((am656in_dec_info.buff_flag[last_receiver_buff_index] & 0x0f) == VIDTYPE_INTERLACE_BOTTOM)  //pre field type is the same to cur field
                {
                    return;
                }

            }
            else
            {
                am656in_dec_info.buff_flag[am656in_dec_info.wr_canvas_index] = VIDTYPE_INTERLACE_TOP;
                //make sure that the data type received ..../top/bottom/top/buttom/top/buttom/....
                if((am656in_dec_info.buff_flag[last_receiver_buff_index] & 0x0f) == VIDTYPE_INTERLACE_TOP)  //pre field type is the same to cur field
                {
                    return;
                }

            }

            if(am656in_dec_info.para.fmt == TVIN_SIG_FMT_BT656IN_576I)     //PAL data
                am656in_dec_info.buff_flag[am656in_dec_info.wr_canvas_index] |= 0x10;

            if((am656in_dec_info.rd_canvas_index > 0xf0) && (am656in_dec_info.rd_canvas_index < 0xff))
             {
                 am656in_dec_info.rd_canvas_index += 1;
                 am656in_dec_info.wr_canvas_index += 1;
                 if(am656in_dec_info.wr_canvas_index > (am656in_dec_info.canvas_total_count -1) )
                     am656in_dec_info.wr_canvas_index = 0;
                 canvas_id = bt656_index2canvas(am656in_dec_info.wr_canvas_index);
                 vdin_set_canvas_id(vdin_devp_bt656, canvas_id);
                 set_next_field_656_601_camera_in_anci_address(am656in_dec_info.wr_canvas_index);
                 return;
               }

             else if(am656in_dec_info.rd_canvas_index == 0xff)
             {
                am656in_dec_info.rd_canvas_index = 0;
             }

            else
            {
                am656in_dec_info.rd_canvas_index++;
                if(am656in_dec_info.rd_canvas_index > (am656in_dec_info.canvas_total_count -1))
                {
                    am656in_dec_info.rd_canvas_index = 0;
                }
            }

            if((am656in_dec_info.buff_flag[am656in_dec_info.rd_canvas_index] & 0xf) == VIDTYPE_INTERLACE_BOTTOM)  // current field is field 1.
            {
                info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_BOTTOM;
            }
            else
            {
                info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_TOP;
            }
            info->canvas0Addr = info->canvas1Addr = bt656_index2canvas(am656in_dec_info.rd_canvas_index);

            if(am656in_dec_info.buff_flag[am656in_dec_info.rd_canvas_index] & 0x10)     //PAL
            {
                info->width= 720;
                info->height = 576;
                info->duration = 1920;
            }
            else
            {
                info->width= 720;
                info->height = 480;
                info->duration = 1600;
            }


            am656in_dec_info.wr_canvas_index += 1;
            if(am656in_dec_info.wr_canvas_index > (am656in_dec_info.canvas_total_count -1) )
                am656in_dec_info.wr_canvas_index = 0;
            canvas_id = bt656_index2canvas(am656in_dec_info.wr_canvas_index);
            info->index = canvas_id;
            vdin_set_canvas_id(vdin_devp_bt656, canvas_id);
            set_next_field_656_601_camera_in_anci_address(am656in_dec_info.wr_canvas_index);
        }
    }
    return;

}

static void bt601_in_dec_run(vframe_t * info)
{
    unsigned ccir656_status;
    unsigned char last_receiver_buff_index, canvas_id;

    ccir656_status = READ_CBUS_REG(BT_STATUS);

    if(ccir656_status & 0x80)
    {
        pr_dbg("bt601 input FIFO over flow, reinit \n");
        reinit_bt601in_dec();
        return ;
    }

    else
    {
        if(am656in_dec_info.para.cap_flag)
        {
            if(am656in_dec_info.wr_canvas_index == 0)
                  last_receiver_buff_index = am656in_dec_info.canvas_total_count - 1;
            else
                  last_receiver_buff_index = am656in_dec_info.wr_canvas_index - 1;

            if(last_receiver_buff_index != am656in_dec_info.rd_canvas_index)
                canvas_id = last_receiver_buff_index;
            else
            {
                if(am656in_dec_info.wr_canvas_index == am656in_dec_info.canvas_total_count - 1)
                      canvas_id = 0;
                else
                      canvas_id = am656in_dec_info.wr_canvas_index + 1;
            }

            vdin_devp_bt656->para.cap_addr = am656in_dec_info.pbufAddr +
                    (am656in_dec_info.decbuf_size * canvas_id) + BT656IN_ANCI_DATA_SIZE ;
        }

        else
        {

            if(ccir656_status & 0x10)  // current field is field 1.
            {
                info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_BOTTOM;
            }
            else
            {
                info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_TOP;
            }

            if(am656in_dec_info.para.fmt == TVIN_SIG_FMT_BT601IN_576I)  //the data in the buffer is PAL
            {
                info->width= 720;
                info->height = 576;
                info->duration = 1920;
            }
            else
            {
                info->width= 720;
                info->height = 480;
                info->duration = 1600;
            }
        }
    }

    return;

}

static void camera_in_dec_run(vframe_t * info)
{
    unsigned ccir656_status;
    unsigned char last_receiver_buff_index, canvas_id;

    ccir656_status = READ_CBUS_REG(BT_STATUS);

    if(ccir656_status & 0x80)
    {
        pr_dbg("camera input FIFO over flow, reinit \n");
        reinit_camera_dec();
        return ;
    }

    else
    {
        if(am656in_dec_info.para.cap_flag)
        {
            if(am656in_dec_info.wr_canvas_index == 0)
                  last_receiver_buff_index = am656in_dec_info.canvas_total_count - 1;
            else
                  last_receiver_buff_index = am656in_dec_info.wr_canvas_index - 1;

            if(last_receiver_buff_index != am656in_dec_info.rd_canvas_index)
                canvas_id = last_receiver_buff_index;
            else
            {
                if(am656in_dec_info.wr_canvas_index == am656in_dec_info.canvas_total_count - 1)
                      canvas_id = 0;
                else
                      canvas_id = am656in_dec_info.wr_canvas_index + 1;
            }

            vdin_devp_bt656->para.cap_addr = am656in_dec_info.pbufAddr +
                    (am656in_dec_info.decbuf_size * canvas_id) + BT656IN_ANCI_DATA_SIZE ;
        }

        else
        {

            if((am656in_dec_info.rd_canvas_index > 0xf0) && (am656in_dec_info.rd_canvas_index < 0xff))
             {
                 am656in_dec_info.rd_canvas_index += 1;
                 am656in_dec_info.wr_canvas_index += 1;
                 if(am656in_dec_info.wr_canvas_index > (am656in_dec_info.canvas_total_count -1) )
                     am656in_dec_info.wr_canvas_index = 0;
                 canvas_id = bt656_index2canvas(am656in_dec_info.wr_canvas_index);
                 vdin_set_canvas_id(vdin_devp_bt656, canvas_id);
                 set_next_field_656_601_camera_in_anci_address(am656in_dec_info.wr_canvas_index);
                 return;
            }

             else if(am656in_dec_info.rd_canvas_index == 0xff)
             {
                am656in_dec_info.rd_canvas_index = 0;
             }

            else
            {
                am656in_dec_info.rd_canvas_index++;
                if(am656in_dec_info.rd_canvas_index > (am656in_dec_info.canvas_total_count -1))
                {
                    am656in_dec_info.rd_canvas_index = 0;
                }
            }


            info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_PROGRESSIVE ;
            info->width= am656in_dec_info.active_pixel;
            info->height = am656in_dec_info.active_line;
            info->duration = 9600000/1716;   //17.16 frame per second
            info->canvas0Addr = info->canvas1Addr = bt656_index2canvas(am656in_dec_info.rd_canvas_index);
            am656in_dec_info.wr_canvas_index += 1;
            if(am656in_dec_info.wr_canvas_index > (am656in_dec_info.canvas_total_count -1) )
                am656in_dec_info.wr_canvas_index = 0;
            canvas_id = bt656_index2canvas(am656in_dec_info.wr_canvas_index);
            info->index = canvas_id;
            vdin_set_canvas_id(vdin_devp_bt656, canvas_id);
            set_next_field_656_601_camera_in_anci_address(am656in_dec_info.wr_canvas_index);
        }
    }

    return;
}


int amvdec_656_601_camera_in_run(vframe_t *info)
{
    unsigned ccir656_status;


    if(am656in_dec_info.dec_status == 0){
        pr_error("bt656in decoder is not started\n");
        return -1;
    }

    ccir656_status = READ_CBUS_REG(BT_STATUS);
    WRITE_CBUS_REG(BT_STATUS, ccir656_status | (1 << 9));   //WRITE_CBUS_REGite 1 to clean the SOF interrupt bit

    if(am656in_dec_info.para.port == TVIN_PORT_BT656)  //NTSC or PAL input(interlace mode): D0~D7(with SAV + EAV )
    {
        bt656_in_dec_run(info);
    }
    else if(am656in_dec_info.para.port == TVIN_PORT_BT601)
    {
        bt601_in_dec_run(info);
    }
    else if(am656in_dec_info.para.port == TVIN_PORT_CAMERA)
    {
        camera_in_dec_run(info);
    }
    else
        return -1;
    return 0;
}

static int bt656in_check_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    switch(cmd)
    {
        case TVIN_EVENT_INFO_CHECK:
            if(vdin_devp_bt656 != NULL)
            {
                //pr_dbg("bt656in format update: cap_flag = %d, format = %d, status = %d ",
                //vdin_devp_bt656->para.cap_flag, vdin_devp_bt656->para.fmt, vdin_devp_bt656->para.status);
                check_amvdec_656_601_camera_fromat();
                if (am656in_dec_info.para.fmt != vdin_devp_bt656->para.fmt)
                {
                    vdin_info_update(vdin_devp_bt656, &am656in_dec_info.para);
                }
            }
            break;
        default:
            //pr_dbg("command is not exit ./n");
            break;
    }
    return 0;
}


struct tvin_dec_ops_s bt656in_op = {
        .dec_run = amvdec_656_601_camera_in_run,
    };


static struct notifier_block bt656_check_notifier = {
    .notifier_call  = bt656in_check_callback,
};


static int bt656in_notifier_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    vdin_dev_t *p = NULL;
    switch(cmd)
    {
        case TVIN_EVENT_DEC_START:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                pr_dbg("bt656in_notifier_callback, para = %x ,mem_start = %x, port = %d, format = %d, ca-_flag = %d.\n" ,
                        (unsigned int)para, p->mem_start, p->para.port, p->para.fmt, p->para.cap_flag);
            //        pr_dbg("bt656in decode start.\n" );
                if((p->para.port != TVIN_PORT_BT656) && (p->para.port != TVIN_PORT_BT601) && (p->para.port != TVIN_PORT_CAMERA))
                    return -1;
                start_amvdec_656_601_camera_in(p->mem_start,p->mem_size, p->para.port, p->para.fmt);
                am656in_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
                vdin_info_update(p, &am656in_dec_info.para);
                tvin_dec_register(p, &bt656in_op);
                tvin_check_notifier_register(&bt656_check_notifier);
                vdin_devp_bt656 = p;
            }
            break;

        case TVIN_EVENT_DEC_STOP:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                if((p->para.port != TVIN_PORT_BT656) && (p->para.port != TVIN_PORT_BT601) && (p->para.port != TVIN_PORT_CAMERA))
                    return -1;
                stop_amvdec_656_601_camera_in();
                tvin_dec_unregister(vdin_devp_bt656);
                tvin_check_notifier_unregister(&bt656_check_notifier);
                vdin_devp_bt656 = NULL;
            }
            break;

        default:
//        pr_dbg("command is not exit ./n");
            break;
    }
    return 0;
}



//static int am656in_open(struct inode *node, struct file *file)
//{
//   am656in_dev_t *bt656_in_devp;

    /* Get the per-device structure that contains this cdev */
//    bt656_in_devp = container_of(node->i_cdev, am656in_dev_t, cdev);
//    file->private_data = bt656_in_devp;

//  return 0;

//}


//static int am656in_release(struct inode *node, struct file *file)
//{
//    am656in_dev_t *bt656_in_devp = file->private_data;

    /* Reset file pointer */
//    bt656_in_devp->current_pointer = 0;

    /* Release some other fields */
    /* ... */
//    return 0;
//}



//static int am656in_ioctl(struct inode *node, struct file *file, unsigned int cmd,   unsigned long args)
//{
//  int   r = 0;
//  switch (cmd) {
//      case BT656_DECODERIO_START:
//                    start_bt656in_dec();
//          break;

//      case BT656_DECODERIO_STOP:
//                    stop_bt656in_dec();
//          break;

//            default:

//                    break;
//  }
//  return r;
//}

//const static struct file_operations am656in_fops = {
//    .owner    = THIS_MODULE,
//    .open     = am656in_open,
//    .release  = am656in_release,
//    .ioctl    = am656in_ioctl,
//};

static struct notifier_block bt656_notifier = {
    .notifier_call  = bt656in_notifier_callback,
};

static int amvdec_656in_probe(struct platform_device *pdev)
{
    int r = 0;
//    unsigned pbufSize;
    struct resource *s;

    pr_dbg("amvdec_656in probe start.\n");



//    r = alloc_chREAD_CBUS_REGev_region(&am656in_id, 0, BT656IN_COUNT, DEVICE_NAME);
//    if (r < 0) {
//        pr_error("Can't register major for am656indec device\n");
//        return r;
//    }

//    am656in_class = class_create(THIS_MODULE, DEVICE_NAME);
//    if (IS_ERR(am656in_class))
//    {
//        unregister_chREAD_CBUS_REGev_region(am656in_id, BT656IN_COUNT);
//        return PTR_ERR(aoe_class);
//    }

//    cdev_init(&am656in_dev_->cdev, &am656in_fops);
//    &am656in_dev_->cdev.owner = THIS_MODULE;
//    cdev_add(&am656in_dev_->cdev, am656in_id, BT656IN_COUNT);

//    am656in_dev = device_create(am656in_class, NULL, am656in_id, "bt656indec%d", 0);
//      if (am656in_dev == NULL) {
//              pr_error("device_create create error\n");
//              class_destroy(am656in_class);
//              r = -EEXIST;
//              return r;
//      }

    s = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(s != NULL)
    {
            am656in_dec_info.pin_mux_mask1 = (unsigned )(s->start);
            am656in_dec_info.pin_mux_reg1 = ((unsigned )(s->end) - am656in_dec_info.pin_mux_mask1 ) & 0xffff;
            pr_dbg(" bt656in pin_mux_reg1 is %x, pin_mux_mask1 is %x . \n",
                am656in_dec_info.pin_mux_reg1,am656in_dec_info.pin_mux_mask1);

//       if(am656in_dec_info.pin_mux_reg1 != 0)
//          SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg1, am656in_dec_info.pin_mux_mask1);
     }
     else
             pr_error("error in getting bt656 resource parameters \n");

    s = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(s != NULL)
    {
            am656in_dec_info.pin_mux_mask2 = (unsigned )(s->start);
            am656in_dec_info.pin_mux_reg2 = ((unsigned )(s->end) - am656in_dec_info.pin_mux_mask2 ) & 0xffff;
            pr_dbg(" bt656in pin_mux_reg2 is %x, pin_mux_mask2 is %x . \n",
                am656in_dec_info.pin_mux_reg2,am656in_dec_info.pin_mux_mask2);
//       if(am656in_dec_info.pin_mux_reg2 != 0)
//          SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg2, am656in_dec_info.pin_mux_mask2);
     }
     else
             pr_error("error in getting bt601 resource parameters \n");

    s = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if(s != NULL)
    {
            am656in_dec_info.pin_mux_mask3 = (unsigned )(s->start);
            am656in_dec_info.pin_mux_reg3 = ((unsigned )(s->end) - am656in_dec_info.pin_mux_mask3 ) & 0xffff;
            pr_dbg(" bt656in pin_mux_reg3 is %x, pin_mux_mask3 is %x . \n",
                am656in_dec_info.pin_mux_reg3,am656in_dec_info.pin_mux_mask3);
//          if(am656in_dec_info.pin_mux_reg3 != 0)
//              SET_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg3, am656in_dec_info.pin_mux_mask3);
     }
     else
             pr_error("error in getting camera resource parameters \n");

#ifdef HANDLE_BT656IN_IRQ
        if (request_irq(INT_BT656, bt656in_dec_irq, IRQF_SHARED, BT656IN_IRQ_NAME, (void *)bt656in_dec_id)
        {
                pr_error("bt656in irq register error.\n");
                return -ENOENT;
        }
#endif
        tvin_dec_notifier_register(&bt656_notifier);

        pr_dbg("amvdec_656in probe end.\n");
        return r;
}

static int amvdec_656in_remove(struct platform_device *pdev)
{
    /* Remove the cdev */
#ifdef HANDLE_BT656IN_IRQ
        free_irq(INT_BT656,(void *)bt656in_dec_id);
#endif

//    cdev_del(&am656in_dev_->cdev);

//    device_destroy(am656in_class, am656in_id);

//    class_destroy(am656in_class);

//    unregister_chREAD_CBUS_REGev_region(am656in_id, BT656IN_COUNT);
      tvin_dec_notifier_unregister(&bt656_notifier);
//      tvin_check_notifier_unregister(&bt656_check_notifier);

      if(am656in_dec_info.pin_mux_reg1 != 0)
         CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg1, am656in_dec_info.pin_mux_mask1);

      if(am656in_dec_info.pin_mux_reg2 != 0)
         CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg2, am656in_dec_info.pin_mux_mask2);

      if(am656in_dec_info.pin_mux_reg3 != 0)
         CLEAR_CBUS_REG_MASK(am656in_dec_info.pin_mux_reg3, am656in_dec_info.pin_mux_mask3);
    return 0;
}



static struct platform_driver amvdec_656in_driver = {
    .probe      = amvdec_656in_probe,
    .remove     = amvdec_656in_remove,
    .driver     = {
        .name   = DEVICE_NAME,
    }
};

static int __init amvdec_656in_init_module(void)
{
    pr_dbg("amvdec_656in module init\n");
    if (platform_driver_register(&amvdec_656in_driver)) {
        pr_error("failed to register amvdec_656in driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit amvdec_656in_exit_module(void)
{
    pr_dbg("amvdec_656in module remove.\n");
    platform_driver_unregister(&amvdec_656in_driver);
    return ;
}



module_init(amvdec_656in_init_module);
module_exit(amvdec_656in_exit_module);

MODULE_DESCRIPTION("AMLOGIC BT656_601 input driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

