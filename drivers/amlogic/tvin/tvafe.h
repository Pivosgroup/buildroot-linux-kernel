/*******************************************************************
*  Copyright C 2010 by Amlogic, Inc. All Rights Reserved.
*  File name: tvafe.h
*  Description: IO function, structure, enum, used in TVIN AFE sub-module processing
*******************************************************************/

#ifndef _TVAFE_H
#define _TVAFE_H


#include "tvin_format_table.h"
#include "tvin_global.h"

/******************************Definitions************************************/
//tuner type definition
#define TUNER_PHILIPS_FQ1216MK3
//#define TUNER_PHILIPS_FM1236_MK3

#define ABS(x) ( (x)<0 ? -(x) : (x))

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************

typedef enum tvafe_adc_pin_e {
    TVAFE_ADC_PIN_NULL = 0,
    TVAFE_ADC_PIN_A_PGA_0,
    TVAFE_ADC_PIN_A_PGA_1,
    TVAFE_ADC_PIN_A_PGA_2,
    TVAFE_ADC_PIN_A_PGA_3,
    TVAFE_ADC_PIN_A_PGA_4,
    TVAFE_ADC_PIN_A_PGA_5,
    TVAFE_ADC_PIN_A_PGA_6,
    TVAFE_ADC_PIN_A_PGA_7,
    TVAFE_ADC_PIN_A_0,
    TVAFE_ADC_PIN_A_1,
    TVAFE_ADC_PIN_A_2,
    TVAFE_ADC_PIN_A_3,
    TVAFE_ADC_PIN_A_4,
    TVAFE_ADC_PIN_A_5,
    TVAFE_ADC_PIN_A_6,
    TVAFE_ADC_PIN_A_7,
    TVAFE_ADC_PIN_B_0,
    TVAFE_ADC_PIN_B_1,
    TVAFE_ADC_PIN_B_2,
    TVAFE_ADC_PIN_B_3,
    TVAFE_ADC_PIN_B_4,
    TVAFE_ADC_PIN_B_5,
    TVAFE_ADC_PIN_B_6,
    TVAFE_ADC_PIN_B_7,
    TVAFE_ADC_PIN_C_0,
    TVAFE_ADC_PIN_C_1,
    TVAFE_ADC_PIN_C_2,
    TVAFE_ADC_PIN_C_3,
    TVAFE_ADC_PIN_C_4,
    TVAFE_ADC_PIN_C_5,
    TVAFE_ADC_PIN_C_6,
    TVAFE_ADC_PIN_C_7,
    TVAFE_ADC_PIN_D_0,
    TVAFE_ADC_PIN_D_1,
    TVAFE_ADC_PIN_D_2,
    TVAFE_ADC_PIN_D_3,
    TVAFE_ADC_PIN_D_4,
    TVAFE_ADC_PIN_D_5,
    TVAFE_ADC_PIN_D_6,
    TVAFE_ADC_PIN_D_7,
    TVAFE_ADC_PIN_MAX,
} tvafe_adc_pin_t;

typedef enum tvafe_src_sig_e {
    CVBS0_Y = 0,
    CVBS1_Y,
    CVBS2_Y,
    CVBS3_Y,
    CVBS4_Y,
    CVBS5_Y,
    CVBS6_Y,
    CVBS7_Y,
    S_VIDEO0_Y,
    S_VIDEO0_C,
    S_VIDEO1_Y,
    S_VIDEO1_C,
    S_VIDEO2_Y,
    S_VIDEO2_C,
    S_VIDEO3_Y,
    S_VIDEO3_C,
    S_VIDEO4_Y,
    S_VIDEO4_C,
    S_VIDEO5_Y,
    S_VIDEO5_C,
    S_VIDEO6_Y,
    S_VIDEO6_C,
    S_VIDEO7_Y,
    S_VIDEO7_C,
    VGA0_G,
    VGA0_B,
    VGA0_R,
    VGA1_G,
    VGA1_B,
    VGA1_R,
    VGA2_G,
    VGA2_B,
    VGA2_R,
    VGA3_G,
    VGA3_B,
    VGA3_R,
    VGA4_G,
    VGA4_B,
    VGA4_R,
    VGA5_G,
    VGA5_B,
    VGA5_R,
    VGA6_G,
    VGA6_B,
    VGA6_R,
    VGA7_G,
    VGA7_B,
    VGA7_R,
    COMPONENT0_Y,
    COMPONENT0_PB,
    COMPONENT0_PR,
    COMPONENT1_Y,
    COMPONENT1_PB,
    COMPONENT1_PR,
    COMPONENT2_Y,
    COMPONENT2_PB,
    COMPONENT2_PR,
    COMPONENT3_Y,
    COMPONENT3_PB,
    COMPONENT3_PR,
    COMPONENT4_Y,
    COMPONENT4_PB,
    COMPONENT4_PR,
    COMPONENT5_Y,
    COMPONENT5_PB,
    COMPONENT5_PR,
    COMPONENT6_Y,
    COMPONENT6_PB,
    COMPONENT6_PR,
    COMPONENT7_Y,
    COMPONENT7_PB,
    COMPONENT7_PR,
    SCART0_G,
    SCART0_B,
    SCART0_R,
    SCART0_CVBS,
    SCART1_G,
    SCART1_B,
    SCART1_R,
    SCART1_CVBS,
    SCART2_G,
    SCART2_B,
    SCART2_R,
    SCART2_CVBS,
    SCART3_G,
    SCART3_B,
    SCART3_R,
    SCART3_CVBS,
    SCART4_G,
    SCART4_B,
    SCART4_R,
    SCART4_CVBS,
    SCART5_G,
    SCART5_B,
    SCART5_R,
    SCART5_CVBS,
    SCART6_G,
    SCART6_B,
    SCART6_R,
    SCART6_CVBS,
    SCART7_G,
    SCART7_B,
    SCART7_R,
    SCART7_CVBS,
    TVAFE_SRC_SIG_MAX_NUM,
} tvafe_src_sig_t;

