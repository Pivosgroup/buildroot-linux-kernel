/*
 * TVIN Modules Exported Header File
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


#ifndef __TVIN_H
#define __TVIN_H

// ***************************************************************************
// *** TVIN general definition/enum/struct ***********************************
// ***************************************************************************

/* tvin input port select */
typedef enum tvin_port_e {
    TVIN_PORT_NULL    = 0x00000000,
    TVIN_PORT_MPEG0   = 0x00000100,
    TVIN_PORT_BT656   = 0x00000200,
    TVIN_PORT_BT601,
    TVIN_PORT_CAMERA,
    TVIN_PORT_VGA0    = 0x00000400,
    TVIN_PORT_VGA1,
    TVIN_PORT_VGA2,
    TVIN_PORT_VGA3,
    TVIN_PORT_VGA4,
    TVIN_PORT_VGA5,
    TVIN_PORT_VGA6,
    TVIN_PORT_VGA7,
    TVIN_PORT_COMP0   = 0x00000800,
    TVIN_PORT_COMP1,
    TVIN_PORT_COMP2,
    TVIN_PORT_COMP3,
    TVIN_PORT_COMP4,
    TVIN_PORT_COMP5,
    TVIN_PORT_COMP6,
    TVIN_PORT_COMP7,
    TVIN_PORT_CVBS0   = 0x00001000,
    TVIN_PORT_CVBS1,
    TVIN_PORT_CVBS2,
    TVIN_PORT_CVBS3,
    TVIN_PORT_CVBS4,
    TVIN_PORT_CVBS5,
    TVIN_PORT_CVBS6,
    TVIN_PORT_CVBS7,
    TVIN_PORT_SVIDEO0 = 0x00002000,
    TVIN_PORT_SVIDEO1,
    TVIN_PORT_SVIDEO2,
    TVIN_PORT_SVIDEO3,
    TVIN_PORT_SVIDEO4,
    TVIN_PORT_SVIDEO5,
    TVIN_PORT_SVIDEO6,
    TVIN_PORT_SVIDEO7,
    TVIN_PORT_HDMI0   = 0x00004000,
    TVIN_PORT_HDMI1,
    TVIN_PORT_HDMI2,
    TVIN_PORT_HDMI3,
    TVIN_PORT_HDMI4,
    TVIN_PORT_HDMI5,
    TVIN_PORT_HDMI6,
    TVIN_PORT_HDMI7,
    TVIN_PORT_DVIN0   = 0x00008000,
    TVIN_PORT_MAX     = 0x80000000,
} tvin_port_t;

