#include $(RT28xx_DIR)/os/linux/config.mk

CHIPSET := 3070
RT28xx_MODE := AP

OSABL := NO

RT28xx_DIR = $(shell pwd)

# Support ATE function
HAS_ATE=y

# Support ATE NEW TXCONT solution
HAS_NEW_TXCONT=n

# Support ATE NEW TXCARR solution
HAS_NEW_TXCARR=n

# Support ATE NEW TXCARS solution
HAS_NEW_TXCARS=n

#-----------------------------------------------#
# NOTE : RT2xxx does not support this feature !
# Do not touch this block !!! 
ifeq ($(CHIPSET),2860)
HAS_NEW_TXCONT=n
HAS_NEW_TXCARR=n
HAS_NEW_TXCARS=n
endif
ifeq ($(CHIPSET),2870)
HAS_NEW_TXCONT=n
HAS_NEW_TXCARR=n
HAS_NEW_TXCARS=n
endif
ifeq ($(CHIPSET),2880)
HAS_NEW_TXCONT=n
HAS_NEW_TXCARR=n
HAS_NEW_TXCARS=n
endif
ifeq ($(CHIPSET),2070)
HAS_NEW_TXCONT=n
HAS_NEW_TXCARR=n
HAS_NEW_TXCARS=n
endif
#-----------------------------------------------#

# Support QA ATE function
HAS_QA_SUPPORT=y

HAS_RSSI_FEEDBACK=n

# Support XLINK mode
HAS_XLINK=n

# Support WSC function
HAS_WSC=y
HAS_WSC_LED=y


# Support LLTD function
HAS_LLTD=n

# Support WDS function
HAS_WDS=n

# Support AP-Client function
HAS_APCLI=n




#Support Net interface block while Tx-Sw queue full
HAS_BLOCK_NET_IF=n

#Support IGMP-Snooping function.
HAS_IGMP_SNOOP_SUPPORT=n

#Support DFS function
HAS_DFS_SUPPORT=n

#Support Carrier-Sense function
HAS_CS_SUPPORT=n


# Support user specific transmit rate of Multicast packet.
HAS_MCAST_RATE_SPECIFIC_SUPPORT=n

# Support for Multiple Cards
HAS_MC_SUPPORT=n

#Support for PCI-MSI
HAS_MSI_SUPPORT=n


#Support for IEEE802.11e DLS
HAS_QOS_DLS_SUPPORT=n

#Support for EXT_CHANNEL
HAS_EXT_BUILD_CHANNEL_LIST=n

#Support for IDS 
HAS_IDS_SUPPORT=n


#Support for Net-SNMP
HAS_SNMP_SUPPORT=n

#Support features of 802.11n Draft3
HAS_DOT11N_DRAFT3_SUPPORT=n

#Support features of Single SKU. 
HAS_SINGLE_SKU_SUPPORT=n

#Support features of 802.11n
HAS_DOT11_N_SUPPORT=y

#Support for WAPI
HAS_WAPI_SUPPORT=n

#Support for RT5392 RT5372
HAS_TEMPERATURE_COMPENSATION=n

#Support for 2860/2880 co-exist 
HAS_RT2880_RT2860_COEXIST=n

HAS_KTHREAD_SUPPORT=n





#Support for Auto channel select enhance
HAS_AUTO_CH_SELECT_ENHANCE=n

#Support statistics count
HAS_STATS_COUNT=n


#Support USB_BULK_BUF_ALIGMENT
HAS_USB_BULK_BUF_ALIGMENT=n

#Support for USB_SUPPORT_SELECTIVE_SUSPEND
HAS_USB_SUPPORT_SELECTIVE_SUSPEND=n







#Client support WDS function
HAS_CLIENT_WDS_SUPPORT=n

#Support for Bridge Fast Path & Bridge Fast Path function open to other module
HAS_BGFP_SUPPORT=n
HAS_BGFP_OPEN_SUPPORT=n

# Support HOSTAPD function
HAS_HOSTAPD_SUPPORT=y

