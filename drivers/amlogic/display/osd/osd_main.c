/*
 * Amlogic osd
 * frame buffer driver
 *
 * Copyright (C) 2009 Amlogic, Inc.
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
 * Author:  Tim Yao <timyao@amlogic.com>
 *		   
 *		    jianfeng_wang : add ge2d support 09/05/21	
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <asm/cacheflush.h>
#include <mach/am_regs.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/osd/osd_main.h>
#include <linux/osd/osd_dev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "osd_log.h"
#include <linux/amlog.h>

MODULE_AMLOG(AMLOG_DEFAULT_LEVEL, 0x0, LOG_LEVEL_DESC, LOG_MASK_DESC);

static myfb_dev_t  *gp_fbdev_list[OSD_COUNT]={NULL,NULL};

static DEFINE_MUTEX(dbg_mutex);




const color_bit_define_t*	
_find_color_format(struct fb_var_screeninfo * var)
{
	u32	upper_margin,lower_margin,i,level;
	const color_bit_define_t *ret=NULL;
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"var bits per pixel:%d\n",var->bits_per_pixel);
	level=(var->bits_per_pixel -1)/8; 
	switch(level)
	{
		case 0:
		upper_margin=COLOR_INDEX_08_PAL256;
		lower_margin=COLOR_INDEX_02_PAL4;
		break;
		case 1:
		upper_margin=COLOR_INDEX_16_565;
		lower_margin=COLOR_INDEX_16_655;
		break;	
		case 2:
		upper_margin=COLOR_INDEX_24_RGB;
		lower_margin=COLOR_INDEX_24_6666_A;
		break;		
		case 3:
		upper_margin=COLOR_INDEX_32_ARGB;
		lower_margin=COLOR_INDEX_32_BGRA;
		break;
		case 4:
		upper_margin=COLOR_INDEX_YUV_422;
		lower_margin=COLOR_INDEX_YUV_422;	
		break;
		default : return NULL;
	}
	//if not provide color component length then we find the first depth match.
	if((var->red.length==0)||(var->green.length==0)||(var->blue.length==0)||
		var->bits_per_pixel != (var->red.length+var->green.length+var->blue.length+var->transp.length))
	{
		amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"not provide color component length,use default color \n");
		ret =&default_color_format_array[upper_margin];
	}
	else
	{
		for ( i=upper_margin;i>=lower_margin;i--)
		{
			if( (default_color_format_array[i].red_length==var->red.length)&&
			(default_color_format_array[i].green_length==var->green.length)&&
			(default_color_format_array[i].blue_length==var->blue.length)&&
			(default_color_format_array[i].transp_length ==var->transp.length)&&
			(default_color_format_array[i].transp_offset==var->transp.offset)&&
			(default_color_format_array[i].green_offset==var->green.offset)&&
			(default_color_format_array[i].blue_offset==var->blue.offset)&&
			(default_color_format_array[i].red_offset==var->red.offset))
			{
				 ret = &default_color_format_array[i];
				 break;
			}
		}
	}
	return ret;
}
static void __init
_fbdev_set_default(struct myfb_dev *fbdev,int index )
{
    	/* setup default value */
	fbdev->fb_info->var = mydef_var[index];
	fbdev->fb_info->fix = mydef_fix;
	fbdev->color=_find_color_format(&fbdev->fb_info->var);
}
static int
osd_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct fb_fix_screeninfo *fix;
    	struct myfb_dev *fbdev=( struct myfb_dev*)info->par;
	const color_bit_define_t   *color_format_pt;

	printk("start check var\n");
	fix = &info->fix;
	color_format_pt=_find_color_format(var);	
	if (color_format_pt == NULL || color_format_pt->color_index==0)
	{
		return -EFAULT ;
	}
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"select color format :index%d,bpp %d\r\n",color_format_pt->color_index, \
												color_format_pt->bpp) ;
	fbdev->color=color_format_pt ;
	var->red.offset = color_format_pt->red_offset;
	var->red.length = color_format_pt->red_length;
	var->red.msb_right= color_format_pt->red_msb_right ;
	var->green.offset  = color_format_pt->green_offset;
	var->green.length  = color_format_pt->green_length;
	var->green.msb_right = color_format_pt->green_msb_right;
	var->blue.offset   = color_format_pt->blue_offset;
	var->blue.length   = color_format_pt->blue_length;
	var->blue.msb_right = color_format_pt->blue_msb_right;
	var->transp.offset= color_format_pt->transp_offset ;
	var->transp.length = color_format_pt->transp_length ;
	var->transp.msb_right = color_format_pt->transp_msb_right ;
	var->bits_per_pixel=color_format_pt->bpp ;
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,
					"rgba(L/O):%d/%d-%d/%d-%d/%d-%d/%d\n",
					var->red.length,var->red.offset,
					var->green.length,var->green.offset,
					var->blue.length,var->blue.offset,
					var->transp.length,var->transp.offset);
	fix->visual=color_format_pt->color_type ;
	//adjust memory length.	
 	fix->line_length = var->xres_virtual*var->bits_per_pixel/8;
	if(var->xres_virtual*var->yres_virtual*var->bits_per_pixel/8> fbdev->fb_len )
	{
		amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_HIGH,"no enough memory for %d*%d*%d\r\n",var->xres,var->yres,var->bits_per_pixel);
		return  -ENOMEM;
	}
    	if (var->xres_virtual < var->xres)
        var->xres_virtual = var->xres;

    	if (var->yres_virtual < var->yres)
        var->yres_virtual = var->yres;

    	var->left_margin = var->right_margin = var->upper_margin = var->lower_margin = 0;
    

	if (var->xres + var->xoffset > var->xres_virtual)
	var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
	var->yoffset = var->yres_virtual - var->yres;
    
    	return 0;
}