/* tvin signal format table */
typedef enum tvin_sig_fmt_e {
    TVIN_SIG_FMT_NULL = 0,
    //VGA Formats
    TVIN_SIG_FMT_VGA_512X384P_60D147,
    TVIN_SIG_FMT_VGA_560X384P_60D147,
    TVIN_SIG_FMT_VGA_640X200P_59D924,
    TVIN_SIG_FMT_VGA_640X350P_85D080,
    TVIN_SIG_FMT_VGA_640X400P_59D940,
    TVIN_SIG_FMT_VGA_640X400P_85D080,
    TVIN_SIG_FMT_VGA_640X400P_59D638,
    TVIN_SIG_FMT_VGA_640X400P_56D416,
    TVIN_SIG_FMT_VGA_640X480I_29D970,
    TVIN_SIG_FMT_VGA_640X480P_66D619,
    TVIN_SIG_FMT_VGA_640X480P_66D667,
    TVIN_SIG_FMT_VGA_640X480P_59D940,
    TVIN_SIG_FMT_VGA_640X480P_60D000,
    TVIN_SIG_FMT_VGA_640X480P_72D809,
    TVIN_SIG_FMT_VGA_640X480P_75D000_A,
    TVIN_SIG_FMT_VGA_640X480P_85D008,
    TVIN_SIG_FMT_VGA_640X480P_59D638,
    TVIN_SIG_FMT_VGA_640X480P_75D000_B,
    TVIN_SIG_FMT_VGA_640X870P_75D000,
    TVIN_SIG_FMT_VGA_720X350P_70D086,
    TVIN_SIG_FMT_VGA_720X400P_85D039,
    TVIN_SIG_FMT_VGA_720X400P_70D086,
    TVIN_SIG_FMT_VGA_720X400P_87D849,
    TVIN_SIG_FMT_VGA_720X400P_59D940,
    TVIN_SIG_FMT_VGA_720X480P_59D940,
    TVIN_SIG_FMT_VGA_752X484I_29D970,
    TVIN_SIG_FMT_VGA_768X574I_25D000,
    TVIN_SIG_FMT_VGA_800X600P_56D250,
    TVIN_SIG_FMT_VGA_800X600P_60D317,
    TVIN_SIG_FMT_VGA_800X600P_72D188,
    TVIN_SIG_FMT_VGA_800X600P_75D000,
    TVIN_SIG_FMT_VGA_800X600P_85D061,
    TVIN_SIG_FMT_VGA_832X624P_75D087,
    TVIN_SIG_FMT_VGA_848X480P_84D751,
    TVIN_SIG_FMT_VGA_1024X768P_59D278,
    TVIN_SIG_FMT_VGA_1024X768P_74D927,
    TVIN_SIG_FMT_VGA_1024X768I_43D479,
    TVIN_SIG_FMT_VGA_1024X768P_60D004,
    TVIN_SIG_FMT_VGA_1024X768P_70D069,
    TVIN_SIG_FMT_VGA_1024X768P_75D029,
    TVIN_SIG_FMT_VGA_1024X768P_84D997,
    TVIN_SIG_FMT_VGA_1024X768P_60D000,
    TVIN_SIG_FMT_VGA_1024X768P_74D925,
    TVIN_SIG_FMT_VGA_1024X768P_75D020,
    TVIN_SIG_FMT_VGA_1024X768P_70D008,
    TVIN_SIG_FMT_VGA_1024X768P_75D782,
    TVIN_SIG_FMT_VGA_1024X768P_77D069,
    TVIN_SIG_FMT_VGA_1024X768P_71D799,
    TVIN_SIG_FMT_VGA_1024X1024P_60D000,
    TVIN_SIG_FMT_VGA_1053X754I_43D453,
    TVIN_SIG_FMT_VGA_1056X768I_43D470,
    TVIN_SIG_FMT_VGA_1120X750I_40D021,
    TVIN_SIG_FMT_VGA_1152X864P_70D012,
    TVIN_SIG_FMT_VGA_1152X864P_75D000,
    TVIN_SIG_FMT_VGA_1152X864P_84D999,
    TVIN_SIG_FMT_VGA_1152X870P_75D062,
    TVIN_SIG_FMT_VGA_1152X900P_65D950,
    TVIN_SIG_FMT_VGA_1152X900P_66D004,
    TVIN_SIG_FMT_VGA_1152X900P_76D047,
    TVIN_SIG_FMT_VGA_1152X900P_76D149,
    TVIN_SIG_FMT_VGA_1244X842I_30D000,
    TVIN_SIG_FMT_VGA_1280X768P_59D995,
    TVIN_SIG_FMT_VGA_1280X768P_74D893,
    TVIN_SIG_FMT_VGA_1280X768P_84D837,
    TVIN_SIG_FMT_VGA_1280X960P_60D000,
    TVIN_SIG_FMT_VGA_1280X960P_75D000,
    TVIN_SIG_FMT_VGA_1280X960P_85D002,
    TVIN_SIG_FMT_VGA_1280X1024I_43D436,
    TVIN_SIG_FMT_VGA_1280X1024P_60D020,
    TVIN_SIG_FMT_VGA_1280X1024P_75D025,
    TVIN_SIG_FMT_VGA_1280X1024P_85D024,
    TVIN_SIG_FMT_VGA_1280X1024P_59D979,
    TVIN_SIG_FMT_VGA_1280X1024P_72D005,
    TVIN_SIG_FMT_VGA_1280X1024P_60D002,
    TVIN_SIG_FMT_VGA_1280X1024P_67D003,
    TVIN_SIG_FMT_VGA_1280X1024P_74D112,
    TVIN_SIG_FMT_VGA_1280X1024P_76D179,
    TVIN_SIG_FMT_VGA_1280X1024P_66D718,
    TVIN_SIG_FMT_VGA_1280X1024P_66D677,
    TVIN_SIG_FMT_VGA_1280X1024P_76D107,
    TVIN_SIG_FMT_VGA_1280X1024P_59D996,
    TVIN_SIG_FMT_VGA_1360X768P_59D799,
    TVIN_SIG_FMT_VGA_1360X1024I_51D476,
    TVIN_SIG_FMT_VGA_1440X1080P_60D000,
    TVIN_SIG_FMT_VGA_1600X1200I_48D040,
    TVIN_SIG_FMT_VGA_1600X1200P_60D000,
    TVIN_SIG_FMT_VGA_1600X1200P_65D000,
    TVIN_SIG_FMT_VGA_1600X1200P_70D000,
    TVIN_SIG_FMT_VGA_1600X1200P_75D000,
    TVIN_SIG_FMT_VGA_1600X1200P_80D000,
    TVIN_SIG_FMT_VGA_1600X1200P_85D000,
    TVIN_SIG_FMT_VGA_1600X1280P_66D931,
    TVIN_SIG_FMT_VGA_1680X1080P_60D000,
    TVIN_SIG_FMT_VGA_1792X1344P_60D000,
    TVIN_SIG_FMT_VGA_1792X1344P_74D997,
    TVIN_SIG_FMT_VGA_1856X1392P_59D995,
    TVIN_SIG_FMT_VGA_1856X1392P_75D000,
    TVIN_SIG_FMT_VGA_1868X1200P_75D000,
    TVIN_SIG_FMT_VGA_1920X1080P_60D000,
    TVIN_SIG_FMT_VGA_1920X1080P_75D000,
    TVIN_SIG_FMT_VGA_1920X1080P_85D000,
    TVIN_SIG_FMT_VGA_1920X1200P_84D932,
    TVIN_SIG_FMT_VGA_1920X1200P_75D000,
    TVIN_SIG_FMT_VGA_1920X1200P_85D000,
    TVIN_SIG_FMT_VGA_1920X1234P_75D000,
    TVIN_SIG_FMT_VGA_1920X1234P_85D000,
    TVIN_SIG_FMT_VGA_1920X1440P_60D000,
    TVIN_SIG_FMT_VGA_1920X1440P_75D000,
    TVIN_SIG_FMT_VGA_2048X1536P_60D000_A,
    TVIN_SIG_FMT_VGA_2048X1536P_75D000,
    TVIN_SIG_FMT_VGA_2048X1536P_60D000_B,
    TVIN_SIG_FMT_VGA_2048X2048P_60D008,
    TVIN_SIG_FMT_VGA_MAX,
    //Component Formats
    TVIN_SIG_FMT_COMP_480P_60D000,
    TVIN_SIG_FMT_COMP_480I_59D940,
    TVIN_SIG_FMT_COMP_576P_50D000,
    TVIN_SIG_FMT_COMP_576I_50D000,
    TVIN_SIG_FMT_COMP_720P_59D940,
    TVIN_SIG_FMT_COMP_720P_50D000,
    TVIN_SIG_FMT_COMP_1080P_23D976,
    TVIN_SIG_FMT_COMP_1080P_24D000,
    TVIN_SIG_FMT_COMP_1080P_25D000,
    TVIN_SIG_FMT_COMP_1080P_30D000,
    TVIN_SIG_FMT_COMP_1080P_50D000,
    TVIN_SIG_FMT_COMP_1080P_60D000,
    TVIN_SIG_FMT_COMP_1080I_47D952,
    TVIN_SIG_FMT_COMP_1080I_48D000,
    TVIN_SIG_FMT_COMP_1080I_50D000_A,
    TVIN_SIG_FMT_COMP_1080I_50D000_B,
    TVIN_SIG_FMT_COMP_1080I_50D000_C,
    TVIN_SIG_FMT_COMP_1080I_60D000,
    TVIN_SIG_FMT_COMP_MAX,
    //HDMI Formats
    TVIN_SIG_FMT_HDMI_640x480P_60Hz,
    TVIN_SIG_FMT_HDMI_720x480P_60Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_60Hz,
    TVIN_SIG_FMT_HDMI_1920x1080I_60Hz,
    TVIN_SIG_FMT_HDMI_1440x480I_60Hz,
    TVIN_SIG_FMT_HDMI_1440x240P_60Hz,
    TVIN_SIG_FMT_HDMI_2880x480I_60Hz,
    TVIN_SIG_FMT_HDMI_2880x240P_60Hz,
    TVIN_SIG_FMT_HDMI_1440x480P_60Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_60Hz,
    TVIN_SIG_FMT_HDMI_720x576P_50Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_50Hz,
    TVIN_SIG_FMT_HDMI_1920x1080I_50Hz_A,
    TVIN_SIG_FMT_HDMI_1440x576I_50Hz,
    TVIN_SIG_FMT_HDMI_1440x288P_50Hz,
    TVIN_SIG_FMT_HDMI_2880x576I_50Hz,
    TVIN_SIG_FMT_HDMI_2880x288P_50Hz,
    TVIN_SIG_FMT_HDMI_1440x576P_50Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_50Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_24Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_25Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_30Hz,
    TVIN_SIG_FMT_HDMI_2880x480P_60Hz,
    TVIN_SIG_FMT_HDMI_2880x576P_60Hz,
    TVIN_SIG_FMT_HDMI_1920x1080I_50Hz_B,
    TVIN_SIG_FMT_HDMI_1920x1080I_100Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_100Hz,
    TVIN_SIG_FMT_HDMI_720x576P_100Hz,
    TVIN_SIG_FMT_HDMI_1440x576I_100Hz,
    TVIN_SIG_FMT_HDMI_1920x1080I_120Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_120Hz,
    TVIN_SIG_FMT_HDMI_720x480P_120Hz,
    TVIN_SIG_FMT_HDMI_1440x480I_120Hz,
    TVIN_SIG_FMT_HDMI_720x576P_200Hz,
    TVIN_SIG_FMT_HDMI_1440x576I_200Hz,
    TVIN_SIG_FMT_HDMI_720x480P_240Hz,
    TVIN_SIG_FMT_HDMI_1440x480I_240Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_24Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_25Hz,
    TVIN_SIG_FMT_HDMI_1280x720P_30Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_120Hz,
    TVIN_SIG_FMT_HDMI_1920x1080P_100Hz,
    TVIN_SIG_FMT_HDMI_MAX,
    //Video Formats
    TVIN_SIG_FMT_CVBS_NTSC_M,
    TVIN_SIG_FMT_CVBS_NTSC_443,
    TVIN_SIG_FMT_CVBS_PAL_I,
    TVIN_SIG_FMT_CVBS_PAL_M,
    TVIN_SIG_FMT_CVBS_PAL_60,
    TVIN_SIG_FMT_CVBS_PAL_CN,
    TVIN_SIG_FMT_CVBS_SECAM,
    //656 Formats
    TVIN_SIG_FMT_BT656IN_576I,
    TVIN_SIG_FMT_BT656IN_480I,
    //601 Formats
    TVIN_SIG_FMT_BT601IN_576I,
    TVIN_SIG_FMT_BT601IN_480I,
    //Camera Formats
    TVIN_SIG_FMT_CAMERA_640X480P_30Hz,
    TVIN_SIG_FMT_CAMERA_800X600P_30Hz,
    TVIN_SIG_FMT_CAMERA_1024X768P_30Hz,
    TVIN_SIG_FMT_CAMERA_1920X1080P_30Hz,
    TVIN_SIG_FMT_MAX,
} tvin_sig_fmt_t;

