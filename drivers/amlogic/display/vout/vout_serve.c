/*************************************************************
 * Amlogic 
 * vout  serve program
 *
 * Copyright (C) 2010 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:   jianfeng_wang@amlogic
 *		   
 *		   
 **************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/vout/vinfo.h>
#include <mach/am_regs.h>
#include <asm/uaccess.h>
#include <linux/major.h>
#include "vout_serve.h"
#include "tvmode.h"
#include "vout_log.h"
#include <linux/amlog.h>

MODULE_AMLOG(0, 0xff, LOG_LEVEL_DESC, LOG_MASK_DESC);
//class attribute

SET_VOUT_CLASS_ATTR(enable,func_default_null)
SET_VOUT_CLASS_ATTR(mode,set_vout_mode)
SET_VOUT_CLASS_ATTR(axis,set_vout_window)
SET_VOUT_CLASS_ATTR(wr_reg,write_reg)
SET_VOUT_CLASS_ATTR(rd_reg,read_reg)


static  vout_info_t	vout_info;

/*****************************************************************
**
**	sysfs impletement part  
**
******************************************************************/
static  void   func_default_null(char  *str)
{
	return ;
}
static   int* parse_para(char *para,char   *para_num)
{
	 static unsigned   int  buffer[MAX_NUMBER_PARA] ; 
	 char  *endp ;
	 int *pt=NULL;
	 int len=0,count=0;

	if(!para) return NULL;
	memset(buffer,0,sizeof(int)*MAX_NUMBER_PARA);
	pt=&buffer[0];
	len=strlen(para);
	endp=(char*)buffer;
	do
	{
		//filter space out 
		while(para && ( isspace(*para) || !isalnum(*para)) && len)
		{
			para++;
			len --; 
		}
		if(len==0) break;
		*pt++=simple_strtoul(para,&endp,0);
		
		para=endp;
		len=strlen(para);
	}while(endp && ++count<*para_num&&count<MAX_NUMBER_PARA) ;
	*para_num=count;
	
	return  buffer;
}

static  void  read_reg(char *para)
{
	char  count=1;
	vout_reg_t  reg;

	memcpy(&reg.addr,parse_para(para+1,&count),sizeof(unsigned int));

	if (((*para) == 'm') || ((*para) == 'M'))
	{
		amlog_level(LOG_LEVEL_HIGH,"[0x%x] : 0x%x\r\n", reg.addr, READ_MPEG_REG(reg.addr));
	}else if (((*para) == 'p') || ((*para) == 'P')) {
		if (APB_REG_ADDR_VALID(reg.addr))
		amlog_level(LOG_LEVEL_HIGH,"[0x%x] : 0x%x\r\n", reg.addr, READ_APB_REG(reg.addr));
	}
}

static  void  write_reg(char *para)
{
	char  count=2;
	vout_reg_t  reg;

	memcpy(&reg,parse_para(para+1,&count),sizeof(vout_reg_t));

	if (((*para) == 'm') || ((*para) == 'M'))
		WRITE_MPEG_REG( reg.addr,reg.value);
	else if (((*para) == 'p') || ((*para) == 'P')) {
		if (APB_REG_ADDR_VALID(reg.addr))
			WRITE_APB_REG(reg.addr,reg.value);
	}		
}


	
#ifdef  CONFIG_PM
static int  meson_vout_suspend(struct platform_device *pdev, pm_message_t state);
static int  meson_vout_resume(struct platform_device *pdev);
#endif

