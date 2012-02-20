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
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <mach/pinmux.h>
#include "aml_spi.h"

static void aml_dbg_spi_reg(struct amlogic_spi *amlogic_spi) 
{
    //printk("spi_flash_cmd:%x\n", amlogic_spi->master_regs->spi_flash_cmd);
//    printk("spi_flash_ctrl:%x\n", amlogic_spi->master_regs->spi_flash_ctrl);
//    printk("spi_flash_ctrl1:%x\n", amlogic_spi->master_regs->spi_flash_ctrl1);
//    printk("spi_flash_ctrl2:%x\n", amlogic_spi->master_regs->spi_flash_ctrl2);
//    printk("spi_flash_clock:%x\n", amlogic_spi->master_regs->spi_flash_clock);
//    printk("spi_flash_user:%x\n", amlogic_spi->master_regs->spi_flash_user);
//    printk("spi_flash_user1:%x\n", amlogic_spi->master_regs->spi_flash_user1);
//    printk("spi_flash_user2:%x\n", amlogic_spi->master_regs->spi_flash_user2);
//    printk("spi_flash_user3:%x\n", amlogic_spi->master_regs->spi_flash_user3);
//    printk("spi_flash_user4:%x\n", amlogic_spi->master_regs->spi_flash_user4);
//    printk("spi_flash_slave:%x\n", amlogic_spi->master_regs->spi_flash_slave);
//    printk("spi_flash_c0:%x\n", amlogic_spi->master_regs->spi_flash_c0[0]);
}

static inline void enable_cs(struct spi_device *spi)
{
    struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
    //spi_dbg;
	if (amlogic_spi->tgl_spi != NULL) { /* If last device toggled after mssg */
		if (amlogic_spi->tgl_spi != spi) { /* if last mssg on diff device */
			/* Deselect the last toggled device */
			gpio_set_value((unsigned )amlogic_spi->tgl_spi->controller_data, spi->mode & SPI_CS_HIGH ? 0 : 1);
		}
		amlogic_spi->tgl_spi = NULL;
	}
    gpio_set_value((unsigned )spi->controller_data, spi->mode & SPI_CS_HIGH ? 1 : 0);
}

static inline void disable_cs(struct spi_device *spi)
{
    struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
    //spi_dbg;
	if (amlogic_spi->tgl_spi == spi)
		amlogic_spi->tgl_spi = NULL;

	gpio_set_value((unsigned )spi->controller_data, spi->mode & SPI_CS_HIGH ? 0 : 1);
}

static void aml_spi_pinmux_hw_enable(struct amlogic_spi *amlogic_spi) 
{
    switch (amlogic_spi->io_pad_type) 
    {
        case SPI_A_GPIOE_0_7:
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, ((1<<18)|(1<<3)|(3<<5)));
    	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, ((1 <<25)|(1 << 27)));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1<<8));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1 << 23));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, ((1<<1)|(1<<16)|(1<<19)));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1 << 29));    	        
    	    }    	    
            break;
        case SPI_B_GPIOA_9_14:
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_12, (7<<2));
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, ((7<<4)|(7<<25)));
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_9, (7<<24));
    	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 <<21));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_12, (1<<0));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, (1<<23));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_9, (1<<29));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 22));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_12, (1<<1));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, ((1<<3)|(1<<24)));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_9, (1<<28));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 23)); 	        
    	    }    	    
            break;
            break;
        case SPI_B_GPIOB_0_7:
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (7<<16)|(7<<20));
            CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_11, (7<<6));
    	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 27));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (0x1f<<0));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 28));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (7<<5));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1<<31));
        	    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1 << 29));    	        
    	    }    	    
            break;
            break;
        default:
			printk("Warning couldn`t find any valid hw io pad!!!\n");
            break;                                    
    }
    asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
}

static void aml_spi_pinmux_hw_disable(struct amlogic_spi *amlogic_spi) 
{
    switch (amlogic_spi->io_pad_type) 
    {
        case SPI_A_GPIOE_0_7:
    	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, ((1 <<25)|(1 << 27)));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 22));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 23)); 	        
    	    }    	    
            break;
        case SPI_B_GPIOA_9_14:
    	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 <<21));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 22));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 23)); 	        
    	    }
            break;
        case SPI_B_GPIOB_0_7:
    	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 27));
    	    if(amlogic_spi->wp_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 28));    	        
    	    }
    	    if(amlogic_spi->hold_pin_enable)
    	    {
        	    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1 << 29));    	        
    	    } 
            break;
        default:
			printk("Warning couldn`t find any valid hw io pad!!!\n");
            break;                                    
    }
    asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
}