#Support GreenAP function
HAS_GREENAP_SUPPORT=n

#Support MAC80211 LINUX-only function
#Please make sure the version for CFG80211.ko and MAC80211.ko is same as the one
#our driver references to.
HAS_CFG80211_SUPPORT=y

#Support RFKILL hardware block/unblock LINUX-only function
HAS_RFKILL_HW_SUPPORT=n




HAS_RTMP_FLASH_SUPPORT=n

ifeq ($(OSABL),YES)
HAS_OSABL_FUNC_SUPPORT=y
HAS_OSABL_OS_PCI_SUPPORT=y
HAS_OSABL_OS_USB_SUPPORT=y
HAS_OSABL_OS_RBUS_SUPPORT=n
HAS_OSABL_OS_AP_SUPPORT=y
HAS_OSABL_OS_STA_SUPPORT=y
endif

HAS_LED_CONTROL_SUPPORT=y


#Support WIDI feature
#Must enable HAS_WSC at the same time.



HAS_RESOURCE_BOOT_ALLOC=n

HAS_REFUSE_SCAN_QUERY_WHILE_SCANING=n

#################################################

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT  -DRT28xx_MODE=$(RT28xx_MODE) -DCHIPSET=$(CHIPSET) -DRESOURCE_PRE_ALLOC

ifeq ($(HAS_RESOURCE_BOOT_ALLOC),y)
WFLAGS += -DRESOURCE_BOOT_ALLOC
endif

ifeq ($(HAS_REFUSE_SCAN_QUERY_WHILE_SCANING),y)
WFLAGS += -DREFUSE_SCAN_QUERY_WHILE_SCANING
endif






ifeq ($(HAS_KTHREAD_SUPPORT),y)
WFLAGS += -DKTHREAD_SUPPORT
endif

ifeq ($(HAS_RTMP_FLASH_SUPPORT),y)
WFLAGS += -DRTMP_FLASH_SUPPORT
endif

# config for AP mode

ifeq ($(RT28xx_MODE),AP)
WFLAGS += -DCONFIG_AP_SUPPORT  -DUAPSD_AP_SUPPORT -DMBSS_SUPPORT -DIAPP_SUPPORT -DDBG -DDOT1X_SUPPORT -DAP_SCAN_SUPPORT

ifeq ($(HAS_HOSTAPD_SUPPORT),y)
WFLAGS += -DHOSTAPD_SUPPORT
endif

ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
WFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
ifeq ($(HAS_NEW_TXCONT),y)
WFLAGS += -DNEW_TXCONT
endif
ifeq ($(HAS_NEW_TXCARR),y)
WFLAGS += -DNEW_TXCARR
endif
ifeq ($(HAS_NEW_TXCARS),y)
WFLAGS += -DNEW_TXCARRSUPP
endif
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_QA
endif
endif

ifeq ($(HAS_RSSI_FEEDBACK),y)
WFLAGS += -DRSSI_FEEDBACK
endif




ifeq ($(HAS_WSC),y)
WFLAGS += -DWSC_AP_SUPPORT

ifeq ($(HAS_WSC_V2),y)
WFLAGS += -DWSC_V2_SUPPORT
endif
ifeq ($(HAS_WSC_LED),y)
WFLAGS += -DWSC_LED_SUPPORT
endif
endif


ifeq ($(HAS_WDS),y)
WFLAGS += -DWDS_SUPPORT
endif

ifeq ($(HAS_APCLI),y)
WFLAGS += -DAPCLI_SUPPORT -DMAT_SUPPORT -DAP_SCAN_SUPPORT
#ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
#WFLAGS += -DETH_CONVERT_SUPPORT
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
WFLAGS += -DIGMP_SNOOP_SUPPORT
endif

ifeq ($(HAS_CS_SUPPORT),y)
WFLAGS += -DCARRIER_DETECTION_SUPPORT
endif

ifeq ($(HAS_MCAST_RATE_SPECIFIC_SUPPORT), y)
WFLAGS += -DMCAST_RATE_SPECIFIC
endif

