/*
 * aml_pio_i2c.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <asm/io.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c-aml.h>
#include "aml_sw_i2c.h"

static void aml_sw_bit_setscl(void *data, int val)
{
	struct aml_sw_i2c* i2c = (struct aml_sw_i2c*)data;
	unsigned int scl;
	unsigned int oe;

	/*printk("c%d ",val);*/

	//set output
	oe = readl(i2c->sw_pins->scl_oe);
	oe &= ~(1<<i2c->sw_pins->scl_bit);
	writel(oe, i2c->sw_pins->scl_oe);

	scl = readl(i2c->sw_pins->scl_reg_out);
	if(val)
		scl |= (1<<(i2c->sw_pins->scl_bit));
	else
		scl &= ~(1<<(i2c->sw_pins->scl_bit));
	writel(scl, i2c->sw_pins->scl_reg_out);
}

static void aml_sw_bit_setsda(void *data, int val)
{
	struct aml_sw_i2c* i2c = (struct aml_sw_i2c*)data;
	unsigned int sda;
	unsigned int oe;

	/*printk("d%d ",val);*/
	
	if(val){
		//set input
		oe = readl(i2c->sw_pins->sda_oe);
		oe |=  (1<<(i2c->sw_pins->sda_bit));
		writel(oe, i2c->sw_pins->sda_oe);
	} else {
		//set output
		oe = readl(i2c->sw_pins->sda_oe);
		oe &= ~(1<<i2c->sw_pins->sda_bit);
		writel(oe, i2c->sw_pins->sda_oe);
	
		sda = readl(i2c->sw_pins->sda_reg_out);
		if(val)
			sda |= (1<<(i2c->sw_pins->sda_bit));
		else
			sda &= ~(1<<(i2c->sw_pins->sda_bit));
		writel(sda, i2c->sw_pins->sda_reg_out);
	}
}

static int aml_sw_bit_getsda(void *data)
{
	struct aml_sw_i2c* i2c = (struct aml_sw_i2c*)data;
	unsigned int sda;
	unsigned int oe;

	//set input
/*	oe = readl(i2c->sw_pins->sda_oe);
	oe |=  (1<<(i2c->sw_pins->sda_bit));
	writel(oe, i2c->sw_pins->sda_oe);
*/	
	sda = readl(i2c->sw_pins->sda_reg_in) & (1<<(i2c->sw_pins->sda_bit));

	/*printk("g%d ",sda? 1 : 0);*/
	
	return sda? 1 : 0;
}

static int aml_sw_i2c_setup(void *data)
{
	struct aml_sw_i2c_pins* gdata = (struct aml_sw_i2c_pins*)data;
	unsigned int oe;
			
	/* set scl output */
	oe = readl(gdata->scl_oe);
	oe &= ~(1<<(gdata->scl_bit));
	writel(oe, gdata->scl_oe);

	/*OD, set sda output */
/*	oe = readl(gdata->sda_oe);
	oe &= ~(1<<(gdata->sda_bit));
	writel(oe, gdata->sda_oe);
*/
	//NOT OD, set sda input
	oe = readl(gdata->sda_oe);
	oe |=  (1<<(gdata->sda_bit));
	writel(oe, gdata->sda_oe);

	return 0;
}	

static struct aml_sw_i2c aml_sw_i2cd = {
	.adapter	= {
		.name			= "aml-sw-i2c",
		.owner			= THIS_MODULE,
		.id				= I2C_SW_B_GPIO,
		.retries			= 2,
		.class			= I2C_CLASS_HWMON,
	},
	.algo_data = {
		.setsda			= aml_sw_bit_setsda,
		.setscl			= aml_sw_bit_setscl,
		.getsda			= aml_sw_bit_getsda,
		.pre_xfer			= NULL,
		.post_xfer		= NULL,
		.getscl			= NULL,
		.udelay			= 30/*30*/,
		.timeout			= 100,
	},
};

/***************i2c class****************/

static ssize_t show_i2c_info(struct class *class, char *buf)
{
	struct aml_sw_i2c *i2c = &aml_sw_i2cd;
	
	printk( "scl_reg_out 0x%x\n", i2c->sw_pins->scl_reg_out);
	printk( "scl_reg_in  0x%x\n", i2c->sw_pins->scl_reg_in);
	printk( "scl_bit  %d\n", i2c->sw_pins->scl_bit);
	printk( "scl_oe  0x%x\n", i2c->sw_pins->scl_oe);
	
	printk( "sda_reg_out 0x%x\n", i2c->sw_pins->sda_reg_out);
	printk( "sda_reg_in  0x%x\n", i2c->sw_pins->sda_reg_in);
	printk( "sda_bit  %d\n", i2c->sw_pins->sda_bit);
	printk( "sda_oe  0x%x\n", i2c->sw_pins->sda_oe);

	return 0;
}

static struct class_attribute i2c_class_attrs[] = {
    __ATTR(info,       S_IRUGO | S_IWUSR, show_i2c_info,    NULL),
    __ATTR_NULL
};

static struct class sw_i2c_class = {
    .name = "sw-i2c",
    .class_attrs = i2c_class_attrs,
};

static int aml_sw_i2c_probe(struct platform_device *pdev)
{
	struct aml_sw_i2c_platform *plat = pdev->dev.platform_data;
	int ret;
	struct aml_sw_i2c *drv_data = &aml_sw_i2cd;

	drv_data->sw_pins = &(plat->sw_pins);
	
	/*private data = gpio pins, set sda/scl will use it*/
	drv_data->algo_data.data = &aml_sw_i2cd;
	drv_data->adapter.algo_data = &(drv_data->algo_data);
	
	/*gpio config*/
	aml_sw_i2c_setup(drv_data->sw_pins);

	/*add to i2c bit algos*/
	if ((ret = i2c_bit_add_numbered_bus(&(drv_data->adapter)) != 0)) 
	{
		printk(KERN_ERR "\033[0;40;36mERROR: Could not add %s to i2c bit" 
							"algos\033[0m\r\n", drv_data->adapter.name);
		return ret;
	}

   	ret = class_register(&sw_i2c_class);
	if(ret){
		printk(" class register sw_i2c_class fail!\n");
	}
	
	printk("aml gpio i2c bus initialized\r\n");
	return 0;
}

static int aml_sw_i2c_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver aml_sw_i2c_driver = { 
	.probe = aml_sw_i2c_probe, 
	.remove = aml_sw_i2c_remove, 
	.driver = {
			.name = "aml-sw-i2c",						
			.owner = THIS_MODULE,						
	}, 
};

static int __init aml_sw_i2c_init(void) 
{
	return platform_driver_register(&aml_sw_i2c_driver);
}

static void __exit aml_sw_i2c_exit(void) 
{
	platform_driver_unregister(&aml_sw_i2c_driver);
} 

module_init(aml_sw_i2c_init);
module_exit(aml_sw_i2c_exit);

MODULE_AUTHOR("AMLOGIC");
MODULE_DESCRIPTION("I2C software gpio driver for amlogic");
MODULE_LICENSE("GPL");

