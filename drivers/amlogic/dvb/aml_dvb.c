/*
 * AMLOGIC Smart card driver.
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
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include <mach/am_regs.h>
#endif
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/amdsc.h>
#include "aml_dvb.h"

#if 1
#define pr_dbg(fmt, args...) printk(KERN_DEBUG "DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "DVB: " fmt, ## args)

#define CARD_NAME "amlogic-dvb"

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static struct aml_dvb aml_dvb_device;
static struct class   aml_stb_class;

static void aml_dvb_dmx_release(struct aml_dvb *advb, struct aml_dmx *dmx)
{
	int i;
	
	dvb_net_release(&dmx->dvb_net);
	aml_dmx_hw_deinit(dmx);
	dmx->demux.dmx.close(&dmx->demux.dmx);
	dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->mem_fe);
	
	for (i=0; i<DMX_DEV_COUNT; i++) {
		dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->hw_fe[i]);
	}
	
	dvb_dmxdev_release(&dmx->dmxdev);
	dvb_dmx_release(&dmx->demux);
}

static int aml_dvb_dmx_init(struct aml_dvb *advb, struct aml_dmx *dmx, int id)
{
	int i, ret;
	struct resource *res;
	struct aml_fe_config cfg;
	char buf[32];
	
	snprintf(buf, sizeof(buf), "demux%d_irq", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_IRQ, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	dmx->dmx_irq = res->start;
	dmx->source  = AM_TS_SRC_TS0;
	
	if(id==0) {
		snprintf(buf, sizeof(buf), "dvr%d_irq", id);
		res = platform_get_resource_byname(advb->pdev, IORESOURCE_IRQ, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			return -1;
		}
		dmx->dvr_irq = res->start;
	} else {
		dmx->dvr_irq = -1;
	}
	
	dmx->demux.dmx.capabilities = (DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_PES_FILTERING | DMX_MEMORY_BASED_FILTERING | DMX_TS_DESCRAMBLING);
	dmx->demux.filternum = dmx->demux.feednum = FILTER_COUNT;
	dmx->demux.priv = advb;
	dmx->demux.start_feed = aml_dmx_hw_start_feed;
	dmx->demux.stop_feed = aml_dmx_hw_stop_feed;
	dmx->demux.write_to_decoder = NULL;
	spin_lock_init(&dmx->slock);
	
	if ((ret = dvb_dmx_init(&dmx->demux)) < 0) {
		pr_error("dvb_dmx failed: error %d",ret);
		goto error_dmx_init;
	}
	
	dmx->dmxdev.filternum = dmx->demux.feednum;
	dmx->dmxdev.demux = &dmx->demux.dmx;
	dmx->dmxdev.capabilities = 0;
	if ((ret = dvb_dmxdev_init(&dmx->dmxdev, &advb->dvb_adapter)) < 0) {
		pr_error("dvb_dmxdev_init failed: error %d",ret);
		goto error_dmxdev_init;
	}
	
	for (i=0; i<DMX_DEV_COUNT; i++) {
		int source = i+DMX_FRONTEND_0;
		dmx->hw_fe[i].source = source;
		
		if ((ret = dmx->demux.dmx.add_frontend(&dmx->demux.dmx, &dmx->hw_fe[i])) < 0) {
			pr_error("adding hw_frontend to dmx failed: error %d",ret);
			dmx->hw_fe[i].source = 0;
			goto error_add_hw_fe;
		}
	}
	
	dmx->mem_fe.source = DMX_MEMORY_FE;
	if ((ret = dmx->demux.dmx.add_frontend(&dmx->demux.dmx, &dmx->mem_fe)) < 0) {
		pr_error("adding mem_frontend to dmx failed: error %d",ret);
		goto error_add_mem_fe;
	}
	
	if ((ret = dmx->demux.dmx.connect_frontend(&dmx->demux.dmx, &dmx->hw_fe[1])) < 0) {
		pr_error("connect frontend failed: error %d",ret);
		goto error_connect_fe;
	}
	
	dmx->id = id;
	dmx->aud_chan = -1;
	dmx->vid_chan = -1;
	dmx->sub_chan = -1;
	
	if ((ret = aml_dmx_hw_init(dmx)) <0) {
		pr_error("demux hw init error %d", ret);
		dmx->id = -1;
		goto error_dmx_hw_init;
	}
	
	dvb_net_init(&advb->dvb_adapter, &dmx->dvb_net, &dmx->demux.dmx);
	
	return 0;
error_dmx_hw_init:
error_connect_fe:
	dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->mem_fe);
error_add_mem_fe:
error_add_hw_fe:
	for (i=0; i<DMX_DEV_COUNT; i++) {
		if (dmx->hw_fe[i].source)
			dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->hw_fe[i]);
	}
	dvb_dmxdev_release(&dmx->dmxdev);
error_dmxdev_init:
	dvb_dmx_release(&dmx->demux);
error_dmx_init:
	return ret;
}

static void aml_dvb_fe_release(struct aml_dvb *advb, struct aml_fe *fe)
{
	if(fe->fe) {
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
	}
}

#ifdef CONFIG_AM_GX1001
#include "gx1001/gx1001.h"
#endif

static int aml_dvb_fe_init(struct aml_dvb *advb, struct aml_fe *fe, int id)
{
	struct dvb_frontend_ops *ops;
	int ret;
	struct resource *res;
	struct aml_fe_config cfg;
	char buf[32];
	
	snprintf(buf, sizeof(buf), "frontend%d_reset", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_MEM, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	cfg.reset_pin = res->start;
	
	snprintf(buf, sizeof(buf), "frontend%d_i2c", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_MEM, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	cfg.i2c_id = res->start;
	
	snprintf(buf, sizeof(buf), "frontend%d_tuner_addr", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_MEM, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	cfg.tuner_addr = res->start>>1;
	
	snprintf(buf, sizeof(buf), "frontend%d_demod_addr", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_MEM, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	cfg.demod_addr = res->start>>1;
	
#ifdef CONFIG_AM_GX1001
	fe->fe = dvb_attach(gx1001_attach, &cfg);
#endif

	if (fe->fe) {
		if ((ret=dvb_register_frontend(&advb->dvb_adapter, fe->fe))) {
			pr_error("frontend registration failed!");
			ops = &fe->fe->ops;
			if (ops->release != NULL)
				ops->release(fe->fe);
			fe->fe = NULL;
			return ret;
		}
	}
	
	fe->id = id;
	return 0;
}

static int dvb_dsc_open(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct aml_dvb *dvb = dvbdev->priv;
	struct aml_dsc *dsc;
	int err, id;
	
	for(id=0; id<DSC_COUNT; id++) {
		if(!dvb->dsc[id].used)
			break;
	}
	
	if(id>= DSC_COUNT) {
		pr_error("too many descrambler\n");
		return -EBUSY;
	}
	
	err = dvb_generic_open(inode, file);
	if (err < 0) {
		return err;
	}
	
	dsc = &dvb->dsc[id];
	dsc->id   = id;
	dsc->pid  = -1;
	dsc->used = 1;
	dsc->set  = 0;
	dsc->dvb  = dvb;
	
	dvbdev->priv = dsc;
	
	return 0;
}

static int dvb_dsc_ioctl(struct inode *inode, struct file *file,
                 unsigned int cmd, void *parg)
{
	struct dvb_device *dvbdev = file->private_data;
	struct aml_dsc *dsc = dvbdev->priv;
	struct aml_dvb *dvb = dsc->dvb;
	struct am_dsc_key *key;
	int ret = 0, i;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);
	
	switch(cmd) {
		case AMDSC_IOC_SET_PID:
			for(i=0; i<DSC_COUNT; i++) {
				if(i==dsc->id)
					continue;
				if(dvb->dsc[i].used && (dvb->dsc[i].pid==(int)parg)) {
					pr_error("descrambler with pid 0x%x already used\n", (int)parg);
					ret = -EBUSY;
				}
			}
			dsc->pid = (int)parg;
			dsc_set_pid(dsc, dsc->pid);
		break;
		case AMDSC_IOC_SET_KEY:
			key = parg;
			if(key->type)
				memcpy(dsc->odd, key->key, 8);
			else
				memcpy(dsc->even, key->key, 8);
			dsc->set |= 1<<(key->type);
			dsc_set_key(dsc, key->type, key->key);
		break;
		default:
			ret = -EINVAL;
		break;
	}
	
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return ret;
}

int dvb_dsc_release(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct aml_dsc *dsc = dvbdev->priv;
	struct aml_dvb *dvb = dsc->dvb;
	unsigned long flags;
	
	dvb_generic_release(inode, file);
	
	spin_lock_irqsave(&dvb->slock, flags);

	dsc->dvb->dsc[dsc->id].used = 0;
	dsc_release(dsc);
	
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return 0;
}

/*Show the STB input source*/
static ssize_t stb_show_source(struct class *class, struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;
	char *src;
	
	switch(dvb->stb_source) {
		case AM_TS_SRC_TS0:
		case AM_TS_SRC_S2P0:
			src = "ts0";
		break;
		case AM_TS_SRC_TS1:
		case AM_TS_SRC_S2P1:
			src = "ts1";
		break;
		case AM_TS_SRC_HIU:
			src = "hiu";
		break;
		default:
			src = "";
		break;
	}
	
	ret = sprintf(buf, "%s\n", src);
	return ret;
}

