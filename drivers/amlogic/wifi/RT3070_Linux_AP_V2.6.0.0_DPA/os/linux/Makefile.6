include $(RT28xx_DIR)/os/linux/config.mk

#ifdef CONFIG_AP_SUPPORT
ifeq ($(RT28xx_MODE),AP)
MOD_NAME = rt$(CHIPSET)ap
DAT_PATH = /etc/Wireless/RT$(CHIPSET_DAT)AP
DAT_FILE_NAME = RT$(CHIPSET_DAT)AP.dat
endif
#endif // CONFIG_AP_SUPPORT //



obj-m := $(MOD_NAME).o

#ifdef CONFIG_AP_SUPPORT
rt$(CHIPSET)ap-objs := \
	../../common/crypt_md5.o\
	../../common/crypt_sha2.o\
	../../common/crypt_hmac.o\
	../../common/crypt_aes.o\
	../../common/crypt_arc4.o\
	../../common/mlme.o\
	../../common/cmm_wep.o\
	../../common/action.o\
	../../common/cmm_data.o\
	../../common/rtmp_init.o\
	../../common/rtmp_init_inf.o\
	../../common/cmm_tkip.o\
	../../common/cmm_aes.o\
	../../common/cmm_sync.o\
	../../common/eeprom.o\
	../../common/cmm_sanity.o\
	../../common/cmm_info.o\
	../../common/cmm_cfg.o\
	../../common/cmm_wpa.o\
	../../common/dfs.o\
	../../common/spectrum.o\
	../../common/rtmp_timer.o\
	../../common/rt_channel.o\
	../../common/cmm_profile.o\
	../../common/cmm_asic.o\
	../../common/cmm_cmd.o\
	../../os/linux/rt_profile.o\
	../../ap/ap_mbss.o\
	../../chips/rtmp_chip.o\
	../../ap/ap.o\
	../../ap/ap_assoc.o\
	../../ap/ap_auth.o\
	../../ap/ap_connect.o\
	../../ap/ap_mlme.o\
	../../ap/ap_sanity.o\
	../../ap/ap_sync.o\
	../../ap/ap_wpa.o\
	../../ap/ap_data.o\
	../../ap/ap_uapsd.o\
	../../ap/ap_autoChSel.o\
	../../ap/ap_qload.o\
	../../ap/ap_cfg.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../ap/ap_mbss_inf.o\
	../../common/rt_os_util.o\
	../../os/linux/ap_ioctl.o\
	../../os/linux/rt_linux.o\
	../../os/linux/rt_main_dev.o
else
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_symb.o
endif

ifeq ($(CHIPSET),PCI)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../os/linux/rt_rbus_pci_drv.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rt_rf.o\
	../../chips/rt28xx.o\
	../../chips/rt30xx.o\
	../../chips/rt35xx.o\
	../../chips/rt3090.o\
	../../chips/rt33xx.o\
	../../chips/rt3390.o\
	../../chips/rt5390.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/pci_main_dev.o
endif
endif

ifeq ($(CHIPSET),USB)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rtmp_mcu.o\
	../../common/rt_rf.o\
	../../common/rtusb_bulk.o\
	../../os/linux/rt_usb.o\
	../../chips/rt28xx.o\
	../../chips/rt3070.o\
	../../chips/rt30xx.o\
	../../chips/rt33xx.o\
	../../chips/rt35xx.o\
	../../chips/rt3370.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),RBUS)
rt$(CHIPSET)sta-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../os/linux/rt_rbus_pci_drv.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/ee_flash.o\
	../../common/rt_rf.o\
	../../common/rtmp_swmcu.o\
	../../chips/rt305x.o\
	../../chips/rt5350.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/rbus_main_dev.o
endif
endif


ifeq ($(CHIPSET),RBUS)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../os/linux/rt_rbus_pci_drv.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/ee_flash.o\
	../../common/rt_rf.o\
	../../common/rtmp_swmcu.o\
	../../chips/rt305x.o\
	../../chips/rt5350.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/rbus_main_dev.o
endif
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/ba_action.o
endif
#endif // DOT11_N_SUPPORT //

#ifdef BG_FT_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_BGFP_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../os/linux/br_ftph.o
endif
endif
#endif // BG_FT_SUPPORT //

