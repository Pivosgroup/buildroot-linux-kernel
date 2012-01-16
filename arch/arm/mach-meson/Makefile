#
# Makefile for the linux kernel.
#

# Common support
obj-y := core.o clock.o common.o
obj-m :=
obj-n :=
obj-  :=

obj-$(CONFIG_CPU_FREQ) 		+= cpu.o
obj-$(CONFIG_ARCH_MESON)	+= pinmux.o
obj-$(CONFIG_ARCH_MESON)	+= gpio.o
obj-$(CONFIG_ARCH_MESON)	+= power_gate.o

obj-$(CONFIG_ARCH_MESON)	+= lm.o usbclock.o sataclock.o clk_set.o

obj-$(CONFIG_MACH_MESON_6236M)	+= board-6236m.o
obj-$(CONFIG_MACH_MESON_8626M)	+= board-8626m.o
obj-$(CONFIG_MACH_MESON_6236M_SH)	+= board-6236m-sh.o
obj-$(CONFIG_MACH_MESON_8626M_SH)	+= board-8626m-sh.o
obj-$(CONFIG_MACH_MESON_8226M)	+= board-8226m.o
obj-$(CONFIG_MACH_MESON_8726M)	+= board-8726m.o
obj-$(CONFIG_MACH_MESON_8726M_ARM)	+= board-8726m-arm.o
obj-$(CONFIG_MACH_MESON_STV_MBX_M3)	+= stv-mbx-m3.o
