/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_dfs.c

    Abstract:
    Support DFS function.

    Revision History:
    Who       When            What
    --------  ----------      ----------------------------------------------
*/

#include "rt_config.h"

typedef struct _RADAR_DURATION_TABLE
{
	ULONG RDDurRegion;
	ULONG RadarSignalDuration;
	ULONG Tolerance;
} RADAR_DURATION_TABLE, *PRADAR_DURATION_TABLE;

#ifdef CONFIG_AP_SUPPORT

#endif /* CONFIG_AP_SUPPORT */


UCHAR RdIdleTimeTable[MAX_RD_REGION][4] =
{
	{9, 250, 250, 250},		/* CE*/
	{4, 250, 250, 250},		/* FCC*/
	{4, 250, 250, 250},		/* JAP*/
	{15, 250, 250, 250},	/* JAP_W53*/
	{4, 250, 250, 250}		/* JAP_W56*/
};

#ifdef TONE_RADAR_DETECT_SUPPORT
static void ToneRadarProgram(PRTMP_ADAPTER pAd);
static void ToneRadarEnable(PRTMP_ADAPTER pAd);
#endif /* TONE_RADAR_DETECT_SUPPORT */



/*
	========================================================================

	Routine Description:
		Radar channel check routine

	Arguments:
		pAd 	Pointer to our adapter

	Return Value:
		TRUE	need to do radar detect
		FALSE	need not to do radar detect

	========================================================================
*/
BOOLEAN RadarChannelCheck(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			Ch)
{
	int		i;
	BOOLEAN result = FALSE;

	for (i=0; i<pAd->ChannelListNum; i++)
	{
		if (Ch == pAd->ChannelList[i].Channel)
		{
			result = pAd->ChannelList[i].DfsReq;
			break;
		}
	}

	return result;
}



#ifdef CONFIG_AP_SUPPORT
void RTMPPrepareRDCTSFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pDA,
	IN	ULONG			Duration,
	IN  UCHAR           RTSRate,
	IN  ULONG           CTSBaseAddr,
	IN  UCHAR			FrameGap)
{
	RTS_FRAME			RtsFrame;
	PRTS_FRAME			pRtsFrame;
	TXWI_STRUC			TxWI;
	PTXWI_STRUC			pTxWI;
	HTTRANSMIT_SETTING	CtsTransmit;
	UCHAR				Wcid;
	UCHAR				*ptr;
	UINT				i;

	if (CTSBaseAddr == HW_CS_CTS_BASE) /* CTS frame for carrier-sense.*/
		Wcid = CS_CTS_WCID(pAd);
	else if (CTSBaseAddr == HW_DFS_CTS_BASE) /* CTS frame for radar-detection.*/
		Wcid = DFS_CTS_WCID(pAd);
	else
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s illegal address (%lx) for CTS frame buffer.\n", __FUNCTION__, CTSBaseAddr));
		return;
	}

	pRtsFrame = &RtsFrame;
	pTxWI = &TxWI;

	NdisZeroMemory(pRtsFrame, sizeof(RTS_FRAME));
	pRtsFrame->FC.Type    = BTYPE_CNTL;
	pRtsFrame->Duration = (USHORT)Duration;

	/* Write Tx descriptor*/
	pRtsFrame->FC.SubType = SUBTYPE_CTS;
	COPY_MAC_ADDR(pRtsFrame->Addr1, pAd->CurrentAddress);

	if (pAd->CommonCfg.Channel <= 14)
		CtsTransmit.word = 0;
	else
		CtsTransmit.word = 0x4000;

	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, 0, Wcid, 
		10, PID_MGMT, 0, 0,IFS_SIFS, FALSE, &CtsTransmit);
	pTxWI->PacketId = 0x5;

	ptr = (PUCHAR)pTxWI;
