##########################################
######This Amlogic Drivers#######################

##########################################
##	THis the Boot Sequence	Do not changed  unbending
##########################################

###For Display, this is for quick logo show##############
obj-y += display/

#obj-y  += hdmi/


#####for Basic drivers#####


obj-y += uart/

obj-y += nand/

obj-y += net/

obj-y += usb/

obj-$(CONFIG_SATA_DWC_AHCI) += sata/

obj-y += canvas/

obj-y += amports/

obj-y += i2c/

obj-y += input/

obj-y += spi_nor/

obj-$(CONFIG_CMEM) += cmem/

obj-$(CONFIG_JPEGLOGO) += jpeglogo/

obj-$(CONFIG_CARDREADER) += cardreader/

obj-$(CONFIG_MALI) += mali/

obj-$(CONFIG_AML_AUDIO_DSP) += audiodsp/
obj-$(CONFIG_AML_AUDIO_DSP_LOG) += audiodsplog/

obj-$(CONFIG_AML_RTC)+= rtc/

obj-y += tvin/

obj-$(CONFIG_EFUSE)+=efuse/

#####Sound system is later init in kernel default..######################
obj-$(CONFIG_AMAUDIO) += amaudio/

obj-y += hdmi/

obj-$(CONFIG_AM_DEMOD) += demod/
obj-$(CONFIG_AM_DVB) += dvb/



