#ifndef _TVAFE_GENERAL_H
#define _TVAFE_GENERAL_H

#include "tvafe.h"

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************

// ***************************************************************************
// *** global parameters **********
// ***************************************************************************
extern enum tvafe_adc_pin_e         tvafe_default_cvbs_out;

// *****************************************************************************
// ******** function claims ********
// *****************************************************************************
enum tvafe_adc_pin_e tvafe_get_free_pga_pin(struct tvafe_pin_mux_s *pinmux);
int tvafe_source_muxing(struct tvafe_info_s *info);
void tvafe_vga_set_edid(struct tvafe_vga_edid_s *edid);
void tvafe_vga_get_edid(struct tvafe_vga_edid_s *edid);
void tvafe_set_fmt(struct tvin_para_s *para);
void tvafe_set_cal_value(struct tvafe_adc_cal_s *para);
enum tvin_scan_mode_e tvafe_top_get_scan_mode(void);

#endif  // _TVAFE_GENERAL_H