/*Set the STB input source*/
static ssize_t stb_store_source(struct class *class,struct class_attribute *attr,
                          const char *buf,
                          size_t size)
{
    dmx_source_t src = -1;
	
	if(!strncmp("ts0", buf, 3)) {
    	src = DMX_SOURCE_FRONT0;
    } else if(!strncmp("ts1", buf, 3)) {
    	src = DMX_SOURCE_FRONT1;
    } else if(!strncmp("hiu", buf, 3)) {
    	src = DMX_SOURCE_DVR0;
    }
    if(src!=-1) {
    	aml_stb_hw_set_source(&aml_dvb_device, src);
    }
    
    return size;
}

/*Show the STB input source*/
#define DEMUX_SOURCE_FUNC_DECL(i)  \
static ssize_t demux##i##_show_source(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_dmx *dmx = &dvb->dmx[i];\
	ssize_t ret = 0;\
	char *src;\
	switch(dmx->source) {\
		case AM_TS_SRC_TS0:\
		case AM_TS_SRC_S2P0:\
			src = "ts0";\
		break;\
		case AM_TS_SRC_TS1:\
		case AM_TS_SRC_S2P1:\
			src = "ts1";\
		break;\
		case AM_TS_SRC_HIU:\
			src = "hiu";\
		break;\
		default:\
			src = "";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}\
