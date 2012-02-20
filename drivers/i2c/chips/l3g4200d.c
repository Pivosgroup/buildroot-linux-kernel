/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : l3g4200d.c
* Authors            : MH - C&I BU - Application Team
*		     : Carmine Iascone (carmine.iascone@st.com)
*		     : Matteo Dameno (matteo.dameno@st.com)
* Version            : V 0.2
* Date               : 09/04/2010
* Description        : L3G4200D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION | DATE 	| AUTHORS	     | DESCRIPTION
*
* 0.1	  | 29/01/2010	| Carmine Iascone    | First Release
* 
* 0.2	  | 09/04/2010  | Carmine Iascone    | Updated the struct l3g4200d_t
*
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/uaccess.h>
#include <linux/i2c/l3g4200d.h>
#include <linux/slab.h> 


#define L3G4200D_MAJOR   102
#define L3G4200D_MINOR   4

/* l3g4200d gyroscope registers */
#define WHO_AM_I    0x0F

#define CTRL_REG1       0x20    /* power control reg */
#define CTRL_REG2       0x21    /* power control reg */
#define CTRL_REG3       0x22    /* power control reg */
#define CTRL_REG4       0x23    /* interrupt control reg */
#define CTRL_REG5       0x24    /* interrupt control reg */
#define AXISDATA_REG    0x28

#define DEBUG 1

/*
 * L3G4200D gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */

struct l3g4200d_t {
	short	x,	/* x-axis angular rate data. Range -2048 to 2047. */
		y,	/* y-axis angluar rate data. Range -2048 to 2047. */
		z;	/* z-axis angular rate data. Range -2048 to 2047. */
};

/* static struct i2c_client *l3g4200d_client; */

struct l3g4200d_data {
	struct i2c_client *client;
	struct l3g4200d_platform_data *pdata;
	/* u8 sensitivity; */
};

static struct l3g4200d_data *gyro;

static struct class *l3g_gyro_dev_class;

static char l3g4200d_i2c_write(unsigned char reg_addr,
				    unsigned char *data,
				    unsigned char len);

static char l3g4200d_i2c_read(unsigned char reg_addr,
				   unsigned char *data,
				   unsigned char len);

/* set l3g4200d digital gyroscope bandwidth */
int l3g4200d_set_bandwidth(char bw)
{
	int res = 0;
	unsigned char data;

	res = i2c_smbus_read_word_data(gyro->client, CTRL_REG1);
	if (res >= 0)
		data = res & 0x000F;

	data = data + bw;
	res = l3g4200d_i2c_write(CTRL_REG1, &data, 1);
	return res;
}

/* read selected bandwidth from l3g4200d */
int l3g4200d_get_bandwidth(unsigned char *bw)
{
	int res = 1;
	/* TO DO */
	return res;
}

int l3g4200d_set_mode(char mode)
{
	int res = 0;
	unsigned char data;

	res = i2c_smbus_read_word_data(gyro->client, CTRL_REG1);
	if (res >= 0)
		data = res & 0x00F7;

	data = mode + data;

	res = l3g4200d_i2c_write(CTRL_REG1, &data, 1);
	return res;
}

int l3g4200d_set_range(char range)
{
	int res = 0;
	unsigned char data;
	
	res = i2c_smbus_read_word_data(gyro->client, CTRL_REG4);
	if (res >= 0)
		data = res & 0x00CF;

	data = range + data;
	res = l3g4200d_i2c_write(CTRL_REG4, &data, 1);
	return res;

}

/* gyroscope data readout */
int l3g4200d_read_gyro_values(struct l3g4200d_t *data)
{
	int res;
	unsigned char gyro_data[6];
	/* x,y,z hardware data */
	int hw_d[3] = { 0 };

	res = l3g4200d_i2c_read(AXISDATA_REG, &gyro_data[0], 6);

	hw_d[0] = (short) (((gyro_data[1]) << 8) | gyro_data[0]);
	hw_d[1] = (short) (((gyro_data[3]) << 8) | gyro_data[2]);
	hw_d[2] = (short) (((gyro_data[5]) << 8) | gyro_data[4]);

	/* hw_d[0] >>= gyro->sensitivity;
	hw_d[1] >>= gyro->sensitivity;
	hw_d[2] >>= gyro->sensitivity; */

	data->x = ((gyro->pdata->negate_x) ? (-hw_d[gyro->pdata->axis_map_x])
		   : (hw_d[gyro->pdata->axis_map_x]));
	data->y = ((gyro->pdata->negate_y) ? (-hw_d[gyro->pdata->axis_map_y])
		   : (hw_d[gyro->pdata->axis_map_y]));
	data->z = ((gyro->pdata->negate_z) ? (-hw_d[gyro->pdata->axis_map_z])
		   : (hw_d[gyro->pdata->axis_map_z]));

	return res;
}


/* Device Initialization  */
static int device_init(void)
{
	int res;
	unsigned char buf[5];
	buf[0] = 0x27;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	res = l3g4200d_i2c_write(CTRL_REG1, &buf[0], 5);
	return res;
}