static int
osd_set_par(struct fb_info *info)
{
	const vinfo_t *vinfo;
	struct myfb_dev *fbdev = (struct myfb_dev *)info->par;
	osd_ctl_t *osd_ctrl=&fbdev->osd_ctl; 
	u32  end;
	
	vinfo = get_current_vinfo();
	if(info->var.xres  >vinfo->width)
	{
		osd_ctrl->disp_end_x=vinfo->width - 1 ;
	}
	else
	{
		end=osd_ctrl->disp_start_x+info->var.xres  ;
		end=(end>vinfo->width?vinfo->width:end); 
		osd_ctrl->disp_end_x=end -1;
	}
	if(info->var.yres  >vinfo->height)
	{
		osd_ctrl->disp_end_y=vinfo->height - 1;
	}
	else
	{     
		end=osd_ctrl->disp_start_y+info->var.yres;
		end=(end>vinfo->height?vinfo->height:end); 
		osd_ctrl->disp_end_y=end - 1; 
	}
	osddev_set((struct myfb_dev *)info->par);
       return  0;
}

static int
osd_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
        unsigned transp, struct fb_info *info)
{
    return osddev_setcolreg(regno, red, green, blue,
                        transp, (struct myfb_dev *)info->par);
}

static int
osd_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int count, index, r;
	u16 *red, *green, *blue, *transp;
	u16 trans = 0xffff;

	red     = cmap->red;
	green   = cmap->green;
	blue    = cmap->blue;
	transp  = cmap->transp;
	index   = cmap->start;

	for (count = 0; count < cmap->len; count++) {
		if (transp)
			trans = *transp++;
		r = osddev_setcolreg(index++, *red++, *green++, *blue++, trans,
				(struct myfb_dev *)info->par);
		if (r != 0)
			return r;
	}

	return 0;
}

