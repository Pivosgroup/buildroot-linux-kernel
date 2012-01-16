/*
 * Amlogic Apollo
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
 * Author:	Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef VINFO_H
#define VINFO_H

typedef enum {
    VMODE_480I       = 0,
    VMODE_480CVBS    = 1,		
    VMODE_480P       = 2,
    VMODE_576I       = 3,
    VMODE_576CVBS    = 4,
    VMODE_576P       = 5,
    VMODE_720P       = 6,
    VMODE_720P_50HZ  = 7,
    VMODE_1080I      = 8,
    VMODE_1080I_50HZ = 9,
    VMODE_1080P      = 10,
    VMODE_1080P_50HZ = 11,
    VMODE_LCD        = 12,
    VMODE_MAX        = 13   
} vmode_t;

typedef struct {
	char  		*name;
	vmode_t		mode;
	u32			width;
	u32			height;
	u32			field_height;
	u32			aspect_ratio_num;
	u32			aspect_ratio_den;
	u32			sync_duration_num;
	u32			sync_duration_den;
} vinfo_t;

#endif /* TVMODE_H */