/*  i2c write routine for l3g4200d digital gyroscope */
static char l3g4200d_i2c_write(unsigned char reg_addr,
				    unsigned char *data,
				    unsigned char len)
{
	int dummy;
	int i;

	if (gyro->client == NULL)  /*  No global client pointer? */
		return -1;
	for (i = 0; i < len; i++) {
		dummy = i2c_smbus_write_byte_data(gyro->client,
						  reg_addr++, data[i]);
		if (dummy) {
			#if DEBUG
			printk(KERN_ERR "i2c write error\n");
			#endif
			return dummy;
		}
	}
	return 0;
}

/*  i2c read routine for l3g4200d digital gyroscope */
static char l3g4200d_i2c_read(unsigned char reg_addr,
				   unsigned char *data,
				   unsigned char len)
{
	int dummy = 0;
	int i = 0;

	if (gyro->client == NULL)  /*  No global client pointer? */
		return -1;
	while (i < len) {
		dummy = i2c_smbus_read_word_data(gyro->client, reg_addr++);
		if (dummy >= 0) {
			data[i] = dummy & 0x00ff;
			i++;
		} else {
			#if DEBUG
			printk(KERN_ERR" i2c read error\n ");
			#endif
			return dummy;
		}
		dummy = len;
	}
	return dummy;
}

/*  read command for l3g4200d device file  */
static ssize_t l3g4200d_read(struct file *file, char __user *buf,
				  size_t count, loff_t *offset)
{
	#if DEBUG
	struct l3g4200d_t data;
	#endif
	if (gyro->client == NULL)
		return -1;
	#if DEBUG
	l3g4200d_read_gyro_values(&data);
	printk(KERN_INFO "X axis: %d\n", data.x);
	printk(KERN_INFO "Y axis: %d\n", data.y);
	printk(KERN_INFO "Z axis: %d\n", data.z);
	#endif
	return 0;
}

/*  write command for l3g4200d device file */
static ssize_t l3g4200d_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *offset)
{
	if (gyro->client == NULL)
		return -1;
	#if DEBUG
	printk(KERN_INFO "l3g4200d should be accessed with ioctl command\n");
	#endif
	return 0;
}

/*  open command for l3g4200d device file  */
static int l3g4200d_open(struct inode *inode, struct file *file)
{
	if (gyro->client == NULL) {
		#if DEBUG
		printk(KERN_ERR "I2C driver not install\n");
		#endif
		return -1;
	}
	device_init();

	#if DEBUG
	printk(KERN_INFO "l3g4200d has been opened\n");
	#endif
	return 0;
}

/*  release command for l3g4200d device file */
static int l3g4200d_close(struct inode *inode, struct file *file)
{
	#if DEBUG
	printk(KERN_INFO "L3G4200D has been closed\n");
	#endif
	return 0;
}


/*  ioctl command for l3g4200d device file */
static int l3g4200d_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	unsigned char data[6];

	/* check l3g4200d_client */
	if (gyro->client == NULL) {
		#if DEBUG
		printk(KERN_ERR "I2C driver not install\n");
		#endif
		return -EFAULT;
	}

	/* cmd mapping */

	switch (cmd) {

	/*case L3G4200D_SELFTEST:
	//TO DO
	return err;*/

	case L3G4200D_SET_RANGE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			#if DEBUG
			printk(KERN_ERR "copy_from_user error\n");
			#endif
			return -EFAULT;
		}
		err = l3g4200d_set_range(*data);
		return err;

	case L3G4200D_SET_MODE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			#if DEBUG
			printk(KERN_ERR "copy_to_user error\n");
			#endif
			return -EFAULT;
		}
		err = l3g4200d_set_mode(*data);
		return err;

	case L3G4200D_SET_BANDWIDTH:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			#if DEBUG
			printk(KERN_ERR "copy_from_user error\n");
			#endif
			return -EFAULT;
		}
		err = l3g4200d_set_bandwidth(*data);
		return err;

	case L3G4200D_READ_GYRO_VALUES:
		err = l3g4200d_read_gyro_values(
				(struct l3g4200d_t *)data);

		if (copy_to_user((struct l3g4200d_t *)arg,
				 (struct l3g4200d_t *)data, 6) != 0) {
			#if DEBUG
			printk(KERN_ERR "copy_to error\n");
			#endif
			return -EFAULT;
		}
		return err;

	default:
		return 0;
	}
}


static const struct file_operations l3g4200d_fops = {
	.owner = THIS_MODULE,
	.read = l3g4200d_read,
	.write = l3g4200d_write,
	.open = l3g4200d_open,
	.release = l3g4200d_close,
	.ioctl = l3g4200d_ioctl,
};


