/*
 * TVAFE cvd2 device driver.
 *
 * Copyright (c) 2010 Frank zhao <frank.zhao@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the smems of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/******************************Includes************************************/
#include <linux/kernel.h>

#include <mach/am_regs.h>

#include "tvafe.h"
#include "tvafe_general.h"
#include "tvafe_regs.h"
#include "tvafe_cvd.h"


/******************************Definitions************************************/
//digital gain value for AGC
#define  DAGC_GAIN_STANDARD                 0xC8
#define  DAGC_GAIN_RANGE                    0x64
#define  DAGC_GAIN_RANGE1                   0xC8
#define  DAGC_GAIN_RANGE2                   0xFF


#define	FC_LESS_NTSC443_TO_NTSCM_MAX        128
#define	FC_LESS_NTSC443_TO_NTSCM_MIN        96

#define	FC_MORE_NTSCM_TO_NTSC443_MAX        200
#define	FC_MORE_NTSCM_TO_NTSC443_MIN        130

#define	FC_LESS_PAL_I_TO_PAL_N_MAX          108
#define	FC_LESS_PAL_I_TO_PAL_N_MIN          52

#define	FC_LESS_PAL_60_TO_PAL_M_MAX	        100
#define	FC_LESS_PAL_60_TO_PAL_M_MIN	        35

#define	FC_MORE_PAL_N_TO_PAL_I_MAX	        205
#define	FC_MORE_PAL_N_TO_PAL_I_MIN	        145

#define	FC_MORE_PAL_M_TO_PAL_60_MAX		    205
#define	FC_MORE_PAL_M_TO_PAL_60_MIN		    150

#define CORDIC_FILTER_COUNT                 10
#define PAL_I_TO_SECAM_CNT                  205
#define SECAM_STABLE_CNT                    211


/***************************Local **********************************/
//cvd2 sub mode switch register table
static enum tvin_sig_event_e  event_cvd = 0;
/***************************Loacal Variables**********************************/
static unsigned int cnt_cvd = 0;
/***************************Global Variables**********************************/
static struct tvafe_cvd2_sig_status_s    cvd2_sig_status = {
    0,//unsigned char  no_sig                :1;
    0,//unsigned char  h_lock                :1;
    0,//unsigned char  v_lock                :1;
    0,//unsigned char  h_nonstd              :1;
    0,//unsigned char  v_nonstd              :1;
    0,//unsigned char  no_color_burst        :1;
    0,//unsigned char  comb3d_off            :1;

    0,//unsigned char  hv_lock               :1;
    0,//unsigned char  chroma_lock           :1;
    0,//unsigned char  pal                   :1;
    0,//unsigned char  secam                 :1;
    0,//unsigned char  line625               :1;
    0,//unsigned char  fc_more               :1;
    0,//unsigned char  fc_Less               :1;
    0,//unsigned char  noisy                 :1;
    0,//unsigned char  vcr                   :1;
    0,//unsigned char  vcrtrick              :1;
    0,//unsigned char  vcrff                 :1;
    0,//unsigned char  vcrrew                :1;
    0,//unsigned char  cordic_data_sum;
    0,//unsigned char cordic_data_min;
    0,//unsigned char cordic_data_max;
    0,//unsigned char stable_cnt;
    0,//unsigned char pali_to_secam_cnt;
    0,//enum tvafe_cvd2_sd_state_e          cur_sd_state;
    0,//enum tvafe_cvd2_sd_state_e          detected_sd_state;

    {0,0,0},//struct tvafe_cvd2_agc_s             agc;
};

/***************************Loacal Variables**********************************/

//--------------------------------------------------------------------------------
// Function:      ReadRegister_CVD2
//	Read register
// Parameters:
//	UINT32 addr - register address
//	int bitWidth - register's width (related bits)
//  int offset - start bit of the register (for example, register of one bit in address
//					0x01.2 has bitwidth 1 and offset of 2.
//	UINT32* val - pointer to buffer
// Returns:
//	BOOL - True if success, otherwise fail.
// Description:
// NOTE: This function should be implemented (the definition may be different)!
//--------------------------------------------------------------------------------
void ReadRegister_CVD2(unsigned addr, int bit_width, int offset, unsigned *val)
{
	// ADD YOUR CODE HERE
	unsigned read_data;

	read_data = READ_CBUS_REG(addr);   //*(volatile unsigned long *)addr ;
	read_data &= bit_width;
	(* val) = read_data >> offset;

    return;
}

//--------------------------------------------------------------------------------
// Function:      WriteRegister_CVD2
//	Read register
// Parameters:
//	UINT32 addr - register address
//	int bitWidth - register's width (related bits)
//  int offset - start bit of the register (for example, register of one bit in address
//					0x01.2 has bitwidth 1 and offset of 2.
//	UINT32 val - value
// Returns:
//	BOOL - True if success, otherwise fail.
// Description:
// NOTE: This function should be implemented (the definition may be different)!
//--------------------------------------------------------------------------------
static void WriteRegister_CVD2(unsigned addr, int bit_width, int offset, unsigned val)
{
	// ADD YOUR CODE HERE
	unsigned read_data;

	read_data = READ_CBUS_REG(addr);   //*(volatile unsigned long *)addr ;
	read_data &= ~(bit_width);
	read_data |=  (val) << offset;
	WRITE_CBUS_REG(addr, read_data);  //*(volatile unsigned long *)addr = read_data;

    return;
}


//static void getCDTOstatus(unsigned data)
//{
//	unsigned tmp_data;
//
//	ReadRegister_CVD2(CVD2_CHROMA_DTO_INCREMENT_31_24, 0xff, 0, &tmp_data);
//	data = (tmp_data & 0xff) << 24;
//	ReadRegister_CVD2(CVD2_CHROMA_DTO_INCREMENT_23_16, 0xff, 0, &tmp_data);
//	data |= (tmp_data & 0xff)  << 16;
//	ReadRegister_CVD2(CVD2_CHROMA_DTO_INCREMENT_15_8, 0xff, 0, &tmp_data);
//	data |= (tmp_data & 0xff)  << 8;
//	ReadRegister_CVD2(CVD2_CHROMA_DTO_INCREMENT_7_0, 0xff, 0, &tmp_data);
//	data |= (tmp_data & 0xff) ;
//
//    return;
//}

static void programCDTO(unsigned data)
{
	unsigned tmp_data;

	tmp_data = (data & 0xff000000) >> 24;
	WRITE_CBUS_REG(CVD2_CHROMA_DTO_INCREMENT_31_24,	tmp_data);
	tmp_data = (data & 0xff0000) >> 16;
    WRITE_CBUS_REG(CVD2_CHROMA_DTO_INCREMENT_23_16,	tmp_data);
	tmp_data = (data & 0xff00) >> 8;
    WRITE_CBUS_REG(CVD2_CHROMA_DTO_INCREMENT_15_8,	tmp_data);
	tmp_data = data & 0xff;
    WRITE_CBUS_REG(CVD2_CHROMA_DTO_INCREMENT_7_0,	tmp_data);

    return;
}

static void tvafe_cvd2_reset_cnt(void)
{
    cvd2_sig_status.cordic_data_sum = 0;
    cvd2_sig_status.stable_cnt = 0;
    cvd2_sig_status.pali_to_secam_cnt = 0;
    cvd2_sig_status.cordic_data_min = 0xff;
    cvd2_sig_status.cordic_data_max = 0;

    cvd2_sig_status.agc.cnt = 0;
    cvd2_sig_status.agc.dgain = 0;
    cvd2_sig_status.agc.again = 0;
    return;
}

const static  int cvbs_acd_reg_default[][2] = {
    {ACD_REG_00                             ,0x22333309,}, // ACD_REG_00
    {ACD_REG_01                             ,0x00021111,}, // ACD_REG_01
    {ACD_REG_02                             ,0x8000c110,}, // ACD_REG_02
    {ACD_REG_03                             ,0x08881e28,}, // ACD_REG_03
    {ACD_REG_04                             ,0x34962858,}, // ACD_REG_04
    {ACD_REG_05                             ,0x00008e19,}, // ACD_REG_05
    {ACD_REG_06                             ,0x00000000,}, // ACD_REG_06
    {ACD_REG_07                             ,0x99664433,}, // ACD_REG_07
    {ACD_REG_08                             ,0x20e000fe,}, // ACD_REG_08
    {ACD_REG_09                             ,0x00000101,}, // ACD_REG_09
    {ACD_REG_0A                             ,0x00000000,}, // ACD_REG_0A
    {ACD_REG_0B                             ,0x80030518,}, // ACD_REG_0B
    {ACD_REG_0C                             ,0x00000000,}, // ACD_REG_0C
    {ACD_REG_0D                             ,0x00000000,}, // ACD_REG_0D
    {ACD_REG_0E                             ,0x00000000,}, // ACD_REG_0E
    {ACD_REG_0F                             ,0x00000000,}, // ACD_REG_0F
    {ACD_REG_10                             ,0x0804603f,}, // ACD_REG_10
    {ACD_REG_11                             ,0x44404020,}, // ACD_REG_11
    {ACD_REG_12                             ,0x18100044,}, // ACD_REG_12
    {ACD_REG_13                             ,0x00282820,}, // ACD_REG_13
    {ACD_REG_14                             ,0x1006844b,}, // ACD_REG_14
    {ACD_REG_15                             ,0x44404018,}, // ACD_REG_15
    {ACD_REG_16                             ,0x44440044,}, // ACD_REG_16
    {ACD_REG_17                             ,0x00008c80,}, // ACD_REG_17
    {ACD_REG_18                             ,0x18008888,}, // ACD_REG_18
    {ACD_REG_19                             ,0x30100c04,}, // ACD_REG_19
    {ACD_REG_1A                             ,0x40ff0486,}, // ACD_REG_1A
    {ACD_REG_1B                             ,0x04a7208c,}, // ACD_REG_1B
    {ACD_REG_1C                             ,0x48484848,}, // ACD_REG_1C
    {ACD_REG_1D                             ,0x00004444,}, // ACD_REG_1D
    {ACD_REG_1F                             ,0x00000000,}, // ACD_REG_1F
    {ACD_REG_20                             ,0x00000000,}, // ACD_REG_20
    {ACD_REG_21                             ,0x10000000,}, // ACD_REG_21 //need check in M2
    {ACD_REG_22                             ,0x00000000,}, // ACD_REG_22
    {ACD_REG_23                             ,0x00000000,}, // ACD_REG_23
    {ACD_REG_24                             ,0x00000000,}, // ACD_REG_24
    {ACD_REG_25                             ,0x00000000,}, // ACD_REG_25
    {ACD_REG_26                             ,0x00000102,}, // ACD_REG_26
    {ACD_REG_27                             ,0x00000000,}, // ACD_REG_27
    {ACD_REG_28                             ,0x00000008,}, // ACD_REG_28
    {ACD_REG_29                             ,0x20800000,}, // ACD_REG_29
    {ACD_REG_2A                             ,0x00075a60,}, // ACD_REG_2A
    {ACD_REG_2B                             ,0x00000000,}, // ACD_REG_2B
    {ACD_REG_2C                             ,0x00000000,}, // ACD_REG_2C
    {ACD_REG_2D                             ,0x00000000,}, // ACD_REG_2D
    {ACD_REG_2E                             ,0x00000000,}, // ACD_REG_2E
    {ACD_REG_2F                             ,0x00286000,}, // ACD_REG_2F //need check in M2
    {ACD_REG_30                             ,0x00270000,}, // ACD_REG_30 //need check in M2
    {ACD_REG_32                             ,0x00000000,}, // ACD_REG_32
    {0xFFFFFFFF                             ,0x00000000,},
};
	    /* ADC */