#ifdef RT_BIG_ENDIAN
	RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif
	for (i=0; i<TXWI_SIZE; i+=4)  /* 16-byte TXWI field*/
	{
		UINT32 longptr =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, CTSBaseAddr + i, longptr);
		ptr += 4;
	}

	/* update CTS frame content. start right after the 24-byte TXINFO field*/
	ptr = (PUCHAR)pRtsFrame;
#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, ptr, DIR_WRITE, FALSE);
#endif
	for (i=0; i<10; i+=4)
	{
		UINT32 longptr =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, CTSBaseAddr + TXWI_SIZE + i, longptr);
		ptr += 4;
	}

	AsicUpdateRxWCIDTable(pAd, Wcid, pAd->ApCfg.MBSSID[MAIN_MBSSID].Bssid);
}


/* 
    ==========================================================================
    Description:
		change channel moving time for DFS testing.

	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 set ChMovTime=[value]
    ==========================================================================
*/
int Set_ChMovingTime_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg)
{
	UINT8 Value;

	Value = (UINT8) simple_strtol(arg, 0, 10);

	pAd->CommonCfg.RadarDetect.ChMovingTime = Value;

	DBGPRINT(RT_DEBUG_TRACE, ("%s:: %d\n", __FUNCTION__,
		pAd->CommonCfg.RadarDetect.ChMovingTime));

	return TRUE;
}

int Set_LongPulseRadarTh_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg)
{
	UINT8 Value;

	Value = (UINT8) simple_strtol(arg, 0, 10) > 10 ? 10 : simple_strtol(arg, 0, 10);
	
	pAd->CommonCfg.RadarDetect.LongPulseRadarTh = Value;

	DBGPRINT(RT_DEBUG_TRACE, ("%s:: %d\n", __FUNCTION__,
		pAd->CommonCfg.RadarDetect.LongPulseRadarTh));

	return TRUE;
}
#endif /* CONFIG_AP_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
#ifdef CARRIER_DETECTION_SUPPORT
#ifdef TONE_RADAR_DETECT_SUPPORT
static ULONG time[20];
static ULONG idle[20];
static ULONG busy[20];
static ULONG cd_idx=0;
#endif /* CARRIER_DETECTION_SUPPORT */
#endif /* TONE_RADAR_DETECT_SUPPORT */
#if defined (DFS_SUPPORT) || defined (CARRIER_DETECTION_SUPPORT)
#if defined (TONE_RADAR_DETECT_SUPPORT) || defined (RTMP_RBUS_SUPPORT) || defined (DFS_INTERRUPT_SUPPORT)

void RTMPHandleRadarInterrupt(PRTMP_ADAPTER  pAd)
{
#ifdef CARRIER_DETECTION_SUPPORT
#ifdef TONE_RADAR_DETECT_SUPPORT 
	UINT32 value, delta;
	UCHAR bbp=0;
#endif /* TONE_RADAR_DETECT_SUPPORT */
#endif /* CARRIER_DETECTION_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
#ifdef TONE_RADAR_DETECT_V2 
	if(pAd->CommonCfg.carrier_func==TONE_RADAR_V2)
		{
			BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x01);
			BBP_IO_READ8_BY_REG_ID(pAd, BBP_R185, &bbp);
			if ((bbp & 0x1) == 0)
			{
				return;
			}
		}
