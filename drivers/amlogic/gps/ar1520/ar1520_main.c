#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/ar1520.h>
#include <linux/cdev.h>
#include <linux/fs.h>


#define AR1520_MODULE_NAME   "ar1520_gps"
#define AR1520_DRIVER_NAME "ar1520_gps"
#define AR1520_DEVICE_NAME   "ar1520_gps"
#define AR1520_CLASS_NAME   "ar1520_gps"

static dev_t ar1520_devno;
static struct cdev *ar1520_cdev = NULL;
static struct device *devp=NULL;

static int ar1520_probe(struct platform_device *pdev);
static int ar1520_remove(struct platform_device *pdev);
static ssize_t ar1520_ctrl(struct class *cla,struct class_attribute *attr,const char *buf,size_t count);
static int  ar1520_open(struct inode *inode,struct file *file);
static int  ar1520_release(struct inode *inode,struct file *file);

static struct platform_driver ar1520_driver = {
	.probe = ar1520_probe,
	.remove = ar1520_remove,
	.driver = {
	.name = AR1520_DRIVER_NAME,
	.owner = THIS_MODULE,
	},
};

static const struct file_operations ar1520_fops = {
	.open	= ar1520_open,
	.release	= ar1520_release,
};

static struct class_attribute ar1520_class_attrs[] = {
    __ATTR(ar1520ctrl,S_IWUGO,NULL,ar1520_ctrl),
    __ATTR_NULL
};

static struct class ar1520_class = {
    .name = AR1520_CLASS_NAME,
    .class_attrs = ar1520_class_attrs,
    .owner = THIS_MODULE,
};

static int  ar1520_open(struct inode *inode,struct file *file)
{
	return 0;
}

static int  ar1520_release(struct inode *inode,struct file *file)
{
	return 0;
}

static ssize_t ar1520_ctrl(struct class *cla,struct class_attribute *attr,const char *buf,size_t count)
{
	struct ar1520_platform_data *pdata = NULL;
	pdata = (struct ar1520_platform_data*)devp->platform_data;
	if(pdata == NULL){
		printk("%s platform data is required!\n",__FUNCTION__);
		return -1;
	}
	if(!strncasecmp(buf,"on",2)){
		if(pdata->power_on)
			pdata->power_on();
	}
	else if (!strncasecmp(buf,"off",3)){
		if(pdata->power_off)
			pdata->power_off();
	}
	else{
		printk( KERN_ERR"%s:%s error!Not support this parameter\n",AR1520_MODULE_NAME,__FUNCTION__);
		return -EINVAL;
	}
	return count;
}

static int ar1520_probe(struct platform_device *pdev)
{
	int ret;
	struct ar1520_platform_data *pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "platform data is required!\n");
		ret = -EINVAL;
		goto out;
	}
	ret = alloc_chrdev_region(&ar1520_devno, 0, 1, AR1520_DRIVER_NAME);
	if (ret < 0) {
		printk(KERN_ERR "%s:%s failed to allocate major number\n",AR1520_MODULE_NAME,__FUNCTION__);
		ret = -ENODEV;
		goto out;
	}
	ret = class_register(&ar1520_class);
	if (ret < 0) {
		printk(KERN_ERR "%s:%s  failed to register class\n",AR1520_MODULE_NAME,__FUNCTION__);
		goto error1;
	}
	ar1520_cdev = cdev_alloc();
	if(!ar1520_cdev){
		printk(KERN_ERR "%s:%s failed to allocate memory\n",AR1520_MODULE_NAME,__FUNCTION__);
		goto error2;
	}
	cdev_init(ar1520_cdev,&ar1520_fops);
	ar1520_cdev->owner = THIS_MODULE;
	ret = cdev_add(ar1520_cdev,ar1520_devno,1);
	if(ret){
		printk(KERN_ERR "%s:%s failed to add device\n",AR1520_MODULE_NAME,__FUNCTION__);
		goto error3;
	}
	devp = device_create(&ar1520_class,NULL,ar1520_devno,NULL,AR1520_DEVICE_NAME);
	if(IS_ERR(devp)){
		printk(KERN_ERR "%s:%s failed to create device node\n",AR1520_MODULE_NAME,__FUNCTION__);
		ret = PTR_ERR(devp);
		goto error3;
	}
	devp->platform_data = pdata;
	return 0;
error3:
	cdev_del(ar1520_cdev);
error2:
	class_unregister(&ar1520_class);
error1:
	unregister_chrdev_region(ar1520_devno,1);
out:
	return ret;
}

static int ar1520_remove(struct platform_device *pdev)
{
	unregister_chrdev_region(ar1520_devno,1);
	class_unregister(&ar1520_class);
	device_destroy(NULL, ar1520_devno);
	cdev_del(ar1520_cdev);
	return 0;
}

static int __init init_gps(void)
{
	int ret = -1;
	ret = platform_driver_register(&ar1520_driver);
	if (ret != 0) {
		printk(KERN_ERR "failed to register ar1520 gps module, error %d\n", ret);
		return -ENODEV;
	}
	return ret;
}

module_init(init_gps);

static void __exit unload_gps(void)
{
	platform_driver_unregister(&ar1520_driver);
}
module_exit(unload_gps);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AMLOGIC");
MODULE_DESCRIPTION("GPS driver for AR1520");


