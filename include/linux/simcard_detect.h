#ifndef __LINUX_SIMCARD_DETECT_H
#define __LINUX_SIMCARD_DETECT_H

#ifdef CONFIG_AMLOGIC_MODEM
#include <linux/aml_modem.h>
#endif

#define SIM_IN  (1)
#define SIM_OUT (0)

struct simcard_status{
    int code;	/* input key code */
    unsigned char *name;
    int chan;
    int value;	/* voltage/3.3v * 1023 */
    int tolerance;
};

struct adc_simdetect_platform_data{
    int (*modem_control)(int param);
    #ifdef CONFIG_AMLOGIC_MODEM
       struct aml_modem_pdata *p_modemdata ;
    #endif

    struct simcard_status *sim_status;
    int sim_status_num;
    int repeat_delay;
    int repeat_period;
};

#endif