#endif /* TONE_RADAR_DETECT_V2  */
#ifdef TONE_RADAR_DETECT_SUPPORT
	RTMP_IO_READ32(pAd, PBF_LIFE_TIMER, &value);
	RTMP_IO_READ32(pAd, CH_IDLE_STA, &pAd->CommonCfg.CarrierDetect.idle_time);
	RTMP_IO_READ32(pAd, CH_BUSY_STA, &pAd->CommonCfg.CarrierDetect.busy_time);
	delta = (value >> 4) - pAd->CommonCfg.CarrierDetect.TimeStamp;
	pAd->CommonCfg.CarrierDetect.TimeStamp = value >> 4;

	pAd->CommonCfg.CarrierDetect.OneSecIntCount++;

	if (pAd->CommonCfg.CarrierDetect.Debug)
	{
		if (cd_idx < 20)
		{
			time[cd_idx] = delta;
			idle[cd_idx] = pAd->CommonCfg.CarrierDetect.idle_time;
			busy[cd_idx] = pAd->CommonCfg.CarrierDetect.busy_time;
			cd_idx++;
		}
		else
		{
			int i;
			pAd->CommonCfg.CarrierDetect.Debug = 0;
			for (i = 0; i < 20; i++)
			{
				printk("%3d %4ld %ld %ld\n", i, time[i], idle[i], busy[i]);
			}
			cd_idx = 0;
			
		}
	}
	

	if (pAd->CommonCfg.CarrierDetect.CD_State == CD_NORMAL)
	{
		if ((delta < pAd->CommonCfg.CarrierDetect.criteria) && (pAd->CommonCfg.CarrierDetect.recheck))
			pAd->CommonCfg.CarrierDetect.recheck --;
		else
			if (pAd->CommonCfg.CarrierDetect.recheck<pAd->CommonCfg.CarrierDetect.recheck1)
			pAd->CommonCfg.CarrierDetect.recheck ++;
		if (pAd->CommonCfg.CarrierDetect.recheck == 0)
		{
			/* declare carrier sense*/
			pAd->CommonCfg.CarrierDetect.CD_State = CD_SILENCE;
			/*pAd->CommonCfg.CarrierDetect.recheck = pAd->CommonCfg.CarrierDetect.recheck1;*/

			if (pAd->CommonCfg.CarrierDetect.CarrierDebug == 0)
			{
	
				DBGPRINT(RT_DEBUG_TRACE, ("Carrier Detected\n"));

				/* disconnect all STAs behind AP.*/
				/*MacTableReset(pAd);*/
				
				/* stop all TX actions including Beacon sending.*/
				AsicDisableSync(pAd);
			}
			else
			{
				printk("Carrier Detected\n");
			}
			

		}
	}

	
	if (pAd->CommonCfg.CarrierDetect.Enable)
	{
		ToneRadarProgram(pAd);
		ToneRadarEnable(pAd);
	}
