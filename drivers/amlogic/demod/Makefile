
obj-$(CONFIG_AM_DEMOD) += aml_fe.o

aml_fe-objs := demod_func.o dvbt_func.o dvbc_func.o i2c_func.o tuner_func.o

TUNER_SRCDIR := $(wildcard $(kbuild-dir)/tuner_src)

ifeq ($(TUNER_SRCDIR), )
	TUNER_SRCDIR := $(wildcard $(kbuild-dir)/tuner)
	obj-$(CONFIG_AM_DEMOD) += tuner/
else
	obj-$(CONFIG_AM_DEMOD) += tuner_src/
endif

ifeq ($(CONFIG_AM_DEMOD_DVBAPI), y)
  aml_fe-objs += amlfrontend.o
endif

ifeq ($(CONFIG_AM_DEMOD_DEBUG), y)
  aml_fe-objs	+= aml_demod.o
endif

EXTRA_CFLAGS += -Idrivers/media/dvb/dvb-core -I$(TUNER_SRCDIR)