static void aml_spi_set_mode(struct amlogic_spi *amlogic_spi, unsigned mode) 
{    
    //spi_dbg;
	if(amlogic_spi->cur_mode == mode)
	    return;    
	amlogic_spi->cur_mode = mode;
	
	if(mode & SPI_CPHA)   //test ok
	{
	    amlogic_spi->master_regs->spi_flash_user |= (1<<7);
	}  
	else
	{
	    amlogic_spi->master_regs->spi_flash_user &= ~(1<<7);
	} 
	
	if(mode & SPI_CPOL) //test OK
	{
	    amlogic_spi->master_regs->spi_flash_user4 |= (1<<29); //idle edge of SPI_CK, high when it is idle, CPOL
	}  
	else
	{
	    amlogic_spi->master_regs->spi_flash_user4 &= ~(1<<29); //idle edge of SPI_CK, low when it is idle, CPOL
	} 
	
	if(mode & SPI_CS_HIGH) //test ok
	{
	    amlogic_spi->master_regs->spi_flash_user4 |= (1<<23); 
	}  
	else
	{
	    amlogic_spi->master_regs->spi_flash_user4 &= ~(1<<23); 
	} 
	
	if(mode & SPI_LSB_FIRST) 
	{
	    amlogic_spi->master_regs->spi_flash_ctrl |= (3<<25); 
	}  
	else
	{
	    amlogic_spi->master_regs->spi_flash_ctrl &= ~(3<<25); 
	} 	
   	     
	if(mode & SPI_3WIRE) 
	{
	    amlogic_spi->master_regs->spi_flash_user |= (1<<16); 
	}  
	else
	{
	    amlogic_spi->master_regs->spi_flash_user &= ~(1<<16); 
	}    	      
}