//tvin signal status
typedef enum tvin_sig_status_e {
    TVIN_SIG_STATUS_NULL = 0, // processing status from init to the finding of the 1st confirmed status
    TVIN_SIG_STATUS_NOSIG,    // no signal - physically no signal
    TVIN_SIG_STATUS_UNSTABLE, // unstable - physically bad signal
    TVIN_SIG_STATUS_NOTSUP,   // not supported - physically good signal & not supported
    TVIN_SIG_STATUS_STABLE,   // stable - physically good signal & supported
} tvin_sig_status_t;

// tvin parameters
#define TVIN_PARM_FLAG_CAP 0x00000001 //tvin_parm_t.flag[ 0]: 1/enable or 0/disable frame capture function
typedef struct tvin_parm_s {
    enum tvin_port_e        port;     // must set port in IOCTL
    enum tvin_sig_fmt_e     fmt;      // signal format of format
    enum tvin_sig_status_e  status;   // signal status of decoder
    unsigned int            cap_addr; // start address of captured frame data [8 bits] in memory
                                      // for Component input, frame data [8 bits] order is Y0Cb0Y1Cr0¡­Y2nCb2nY2n+1Cr2n¡­
                                      // for VGA       input, frame data [8 bits] order is R0G0B0¡­RnGnBn¡­
    unsigned int            flag;     // flags
    unsigned int            reserved; // reserved
} tvin_parm_t;

