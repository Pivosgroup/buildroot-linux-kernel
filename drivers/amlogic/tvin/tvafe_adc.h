/*******************************************************************
*  Copyright C 2010 by Amlogic, Inc. All Rights Reserved.
*  File name: TVAFE_ADC.h
*  Description: IO function, structure, enum, used in TVIN AFE sub-module processing
*******************************************************************/
#ifndef _TVAFE_ADC_H
#define _TVAFE_ADC_H

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************

// ***************************************************************************
// *** structure definitions *********************************************
// ***************************************************************************
typedef struct tvafe_adc_status_s {
    unsigned char        no_sig;
    unsigned short       h_offset;
    unsigned short       v_offset;
    struct tvin_format_s prop;
} tvafe_adc_status_t;


// *****************************************************************************
// ******** GLOBAL FUNCTION CLAIM ********
// *****************************************************************************
void tvafe_adc_set_param(struct tvafe_info_s *info);
void tvafe_adc_state_handler(struct tvafe_info_s *info);
void tvafe_set_vga_default(void);
void tvafe_set_comp_default(void);
void tvafe_adc_configure(enum tvin_sig_fmt_e fmt);
void tvafe_adc_state_init(void);

void tvafe_vga_set_border_cfg(struct tvafe_vga_boarder_cfg_s *bd);
void tvafe_vga_set_ap_cfg(struct tvafe_vga_ap_cfg_s *ap);
void tvafe_vga_get_border_info(struct tvafe_vga_boarder_info_s *bd);
void tvafe_vga_get_ap_info(struct tvafe_vga_ap_info_s *pixel_info);

#endif // _TVAFE_ADC_H

