#ifndef _RDA5880e_H_
#define _RDA5880e_H_


#include "RDA5880_adp.h"

/*
void  tun_rda5880e_if_open(void);
void  tun_rda5880e_rxon_open_if(void);
*/

void  tun_rda5880e_init(void);
void  tun_rda5880e_gain_initial(void);
void  tun_rda5880e_control_if(MS_U32 freq/*KHz*/, MS_U32 IF_freq/*KHz*/,MS_U32 bandwidth/*KHz*/);
void  tun_rda5880e_IntoPwrDwn_with_loop(void);
void  tun_rda5880e_IntoPwrDwn_without_loop(void);
int  tun_rda5880e_get_agc(void);



#endif/*_RDA5880e_H_*/