// ***************************************************************************
// *** AFE module definition/enum/struct *************************************
// ***************************************************************************

typedef enum tvafe_cmd_status_e {
    TVAFE_CMD_STATUS_IDLE = 0,   // idle, be ready for TVIN_IOC_S_AFE_VGA_AUTO command
    TVAFE_CMD_STATUS_PROCESSING, // TVIN_IOC_S_AFE_VGA_AUTO command is in process
    TVAFE_CMD_STATUS_SUCCESSFUL, // TVIN_IOC_S_AFE_VGA_AUTO command is done with success
    TVAFE_CMD_STATUS_FAILED,     // TVIN_IOC_S_AFE_VGA_AUTO command is done with failure
} tvafe_cmd_status_t;

typedef struct tvafe_vga_edid_s {
    unsigned char value[256]; //256 byte EDID
} tvafe_vga_edid_t;

typedef struct tvafe_comp_wss_s {
    unsigned int wss1[5];
    unsigned int wss2[5];
} tvafe_comp_wss_t;

typedef struct tvafe_vga_parm_s {
    unsigned short clk_step;  // clock is 0~100 steps, it is relative step
                              // clock default (the absolute freq is set by driver upon std) is 50
                              // clock < 50, means tune down clock freq from 50 with the unit of min ADC clock step
                              // clock > 50, means tune up clock freq from 50 with the unit of min ADC clock step
    unsigned short phase;     // phase is 0~31, it is absolute value
    unsigned short hpos_step; // hpos_step is 0~100 steps, it is relative step
                              // hpos_step default (the absolute position is set by driver upon std) is 50
                              // hpos_step < 50, means shifting display to left from 50 with the unit of pixel
                              // hpos_step > 50, means shifting display right from 50 with the unit of pixel
    unsigned short vpos_step; // vpos_step is 0~100 steps, it is relative step
                              // vpos_step default (the absolute position is set by driver upon std) is 50
                              // vpos_step < 50, means shifting display to top from 50 with the unit of pixel
                              // vpos_step > 50, means shifting display bottom from 50 with the unit of pixel
    unsigned int   reserved;  // reserved
} tvafe_vga_parm_t;