const static  int cvbs_adc_reg_default[][2] = {
    {ADC_REG_00                             ,0x00000000,}, // ADC_REG_00
    {ADC_REG_01 	                        ,0x0000006b,}, // ADC_REG_01
    {ADC_REG_02 	                        ,0x000000f0,}, // ADC_REG_02
    {ADC_REG_03 	                        ,0x00000008,}, // ADC_REG_03
    {ADC_REG_04 	                        ,0x00000014,}, // ADC_REG_04
    {ADC_REG_05 	                        ,0x00000020,}, // ADC_REG_05
    {ADC_REG_06 	                        ,0x00000010,}, // ADC_REG_06
    {ADC_REG_07 	                        ,0x00000000,}, // ADC_REG_07
    {ADC_REG_08 	                        ,0x00000000,}, // ADC_REG_08
    {ADC_REG_09 	                        ,0x00000000,}, // ADC_REG_09
    {ADC_REG_0B 	                        ,0x0000000e,}, // ADC_REG_0B
    {ADC_REG_0C 	                        ,0x00000010,}, // ADC_REG_0C
    {ADC_REG_0D 	                        ,0x00000010,}, // ADC_REG_0D
    //{ADC_REG_0E,	                        ,0x00000000,}, // ADC_REG_0E
    {ADC_REG_0F 	                        ,0x00000000,}, // ADC_REG_0F
    {ADC_REG_10 	                        ,0x00000000,}, // ADC_REG_10
    {ADC_REG_11 	                        ,0x00000000,}, // ADC_REG_11
    //{ADC_REG_12,	                        ,0x00000040,}, // ADC_REG_12
    {ADC_REG_13 	                        ,0x00000023,}, // ADC_REG_13
    {ADC_REG_14 	                        ,0x00000023,}, // ADC_REG_14
    {ADC_REG_15 	                        ,0x000000a3,}, // ADC_REG_15
    //{ADC_REG_16,	                        ,0x00000022,}, // ADC_REG_16
    {ADC_REG_17 	                        ,0x00000000,}, // ADC_REG_17
    {ADC_REG_18 	                        ,0x00000000,}, // ADC_REG_18
    {ADC_REG_19 	                        ,0x00000030,}, // ADC_REG_19
    {ADC_REG_1A 	                        ,0x00000030,}, // ADC_REG_1A
    {ADC_REG_1B 	                        ,0x00000000,}, // ADC_REG_1B
    //{ADC_REG_1C,	                        ,0x00000000,}, // ADC_REG_1C
    //{ADC_REG_1D,	                        ,0x00000000,}, // ADC_REG_1D
    {ADC_REG_1E 	                        ,0x00000069,}, // ADC_REG_1E
    {ADC_REG_1F 	                        ,0x00000088,}, // ADC_REG_1F
    {ADC_REG_20 	                        ,0x00000030,}, // ADC_REG_20
    {ADC_REG_21 	                        ,0x00000007,}, // ADC_REG_21
    //{ADC_REG_22,	                        ,0x000000c0,}, // ADC_REG_22
    //{ADC_REG_23,	                        ,0x00000000,}, // ADC_REG_23
    {ADC_REG_24 	                        ,0x00000000,}, // ADC_REG_24
    //{ADC_REG_25,	                        ,0x00000000,}, // ADC_REG_25
    {ADC_REG_26 	                        ,0x0000003f,}, // ADC_REG_26
    {ADC_REG_27 	                        ,0x0000003f,}, // ADC_REG_27
    {ADC_REG_28 	                        ,0x00000000,}, // ADC_REG_28
    //{ADC_REG_29,	                        ,0x00000000,}, // ADC_REG_29
    {ADC_REG_2A 	                        ,0x00000000,}, // ADC_REG_2A
    {ADC_REG_2B 	                        ,0x00000000,}, // ADC_REG_2B
    //{ADC_REG_2C,	                        ,0x000000ff,}, // ADC_REG_2C
    //{ADC_REG_2D,	                        ,0x00000000,}, // ADC_REG_2D
    {ADC_REG_2E 	                        ,0x00000040,}, // ADC_REG_2E
    {ADC_REG_2F 	                        ,0x00000028,}, // ADC_REG_2F
    {ADC_REG_30 	                        ,0x00000020,}, // ADC_REG_30
    {ADC_REG_31 	                        ,0x00000020,}, // ADC_REG_31
    {ADC_REG_32 	                        ,0x00000008,}, // ADC_REG_32
    {ADC_REG_33 	                        ,0x0000000f,}, // ADC_REG_33
    {ADC_REG_34 	                        ,0x00000025,}, // ADC_REG_34
    {ADC_REG_35 	                        ,0x00000002,}, // ADC_REG_35
    //{ADC_REG_36,	                        ,0x00000010,}, // ADC_REG_36
    //{ADC_REG_37,	                        ,0x00000000,}, // ADC_REG_37
    {ADC_REG_38 	                        ,0x00000000,}, // ADC_REG_38
    {ADC_REG_39 	                        ,0x000000c0,}, // ADC_REG_39
    {ADC_REG_3A 	                        ,0x000000e3,}, // ADC_REG_3A
    {ADC_REG_3B 	                        ,0x0000005c,}, // ADC_REG_3B
    {ADC_REG_3C 	                        ,0x00000054,}, // ADC_REG_3C
    {ADC_REG_3D 	                        ,0x00000081,}, // ADC_REG_3D
    {ADC_REG_3E 	                        ,0x00000001,}, // ADC_REG_3E
    {ADC_REG_3F 	                        ,0x00000008,}, // ADC_REG_3F
    //{ADC_REG_40,	                        ,0x00000070,}, // ADC_REG_40
    {ADC_REG_41 	                        ,0x00000007,}, // ADC_REG_41
    //{ADC_REG_42,	                        ,0x00000075,}, // ADC_REG_42
    {ADC_REG_43 	                        ,0x000000f8,}, // ADC_REG_43
    //{ADC_REG_44,	                        ,0x000000f0,}, // ADC_REG_44
    //{ADC_REG_45,	                        ,0x000000f0,}, // ADC_REG_45
    {ADC_REG_46 	                        ,0x00000051,}, // ADC_REG_46
    {ADC_REG_47 	                        ,0x0000005a,}, // ADC_REG_47
    //{ADC_REG_48,	                        ,0x00000000,}, // ADC_REG_48
    //{ADC_REG_49,	                        ,0x00000000,}, // ADC_REG_49
    //{ADC_REG_4A,	                        ,0x00000085,}, // ADC_REG_4A
    //{ADC_REG_4B,	                        ,0x00000001,}, // ADC_REG_4B
    //{ADC_REG_4C,	                        ,0x00000000,}, // ADC_REG_4C
    //{ADC_REG_4D,	                        ,0x00000030,}, // ADC_REG_4D
    //{ADC_REG_4E,	                        ,0x00000000,}, // ADC_REG_4E
    //{ADC_REG_4F,	                        ,0x00000000,}, // ADC_REG_4F
    //{ADC_REG_50,	                        ,0x00000018,}, // ADC_REG_50
    //{ADC_REG_51,	                        ,0x00000015,}, // ADC_REG_51
    //{ADC_REG_52,	                        ,0x00000003,}, // ADC_REG_52
    //{ADC_REG_53,	                        ,0x00000000,}, // ADC_REG_53
    //{ADC_REG_54,	                        ,0x000000e0,}, // ADC_REG_54
    //{ADC_REG_55,	                        ,0x00000000,}, // ADC_REG_55
    {ADC_REG_56 	                        ,0x00000010,}, // ADC_REG_56
    //{ADC_REG_57,	                        ,0x00000000,}, // ADC_REG_57
    {ADC_REG_58 	                        ,0x00000009,}, // ADC_REG_58
    {ADC_REG_59 	                        ,0x00000007,}, // ADC_REG_59
    {ADC_REG_5A 	                        ,0x00000002,}, // ADC_REG_5A
    {ADC_REG_5B 	                        ,0x00000022,}, // ADC_REG_5B
    {ADC_REG_5C 	                        ,0x0000008e,}, // ADC_REG_5C
    {ADC_REG_5D 	                        ,0x0000006f,}, // ADC_REG_5D
    {ADC_REG_5E 	                        ,0x0000000f,}, // ADC_REG_5E
    {ADC_REG_5F 	                        ,0x00000009,}, // ADC_REG_5F
    {ADC_REG_60 	                        ,0x000000f4,}, // ADC_REG_60
    {ADC_REG_61 	                        ,0x00000000,}, // ADC_REG_61
    {ADC_REG_62 	                        ,0x0000000f,}, // ADC_REG_62
    {ADC_REG_63 	                        ,0x00000019,}, // ADC_REG_63
    {ADC_REG_64 	                        ,0x00000000,}, // ADC_REG_64
    {ADC_REG_65 	                        ,0x00000000,}, // ADC_REG_65
    {ADC_REG_66 	                        ,0x00000010,}, // ADC_REG_66
    //{ADC_REG_67,	                        ,0x00000000,}, // ADC_REG_67
    {ADC_REG_68 	                        ,0x0000000b,}, // ADC_REG_68
    {ADC_REG_69 	                        ,0x00000039,}, // ADC_REG_69
    //{ADC_REG_6A,	                        ,0x00000000,}, // ADC_REG_6A
    //{ADC_REG_6B,	                        ,0x00000000,}, // ADC_REG_6B
    //{ADC_REG_6C,	                        ,0x00000000,}, // ADC_REG_6C
    //{ADC_REG_6D,	                        ,0x00000000,}, // ADC_REG_6D
    {0xFFFFFFFF                             ,0x00000000,}
};
	    /* TOP */