static int
osd_ioctl(struct fb_info *info, unsigned int cmd,
               unsigned long arg)
{
	 struct myfb_dev *fbdev = (struct myfb_dev *)info->par;
	 void __user *argp = (void __user *)arg;
   	 u32  src_colorkey;//16 bit or 24 bit 
   	 u32  srckey_enable;
	 u32  gbl_alpha;


    	switch (cmd)
  	{
   		case  FBIOPUT_OSD_SRCKEY_ENABLE:
			copy_from_user(&srckey_enable,argp,sizeof(u32));
			break;
   		case  FBIOPUT_OSD_SRCCOLORKEY:
			copy_from_user(&src_colorkey,argp,sizeof(u32));
			break ;
		case FBIOPUT_OSD_SET_GBL_ALPHA:
			copy_from_user(&gbl_alpha,argp,sizeof(u32));
			break;
		case FBIOGET_OSD_GET_GBL_ALPHA:
		case FBIOPUT_OSD_2X_SCALE:	
			break;
		default :
			amlog_mask_level(LOG_MASK_IOCTL,LOG_LEVEL_HIGH,"command not supported\r\n ");
			return -1;
	}
	mutex_lock(&fbdev->lock);

  	switch (cmd)
    	{
    		case FBIOPUT_OSD_2X_SCALE: //arg :higher 16 bit h_scale_enable, lower 16 bit v_scale_enable
		osddev_set_2x_scale(info->node,arg&0xffff0000?1:0,arg&0xffff?1:0);
		break;		
    		case FBIOPUT_OSD_SRCCOLORKEY:
	    	switch(fbdev->color->color_index)
	  	{
	 		case COLOR_INDEX_16_655:
			case COLOR_INDEX_16_844:
			case COLOR_INDEX_16_565:
			case COLOR_INDEX_24_888_B:
			case COLOR_INDEX_24_RGB:
			case COLOR_INDEX_YUV_422:
	  	   	amlog_mask_level(LOG_MASK_IOCTL,LOG_LEVEL_LOW,"set osd color key 0x%x\r\n",src_colorkey);
	  	 	osddev_set_colorkey(info->node,fbdev->color->color_index,src_colorkey);
			break;
			default: break;
	  	}
	   	break ;
	  	case FBIOPUT_OSD_SRCKEY_ENABLE:
	   	switch(fbdev->color->color_index)
	  	{
	 		case COLOR_INDEX_16_655:
			case COLOR_INDEX_16_844:
			case COLOR_INDEX_16_565:
			case COLOR_INDEX_24_888_B:
			case COLOR_INDEX_24_RGB:
			case COLOR_INDEX_YUV_422:	
			amlog_mask_level(LOG_MASK_IOCTL,LOG_LEVEL_LOW,"set osd color key %s\r\n",srckey_enable?"enable":"disable");
		   	osddev_srckey_enable(info->node,srckey_enable!=0?1:0);	
			break;
			default:break;
	 	}
	   	break;
	 	case FBIOPUT_OSD_SET_GBL_ALPHA:
	 	osddev_set_gbl_alpha(info->node,gbl_alpha);
	 	break;
	 	case  FBIOGET_OSD_GET_GBL_ALPHA:
	 	gbl_alpha=osddev_get_gbl_alpha(info->node);
	 	copy_to_user(argp, &gbl_alpha, sizeof(u32));
	  	break;
    	}

   	mutex_unlock(&fbdev->lock);
	
	return  0;
}
static int osd_open(struct fb_info *info, int arg)
{
	return 0;
	
}


static int
osd_blank(int blank_mode, struct fb_info *info)
{
 	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"osd%d\r\n"	,info->node);
    	osddev_enable((blank_mode != 0) ? 0 : 1,info->node);

    	return 0;
}

static int osd_pan_display(struct fb_var_screeninfo *var,
                        struct fb_info *fbi)
{
	
    	osddev_pan_display(var,fbi);
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"osd_pan_display:=>osd%d\r\n",fbi->node);
	return 0;
}

static int osd_sync(struct fb_info *info)
{
     	
     	return 0;
}


/* fb_ops structures */
static struct fb_ops osd_ops = {
    	.owner          = THIS_MODULE,
    	.fb_check_var   = osd_check_var,
    	.fb_set_par     = osd_set_par,
    	.fb_setcolreg   = osd_setcolreg,
    	.fb_setcmap     = osd_setcmap,
    	.fb_fillrect    = cfb_fillrect,
    	.fb_copyarea    =  cfb_copyarea,
    	.fb_imageblit   = cfb_imageblit,
#ifdef CONFIG_FB_SOFT_CURSOR
    	.fb_cursor      = soft_cursor,
#endif
    	.fb_ioctl       = osd_ioctl,
    	.fb_open      = osd_open,
    	.fb_blank       = osd_blank,
    	.fb_pan_display = osd_pan_display,
	.fb_sync	    = osd_sync,
};
static  void  set_default_display_axis(struct fb_var_screeninfo *var,osd_ctl_t *osd_ctrl,const vinfo_t *vinfo)
{
	
	osd_ctrl->disp_start_x=0;
	osd_ctrl->disp_start_y=0;
	if(var->xres > vinfo->width)
	{
		osd_ctrl->disp_end_x=vinfo->width- 1 ;//screen axis 
	}
	else
	{
		osd_ctrl->disp_end_x=var->xres- 1 ;//screen axis 
	}
	if(var->yres > vinfo->height)
	{
		osd_ctrl->disp_end_y=vinfo->height- 1 ;
	}
	else
	{
		osd_ctrl->disp_end_y=var->yres- 1 ;//screen axis 
	}
	return ;
	

}

