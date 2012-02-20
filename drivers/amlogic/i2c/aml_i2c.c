/*
 * linux/drivers/amlogic/i2c/aml_i2c.c
 */

#include <asm/errno.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/i2c-aml.h>
#include <linux/i2c-algo-bit.h>

#include "aml_i2c.h"

struct aml_i2c aml_i2c_ddata = {
	.i2c_debug = 0,
};

static void aml_i2c_set_clk(struct aml_i2c *i2c) 
{	
	unsigned int i2c_clock_set;
	unsigned int sys_clk_rate;
	struct clk *sys_clk;
	struct aml_i2c_reg_ctrl* ctrl;

	sys_clk = clk_get_sys("clk81", NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	//sys_clk_rate = get_mpeg_clk();

	i2c_clock_set = sys_clk_rate / i2c->master_i2c_speed;
	i2c_clock_set >>= 2;

	ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
	ctrl->clk_delay = i2c_clock_set & AML_I2C_CTRL_CLK_DELAY_MASK;
} 

static void aml_i2c_set_platform_data(struct aml_i2c *i2c, 
										struct aml_i2c_platform *plat)
{
	i2c->master_i2c_speed = plat->master_i2c_speed;
	i2c->wait_count = plat->wait_count;
	i2c->wait_ack_interval = plat->wait_ack_interval;
	i2c->wait_read_interval = plat->wait_read_interval;
	i2c->wait_xfer_interval = plat->wait_xfer_interval;

	if(i2c->master_no == MASTER_A){
		i2c->master_pinmux.scl_reg = plat->master_a_pinmux.scl_reg;
		i2c->master_pinmux.scl_bit = plat->master_a_pinmux.scl_bit;
		i2c->master_pinmux.sda_reg = plat->master_a_pinmux.sda_reg;
		i2c->master_pinmux.sda_bit = plat->master_a_pinmux.sda_bit;
	}
	else {
		i2c->master_pinmux.scl_reg = plat->master_b_pinmux.scl_reg;
		i2c->master_pinmux.scl_bit = plat->master_b_pinmux.scl_bit;
		i2c->master_pinmux.sda_reg = plat->master_b_pinmux.sda_reg;
		i2c->master_pinmux.sda_bit = plat->master_b_pinmux.sda_bit;
	}
}

static void aml_i2c_pinmux_master(struct aml_i2c *i2c)
{
	unsigned int scl_pinmux;
	unsigned int sda_pinmux;
	
	scl_pinmux = readl(i2c->master_pinmux.scl_reg);
	scl_pinmux |= i2c->master_pinmux.scl_bit;
	writel(scl_pinmux, i2c->master_pinmux.scl_reg);
	
	sda_pinmux = readl(i2c->master_pinmux.sda_reg);
	sda_pinmux |= i2c->master_pinmux.sda_bit;
	writel(sda_pinmux, i2c->master_pinmux.sda_reg);
}

static void aml_i2c_dbg(struct aml_i2c *i2c)
{
	int i;
	struct aml_i2c_reg_ctrl* ctrl;

	if(i2c->i2c_debug == 0)
		return ;
	
	printk( "i2c_slave_addr:  0x%x\n", 
								i2c->master_regs->i2c_slave_addr);
	printk( "i2c_token_list_0:  0x%x\n", 
								i2c->master_regs->i2c_token_list_0);
	printk( "i2c_token_list_1:  0x%x\n", 
								i2c->master_regs->i2c_token_list_1);
	printk( "i2c_token_wdata_0:  0x%x\n", 
								i2c->master_regs->i2c_token_wdata_0);
	printk( "i2c_token_wdata_1:  0x%x\n", 
								i2c->master_regs->i2c_token_wdata_1);
	printk( "i2c_token_rdata_0:  0x%x\n", 
								i2c->master_regs->i2c_token_rdata_0);
	printk( "i2c_token_rdata_1:  0x%x\n", 
								i2c->master_regs->i2c_token_rdata_1);
	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
		printk("token_tag[%d]  %d\n", i, i2c->token_tag[i]);
	
	ctrl = ((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl));
	printk( "i2c_ctrl:  0x%x\n", i2c->master_regs->i2c_ctrl);
	printk( "ctrl.rdsda  0x%x\n", ctrl->rdsda);
	printk( "ctrl.rdscl  0x%x\n", ctrl->rdscl);
	printk( "ctrl.wrsda  0x%x\n", ctrl->wrsda);
	printk( "ctrl.wrscl  0x%x\n", ctrl->wrscl);
	printk( "ctrl.manual_en  0x%x\n", ctrl->manual_en);
	printk( "ctrl.clk_delay  0x%x\n", ctrl->clk_delay);
	printk( "ctrl.rd_data_cnt  0x%x\n", ctrl->rd_data_cnt);
	printk( "ctrl.cur_token  0x%x\n", ctrl->cur_token);
	printk( "ctrl.error  0x%x\n", ctrl->error);
	printk( "ctrl.status  0x%x\n", ctrl->status);
	printk( "ctrl.ack_ignore  0x%x\n", ctrl->ack_ignore);
	printk( "ctrl.start  0x%x\n", ctrl->start);
								
}

static void aml_i2c_clear_token_list(struct aml_i2c *i2c)
{
	i2c->master_regs->i2c_token_list_0 = 0;
	i2c->master_regs->i2c_token_list_1 = 0;
	memset(i2c->token_tag, TOKEN_END, AML_I2C_MAX_TOKENS);
}

static void aml_i2c_set_token_list(struct aml_i2c *i2c)
{
	int i;
	unsigned int token_reg=0;
	
	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
		token_reg |= i2c->token_tag[i]<<(i*4);

	i2c->master_regs->i2c_token_list_0=token_reg;
}

static void aml_i2c_hw_init(struct aml_i2c *i2c, unsigned int use_pio)
{
	struct aml_i2c_reg_ctrl* ctrl;

	aml_i2c_set_clk(i2c);

	/*manual mode*/
	if(use_pio){
		ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
		ctrl->manual_en = 1;
	}
}

static int aml_i2c_check_error(struct aml_i2c *i2c)
{
	struct aml_i2c_reg_ctrl* ctrl;
	ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
	
	if(ctrl->error)
	{
		//printk( "ctrl.cur_token  0x%x\n", ctrl->cur_token);
		return -EIO;
	}
	else
		return 0;
}

/*poll status*/
static int aml_i2c_wait_ack(struct aml_i2c *i2c)
{
	int i;
	struct aml_i2c_reg_ctrl* ctrl;
	
	for(i=0; i<i2c->wait_count; i++) {
		udelay(i2c->wait_ack_interval);
		ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
		if(ctrl->status == IDLE)
			return aml_i2c_check_error(i2c);
		
		cond_resched();
	}

	return -ETIMEDOUT;			
}

static void aml_i2c_get_read_data(struct aml_i2c *i2c, unsigned char *buf, 
														size_t len)
{
	int i;
	unsigned long rdata0 = i2c->master_regs->i2c_token_rdata_0;
	unsigned long rdata1 = i2c->master_regs->i2c_token_rdata_1;

	for(i=0; i< min_t(size_t, len, AML_I2C_MAX_TOKENS>>1); i++)
		*buf++ = (rdata0 >> (i*8)) & 0xff;

	for(; i< min_t(size_t, len, AML_I2C_MAX_TOKENS); i++) 
		*buf++ = (rdata1 >> ((i - (AML_I2C_MAX_TOKENS>>1))*8)) & 0xff;
}

static void aml_i2c_fill_data(struct aml_i2c *i2c, unsigned char *buf, 
							size_t len)
{
	int i;
	unsigned int wdata0 = 0;
	unsigned int wdata1 = 0;

	for(i=0; i< min_t(size_t, len, AML_I2C_MAX_TOKENS>>1); i++)
		wdata0 |= (*buf++) << (i*8);

	for(; i< min_t(size_t, len, AML_I2C_MAX_TOKENS); i++)
		wdata1 |= (*buf++) << ((i - (AML_I2C_MAX_TOKENS>>1))*8); 

	i2c->master_regs->i2c_token_wdata_0 = wdata0;
	i2c->master_regs->i2c_token_wdata_1 = wdata1;
}

static void aml_i2c_xfer_prepare(struct aml_i2c *i2c)
{
	aml_i2c_pinmux_master(i2c);
	aml_i2c_set_clk(i2c);
} 

static void aml_i2c_start_token_xfer(struct aml_i2c *i2c)
{
	//((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl))->start = 0;	/*clear*/
	//((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl))->start = 1;	/*set*/
	i2c->master_regs->i2c_ctrl &= ~1;	/*clear*/
	i2c->master_regs->i2c_ctrl |= 1;	/*set*/
	
	udelay(i2c->wait_xfer_interval);
}

/*our controller should send write data with slave addr in a token list,
	so we can't do normal address, just set addr into addr reg*/
static int aml_i2c_do_address(struct aml_i2c *i2c, unsigned int addr)
{
	int ret;
	
	i2c->cur_slave_addr = addr&0x7f;
	//((struct aml_i2c_reg_slave_addr*)&(i2c->master_regs->i2c_slave_addr))
	//								->slave_addr = i2c->cur_slave_addr<<1;
	i2c->master_regs->i2c_slave_addr = i2c->cur_slave_addr<<1;
	
	return 0;
}

static void aml_i2c_stop(struct aml_i2c *i2c)
{
	aml_i2c_clear_token_list(i2c);
	i2c->token_tag[0]=TOKEN_STOP;
	aml_i2c_set_token_list(i2c);
	aml_i2c_start_token_xfer(i2c);
}

static int aml_i2c_read(struct aml_i2c *i2c, unsigned char *buf, 
							size_t len) 
{
	int i;
	int ret;
	size_t rd_len;
	int tagnum=0;

	aml_i2c_clear_token_list(i2c);
	
	if(! (i2c->msg_flags & I2C_M_NOSTART)){
		i2c->token_tag[tagnum++]=TOKEN_START;
		i2c->token_tag[tagnum++]=TOKEN_SLAVE_ADDR_READ;

		aml_i2c_set_token_list(i2c);
		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		udelay(i2c->wait_ack_interval);
		
		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;	
		aml_i2c_clear_token_list(i2c);
	}
	
	while(len){
		tagnum = 0;
		rd_len = min_t(size_t, len, AML_I2C_MAX_TOKENS);
		if(rd_len == 1)
			i2c->token_tag[tagnum++]=TOKEN_DATA_LAST;
		else{
			for(i=0; i<rd_len-1; i++)
				i2c->token_tag[tagnum++]=TOKEN_DATA;
			if(len > rd_len)
				i2c->token_tag[tagnum++]=TOKEN_DATA;
			else
				i2c->token_tag[tagnum++]=TOKEN_DATA_LAST;
		}
		aml_i2c_set_token_list(i2c);
		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		udelay(i2c->wait_ack_interval);
		
		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;	
		
		aml_i2c_get_read_data(i2c, buf, rd_len);
		len -= rd_len;
		buf += rd_len;

		aml_i2c_dbg(i2c);
		udelay(i2c->wait_read_interval);
		aml_i2c_clear_token_list(i2c);
	}
	return 0;
}

static int aml_i2c_write(struct aml_i2c *i2c, unsigned char *buf, 
							size_t len) 
{
        int i;
        int ret;
        size_t wr_len;
	int tagnum=0;

	aml_i2c_clear_token_list(i2c);
	if(! (i2c->msg_flags & I2C_M_NOSTART)){
		i2c->token_tag[tagnum++]=TOKEN_START;
		i2c->token_tag[tagnum++]=TOKEN_SLAVE_ADDR_WRITE;
	}
	while(len){
		wr_len = min_t(size_t, len, AML_I2C_MAX_TOKENS-tagnum);
		for(i=0; i<wr_len; i++)
			i2c->token_tag[tagnum++]=TOKEN_DATA;
		
		aml_i2c_set_token_list(i2c);
		
		aml_i2c_fill_data(i2c, buf, wr_len);
		
		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		len -= wr_len;
		buf += wr_len;
		tagnum = 0;

		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;
		
		aml_i2c_clear_token_list(i2c);
    	}
	return 0;
}

static struct aml_i2c_ops aml_i2c_m1_ops = {
	.xfer_prepare 	= aml_i2c_xfer_prepare,
	.read 		= aml_i2c_read,
	.write 		= aml_i2c_write,
	.do_address	= aml_i2c_do_address,
	.stop		= aml_i2c_stop,
};

/*General i2c master transfer*/
static int aml_i2c_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, 
							int num)
{
	struct aml_i2c *i2c = &aml_i2c_ddata;
	struct i2c_msg * p;
	unsigned int i;
	unsigned int ret=0;
	
	mutex_lock(&i2c->lock);
	
	i2c->ops->xfer_prepare(i2c);

	for (i = 0; !ret && i < num; i++) {
		p = &msgs[i];
		i2c->msg_flags = p->flags;
		ret = i2c->ops->do_address(i2c, p->addr, p->buf, p->flags & I2C_M_RD, p->len);
		if (ret || !p->len)
			continue;
		if (p->flags & I2C_M_RD)
			ret = i2c->ops->read(i2c, p->buf, p->len);
		else
			ret = i2c->ops->write(i2c, p->buf, p->len);
	}
	
	i2c->ops->stop(i2c);

	mutex_unlock(&i2c->lock);
	
	if (p->flags & I2C_M_RD){
		AML_I2C_DBG("read ");
	}
	else {
		AML_I2C_DBG("write ");
	}
	for(i=0;i<p->len;i++)
		AML_I2C_DBG("%x-",*(p->buf)++);
	AML_I2C_DBG("\n");
	
	/* Return the number of messages processed, or the error code*/
	if (ret == 0)
		return num;
	else {
		printk("[aml_i2c_xfer] error ret = %d \t", ret);
		printk( "i2c master %s current slave addr is 0x%x\n", 
						i2c->master_no?"a":"b", i2c->cur_slave_addr);
		return ret;
	}
}