const static  int cvbs_top_reg_default[][2] = {
    {TVFE_DVSS_MUXCTRL                      ,0x00000000,}, // TVFE_DVSS_MUXCTRL
    {TVFE_DVSS_MUXVS_REF                    ,0x00000000,}, // TVFE_DVSS_MUXVS_REF
    {TVFE_DVSS_MUXCOAST_V                   ,0x00000000,}, // TVFE_DVSS_MUXCOAST_V
    {TVFE_DVSS_SEP_HVWIDTH                  ,0x00000000,}, // TVFE_DVSS_SEP_HVWIDTH
    {TVFE_DVSS_SEP_HPARA                    ,0x00000000,}, // TVFE_DVSS_SEP_HPARA
    {TVFE_DVSS_SEP_VINTEG                   ,0x00000000,}, // TVFE_DVSS_SEP_VINTEG
    {TVFE_DVSS_SEP_H_THR                    ,0x00000000,}, // TVFE_DVSS_SEP_H_THR
    {TVFE_DVSS_SEP_CTRL                     ,0x00000000,}, // TVFE_DVSS_SEP_CTRL
    {TVFE_DVSS_GEN_WIDTH                    ,0x00000000,}, // TVFE_DVSS_GEN_WIDTH
    {TVFE_DVSS_GEN_PRD                      ,0x00000000,}, // TVFE_DVSS_GEN_PRD
    {TVFE_DVSS_GEN_COAST                    ,0x00000000,}, // TVFE_DVSS_GEN_COAST
    {TVFE_DVSS_NOSIG_PARA                   ,0x00000000,}, // TVFE_DVSS_NOSIG_PARA
    {TVFE_DVSS_NOSIG_PLS_TH                 ,0x00000000,}, // TVFE_DVSS_NOSIG_PLS_TH
    {TVFE_DVSS_GATE_H                       ,0x00000000,}, // TVFE_DVSS_GATE_H
    {TVFE_DVSS_GATE_V                       ,0x00000000,}, // TVFE_DVSS_GATE_V
    {TVFE_DVSS_INDICATOR1                   ,0x00000000,}, // TVFE_DVSS_INDICATOR1
    {TVFE_DVSS_INDICATOR2                   ,0x00000000,}, // TVFE_DVSS_INDICATOR2
    {TVFE_DVSS_MVDET_CTRL1                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL1
    {TVFE_DVSS_MVDET_CTRL2                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL2
    {TVFE_DVSS_MVDET_CTRL3                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL3
    {TVFE_DVSS_MVDET_CTRL4                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL4
    {TVFE_DVSS_MVDET_CTRL5                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL5
    {TVFE_DVSS_MVDET_CTRL6                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL6
    {TVFE_DVSS_MVDET_CTRL7                  ,0x00000000,}, // TVFE_DVSS_MVDET_CTRL7
    {TVFE_SYNCTOP_SPOL_MUXCTRL              ,0x00000009,}, // TVFE_SYNCTOP_SPOL_MUXCTRL
    {TVFE_SYNCTOP_INDICATOR1_HCNT           ,0x00000000,}, // TVFE_SYNCTOP_INDICATOR1_HCNT
    {TVFE_SYNCTOP_INDICATOR2_VCNT           ,0x00000000,}, // TVFE_SYNCTOP_INDICATOR2_VCNT
    {TVFE_SYNCTOP_INDICATOR3                ,0x00000000,}, // TVFE_SYNCTOP_INDICATOR3
    {TVFE_SYNCTOP_SFG_MUXCTRL1              ,0x00000000,}, // TVFE_SYNCTOP_SFG_MUXCTRL1
    {TVFE_SYNCTOP_SFG_MUXCTRL2              ,0x00330000,}, // TVFE_SYNCTOP_SFG_MUXCTRL2
    {TVFE_SYNCTOP_INDICATOR4                ,0x00000000,}, // TVFE_SYNCTOP_INDICATOR4
    {TVFE_SYNCTOP_SAM_MUXCTRL               ,0x00082001,}, // TVFE_SYNCTOP_SAM_MUXCTRL
    {TVFE_MISC_WSS1_MUXCTRL1                ,0x00000000,}, // TVFE_MISC_WSS1_MUXCTRL1
    {TVFE_MISC_WSS1_MUXCTRL2                ,0x00000000,}, // TVFE_MISC_WSS1_MUXCTRL2
    {TVFE_MISC_WSS2_MUXCTRL1                ,0x00000000,}, // TVFE_MISC_WSS2_MUXCTRL1
    {TVFE_MISC_WSS2_MUXCTRL2                ,0x00000000,}, // TVFE_MISC_WSS2_MUXCTRL2
    {TVFE_MISC_WSS1_INDICATOR1              ,0x00000000,}, // TVFE_MISC_WSS1_INDICATOR1
    {TVFE_MISC_WSS1_INDICATOR2              ,0x00000000,}, // TVFE_MISC_WSS1_INDICATOR2
    {TVFE_MISC_WSS1_INDICATOR3              ,0x00000000,}, // TVFE_MISC_WSS1_INDICATOR3
    {TVFE_MISC_WSS1_INDICATOR4              ,0x00000000,}, // TVFE_MISC_WSS1_INDICATOR4
    {TVFE_MISC_WSS1_INDICATOR5              ,0x00000000,}, // TVFE_MISC_WSS1_INDICATOR5
    {TVFE_MISC_WSS2_INDICATOR1              ,0x00000000,}, // TVFE_MISC_WSS2_INDICATOR1
    {TVFE_MISC_WSS2_INDICATOR2              ,0x00000000,}, // TVFE_MISC_WSS2_INDICATOR2
    {TVFE_MISC_WSS2_INDICATOR3              ,0x00000000,}, // TVFE_MISC_WSS2_INDICATOR3
    {TVFE_MISC_WSS2_INDICATOR4              ,0x00000000,}, // TVFE_MISC_WSS2_INDICATOR4
    {TVFE_MISC_WSS2_INDICATOR5              ,0x00000000,}, // TVFE_MISC_WSS2_INDICATOR5
    {TVFE_AP_MUXCTRL1                       ,0x00000000,}, // TVFE_AP_MUXCTRL1
    {TVFE_AP_MUXCTRL2                       ,0x00000000,}, // TVFE_AP_MUXCTRL2
    {TVFE_AP_MUXCTRL3                       ,0x00000000,}, // TVFE_AP_MUXCTRL3
    {TVFE_AP_MUXCTRL4                       ,0x00000000,}, // TVFE_AP_MUXCTRL4
    {TVFE_AP_MUXCTRL5                       ,0x00000000,}, // TVFE_AP_MUXCTRL5
    {TVFE_AP_INDICATOR1                     ,0x00000000,}, // TVFE_AP_INDICATOR1
    {TVFE_AP_INDICATOR2                     ,0x00000000,}, // TVFE_AP_INDICATOR2
    {TVFE_AP_INDICATOR3                     ,0x00000000,}, // TVFE_AP_INDICATOR3
    {TVFE_AP_INDICATOR4                     ,0x00000000,}, // TVFE_AP_INDICATOR4
    {TVFE_AP_INDICATOR5                     ,0x00000000,}, // TVFE_AP_INDICATOR5
    {TVFE_AP_INDICATOR6                     ,0x00000000,}, // TVFE_AP_INDICATOR6
    {TVFE_AP_INDICATOR7                     ,0x00000000,}, // TVFE_AP_INDICATOR7
    {TVFE_AP_INDICATOR8                     ,0x00000000,}, // TVFE_AP_INDICATOR8
    {TVFE_AP_INDICATOR9                     ,0x00000000,}, // TVFE_AP_INDICATOR9
    {TVFE_AP_INDICATOR10                    ,0x00000000,}, // TVFE_AP_INDICATOR10
    {TVFE_AP_INDICATOR11                    ,0x00000000,}, // TVFE_AP_INDICATOR11
    {TVFE_AP_INDICATOR12                    ,0x00000000,}, // TVFE_AP_INDICATOR12
    {TVFE_AP_INDICATOR13                    ,0x00000000,}, // TVFE_AP_INDICATOR13
    {TVFE_AP_INDICATOR14                    ,0x00000000,}, // TVFE_AP_INDICATOR14
    {TVFE_AP_INDICATOR15                    ,0x00000000,}, // TVFE_AP_INDICATOR15
    {TVFE_AP_INDICATOR16                    ,0x00000000,}, // TVFE_AP_INDICATOR16
    {TVFE_AP_INDICATOR17                    ,0x00000000,}, // TVFE_AP_INDICATOR17
    {TVFE_AP_INDICATOR18                    ,0x00000000,}, // TVFE_AP_INDICATOR18
    {TVFE_AP_INDICATOR19                    ,0x00000000,}, // TVFE_AP_INDICATOR19
    {TVFE_BD_MUXCTRL1                       ,0x00000000,}, // TVFE_BD_MUXCTRL1
    {TVFE_BD_MUXCTRL2                       ,0x00000000,}, // TVFE_BD_MUXCTRL2
    {TVFE_BD_MUXCTRL3                       ,0x00000000,}, // TVFE_BD_MUXCTRL3
    {TVFE_BD_MUXCTRL4                       ,0x00000000,}, // TVFE_BD_MUXCTRL4
    {TVFE_CLP_MUXCTRL1                      ,0x00000000,}, // TVFE_CLP_MUXCTRL1
    {TVFE_CLP_MUXCTRL2                      ,0x00000000,}, // TVFE_CLP_MUXCTRL2
    {TVFE_CLP_MUXCTRL3                      ,0x00000000,}, // TVFE_CLP_MUXCTRL3
    {TVFE_CLP_MUXCTRL4                      ,0x00000000,}, // TVFE_CLP_MUXCTRL4
    {TVFE_CLP_INDICATOR1                    ,0x00000000,}, // TVFE_CLP_INDICATOR1
    {TVFE_BPG_BACKP_H                       ,0x00000000,}, // TVFE_BPG_BACKP_H
    {TVFE_BPG_BACKP_V                       ,0x00000000,}, // TVFE_BPG_BACKP_V
    {TVFE_DEG_H                             ,0x00000000,}, // TVFE_DEG_H
    {TVFE_DEG_VODD                          ,0x00000000,}, // TVFE_DEG_VODD
    {TVFE_DEG_VEVEN                         ,0x00000000,}, // TVFE_DEG_VEVEN
    {TVFE_OGO_OFFSET1                       ,0x00000000,}, // TVFE_OGO_OFFSET1
    {TVFE_OGO_GAIN1                         ,0x00000000,}, // TVFE_OGO_GAIN1
    {TVFE_OGO_GAIN2                         ,0x00000000,}, // TVFE_OGO_GAIN2
    {TVFE_OGO_OFFSET2                       ,0x00000000,}, // TVFE_OGO_OFFSET2
    {TVFE_OGO_OFFSET3                       ,0x00000000,}, // TVFE_OGO_OFFSET3
    {TVFE_VAFE_CTRL                         ,0x00000000,}, // TVFE_VAFE_CTRL
    {TVFE_VAFE_STATUS                       ,0x00000000,}, // TVFE_VAFE_STATUS
    {TVFE_TOP_CTRL                          ,0x00004360,}, // TVFE_TOP_CTRL
    {TVFE_CLAMP_INTF                        ,0x00008666,}, // TVFE_CLAMP_INTF
    {TVFE_RST_CTRL                          ,0x00000000,}, // TVFE_RST_CTRL
    {TVFE_EXT_VIDEO_AFE_CTRL_MUX1           ,0x00000000,}, // TVFE_EXT_VIDEO_AFE_CTRL_MUX1
    {TVFE_AAFILTER_CTRL1                    ,0x00082222,}, // TVFE_AAFILTER_CTRL1
    {TVFE_AAFILTER_CTRL2                    ,0x252b39c6,}, // TVFE_AAFILTER_CTRL2
    {TVFE_EDID_CONFIG                       ,0x00000000,}, // TVFE_EDID_CONFIG
    {TVFE_EDID_RAM_ADDR                     ,0x00000000,}, // TVFE_EDID_RAM_ADDR
    {TVFE_EDID_RAM_WDATA                    ,0x00000000,}, // TVFE_EDID_RAM_WDATA
    {TVFE_EDID_RAM_RDATA                    ,0x00000000,}, // TVFE_EDID_RAM_RDATA
    {TVFE_APB_ERR_CTRL_MUX1                 ,0x00000000,}, // TVFE_APB_ERR_CTRL_MUX1
    {TVFE_APB_ERR_CTRL_MUX2                 ,0x00000000,}, // TVFE_APB_ERR_CTRL_MUX2
    {TVFE_APB_INDICATOR1                    ,0x00000000,}, // TVFE_APB_INDICATOR1
    {TVFE_APB_INDICATOR2                    ,0x00000000,}, // TVFE_APB_INDICATOR2
    {TVFE_ADC_READBACK_CTRL                 ,0x80140003,}, // TVFE_ADC_READBACK_CTRL
    {TVFE_ADC_READBACK_INDICATOR            ,0x00000000,}, // TVFE_ADC_READBACK_INDICATOR
    {TVFE_INT_CLR                           ,0x00000000,}, // TVFE_INT_CLR
    {TVFE_INT_MSKN                          ,0x00000000,}, // TVFE_INT_MASKN
    {TVFE_INT_INDICATOR1                    ,0x00000000,}, // TVFE_INT_INDICATOR1
    {TVFE_INT_SET                           ,0x00000000,}, // TVFE_INT_SET
    {TVFE_CHIP_VERSION                      ,0x00000000,}, // TVFE_CHIP_VERSION
    {0xFFFFFFFF                             ,0x00000000,}
};
	    /* CVD2 */
const static  int cvbs_cvd_reg_default[][2] = {
    {CVD2_CONTROL0                          ,0x00000032,}, // CVD2_CONTROL0
    {CVD2_CONTROL1                          ,0x00000008,}, // CVD2_CONTROL1
    {CVD2_CONTROL2                          ,0x00000043,}, // CVD2_CONTROL2
    {CVD2_YC_SEPARATION_CONTROL             ,0x00000012,}, // CVD2_YC_SEPARATION_CONTROL
    {CVD2_LUMA_AGC_VALUE                    ,0x000000dd,}, // CVD2_LUMA_AGC_VALUE
    {CVD2_NOISE_THRESHOLD                   ,0x000000ff,}, // CVD2_NOISE_THRESHOLD
    {CVD2_REG_06                            ,0x00000080,}, // CVD2_REG_06
    {CVD2_OUTPUT_CONTROL                    ,0x00000023,}, // CVD2_OUTPUT_CONTROL
    {CVD2_LUMA_CONTRAST_ADJUSTMENT          ,0x00000080,}, // CVD2_LUMA_CONTRAST_ADJUSTMENT
    {CVD2_LUMA_BRIGHTNESS_ADJUSTMENT        ,0x00000016,}, // CVD2_LUMA_BRIGHTNESS_ADJUSTMENT
    {CVD2_CHROMA_SATURATION_ADJUSTMENT      ,0x00000080,}, // CVD2_CHROMA_SATURATION_ADJUSTMENT
    {CVD2_CHROMA_HUE_PHASE_ADJUSTMENT       ,0x00000000,}, // CVD2_CHROMA_HUE_PHASE_ADJUSTMENT
    {CVD2_CHROMA_AGC                        ,0x00000080,}, // CVD2_CHROMA_AGC
    {CVD2_CHROMA_KILL                       ,0x00000000,}, // CVD2_CHROMA_KILL
    {CVD2_NON_STANDARD_SIGNAL_THRESHOLD     ,0x00000006,}, // CVD2_NON_STANDARD_SIGNAL_THRESHOLD
    {CVD2_CONTROL0F                         ,0x00000000,}, // CVD2_CONTROL0F
    {CVD2_AGC_PEAK_NOMINAL                  ,0x0000000a,}, // CVD2_AGC_PEAK_NOMINAL
    {CVD2_AGC_PEAK_AND_GATE_CONTROLS        ,0x00000009,}, // CVD2_AGC_PEAK_AND_GATE_CONTROLS
    {CVD2_BLUE_SCREEN_Y                     ,0x00000010,}, // CVD2_BLUE_SCREEN_Y
    {CVD2_BLUE_SCREEN_CB                    ,0x000000b4,}, // CVD2_BLUE_SCREEN_CB
    {CVD2_BLUE_SCREEN_CR                    ,0x00000080,}, // CVD2_BLUE_SCREEN_CR
    {CVD2_HDETECT_CLAMP_LEVEL               ,0x00000020,}, // CVD2_HDETECT_CLAMP_LEVEL
    {CVD2_LOCK_COUNT                        ,0x000000f6,}, // CVD2_LOCK_COUNT
    {CVD2_H_LOOP_MAXSTATE                   ,0x0000000b,}, // CVD2_H_LOOP_MAXSTATE
    {CVD2_CHROMA_DTO_INCREMENT_31_24        ,0x0000002d,}, // CVD2_CHROMA_DTO_INCREMENT_31_24
    {CVD2_CHROMA_DTO_INCREMENT_23_16        ,0x00000066,}, // CVD2_CHROMA_DTO_INCREMENT_23_16
    {CVD2_CHROMA_DTO_INCREMENT_15_8         ,0x00000077,}, // CVD2_CHROMA_DTO_INCREMENT_15_8
    {CVD2_CHROMA_DTO_INCREMENT_7_0          ,0x0000002d,}, // CVD2_CHROMA_DTO_INCREMENT_7_0
    {CVD2_HSYNC_DTO_INCREMENT_31_24         ,0x00000022,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_31 _24
    {CVD2_HSYNC_DTO_INCREMENT_23_16         ,0x0000008f,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_23 _16
    {CVD2_HSYNC_DTO_INCREMENT_15_8          ,0x0000005c,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_15 _8
    {CVD2_HSYNC_DTO_INCREMENT_7_0           ,0x00000028,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_7_ 0
    {CVD2_HSYNC_RISING_EDGE          	    ,0x0000003e,}, // CVD2_HORIZONTAL_SYNC_RISING_EDGE_OCCU RRENCE_TIME
    {CVD2_HSYNC_PHASE_OFFSET                ,0x0000003e,}, // CVD2_HORIZONTAL_SYNC_PHASE_OFFSET
    {CVD2_HSYNC_DETECT_WINDOW_START         ,0x00000000,}, // CVD2_HORIZONTAL_SYNC_DETECT_WINDOW_ST ART_TIME
    {CVD2_HSYNC_DETECT_WINDOW_END           ,0x00000080,}, // CVD2_HORIZONTAL_SYNC_DETECT_WINDOW_EN D_TIME
    {CVD2_CLAMPAGC_CONTROL                  ,0x000000f3,}, // CVD2_CLAMPAGC_CONTROL
    {CVD2_HSYNC_WIDTH_STATUS                ,0x0000003e,}, // CVD2_HSYNC_WIDTH_STATUS
    {CVD2_HSYNC_RISING_EDGE_START  	        ,0x0000006d,}, // CVD2_HORIZONTAL_SYNC_RISING_EDGE_DETECT_WINDOW_START_TIME
    {CVD2_HSYNC_RISING_EDGE_END    	        ,0x0000005a,}, // CVD2_HORIZONTAL_SYNC_RISING_EDGE_DETECT_WINDOW_END_TIME
    {CVD2_STATUS_BURST_MAGNITUDE_LSB        ,0x00000007,}, // CVD2_STATUS_BURST_MAGNITUDE_LSB
    {CVD2_STATUS_BURST_MAGNITUDE_MSB        ,0x00000029,}, // CVD2_STATUS_BURST_MAGNITUDE_MSB
    {CVD2_HSYNC_FILTER_GATE_START           ,0x000000d6,}, // CVD2_HSYNC_FILTER_GATE_START_TIME
    {CVD2_HSYNC_FILTER_GATE_END             ,0x0000004e,}, // CVD2_HSYNC_FILTER_GATE_END_TIME
    {CVD2_CHROMA_BURST_GATE_START           ,0x00000032,}, // CVD2_CHROMA_BURST_GATE_START_TIME
    {CVD2_CHROMA_BURST_GATE_END             ,0x00000046,}, // CVD2_CHROMA_BURST_GATE_END_TIME
    {CVD2_ACTIVE_VIDEO_HSTART               ,0x00000084,}, // CVD2_ACTIVE_VIDEO_HORIZONTAL_START_TIME
    {CVD2_ACTIVE_VIDEO_HWIDTH               ,0x00000052,}, // CVD2_ACTIVE_VIDEO_HORIZONTAL_WIDTH
    {CVD2_ACTIVE_VIDEO_VSTART               ,0x0000002a,}, // CVD2_ACTIVE_VIDEO_VERTICAL_START_TIME
    {CVD2_ACTIVE_VIDEO_VHEIGHT              ,0x000000c4,}, // CVD2_ACTIVE_VIDEO_VERTICAL_HEIGHT
    {CVD2_VSYNC_H_LOCKOUT_START             ,0x00000070,}, // CVD2_VSYNC_H_LOCKOUT_START
    {CVD2_VSYNC_H_LOCKOUT_END               ,0x0000000e,}, // CVD2_VSYNC_H_LOCKOUT_END
    {CVD2_VSYNC_AGC_LOCKOUT_START           ,0x0000006c,}, // CVD2_VSYNC_AGC_LOCKOUT_START
    {CVD2_VSYNC_AGC_LOCKOUT_END             ,0x00000010,}, // CVD2_VSYNC_AGC_LOCKOUT_END
    {CVD2_VSYNC_VBI_LOCKOUT_START           ,0x00000000,}, // CVD2_VSYNC_VBI_LOCKOUT_START
    {CVD2_VSYNC_VBI_LOCKOUT_END             ,0x00000023,}, // CVD2_VSYNC_VBI_LOCKOUT_END
    {CVD2_VSYNC_CNTL                        ,0x00000001,}, // CVD2_VSYNC_CNTL
    {CVD2_VSYNC_TIME_CONSTANT               ,0x0000000a,}, // CVD2_VSYNC_TIME_CONSTANT
    {CVD2_STATUS_REGISTER1                  ,0x00000000,}, // CVD2_STATUS REGISTER1
    {CVD2_STATUS_REGISTER2                  ,0x00000000,}, // CVD2_STATUS_REGISTER2
    {CVD2_STATUS_REGISTER3                  ,0x00000000,}, // CVD2_STATUS_REGISTER3
    {CVD2_DEBUG_ANALOG                      ,0x00000000,}, // CVD2_DEBUG_ANALOG
    {CVD2_DEBUG_DIGITAL                     ,0x00000000,}, // CVD2_DEBUG_DIGITAL
    {CVD2_RESET_REGISTER                    ,0x00000000,}, // CVD2_RESET_REGISTER
    {CVD2_HSYNC_DTO_INC_STATUS_29_24        ,0x00000000,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_STATUS_29_24
    {CVD2_HSYNC_DTO_INC_STATUS_23_16        ,0x00000000,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_STATUS_23_16
    {CVD2_HSYNC_DTO_INC_STATUS_15_8         ,0x00000000,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_STATUS_15_8
    {CVD2_HSYNC_DTO_INC_STATUS_7_0          ,0x00000000,}, // CVD2_HORIZONTAL_SYNC_DTO_INCREMENT_STATUS_7_0
    {CVD2_CHROMA_DTO_INC_STATUS_29_24       ,0x00000000,}, // CVD2_CHROMA_SYNC_DTO_INCREMENT_STATUS_29_24
    {CVD2_CHROMA_DTO_INC_STATUS_23_16       ,0x00000000,}, // CVD2_CHROMA_SYNC_DTO_INCREMENT_STATUS_23_16
    {CVD2_CHROMA_DTO_INC_STATUS_15_8        ,0x00000000,}, // CVD2_CHROMA_SYNC_DTO_INCREMENT_STATUS_15_8
    {CVD2_CHROMA_DTO_INC_STATUS_7_0         ,0x00000000,}, // CVD2_CHROMA_SYNC_DTO_INCREMENT_STATUS_7_0
    {CVD2_AGC_GAIN_STATUS_11_8              ,0x00000000,}, // CVD2_AGC_GAIN_STATUS_11_8
    {CVD2_AGC_GAIN_STATUS_7_0               ,0x00000000,}, // CVD2_AGC_GAIN_STATUS_7_0
    {CVD2_CHROMA_MAGNITUDE_STATUS           ,0x00000000,}, // CVD2_CHROMA_MAGNITUDE_STATUS
    {CVD2_CHROMA_GAIN_STATUS_13_8           ,0x00000000,}, // CVD2_CHROMA_GAIN_STATUS_13_8
    {CVD2_CHROMA_GAIN_STATUS_7_0            ,0x00000000,}, // CVD2_CHROMA_GAIN_STATUS_7_0
    {CVD2_CORDIC_FREQUENCY_STATUS           ,0x00000000,}, // CVD2_CORDIC_FREQUENCY_STATUS
    {CVD2_SYNC_HEIGHT_STATUS                ,0x00000000,}, // CVD2_SYNC_HEIGHT_STATUS
    {CVD2_SYNC_NOISE_STATUS                 ,0x00000000,}, // CVD2_SYNC_NOISE_STATUS
    {CVD2_COMB_FILTER_THRESHOLD1            ,0x00000004,}, // CVD2_COMB_FILTER_THRESHOLD1
    {CVD2_COMB_FILTER_CONFIG                ,0x00000042,}, // CVD2_COMB_FILTER_CONFIG
    {CVD2_COMB_LOCK_CONFIG                  ,0x0000006f,}, // CVD2_COMB_LOCK_CONFIG
    {CVD2_COMB_LOCK_MODE                    ,0x00000003,}, // CVD2_COMB_LOCK_MODE
    {CVD2_NONSTANDARD_SIGNAL_STATUS_10_8    ,0x00000000,}, // CVD2_NONSTANDARD_SIGNAL_STATUS_BITS_10_8
    {CVD2_NONSTANDARD_SIGNAL_STATUS_7_0     ,0x00000000,}, // CVD2_NONSTANDARD_SIGNAL_STATUS_BITS_7_0
    {CVD2_REG_87                            ,0x00000000,}, // CVD2_REG_87
    {CVD2_COLORSTRIPE_DETECTION_CONTROL     ,0x00000000,}, // CVD2_COLORSTRIPE_DETECTION_CONTROL
    {CVD2_CHROMA_LOOPFILTER_STATE           ,0x0000000a,}, // CVD2_CHROMA_LOOPFILTER_STATE
    {CVD2_CHROMA_HRESAMPLER_CONTROL         ,0x00000011,}, // CVD2_CHROMA_HRESAMPLER_CONTROL
    {CVD2_CHARGE_PUMP_DELAY_CONTROL         ,0x0000000a,}, // CVD2_CHARGE_PUMP_DELAY_CONTROL
    {CVD2_CHARGE_PUMP_ADJUSTMENT            ,0x00000000,}, // CVD2_CHARGE_PUMP_ADJUSTMENT
    {CVD2_CHARGE_PUMP_DELAY                 ,0x000000e2,}, // CVD2_CHARGE_PUMP_ DELAY
    {CVD2_MACROVISION_SELECTION             ,0x00000000,}, // CVD2_MACROVISION_SELECTION
    {CVD2_CPUMP_KILL                        ,0x00000000,}, // CVD2_CPUMP_KILL
    {CVD2_CVBS_Y_DELAY                      ,0x00000001,}, // CVD2_CVBS_Y_DELAY
    {CVD2_REG_93                            ,0x20000000,}, // CVD2_REG_93
    {CVD2_REG_94                            ,0x00000008,}, // CVD2_REG_94
    {CVD2_REG_95                            ,0x00000000,}, // CVD2_REG_95
    {CVD2_REG_96                            ,0x00200000,}, // update for M2.CVD2_REG_96
    {CVD2_CHARGE_PUMP_AUTO_CONTROL          ,0x000000c6,}, // CVD2_CHARGE_PUMP_AUTO_CONTROL
    {CVD2_CHARGE_PUMP_FILTER_CONTROL        ,0x0000000b,}, // CVD2_CHARGE_PUMP_FILTER_CONTROL
    {CVD2_CHARGE_PUMP_UP_MAX                ,0x00000000,}, // CVD2_CHARGE_PUMP_UP_MAX
    {CVD2_CHARGE_PUMP_DN_MAX                ,0x00000000,}, // CVD2_CHARGE_PUMP_DN_MAX
    {CVD2_CHARGE_PUMP_UP_DIFF_MAX           ,0x00000008,}, // CVD2_CHARGE_PUMP_UP_DIFF_MAX
    {CVD2_CHARGE_PUMP_DN_DIFF_MAX           ,0x00000008,}, // CVD2_CHARGE_PUMP_DN_DIFF_MAX
    {CVD2_CHARGE_PUMP_Y_OVERRIDE            ,0x00000000,}, // CVD2_CHARGE_PUMP_Y_OVERRIDE
    {CVD2_CHARGE_PUMP_PB_OVERRIDE           ,0x00000000,}, // CVD2_CHARGE_PUMP_PB_OVERRIDE
    {CVD2_CHARGE_PUMP_PR_OVERRIDE           ,0x00000000,}, // CVD2_CHARGE_PUMP_PR_OVERRIDE
    {CVD2_DR_FREQ_11_8                      ,0x00000000,}, // CVD2_DR_FREQ_11_8
    {CVD2_DR_FREQ_7_0                       ,0x00000016,}, // CVD2_DR_FREQ_7_0
    {CVD2_DB_FREQ_11_8                      ,0x0000000f,}, // CVD2_DB_FREQ_11_8
    {CVD2_DB_FREQ_7_0                       ,0x000000c1,}, // CVD2_DB_FREQ_7_0
    {CVD2_2DCOMB_VCHROMA_TH              	,0x000000cf,}, // CVD2_2D_COMB_VERTICAL_CHROMA_CHANGE_THRESHOLD
    {CVD2_2DCOMB_NOISE_TH                   ,0x0000008c,}, // CVD2_2D_COMB_VERTICAL_NOISE_THRESHOLD
    {CVD2_REG_B0                            ,0x000000e0,}, // CVD2_REG_B0
    {CVD2_3DCOMB_FILTER                     ,0x0000002f,}, // CVD2_3D_COMB_FILTER
    {CVD2_REG_B2                            ,0x00000008,}, // CVD2_REG_B2
    {CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL      ,0x00000003,}, // CVD2_2D_COMB_ADAPTIVE_GAIN_CONTROL
    {CVD2_MOTION_DETECTOR_NOISE_TH          ,0x00000024,}, // CVD2_MOTION_DETECTOR_NOISE_THRESHOLD
    {CVD2_CHROMA_EDGE_ENHANCEMENT           ,0x00000022,}, // CVD2_CHROMA_EDGE_ENHANCEMENT
    {CVD2_REG_B6                            ,0x00000002,}, // CVD2_REG_B6
    {CVD2_2D_COMB_NOTCH_GAIN                ,0x00000000,}, // CVD2_2D_COMB_FILTER_AND_NOTCH_FILTER_GAIN
    {CVD2_TEMPORAL_COMB_FILTER_GAIN         ,0x00000000,}, // CVD2_TEMPORAL_COMB_FILTER_GAIN
    {CVD2_ACTIVE_VSTART_FRAME_BUFFER        ,0x00000024,}, // CVD2_ACTIVE_VIDEO_VERTICAL_START_FOR_FRAME_BUFFER
    {CVD2_ACTIVE_VHEIGHT_FRAME_BUFFER       ,0x0000005f,}, // CVD2_ACTIVE_VIDEO_VERTICAL_HEIGHT_FOR_FRAME_BUFFER
    {CVD2_HSYNC_PULSE_CONFIG                ,0x00000000,}, // CVD2_HSYNC_PULSE_CONFIG
    {CVD2_CAGC_TIME_CONSTANT_CONTROL        ,0x00000025,}, // CVD2_CRHOMA_AGC TIME_CONSTANT_CONTROL
    {CVD2_CAGC_CORING_FUNCTION_CONTROL      ,0x00000055,}, // CVD2_CRHOMA_AGC_CORING_FUNCTION_CONTROL
    {CVD2_NEW_DCRESTORE_CNTL                ,0x00000017,}, // CVD2_NEW_DCRESTORE_CNTL
    {CVD2_DCRESTORE_ACCUM_WIDTH             ,0x0000009b,}, // CVD2_DCRESTORE_ACCUM_WIDTH
    {CVD2_MANUAL_GAIN_CONTROL               ,0x00000040,}, // CVD2_MANUAL_GAIN_CONTROL
    {CVD2_BACKPORCH_KILL_THRESHOLD          ,0x000000c0,}, // CVD2_BACKPORCH_KILL_THRESHOLD
    {CVD2_DCRESTORE_HSYNC_MIDPOINT          ,0x000000fe,}, // CVD2_DCRESTORE_HSYNC_MIDPOINT
    {CVD2_SYNC_HEIGHT                       ,0x00000090,}, // CVD2_SYNC_HEIGHT
    {CVD2_VSYNC_SIGNAL_THRESHOLD            ,0x00000000,}, // CVD2_VSYNC_SIGNAL_THRESHOLD
    {CVD2_VSYNC_NO_SIGNAL_THRESHOLD         ,0x00000000,}, // CVD2_VSYNC_NO_SIGNAL_THRESHOLD
    {CVD2_VSYNC_CNTL2                       ,0x00000001,}, // CVD2_VSYNC_CNTL2
    {CVD2_VSYNC_POLARITY_CONTROL            ,0x00000050,}, // CVD2_VSYNC_POLARITY_CONTROL
    {CVD2_VBI_HDETECT_CNTL                  ,0x00000062,}, // CVD2_VBI_HDETECT_CNTL
    {CVD2_MV_PSEUDO_SYNC_RISING_START       ,0x00000090,}, // CVD2_MACROVISION_PSEUDO_SYNC_RISING_START
    {CVD2_MV_PSEUDO_SYNC_RISING_END         ,0x000000e3,}, // CVD2_MACROVISION_PSEUDO_SYNC_RISING_END
    {CVD2_REG_CD                            ,0x0000002c,}, // CVD2_REG_CD
    {CVD2_BIG_HLUMA_TH                      ,0x00000023,}, // CVD2_BIG_HLUMA_TH
    {CVD2_MOTION_DETECTOR_CONTROL           ,0x000000c0,}, // CVD2_MOTION_DETECTOR_CONTROL
    {CVD2_MD_CF_LACTIVITY_LOW               ,0x00000008,}, // CVD2_MD_CF_LACTIVITY_LOW
    {CVD2_MD_CF_CACTIVITY_LOW               ,0x00000010,}, // CVD2_MD_CF_CACTIVITY_LOW
    {CVD2_MD_CF_LACTIVITY_HIGH              ,0x00000010,}, // CVD2_MD_CF_LACTIVITY_HIGH
    {CVD2_MD_CF_CACTIVITY_HIGH              ,0x0000000a,}, // CVD2_MD_CF_CACTIVITY_HIGH
    {CVD2_MD_K_THRESHOLD                    ,0x00000000,}, // CVD2_MD_K_THRESHOLD
    {CVD2_CHROMA_LEVEL                      ,0x0000000d,}, // CVD2_CHROMA_LEVEL
    {CVD2_SPATIAL_LUMA_LEVEL                ,0x00000000,}, // CVD2_SPATIAL_LUMA_LEVEL
    {CVD2_SPATIAL_CHROMA_LEVEL              ,0x00000000,}, // CVD2_SPATIAL_CHROMA_LEVEL
    {CVD2_TCOMB_CHROMA_LEVEL                ,0x00000005,}, // CVD2_TCOMB_CHROMA_LEVEL
    {CVD2_FMDLF_TH                          ,0x00000000,}, // CVD2_FMDLF_TH
    {CVD2_CHROMA_ACTIVITY_LEVEL             ,0x00000005,}, // CVD2_CHROMA_ACTIVITY_LEVEL
    {CVD2_SECAM_FREQ_OFFSET_RANGE           ,0x00000040,}, // CVD2_SECAM_FREQ_OFFSET_RANGE
    {CVD2_SECAM_FLAG_THRESHOLD              ,0x00000008,}, // CVD2_SECAM_FLAG_THRESHOLD
    {CVD2_3DCOMB_MOTION_STATUS_31_24        ,0x00000000,}, // CVD2_3D_COMB_FILTER_MOTION_STATUS_31_24
    {CVD2_3DCOMB_MOTION_STATUS_23_16        ,0x00000000,}, // CVD2_3D_COMB_FILTER_MOTION_STATUS_23_16
    {CVD2_3DCOMB_MOTION_STATUS_15_8         ,0x00000000,}, // CVD2_3D_COMB_FILTER_MOTION_STATUS_15_8
    {CVD2_3DCOMB_MOTION_STATUS_7_0          ,0x00000000,}, // CVD2_3D_COMB_FILTER_MOTION_STATUS_7_0
    {CVD2_HACTIVE_MD_START                  ,0x0000006e,}, // CVD2_HACTIVE_MD_START
    {CVD2_HACTIVE_MD_WIDTH                  ,0x00000040,}, // CVD2_HACTIVE_MD_WIDTH
    {CVD2_REG_E6                            ,0x0000008c,}, // CVD2_REG_E6
    {CVD2_MOTION_CONFIG                     ,0x00000000,}, // CVD2_MOTION_CONFIG
    {CVD2_CHROMA_BW_MOTION                  ,0x00000020,}, // CVD2_CHROMA_BW_MOTION
    {CVD2_FLAT_LUMA_SHIFT                   ,0x00000000,}, // CVD2_FLAT_LUMA_SHIFT
    {CVD2_FRAME_MOTION_TH                   ,0x00000000,}, // CVD2_FRAME_MOTION_TH
    {CVD2_FLAT_LUMA_OFFSET                  ,0x00000000,}, // CVD2_FLAT_LUMA_OFFSET
    {CVD2_FLAT_CHROMA_OFFSET                ,0x00000000,}, // CVD2_FLAT_CHROMA_OFFSET
    {CVD2_CF_FLAT_MOTION_SHIFT              ,0x00000000,}, // CVD2_CF_FLAT_MOTION_SHIFT
    {CVD2_MOTION_DEBUG                      ,0x00000000,}, // CVD2_MOTION_DEBUG
    {CVD2_PHASE_OFFSE_RANGE                 ,0x00000020,}, // CVD2_PHASE_OFFSE_RANGE
    {CVD2_PAL_DETECTION_THRESHOLD           ,0x0000001f,}, // CVD2_PA_DETECTION_THRESHOLD
    {CVD2_CORDIC_FREQUENCY_GATE_START       ,0x0000003c,}, // CVD2_CORDIC_FREQUENCY_GATE_START
    {CVD2_CORDIC_FREQUENCY_GATE_END         ,0x0000006e,}, // CVD2_CORDIC_FREQUENCY_GATE_END
    {CVD2_ADC_CPUMP_SWAP                    ,0x000000e4,}, // CVD2_ADC_CPUMP_SWAP
    {CVD2_COMB3D_CONFIG                     ,0x00000001,}, // CVD2_COMB3D_CONFIG
    {CVD2_REG_FA                            ,0x00000020,}, // CVD2_REG_FA
    {CVD2_CAGC_GATE_START                   ,0x00000032,}, // CVD2_CAGC_GATE_START
    {CVD2_CAGC_GATE_END                     ,0x00000050,}, // CVD2_CAGC_GATE_END
    {CVD2_CKILL_LEVEL_15_8                  ,0x0000001c,}, // CVD2_CKILL_LEVEL_15_8
    {CVD2_CKILL_LEVEL_7_0                   ,0x00000000,}, // CVD2_CKILL_LEVEL_7_0
    {0xFFFFFFFF                             ,0x00000000,}

};

void tvafe_cvd2_reg_init(void )
{
    unsigned int i = 0;
	unsigned temp_data;

    //WRITE_MPEG_REG(CVD2_REG_BASE+0xdf, 2) ;   //make sure cubs point to CVD2

	WRITE_CBUS_REG(CVD2_REG_CD, 0x0c);
	// set threshold to determine if this is PAL input, in detect PAL_M /PAL_CN / PAL_60, sometime, the signal is stable,
	//but PAL detected flag is lost in CVD register 3Ch [0]( STATUS PAL DETECTED)
	WRITE_CBUS_REG(CVD2_PAL_DETECTION_THRESHOLD, 0x40);

	// Hsync DTO infc, input clock is 25Mhz
	WRITE_CBUS_REG(CVD2_HSYNC_DTO_INCREMENT_31_24, 0x22);
	WRITE_CBUS_REG(CVD2_HSYNC_DTO_INCREMENT_23_16, 0x8f);
	WRITE_CBUS_REG(CVD2_HSYNC_DTO_INCREMENT_15_8, 0x5c) ;
	WRITE_CBUS_REG(CVD2_HSYNC_DTO_INCREMENT_7_0, 0x28) ;

	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);  //soft rest
	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);
	temp_data = READ_CBUS_REG(CVD2_RESET_REGISTER);
	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);  //soft reset

    WRITE_CBUS_REG(CVD2_CONTROL0, 0x00);    //bit7--input video format is composite
    							            //bit[6:5]--input video color standard is NTSC
    							            //bit5--the number of scan lines per frame is 525
    							            //bit[3:1]--output display format is NTSC or PAL_M
    							            //bit0--disable HV delay mode(found on Sony studio monitors)
	WRITE_CBUS_REG(CVD2_SECAM_FREQ_OFFSET_RANGE, 0x50);  //adjut SECAM bit threshould value
	cvd2_sig_status.cur_sd_state = SD_NO_SIG;
	cvd2_sig_status.detected_sd_state = SD_NO_SIG;

    //configure ACD setting
        /** write 7740 register **/
    while (cvbs_adc_reg_default[i][0] != 0xFFFFFFFF) {
        WRITE_CBUS_REG(cvbs_adc_reg_default[i][0], cvbs_adc_reg_default[i][1]);
        i++;
    }

    i = 0;
    /** write top register **/
    while (cvbs_top_reg_default[i][0] != 0xFFFFFFFF) {
        WRITE_CBUS_REG(cvbs_top_reg_default[i][0], cvbs_top_reg_default[i][1]);
        i++;
    }

    while (cvbs_acd_reg_default[i][0] != 0xFFFFFFFF) {
        WRITE_CBUS_REG(cvbs_acd_reg_default[i][0], cvbs_acd_reg_default[i][1]);
        i++;
    }

    while (cvbs_cvd_reg_default[i][0] != 0xFFFFFFFF) {
        WRITE_CBUS_REG(cvbs_cvd_reg_default[i][0], cvbs_cvd_reg_default[i][1]);
        i++;
    }

    tvafe_cvd2_reset_cnt();

    return;
}

static void ProgramPAL_I(void)
{
	unsigned buf;
	unsigned hagcVal;

	WriteRegister_CVD2(CVD2_CONTROL0, 0x0e, 1, 1);  // colour_mode
	WriteRegister_CVD2(CVD2_CONTROL0, 0x10, 4, 1);  // vline_625
	WriteRegister_CVD2(CVD2_CONTROL0, 0x60, 5, 1);  // hpixel
	WriteRegister_CVD2(CVD2_YC_SEPARATION_CONTROL, 0x80, 7, 0); // ntsc443_mode

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 165) : (hagcVal = 220);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal);  // hagc--specify the luma AGC target value.
    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32);  //ok--value sets the noise THRESHOLD, Default value =50
    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x20);  //ok swaps cb/cr,
    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0x8a);  //ok--set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_HSTART, 0xff, 0, 132); //130);   //hactive_start--These bits control the active video line time interval. This specifies the beginning of
								//active line. This register is used to centre the horizontal position, and should not be used to crop the image to a smaller size.
	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_HWIDTH, 0xff, 0, 80);    // hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 42);  //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 193); //ok--vactive_height = 384 + value

	WriteRegister_CVD2(CVD2_CHROMA_AGC, 0xff, 0, 138);   //0x75); // cagc: set the chroma AGC target

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);    // ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END,	0x46);  // ok  chroma burst gate  end

	WriteRegister_CVD2(CVD2_CORDIC_FREQUENCY_GATE_END, 0xff, 0, 0x5a);  // cordic_gate_end :--start position for the secam burst gate for cordic frequency measurment
	WriteRegister_CVD2(CVD2_CORDIC_FREQUENCY_GATE_START, 0xff, 0, 0x46);    // cordic_gate_start	,add by xintan

	WriteRegister_CVD2(CVD2_CAGC_GATE_END, 0xff, 0, 80);    // cagc_gate_end : for the burst position used by the chroma agc calculation,add by xintan
	WriteRegister_CVD2(CVD2_CAGC_GATE_START, 0xff, 0, 50);  // cagc_gate_start--specified the beginning of the burst position used by the chroma agc calculation

	hagcVal = 0x2d66772d;
	programCDTO(hagcVal);   //the most-significant byte is stored in 0x18 and the least-significant byte is stored in 0x1b

	WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8, 0x1c);    // ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0, 0x00);     //chroma burst level is before the ckill value. Chroma kill will be active.

	WriteRegister_CVD2(CVD2_REG_B2, 0x40, 6, 0);    // comb2only

	// reset MMU (toggle lbadrgen_rst)
	WriteRegister_CVD2(CVD2_REG_B2, 0x80, 7, 1);    // lbadrgen_rst
	WriteRegister_CVD2(CVD2_REG_B2, 0x80, 7, 0);    // lbadrgen_rst

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);  //SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is PAL_I. \n" );

    return;
}

static void ProgramNTSC(void)
{
	unsigned buf;
	unsigned hagcVal;
	WriteRegister_CVD2(CVD2_CONTROL0, 0xe, 1, 0);   // colour_mode--NTSC
	WriteRegister_CVD2(CVD2_CONTROL0, 0x10, 4, 0);  // vline_525
	WriteRegister_CVD2(CVD2_CONTROL0, 0x60, 5, 0);  // video color standard

	WriteRegister_CVD2(CVD2_CONTROL1, 1, 0, 1);     // ped -- why?

	WriteRegister_CVD2(CVD2_YC_SEPARATION_CONTROL, 0x80, 7, 0); // ntsc443_mode
    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32); //ok--value sets the noise THRESHOLD, Default value =50
	WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x60);  //ok swaps cb/cr, auto blue screen mode.

    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0x8a);  //ok--set chroma  AGC target (default = 138)
	WriteRegister_CVD2(CVD2_CHROMA_KILL, 1, 0, 0);  // Pal60_mode   ,add by xintan

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 166) : (hagcVal = 221);	//if a MacroVision signal is detected and ¡°mv_hagc_mode¡± (02.6h) is set,
										//then this value is automatically reduced by 25%.--(221 * 0.75% = 166)
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal);	//specify the luma AGC target value.
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_HSTART, 0xff, 0, 130);   //132);		// hactive_start
	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_HWIDTH, 0xff, 0, 80); // hactive_width = data + 640,add by xintan

	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_VSTART, 0xff, 0, 34); // vactive_start  (unit is half lines)
	WriteRegister_CVD2(CVD2_ACTIVE_VIDEO_VHEIGHT, 0xff, 0, 97); // vactive_height -- 97=> 384+97=481 half lines)

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);// ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END,	0x46);// ok  chroma burst gate  end

	WriteRegister_CVD2(CVD2_CORDIC_FREQUENCY_GATE_END, 0xff, 0, 0x5a);  	// cordic_gate_end : --start position for the secam burst gate for cordic frequency measurment
	WriteRegister_CVD2(CVD2_CORDIC_FREQUENCY_GATE_START, 0xff, 0, 0x46);    // cordic_gate_start	,add by xintan

	WriteRegister_CVD2(CVD2_CAGC_GATE_END, 0xff, 0, 80);  	// cagc_gate_end : for the burst position used by the chroma agc calculation,add by xintan
	WriteRegister_CVD2(CVD2_CAGC_GATE_START, 0xff, 0, 50);    // cagc_gate_start,--specified the beginning of the burst position used by the chroma agc calculation

	hagcVal = 0x24a78ffc;
	programCDTO(hagcVal);   //the most-significant byte is stored in 0x18 and the least-significant byte is stored in 0x1b

    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8,	0x1c);// ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0,	0x00);//chroma burst level is before the ckill value. Chroma kill will be active.

	WriteRegister_CVD2(CVD2_REG_B2, 0x40, 6, 0);    // comb2only, This bit will enable CVD2 to work only with2D YC separation mode

	// reset MMU (toggle lbadrgen_rst)
	WriteRegister_CVD2(CVD2_REG_B2, 0x80, 7, 1);    // lbadrgen_rst: This bit is use to reset the Line/Frame buffer address generation. This also reset the
                                          //address pointer of the FIFO type FRAME buffer. Each time if the video standard is
                                          //changed (e.g from PAL to NTSC). The FRAME buffer has to set up the read/write
                                          //pointer properly. This bit can be used to force the set up to occur even though the set
                                          //up should be done by the CVD2 automatically.
	WriteRegister_CVD2(CVD2_REG_B2, 0x80, 7, 0);    // lbadrgen_rst

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);  //SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is NTSC. \n ");

    return;
}