static  void  set_vout_mode(char * name)
{
	vmode_t    mode;

	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"tvmode set to %s\r\n",name);
	mode=validate_vmode(name);
	if(VMODE_MAX==mode)
	{
		amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_HIGH,"no matched vout mode\n");
		return ; 
	}
	if(mode==get_current_vmode())
	{
		amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_HIGH,"don't set the same mode as current.\r\n");	
		return ;
	}
	set_current_vmode(mode);
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"new mode %s set ok\r\n",name);
	vout_notifier_call_chain(VOUT_EVENT_MODE_CHANGE,&mode) ;
}
//axis type : 0x12  0x100 0x120 0x130
static void  set_vout_window(char *para) 
{
#define   OSD_COUNT   2
	static  disp_rect_t  disp_rect[OSD_COUNT];
	char  count=OSD_COUNT*4;	
	int   *pt=&disp_rect[0].x;
	

	//parse window para .
	memcpy(pt,parse_para(para,&count),sizeof(disp_rect_t)*OSD_COUNT);
	
	if(count >=4 && count <8 )
	{
		disp_rect[1]=disp_rect[0] ;
	}
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"osd0=>x:%d ,y:%d,w:%d,h:%d\r\n osd1=> x:%d,y:%d,w:%d,h:%d \r\n", \
			*pt,*(pt+1),*(pt+2),*(pt+3),*(pt+4),*(pt+5),*(pt+6),*(pt+7));
	vout_notifier_call_chain(VOUT_EVENT_OSD_DISP_AXIS,&disp_rect[0]) ;
}

/*****************************************************************
**
**	sysfs  declare part 
**
******************************************************************/

static  struct  class_attribute   *vout_attr[]={
&class_vout_attr_enable,
&class_vout_attr_mode,	
&class_vout_attr_axis ,
&class_vout_attr_wr_reg,
&class_vout_attr_rd_reg,
};

static int  create_vout_attr(void)
{
	//create base class for display
	int  i;

	vout_info.base_class=class_create(THIS_MODULE,VOUT_CLASS_NAME);
	if(IS_ERR(vout_info.base_class))
	{
		amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"create vout class fail\r\n");
		return  -1 ;
	}
	//create  class attr
	for(i=0;i<VOUT_ATTR_MAX;i++)
	{
		if ( class_create_file(vout_info.base_class,vout_attr[i]))
		{
			amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"create disp attribute %s fail\r\n",vout_attr[i]->attr.name);
		}
	}
	return   0;
}
/*****************************************************************
**
**	vout driver interface  
**
******************************************************************/
static int __init
 meson_vout_probe(struct platform_device *pdev)
{
	int ret =-1;
	
	vout_info.base_class=NULL;
	amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"start init vout module \r\n");
	ret =create_vout_attr();
	if(ret==0)
	{
		amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"create  vout attribute ok \r\n");
	}
	else
	{
		amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"create  vout attribute fail \r\n");
	}

	return ret;
}
static int
 meson_vout_remove(struct platform_device *pdev)
{
   	int i;
	if(vout_info.base_class==NULL) return -1;
	
	for(i=0;i<VOUT_ATTR_MAX;i++)
	{
		class_remove_file(vout_info.base_class,vout_attr[i]) ;
	}
		
	class_destroy(vout_info.base_class);
	
	return 0;
}

#ifdef  CONFIG_PM
static int  meson_vout_suspend(struct platform_device *pdev, pm_message_t state)
{	
	vout_suspend();
	return 0;
}

static int  meson_vout_resume(struct platform_device *pdev)
{
	vout_resume();
	return 0;
}


#endif 

static struct platform_driver
vout_driver = {
    .probe      = meson_vout_probe,
    .remove     = meson_vout_remove,
#ifdef  CONFIG_PM      
    .suspend  =meson_vout_suspend,
    .resume    =meson_vout_resume,
#endif    
    .driver     = {
        .name   = "mesonvout",
    }
};
static int __init vout_init_module(void)
{
	int ret =0;

	if (platform_driver_register(&vout_driver)) 
	{
       		amlog_level(LOG_LEVEL_HIGH,"failed to register osd driver\n");
        	ret= -ENODEV;
    	}
	
	return ret;

}
static __exit void vout_exit_module(void)
{
	
	amlog_level(LOG_LEVEL_HIGH,"osd_remove_module.\n");

    	platform_driver_unregister(&vout_driver);
}
module_init(vout_init_module);
module_exit(vout_exit_module);

MODULE_DESCRIPTION("vout serve  module");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jianfeng_wang <jianfeng.wang@amlogic.com>");