typedef enum tvafe_src_type_e {
    TVAFE_SRC_TYPE_NULL = 0,
    TVAFE_SRC_TYPE_CVBS,
	TVAFE_SRC_TYPE_SVIDEO,
    TVAFE_SRC_TYPE_VGA,
    TVAFE_SRC_TYPE_COMPONENT,
    TVAFE_SRC_TYPE_SCART,
} tvafe_src_type_t;

typedef enum tvin_sig_event_e {
    TVIN_EVENT_NOSIG = 0,  // Confirmed status of "no signal     - physically no   signal                "
    TVIN_EVENT_UNSTABLE,   // Confirmed status of "unstable      - physically bad  signal                "
    TVIN_EVENT_STABLE,     // Confirmed status of "stable        - physically good signal & supported    "
} tvin_sig_event_t;

typedef enum tvafe_adc_ch_e {
    TVAFE_ADC_CH_NULL = 0,
    TVAFE_ADC_CH_PGA,
    TVAFE_ADC_CH_A,
    TVAFE_ADC_CH_B,
    TVAFE_ADC_CH_C,
} tvafe_adc_ch_t;

// ***************************************************************************
// *** structure definitions *********************************************
// ***************************************************************************

typedef struct tvafe_pin_mux_s {
	enum tvafe_adc_pin_e pin[TVAFE_SRC_SIG_MAX_NUM];
} tvafe_pin_mux_t;

typedef struct tvafe_vga_edid_s {
    unsigned char   value[256];  //256 byte EDID
} tvafe_vga_edid_t;

typedef struct tvafe_comp_wss_s {
    unsigned int    wss1[5];
    unsigned int    wss2[5];
} tvafe_comp_wss_t;

typedef struct tvafe_adc_cal_s {
	//enum tvafe_adc_cal_result_validity_e valid_flag;
    //ADC A
    unsigned short                       a_analog_gain;
    signed short                         a_digital_offset1;
    unsigned short                       a_digital_gain;
    signed short                         a_digital_offset2;
    //ADC B
    unsigned short                       b_analog_gain;
    signed short                         b_digital_offset1;
    unsigned short                       b_digital_gain;
    signed short                         b_digital_offset2;
    //ADC C
    unsigned short                       c_analog_gain;
    signed short                         c_digital_offset1;
    unsigned short                       c_digital_gain;
    signed short                         c_digital_offset2;
} tvafe_adc_cal_t;

typedef struct tvafe_adc_param_s {
    unsigned int                            clk;
    unsigned int                            phase;
    unsigned int                            hpos;
    unsigned int                            vpos;
} tvafe_adc_param_t;

//window setting  for auto phase and border detection
typedef struct tvafe_vga_sig_win_s {
    unsigned int hstart;             //window left boarder
    unsigned int hend;               //window right boarder
    unsigned int vstart;             //window top boarder
    unsigned int vend;               //window bottom boarder
} tvafe_vga_sig_win_t;

typedef struct tvafe_vga_boarder_cfg_s {
    unsigned int                th_pixel;
    unsigned int                th_pixel_sel;
    unsigned int                th_line;
    unsigned int                th_line_sel;
    struct tvafe_vga_sig_win_s  bd_win;
    unsigned int                bd_en;
} tvafe_vga_boarder_cfg_t;

typedef struct tvafe_vga_ap_cfg_s {
    unsigned int                ap_specific_sel;
    unsigned int                ap_diff_sel;
    struct tvafe_vga_sig_win_s  ap_win;
    unsigned int                ap_th_coring;
    unsigned int                ap_specific_max_hpos;
    unsigned int                ap_specific_max_vpos;
    unsigned int                ap_specific_min_hpos;
    unsigned int                ap_specific_min_vpos;
    unsigned int                ap_en;
} tvafe_vga_ap_cfg_t;