static void ProgramNTSC443(void)
{
	unsigned buf;
	unsigned hagcVal;

    WRITE_CBUS_REG(CVD2_CONTROL0, 0x00);  //ok
    WRITE_CBUS_REG(CVD2_CONTROL1, 0x01);  //ok--enable pedestal subtraction in order to luma -= some offset
    WRITE_CBUS_REG(CVD2_CONTROL2, 0x43);  //ok-- bit0 : enable set, enables the luma/composite AGC .
    								//bit1 : enable set, enables the chroma AGC .
    								//bit6--auto resuces the gain (set in register 4) by 25% when macro-vision encoded signals are detected.
    WRITE_CBUS_REG(CVD2_YC_SEPARATION_CONTROL, 0x82);  //ok
	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 166) : (hagcVal = 221);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal); // hagc--specify the luma AGC target value.

    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32);  //ok--value sets the noise THRESHOLD, Default value =50
    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x60);  //ok swaps cb/cr, auto blue screen mode.
    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0x8a);	//ok--set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

//        cubs_write_reg(0x18,	0x2d);// Chroma DTO infc
//        cubs_write_reg(0x19,	0x66);
//        cubs_write_reg(0x1a,	0x77);
//        cubs_write_reg(0x1b,	0x2d);
	hagcVal = 0x2d66772d;
	programCDTO(hagcVal);

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);    // ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END,	0x46);  // ok  chroma burst gate  end

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HSTART,	0x82);  //ok-- hactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HWIDTH, 0x50);   //ok--hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 0x22);    //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 0x61);    //ok--vactive_height = 384 + value

    WRITE_CBUS_REG(CVD2_REG_B2,	0x0c);  // ok  adaptive C bandwide
    WRITE_CBUS_REG(CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL, 0x03);   // ok  frame buffer
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_START, 0x46); // ok--start position for the secam burst gate for cordic frequency measurment
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_END, 0x5a);   // ok
    WRITE_CBUS_REG(CVD2_CAGC_GATE_START, 0x32); // ok  burst agc start--specified the beginning of the burst position used by the chroma agc calculation
    WRITE_CBUS_REG(CVD2_CAGC_GATE_END, 0x50);   // ok  burst agc end

    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8, 0x1c);    // ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0, 0x00); //chroma burst level is before the ckill value. Chroma kill will be active.

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);  //SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is NTSC_443. \n ");

    return;
}