/*General i2c master transfer 100k*/
static int aml_i2c_xfer_slow(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, 
							int num)
{
	struct aml_i2c *i2c = &aml_i2c_ddata;
	struct i2c_msg * p;
	unsigned int i;
	unsigned int ret=0;
	unsigned int last_speed = i2c->master_i2c_speed;
	
	mutex_lock(&i2c->lock);

	i2c->master_i2c_speed = AML_I2C_SPPED_100K;/* change speed in i2c->lock*/
	i2c->ops->xfer_prepare(i2c);

	for (i = 0; !ret && i < num; i++) {
		p = &msgs[i];
		i2c->msg_flags = p->flags;
		ret = i2c->ops->do_address(i2c, p->addr, p->buf, p->flags & I2C_M_RD, p->len);
		if (ret || !p->len)
			continue;
		if (p->flags & I2C_M_RD)
			ret = i2c->ops->read(i2c, p->buf, p->len);
		else
			ret = i2c->ops->write(i2c, p->buf, p->len);
	}
	
	i2c->ops->stop(i2c);

	mutex_unlock(&i2c->lock);

	AML_I2C_DBG("aml_i2c_xfer_slow");
	if (p->flags & I2C_M_RD){
		AML_I2C_DBG("read ");
	}
	else {
		AML_I2C_DBG("write ");
	}
	for(i=0;i<p->len;i++)
		AML_I2C_DBG("%x-",*(p->buf)++);
	AML_I2C_DBG("\n");
	
	i2c->master_i2c_speed = last_speed;
	/* Return the number of messages processed, or the error code*/
	if (ret == 0)
		return num;
	else {
		struct aml_i2c_reg_ctrl* ctrl;
		
		//printk("i2c master %s current slave addr is 0x%x \t", 
						//i2c->master_no?"a":"b", i2c->cur_slave_addr);

		ctrl = ((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl));
		//if(ctrl->cur_token == TOKEN_START)
			//printk("error addr\n");
		//else
			//printk("error data\n");
		return ret;
	}
}
static u32 aml_i2c_func(struct i2c_adapter *i2c_adap)
{
	return I2C_FUNC_I2C;
}

