#ifndef __EFUSE_H
#define __EFUSE_H

#include <linux/ioctl.h>

#define EFUSE_ENCRYPT_DISABLE   _IO('f', 0x10)
#define EFUSE_ENCRYPT_ENABLE    _IO('f', 0x20)
#define EFUSE_ENCRYPT_RESET     _IO('f', 0x30)
#ifdef __DEBUG
#define __D(fmt, args...) fprintf(stderr, "debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#ifdef __ERROR
#define __E(fmt, args...) fprintf(stderr, "error: " fmt, ## args)
#else
#define __E(fmt, args...)
#endif



#define BCH_T           1
#define BCH_M           8
#define BCH_P_BYTES     30

#define EFUSE_BITS             3072
#define EFUSE_BYTES            384  //(EFUSE_BITS/8)
#define EFUSE_DWORDS            96  //(EFUSE_BITS/32)

#define DOUBLE_WORD_BYTES        4
#define EFUSE_IS_OPEN           (0x01)

#define EFUSE_LICENSE_BYTES		4
#define EFUSE_MACADDR_BYTES		7
#define EFUSE_HDMHDCP_BYTES		310
#define EFUSE_USERIDF_BYTES		48



void efuse_bch_enc(const char *ibuf, int isize, char *obuf);
void efuse_bch_dec(const char *ibuf, int isize, char *obuf);

struct efuse_platform_data {
	loff_t pos;
	size_t count;
	bool (*data_verify)(unsigned char *usid);
};

#endif