ifeq ($(CHIPSET),2860)
ifeq ($(HAS_MSI_SUPPORT),y)
WFLAGS += -DPCI_MSI_SUPPORT
endif
endif


ifeq ($(HAS_QOS_DLS_SUPPORT),y)
WFLAGS += -DQOS_DLS_SUPPORT
endif

ifeq ($(HAS_SNMP_SUPPORT),y)
WFLAGS += -DSNMP_SUPPORT
endif


ifeq ($(HAS_SINGLE_SKU_SUPPORT),y)
WFLAGS += -DSINGLE_SKU
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif
endif
ifeq ($(HAS_GREENAP_SUPPORT),y)
WFLAGS += -DGREENAP_SUPPORT
endif

ifeq ($(HAS_AUTO_CH_SELECT_ENHANCE),y)
WFLAGS += -DAUTO_CH_SELECT_ENHANCE
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif


ifeq ($(HAS_USB_BULK_BUF_ALIGMENT),y)
WFLAGS += -DUSB_BULK_BUF_ALIGMENT
endif


ifeq ($(HAS_CFG80211_SUPPORT),y)
WFLAGS += -DRT_CFG80211_SUPPORT
ifeq ($(HAS_RFKILL_HW_SUPPORT),y)
WFLAGS += -DRFKILL_HW_SUPPORT
endif
endif

ifeq ($(OSABL),YES)
WFLAGS += -DOS_ABL_SUPPORT
ifeq ($(HAS_OSABL_FUNC_SUPPORT),y)
WFLAGS += -DOS_ABL_FUNC_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_PCI_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_PCI_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_USB_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_USB_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_RBUS_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_RBUS_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_AP_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_AP_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_STA_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_STA_SUPPORT
endif
endif



endif #// endif of RT2860_MODE == AP //

#################################################


#################################################

#################################################

#
# Common compiler flag
#






ifeq ($(HAS_EXT_BUILD_CHANNEL_LIST),y)
WFLAGS += -DEXT_BUILD_CHANNEL_LIST
endif

ifeq ($(HAS_IDS_SUPPORT),y)
WFLAGS += -DIDS_SUPPORT
endif

ifeq ($(HAS_WAPI_SUPPORT),y)
WFLAGS += -DWAPI_SUPPORT -DSOFT_ENCRYPT -DEXPORT_SYMTAB
endif


ifeq ($(HAS_TEMPERATURE_COMPENSATION),y)
WFLAGS += -DRTMP_TEMPERATURE_COMPENSATION
endif



ifeq ($(OSABL),YES)
WFLAGS += -DEXPORT_SYMTAB
endif

ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
WFLAGS += -DCLIENT_WDS
endif

ifeq ($(HAS_BGFP_SUPPORT),y)
WFLAGS += -DBG_FT_SUPPORT
endif

ifeq ($(HAS_BGFP_OPEN_SUPPORT),y)
WFLAGS += -DBG_FT_OPEN_SUPPORT
endif


ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
WFLAGS += -DLED_CONTROL_SUPPORT
endif

#################################################
# ChipSet specific definitions.
#
ifeq ($(CHIPSET),2860)
WFLAGS +=-DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRT2860 -DRT28xx -DA_BAND_SUPPORT
CHIPSET_DAT = 2860
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif
endif

ifeq ($(CHIPSET),3090)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT3090  -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),2870)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRTMP_TIMER_TASK_SUPPORT -DA_BAND_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif

endif

ifeq ($(CHIPSET),2070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRT2070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),3070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),2880)
WFLAGS += -DRT2880 -DRT28xx -DRTMP_MAC_PCI -DCONFIG_RALINK_RT2880_MP -DRTMP_RBUS_SUPPORT -DMERGE_ARCH_TEAM -DA_BAND_SUPPORT -DCONFIG_SWMCU_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT -DDFS_FCC_BW40_FIX -DDFS_DEBUG
endif
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET),3572)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRT30xx -DRT35xx -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif
endif

