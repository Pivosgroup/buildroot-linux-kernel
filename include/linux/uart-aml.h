/* 
* include/linux/uart-aml.h
*/
#ifndef AML_MACH_UART
#define AML_MACH_UART

#include <mach/am_regs.h>

#define UART_A     0
#define UART_B     1

struct aml_uart_platform{
	int uart_line[2];
};


#endif