#endif /* TONE_RADAR_DETECT_SUPPORT */
#endif /* CARRIER_DETECTION_SUPPORT */

}
#endif /* define(TONE_RADAR_DETECT_SUPPORT) || defined(DFS_INTERRUPT_SUPPORT) */
#endif /* define(DFS_SUPPORT) || define(CARRIER_DETECTION_SUPPORT) */
#ifdef CARRIER_DETECTION_SUPPORT
VOID CarrierDetectionFsm(
	IN PRTMP_ADAPTER pAd,
	IN UINT32 CurFalseCCA)
{
#define CAR_SAMPLE_NUM 3


	int i;
	BOOLEAN bCarExist = TRUE;
	BCN_TIME_CFG_STRUC csr;
	CD_STATE CurrentState = pAd->CommonCfg.CarrierDetect.CD_State;
	/*UINT32 AvgFalseCCA;*/
	UINT32 FalseCCAThreshold;
	static int Cnt = 0;
	static UINT32 FalseCCA[CAR_SAMPLE_NUM] = {0};

	FalseCCA[(Cnt++) % CAR_SAMPLE_NUM] = CurFalseCCA;
	/*AvgFalseCCA = (FalseCCA[0] + FalseCCA[1] + FalseCCA[2]) / 3;*/
	/*DBGPRINT(RT_DEBUG_TRACE, ("%s: AvgFalseCCA=%d, CurFalseCCA=%d\n", __FUNCTION__, AvgFalseCCA, CurFalseCCA));*/
	DBGPRINT(RT_DEBUG_TRACE, ("%s: FalseCCA[0]=%d, FalseCCA[1]=%d, FalseCCA[2]=%d,\n",
		__FUNCTION__, FalseCCA[0], FalseCCA[1], FalseCCA[2]));

	if ((pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == TRUE))
	{
		FalseCCAThreshold = CARRIER_DETECT_HIGH_BOUNDARY_1;
	}
	else
	{
		FalseCCAThreshold = CARRIER_DETECT_HIGH_BOUNDARY_2;
	}

	switch (CurrentState)
	{
		case CD_NORMAL:
			for (i = 0; i < CAR_SAMPLE_NUM; i++)
				bCarExist &= (FalseCCA[i] > FalseCCAThreshold);
			/*if (AvgFalseCCA > FalseCCAThreshold)*/
			/*if (FalseCCA[0] > FalseCCAThreshold*/
			/*	&& FalseCCA[1] > FalseCCAThreshold*/
			/*	&& FalseCCA[2] > FalseCCAThreshold)*/
			if (bCarExist)
			{
				/* MacTabReset() will clean whole MAC table no matther what kind entry.*/
				/* In the case, the WDS links will be deleted here and never recovery it back again.*/
				/* Ralink STA also added Carrier-Sense function now*/
				/* os it's no necessary to disconnect STAs here.*/
				/* disconnect all STAs behind AP. */
				/*MacTableReset(pAd);*/

				/* change state to CD_SILENCE.*/
				pAd->CommonCfg.CarrierDetect.CD_State = CD_SILENCE;

				/* Stop sending CTS for Carrier Detection.*/

				CARRIER_DETECT_START(pAd, 0);

				/* stop all TX actions including Beacon sending.*/
				AsicDisableSync(pAd);
				if ((pAd->CommonCfg.Channel > 14)
					&& (pAd->CommonCfg.bIEEE80211H == TRUE))
				{
				}

				DBGPRINT(RT_DEBUG_TRACE, ("Carrier signal detected. Change State to CD_SILENCE.\n"));
			}
			break;

		case CD_SILENCE:
			/* check that all TX been blocked properly.*/
			RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);
			if (csr.field.bBeaconGen == 1)
			{
				/* Stop sending CTS for Carrier Detection.*/

				CARRIER_DETECT_START(pAd, 0);
				AsicDisableSync(pAd);
				if ((pAd->CommonCfg.Channel > 14)
					&& (pAd->CommonCfg.bIEEE80211H == TRUE))
				{
				}
			}

			/* check carrier signal.*/
			for (i = 0; i < CAR_SAMPLE_NUM; i++)
				bCarExist &= (FalseCCA[i] < CARRIER_DETECT_LOW_BOUNDARY);

			/*if (AvgFalseCCA < CARRIER_DETECT_LOW_BOUNDARY)*/
			/*if (FalseCCA[0] < CARRIER_DETECT_LOW_BOUNDARY*/
			/*	&& FalseCCA[1] < CARRIER_DETECT_LOW_BOUNDARY*/
			/*	&& FalseCCA[2] < CARRIER_DETECT_LOW_BOUNDARY)*/
			if (bCarExist)
			{
				/* change state to CD_NORMAL.*/
				pAd->CommonCfg.CarrierDetect.CD_State = CD_NORMAL;

				/* Start sending CTS for Carrier Detection.*/
				CARRIER_DETECT_START(pAd, 1);

				/* start all TX actions.*/
				APMakeAllBssBeacon(pAd);
				APUpdateAllBeaconFrame(pAd);
				AsicEnableBssSync(pAd);


				DBGPRINT(RT_DEBUG_TRACE, ("Carrier signal gone. Change State to CD_NORMAL.\n"));
			}
			break;

		default:
			DBGPRINT(RT_DEBUG_ERROR, ("Unknow Carrier Detection state.\n"));
			break;
	}
}

