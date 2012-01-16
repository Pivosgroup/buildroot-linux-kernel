/*
 * TVIN global definition
 * enum, structure & global parameters used in all TVIN modules.
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


#ifndef __TVIN_GLOBAL_H
#define __TVIN_GLOBAL_H

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************
#define STATUS_ANTI_SHOCKING    3
#define MINIMUM_H_CNT       1400


typedef enum tvin_sync_pol_e {
	TVIN_SYNC_POL_NULL = 0,
    TVIN_SYNC_POL_NEGATIVE,
    TVIN_SYNC_POL_POSITIVE,
} tvin_sync_pol_t;

typedef enum tvin_scan_mode_e {
	TVIN_SCAN_MODE_NULL = 0,
    TVIN_SCAN_MODE_PROGRESSIVE,
    TVIN_SCAN_MODE_INTERLACED,
} tvin_scan_mode_t;

typedef enum tvin_color_space_e {
    TVIN_CS_RGB444 = 0,
    TVIN_CS_YUV444,
    TVIN_CS_YUV422_16BITS,
    TVIN_CS_YCbCr422_8BITS,
    TVIN_CS_MAX
} tvin_color_space_t;


// ***************************************************************************
// *** structure definitions *********************************************
// ***************************************************************************
//      Hs_cnt        Pixel_Clk(Khz/10)

typedef struct tvin_format_s {
    unsigned short         h_active;        //Th in the unit of pixel
    unsigned short         v_active;        //Tv in the unit of line
    unsigned short         h_cnt;           //Th in the unit of T, while 1/T = 24MHz or 27MHz or even 100MHz
    unsigned short         h_cnt_offset;    //Tolerance of h_cnt
    unsigned short         v_cnt_offset;    //Tolerance of v_cnt
    unsigned short         hs_cnt;          //Ths in the unit of T, while 1/T = 24MHz or 27MHz or even 100MHz
    unsigned short         hs_cnt_offset;   //Tolerance of hs_cnt
    unsigned short         h_total;         //Th in the unit of pixel
    unsigned short         v_total;         //Tv in the unit of line
    unsigned short         hs_front;        //h front proch
    unsigned short         hs_width;        //HS in the unit of pixel
    unsigned short         hs_bp;           //HS in the unit of pixel
    unsigned short         vs_front;        //v front proch
    unsigned short         vs_width;        //VS in the unit of pixel
    unsigned short         vs_bp;           //HS in the unit of pixel
    enum tvin_sync_pol_e   hs_pol;
    enum tvin_sync_pol_e   vs_pol;
    enum tvin_scan_mode_e  scan_mode;
    unsigned short         pixel_clk;       //(Khz/10)
    unsigned short         vbi_line_start;
    unsigned short         vbi_line_end;
} tvin_format_t;



#define VDIN_DEBUG
#ifdef VDIN_DEBUG
#define pr_dbg(fmt, args...) printk(fmt,## args)
#else
#define pr_dbg(fmt, args...)
#endif
#define pr_error(fmt, args...) printk(fmt,## args)


#define VDIN_START_CANVAS               70U
#define BT656IN_VF_POOL_SIZE            8

#define TVAFE_VF_POOL_SIZE              8
#define VDIN_VF_POOL_MAX_SIZE           8

#define BT656IN_ANCI_DATA_SIZE          0x4000 //save anci data from bt656in
#define CAMERA_IN_ANCI_DATA_SIZE        0x4000 //save anci data from bt656in


#define TVIN_MAX_PIXS					1920*1080

#endif // __TVIN_GLOBAL_H