//window setting  for auto phase and border detection
typedef struct tvafe_vga_boarder_info_s {
    unsigned int r_hstart;             //window left boarder
    unsigned int r_hend;               //window right boarder
    unsigned int r_vstart;             //window top boarder
    unsigned int r_vend;               //window bottom boarder

    unsigned int g_hstart;             //window left boarder
    unsigned int g_hend;               //window right boarder
    unsigned int g_vstart;             //window top boarder
    unsigned int g_vend;               //window bottom boarder

    unsigned int b_hstart;             //window left boarder
    unsigned int b_hend;               //window right boarder
    unsigned int b_vstart;             //window top boarder
    unsigned int b_vend;               //window bottom boarder

} tvafe_vga_boarder_info_t;

//sum value for coarse phase
typedef struct tvafe_vga_max_min_pixel_info_s {
	unsigned int r_max;
    unsigned int r_max_hcnt;
    unsigned int r_max_vcnt;
	unsigned int r_min;
    unsigned int r_min_hcnt;
    unsigned int r_min_vcnt;

	unsigned int g_max;
    unsigned int g_max_hcnt;
    unsigned int g_max_vcnt;
	unsigned int g_min;
    unsigned int g_min_hcnt;
    unsigned int g_min_vcnt;

	unsigned int b_max;
    unsigned int b_max_hcnt;
    unsigned int b_max_vcnt;
	unsigned int b_min;
    unsigned int b_min_hcnt;
    unsigned int b_min_vcnt;
} tvafe_vga_max_min_pixel_info_t;

typedef struct tvafe_vga_diff_sum_info_s {
	unsigned int r_sum;
	unsigned int g_sum;
	unsigned int b_sum;
} tvafe_vga_diff_sum_info_t;

typedef struct tvafe_vga_ap_info_s {
    struct tvafe_vga_diff_sum_info_s      diff_sum;
    struct tvafe_vga_max_min_pixel_info_s val_point;
} tvafe_vga_ap_info_t;

typedef struct tvafe_info_s {
    struct tvafe_pin_mux_s                  *pinmux;

    //signal parameters
    struct tvin_para_s                      param;
    enum tvafe_src_type_e                   src_type;

    //VGA settings
    struct tvafe_adc_param_s                adc_param;

    //adc calibration data
    struct tvafe_adc_cal_s                  adc_cal_val;

    //WSS data
    struct tvafe_comp_wss_s                 comp_wss;

    //for canvas
    unsigned char                           rd_canvas_index;
    unsigned char                           wr_canvas_index;
    unsigned char                           buff_flag[TVAFE_VF_POOL_SIZE];
    unsigned                                pbufAddr;
    unsigned                                decbuf_size;
    unsigned                                canvas_total_count : 4;

    unsigned                                sig_status_cnt          : 16;
    unsigned                                cvbs_dec_state          : 1;    //1: enable the decode; 0: disable the decode
    unsigned                                s_video_dec_state       : 1;    //1: enable the decode; 0: disable the decode
    unsigned                                vga_dec_state           : 1;    //1: enable the decode; 0: disable the decode
    unsigned                                comp_dec_state          : 1;    //1: enable the decode; 0: disable the decode

    unsigned                                video_in_changing_flag  : 1;
} tvafe_info_t;

// ***************************************************************************
// *** IOCTL command definitions **********
// ***************************************************************************

#define TVAFE_IOC_MAGIC 'O'

#define TVAFEIOC_G_COMP_WSS           _IOR(TVAFE_IOC_MAGIC, 0x01, struct tvafe_comp_wss_s)
#define TVAFEIOC_S_VGA_EDID           _IOW(TVAFE_IOC_MAGIC, 0x02, struct tvafe_vga_edid_s)
#define TVAFEIOC_G_VGA_EDID           _IOR(TVAFE_IOC_MAGIC, 0x03, struct tvafe_vga_edid_s)
#define TVAFEIOC_S_ADC_PARAM          _IOW(TVAFE_IOC_MAGIC, 0x04, struct tvafe_adc_param_s)
#define TVAFEIOC_G_ADC_PARAM          _IOR(TVAFE_IOC_MAGIC, 0x05, struct tvafe_adc_param_s)
#define TVAFEIOC_S_ADC_CAL_VAL        _IOR(TVAFE_IOC_MAGIC, 0x06, struct tvafe_adc_cal_s)
#define TVAFEIOC_G_ADC_CAL_VAL        _IOR(TVAFE_IOC_MAGIC, 0x07, struct tvafe_adc_cal_s)
//for auto adjust
#define TVAFEIOC_S_VGA_BD_PARAM       _IOW(TVAFE_IOC_MAGIC, 0x08, struct tvafe_vga_boarder_cfg_s)
#define TVAFEIOC_G_VGA_BD_PARAM       _IOR(TVAFE_IOC_MAGIC, 0x09, struct tvafe_adc_cal_s)
#define TVAFEIOC_S_VGA_AP_PARAM       _IOW(TVAFE_IOC_MAGIC, 0x0A, struct tvafe_vga_ap_cfg_s)
#define TVAFEIOC_G_VGA_AP_PARAM       _IOR(TVAFE_IOC_MAGIC, 0x0B, struct tvafe_vga_ap_info_s)
#endif  // _TVAFE_H