static const struct i2c_algorithm aml_i2c_algorithm = { 
	.master_xfer = aml_i2c_xfer, 
	.functionality = aml_i2c_func, 
};

static struct i2c_adapter aml_i2c_adapter = { 
	.owner = THIS_MODULE, 
	.name = "aml_i2c_adapter", 
	.id		= I2C_DRIVERID_AML,
	.class = I2C_CLASS_HWMON, 
	.algo = &aml_i2c_algorithm,
	.nr = 0,
};

static const struct i2c_algorithm aml_i2c_algorithm_slow = { 
	.master_xfer = aml_i2c_xfer_slow, 
	.functionality = aml_i2c_func, 
};

static struct i2c_adapter aml_i2c_adapter_slow = { 
	.owner = THIS_MODULE, 
	.name = "aml_i2c_adapter_slow", 
	.id		= I2C_DRIVERID_AML_SLOW,
	.class = I2C_CLASS_HWMON, 
	.algo = &aml_i2c_algorithm_slow,
	.nr = 1,
};

/***************i2c class****************/

static ssize_t show_i2c_debug(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
	struct aml_i2c *i2c = &aml_i2c_ddata;
	return sprintf(buf, "i2c debug is 0x%x\n", i2c->i2c_debug);
}

static ssize_t store_i2c_debug(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned int dbg;
	ssize_t r;
	struct aml_i2c *i2c = &aml_i2c_ddata;

	r = sscanf(buf, "%d", &dbg);
	if (r != 1)
	return -EINVAL;

	i2c->i2c_debug = dbg;
	return count;
}