static ssize_t demux##i##_store_source(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
    dmx_source_t src = -1;\
    \
	if(!strncmp("ts0", buf, 3)) {\
    	src = DMX_SOURCE_FRONT0;\
    } else if(!strncmp("ts1", buf, 3)) {\
    	src = DMX_SOURCE_FRONT1;\
    } else if(!strncmp("hiu", buf, 3)) {\
    	src = DMX_SOURCE_DVR0;\
    }\
    if(src!=-1) {\
    	aml_dmx_hw_set_source(aml_dvb_device.dmx[i].dmxdev.demux, src);\
    }\
    return size;\
}

#if DMX_DEV_COUNT>0
	DEMUX_SOURCE_FUNC_DECL(0)
#endif
#if DMX_DEV_COUNT>1
	DEMUX_SOURCE_FUNC_DECL(1)
#endif
#if DMX_DEV_COUNT>2
	DEMUX_SOURCE_FUNC_DECL(2)
#endif


static struct file_operations dvb_dsc_fops = {
        .owner          = THIS_MODULE,
        .read           = NULL,
        .write          = NULL,
        .ioctl          = dvb_generic_ioctl,
        .open           = dvb_dsc_open,
        .release        = dvb_dsc_release,
        .poll           = NULL,
};

static struct dvb_device dvbdev_dsc = {
        .priv           = NULL,
        .users          = DSC_COUNT,
        .writers        = DSC_COUNT,
        .fops           = &dvb_dsc_fops,
        .kernel_ioctl   = dvb_dsc_ioctl,
};