static void ProgramPALCN(void)
{
	unsigned buf;
	unsigned hagcVal;

	WRITE_CBUS_REG(CVD2_CONTROL0, 0x36);  //ok
    WRITE_CBUS_REG(CVD2_CONTROL1, 0x01);  //ok --enable pedestal subtraction in order to luma -= some offset
    WRITE_CBUS_REG(CVD2_CONTROL2, 0x43);  //ok-- bit0 : enable set, enables the luma/composite AGC .
    							//bit1 : enable set, enables the chroma AGC .
    							//bit6--auto resuces the gain (set in register 4) by 25% when macro-vision encoded signals are detected.
    WRITE_CBUS_REG(CVD2_YC_SEPARATION_CONTROL, 0x02);  //ok

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 166) : (hagcVal = 221);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal);// hagc--specify the luma AGC target value.

    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32);  //ok--value sets the noise THRESHOLD, Default value =50
    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x20);  //ok swaps cb/cr
    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0x8a);	//ok--set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

//        cubs_write_reg(0x18,	0x24);// Chroma DTO infc
//        cubs_write_reg(0x19,	0xae);
//        cubs_write_reg(0x1a,	0x25);
//        cubs_write_reg(0x1b,	0x41);
	hagcVal = 0x24ae2541;
	programCDTO(hagcVal);

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);// ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END, 0x46);// ok  chroma burst gate  end

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HSTART,	0x8a); //ok-- hactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HWIDTH,	0x50); //ok--hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 0x2a); //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 0xc1); //ok--vactive_height = 384 + value

    WRITE_CBUS_REG(CVD2_REG_B2,	0x0c);// ok  adaptive C bandwide
    WRITE_CBUS_REG(CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL, 0x03);// ok  frame buffer
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_START, 0x46);// ok--start position for the secam burst gate for cordic frequency measurment
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_END, 0x5a);// ok
    WRITE_CBUS_REG(CVD2_CAGC_GATE_START, 0x32);// ok  burst agc start--specified the beginning of the burst position used by the chroma agc calculation
    WRITE_CBUS_REG(CVD2_CAGC_GATE_END, 0x50);// ok  burst agc end
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8, 0x1c);// ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0, 0xdc);//chroma burst level is before the ckill value. Chroma kill will be active.

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);//SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is PAL_CN. \n ");

    return;
}