int osd_notify_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
	const vinfo_t *vinfo;
	myfb_dev_t *fb_dev;
	int  i,blank;
	disp_rect_t  *disp_rect;

	
	vinfo = get_current_vinfo();
	amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"tv_server:vmode=%s\r\n", vinfo->name);

	switch(cmd)
	{
		case  VOUT_EVENT_MODE_CHANGE:
		amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"recevie change mode  message \r\n");
		for(i=0;i<OSD_COUNT;i++)
		{
			if(NULL==(fb_dev=gp_fbdev_list[i])) continue;
			set_default_display_axis(&fb_dev->fb_info->var,&fb_dev->osd_ctl,vinfo);
			osddev_update_disp_axis(fb_dev,1);
		}
		break;

		case VOUT_EVENT_OSD_BLANK:
		blank=*(int*)para ;	
		for(i=0;i<OSD_COUNT;i++)
		{
			if(NULL==(fb_dev=gp_fbdev_list[i])) continue;
			osd_blank(blank,fb_dev->fb_info);
		}
		break;
		case   VOUT_EVENT_OSD_DISP_AXIS:
		disp_rect=(disp_rect_t*)para;	
		
		for(i=0;i<OSD_COUNT;i++)
		{	
			if(!disp_rect)  break;
			fb_dev=gp_fbdev_list[i];
			fb_dev->osd_ctl.disp_start_x=disp_rect->x  ;
			fb_dev->osd_ctl.disp_start_y=disp_rect->y  ;
			amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"set disp axis: x:%d y:%d w:%d h:%d\r\n"  , \
					disp_rect->x, disp_rect->y,\
					disp_rect->w, disp_rect->h );
			
			if(disp_rect->x+disp_rect->w > vinfo->width)
			{
				fb_dev->osd_ctl.disp_end_x=vinfo->width - 1;
			}
			else
			{
				fb_dev->osd_ctl.disp_end_x=fb_dev->osd_ctl.disp_start_x+disp_rect->w -1 ; 
			}
			if(disp_rect->y+ disp_rect->h>vinfo->height)
			{
				fb_dev->osd_ctl.disp_end_y=vinfo->height- 1;
			}
			else
			{
				fb_dev->osd_ctl.disp_end_y=fb_dev->osd_ctl.disp_start_y + disp_rect->h - 1 ;
			}
		
			disp_rect ++;
			amlog_mask_level(LOG_MASK_PARA,LOG_LEVEL_LOW,"new disp axis: startx:%d starty:%d endx:%d endy:%d\r\n"  , \
					fb_dev->osd_ctl.disp_start_x, fb_dev->osd_ctl.disp_start_y,\
					fb_dev->osd_ctl.disp_end_x,fb_dev->osd_ctl.disp_end_y);
			osddev_update_disp_axis(fb_dev,0);
		}
		
		break;
	}
	return 0;
}

static struct notifier_block osd_notifier_nb = {
	.notifier_call	= osd_notify_callback,
};


