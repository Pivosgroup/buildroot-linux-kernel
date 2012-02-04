#ifndef _AR1520_H_
#define _AR1520_H_
struct ar1520_platform_data{
	void (*power_on)(void);
	void (*power_off)(void);
};


#endif

