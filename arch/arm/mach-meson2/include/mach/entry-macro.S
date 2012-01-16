/*
 *
 * arch-arm/mach-meson/include/mach/entry-macro.S
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Low-level IRQ helper macros for AMLOGIC Meson series
 */
#include <mach/hardware.h>

	.macro	disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
	ldr	\base, = CBUS_REG_ADDR(A9_0_IRQ_IN0_INTR_STAT)

	ldr	\irqstat, [\base, #0]
	mov	\irqnr, #0
	teq	\irqstat, #0
	bne	1004f

1001:
	ldr	\irqstat, [\base, #16]
	mov	\irqnr, #32
	teq	\irqstat, #0
	bne	1004f

1002:
	ldr	\irqstat, [\base, #32]
	mov	\irqnr, #64
	teq	\irqstat, #0
	bne	1004f

1003:
	ldr	\irqstat, [\base, #48]
	mov	\irqnr, #96
	teq	\irqstat, #0
	beq	1005f

1004:
	tst	\irqstat, #1
	bne	1005f
	add	\irqnr, \irqnr, #1
	movs	\irqstat, \irqstat, lsr #1
	bne	1004b
1005:		/* EQ will be set if no irqs pending */
	.endm
