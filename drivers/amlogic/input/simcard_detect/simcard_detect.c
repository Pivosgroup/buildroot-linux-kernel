/*
 * linux/drivers/input/adc_kbd/adc_keypad.c
 *
 * ADC sim card detect Driver
 *
 * Copyright (C) 2010 Amlogic Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * author :   alex.deng
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/saradc.h>
#include <linux/simcard_detect.h>
#include <linux/aml_modem.h>

struct sim_detect{
       int (*modem_control)(int param);

	struct timer_list timer;
	unsigned int cur_simcard_status;
	int config_major;
	char config_name[20];
	struct class *config_class;
	struct device *config_dev;
	int chan[SARADC_CHAN_NUM];
	int chan_num;
	struct simcard_status *sim_status;
	int sim_status_num;
};

static struct sim_detect *gp_sim_detect=NULL;

static int in_count = 0 ;
static int out_count = 0;
static int start_count_timer = 0 ;

static int simcard_search(struct sim_detect *p_sd)
{
	struct simcard_status *p_sim_status;
	int value, i, j;
	
	for (i=0; i<p_sd->chan_num; i++) {
		value = get_adc_sample(p_sd->chan[i]);
		if (value < 0) {
			continue;
		}
		p_sim_status = p_sd->sim_status;
	 	for (j=0; j<p_sd->sim_status_num; j++) {
			if ((p_sim_status->chan == p_sd->chan[i])
			&& (value >= p_sim_status->value - p_sim_status->tolerance)
			&& (value <= p_sim_status->value + p_sim_status->tolerance)) {
				return p_sim_status->code;
			}
                    
			p_sim_status++;
		}
	}
	
	return SIM_OUT;
}

static void simcard_detect_work(struct sim_detect *p_sd)
{
#if 0
       if(SIM_IN == p_sd->cur_simcard_status){
            if(in_count >= 5){
                    in_count = 0 ;
                    start_count_timer = 0 ;
                    return ;
            }
       }
       else if(SIM_OUT == p_sd->cur_simcard_status){
            if( out_count >= 5 ){
                    out_count = 0;
                    start_count_timer = 0 ;
                    return ;
            }
       }
#endif       
	int code = simcard_search(p_sd);
	if(SIM_IN == p_sd->cur_simcard_status){
            if (SIM_OUT == code) {
                    printk("simcard pulled out.\n");
                    p_sd->cur_simcard_status = SIM_OUT ;
                    start_count_timer = 1 ; 
            }
            else if(start_count_timer > 0){
                    in_count++ ;
            }
	}
       else{
            if(SIM_IN == code){
                    printk("simcard pushed in.\n");
                    p_sd->cur_simcard_status = SIM_IN ;
                    start_count_timer = 1 ;
            }
            else if(start_count_timer > 0){
                   out_count++ ;
            }
       }
}

void simdetect_timer_sr(unsigned long data)
{
    struct sim_detect *p_sd=(struct sim_detect *)data;

    simcard_detect_work(p_sd);

    mod_timer(&p_sd->timer,jiffies+msecs_to_jiffies(200));
}

static int
adcsimdetect_config_open(struct inode *inode, struct file *file)
{
    file->private_data = gp_sim_detect;
    return 0;
}

static int
adcsimdetect_config_release(struct inode *inode, struct file *file)
{
    file->private_data=NULL;
    return 0;
}

static const struct file_operations simdetect_fops = {
    .owner      = THIS_MODULE,
    .open       = adcsimdetect_config_open,
    .ioctl      = NULL,
    .release    = adcsimdetect_config_release,
};

static int register_simdetect_dev(struct sim_detect  *p_sd, int dev_id)
{
    int ret=0;
    strcpy(p_sd->config_name,"am_adc_simdetect");
    p_sd->config_name[strlen(p_sd->config_name)] = (char)('0' + dev_id);
    ret=register_chrdev(0, p_sd->config_name, &simdetect_fops);
    if(ret<=0)
    {
        printk("register char device error\r\n");
        return  ret ;
    }
    p_sd->config_major=ret;
    printk("adc simdetect major:%d\r\n",ret);
    p_sd->config_class=class_create(THIS_MODULE,p_sd->config_name);
    p_sd->config_dev=device_create(p_sd->config_class,	NULL,
    		MKDEV(p_sd->config_major,0),NULL,p_sd->config_name);
    return ret;
}

static ssize_t simstatus_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gp_sim_detect->cur_simcard_status ); ;
}

static struct class_attribute simstatus_class_attrs[] = {
    __ATTR_RO(simstatus),                   
    __ATTR_NULL
};
static struct class simstatus_class = {
    .name = "simstatus",
    .class_attrs = simstatus_class_attrs,
};

static int __init simdetect_probe(struct platform_device *pdev)
{
    struct sim_detect *p_sim_detect;
    int i, j ;
    struct adc_simdetect_platform_data *pdata = pdev->dev.platform_data;

    if (!pdata) {
        dev_err(&pdev->dev, "platform data is required!\n");
        return -EINVAL;
    }
   
    p_sim_detect = kzalloc(sizeof(struct sim_detect), GFP_KERNEL);
    if (!p_sim_detect ) {
        return -ENOMEM;
    }
    gp_sim_detect = p_sim_detect;

    platform_set_drvdata(pdev, p_sim_detect);
    p_sim_detect->cur_simcard_status = SIM_OUT;
     
    setup_timer(&p_sim_detect->timer, simdetect_timer_sr, p_sim_detect) ;
    mod_timer(&p_sim_detect->timer, jiffies+msecs_to_jiffies(100));

        
    p_sim_detect->sim_status = pdata->sim_status;
    p_sim_detect->sim_status_num = pdata->sim_status_num;
    if (pdata->modem_control){
    	p_sim_detect->modem_control = pdata->modem_control;
    }

    struct simcard_status *sc_s = pdata->sim_status;
    int new_chan_flag;
    p_sim_detect->chan_num = 0;
    for (i=0; i<p_sim_detect->sim_status_num; i++) {
        /* search the chan */
        new_chan_flag = 1;
        for (j=0; j<p_sim_detect->chan_num; j++) {
            if (sc_s->chan == p_sim_detect->chan[j]) {
                new_chan_flag = 0;
                break;
            }
        }
        if (new_chan_flag) {
            p_sim_detect->chan[p_sim_detect->chan_num] = sc_s->chan;
            printk(KERN_INFO "chan #%d used for ADC simcard detect\n", sc_s->chan);
            p_sim_detect->chan_num++;
        }    
        printk(KERN_INFO "%s code(%d) registed.\n", sc_s->name, sc_s->code);
        sc_s++;
    }
    

    register_simdetect_dev(gp_sim_detect, pdev->id);
    return 0;
}

