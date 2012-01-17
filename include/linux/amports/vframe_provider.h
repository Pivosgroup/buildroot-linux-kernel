/*
 * AMLOGIC Audio/Video streaming port driver.
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
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef VFRAME_PROVIDER_H
#define VFRAME_PROVIDER_H

#include <linux/amports/vframe.h>

typedef struct vframe_states {
	int vf_pool_size;
	int buf_free_num;
	int buf_recycle_num;
    int buf_avail_num;
} vframe_states_t;

#define VFRAME_EVENT_PROVIDER_UNREG             1
#define VFRAME_EVENT_PROVIDER_LIGHT_UNREG       2
#define VFRAME_EVENT_PROVIDER_START             3
#define VFRAME_EVENT_PROVIDER_VFRAME_READY      4
#define VFRAME_EVENT_PROVIDER_QUREY_STATE        5 

typedef enum {
	RECEIVER_INACTIVE = 0 ,
	RECEIVER_ACTIVE
}receviver_start_e;

#define VFRAME_EVENT_RECEIVER_GET               0x01
#define VFRAME_EVENT_RECEIVER_PUT               0x02
#define VFRAME_EVENT_RECEIVER_FRAME_WAIT               0x04

typedef struct vframe_provider_s {
    vframe_t * (*peek)(void);
    vframe_t * (*get )(void);
    void       (*put )(vframe_t *);
    int        (*event_cb)(int type, void* data, void* private_data);
	int 	   (*vf_states)(vframe_states_t *states);
} vframe_provider_t;

typedef struct vframe_receiver_op_s {
    int (*event_cb)(int type, void* data, void* private_data);
} vframe_receiver_op_t;

void vf_reg_provider(const vframe_provider_t *p);
void vf_unreg_provider(void);
void vf_light_unreg_provider(void);
unsigned int get_post_canvas(void);
unsigned int vf_keep_current(void);
vframe_receiver_op_t* vf_vm_reg_provider(const vframe_provider_t *p);
vframe_receiver_op_t* vf_vm_unreg_provider(void);
 #ifdef CONFIG_POST_PROCESS_MANAGER
const vframe_receiver_op_t* vf_ppmgr_reg_provider(const struct vframe_provider_s *p);
void vf_ppmgr_reset(void);
void vf_ppmgr_unreg_provider(void);
#endif
#endif /* VFRAME_PROVIDER_H */