ifeq ($(PLATFORM),BL2348)
rtutil$(CHIPSET)ap-objs += \
	../../os/linux/vr_bdlt.o
endif

ifeq ($(PLATFORM),BLUBB)
rtutil$(CHIPSET)ap-objs += \
	../../os/linux/vr_bdlt.o
endif

#ifdef CRDA_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../os/linux/cfg80211.o\
	../../os/linux/cfg80211drv.o
endif
endif

ifeq ($(OSABL),YES)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../os/linux/cfg80211drv.o
endif
endif
#endif // CRDA_SUPPORT //

#ifdef LED_CONTROL_SUPPORT
ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/rt_led.o
endif
#endif // LED_CONTROL_SUPPORT //




#ifdef RT3070
ifeq ($(CHIPSET),2070)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/rtusb_bulk.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../chips/rt30xx.o\
	../../common/rt_rf.o\
	../../os/linux/rt_usb.o\
	../../chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),3070)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/rtusb_bulk.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rt_rf.o\
	../../os/linux/rt_usb.o\
	../../chips/rt30xx.o\
	../../chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif
#endif // RT3070 //


#ifdef RT3052
ifeq ($(CHIPSET),3052)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o

ifeq ($(HAS_DFS_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/dfs_mcu.o
endif

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_proc.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/rt_rbus_pci_drv.o\
	../../os/linux/rbus_main_dev.o
endif
endif
#endif // RT3052 //


#ifdef RT5350
ifeq ($(CHIPSET),5350)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o\
	../../chips/rt5350.o

ifeq ($(HAS_DFS_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/dfs_mcu.o
endif

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_proc.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/rt_rbus_pci_drv.o\
	../../os/linux/rbus_main_dev.o
endif
endif
#endif // RT5350 //



#ifdef RT3370
ifeq ($(CHIPSET),3370)
rt$(CHIPSET)ap-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/rtmp_mcu.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rt_rf.o\
	../../common/rtusb_bulk.o\
	../../os/linux/rt_usb.o\
	../../chips/rt33xx.o\
	../../chips/rt3370.o\
	../../chips/rt3070.o\
	../../chips/rt30xx.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif
#endif // RT3370 //


#ifdef RT5370
ifeq ($(CHIPSET),5370)
rt$(CHIPSET)ap-objs += \
        ../../common/cmm_mac_usb.o\
        ../../common/rtusb_io.o\
        ../../common/rtusb_data.o\
        ../../common/cmm_data_usb.o\
        ../../common/rtmp_mcu.o\
        ../../common/ee_prom.o\
        ../../common/ee_efuse.o\
        ../../common/rt_rf.o\
        ../../common/rtusb_bulk.o\
        ../../os/linux/rt_usb.o\
        ../../chips/rt33xx.o\
        ../../chips/rt3370.o\
        ../../chips/rt3070.o\
        ../../chips/rt30xx.o\
	../../chips/rt5390.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
        ../../common/rtusb_dev_id.o\
        ../../os/linux/rt_usb_util.o\
        ../../os/linux/usb_main_dev.o
endif
endif
#endif // RT5370 //

ifeq ($(HAS_RT2880_RT2860_COEXIST),y)	
RT28XX_AP_OBJ += \
	../../os/linux/rt_pci_rbus.o\
	../../os/linux/rt_rbus_pci_util.o\
	../../os/linux/pci_main_dev.o\
	../../common/dfs.o
endif

ifeq ($(HAS_ATE),y)
rt$(CHIPSET)ap-objs += ../../common/rt_ate.o
endif

#ifdef WSC_INCLUDED
ifeq ($(HAS_WSC),y)
rt$(CHIPSET)ap-objs += \
	../../common/wsc.o\
	../../common/wsc_tlv.o\
	../../common/wsc_ufd.o\
	../../common/crypt_biginteger.o\
	../../common/crypt_dh.o	
endif
ifeq ($(HAS_WSC_V2),y)
rt$(CHIPSET)ap-objs += ../../common/wsc_v2.o
endif
#endif // WSC_INCLUDED //



ifeq ($(HAS_WDS),y)
rt$(CHIPSET)ap-objs += ../../ap/ap_wds.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../ap/ap_wds_inf.o
endif
endif

#ifdef APCLI_SUPPORT
ifeq ($(HAS_APCLI),y)
rt$(CHIPSET)ap-objs += \
	../../ap/ap_apcli.o \
	../../ap/apcli_ctrl.o \
	../../ap/apcli_sync.o \
	../../ap/apcli_auth.o \
	../../ap/apcli_assoc.o \
	../../common/cmm_mat.o \
	../../common/cmm_mat_iparp.o \
	../../common/cmm_mat_pppoe.o \
	../../common/cmm_mat_ipv6.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += \
	../../ap/ap_apcli_inf.o
endif
endif
#endif // APCLI_SUPPORT //

ifeq ($(HAS_BLOCK_NET_IF),y)
rt$(CHIPSET)ap-objs += ../../common/netif_block.o
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
rt$(CHIPSET)ap-objs += ../../common/igmp_snoop.o
endif


ifeq ($(HAS_QOS_DLS_SUPPORT),y)
rt$(CHIPSET)ap-objs += ../../ap/ap_dls.o
endif

ifeq ($(HAS_IDS_SUPPORT),y)
rt$(CHIPSET)ap-objs += ../../ap/ap_ids.o
endif

ifeq ($(PLATFORM),IKANOS_V160)
rt$(CHIPSET)ap-objs += ../../os/linux/vr_ikans.o
endif

ifeq ($(PLATFORM),IKANOS_V180)
rt$(CHIPSET)ap-objs += ../../os/linux/vr_ikans.o
endif

ifeq ($(PLATFORM),BL2348)
ifeq ($(OSABL),NO)
rt$(CHIPSET)ap-objs += ../../os/linux/vr_bdlt.o
endif
endif


#ifdef WAPI_SUPPORT
ifeq ($(HAS_WAPI_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/wapi.o
endif
#endif // WAPI_SUPPORT //





ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
rt$(CHIPSET)ap-objs += \
	../../common/client_wds.o
endif

ifeq ($(HAS_EASY_CONFIG_SETUP_SUPPORT),y)
ifeq ($(PLATFORM),RALINK_3052)
rt$(CHIPSET)ap-objs += lib.a
endif
endif
#endif // CONFIG_AP_SUPPORT //




PHONY := clean install uninstall

clean:
	rm -f ../../common/*.o
	rm -f ../../common/.*.{cmd,flags,d}
	rm -f ../../os/linux/*.{o,ko,mod.{o,c}}
	rm -f ../../os/linux/.*.{cmd,flags,d}
	rm -fr ../../os/linux/.tmp_versions
#Must clean Module.symvers; or you will suffer symbol version not match
#when OS_ABL = YES.
	rm -f ../../os/linux/Module.symvers
	rm -f ../../os/linux/Modules.symvers
	rm -f ../../os/linux/Module.markers
	rm -f ../../os/linux/modules.order
	rm -f ../../chips/*.o
	rm -f ../../chips/.*.{cmd,flags,d}
ifeq ($(RT28xx_MODE),AP)
	rm -f ../../ap/*.o
	rm -f ../../ap/.*.{cmd,flags,d}
else
ifeq ($(RT28xx_MODE),STA)
	rm -f ../../sta/*.o
	rm -f ../../sta/.*.{cmd,flags,d}
else
ifeq ($(RT28xx_MODE),APSTA)
	rm -f ../../ap/*.o
	rm -f ../../ap/.*.{cmd,flags,d}
	rm -f ../../sta/*.o
	rm -f ../../sta/.*.{cmd,flags,d}
endif
endif
endif

install:
	rm -rf $(DAT_PATH)
	$(shell [ ! -f /etc/Wireless ] && mkdir /etc/Wireless)
	mkdir $(DAT_PATH)
	cp $(RT28xx_DIR)/$(DAT_FILE_NAME) $(DAT_PATH)/.
	install -d $(LINUX_SRC_MODULE)
	install -m 644 -c $(addsuffix .ko,$(MOD_NAME)) $(LINUX_SRC_MODULE)
	/sbin/depmod -a ${shell uname -r}

uninstall:
#	rm -rf $(DAT_PATH)
	rm -rf $(addprefix $(LINUX_SRC_MODULE),$(addsuffix .ko,$(MOD_NAME)))
	/sbin/depmod -a ${shell uname -r}

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
