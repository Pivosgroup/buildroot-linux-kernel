#ifndef __EFUSE_H
#define __EFUSE_H

#include <linux/ioctl.h>

#define EFUSE_ENCRYPT_DISABLE   _IO('f', 0x10)
#define EFUSE_ENCRYPT_ENABLE    _IO('f', 0x20)
#define EFUSE_ENCRYPT_RESET     _IO('f', 0x30)


#endif