static struct class_attribute aml_stb_class_attrs[] = {
	__ATTR(source,  S_IRUGO | S_IWUSR, stb_show_source, stb_store_source),
#define DEMUX_SOURCE_ATTR_DECL(i)\
		__ATTR(demux##i##_source,  S_IRUGO | S_IWUSR, demux##i##_show_source, demux##i##_store_source)
#if DMX_DEV_COUNT>0
	DEMUX_SOURCE_ATTR_DECL(0),
#endif
#if DMX_DEV_COUNT>1
	DEMUX_SOURCE_ATTR_DECL(1),
#endif
#if DMX_DEV_COUNT>2
	DEMUX_SOURCE_ATTR_DECL(2),
#endif
	__ATTR_NULL
};

static struct class aml_stb_class = {
	.name = "stb",
	.class_attrs = aml_stb_class_attrs,
};


static int aml_dvb_probe(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i, ret = 0;
	
	advb = &aml_dvb_device;
	memset(advb, 0, sizeof(aml_dvb_device));
	
	spin_lock_init(&advb->slock);
	
	advb->dev  = &pdev->dev;
	advb->pdev = pdev;

	for (i=0; i<DMX_DEV_COUNT; i++) {
		advb->dmx[i].dmx_irq = -1;
		advb->dmx[i].dvr_irq = -1;
	}
	
	ret = dvb_register_adapter(&advb->dvb_adapter, CARD_NAME, THIS_MODULE, advb->dev, adapter_nr);
	if (ret < 0) {
		return ret;
	}
	
	for (i=0; i<DMX_DEV_COUNT; i++)
		advb->dmx[i].id = -1;
	for (i=0; i<FE_DEV_COUNT; i++)
		advb->fe[i].id = -1;
	
	advb->dvb_adapter.priv = advb;
	dev_set_drvdata(advb->dev, advb);
	
	for (i=0; i<DMX_DEV_COUNT; i++) {
		if ((ret=aml_dvb_dmx_init(advb, &advb->dmx[i], i))<0) {
			goto error;
		}
	}
	
	for (i=0; i<FE_DEV_COUNT; i++) {
		if ((ret=aml_dvb_fe_init(advb, &advb->fe[i], i))<0) {
			goto error;
		}
	}
	//removed by whh
#if 0 	
	/*Register descrambler device*/
	ret = dvb_register_device(&advb->dvb_adapter, &advb->dsc_dev,
                                   &dvbdev_dsc, advb, DVB_DEVICE_DSC);
	if(ret<0) {
		goto error;
	}
#endif
	if(class_register(&aml_stb_class)<0) {
                pr_error("register class error\n");
                goto error;
        }
	

	return ret;
error:
	if(advb->dsc_dev) {
		dvb_unregister_device(advb->dsc_dev);
	}
	
	for (i=0; i<DMX_DEV_COUNT; i++) {
		if (advb->dmx[i].id!=-1) {
			aml_dvb_dmx_release(advb, &advb->dmx[i]);
		}
	}
	for (i=0; i<FE_DEV_COUNT; i++) {
		if (advb->fe[i].id!=-1) {
			aml_dvb_fe_release(advb, &advb->fe[i]);
		}
	}
	dvb_unregister_adapter(&advb->dvb_adapter);
	
	return ret;
}

static int aml_dvb_remove(struct platform_device *pdev)
{
	struct aml_dvb *advb = (struct aml_dvb*)dev_get_drvdata(&pdev->dev);
	int i;
	
	class_unregister(&aml_stb_class);
	
	dvb_unregister_device(advb->dsc_dev);
	
	for (i=0; i<DMX_DEV_COUNT; i++) {
		aml_dvb_dmx_release(advb, &advb->dmx[i]);
	}
	for (i=0; i<FE_DEV_COUNT; i++) {
		aml_dvb_fe_release(advb, &advb->fe[i]);
	}
	dvb_unregister_adapter(&advb->dvb_adapter);
	
	return 0;
}

static struct platform_driver aml_dvb_driver = {
	.probe		= aml_dvb_probe,
	.remove		= aml_dvb_remove,	
	.driver		= {
		.name	= "amlogic-dvb",
		.owner	= THIS_MODULE,
	}
};

static int __init aml_dvb_init(void)
{
	return platform_driver_register(&aml_dvb_driver);
}

static void __exit aml_dvb_exit(void)
{
	 platform_driver_unregister(&aml_dvb_driver);
}

/*Get the STB source demux*/
static struct aml_dmx* get_stb_dmx(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	int i;
	
	for(i=0; i<DMX_DEV_COUNT; i++) {
		dmx = &dvb->dmx[i];
		if(dmx->source==dvb->stb_source) {
			return dmx;
		}
	}
	
	return NULL;
}

extern int dmx_reset_hw(struct aml_dvb *dvb);

int tsdemux_reset(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);
	if(dvb->reset_flag) {
		dvb->reset_flag = 0;
		dmx_reset_hw(dvb);
	}
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return 0;
}