static int __init
osd_probe(struct platform_device *pdev)
{
	int r;
       	struct fb_info *fbi=NULL;
	const vinfo_t *vinfo;
    	struct fb_var_screeninfo *var;
  	struct fb_fix_screeninfo *fix;
	struct resource *mem;
	int  index,Bpp;
	logo_object_t  *init_logo_obj=NULL;
	int  logo_osd_index=0;
	myfb_dev_t 	*fbdev = NULL;
	
	

	
	vout_register_client(&osd_notifier_nb);

#ifdef CONFIG_AM_LOGO
	init_logo_obj = get_current_logo_obj();
	if( init_logo_obj )
	{
		if(init_logo_obj->para.output_dev_type<LOGO_DEV_VID) //osd0 or osd1
		{
			logo_osd_index= init_logo_obj->para.output_dev_type;
		}else{
			if(init_logo_obj->para.output_dev_type!=LOGO_DEV_VID)
			init_logo_obj=NULL; //if logo device on video layer ,
		}					 //we cant use it .
		
	}
#endif
	
   	if (NULL==init_logo_obj )
    	{
#ifdef CONFIG_AM_TCON_OUTPUT
		printk("select LCD output \n");
		set_current_vmode(VMODE_LCD);
#else
		printk("select TV output 720p");
    		set_current_vmode(VMODE_720P);	
#endif
		osddev_init();
    	}
	
	vinfo = get_current_vinfo();
    	for (index=0;index<OSD_COUNT;index++)
    	{
    		//platform resource 
		if (!(mem = platform_get_resource(pdev, IORESOURCE_MEM, index)))
		{
			amlog_level(LOG_LEVEL_HIGH,"No frame buffer memory define.\n");
			r = -EFAULT;
			goto failed2;
		}
		//if we have no resource then no need to create this device.
		amlog_level(LOG_LEVEL_HIGH,"[osd%d] 0x%x-0x%x\n",index,mem->start,mem->end);
		if (!mem || mem->start== 0 || mem->end==0 || mem->start==mem->end)
		{
			continue ;
		}
		
    		fbi = framebuffer_alloc(sizeof(struct myfb_dev), &pdev->dev);
    		if(!fbi)
    		{
        		r = -ENOMEM;
        		goto failed1;
    		}
	
		fbdev = (struct myfb_dev *)fbi->par;
		fbdev->fb_info = fbi;
		fbdev->dev = pdev;
 	
		mutex_init(&fbdev->lock);

    		var = &fbi->var;
    		fix = &fbi->fix;

		
		gp_fbdev_list[index]=fbdev;
		fbdev->fb_mem_paddr = mem->start;
		fbdev->fb_len = mem->end - mem->start + 1;
		fbdev->fb_mem_vaddr = ioremap_wc(fbdev->fb_mem_paddr, fbdev->fb_len);

		if (!fbdev->fb_mem_vaddr)
		{
			amlog_level(LOG_LEVEL_HIGH,"failed to ioremap framebuffer\n");
        		r = -ENOMEM;
        		goto failed1;
		}
	
		//clear framebuffer memory
		 amlog_level(LOG_LEVEL_HIGH,"Frame buffer memory assigned at phy:0x%08x, vir:0x%p, size=%dK\n",
	    	fbdev->fb_mem_paddr, fbdev->fb_mem_vaddr, fbdev->fb_len >> 10);
		 

		if(init_logo_obj && index==logo_osd_index ) //adjust default var info
		{
			mydef_var[index].xres=init_logo_obj->dev->vinfo->width;
			mydef_var[index].yres=init_logo_obj->dev->vinfo->height;	
			mydef_var[index].xres_virtual=init_logo_obj->dev->vinfo->width;
			mydef_var[index].yres_virtual=init_logo_obj->dev->vinfo->height<<1;//logo always use double buffer
			mydef_var[index].bits_per_pixel=init_logo_obj->dev->output_dev.osd.color_depth ;
			amlog_level(LOG_LEVEL_HIGH,"init fbdev bpp is :%d\r\n",mydef_var[index].bits_per_pixel);
			
			if(mydef_var[index].bits_per_pixel>32) 
			{
				mydef_var[index].bits_per_pixel=32;
			}
		}else{
			memset((char*)fbdev->fb_mem_vaddr, 0, fbdev->fb_len);	
		}
	
		_fbdev_set_default(fbdev,index);
		if(NULL==fbdev->color)
		{
			r = -ENOENT;
        		goto failed1;
		}
		Bpp=(fbdev->color->color_index >8?(fbdev->color->color_index>16?(fbdev->color->color_index>24?4:3):2):1);
		fix->line_length=var->xres_virtual*Bpp;
		fix->smem_start = fbdev->fb_mem_paddr;
		fix->smem_len = fbdev->fb_len;

		if (fb_alloc_cmap(&fbi->cmap, 16, 0) != 0) {
			amlog_level(LOG_LEVEL_HIGH,"unable to allocate color map memory\n");
      		r = -ENOMEM;
        	goto failed2;
    		}

		if (!(fbi->pseudo_palette = kmalloc(sizeof(u32) * 16, GFP_KERNEL))) {
			amlog_level(LOG_LEVEL_HIGH,"unable to allocate pseudo palette memory\n");
        	r = -ENOMEM;
        	goto failed2;
		}

		memset(fbi->pseudo_palette, 0, sizeof(u32) * 16);

	   	fbi->fbops = &osd_ops;
    		fbi->screen_base = (char __iomem *)fbdev->fb_mem_vaddr ;
		fbi->screen_size = fix->smem_len;
		set_default_display_axis(&fbdev->fb_info->var,&fbdev->osd_ctl,vinfo);
		osd_check_var(var, fbi);
    		register_framebuffer(fbi);
		if(NULL==init_logo_obj )//if we have init a logo object ,then no need to setup hardware . 
		{
			osddev_set(fbdev);
		}
		
   	}	

	index=0;

	amlog_level(LOG_LEVEL_HIGH,"osd probe ok  \r\n");
	return 0;

failed2:
	fb_dealloc_cmap(&fbi->cmap);
failed1:
    	amlog_level(LOG_LEVEL_HIGH,"Driver module insert failed.\n");
   	return r;
}

