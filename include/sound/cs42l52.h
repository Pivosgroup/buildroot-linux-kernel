#ifndef __CS42L52_PDATA_H__
#define __CS42L52_PDATA_H__

/**
 * struct cs42l52_platform_data - platform-specific cs42l52 data
  @cs42l52_pwr_rst: reset function
  @spk_pwr_ctrl
 		00: speaker channel is on/off when SPK/HP pin low/high
		01: speaker channel is on/off when SPK/HP pin high/low
		10: speaker channel is always on.
		11: speaker channel is always off
  @hp_pwr_ctrl
		00: headphone channel is on/off when SPK/HP pin low/high
		01: headphone channel is on/off when SPK/HP pin high/low
		10: headphone channel is always on.
		11: headphone channel is always off
  @spk_mono
		0: disable
		1: enable
 */
struct cs42l52_platform_data {
    int (*is_hp_pluged)(void);
    int (*cs42l52_pwr_rst)(void);
    int (*cs42l52_power_down_io)(void);
    unsigned int spk_pwr_ctrl :2;
    unsigned int hp_pwr_ctrl :2;
    unsigned int spk_mono :1;
    unsigned short master_vol;
    unsigned short hp_vol;
    unsigned short spk_vol;
    unsigned short mic_ctrl;
    unsigned short beep_tone_ctrl;
    unsigned short tone_ctrl;    
};


#define cs42l52_setting(val) ((val) | 0x100)
#define cs42l52_get_setting(val, def_val) (((val) >= 0x100) ? ((val) & 0xff) : (def_val))

#endif