VOID CarrierDetectionCheck(
	IN PRTMP_ADAPTER pAd)
{
	RX_STA_CNT1_STRUC RxStaCnt1;

	if (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
		return;

	RTMP_IO_READ32(pAd, RX_STA_CNT1, &RxStaCnt1.word);

	pAd->RalinkCounters.OneSecFalseCCACnt += RxStaCnt1.field.FalseCca;
	pAd->PrivateInfo.PhyRxErrCnt += RxStaCnt1.field.PlcpErr;
	CarrierDetectionFsm(pAd, RxStaCnt1.field.FalseCca);

}

VOID CarrierDetectStartTrigger(
	IN PRTMP_ADAPTER pAd)
{
	AsicSendCommandToMcu(pAd, 0x62, 0xff, 0x01, 0x00);
}

/* 
    ==========================================================================
    Description:
        Enable or Disable Carrier Detection feature.
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 set CarrierDetect=[1/0]
    ==========================================================================
*/
int Set_CarrierDetect_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg)
{
    POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR apidx = pObj->ioctl_if;

	BOOLEAN CarrierSenseCts;
	UINT Enable;


	if (apidx != MAIN_MBSSID)
		return FALSE;

	Enable = (UINT) simple_strtol(arg, 0, 10);

	pAd->CommonCfg.CarrierDetect.Enable = (BOOLEAN)(Enable == 0 ? FALSE : TRUE);

	if (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
		CarrierSenseCts = 0;
	else
		CarrierSenseCts = 1;

	if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
		CARRIER_DETECT_START(pAd, CarrierSenseCts);
	else
		CARRIER_DETECT_STOP(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s:: %s\n", __FUNCTION__,
		pAd->CommonCfg.CarrierDetect.Enable == TRUE ? "Enable Carrier Detection":"Disable Carrier Detection"));

	return TRUE;
}
#ifdef TONE_RADAR_DETECT_SUPPORT
int Set_CarrierCriteria_Proc(
	IN PRTMP_ADAPTER 	pAd, 
	IN PSTRING			arg)
{
	UINT32 Value;

	Value = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.CarrierDetect.criteria = Value;
	/* If want to carrier detection debug, set Criteria to odd number*/
	/* To stop debug, set Criteria to even number*/
	pAd->CommonCfg.CarrierDetect.CarrierDebug = Value & 1;

	return TRUE;
}


int Set_CarrierReCheck_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg)
{
	pAd->CommonCfg.CarrierDetect.recheck1 = simple_strtol(arg, 0, 10);
	
	return TRUE;
}







static void ToneRadarProgram(PRTMP_ADAPTER pAd)
{
	UCHAR bbp;

#ifdef TONE_RADAR_DETECT_V1
		if(pAd->CommonCfg.carrier_func==TONE_RADAR_V1)
		{	/* programe delta delay & division bit*/
	DBGPRINT(RT_DEBUG_TRACE, ("3090 ToneRadarProgram\n"));

	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0xf0);
	bbp = pAd->CommonCfg.CarrierDetect.delta << 4;
	bbp |= (pAd->CommonCfg.CarrierDetect.div_flag & 0x1) << 3;
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, bbp);
	
	/* program threshold*/
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x34);
			BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff000000) >> 24);
	
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x24);
			BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff0000) >> 16);
	
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x14);
			BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff00) >> 8);

	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x04);
			BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, pAd->CommonCfg.CarrierDetect.threshold & 0xff);
		}
#endif /* TONE_RADAR_DETECT_V1 */