ifeq ($(CHIPSET),3062)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRT3062 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3562)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG 
endif

CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3593)
WFLAGS +=-DRTMP_MAC_PCI -DDOT11N_SS3_SUPPORT -DNEW_RATE_ADAPT_SUPPORT -DRT3593 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DNEW_MBSSID_MODE -DSPECIFIC_BCN_BUF_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG
endif

CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3390)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3370)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),5390)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRT5390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),5370)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRT5370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DhSPECIFIC_BCN_BUF_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET), 3052)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3052 -DRT305x -DRTMP_RF_RW_SUPPORT -DCONFIG_SWMCU_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET), 3352)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3352 -DRT305x -DRTMP_RF_RW_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC
CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET), 5350)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT5350 -DRT305x -DRT3050 -DRT3350 -DRTMP_RF_RW_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC -DRTMP_FREQ_CALIBRATION_SUPPORT
CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif




ifeq ($(CHIPSET),USB)
#3572
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRT30xx -DRT35xx -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
#3370
WFLAGS += -DRT33xx -DRT3070 -DRT3370 -DRTMP_TIMER_TASK_SUPPORT -DRTMP_INTERNAL_TX_ALC
CHIPSET_DAT = 2870
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif
endif


ifeq ($(CHIPSET),PCI)
#3562		
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
#3390
WFLAGS +=-DRT33xx -DRT3090 -DRT3390 -DRTMP_INTERNAL_TX_ALC
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG 
endif
endif


ifeq ($(CHIPSET),RBUS)
WFLAGS += -DMERGE_ARCH_TEAM -DCONFIG_SWMCU_SUPPORT -DCONFIG_RA_NAT_NONE -DRTMP_RBUS_SUPPORT
#5350, 3050, 3350
WFLAGS +=-DRTMP_MAC_PCI -DRT305x -DRT5350 -DRT3050 -DRT3350 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DA_BAND_SUPPORT -DVCORECAL_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG 
endif
endif


#################################################

ifeq ($(HAS_BLOCK_NET_IF),y)
WFLAGS += -DBLOCK_NET_IF
endif

ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SUPPORT
endif

ifeq ($(HAS_MC_SUPPORT),y)
WFLAGS += -DMULTIPLE_CARD_SUPPORT
endif

ifeq ($(HAS_LLTD),y)
WFLAGS += -DLLTD_SUPPORT
endif

#ifdef CONFIG_AP_SUPPORT
ifeq ($(RT28xx_MODE),AP)
#MOD_NAME = rt$(CHIPSET)ap
MOD_NAME := wlan
DAT_PATH = /etc/Wireless/RT$(CHIPSET_DAT)AP
DAT_FILE_NAME = RT$(CHIPSET_DAT)AP.dat
endif
#endif // CONFIG_AP_SUPPORT //

EXTRA_CFLAGS := -nostdinc -iwithprefix include -D__KERNEL__ -DCONFIG_LITTLE_ENDIAN -DCONFIG_PLATFORM_ANDROID -I$(RT28xx_DIR)/include -I$(LINUX_SRC)/include -I$(src)/include -mlittle-endian -Iarch/arm/mach-meson/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -c -o $(WFLAGS)
#export EXTRA_CFLAGS

obj-m := $(MOD_NAME).o

