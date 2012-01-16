/*
 * TVAFE char device driver.
 *
 * Copyright (c) 2010 Bo Yang <bo.yang@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the smems of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/* Standard Linux headers */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

/* Amlogic headers */
#include <linux/amports/canvas.h>
#include <mach/am_regs.h>
#include <linux/amports/vframe.h>
#include <linux/tvin/tvin.h>
/* Local include */
#include "tvin_global.h"
#include "vdin.h"
#include "tvin_notifier.h"

#include "tvafe_regs.h"
#include "tvafe_general.h"   /* For Kernel used only */
#include "tvafe_adc.h"       /* For Kernel used only */
#include "tvafe_cvd.h"       /* For Kernel used only */
#include "tvafe.h"           /* For user used */

#define TVAFE_NAME               "tvafe"
#define TVAFE_DRIVER_NAME        "tvafe"
#define TVAFE_MODULE_NAME        "tvafe"
#define TVAFE_DEVICE_NAME        "tvafe"
#define TVAFE_CLASS_NAME         "tvafe"

#define TVAFE_COUNT              1

#define TIMER_10MS               (1*HZ/100)          //10ms timer for tvafe main loop
#define TIMER_200MS              (1*HZ/5)            //200ms timer for tvafe main loop

static dev_t                     tvafe_devno;
static struct class              *tvafe_clsp;

typedef struct tvafe_dev_s {
    int                          index;
    struct cdev                  cdev;
    unsigned int                 flags;
    unsigned int                 mem_start;
    unsigned int                 mem_size;
    struct tvafe_pin_mux_s       *pin_mux;

    struct mutex                 tvafe_mutex;


} tvafe_dev_t;

//const static char tvafe_irq_id[] = "tvafe_irq_id";
static struct tvafe_dev_s *tvafe_devp[TVAFE_COUNT];

static struct tvafe_info_s tvafe_info = {
    NULL,
    //signal parameters
    {
        TVIN_PORT_NULL,
        TVIN_SIG_FMT_NULL,
        TVIN_SIG_STATUS_NULL,
        0x81000000,
        0,
    },
    TVAFE_SRC_TYPE_NULL,
    //VGA settings
    {
        0x50,
        0x50,
        0x50,
        0x50,
    },
    //adc calibration data
    {
        0,
        0,
        0,
        0,

        0,
        0,
        0,
        0,

        0,
        0,
        0,
        0
    },
    //WSS data
    {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
    },

    //for canvas
    (0xFF - (TVAFE_VF_POOL_SIZE + 2)),
    TVAFE_VF_POOL_SIZE,
    {
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP,
        VIDTYPE_INTERLACE_BOTTOM,
        VIDTYPE_INTERLACE_TOP
    },
    0x81000000,
    0x70000,
    0,

    0,
    0,
    0,
    0,
    0,

    0,
};

static struct vdin_dev_s *vdin_devp_tvafe = NULL;

//CANVAS module should be moved to vdin
static inline unsigned int tvafe_index2canvas(u32 index)
{
    const unsigned int canvas_tab[TVAFE_VF_POOL_SIZE] = {
        VDIN_START_CANVAS,
        VDIN_START_CANVAS + 1,
        VDIN_START_CANVAS + 2,
        VDIN_START_CANVAS + 3,
        VDIN_START_CANVAS + 4,
        VDIN_START_CANVAS + 5,
        VDIN_START_CANVAS + 6,
        VDIN_START_CANVAS + 7,
    };

    if(index < TVAFE_VF_POOL_SIZE)
        return canvas_tab[index];
    else
        return 0xff;
}