static void aml_spi_set_clk(struct amlogic_spi *amlogic_spi, unsigned spi_speed) 
{	
	unsigned spi_clock_divide, pre_scale_divide;
	unsigned int sys_clk_rate;
	struct clk *sys_clk;
	unsigned int reg_value = 0;

	if((amlogic_spi->cur_speed == spi_speed) && spi_speed)
	    return;

	sys_clk = clk_get_sys("clk81", NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	//sys_clk_rate = get_mpeg_clk();

    amlogic_spi->cur_speed = spi_speed;
	
	spi_clock_divide = sys_clk_rate/spi_speed;
	
	if(spi_clock_divide > 62)  //lower than 3MHz
	{
	    switch(spi_speed)
	    {
    	    case SPI_CLK_1M:
    	        pre_scale_divide = 3;
    	        spi_clock_divide = 32;
    	        break;
    	    case SPI_CLK_1M5:
    	        pre_scale_divide = 2;
    	        spi_clock_divide = 62;
    	        break;	        
    	    case SPI_CLK_2M:
    	        pre_scale_divide = 2;
    	        spi_clock_divide = 31;
    	        break;
    	    case SPI_CLK_2M5:
    	        pre_scale_divide = 2;
    	        spi_clock_divide = 38;
    	        break;
    	    default:     
    	        printk("enter %s here and spi_speed not match", __func__);  	        	    
    	        pre_scale_divide = spi_clock_divide/63;
    	        spi_clock_divide = 63;
    	        break;	    
        }         
	} 
	else
	{
	    pre_scale_divide = 0;    
	}       

    reg_value = (0<<31)|  //SPI clock use clock divider
                ((pre_scale_divide-1)<<18)|  //not pre-scale divider, need check
                (((spi_clock_divide-1))<<12)|  //clock counter for clock divider
                ((((spi_clock_divide>>1) - 1))<<6)| //clock high counter
                ((spi_clock_divide-1));   //clock low counter
    amlogic_spi->master_regs->spi_flash_clock = reg_value;//0x7d7bd;
    printk("enter %s here and spi_clock_divide:%d, pre_scale_divide:%d, spi_speed:%dKHz, reg_value:%x\n", __func__, spi_clock_divide, pre_scale_divide, spi_speed/1000, reg_value);
} 

static void aml_spi_set_platform_data(struct amlogic_spi *spi, 
										struct aml_spi_platform *plat)
{
	spi->spi_dev.dev.platform_data = plat;
	
    spi->master_no = plat->master_no;
    spi->io_pad_type = plat->io_pad_type;
    spi->wp_pin_enable = plat->wp_pin_enable;
    spi->hold_pin_enable = plat->hold_pin_enable;
    //spi->irq = plat->irq;
    spi->num_cs = plat->num_cs;
    //spi_dbg;
}

static void spi_hw_init(struct amlogic_spi	*amlogic_spi)
{
    //SW reset and setting master mode
    amlogic_spi->master_regs->spi_flash_slave |= (1<<31);
    amlogic_spi->master_regs->spi_flash_slave &= ~(1<<30);
    //bit[17]: disable AHB request
    amlogic_spi->master_regs->spi_flash_ctrl = 0x2A000;
    //for spi flash, not used
    amlogic_spi->master_regs->spi_flash_ctrl1 = 0xf0280100;
    //for common spi master, disable backward compatible to apollo
    amlogic_spi->master_regs->spi_flash_user = 0;
    //amlogic_spi->master_regs->spi_flash_user ~= ~(0<<2);    
}


static int spi_add_dev(struct amlogic_spi *amlogic_spi, struct spi_master	*master)
{
	amlogic_spi->spi_dev.master = master;
	device_initialize(&amlogic_spi->spi_dev.dev);
	amlogic_spi->spi_dev.dev.parent = &amlogic_spi->master->dev;
	amlogic_spi->spi_dev.dev.bus = &spi_bus_type;

	strcpy((char *)amlogic_spi->spi_dev.modalias, SPI_DEV_NAME);
	dev_set_name(&amlogic_spi->spi_dev.dev, "%s:%d", "m1spi", master->bus_num);
	return device_add(&amlogic_spi->spi_dev.dev);
}

//setting clock and pinmux here
static  int amlogic_spi_setup(struct spi_device	*spi)
{/*
    struct amlogic_spi	*amlogic_spi;
    amlogic_spi = spi_master_get_devdata(spi->master);
    
	if((amlogic_spi->cur_speed != spi->max_speed_hz) && spi->max_speed_hz)
	{
	    aml_spi_set_clk(amlogic_spi, spi->max_speed_hz);	    
	}
	if(amlogic_spi->cur_mode != spi->mode)
	{
	    aml_spi_set_mode(amlogic_spi, spi->mode);
	}
	*/
    return 0;
}
/*
static void amlogic_spi_chipselect(struct spi_device *spi, int value)
{ //need to do
	
	struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
	struct fsl_spi_platform_data *pdata = spi->dev.parent->platform_data;
	bool pol = spi->mode & SPI_CS_HIGH;
	struct spi_amlogic_cs	*cs = spi->controller_state;

	if (value == BITBANG_CS_INACTIVE) {
		if (pdata->cs_control)
			pdata->cs_control(spi, !pol);
	}

	if (value == BITBANG_CS_ACTIVE) {
		amlogic_spi->rx_shift = cs->rx_shift;
		amlogic_spi->tx_shift = cs->tx_shift;
		amlogic_spi->get_rx = cs->get_rx;
		amlogic_spi->get_tx = cs->get_tx;

		amlogic_spi_change_mode(spi);

		if (pdata->cs_control)
			pdata->cs_control(spi, pol);
	}
	
}
*/
static  void amlogic_spi_cleanup(struct spi_device	*spi)
{
	if (spi->modalias)
		kfree(spi->modalias);
}

static int spi_receive_cycle(struct spi_transfer *t, struct spi_device *spi)
{
    struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
	int spi_rx_len = t->len;
	unsigned *data_buf = (unsigned *)t->rx_buf;
	int i, read_len;
	unsigned temp_buf[8], data_offset = 0;
	
	//spi_dbg;
    amlogic_spi->master_regs->spi_flash_user &= ~(1<<27);
    amlogic_spi->master_regs->spi_flash_user &= ~(0x7<<29);
    amlogic_spi->master_regs->spi_flash_user |= (1<<28);
    
   	amlogic_spi->master_regs->spi_flash_user2 = 0;   
   	amlogic_spi->master_regs->spi_flash_user3 = 0;  

	while (spi_rx_len > 0 )
	{
		
        read_len = (spi_rx_len > 33)? 32:spi_rx_len;
	    amlogic_spi->master_regs->spi_flash_user1 = ((0<<26)|
                	                                (0<<17) |
                	                                (((read_len<<3)-1)<<8)  |    
                	                                 0);   
        amlogic_spi->master_regs->spi_flash_slave &= ~(1<<4); //clear transtion state bit             	                                 
                 		/* Slave Select */
		enable_cs(spi);
   	                                          
    	amlogic_spi->master_regs->spi_flash_cmd |= (1<<18); //USER defined command	
        //printk("rx_delay:%d\n", read_len * 16000000 / amlogic_spi->cur_speed);
        //udelay(read_len * 32000000 / amlogic_spi->cur_speed);
        while((amlogic_spi->master_regs->spi_flash_slave & (1<<4)) == 0);
        //udelay(30);
		for (i=0; i< 8; i++) {
			if (spi_rx_len <= 0)
				break;
            *(temp_buf+i) = amlogic_spi->master_regs->spi_flash_c0[i];	
			//printk("enter %s here and spi_flash_c0[%d]:%x\n", __func__, i, amlogic_spi->master_regs->spi_flash_c0[i]);
			spi_rx_len -= 4;
		}
		memcpy((unsigned char *)data_buf+data_offset, (unsigned char *)temp_buf, read_len);
		
		data_offset += read_len;
	}
	
	return 0;
}

static int spi_transmit_cycle(struct spi_transfer *t, struct spi_device *spi)
{			
    struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
	unsigned *data_buf = (unsigned *)t->tx_buf;
	int i, tx_len, len;
	
	//spi_dbg;
	//printk("enter %d here and t->tx_buf:%x, and  data_buf:%x, data_buf[0]:%x\n", __LINE__, t->tx_buf, data_buf, *data_buf );  	
	
	amlogic_spi->master_regs->spi_flash_user |= (1<<27);
    amlogic_spi->master_regs->spi_flash_user &= ~(0xf<<28);
    amlogic_spi->master_regs->spi_flash_user &= ~(1<<2);
    
   	amlogic_spi->master_regs->spi_flash_user2 = 0;   
   	amlogic_spi->master_regs->spi_flash_user3 = 0;  
   	len = t->len;

	do 
	{
        
        tx_len = (len > 33)? 255:((len<<3)-1);
           
	    amlogic_spi->master_regs->spi_flash_user1 = ((0<<26)|
                	                                (tx_len<<17) |
                	                                (0<<8)  |    
                	                                 0);   
              	                                           	                                 
               	                                 
		for (i=0; i<8; i++) 
		{
			if (len <= 0)
				break;
			//printk("enter %d here and data_buf[%d]:%x\n", __LINE__, i, *(data_buf+i));  	
		    amlogic_spi->master_regs->spi_flash_c0[i] = *(data_buf+i);		
			//printk("enter %d here and spi_flash_c0[%d]:%x\n", __LINE__, i, amlogic_spi->master_regs->spi_flash_c0[i], *(data_buf+i));  	
			len -= 4;
		}
        aml_dbg_spi_reg(amlogic_spi);
        
        amlogic_spi->master_regs->spi_flash_slave &= ~(1<<4); //clear transtion state bit
        		/* Slave Select */
		enable_cs(spi);
		amlogic_spi->master_regs->spi_flash_cmd |= (1<<18); //USER defined command
        //udelay(tx_len * 3000000 / amlogic_spi->cur_speed);
        while((amlogic_spi->master_regs->spi_flash_slave & (1<<4)) == 0);
        //spi_dbg;
        //printk("tx_delay:%d\n", tx_len * 2000000 / amlogic_spi->cur_speed);
        //udelay(20);
	}while (len > 0 );

	return 0;
}

static int amlogic_spi_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	int ret;
#if 0	
	unsigned int len = t->len;
	u8 bits_per_word;
	

	bits_per_word = spi->bits_per_word;
	if (t->bits_per_word)
		bits_per_word = t->bits_per_word;

	if (bits_per_word > 8) {
		/* invalid length? */
		if (len & 1)
			return -EINVAL;
		len /= 2;
	}
	if (bits_per_word > 16) {
		/* invalid length? */
		if (len & 1)
			return -EINVAL;
		len /= 2;
	}
#endif
	//INIT_COMPLETION(amlogic_spi->done);

    if(t->tx_buf != NULL)
    {
        ret = spi_transmit_cycle(t, spi);
    }
    
    if(t->rx_buf != NULL)
    {
        ret = spi_receive_cycle(t, spi);
    }
    
	if (ret)
		return ret;

	//wait_for_completion(&amlogic_spi->done);

	/* disable rx ints */
	//amlogic_spi_write_reg(&amlogic_spi->base->mask, 0);

	return 0;//amlogic_spi->count;
}