static ssize_t show_i2c_info(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
	struct aml_i2c *i2c = &aml_i2c_ddata;
	struct aml_i2c_reg_ctrl* ctrl;

	printk( "i2c master %s current slave addr is 0x%x\n", 
						i2c->master_no?"a":"b", i2c->cur_slave_addr);
	printk( "wait ack timeout is 0x%x\n", 
							i2c->wait_count * i2c->wait_ack_interval);
	printk( "master regs base is 0x%x \n", 
								(unsigned int)(i2c->master_regs));

	ctrl = ((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl));
	printk( "i2c_ctrl:  0x%x\n", i2c->master_regs->i2c_ctrl);
	printk( "ctrl.rdsda  0x%x\n", ctrl->rdsda);
	printk( "ctrl.rdscl  0x%x\n", ctrl->rdscl);
	printk( "ctrl.wrsda  0x%x\n", ctrl->wrsda);
	printk( "ctrl.wrscl  0x%x\n", ctrl->wrscl);
	printk( "ctrl.manual_en  0x%x\n", ctrl->manual_en);
	printk( "ctrl.clk_delay  0x%x\n", ctrl->clk_delay);
	printk( "ctrl.rd_data_cnt  0x%x\n", ctrl->rd_data_cnt);
	printk( "ctrl.cur_token  0x%x\n", ctrl->cur_token);
	printk( "ctrl.error  0x%x\n", ctrl->error);
	printk( "ctrl.status  0x%x\n", ctrl->status);
	printk( "ctrl.ack_ignore  0x%x\n", ctrl->ack_ignore);
	printk( "ctrl.start  0x%x\n", ctrl->start);
	
	printk( "i2c_slave_addr:  0x%x\n", 
								i2c->master_regs->i2c_slave_addr);
	printk( "i2c_token_list_0:  0x%x\n", 
								i2c->master_regs->i2c_token_list_0);
	printk( "i2c_token_list_1:  0x%x\n", 
								i2c->master_regs->i2c_token_list_1);
	printk( "i2c_token_wdata_0:  0x%x\n", 
								i2c->master_regs->i2c_token_wdata_0);
	printk( "i2c_token_wdata_1:  0x%x\n", 
								i2c->master_regs->i2c_token_wdata_1);
	printk( "i2c_token_rdata_0:  0x%x\n", 
								i2c->master_regs->i2c_token_rdata_0);
	printk( "i2c_token_rdata_1:  0x%x\n", 
								i2c->master_regs->i2c_token_rdata_1);
								
	printk( "master pinmux\n");
	printk( "scl_reg:  0x%x\n", i2c->master_pinmux.scl_reg);
	printk( "scl_bit:  0x%x\n", i2c->master_pinmux.scl_bit);
	printk( "sda_reg:  0x%x\n", i2c->master_pinmux.sda_reg);
	printk( "sda_bit:  0x%x\n", i2c->master_pinmux.sda_bit);

	return 0;
}