static void tvafe_canvas_init(unsigned int mem_start, unsigned int mem_size)
{
    unsigned i = 0;
    unsigned int canvas_width  = 1920;
    unsigned int canvas_height = 1080;

    tvafe_info.decbuf_size   = 0x70000;
    tvafe_info.pbufAddr  = mem_start ;
    i = (unsigned)(mem_size / tvafe_info.decbuf_size);
    if(i % 2)
        i -= 1;     //sure the counter is even

    tvafe_info.canvas_total_count = (TVAFE_VF_POOL_SIZE > i)? i : TVAFE_VF_POOL_SIZE;
    for ( i = 0; i < tvafe_info.canvas_total_count; i++)
    {
        canvas_config(VDIN_START_CANVAS + i, mem_start + i * tvafe_info.decbuf_size,
            canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
    }

}

static void tvafe_release_canvas(void)
{
    //to do ...

}

static void tvafe_start_dec(unsigned int mem_start, unsigned int mem_size, tvin_port_t port)
{
    /** init canvas and variables**/
    //init canvas
    tvafe_canvas_init(mem_start, mem_size);
    tvafe_info.rd_canvas_index = 0xFF - (TVAFE_VF_POOL_SIZE + 2);
    tvafe_info.wr_canvas_index = 0;
    tvafe_info.param.port = port;


    switch (tvafe_info.src_type) {
        case TVAFE_SRC_TYPE_CVBS:
		case TVAFE_SRC_TYPE_SVIDEO:
            //load init register map by format
            tvafe_cvd2_reg_init();
            tvafe_cvd2_state_init();
			break;
		case TVAFE_SRC_TYPE_VGA:
            //register init with XGA format
            tvafe_set_vga_default();
            tvafe_adc_state_init();
            break;
		case TVAFE_SRC_TYPE_COMPONENT:
            //load init register map by 576i format
            tvafe_set_comp_default();
            tvafe_adc_state_init();
			break;
		case TVAFE_SRC_TYPE_SCART:
			break;
		default:
			break;
    }

    tvafe_source_muxing(&tvafe_info);

}

static void tvafe_stop_dec(void)
{

    tvafe_release_canvas();

    tvafe_info.param.fmt = TVIN_SIG_FMT_NULL;
    tvafe_info.param.status = TVIN_SIG_STATUS_NULL;
    tvafe_info.param.cap_addr = 0;
    tvafe_info.param.cap_flag = 0;
    //need to do ...

}
// interrupt handler
static int tvafe_isr_run(struct vframe_s *vf_info)
{
    int ret = 0;
    unsigned int canvas_id = 0;
    unsigned char field_status = 0, last_recv_index = 0;
    enum tvin_scan_mode_e scan_mode = 0;
	/* Fetch WSS data */

	/* Monitoring CVBS amplitude */
    if (tvafe_info.src_type == TVAFE_SRC_TYPE_CVBS)
        tvafe_cvd2_video_agc_handler(&tvafe_info);

    /* Monitoring VGA phase */
    //if ((tvafe_info.src_type == TVAFE_SOURCE_TYPE_VGA)
    //    && (tvafe_info.command_state != TVAFE_COMMAND_STATE_VGA_AUTO_ADJUST))
    //{
    //    tvafe_vga_auto_phase_handler(tvafe_info);
    //}

    //video frame info setting
    if (tvafe_info.param.status != TVIN_SIG_STATUS_STABLE) {
        pr_dbg("tvafe no signal! \n");
        return ret;
    } else
        scan_mode = tvafe_top_get_scan_mode();

    if (tvafe_info.param.cap_flag)  //frame capture function
    {
        if (tvafe_info.wr_canvas_index == 0)
            last_recv_index = tvafe_info.canvas_total_count - 1;
        else
            last_recv_index = tvafe_info.wr_canvas_index - 1;

        if (last_recv_index != tvafe_info.rd_canvas_index)
            canvas_id = last_recv_index;
        else
        {
            if(tvafe_info.wr_canvas_index == tvafe_info.canvas_total_count - 1)
                canvas_id = 0;
            else
                canvas_id = tvafe_info.wr_canvas_index + 1;
        }

        vdin_devp_tvafe->para.cap_addr = tvafe_info.pbufAddr +
                    (tvafe_info.decbuf_size * canvas_id);

        return ret;
    }


    if (scan_mode == TVIN_SCAN_MODE_PROGRESSIVE) {
        //progressive mode
        if ((tvafe_info.rd_canvas_index > 0xF0) && (tvafe_info.rd_canvas_index < 0xFF)) {
            tvafe_info.rd_canvas_index++;
            tvafe_info.wr_canvas_index++;
            if(tvafe_info.wr_canvas_index > (TVAFE_VF_POOL_SIZE - 1) )
                tvafe_info.wr_canvas_index = 0;
            canvas_id = tvafe_index2canvas(tvafe_info.wr_canvas_index);
            WRITE_MPEG_REG_BITS(VDIN_WR_CTRL, canvas_id, 0, 8);

            return ret;
        } else if (tvafe_info.rd_canvas_index == 0xFF)
            tvafe_info.rd_canvas_index = 0;
        else {
            tvafe_info.rd_canvas_index++;
            if (tvafe_info.rd_canvas_index > (TVAFE_VF_POOL_SIZE - 1))
                tvafe_info.rd_canvas_index = 0;
        }

        vf_info->type = VIDTYPE_VIU_SINGLE_PLANE
                        | VIDTYPE_VIU_422
                        | VIDTYPE_VIU_FIELD
                        | VIDTYPE_PROGRESSIVE;
        vf_info->width = tvin_fmt_tbl[tvafe_info.param.fmt].v_active;
        vf_info->height = tvin_fmt_tbl[tvafe_info.param.fmt].h_active;
        vf_info->duration = 9600000 /
            (tvin_fmt_tbl[tvafe_info.param.fmt].pixel_clk*10000
             /tvin_fmt_tbl[tvafe_info.param.fmt].h_total
             *tvin_fmt_tbl[tvafe_info.param.fmt].v_total);
        vf_info->canvas0Addr = vf_info->canvas1Addr
                             = tvafe_index2canvas(tvafe_info.rd_canvas_index);
        tvafe_info.wr_canvas_index++;
        if (tvafe_info.wr_canvas_index > (TVAFE_VF_POOL_SIZE - 1))
            tvafe_info.wr_canvas_index = 0;
        canvas_id = tvafe_index2canvas(tvafe_info.wr_canvas_index);
        vf_info->index = canvas_id;
        WRITE_MPEG_REG_BITS(VDIN_WR_CTRL, canvas_id, 0, 8);
    } else {
        //interlacing mode
        field_status = READ_MPEG_REG_BITS(VDIN_COM_STATUS0, 0, 1);
        if(tvafe_info.wr_canvas_index == 0)
            last_recv_index = TVAFE_VF_POOL_SIZE - 1;
        else
            last_recv_index = tvafe_info.wr_canvas_index - 1;

        if (field_status == 1) {
            tvafe_info.buff_flag[tvafe_info.wr_canvas_index] = VIDTYPE_INTERLACE_BOTTOM;
            if((tvafe_info.buff_flag[last_recv_index] & 0x0f) == VIDTYPE_INTERLACE_BOTTOM)
                return ret;
        } else {
            tvafe_info.buff_flag[tvafe_info.wr_canvas_index] = VIDTYPE_INTERLACE_TOP;
            if((tvafe_info.buff_flag[last_recv_index] & 0x0f) == VIDTYPE_INTERLACE_TOP)
                return ret;
        }

        if((tvafe_info.rd_canvas_index > 0xF0) && (tvafe_info.rd_canvas_index < 0xFF)) {
            tvafe_info.rd_canvas_index++;
            tvafe_info.wr_canvas_index++;
            if(tvafe_info.wr_canvas_index > (TVAFE_VF_POOL_SIZE - 1))
                tvafe_info.wr_canvas_index = 0;
            canvas_id = tvafe_index2canvas(tvafe_info.wr_canvas_index);
            WRITE_MPEG_REG_BITS(VDIN_WR_CTRL, canvas_id, 0, 8);
                return ret;
        } else if(tvafe_info.rd_canvas_index == 0xFF)
            tvafe_info.rd_canvas_index = 0;
        else {
            tvafe_info.rd_canvas_index++;
            if(tvafe_info.rd_canvas_index > (TVAFE_VF_POOL_SIZE - 1))
                tvafe_info.rd_canvas_index = 0;
        }

        if((tvafe_info.buff_flag[tvafe_info.rd_canvas_index] & 0xf) == VIDTYPE_INTERLACE_BOTTOM)
            vf_info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_BOTTOM;
        else
            vf_info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_TOP;

        vf_info->canvas0Addr = vf_info->canvas1Addr
                             = tvafe_index2canvas(tvafe_info.rd_canvas_index);

        vf_info->width = tvin_fmt_tbl[tvafe_info.param.fmt].v_active;
        vf_info->height = tvin_fmt_tbl[tvafe_info.param.fmt].h_active;
        vf_info->duration = 9600000 /
            (tvin_fmt_tbl[tvafe_info.param.fmt].pixel_clk*10000
             /tvin_fmt_tbl[tvafe_info.param.fmt].h_total
             *tvin_fmt_tbl[tvafe_info.param.fmt].v_total);

        tvafe_info.wr_canvas_index++;
        if(tvafe_info.wr_canvas_index > (TVAFE_VF_POOL_SIZE - 1))
            tvafe_info.wr_canvas_index = 0;
        canvas_id = tvafe_index2canvas(tvafe_info.wr_canvas_index);
        vf_info->index = canvas_id;
        WRITE_MPEG_REG_BITS(VDIN_WR_CTRL, canvas_id, 0, 8);
    }

    return ret;
}

static void check_tvafe_format(void )
{
    unsigned int interrupt_status = READ_MPEG_REG(TVFE_INT_INDICATOR1);


    if (vdin_devp_tvafe == NULL)  //check tvafe status
        return;

    //get frame capture flag from vdin
    tvafe_info.param.cap_flag = vdin_devp_tvafe->para.cap_flag;

    switch (tvafe_info.src_type) {
        case TVAFE_SRC_TYPE_CVBS:
		case TVAFE_SRC_TYPE_SVIDEO:
			/* TVAFE CVD2 3D works abnormally => reset cvd2 */
			//if ((interrupt_status>>WARNING_3D_BIT)&WARNING_3D_MSK)
			if ((interrupt_status>>WARNING_3D_BIT) & 0x01) {
                WRITE_CBUS_REG_BITS(TVFE_INT_INDICATOR1,
                    0, WARNING_3D_BIT, WARNING_3D_WID);
				WRITE_CBUS_REG_BITS(CVD2_RESET_REGISTER,
                    1, SOFT_RST_BIT, SOFT_RST_WID);
				WRITE_CBUS_REG_BITS(CVD2_RESET_REGISTER,
                    0, SOFT_RST_BIT, SOFT_RST_WID);
			}

            /*vcd2 video state handler*/
            tvafe_cvd2_state_handler(&tvafe_info);

			break;
		case TVAFE_SRC_TYPE_VGA:
		case TVAFE_SRC_TYPE_COMPONENT:
            /*ADC state handler*/
            tvafe_adc_state_handler(&tvafe_info);
			break;
		case TVAFE_SRC_TYPE_SCART:
			break;
		default:
			break;
    }

    return;
}

static int tvafe_check_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    switch(cmd)
    {
        case TVIN_EVENT_INFO_CHECK:
            if(vdin_devp_tvafe != NULL)
            {
                printk("tvafe format update ./n");
                check_tvafe_format();
                if (tvafe_info.param.fmt != vdin_devp_tvafe->para.fmt)
                {
                    tvafe_set_fmt(&tvafe_info.param);
                    vdin_info_update(vdin_devp_tvafe, &tvafe_info.param);
                }
            }
            break;
        default:
            //pr_dbg("command is not exit ./n");
            break;
    }
    return 0;
}