static int l3g4200d_validate_pdata(struct l3g4200d_data *gyro)
{
	if (gyro->pdata->axis_map_x > 2 ||
	    gyro->pdata->axis_map_y > 2 ||
	    gyro->pdata->axis_map_z > 2) {
		dev_err(&gyro->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			gyro->pdata->axis_map_x, gyro->pdata->axis_map_y,
			gyro->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (gyro->pdata->negate_x > 1 ||
	    gyro->pdata->negate_y > 1 ||
	    gyro->pdata->negate_z > 1) {
		dev_err(&gyro->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			gyro->pdata->negate_x, gyro->pdata->negate_y,
			gyro->pdata->negate_z);
		return -EINVAL;
	}

	return 0;
}

static int l3g4200d_probe(struct i2c_client *client,
			       const struct i2c_device_id *devid)
{
	struct l3g4200d_data *data;
	struct device *dev;
	int err = 0;
	int tempvalue;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit;
	}

	//if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
	//	goto exit;

	/*
	 * OK. For now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */
	data = kzalloc(sizeof(struct l3g4200d_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	data->pdata = kmalloc(sizeof(*data->pdata), GFP_KERNEL);

	if (data->pdata == NULL)
		goto exit_kfree;

	memcpy(data->pdata, client->dev.platform_data, sizeof(*data->pdata));

	err = l3g4200d_validate_pdata(data);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}

	if (i2c_smbus_read_byte(client) < 0) {
		#if DEBUG
		printk(KERN_ERR "i2c_smbus_read_byte error!!\n");
		#endif
		goto exit_kfree;
	} else {
		#if DEBUG
		printk(KERN_INFO "L3G4200D Device detected!\n");
		#endif
	}

	/* read chip id */
	tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);
	if ((tempvalue & 0x00FF) == 0x00D3) {
		#if DEBUG
		printk(KERN_INFO "I2C driver registered!\n");
		#endif
	} else {
		data->client = NULL;
		goto exit_kfree;
	}

	gyro = data;

	/* register a char dev */
	err = register_chrdev(L3G4200D_MAJOR,
			      "l3g4200d",
			      &l3g4200d_fops);
	if (err)
		goto out;
	/* create l3g-dev device class */
	l3g_gyro_dev_class = class_create(THIS_MODULE, "L3G_GYRO-dev");
	if (IS_ERR(l3g_gyro_dev_class)) {
		err = PTR_ERR(l3g_gyro_dev_class);
		goto out_unreg_chrdev;
	}

	/* create device node for l3g4200d digital gyroscope */
	dev = device_create(l3g_gyro_dev_class, NULL,
		MKDEV(L3G4200D_MAJOR, 0),
		NULL,
		"l3g4200d");
	if (IS_ERR(dev)) {
		err = PTR_ERR(dev);
		goto error_destroy;
	}

	#if DEBUG
	printk(KERN_INFO "L3G4200D device created successfully\n");
	#endif

	return 0;
error_destroy:
	class_destroy(l3g_gyro_dev_class);
out_unreg_chrdev:
	unregister_chrdev(L3G4200D_MAJOR, "l3g4200d");
out:
	#if DEBUG
	printk(KERN_ERR "%s: Driver Initialization failed\n", __FILE__);
	#endif
exit_kfree_pdata:
	kfree(data->pdata);
exit_kfree:
	kfree(data);
exit:
	return err;
}

static int l3g4200d_remove(struct i2c_client *client)
{
	struct l3g4200d_data *lis = i2c_get_clientdata(client);
	#if DEBUG
	printk(KERN_INFO "L3G4200D driver removing\n");
	#endif
	device_destroy(l3g_gyro_dev_class, MKDEV(L3G4200D_MAJOR, 0));
	class_destroy(l3g_gyro_dev_class);
	unregister_chrdev(L3G4200D_MAJOR, "l3g4200d");

	kfree(lis);
	gyro->client = NULL;
	return 0;
}
#ifdef CONFIG_PM
static int l3g4200d_suspend(struct i2c_client *client, pm_message_t state)
{
	#if DEBUG
	printk(KERN_INFO "l3g4200d_suspend\n");
	#endif
	/* TO DO */
	return 0;
}

static int l3g4200d_resume(struct i2c_client *client)
{
	#if DEBUG
	printk(KERN_INFO "l3g4200d_resume\n");
	#endif
	/* TO DO */
	return 0;
}
#endif

static const struct i2c_device_id l3g4200d_id[] = {
	{ "l3g4200d", 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, l3g4200d_id);

static struct i2c_driver l3g4200d_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = l3g4200d_probe,
	.remove = __devexit_p(l3g4200d_remove),
	.id_table = l3g4200d_id,
	#ifdef CONFIG_PM
	.suspend = l3g4200d_suspend,
	.resume = l3g4200d_resume,
	#endif
	.driver = {
	.owner = THIS_MODULE,
	.name = "l3g4200d",
	},
	/*
	.detect = l3g4200d_detect,
	*/
};

static int __init l3g4200d_init(void)
{
	#if DEBUG
	printk(KERN_INFO "L3G4200D init driver\n");
	#endif
	return i2c_add_driver(&l3g4200d_driver);
}

static void __exit l3g4200d_exit(void)
{
	#if DEBUG
	printk(KERN_INFO "L3G4200D exit\n");
	#endif
	i2c_del_driver(&l3g4200d_driver);
	return;
}

module_init(l3g4200d_init);
module_exit(l3g4200d_exit);

MODULE_DESCRIPTION("l3g4200d digital gyroscope driver");
MODULE_AUTHOR("STMicroelectronics");
MODULE_LICENSE("GPL");