static void ProgramPAL60(void)
{
	unsigned buf;
	unsigned hagcVal;

    WRITE_CBUS_REG(CVD2_CONTROL0, 0x04);
    WRITE_CBUS_REG(CVD2_CONTROL1, 0x00);    //pedestal
    WRITE_CBUS_REG(CVD2_CONTROL2, 0x43);  //ok-- bit0 : enable set, enables the luma/composite AGC .
    							//bit1 : enable set, enables the chroma AGC .
    							//bit6--auto resuces the gain (set in register 4) by 25% when macro-vision encoded signals are detected.
    WRITE_CBUS_REG(CVD2_YC_SEPARATION_CONTROL, 0x02);   //adaptive 3d

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 165) : (hagcVal = 220);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal);  // hagc--specify the luma AGC target value.

    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x0a); //ok--value sets the noise THRESHOLD, Default value =50
    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x20);  //YC DELAY for PAL
    WRITE_CBUS_REG(CVD2_CHROMA_AGC, 0x67);  //-set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x01); //Set  video input to Pal60

//        cubs_write_reg(0x18,	0x2d);// Chroma DTO infc
//        cubs_write_reg(0x19,	0x66);
//        cubs_write_reg(0x1a,	0x77);
//        cubs_write_reg(0x1b,	0x2d);
	hagcVal = 0x2d66772d;
	programCDTO(hagcVal);

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);  // ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END, 0x46);  // ok  chroma burst gate  end

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HSTART, 0x84);  //ok-- hactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HWIDTH, 0x50);  //ok--hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 0x2a);  //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 0x61);  //ok--vactive_height = 384 + value

    WRITE_CBUS_REG(CVD2_REG_B2,	0x0c);  //adaptive C bandwide
    WRITE_CBUS_REG(CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL,	0x0b);  //frame buffer
    WRITE_CBUS_REG(CVD2_CAGC_GATE_START,	0x32);  // ok  burst agc start--specified the beginning of the burst position used by the chroma agc calculation
    WRITE_CBUS_REG(CVD2_CAGC_GATE_END,	0x50);  //bust agc end

    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8, 0x05);  // ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0, 0x00);  //chroma burst level is before the ckill value. Chroma kill will be active.

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);  //SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is PAL_60. \n ");

    return;
}

static void ProgramPALM(void)
{
	unsigned buf;
	unsigned hagcVal;

    WRITE_CBUS_REG(CVD2_CONTROL0,	0x04);  //ok1
    WRITE_CBUS_REG(CVD2_CONTROL1,	0x00);  //ok1
    WRITE_CBUS_REG(CVD2_CONTROL2,	0x43);  //ok-- bit0 : enable set, enables the luma/composite AGC .
    							//bit1 : enable set, enables the chroma AGC .
    							//bit6--auto resuces the gain (set in register 4) by 25% when macro-vision encoded signals are detected.
    WRITE_CBUS_REG(CVD2_YC_SEPARATION_CONTROL,	0x02);  //ok

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 166) : (hagcVal = 221);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal); // hagc--specify the luma AGC target value.

    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32);  //ok--value sets the noise THRESHOLD, Default value =50
    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL, 0x20);  //ok swaps cb/cr
    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0x8a);  //set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

	hagcVal = 0x249d4040;
	programCDTO(hagcVal);
	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x32);  // ok chroma burst gate start--default value
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END,	0x46);  // ok  chroma burst gate  end

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HSTART,	0x82); //ok-- hactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HWIDTH, 0x50); //ok--hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 0x22); //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 0x61); //ok--vactive_height = 384 + value

    WRITE_CBUS_REG(CVD2_REG_B0,	0x3a);
    WRITE_CBUS_REG(CVD2_3DCOMB_FILTER,	0xf3);
    WRITE_CBUS_REG(CVD2_REG_B2,	0x0c);  // ok  adaptive C bandwide
    WRITE_CBUS_REG(CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL, 0x03);   // ok  frame buffer
    WRITE_CBUS_REG(CVD2_MOTION_DETECTOR_NOISE_TH, 0x23);

    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_START, 0x46); // ok--start position for the secam burst gate for cordic frequency measurment
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_END, 0x5a);   // ok

    WRITE_CBUS_REG(CVD2_CAGC_GATE_START, 0x32); // ok  burst agc start--specified the beginning of the burst position used by the chroma agc calculation
    WRITE_CBUS_REG(CVD2_CAGC_GATE_END, 0x50);   // ok  burst agc end

    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8,	0x1c);  // ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0,	0x00);  //chroma burst level is before the ckill value. Chroma kill will be active.

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);  //SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is PAL_M. \n ");

   return;
}

static void ProgramSECAM(void)
{
	unsigned int buf;
	unsigned int hagcVal;

    WRITE_CBUS_REG(CVD2_CONTROL0, 0x38);  //ok
    WRITE_CBUS_REG(CVD2_CONTROL1, 0x00);  //ok
    WRITE_CBUS_REG(CVD2_CONTROL2, 0x43);  //ok-- bit0 : enable set, enables the luma/composite AGC .
    							//bit1 : enable set, enables the chroma AGC .
    							//bit6--auto resuces the gain (set in register 4) by 25% when macro-vision encoded signals are detected.
    WRITE_CBUS_REG(CVD2_YC_SEPARATION_CONTROL, 0x00);  //ok--fully adaptive comb (2-D adaptive comb)

	// read macrovision flag mv_vbi_detected
	ReadRegister_CVD2(CVD2_STATUS_REGISTER1, 0x10, 4, &buf);
	buf ? (hagcVal = 165) : (hagcVal = 220);
	WriteRegister_CVD2(CVD2_LUMA_AGC_VALUE, 0xff, 0, hagcVal);  // hagc--specify the luma AGC target value.

    WRITE_CBUS_REG(CVD2_NOISE_THRESHOLD, 0x32); //ok--value sets the noise THRESHOLD, Default value =50

    WRITE_CBUS_REG(CVD2_OUTPUT_CONTROL,	0x20);  //ok swaps cb/cr
    WRITE_CBUS_REG(CVD2_CHROMA_AGC,	0xc8);  //ok--set chroma  AGC target (default = 138)
    WRITE_CBUS_REG(CVD2_CHROMA_KILL, 0x00);

	hagcVal = 0x2be37de9;
	programCDTO(hagcVal);

	//burst gate window-- Note that this window is set to be bigger than the burst. The automatic burst position tracker finds
	//the burst within this window.
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_START, 0x5a);    // ok chroma burst gate start
    WRITE_CBUS_REG(CVD2_CHROMA_BURST_GATE_END,	0x6e);  // ok  chroma burst gate  end

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HSTART,	0x76);  //ok-- hactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_HWIDTH,	0x50);  //ok--hactive_width = data + 640

    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VSTART, 0x29);    //ok-- vactive_start
    WRITE_CBUS_REG(CVD2_ACTIVE_VIDEO_VHEIGHT, 0xbf);    //ok--vactive_height = 384 + value

    WRITE_CBUS_REG(CVD2_REG_B2,	0x0c);  // ok  adaptive C bandwide
    WRITE_CBUS_REG(CVD2_2DCOMB_ADAPTIVE_GAIN_CONTROL, 0x03);   // ok  frame buffer

    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_START, 0x3c); // ok--start position for the secam burst gate for cordic frequency measurment
    WRITE_CBUS_REG(CVD2_CORDIC_FREQUENCY_GATE_END, 0x6e);   // ok

    WRITE_CBUS_REG(CVD2_CAGC_GATE_START, 0x50); // ok  burst agc start--specified the beginning of the burst position used by the chroma agc calculation
    WRITE_CBUS_REG(CVD2_CAGC_GATE_END, 0x6e);   // ok  burst agc end

    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_15_8, 0x05);    // ok C kill ms --determine the threshold for the chroma kill to be active. If the calculated
    WRITE_CBUS_REG(CVD2_CKILL_LEVEL_7_0, 0xdc); //chroma burst level is before the ckill value. Chroma kill will be active.

    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);	//SOFT RESET memory
    WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);
    printk("current SD state is SECAM. \n ");

    return;
}


