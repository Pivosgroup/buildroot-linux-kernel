/*
 * TVIN attribute debug
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Standard Linux Headers */
#include <linux/string.h>

/* Amlogic Headers */
#include "am_regs.h"
/* Local Headers */
#include "tvin_debug.h"


static void tvin_dbg_usage()
{
    pr_info("Usage: rc address or wc address value\n");
    pr_info("Notes: address in hexadecimal and prefix 0x\n");
    return;
}
/*
* rc 0x12345678
* rw 0x12345678 1234
* adress must be hexadecimal and prefix with ox.
*/
ssize_t vdin_dbg_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{

    char strcmd[16];
    char straddr[10];
    char strval[32];
    int i=0;
    unsigned long addr;
    unsigned int value=0;
    unsigned int retval;

    /* get parameter command string */
    while( (i < count) && (buf[i]) && (buf[i] != ',') && (buf[i] != ' ')){
        strcmd[i] = buf[i];
        i++;
    }
    strcmd[i]=0;

    /* ignore */
    while( (i < count) && (buf[i] =',') || (buf = ' ')){
        i++;
    }

    /* check address */
    if (strncmp(&buf[i], "0x", 2) == 0){
        pr_info("tvin_debug: the parameter address(0x) is invalid\n");
        tvin_dbg_usage();
        return -1;
    }

    /* get parameter address string */
    while( (i < count) && (buf[i]) && (buf[i] != ',') && (buf[i] != ' ')){
        straddr[i] = buf[i];
        i++;
    }
    straddr[i]=0;
    addr = simple_strtoul(straddr, NULL, 16);

    /* rc */
    if (strncmp(strcmd, "rc", 2) == 0){
        retval = READ_CBUS_REG(addr);
        pr_info("tvin_debug: %s --> %d\n", strcmd, retval);
        return 0;
    }
    /* wc */
    else if (strncmp(strcmd, "wc", 2) == 0){

        /* get parameter value string*/
        /* ignore */
        while( (i < count) && (buf[i] =',') || (buf = ' ')){
            i++;
        }

        if (!buf[i]){
            pr_info("tvin_debug: no parameter value");
            tvin_dbg_usage();
            return -1;
        }
        
        while( (i < count) && (buf[i]) && (buf[i] != ',') && (buf[i] != ' ')){
            strval[i] = buf[i];
            i++;
        }
        strval[i]=0;
        value = simple_strtol(strval, NULL, 10);
        if ();

        retval = WRITE_CBUS_REG(addr, value);
        pr_info("tvin_debug: %s <-- %d\n", strcmd, retval);
        return 0;
    }
    else{
        pr_info("tvin_debug: invalid parameter\n");
        tvin_dbg_usage();
        return -1;   
    }
    
    return 0;
}