static void amlogic_spi_handle_one_msg(struct amlogic_spi *amlogic_spi, struct spi_message *m)
{
	struct spi_device *spi = m->spi;
	struct spi_transfer *t;
	int status = 0, cs_toggle = 0;
	
	//spi_dbg;
    if((amlogic_spi->cur_speed != spi->max_speed_hz) && spi->max_speed_hz)
	{
	    aml_spi_set_clk(amlogic_spi, spi->max_speed_hz);	    
	}
	if(amlogic_spi->cur_mode != spi->mode)
	{
	    aml_spi_set_mode(amlogic_spi, spi->mode);
	}

    aml_spi_pinmux_hw_enable(amlogic_spi);
    amlogic_spi->master_regs->spi_flash_ctrl &= ~(1<<17); 
	list_for_each_entry(t, &m->transfers, transfer_list) {
	    #if 0  //need check
	    if (t->bits_per_word || t->speed_hz) {
			/* Don't allow changes if CS is active */
			status = -EINVAL;

			if (cs_change)
				status = amlogic_spi_setup_transfer(spi, t);
			if (status < 0)
				break;
		}
		#endif
		
    	if((amlogic_spi->cur_speed != t->speed_hz) && t->speed_hz)
    	{
    	    aml_spi_set_clk(amlogic_spi, t->speed_hz);	    
    	}

  
		if (t->len)
			status = amlogic_spi_bufs(spi, t);
		if (status) {
			status = -EMSGSIZE;
			break;
		}
		m->actual_length += t->len;

		if (t->delay_usecs)
			udelay(t->delay_usecs);

		if (t->cs_change) {
			/* Hint that the next mssg is gonna be
			   for the same device */
			if (list_is_last(&t->transfer_list,
						&m->transfers))
				cs_toggle = 1;
			else
				disable_cs(spi);
		}
	}
	//spi_dbg;
	if (!cs_toggle || status)
		disable_cs(spi);
	else
		amlogic_spi->tgl_spi = spi;	
	//spi_dbg;	
    amlogic_spi->master_regs->spi_flash_ctrl |= (1<<17); 
    aml_spi_pinmux_hw_disable(amlogic_spi);
	m->status = status;
	if(m->context)
	    m->complete(m->context);

//	if (status || !cs_change) {
//		ndelay(nsecs);
//		amlogic_spi_chipselect(spi, BITBANG_CS_INACTIVE);
//	}

	//amlogic_spi_setup_transfer(spi, NULL);
}