static int simdetect_remove(struct platform_device *pdev)
{
    struct sim_detect *p_sim_detect = platform_get_drvdata(pdev);

    p_sim_detect->modem_control(0);
    unregister_chrdev(p_sim_detect->config_major,p_sim_detect->config_name);
    if(p_sim_detect->config_class)
    {
        if(p_sim_detect->config_dev)
        device_destroy(p_sim_detect->config_class,MKDEV(p_sim_detect->config_major,0));
        class_destroy(p_sim_detect->config_class);
    }
    kfree(p_sim_detect);
    gp_sim_detect=NULL ;
    return 0;
}

static struct platform_driver simdetect_driver = {
    .probe      = simdetect_probe,
    .remove     = simdetect_remove,
    .suspend    = NULL,
    .resume     = NULL,
    .driver     = {
        .name   = "m1-adc-simdetect",
    },
};

static int __devinit simdetect_init(void)
{
    printk(KERN_INFO "ADC Simcard detect Driver init.\n");

    class_register(&simstatus_class);
    return platform_driver_register(&simdetect_driver);
}

static void __exit simdetect_exit(void)
{
    printk(KERN_INFO "ADC Simcard detect Driver exit.\n");

    platform_driver_unregister(&simdetect_driver);
    class_unregister(&simstatus_class);
}

module_init(simdetect_init);
module_exit(simdetect_exit);

MODULE_AUTHOR("Alex Deng");
MODULE_DESCRIPTION("ADC Simdetec Driver");
MODULE_LICENSE("GPL");




