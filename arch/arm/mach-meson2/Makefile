#
# Makefile for the linux kernel.
#

# Common support
obj-y := core.o clock.o common.o
obj-m :=
obj-n :=
obj-  :=

obj-$(CONFIG_CPU_FREQ) 		+= cpu.o
obj-$(CONFIG_ARCH_MESON2)	+= pinmux.o
obj-$(CONFIG_ARCH_MESON2)	+= gpio.o
obj-$(CONFIG_ARCH_MESON2)	+= power_gate.o
obj-$(CONFIG_ARCH_MESON2)	+= lm.o usbclock.o sataclock.o clk_set.o

obj-$(CONFIG_MACH_MESON2_7366M)	+= board-7366m.o