static int amlogic_spi_transfer(struct spi_device *spi,
				struct spi_message *m)
{
	struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
	unsigned long flags;

	m->actual_length = 0;
	m->status = -EINPROGRESS;

	spin_lock_irqsave(&amlogic_spi->lock, flags);
	list_add_tail(&m->queue, &amlogic_spi->msg_queue);
	queue_work(amlogic_spi->workqueue, &amlogic_spi->work);
	spin_unlock_irqrestore(&amlogic_spi->lock, flags);

	return 0;
}

static void amlogic_spi_work(struct work_struct *work)
{
	struct amlogic_spi *amlogic_spi = container_of(work, struct amlogic_spi,
						       work);
    unsigned long flags;

	spin_lock_irqsave(&amlogic_spi->lock, flags);
	while (!list_empty(&amlogic_spi->msg_queue)) {
		struct spi_message *m = container_of(amlogic_spi->msg_queue.next,
						   struct spi_message, queue);

		list_del_init(&m->queue);
		spin_unlock_irqrestore(&amlogic_spi->lock, flags);

		amlogic_spi_handle_one_msg(amlogic_spi, m);

		spin_lock_irqsave(&amlogic_spi->lock, flags);
	}
	spin_unlock_irqrestore(&amlogic_spi->lock, flags);
}