static struct notifier_block tvafe_check_notifier = {
    .notifier_call  = tvafe_check_callback,
};

struct tvin_dec_ops_s tvafe_op = {
    .dec_run = tvafe_isr_run,
};

static int tvafe_notifier_callback(struct notifier_block *block,
                                        unsigned long cmd , void *para)
{
    vdin_dev_t *p = NULL;
    switch(cmd)
    {
        case TVIN_EVENT_DEC_START:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                if ((p->para.port < TVIN_PORT_VGA0) || (p->para.port > TVIN_PORT_SVIDEO7))
                    return -1;
                tvafe_start_dec(p->mem_start, p->mem_size, p->para.port);
                tvin_dec_register(p, &tvafe_op);
                tvin_check_notifier_register(&tvafe_check_notifier);
                vdin_devp_tvafe = p;
            }
            break;
        case TVIN_EVENT_DEC_STOP:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                if ((p->para.port < TVIN_PORT_VGA0) || (p->para.port > TVIN_PORT_SVIDEO7))
                    return -1;
                tvafe_stop_dec();
                tvin_dec_unregister(vdin_devp_tvafe);
                tvin_check_notifier_unregister(&tvafe_check_notifier);
                vdin_devp_tvafe = NULL;
            }
            break;
    }

    return 0;
}