// *****************************************************************************
// Function: get CVD2 signal status
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
static int tvafe_cvd2_get_signal_status(void)
{
    int ret = 0;
    unsigned char data_0;

    data_0 = READ_CBUS_REG(CVD2_STATUS_REGISTER1);
    //signal status
    cvd2_sig_status.no_sig = data_0 & (1 << NO_SIGNAL_BIT);
    //lock status
    cvd2_sig_status.h_lock = data_0 & (1 << HLOCK_BIT);
    cvd2_sig_status.v_lock = data_0 & (1 << VLOCK_BIT);
    cvd2_sig_status.chroma_lock = data_0 & (1 << CHROMALOCK_BIT);

    data_0 = READ_CBUS_REG(CVD2_STATUS_REGISTER2);
    //color burst status
    cvd2_sig_status.no_color_burst = data_0 & (1 << BKNWT_DETECTED_BIT);
    //H/V frequency non standard status
    cvd2_sig_status.h_nonstd = data_0 & (1 << HNON_STD_BIT);
    cvd2_sig_status.v_nonstd = data_0 & (1 << VNON_STD_BIT);
    cvd2_sig_status.comb3d_off = data_0 & (1 << STATUS_COMB3D_OFF_BIT);

    data_0 = READ_CBUS_REG(CVD2_STATUS_REGISTER3);
    //video standard status
    cvd2_sig_status.line625 = data_0 & (1 << LINES625_DETECTED_BIT);
    cvd2_sig_status.secam = data_0 & (1 << SECAM_DETECTED_BIT);
    cvd2_sig_status.pal = data_0 & (1 << PAL_DETECTED_BIT);
    cvd2_sig_status.noisy = data_0 & (1 << NOISY_BIT);

    //VCR status
    cvd2_sig_status.vcr = data_0 & (1 << VCR_BIT);
    cvd2_sig_status.vcrtrick = data_0 & (1 << VCR_TRICK_BIT);
    cvd2_sig_status.vcrff = data_0 & (1 << VCR_FF_BIT);
    cvd2_sig_status.vcrrew = data_0 >> VCR_REW_BIT;

    return ret;
}

// *****************************************************************************
// Function: check CVD2 lock status by register's status
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
int  tvafe_cvd2_video_locked(void)
{
    int ret = 0;

    //VCR detection status
    if (cvd2_sig_status.vcr == 0) {  // non-vcr
        //H/V locked
        if((cvd2_sig_status.h_lock == 1) || (cvd2_sig_status.v_lock == 1)) {
            ret = 1;
            cvd2_sig_status.hv_lock = 1;
         } else
            cvd2_sig_status.hv_lock = 0;
    } else {                            // vcr mode
        //H/V locked
        if((cvd2_sig_status.h_lock == 1) && (cvd2_sig_status.v_lock == 1)) {
            ret = 1;
            cvd2_sig_status.hv_lock = 1;
        } else
            cvd2_sig_status.hv_lock = 0;
    }

    return ret;
}

// *****************************************************************************
// Function: CVD2 soft reset in video format searching
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
//static void  tvafe_cvd2_soft_reset(void)
//{
//    int temp_data = 0;
//
//    //reset
//	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);//soft rest
//	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x01);
//	temp_data = READ_CBUS_REG(CVD2_RESET_REGISTER);
//	WRITE_CBUS_REG(CVD2_RESET_REGISTER,	0x00);//soft reset
//
//    return ;
//}

// *****************************************************************************
// Function: check CVD2 VCR mode status by register's status
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
//static int  tvafe_cvd2_vcr_mode(void)
//{
//    int ret = 0;

//    if ((cvd2_sig_status.vcr_trick_mode == 1 || cvd2_sig_status.vcr_ff_mode  || cvd2_sig_status.vcr_rewind) && cvd2_sig_status.vcr == 1)
//        ret = 1;

//    return ret;
//}

// *****************************************************************************
// Function: check CVD2 H/V frequency non standard status by register's status
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
//static int  tvafe_cvd2_non_standard_mode(void)
//{
//    int ret = 0;
//
//    if (cvd2_sig_status.h_nonstd == 1 && cvd2_sig_status.v_nonstd == 1)
//        ret = 1;
//
//    return ret;
//}



// *****************************************************************************
// Function: CVD2 search the new video mode
//
//   Params: none
//
//   Return: mode detecion result
//
// *****************************************************************************
static void tvafe_cvd2_search_video_mode(void)
{
    unsigned data;

    tvafe_cvd2_get_signal_status();

    if (cvd2_sig_status.no_sig) {   // if no signal, switch to SD_NO_SIGNAL (from any state)
		cvd2_sig_status.detected_sd_state = SD_NO_SIG;
        tvafe_cvd2_reset_cnt();
		return;
	}

    //if video unlocked, check signal status again
    if (tvafe_cvd2_video_locked() == 0) {
        cvd2_sig_status.detected_sd_state = SD_UNLOCK;
        tvafe_cvd2_reset_cnt();
	    return;
    }

     if(cvd2_sig_status.cur_sd_state == SD_SECAM) {
        if(cvd2_sig_status.pali_to_secam_cnt < PAL_I_TO_SECAM_CNT) {
            cvd2_sig_status.pali_to_secam_cnt++;     // wait 2000 millisec for secam flag to be set
            return;
        } else
            cvd2_sig_status.pali_to_secam_cnt = SECAM_STABLE_CNT;
    }

   ReadRegister_CVD2(CVD2_CORDIC_FREQUENCY_STATUS, 0xFF, 0, &data);
//  if freq non_standard, stop
//    if (tvafe_cvd2_non_standard_mode() == 1)
//    {
//        cvd2_sig_status.detected_sd_state = SD_NONSTANDARD;  //video non-standard signal
//        cvd2_sig_status.cordic_data_sum = 0;
//        cvd2_sig_status.stable_cnt = 0;
//        return ;
//    }

//  if mode is vcr, stop
//    if (tvafe_cvd2_vcr_mode() == 1)
//    {
//        cvd2_sig_status.detected_sd_state = SD__VCR;  //video  VCR mode
//        return ;
//    }

	// If no siganl is off and hv lock is on, the detected standard depends on the current standard
	switch (cvd2_sig_status.cur_sd_state) {
		case SD_NO_SIG:
        case SD_UNLOCK:
            if (tvafe_cvd2_video_locked())
				cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
			break;
		case SD_HV_LOCK:
        case SD_NONSTD:
			if (cvd2_sig_status.hv_lock) {
				if (cvd2_sig_status.line625)
					cvd2_sig_status.detected_sd_state = SD_PAL_I;
				else
					cvd2_sig_status.detected_sd_state = SD_PAL_M;
			}
			break;
		case SD_PAL_I:
			if ((cvd2_sig_status.line625) && (cvd2_sig_status.chroma_lock) \
                && (cvd2_sig_status.pal)) {  // check current state (PAL i)
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                    cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT - 1) {
                    //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum -= cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max;
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        			cvd2_sig_status.fc_Less = ((cvd2_sig_status.cordic_data_sum < FC_LESS_PAL_I_TO_PAL_N_MAX) && (cvd2_sig_status.cordic_data_sum > FC_LESS_PAL_I_TO_PAL_N_MIN)) ? 1 : 0;
        			if(cvd2_sig_status.fc_Less)
        				cvd2_sig_status.detected_sd_state = SD_PAL_CN;
        			else
        				cvd2_sig_status.detected_sd_state = SD_PAL_I;

                    tvafe_cvd2_reset_cnt();
                }
			} else {
                tvafe_cvd2_reset_cnt();

                //PAL_ i''s chroma subcarries & PAL_60's one  are  approximate
                if ((cvd2_sig_status.chroma_lock) && (cvd2_sig_status.pal))
                    cvd2_sig_status.detected_sd_state = SD_PAL_60;
                else
					cvd2_sig_status.detected_sd_state = SD_SECAM;
            }

			break;
		case SD_PAL_M:
            // check current state (PAL M)
			if ((!cvd2_sig_status.line625) && (cvd2_sig_status.chroma_lock) && (cvd2_sig_status.pal) ) {
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                     cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT -1) {
                    cvd2_sig_status.cordic_data_sum -= (cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max);  //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        		    cvd2_sig_status.fc_more   	= (cvd2_sig_status.cordic_data_sum < FC_MORE_PAL_M_TO_PAL_60_MAX) && (cvd2_sig_status.cordic_data_sum > FC_MORE_PAL_M_TO_PAL_60_MIN) ? 1 : 0;
        		    if(cvd2_sig_status.fc_more)
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_60;
        		    else
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_M;

                   tvafe_cvd2_reset_cnt();
                }
			} else {
                tvafe_cvd2_reset_cnt();

                if ((cvd2_sig_status.chroma_lock) && (cvd2_sig_status.pal))
                    cvd2_sig_status.detected_sd_state = SD_PAL_CN;    //PAL_CN''s chroma subcarries & PAL_M's one  are  approximate
                else {
					if ((!cvd2_sig_status.line625) && (!cvd2_sig_status.pal))
						cvd2_sig_status.detected_sd_state = SD_NTSC;  //NTSC_M''s chroma subcarries & PAL_M's one  are  approximate
					else
						cvd2_sig_status.detected_sd_state = SD_HV_LOCK; ;
				}
            }
			break;
		case SD_PAL_CN:
			if ((cvd2_sig_status.line625) &&( cvd2_sig_status.pal) && (cvd2_sig_status.chroma_lock)) {
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                     cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT -1) {
                    cvd2_sig_status.cordic_data_sum -= (cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max);  //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        		    cvd2_sig_status.fc_more   	= (cvd2_sig_status.cordic_data_sum < FC_MORE_PAL_N_TO_PAL_I_MAX) && (cvd2_sig_status.cordic_data_sum > FC_MORE_PAL_N_TO_PAL_I_MIN) ? 1 : 0;
        		    if(cvd2_sig_status.fc_more)
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_I;
        		    else
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_CN;

                    tvafe_cvd2_reset_cnt();
                }
			} else {
                tvafe_cvd2_reset_cnt();

                if ((cvd2_sig_status.chroma_lock) && (cvd2_sig_status.pal))
                    cvd2_sig_status.detected_sd_state = SD_PAL_M;     //PAL_CN''s chroma subcarries & PAL_M's one  are  approximate
                else
				    cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
            }
			break;
        case SD_NTSC_443:
            if ((!cvd2_sig_status.line625) && (!cvd2_sig_status.pal) && (cvd2_sig_status.chroma_lock)) {
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                     cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT -1) {
                    cvd2_sig_status.cordic_data_sum -= (cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max);  //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        		    cvd2_sig_status.fc_Less   	= (cvd2_sig_status.cordic_data_sum < FC_LESS_NTSC443_TO_NTSCM_MAX) && (cvd2_sig_status.cordic_data_sum > FC_LESS_NTSC443_TO_NTSCM_MIN) ? 1 : 0;
        		    if(cvd2_sig_status.fc_Less)
        		    	cvd2_sig_status.detected_sd_state = SD_NTSC;
        		    else
        		    	cvd2_sig_status.detected_sd_state = SD_NTSC_443;

                    tvafe_cvd2_reset_cnt();
                }
			} else {
                tvafe_cvd2_reset_cnt();
				if((!cvd2_sig_status.line625)&& (cvd2_sig_status.pal))
				    cvd2_sig_status.detected_sd_state = SD_PAL_60;      //NTSC_433''s chroma subcarries & PAL_60's one  are  approximate
				else
				    cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
            }
			break;
		case SD_NTSC:
			if ((!cvd2_sig_status.line625) && (!cvd2_sig_status.pal) && (cvd2_sig_status.chroma_lock)) {
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                     cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT -1) {
                    cvd2_sig_status.cordic_data_sum -= (cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max);  //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        		    cvd2_sig_status.fc_more   	= (cvd2_sig_status.cordic_data_sum < FC_MORE_NTSCM_TO_NTSC443_MAX) && (cvd2_sig_status.cordic_data_sum > FC_MORE_NTSCM_TO_NTSC443_MIN) ? 1 : 0;
        		    if(cvd2_sig_status.fc_more)
        		    	cvd2_sig_status.detected_sd_state = SD_NTSC_443;
        		    else
        		    	cvd2_sig_status.detected_sd_state = SD_NTSC;

                    tvafe_cvd2_reset_cnt();
			    }
            } else {
                tvafe_cvd2_reset_cnt();
				if((!cvd2_sig_status.line625)&& (cvd2_sig_status.pal))
				    cvd2_sig_status.detected_sd_state = SD_PAL_M;      //NTSC_M''s chroma subcarries & PAL_M's one  are  approximate
				else
				    cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
            }
			break;
		case SD_PAL_60:
			if ((!cvd2_sig_status.line625) && (cvd2_sig_status.pal) && (cvd2_sig_status.chroma_lock)) {
                cvd2_sig_status.cordic_data_sum += data;
                cvd2_sig_status.stable_cnt++;

                if( cvd2_sig_status.cordic_data_min > data)
                     cvd2_sig_status.cordic_data_min = data;
                if( cvd2_sig_status.cordic_data_max < data)
                    cvd2_sig_status.cordic_data_max = data;

                if(cvd2_sig_status.stable_cnt > CORDIC_FILTER_COUNT -1) {
                    cvd2_sig_status.cordic_data_sum -= (cvd2_sig_status.cordic_data_min + cvd2_sig_status.cordic_data_max);  //get rid off the min & max value
                    cvd2_sig_status.cordic_data_sum = cvd2_sig_status.cordic_data_sum / (CORDIC_FILTER_COUNT - 2);
        		    cvd2_sig_status.fc_Less   	= (cvd2_sig_status.cordic_data_sum < FC_LESS_PAL_60_TO_PAL_M_MAX) && (cvd2_sig_status.cordic_data_sum > FC_LESS_PAL_60_TO_PAL_M_MIN) ? 1 : 0;
        		    if(cvd2_sig_status.fc_Less)
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_M;
        		    else
        		    	cvd2_sig_status.detected_sd_state = SD_PAL_60;

                    tvafe_cvd2_reset_cnt();
                }
			} else {
                tvafe_cvd2_reset_cnt();
                if ((cvd2_sig_status.chroma_lock)&&(cvd2_sig_status.pal))
                    cvd2_sig_status.detected_sd_state = SD_PAL_I;	//PAL_ i''s chroma subcarries & PAL_60's one  are  approximate
                else {
					if((!cvd2_sig_status.line625)&& (!cvd2_sig_status.pal))
					    cvd2_sig_status.detected_sd_state = SD_NTSC_443;      //NTSC_433''s chroma subcarries & PAL_60's one  are  approximate
					else
					    cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
			    }
            }
			break;
		case SD_SECAM:
            if ((cvd2_sig_status.line625) && (cvd2_sig_status.secam) && (cvd2_sig_status.chroma_lock))
				cvd2_sig_status.detected_sd_state = SD_SECAM;
			else {
                tvafe_cvd2_reset_cnt();
                if((cvd2_sig_status.line625) && (!cvd2_sig_status.secam) && (cvd2_sig_status.pal))
                    cvd2_sig_status.detected_sd_state = SD_PAL_I;
                else
					cvd2_sig_status.detected_sd_state = SD_HV_LOCK;
            }
			break;
        case SD_VCR :
            break;
		default:
			break;
	}

    return ;
}
// *****************************************************************************
// Function: configure video mode settings
//
//   Params: video format
//
//   Return: success/error
//
// *****************************************************************************
void tvafe_cvd2_write_mode_reg(enum tvin_sig_fmt_e fmt)
{
    switch (fmt) {
        case TVIN_SIG_FMT_CVBS_NTSC_M:
            ProgramNTSC();
            break;
        case TVIN_SIG_FMT_CVBS_NTSC_443:
            ProgramNTSC443();
            break;
        case TVIN_SIG_FMT_CVBS_PAL_I:
            ProgramPAL_I();
            break;
        case TVIN_SIG_FMT_CVBS_PAL_M:
            ProgramPALM();
            break;
        case TVIN_SIG_FMT_CVBS_SECAM:
            ProgramSECAM();
            break;
        case TVIN_SIG_FMT_CVBS_PAL_60:
            ProgramPAL60();
            break;
        case TVIN_SIG_FMT_CVBS_PAL_CN:
            ProgramPALCN();
            break;
        default :
            ProgramNTSC();
            break;
    }

    return;
}
static enum tvin_sig_fmt_e tvafe_cvd2_get_format(enum tvafe_cvd2_sd_state_e sd_state)
{
    enum tvin_sig_fmt_e fmt = 0;
    switch (sd_state) {
        case SD_NTSC:
            fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            break;
        case SD_NTSC_443:
            fmt = TVIN_SIG_FMT_CVBS_NTSC_443;
            break;
        case SD_PAL_I:
            fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            break;
        case SD_PAL_M:
            fmt = TVIN_SIG_FMT_CVBS_PAL_M;
            break;
        case SD_SECAM:
            fmt = TVIN_SIG_FMT_CVBS_SECAM;
            break;
        case SD_PAL_60:
            fmt = TVIN_SIG_FMT_CVBS_PAL_60;
            break;
        case SD_PAL_CN:
            fmt = TVIN_SIG_FMT_CVBS_PAL_CN;
            break;
        default :
            break;
    }