#ifdef CONFIG_AP_SUPPORT
$(MOD_NAME)-objs := \
	common/crypt_md5.o\
	common/crypt_sha2.o\
	common/crypt_hmac.o\
	common/crypt_aes.o\
	common/crypt_arc4.o\
	common/mlme.o\
	common/cmm_wep.o\
	common/action.o\
	common/cmm_data.o\
	common/rtmp_init.o\
	common/rtmp_init_inf.o\
	common/cmm_tkip.o\
	common/cmm_aes.o\
	common/cmm_sync.o\
	common/eeprom.o\
	common/cmm_sanity.o\
	common/cmm_info.o\
	common/cmm_cfg.o\
	common/cmm_wpa.o\
	common/dfs.o\
	common/spectrum.o\
	common/rtmp_timer.o\
	common/rt_channel.o\
	common/cmm_profile.o\
	common/cmm_asic.o\
	common/cmm_cmd.o\
	os/linux/rt_profile.o\
	ap/ap_mbss.o\
	chips/rtmp_chip.o\
	ap/ap.o\
	ap/ap_assoc.o\
	ap/ap_auth.o\
	ap/ap_connect.o\
	ap/ap_mlme.o\
	ap/ap_sanity.o\
	ap/ap_sync.o\
	ap/ap_wpa.o\
	ap/ap_data.o\
	ap/ap_uapsd.o\
	ap/ap_autoChSel.o\
	ap/ap_qload.o\
	ap/ap_cfg.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	ap/ap_mbss_inf.o\
	common/rt_os_util.o\
	os/linux/ap_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o
else
$(MOD_NAME)-objs += \
	os/linux/rt_symb.o
endif

ifeq ($(CHIPSET),PCI)
$(MOD_NAME)-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	os/linux/rt_rbus_pci_drv.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	chips/rt28xx.o\
	chips/rt30xx.o\
	chips/rt35xx.o\
	chips/rt3090.o\
	chips/rt33xx.o\
	chips/rt3390.o\
	chips/rt5390.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/pci_main_dev.o
endif
endif

ifeq ($(CHIPSET),USB)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt28xx.o\
	chips/rt3070.o\
	chips/rt30xx.o\
	chips/rt33xx.o\
	chips/rt35xx.o\
	chips/rt3370.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),RBUS)
rt$(CHIPSET)sta-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	os/linux/rt_rbus_pci_drv.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/ee_flash.o\
	common/rt_rf.o\
	common/rtmp_swmcu.o\
	chips/rt305x.o\
	chips/rt5350.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/rbus_main_dev.o
endif
endif


ifeq ($(CHIPSET),RBUS)
$(MOD_NAME)-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	os/linux/rt_rbus_pci_drv.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/ee_flash.o\
	common/rt_rf.o\
	common/rtmp_swmcu.o\
	chips/rt305x.o\
	chips/rt5350.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/rbus_main_dev.o
endif
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/ba_action.o
endif
#endif // DOT11_N_SUPPORT //

#ifdef BG_FT_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_BGFP_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/br_ftph.o
endif
endif
#endif // BG_FT_SUPPORT //

ifeq ($(PLATFORM),BL2348)
rtutil$(CHIPSET)ap-objs += \
	os/linux/vr_bdlt.o
endif

ifeq ($(PLATFORM),BLUBB)
rtutil$(CHIPSET)ap-objs += \
	os/linux/vr_bdlt.o
endif

#ifdef CRDA_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211.o\
	os/linux/cfg80211drv.o
endif
endif

ifeq ($(OSABL),YES)
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211drv.o
endif
endif
#endif // CRDA_SUPPORT //

#ifdef LED_CONTROL_SUPPORT
ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/rt_led.o
endif
#endif // LED_CONTROL_SUPPORT //




#ifdef RT3070
ifeq ($(CHIPSET),2070)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtusb_bulk.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	chips/rt30xx.o\
	common/rt_rf.o\
	os/linux/rt_usb.o\
	chips/rt3070.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),3070)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtusb_bulk.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	os/linux/rt_usb.o\
	chips/rt30xx.o\
	chips/rt3070.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3070 //


#ifdef RT3052
ifeq ($(CHIPSET),3052)
$(MOD_NAME)-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	common/ee_flash.o\
	common/rtmp_swmcu.o\
	common/rt_rf.o\
	chips/rt305x.o