typedef struct tvafe_adc_cal_s {
    // ADC A
    unsigned short a_analog_gain;     // 0x00~0xff, means 0dB~6dB
    unsigned short a_digital_offset1; // offset for fine-tuning
                                      // s11.0:   signed value, 11 integer bits,  0 fraction bits
    unsigned short a_digital_gain;    // 0~3.999
                                      // u2.10: unsigned value,  2 integer bits, 10 fraction bits
    unsigned short a_digital_offset2; // offset for format
                                      // s11.0:   signed value, 11 integer bits,  0 fraction bits
    // ADC B
    unsigned short b_analog_gain;     // ditto to ADC A
    unsigned short b_digital_offset1;
    unsigned short b_digital_gain;
    unsigned short b_digital_offset2;
    // ADC C
    unsigned short c_analog_gain;     // ditto to ADC A
    unsigned short c_digital_offset1;
    unsigned short c_digital_gain;
    unsigned short c_digital_offset2;
    // ADC D
    unsigned short d_analog_gain;     // ditto to ADC A
    unsigned short d_digital_offset1;
    unsigned short d_digital_gain;
    unsigned short d_digital_offset2;
    unsigned int   reserved;          // reserved
} tvafe_adc_cal_t;

typedef enum tvafe_cvbs_video_e {
    TVAFE_CVBS_VIDEO_UNLOCKED = 0,
    TVAFE_CVBS_VIDEO_LOCKED,
} tvafe_cvbs_video_t;

// ***************************************************************************
// *** analog tuner module definition/enum/struct ****************************
// ***************************************************************************

typedef unsigned long long tuner_std_id;

