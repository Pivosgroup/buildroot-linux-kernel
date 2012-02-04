#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <linux/spi/spi.h>
#include <mach/spi.h>
#include <mach/pinmux.h>

#define AML_SPI_DBG 0

#ifdef AML_SPI_DBG
#define spi_dbg     printk("enter %s line:%d \n", __func__, __LINE__)
#else
#define spi_dbg
#endif


struct aml_spi_reg_master {
	volatile unsigned int spi_flash_cmd;
	volatile unsigned int spi_flash_addr;
	volatile unsigned int spi_flash_ctrl;
	volatile unsigned int spi_flash_ctrl1;
	volatile unsigned int spi_flash_status;
	volatile unsigned int spi_flash_ctrl2;
	volatile unsigned int spi_flash_clock;
	volatile unsigned int spi_flash_user;
	volatile unsigned int spi_flash_user1;
	volatile unsigned int spi_flash_user2;	
	volatile unsigned int spi_flash_user3;
	volatile unsigned int spi_flash_user4;
	volatile unsigned int spi_flash_slave;
	volatile unsigned int spi_flash_slave1;
	volatile unsigned int spi_flash_slave2;
	volatile unsigned int spi_flash_slave3;
	volatile unsigned int spi_flash_c0[8];
	volatile unsigned int spi_flash_b8[8];				
};

struct amlogic_spi {
	spinlock_t		    lock;
	struct list_head	msg_queue;
	//struct completion   done;
	struct workqueue_struct *workqueue;
	struct work_struct work;	
	int			        irq;
	
		
	struct spi_master	*master;
	struct spi_device	spi_dev;
	struct spi_device   *tgl_spi;
	
	
	struct  aml_spi_reg_master __iomem*  master_regs;
	

	unsigned        master_no;

	unsigned        io_pad_type;
	unsigned char   wp_pin_enable;
	unsigned char   hold_pin_enable;
	unsigned        num_cs;
	
	unsigned        cur_mode;
	unsigned        cur_bpw;
	unsigned        cur_speed;

};


