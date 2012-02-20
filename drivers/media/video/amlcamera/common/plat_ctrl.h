#ifndef _AMLOGIC_CAMERA_PLAT_CTRL_H
#define _AMLOGIC_CAMERA_PLAT_CTRL_H

extern int i2c_get_byte(struct i2c_client *client,unsigned short addr);
extern int i2c_get_word(struct i2c_client *client,unsigned short addr);
extern int i2c_get_byte_add8(struct i2c_client *client,unsigned short addr);
extern int i2c_put_byte(struct i2c_client *client, unsigned short addr, unsigned char data);
extern int i2c_put_word(struct i2c_client *client, unsigned short addr, unsigned short data);
extern int i2c_put_byte_add8(struct i2c_client *client,char *buf, int len);


#endif /* _AMLOGIC_CAMERA_PLAT_CTRL_H. */