/* one bit for each */
#define TUNER_STD_PAL_B     ((tuner_std_id)0x00000001)
#define TUNER_STD_PAL_B1    ((tuner_std_id)0x00000002)
#define TUNER_STD_PAL_G     ((tuner_std_id)0x00000004)
#define TUNER_STD_PAL_H     ((tuner_std_id)0x00000008)
#define TUNER_STD_PAL_I     ((tuner_std_id)0x00000010)
#define TUNER_STD_PAL_D     ((tuner_std_id)0x00000020)
#define TUNER_STD_PAL_D1    ((tuner_std_id)0x00000040)
#define TUNER_STD_PAL_K     ((tuner_std_id)0x00000080)
#define TUNER_STD_PAL_M     ((tuner_std_id)0x00000100)
#define TUNER_STD_PAL_N     ((tuner_std_id)0x00000200)
#define TUNER_STD_PAL_Nc    ((tuner_std_id)0x00000400)
#define TUNER_STD_PAL_60    ((tuner_std_id)0x00000800)
#define TUNER_STD_NTSC_M    ((tuner_std_id)0x00001000)
#define TUNER_STD_NTSC_M_JP ((tuner_std_id)0x00002000)
#define TUNER_STD_NTSC_443  ((tuner_std_id)0x00004000)
#define TUNER_STD_NTSC_M_KR ((tuner_std_id)0x00008000)
#define TUNER_STD_SECAM_B   ((tuner_std_id)0x00010000)
#define TUNER_STD_SECAM_D   ((tuner_std_id)0x00020000)
#define TUNER_STD_SECAM_G   ((tuner_std_id)0x00040000)
#define TUNER_STD_SECAM_H   ((tuner_std_id)0x00080000)
#define TUNER_STD_SECAM_K   ((tuner_std_id)0x00100000)
#define TUNER_STD_SECAM_K1  ((tuner_std_id)0x00200000)
#define TUNER_STD_SECAM_L   ((tuner_std_id)0x00400000)
#define TUNER_STD_SECAM_LC  ((tuner_std_id)0x00800000)

/* some merged standards */
#define TUNER_STD_MN       (TUNER_STD_PAL_M    |\
                            TUNER_STD_PAL_N    |\
                            TUNER_STD_PAL_Nc   |\
                            TUNER_STD_NTSC)
#define TUNER_STD_B        (TUNER_STD_PAL_B    |\
                            TUNER_STD_PAL_B1   |\
                            TUNER_STD_SECAM_B)
#define TUNER_STD_GH       (TUNER_STD_PAL_G    |\
                            TUNER_STD_PAL_H    |\
                            TUNER_STD_SECAM_G  |\
                            TUNER_STD_SECAM_H)
#define TUNER_STD_DK       (TUNER_STD_PAL_DK   |\
                            TUNER_STD_SECAM_DK)

/* some common needed stuff */
#define TUNER_STD_PAL_BG   (TUNER_STD_PAL_B     |\
                            TUNER_STD_PAL_B1    |\
                            TUNER_STD_PAL_G)
#define TUNER_STD_PAL_DK   (TUNER_STD_PAL_D     |\
                            TUNER_STD_PAL_D1    |\
                            TUNER_STD_PAL_K)
#define TUNER_STD_PAL      (TUNER_STD_PAL_BG    |\
                            TUNER_STD_PAL_DK    |\
                            TUNER_STD_PAL_H     |\
                            TUNER_STD_PAL_I)
#define TUNER_STD_NTSC     (TUNER_STD_NTSC_M    |\
                            TUNER_STD_NTSC_M_JP |\
                            TUNER_STD_NTSC_M_KR)
#define TUNER_STD_SECAM_DK (TUNER_STD_SECAM_D   |\
                            TUNER_STD_SECAM_K   |\
                            TUNER_STD_SECAM_K1)
#define TUNER_STD_SECAM    (TUNER_STD_SECAM_B   |\
                            TUNER_STD_SECAM_G   |\
                            TUNER_STD_SECAM_H   |\
                            TUNER_STD_SECAM_DK  |\
                            TUNER_STD_SECAM_L   |\
                            TUNER_STD_SECAM_LC)
#define TUNER_STD_525_60   (TUNER_STD_PAL_M     |\
                            TUNER_STD_PAL_60    |\
                            TUNER_STD_NTSC      |\
                            TUNER_STD_NTSC_443)