    return fmt;
}

// *****************************************************************************
// Function: configure video mode settings
//
//   Params: video format
//
//   Return: success/error
//
// *****************************************************************************
void tvafe_cvd2_video_mode_confiure(enum tvin_sig_fmt_e fmt)
{

    if (cvd2_sig_status.detected_sd_state != cvd2_sig_status.cur_sd_state) {
        if (cvd2_sig_status.detected_sd_state > SD_HV_LOCK) {
            tvafe_cvd2_get_format(cvd2_sig_status.detected_sd_state);
            tvafe_cvd2_write_mode_reg(fmt);
        } else {
            fmt = TVIN_SIG_FMT_MAX;
            tvafe_cvd2_reg_init();
        }

        //update cvd2 state
        cvd2_sig_status.cur_sd_state = cvd2_sig_status.detected_sd_state;
    }

    return;
}
// *****************************************************************************
// Function: CVD2 video AGC handler
//
//   Params: none
//
//   Return: success/error
//
// *****************************************************************************
int  tvafe_cvd2_video_agc_handler(struct tvafe_info_s *info)
{
    int ret = 0;
    unsigned char i, pga_mode;
    unsigned int reg_val,dgain_total=0,diff;

    //signal stable check
    if (info->param.status != TVIN_SIG_STATUS_STABLE)
        return ret;

    //if video unlock
    if (tvafe_cvd2_video_locked() == 0) {
        //reset agv counter
        cvd2_sig_status.agc.cnt = 0;
        cvd2_sig_status.agc.dgain = 0;
        return ret;
    } else {
        //counter overflow
        if (++cvd2_sig_status.agc.cnt >= 60)
            cvd2_sig_status.agc.cnt = 0;
    }

    for (i=0; i<4; i++) {
        //if dgain is 200, do not need agc
        reg_val = READ_CBUS_REG_BITS(CVD2_AGC_GAIN_STATUS_7_0, AGC_GAIN_7_0_BIT, AGC_GAIN_7_0_WID);
        reg_val |= READ_CBUS_REG_BITS(CVD2_AGC_GAIN_STATUS_11_8, AGC_GAIN_11_8_BIT, AGC_GAIN_11_8_WID)<<8;
        dgain_total += reg_val;
    }
    cvd2_sig_status.agc.dgain += dgain_total >> 4;

    //adjust adc gain with 10 sync interval
    if ((cvd2_sig_status.agc.cnt%10) != 0)
        return ret;

    //get average value
    cvd2_sig_status.agc.dgain /= 10;

    // Gain adjust
    diff = ABS(cvd2_sig_status.agc.dgain - DAGC_GAIN_STANDARD);
    if (diff > DAGC_GAIN_RANGE) {           // if large than 100, need adjust
        if (cvd2_sig_status.agc.cnt <= 20) { }        // Normal Range in the beginning 2 sec
        else if (diff > DAGC_GAIN_RANGE2)   // Big    Range
            diff = 20;
        else if (diff > DAGC_GAIN_RANGE1)   // Medium Range
            diff = 8;
        else if (diff > DAGC_GAIN_RANGE)    // Small  Range
            diff = 2;
        else                                // FineTune Range
            diff = 1;

        if (cvd2_sig_status.agc.dgain > DAGC_GAIN_STANDARD) {
            if (cvd2_sig_status.agc.again >= 0xFE) {
                pga_mode = READ_CBUS_REG_BITS(ADC_REG_06, PGAMODE_BIT, PGAMODE_WID);
                if (pga_mode == 0) {
                    WRITE_CBUS_REG_BITS(ADC_REG_06, 1, PGAMODE_BIT, PGAMODE_WID);
                    cvd2_sig_status.agc.again -= 3;
                } else
                    return ret;
            } else
                cvd2_sig_status.agc.again += diff;
        } else {
            if (cvd2_sig_status.agc.again <= 1) {
                pga_mode = READ_CBUS_REG_BITS(ADC_REG_06, PGAMODE_BIT, PGAMODE_WID);
                if (pga_mode == 1) {
                    WRITE_CBUS_REG_BITS(ADC_REG_06, 0, PGAMODE_BIT, PGAMODE_WID);
                    cvd2_sig_status.agc.again -= 3;
                } else
                    return ret;
            } else
                cvd2_sig_status.agc.again -= diff;
        }

        //adjust adc gain value
        WRITE_CBUS_REG_BITS(ADC_REG_07, cvd2_sig_status.agc.again, ADCGAINA_BIT, ADCGAINA_WID);
    }

    cvd2_sig_status.agc.again = 0;

    return ret;
}

// call by 10ms_timer at frontend side

void tvafe_cvd2_state_handler(struct tvafe_info_s *info)
{
    enum tvin_sig_fmt_e tmp_fmt = 0;
    //struct tvin_para_s info = {TVIN_SIG_FMT_NULL, TVIN_SIG_STATUS_NULL};

    //new_format = ...;
    tvafe_cvd2_search_video_mode();
    switch (event_cvd)
    {
        case TVIN_EVENT_NOSIG:
            if (cvd2_sig_status.detected_sd_state == SD_NO_SIG)
                cnt_cvd += 0x00000001;
            else
            {
                cnt_cvd &= 0xffffff00;
                event_cvd = TVIN_EVENT_UNSTABLE;
            }

            if ((cnt_cvd & 0x000000ff) > 8)
            {
                cnt_cvd &= 0xffffff00;
                if (event_cvd != TVIN_EVENT_NOSIG)  // newly confirmed TVIN_SIG_STATUS_NOSIG
                {
                    event_cvd = TVIN_EVENT_NOSIG;
                    info->param.fmt = TVIN_SIG_FMT_NULL;
                    info->param.status = TVIN_SIG_STATUS_NOSIG;
                }
            }
            break;
        case TVIN_EVENT_UNSTABLE:
            if (cvd2_sig_status.detected_sd_state == SD_NO_SIG)
                cnt_cvd += 0x00000001;
            else if (cvd2_sig_status.detected_sd_state == SD_UNLOCK ||
                     cvd2_sig_status.detected_sd_state == SD_NONSTD)
                cnt_cvd += 0x00000100;
            else
                cnt_cvd += 0x00010000;

            if ((cnt_cvd & 0x000000ff) > 4) {
                cnt_cvd &= 0xffffff00;
                if (event_cvd != TVIN_EVENT_NOSIG)  // newly confirmed TVIN_SIG_STATUS_UNSTABLE
                {
                    event_cvd = TVIN_EVENT_NOSIG;
                    info->param.fmt = TVIN_SIG_FMT_NULL;
                    info->param.status = TVIN_SIG_STATUS_NOSIG;
                }
            }
            if ((cnt_cvd & 0x0000ff00) > 8) {
                cnt_cvd &= 0xffff00ff;
                if (event_cvd != TVIN_EVENT_UNSTABLE)  // newly confirmed TVIN_SIG_STATUS_UNSTABLE
                {
                    event_cvd = TVIN_EVENT_UNSTABLE;
                    info->param.fmt = TVIN_SIG_FMT_NULL;
                    info->param.status = TVIN_SIG_STATUS_UNSTABLE;
                }
            }

            if ((cnt_cvd & 0x00ff0000) > 8) {
                cnt_cvd &= 0xff00ffff;
                event_cvd = TVIN_EVENT_STABLE;
                tmp_fmt = tvafe_cvd2_get_format(cvd2_sig_status.detected_sd_state);
                if (tmp_fmt == TVIN_SIG_FMT_NULL)
                    info->param.status = TVIN_SIG_STATUS_NOTSUP;
                else
                    info->param.status = TVIN_SIG_STATUS_STABLE;
                info->param.fmt = tmp_fmt;
            }
            break;
        case TVIN_EVENT_STABLE:
            if (cvd2_sig_status.detected_sd_state == SD_NO_SIG)
                cnt_cvd += 0x00000001;
            else if (cvd2_sig_status.detected_sd_state == SD_UNLOCK ||
                     cvd2_sig_status.detected_sd_state == SD_NONSTD)
                cnt_cvd += 0x00000100;
            else
                cnt_cvd += 0x00010000;

            if (((cnt_cvd & 0x000000ff) > 2) || ((cnt_cvd & 0x0000ff00) > 2)){
                cnt_cvd &= 0xffff0000;
                if (event_cvd != TVIN_EVENT_UNSTABLE)  // newly confirmed TVIN_SIG_STATUS_UNSTABLE
                {
                    event_cvd = TVIN_EVENT_UNSTABLE;
                }
            }

            if ((cnt_cvd & 0x00ff0000) > 16) {  //check format again and again
                cnt_cvd &= 0xff00ffff;
                tmp_fmt = tvafe_cvd2_get_format(cvd2_sig_status.detected_sd_state);
                if (info->param.fmt != tmp_fmt) {
                    if (tmp_fmt == TVIN_SIG_FMT_NULL)
                        info->param.status = TVIN_SIG_STATUS_NOTSUP;
                    else
                        info->param.status = TVIN_SIG_STATUS_STABLE;
                    info->param.fmt = tmp_fmt;
                }
            }
            break;
        default:
            break;
    }

}

// *****************************************************************************
// Function: cvd2 state init
//
//   Params: none
//
//   Return: none
//
// *****************************************************************************
void tvafe_cvd2_state_init(void)
{
    cvd2_sig_status.cur_sd_state = SD_NO_SIG;
    cvd2_sig_status.detected_sd_state = SD_NO_SIG;
    event_cvd = TVIN_EVENT_NOSIG;

}