static int amlogic_spi_probe(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct amlogic_spi	*amlogic_spi;
	struct resource		*res;
	int			status = 0;

    spi_dbg;
    
	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "platform_data missing!\n");
		return -ENODEV;
	}
	spi_dbg;
	master = spi_alloc_master(&pdev->dev, sizeof *amlogic_spi);
	if (master == NULL) {
		dev_dbg(&pdev->dev, "Unable to allocate SPI Master\n");
		return -ENOMEM;
	}	
    spi_dbg;
    master->bus_num = (pdev->id != -1)? pdev->id:1;

	master->setup = amlogic_spi_setup;
	master->transfer = amlogic_spi_transfer;
	master->cleanup = amlogic_spi_cleanup;

	dev_set_drvdata(&pdev->dev, master);

	amlogic_spi = spi_master_get_devdata(master);
	amlogic_spi->master = master;

	aml_spi_set_platform_data(amlogic_spi, pdev->dev.platform_data);
	//init_completion(&amlogic_spi->done);
	
    master->num_chipselect = amlogic_spi->num_cs;
	spi_dbg;
	res = platform_get_resource(pdev, IORESOURCE_MEM, amlogic_spi->master_no);
	if (res == NULL) {
	    status = -ENOMEM;
	    dev_err(&pdev->dev, "Unable to get SPI MEM resource!\n");
        goto err1;
	}		
    amlogic_spi->master_regs = (struct aml_spi_reg_master __iomem*)(res->start);
#if 0	//remove irq 
	/* Register for SPI Interrupt */
	status = request_irq(amlogic_spi->irq, amlogic_spi_irq_handle,
			  0, "aml_spi", amlogic_spi);	
	if (status != 0)
	{
	    dev_dbg(&pdev->dev, "spi request irq failed and status:%d\n", status);
	    goto err1;
	}
#endif	  
	amlogic_spi->workqueue = create_singlethread_workqueue(
		dev_name(master->dev.parent));
	if (amlogic_spi->workqueue == NULL) {
		status = -EBUSY;
		goto err1;
	}	
	  		  
	spin_lock_init(&amlogic_spi->lock);
	INIT_WORK(&amlogic_spi->work, amlogic_spi_work);
	INIT_LIST_HEAD(&amlogic_spi->msg_queue);
	
    spi_dbg;
	
	status = spi_register_master(master);
	if (status < 0)
    {
        dev_dbg(&pdev->dev, "spi register failed and status:%d\n", status);
		goto err0;
	}
	
	spi_hw_init(amlogic_spi);
    spi_dbg;
	status = spi_add_dev(amlogic_spi, master);
	spi_dbg;
	printk("aml spi init ok and status:%d \n", status);
	
	return status;    
err0:
   destroy_workqueue(amlogic_spi->workqueue);     
err1:
	spi_master_put(master);
	return status;
}

static int amlogic_spi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct amlogic_spi	*amlogic_spi;

	master = dev_get_drvdata(&pdev->dev);
	amlogic_spi = spi_master_get_devdata(master);

	spi_unregister_master(master);

	return 0;
}

static struct platform_driver amlogic_spi_driver = { 
	.probe = amlogic_spi_probe, 
	.remove = amlogic_spi_remove, 
	.driver =
	    {
			.name = "aml_spi", 
			.owner = THIS_MODULE, 
		}, 
};

static int __init amlogic_spi_init(void) 
{	
	return platform_driver_register(&amlogic_spi_driver);
}

static void __exit amlogic_spi_exit(void) 
{	
	platform_driver_unregister(&amlogic_spi_driver);
} 

//subsys_initcall(amlogic_spi_init);
//arch_initcall(amlogic_spi_init);
module_init(amlogic_spi_init);
module_exit(amlogic_spi_exit);

MODULE_DESCRIPTION("Amlogic Spi driver");
MODULE_LICENSE("GPL");
