/*
 * VDIN vframe support
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>

#include "tvin_global.h"
#include "vdin_vf.h"


static vframe_t *vdin_vf_peek(void);
static vframe_t *vdin_vf_get(void);
static void vdin_vf_put(vframe_t *vf);

static vfq_t newframe_q, display_q, recycle_q;
static struct vframe_s vfpool[BT656IN_VF_POOL_SIZE];



static inline void ptr_atomic_wrap_inc(u32 *ptr)
{
    u32 i = *ptr;
    i++;
    if (i >= BT656IN_VF_POOL_SIZE)
        i = 0;
    *ptr = i;
}

inline bool vfq_empty(vfq_t *q)
{
	return (q->rd_index == q->wr_index);
}

#if 1
inline bool vfq_empty_newframe(void)
{
    return vfq_empty(&newframe_q);
}

inline bool vfq_empty_display(void)
{
    return vfq_empty(&display_q);
}

inline bool vfq_empty_recycle(void)
{
    return vfq_empty(&recycle_q);
}
#endif

inline void vfq_push(vfq_t *q, vframe_t *vf)
{
    u32 index = q->wr_index;
    q->pool[index] = vf;
	ptr_atomic_wrap_inc(&q->wr_index);
}

#if 1
inline void vfq_push_newframe(vframe_t *vf)
{
    vfq_push(&newframe_q, vf);
}

inline void vfq_push_display(vframe_t *vf)
{
    vfq_push(&display_q, vf);
}

inline void vfq_push_recycle(vframe_t *vf)
{
    vfq_push(&recycle_q, vf);
}
#endif

inline vframe_t *vfq_pop(vfq_t *q)
{
	vframe_t *vf;

	if (vfq_empty(q))
		return NULL;

	vf = q->pool[q->rd_index];

	ptr_atomic_wrap_inc(&q->rd_index);

	return vf;
}

#if 1
inline vframe_t *vfq_pop_newframe(void)
{
    return vfq_pop(&newframe_q);
}

inline vframe_t *vfq_pop_display(void)
{
    return vfq_pop(&display_q);
}

inline vframe_t *vfq_pop_recycle(void)
{
    return vfq_pop(&recycle_q);
}
#endif

static inline vframe_t *vfq_peek(vfq_t *q)
{
	if (vfq_empty(q))
		return NULL;

	return q->pool[q->rd_index];
}

static inline void vfq_init(vfq_t *q)
{
	q->rd_index = q->wr_index = 0;
}

void vdin_vf_init(void)
{
    int i;

	vfq_init(&display_q);
	vfq_init(&recycle_q);
	vfq_init(&newframe_q);

	for (i = 0; i < (BT656IN_VF_POOL_SIZE - 1); ++i)
	{
		vfq_push(&newframe_q, &vfpool[i]);
	}
}

static vframe_t *vdin_vf_peek(void)
{
    return vfq_peek(&display_q);
}

static vframe_t *vdin_vf_get(void)
{
    return vfq_pop(&display_q);
}

static void vdin_vf_put(vframe_t *vf)
{
	vfq_push(&recycle_q, vf);
}


static const struct vframe_provider_s vdin_vf_provider =
{
    .peek = vdin_vf_peek,
    .get  = vdin_vf_get,
    .put  = vdin_vf_put,
};

void vdin_reg_vf_provider(void)
{
    vf_reg_provider(&vdin_vf_provider);
}

void vdin_unreg_vf_provider(void)
{
    vf_unreg_provider();
}