static ssize_t store_register(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned int reg, val, ret;
	int n=1,i;
	if(buf[0] == 'w'){
		ret = sscanf(buf, "w %x %x", &reg, &val);
		//printk("sscanf w reg = %x, val = %x\n",reg, val);
		printk("write cbus reg 0x%x value %x\n", reg, val);
		WRITE_CBUS_REG(reg, val);
	}
	else{
		ret =  sscanf(buf, "%x %d", &reg,&n);
		printk("read %d cbus register from reg: %x \n",n,reg);
		for(i=0;i<n;i++)
		{
			val = READ_CBUS_REG(reg+i);
			printk("reg 0x%x : 0x%x\n", reg+i, val);
		}
	}
	
	if (ret != 1 || ret !=2)
		return -EINVAL;

	return 0;
}

static unsigned int clock81_reading(void)
{
	int val;
	
	val = READ_CBUS_REG(0x1070);
	printk( "1070=%x\n", val);
	val = READ_CBUS_REG(0x105d);
	printk( "105d=%x\n", val);
	return 148;
}

static ssize_t rw_special_reg(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned int id, val, ret;
	
	if(buf[0] == 'w'){
		ret = sscanf(buf, "w %x", &id);
		switch(id)
		{
			case 0:
				break;
			default:
				printk( "'echo h > customize' for help\n");
				break;
		}
		//printk("sscanf w reg = %x, val = %x\n",reg, val);
		//printk("write cbus reg 0x%x value %x\n", reg, val);
		//WRITE_CBUS_REG(reg, val);
	}
	else if(buf[0] == 'r'){
		ret =  sscanf(buf, "r %x", &id);
		switch(id)
		{
			case 0:
				val = clock81_reading();
				printk("Reading Value=%04d Mhz\n", val);
				break;
			default:
				printk( "'echo h > customize' for help\n");
				break;
		}
		//printk("sscanf r reg = %x\n", reg);
		//val = READ_CBUS_REG(reg);
		//printk("read cbus reg 0x%x value %x\n", reg, val);
	}
	else if(buf[0] == 'h'){
		printk( "Customize sys fs help\n");
		printk( "**********************************************************\n");
		printk( "This interface for customer special register value getting\n");
		printk( "echo w id > customize: for write the value to customer specified register\n");
		printk( "echo r id > customize: for read the value from customer specified register\n");
		printk( "reading ID: 0--for clock81 reading\n");
		printk( "writting ID: reserved currently \n");
		printk( "**********************************************************\n");
	}
	else
		printk( "'echo h > customize' for help\n");

	if (ret != 1 || ret !=2)
		return -EINVAL;
		
	return 0;
}