static struct notifier_block tvafe_notifier = {
    .notifier_call  = tvafe_notifier_callback,
};


static int tvafe_open(struct inode *inode, struct file *file)
{
    tvafe_dev_t *devp;

    /* Get the per-device structure that contains this cdev */
    devp = container_of(inode->i_cdev, tvafe_dev_t, cdev);
    file->private_data = devp;

    //tvafe_start_dec(&tvafe_info, devp);

    /*2.4 kernel*/
    /* hook vsync isr */ // ??? Should be changed to Input Vsync
    //ret = request_irq(AM_ISA_AMRISC_IRQ(IRQNUM_VSYNC), &tvafe_vsync_isr,
    //                  IRQF_SHARED | IRQ_ISA_FAST, TVAFE_NAME,
    //                  (void *)tvafe_id);
    //enable_irq(7);
    //kernel 2.6 request ir
    //request_irq(tvafe_info.irq_index, tvafe_vsync_isr,
    //            IRQF_SHARED, TVAFE_NAME,
    //            tvafe_id);

    //init  tasklet schedule

    return 0;
}

static int tvafe_release(struct inode *inode, struct file *file)
{
    tvafe_dev_t *devp = file->private_data;

    file->private_data = NULL;

    //stop tv afe polling timer
    //del_timer(&devp->tvafe_timer);
    //stop tasklet schedule
    //tasklet_kill(&tvafe_tasklet);
    //disble ir or vsync
    //free_irq(tvafe_info.irq_index, (void *)tvafe_id);

    /* Release some other fields */
    /* ... */

    printk(KERN_ERR "tvafe: device %d release ok.\n", devp->index);

    return 0;
}