#ifdef TONE_RADAR_DETECT_V2
	if(pAd->CommonCfg.carrier_func==TONE_RADAR_V2)
	{


	/* programe delta delay & division bit*/
		DBGPRINT(RT_DEBUG_TRACE, ("3390/3090A ToneRadarProgram\n"));
	
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x05);
		bbp = pAd->CommonCfg.CarrierDetect.delta;
		bbp |= 0x10<<4;
		bbp |= (pAd->CommonCfg.CarrierDetect.div_flag & 0x1) << 6;
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, bbp);
	/* program *_mask*/
		/*RTMP_CARRIER_IO_WRITE8(pAd, 2, pAd->CommonCfg.CarrierDetect.VGA_Mask);*/
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x02);
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, pAd->CommonCfg.CarrierDetect.VGA_Mask);
		/*RTMP_CARRIER_IO_WRITE8(pAd, 3, pAd->CommonCfg.CarrierDetect.Packet_End_Mask);*/
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x03);
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, pAd->CommonCfg.CarrierDetect.Packet_End_Mask);
	
		/*RTMP_CARRIER_IO_WRITE8(pAd, 4, pAd->CommonCfg.CarrierDetect.Rx_PE_Mask);*/
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x04);
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, pAd->CommonCfg.CarrierDetect.Rx_PE_Mask);
	

	/* program threshold*/
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x09);
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff000000) >> 24);

		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x08);
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff0000) >> 16);
	
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x07);
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, (pAd->CommonCfg.CarrierDetect.threshold & 0xff00) >> 8);
	
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x06);
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, pAd->CommonCfg.CarrierDetect.threshold & 0xff);
	}
	
#endif /* TONE_RADAR_DETECT_V2 */

}

static void ToneRadarEnable(PRTMP_ADAPTER pAd)
{

#ifdef TONE_RADAR_DETECT_V1
if(pAd->CommonCfg.carrier_func==TONE_RADAR_V1)
{
	BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x05);
	DBGPRINT(RT_DEBUG_TRACE, ("3090 ToneRadarEnable\n"));

}
#endif /* TONE_RADAR_DETECT_V1 */

#ifdef TONE_RADAR_DETECT_V2
	if(pAd->CommonCfg.carrier_func==TONE_RADAR_V2)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("3390/3090A ToneRadarEnable\n"));

		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x01);
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, 0x01);
		
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R184, 0x00);
		BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R185, 0x01);

		
	}

#endif /* TONE_RADAR_DETECT_V2 */

}


void NewCarrierDetectionStart(PRTMP_ADAPTER pAd)
{	
	ULONG Value;
	/* Enable Bandwidth usage monitor*/
		
	DBGPRINT(RT_DEBUG_TRACE, ("NewCarrierDetectionStart\n"));
		
	RTMP_IO_READ32(pAd, CH_TIME_CFG, &Value);
	RTMP_IO_WRITE32(pAd, CH_TIME_CFG, Value | 0x1f);
	pAd->CommonCfg.CarrierDetect.recheck1 = CARRIER_DETECT_RECHECK_TIME;

	/*pAd->CommonCfg.CarrierDetect.recheck2 = CARRIER_DETECT_STOP_RECHECK_TIME;*/
	

	/* Init Carrier Detect*/
	if (pAd->CommonCfg.CarrierDetect.Enable)
	{
		pAd->CommonCfg.CarrierDetect.TimeStamp = 0;
		pAd->CommonCfg.CarrierDetect.recheck = pAd->CommonCfg.CarrierDetect.recheck1;
		ToneRadarProgram(pAd);
		ToneRadarEnable(pAd);
	}
	
}

#endif /* TONE_RADAR_DETECT_SUPPORT */
#endif /* CARRIER_DETECTION_SUPPORT */




#ifdef CARRIER_DETECTION_SUPPORT
#ifdef WORKQUEUE_BH
void carrier_sense_workq(struct work_struct *work)
{
	POS_COOKIE pObj = container_of(work, POS_COOKIE, carrier_sense_work);
	PRTMP_ADAPTER pAd = pObj->pAd_va;
	
	CarrierDetectionCheck(pAd);
}
#else
void carrier_sense_tasklet(unsigned long data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	CarrierDetectionCheck(pAd);
}
#endif /* WORKQUEUE_BH */
#endif /* CARRIER_DETECTION_SUPPORT */


#endif /* CONFIG_AP_SUPPORT */