EXPORT_SYMBOL(tsdemux_reset);

int tsdemux_set_reset_flag(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);
	dvb->reset_flag = 1;
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return 0;

}

EXPORT_SYMBOL(tsdemux_set_reset_flag);

/*Add the amstream irq handler*/
int tsdemux_request_irq(irq_handler_t handler, void *data)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);
	
	dmx = get_stb_dmx();
	if(dmx) {
		dmx->irq_handler = handler;
		dmx->irq_data = data;
	}
	
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return 0;
}

EXPORT_SYMBOL(tsdemux_request_irq);

/*Free the amstream irq handler*/
int tsdemux_free_irq(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	
	spin_lock_irqsave(&dvb->slock, flags);
	
	dmx = get_stb_dmx();
	if(dmx) {
		dmx->irq_handler = NULL;
		dmx->irq_data = NULL;
	}
	
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	return 0;
}

EXPORT_SYMBOL(tsdemux_free_irq);

/*Reset the video PID*/
int tsdemux_set_vid(int vpid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;
	
	spin_lock_irqsave(&dvb->slock, flags);
	dmx = get_stb_dmx();
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);
		
		spin_lock_irqsave(&dmx->slock, flags);
		
		if(dmx->vid_chan!=-1) {
			dmx_free_chan(dmx, dmx->vid_chan);
			dmx->vid_chan = -1;
		}
		
		if((vpid>=0) && (vpid<0x1FFF)) {
			dmx->vid_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_TS_PES_VIDEO, vpid);
			if(dmx->vid_chan==-1) {
				ret = -1;
			}
		}
		
		spin_unlock_irqrestore(&dmx->slock, flags);
		
		mutex_unlock(&dmx->dmxdev.mutex);
	}
	
	return ret;
}

EXPORT_SYMBOL(tsdemux_set_vid);

/*Reset the audio PID*/
int tsdemux_set_aid(int apid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;
	
	spin_lock_irqsave(&dvb->slock, flags);
	dmx = get_stb_dmx();
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);
		
		spin_lock_irqsave(&dmx->slock, flags);
		
		if(dmx->aud_chan!=-1) {
			dmx_free_chan(dmx, dmx->aud_chan);
			dmx->aud_chan = -1;
		}
		
		if((apid>=0) && (apid<0x1FFF)) {
			dmx->aud_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_TS_PES_AUDIO, apid);
			if(dmx->aud_chan==-1) {
				ret = -1;
			}
		}
		
		spin_unlock_irqrestore(&dmx->slock, flags);
		
		mutex_unlock(&dmx->dmxdev.mutex);
	}
	
	return ret;
}

EXPORT_SYMBOL(tsdemux_set_aid);

/*Reset the subtitle PID*/
int tsdemux_set_sid(int spid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;
	
	spin_lock_irqsave(&dvb->slock, flags);
	dmx = get_stb_dmx();
	spin_unlock_irqrestore(&dvb->slock, flags);
	
	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);
		
		spin_lock_irqsave(&dmx->slock, flags);
		
		if(dmx->sub_chan!=-1) {
			dmx_free_chan(dmx, dmx->sub_chan);
			dmx->sub_chan = -1;
		}
		
		if((spid>=0) && (spid<0x1FFF)) {
			dmx->sub_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_TS_PES_SUBTITLE, spid);
			if(dmx->sub_chan==-1) {
				ret = -1;
			}
		}
		
		spin_unlock_irqrestore(&dmx->slock, flags);
		
		mutex_unlock(&dmx->dmxdev.mutex);
	}
	
	return ret;
}

EXPORT_SYMBOL(tsdemux_set_sid);

module_init(aml_dvb_init);
module_exit(aml_dvb_exit);

MODULE_DESCRIPTION("driver for the AMLogic DVB card");
MODULE_AUTHOR("AMLOGIC");
MODULE_LICENSE("GPL");