static int
osd_remove(struct platform_device *pdev)
{
   	struct fb_info *fbi;
	int i=0;

    	amlog_level(LOG_LEVEL_HIGH,"osd_remove.\n");
	if (!pdev)
		return -ENODEV;
	
	vout_unregister_client(&osd_notifier_nb);
	
	
	for(i=0;i<OSD_COUNT;i++)
	{
		if(gp_fbdev_list[i])
		{
			myfb_dev_t * fbdev=gp_fbdev_list[i];

			fbi = fbdev->fb_info;
			iounmap(fbdev->fb_mem_vaddr);
      			kfree(fbi->pseudo_palette);
     			fb_dealloc_cmap(&fbi->cmap);
         		unregister_framebuffer(fbi);
			framebuffer_release(fbi);
		}
	}
	return 0;
}

#ifndef MODULE

/* Process kernel command line parameters */
static int __init
osd_setup_attribute(char *options)
{
    char *this_opt = NULL;
    int r = 0;

    if (!options || !*options)
        goto exit;

    while (!r && (this_opt = strsep(&options, ",")) != NULL) {
        if (!strncmp(this_opt, "xres:", 5))
            mydef_var[0].xres = mydef_var[0].xres_virtual = simple_strtoul(this_opt + 5, NULL, 0);
        else if (!strncmp(this_opt, "yres:", 5))
            mydef_var[0].yres = mydef_var[0].yres_virtual = simple_strtoul(this_opt + 5, NULL, 0);
        else {
            amlog_level(LOG_LEVEL_HIGH,"invalid option\n");
            r = -1;
        }
    }
exit:
    return r;
}

#endif

#ifdef  CONFIG_PM
static int osd_suspend(struct platform_device *pdev, pm_message_t state)
{
	osddev_suspend();
	return 0;
}

static int osd_resume(struct platform_device *pdev)
{
	osddev_resume();
	return 0;
}


#endif 


/****************************************/

static struct platform_driver
osd_driver = {
    .probe      = osd_probe,
    .remove     = osd_remove,
#ifdef  CONFIG_PM      
    .suspend  =osd_suspend,
    .resume    =osd_resume,
#endif    
    .driver     = {
        .name   = "mesonfb",
    }
};

static int __init
osd_init_module(void)
{
	amlog_level(LOG_LEVEL_HIGH,"osd_init\n");

#ifndef MODULE
    {
      	char *option;
      if (fb_get_options("mesonfb", &option)) {
           return -ENODEV;
   	}
	  
   	osd_setup_attribute(option);
    }
#endif

    if (platform_driver_register(&osd_driver)) {
        amlog_level(LOG_LEVEL_HIGH,"failed to register osd driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit
osd_remove_module(void)
{
    amlog_level(LOG_LEVEL_HIGH,"osd_remove_module.\n");

    platform_driver_unregister(&osd_driver);
}

/****************************************/
module_init(osd_init_module);
module_exit(osd_remove_module);

MODULE_DESCRIPTION("AMLOGIC framebuffer driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");