ifeq ($(HAS_DFS_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/dfs_mcu.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_proc.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/rt_rbus_pci_drv.o\
	os/linux/rbus_main_dev.o
endif
endif
#endif // RT3052 //


#ifdef RT5350
ifeq ($(CHIPSET),5350)
$(MOD_NAME)-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	common/ee_flash.o\
	common/rtmp_swmcu.o\
	common/rt_rf.o\
	chips/rt305x.o\
	chips/rt5350.o

ifeq ($(HAS_DFS_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/dfs_mcu.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_proc.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/rt_rbus_pci_drv.o\
	os/linux/rbus_main_dev.o
endif
endif
#endif // RT5350 //



#ifdef RT3370
ifeq ($(CHIPSET),3370)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt33xx.o\
	chips/rt3370.o\
	chips/rt3070.o\
	chips/rt30xx.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3370 //


#ifdef RT5370
ifeq ($(CHIPSET),5370)
$(MOD_NAME)-objs += \
        common/cmm_mac_usb.o\
        common/rtusb_io.o\
        common/rtusb_data.o\
        common/cmm_data_usb.o\
        common/rtmp_mcu.o\
        common/ee_prom.o\
        common/ee_efuse.o\
        common/rt_rf.o\
        common/rtusb_bulk.o\
        os/linux/rt_usb.o\
        chips/rt33xx.o\
        chips/rt3370.o\
        chips/rt3070.o\
        chips/rt30xx.o\
	chips/rt5390.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
        common/rtusb_dev_id.o\
        os/linux/rt_usb_util.o\
        os/linux/usb_main_dev.o
endif
endif
#endif // RT5370 //

ifeq ($(HAS_RT2880_RT2860_COEXIST),y)	
RT28XX_AP_OBJ += \
	os/linux/rt_pci_rbus.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/pci_main_dev.o\
	common/dfs.o
endif

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += common/rt_ate.o
endif

#ifdef WSC_INCLUDED
ifeq ($(HAS_WSC),y)
$(MOD_NAME)-objs += \
	common/wsc.o\
	common/wsc_tlv.o\
	common/wsc_ufd.o\
	common/crypt_biginteger.o\
	common/crypt_dh.o	
endif
ifeq ($(HAS_WSC_V2),y)
$(MOD_NAME)-objs += common/wsc_v2.o
endif
#endif // WSC_INCLUDED //



ifeq ($(HAS_WDS),y)
$(MOD_NAME)-objs += ap/ap_wds.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	ap/ap_wds_inf.o
endif
endif

#ifdef APCLI_SUPPORT
ifeq ($(HAS_APCLI),y)
$(MOD_NAME)-objs += \
	ap/ap_apcli.o \
	ap/apcli_ctrl.o \
	ap/apcli_sync.o \
	ap/apcli_auth.o \
	ap/apcli_assoc.o \
	common/cmm_mat.o \
	common/cmm_mat_iparp.o \
	common/cmm_mat_pppoe.o \
	common/cmm_mat_ipv6.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	ap/ap_apcli_inf.o
endif
endif
#endif // APCLI_SUPPORT //

ifeq ($(HAS_BLOCK_NET_IF),y)
$(MOD_NAME)-objs += common/netif_block.o
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
$(MOD_NAME)-objs += common/igmp_snoop.o
endif


ifeq ($(HAS_QOS_DLS_SUPPORT),y)
$(MOD_NAME)-objs += ap/ap_dls.o
endif

ifeq ($(HAS_IDS_SUPPORT),y)
$(MOD_NAME)-objs += ap/ap_ids.o
endif

ifeq ($(PLATFORM),IKANOS_V160)
$(MOD_NAME)-objs += os/linux/vr_ikans.o
endif

ifeq ($(PLATFORM),IKANOS_V180)
$(MOD_NAME)-objs += os/linux/vr_ikans.o
endif

ifeq ($(PLATFORM),BL2348)
ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += os/linux/vr_bdlt.o
endif
endif


#ifdef WAPI_SUPPORT
ifeq ($(HAS_WAPI_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/wapi.o
endif
#endif // WAPI_SUPPORT //





ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/client_wds.o
endif

ifeq ($(HAS_EASY_CONFIG_SETUP_SUPPORT),y)
ifeq ($(PLATFORM),RALINK_3052)
$(MOD_NAME)-objs += lib.a
endif
endif
#endif // CONFIG_AP_SUPPORT //