static struct class_attribute i2c_class_attrs[] = {
    __ATTR(debug,  S_IRUGO | S_IWUSR, show_i2c_debug,    store_i2c_debug),
    __ATTR(info,       S_IRUGO | S_IWUSR, show_i2c_info,    NULL),
    __ATTR(cbus_reg,  S_IRUGO | S_IWUSR, NULL,    store_register),
    __ATTR(customize,  S_IRUGO | S_IWUSR, NULL,    rw_special_reg),
    __ATTR_NULL
};

static struct class i2c_class = {
    .name = "i2c",
    .class_attrs = i2c_class_attrs,
};

static int aml_i2c_probe(struct platform_device *pdev) 
{
	int ret;
	
	struct aml_i2c_platform *plat = pdev->dev.platform_data;
	struct resource *res; 
	struct aml_i2c *i2c = &aml_i2c_ddata;

	i2c->ops = &aml_i2c_m1_ops;
	i2c->master_no = plat->master_no;
	i2c->use_pio = plat->use_pio;
	AML_I2C_ASSERT((i2c->master_no >= 0) && (i2c->master_no <= 1));

	/*master a or master b*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, i2c->master_no);
	i2c->master_regs = (struct aml_i2c_reg_master __iomem*)(res->start);

	AML_I2C_ASSERT(i2c->master_regs);
	AML_I2C_ASSERT(plat);
	aml_i2c_set_platform_data(i2c, plat);

	/*lock init*/
	mutex_init(&i2c->lock);

	aml_i2c_hw_init(i2c , i2c->use_pio);

	ret = i2c_add_numbered_adapter(&aml_i2c_adapter);
	i2c->adap = &aml_i2c_adapter;

	if (ret < 0) 
	{
		dev_err(&pdev->dev, "Adapter %s registration failed\n",	 
			aml_i2c_adapter.name);
		return -1;
	}
	dev_info(&pdev->dev, "add adapter %s\n", aml_i2c_adapter.name);
	
	ret = i2c_add_numbered_adapter(&aml_i2c_adapter_slow);
	i2c->adap = &aml_i2c_adapter_slow;
	if (ret < 0) 
	{
		dev_err(&pdev->dev, "Adapter %s registration failed\n",	 
			aml_i2c_adapter.name);
		return -1;
	}
	dev_info(&pdev->dev, "add adapter %s\n", aml_i2c_adapter_slow.name);
	
	dev_info(&pdev->dev, "aml i2c bus driver.\n");
	
   	ret = class_register(&i2c_class);
	if(ret){
		printk(" class register i2c_class fail!\n");
	}
	
	return 0;
}



static int aml_i2c_remove(struct platform_device *pdev) 
{
	i2c_del_adapter(&aml_i2c_adapter);
	return 0;
}

static struct platform_driver aml_i2c_driver = { 
	.probe = aml_i2c_probe, 
	.remove = aml_i2c_remove, 
	.driver = {
			.name = "aml-i2c",						
			.owner = THIS_MODULE,						
	}, 
};

static int __init aml_i2c_init(void) 
{
	return platform_driver_register(&aml_i2c_driver);
}

static void __exit aml_i2c_exit(void) 
{
	platform_driver_unregister(&aml_i2c_driver);
} 

arch_initcall(aml_i2c_init);
module_exit(aml_i2c_exit);

MODULE_AUTHOR("AMLOGIC");
MODULE_DESCRIPTION("I2C driver for amlogic");
MODULE_LICENSE("GPL");