static int tvafe_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    void __user *argp = (void __user *)arg;

    struct tvafe_vga_edid_s *edid = NULL;

    switch (cmd)
    {
		case TVAFEIOC_G_COMP_WSS:
			if (copy_to_user(argp, &tvafe_info.comp_wss, sizeof(struct tvafe_comp_wss_s)))
			{
				ret = -EFAULT;
			}
			break;
        case TVAFEIOC_S_VGA_EDID:
            if (copy_from_user(edid, argp, sizeof(struct tvafe_vga_edid_s)))
            {
                ret = -EFAULT;
            }
            else
            {
                tvafe_vga_set_edid(edid);
            }
            break;
        case TVAFEIOC_G_VGA_EDID:
            tvafe_vga_get_edid(edid);
            if (copy_to_user(argp, edid, sizeof(struct tvafe_vga_edid_s)))
			{
				ret = -EFAULT;
			}
            break;
        case TVAFEIOC_S_ADC_PARAM:
            if (copy_from_user(argp, &tvafe_info.adc_param, sizeof(struct tvafe_adc_param_s)))
			{
				ret = -EFAULT;
			}
            else
            {
                tvafe_adc_set_param(&tvafe_info);
            }
            break;
        case TVAFEIOC_G_ADC_PARAM:
            if (copy_to_user(argp, &tvafe_info.adc_param, sizeof(struct tvafe_adc_param_s)))
			{
				ret = -EFAULT;
			}
            break;
		case TVAFEIOC_G_ADC_CAL_VAL:
			if (copy_to_user(argp, &tvafe_info.adc_cal_val, sizeof(struct tvafe_adc_cal_s)))
			{
				ret = -EFAULT;
			}

            break;
        case TVAFEIOC_S_ADC_CAL_VAL:
            if (copy_from_user(argp, &tvafe_info.adc_cal_val, sizeof(struct tvafe_adc_cal_s)))
			{
				ret = -EFAULT;
			} else
                tvafe_set_cal_value(&tvafe_info.adc_cal_val);
            break;

        case TVAFEIOC_S_VGA_BD_PARAM:
        {
            struct tvafe_vga_boarder_cfg_s boarder_cfg = {0, 0, 0, 0,{0,0,0,0}, 0};
            if (copy_from_user(argp, &boarder_cfg, sizeof(struct tvafe_vga_boarder_cfg_s)))
			{
				ret = -EFAULT;
			} else
                tvafe_vga_set_border_cfg(&boarder_cfg);
            break;
        }
        case TVAFEIOC_G_VGA_BD_PARAM:
        {
            struct tvafe_vga_boarder_info_s boarder_info = {0, 0, 0, 0,0, 0, 0, 0,0, 0, 0, 0,};
            tvafe_vga_get_border_info(&boarder_info);
			if (copy_to_user(argp, &boarder_info, sizeof(struct tvafe_vga_boarder_info_s)))
			{
				ret = -EFAULT;
			}
            break;
        }
        case TVAFEIOC_S_VGA_AP_PARAM:
        {
            struct tvafe_vga_ap_cfg_s ap_cfg = {0, 0, {0,0,0,0},0, 0,0, 0, 0, 0};
            if (copy_from_user(argp, &ap_cfg, sizeof(struct tvafe_vga_ap_cfg_s)))
			{
				ret = -EFAULT;
			} else
                tvafe_vga_set_ap_cfg(&ap_cfg);
            break;
        }
        case TVAFEIOC_G_VGA_AP_PARAM:
        {
            struct tvafe_vga_ap_info_s ap_info = {{0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
            tvafe_vga_get_ap_info(&ap_info);
			if (copy_to_user(argp, &ap_info, sizeof(struct tvafe_vga_ap_info_s)))
			{
				ret = -EFAULT;
			}
            break;
        }
        default:
            ret = -ENOIOCTLCMD;
            break;
    }
    return ret;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations tvafe_fops = {
    .owner   = THIS_MODULE,         /* Owner */
    .open    = tvafe_open,          /* Open method */
    .release = tvafe_release,       /* Release method */
    .ioctl   = tvafe_ioctl,         /* Ioctl method */
    //.fasync  = tvafe_fasync,
    //.poll    = tvafe_poll,                    // poll function
    /* ... */
};

static int tvafe_probe(struct platform_device *pdev)
{
    int ret;
    int i;
    struct device *devp;
    struct resource *res;

    ret = alloc_chrdev_region(&tvafe_devno, 0, TVAFE_COUNT, TVAFE_NAME);
	if (ret < 0) {
		printk(KERN_ERR "tvafe: failed to allocate major number\n");
		return 0;
	}

    tvafe_clsp = class_create(THIS_MODULE, TVAFE_NAME);
    if (IS_ERR(tvafe_clsp))
    {
        unregister_chrdev_region(tvafe_devno, TVAFE_COUNT);
        return PTR_ERR(tvafe_clsp);
	}

    /* @todo do with resources */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        printk(KERN_ERR "tvafe: can't get memory resource\n");
        return -EFAULT;
    }

    for (i = 0; i < TVAFE_COUNT; ++i)
    {
        /* allocate memory for the per-device structure */
        tvafe_devp[i] = kmalloc(sizeof(struct tvafe_dev_s), GFP_KERNEL);
        if (!tvafe_devp[i])
        {
            printk(KERN_ERR "tvafe: failed to allocate memory for tvafe device\n");
            return -ENOMEM;
        }
        tvafe_devp[i]->index = i;

        /* connect the file operations with cdev */
        cdev_init(&tvafe_devp[i]->cdev, &tvafe_fops);
        tvafe_devp[i]->cdev.owner = THIS_MODULE;
        /* connect the major/minor number to the cdev */
        ret = cdev_add(&tvafe_devp[i]->cdev, (tvafe_devno + i), 1);
	    if (ret) {
    		printk(KERN_ERR "tvafe: failed to add device\n");
            /* @todo do with error */
    		return ret;
    	}
        /* create /dev nodes */
        devp = device_create(tvafe_clsp, NULL, MKDEV(MAJOR(tvafe_devno), i),
                            NULL, "tvafe%d", i);
        if (IS_ERR(devp)) {
            printk(KERN_ERR "tvafe: failed to create device node\n");
            class_destroy(tvafe_clsp);
            /* @todo do with error */
            return PTR_ERR(devp);;
	}

        tvafe_devp[i]->mem_start = res->start;
        tvafe_devp[i]->mem_size  = res->end - res->start + 1;
        pr_dbg(" tvafe[%d] memory start addr is %x, mem_size is %x . \n",i,
            tvafe_devp[i]->mem_start, tvafe_devp[i]->mem_size);

        mutex_init(&tvafe_devp[i]->tvafe_mutex);  //tvafe mutex init
    }

    tvin_dec_notifier_register(&tvafe_notifier);

    printk(KERN_INFO "tvafe: driver initialized ok\n");

    return 0;
}

static int tvafe_remove(struct platform_device *pdev)
{
    int i = 0;

    tvin_dec_notifier_unregister(&tvafe_notifier);
    unregister_chrdev_region(tvafe_devno, TVAFE_COUNT);
    for (i = 0; i < TVAFE_COUNT; ++i)
    {
        device_destroy(tvafe_clsp, MKDEV(MAJOR(tvafe_devno), i));
        cdev_del(&tvafe_devp[i]->cdev);
        kfree(tvafe_devp[i]);
    }
    class_destroy(tvafe_clsp);

    printk(KERN_ERR "tvafe: driver removed ok.\n");

    return 0;
}

static struct platform_driver tvafe_driver = {
    .probe      = tvafe_probe,
    .remove     = tvafe_remove,
    .driver     = {
        .name   = TVAFE_DRIVER_NAME,
    }
};

static int __init tvafe_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&tvafe_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register tvafe module, error %d\n", ret);
        return -ENODEV;
    }

    return ret;
}

static void __exit tvafe_exit(void)
{
    platform_driver_unregister(&tvafe_driver);
}

module_init(tvafe_init);
module_exit(tvafe_exit);

MODULE_DESCRIPTION("AMLOGIC TVAFE driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xu Lin <lin.xu@amlogic.com>");