#define TUNER_STD_625_50   (TUNER_STD_PAL       |\
                            TUNER_STD_PAL_N     |\
                            TUNER_STD_PAL_Nc    |\
                            TUNER_STD_SECAM)
#define TUNER_STD_UNKNOWN   0
#define TUNER_STD_ALL      (TUNER_STD_525_60    |\
                            TUNER_STD_625_50)

typedef enum tuner_signal_status_e {
    TUNER_SIGNAL_STATUS_WEAK = 0, // RF PLL unlocked
    TUNER_SIGNAL_STATUS_STRONG,   // RF PLL   locked
}tuner_signal_status_t;

typedef struct tuner_parm_s {
    unsigned int               rangelow;  // tuner frequency is in the unit of Hz
    unsigned int               rangehigh; // tuner frequency is in the unit of Hz
    enum tuner_signal_status_e signal;
             int               afc;       // Fc-Fo in the unit of Hz
                                          // Fc is the current freq
                                          // Fo is the target freq
    unsigned int               reserved;  // reserved
} tuner_parm_t;

typedef struct tuner_freq_s {
    unsigned int freq;     // tuner frequency is in the unit of Hz
    unsigned int reserved; // reserved
} tuner_freq_s;

// ***************************************************************************
// *** IOCTL command definition **********************************************
// ***************************************************************************

#define TVIN_IOC_MAGIC 'T'

//GENERAL
#define TVIN_IOC_START_DEC          _IOW(TVIN_IOC_MAGIC, 0x01, struct tvin_parm_s)
#define TVIN_IOC_STOP_DEC           _IO( TVIN_IOC_MAGIC, 0x02)
#define TVIN_IOC_G_PARM             _IOR(TVIN_IOC_MAGIC, 0x03, struct tvin_parm_s)
#define TVIN_IOC_S_PARM             _IOW(TVIN_IOC_MAGIC, 0x04, struct tvin_parm_s)

//TVAFE
#define TVIN_IOC_S_AFE_ADC_CAL      _IOW(TVIN_IOC_MAGIC, 0x11, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_ADC_CAL      _IOR(TVIN_IOC_MAGIC, 0x12, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_COMP_WSS     _IOR(TVIN_IOC_MAGIC, 0x13, struct tvafe_comp_wss_s)
#define TVIN_IOC_S_AFE_VGA_EDID     _IOW(TVIN_IOC_MAGIC, 0x14, struct tvafe_vga_edid_s)
#define TVIN_IOC_G_AFE_VGA_EDID     _IOR(TVIN_IOC_MAGIC, 0x15, struct tvafe_vga_edid_s)
#define TVIN_IOC_S_AFE_VGA_PARM     _IOW(TVIN_IOC_MAGIC, 0x16, struct tvafe_vga_parm_s)
#define TVIN_IOC_G_AFE_VGA_PARM     _IOR(TVIN_IOC_MAGIC, 0x17, struct tvafe_vga_parm_s)
#define TVIN_IOC_S_AFE_VGA_AUTO     _IO( TVIN_IOC_MAGIC, 0x18)
#define TVIN_IOC_G_AFE_CMD_STATUS   _IOR(TVIN_IOC_MAGIC, 0x19, enum tvafe_cmd_status_e)
#define TVIN_IOC_G_AFE_CVBS_LOCK    _IOR(TVIN_IOC_MAGIC, 0x1a, enum tvafe_cvbs_video_e)

//TUNER
#define TVIN_IOC_G_TUNER_STD        _IOR(TVIN_IOC_MAGIC, 0x21, tuner_std_id)
#define TVIN_IOC_S_TUNER_STD        _IOW(TVIN_IOC_MAGIC, 0x22, tuner_std_id)
#define TVIN_IOC_G_TUNER_FREQ       _IOR(TVIN_IOC_MAGIC, 0x23, struct tuner_freq_s)
#define TVIN_IOC_S_TUNER_FREQ       _IOW(TVIN_IOC_MAGIC, 0x24, struct tuner_freq_s)
#define TVIN_IOC_G_TUNER_PARM       _IOR(TVIN_IOC_MAGIC, 0x25, struct tuner_parm_s)

#endif // __TVIN_VDIN_H


