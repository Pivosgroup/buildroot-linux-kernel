
#ifndef NAND_H_INCLUDED
#define NAND_H_INCLUDED
#include "am_regs.h"
/** Register defination **/
#define NFC_BASE			  CBUS_REG_ADDR(NAND_CMD)
#define NFC_OFF_CMD           ((NAND_CMD -NAND_CMD)<<2)
#define NFC_OFF_CFG           ((NAND_CFG -NAND_CMD)<<2)
#define NFC_OFF_DADR          ((NAND_DADR-NAND_CMD)<<2)
#define NFC_OFF_IADR          ((NAND_IADR-NAND_CMD)<<2)
#define NFC_OFF_BUF           ((NAND_BUF -NAND_CMD)<<2)
#define NFC_OFF_INFO          ((NAND_INFO-NAND_CMD)<<2)
#define NFC_OFF_DC            ((NAND_DC  -NAND_CMD)<<2)
#define NFC_OFF_ADR           ((NAND_ADR -NAND_CMD)<<2)
#define NFC_OFF_DL            ((NAND_DL  -NAND_CMD)<<2)
#define NFC_OFF_DH            ((NAND_DH  -NAND_CMD)<<2)

/*
   Common Nand Read Flow
*/
#define CE0         (0xe<<10)
#define CE1         (0xd<<10)
#define CE2         (0xb<<10)
#define CE3         (0x7<<10)
#define CE_NOT_SEL  (0xf<<10)


#define CLE         (0x5<<14)
#define ALE         (0x6<<14)
#define DWR         (0x4<<14)
#define DRD         (0x8<<14)
#define IDLE        (0xc<<14)
#define RB          (0x10<<14)
#define STANDBY     (0xf<<10)

#define ADL  (0xc<<16)
#define ADH  (0xd<<16)
#define AIL  (0xe<<16)
#define AIH  (0xf<<16)
#define M2N  (0x4<<17)
#define N2M  (0x5<<17)
/**
    Nand Flash Controller (M1)
    Global Macros
*/
/**
   Config Group
*/
#define NFC_SET_TIMING(mode,cycles,adjust)    WRITE_CBUS_REG_BITS(NAND_CFG,((cycles)|((adjust&0xf)<<10)|((mode&7)<<5)),0,14)
#define NFC_SET_DMA_MODE(is_apb,spare_only)   WRITE_CBUS_REG_BITS(NAND_CFG,((spare_only<<1)|(is_apb)),14,2)

/**
    CMD relative Macros
    Shortage word . NFCC
*/
#define NFC_CMD_IDLE(ce,time)          ((ce)|IDLE|(time&0x3ff))
#define NFC_CMD_CLE(ce,cmd  )          ((ce)|CLE |(cmd &0x0ff))
#define NFC_CMD_ALE(ce,addr )          ((ce)|ALE |(addr&0x0ff))
#define NFC_CMD_RB(ce,time  )          ((ce)|RB  |(time&0x3ff))
#define NFC_CMD_STANDBY(time)          (STANDBY  |(time&0x3ff))
#define NFC_CMD_ADL(addr)              (ADL     |(addr&0xffff))
#define NFC_CMD_ADH(addr)              (ADH|((addr>>16)&0xffff))
#define NFC_CMD_AIL(addr)              (AIL     |(addr&0xffff))
#define NFC_CMD_AIH(addr)              (AIH|((addr>>16)&0xffff))
#define NFC_CMD_M2N(size,ecc)          (M2N |(ecc<<14)|(1<<16)|(size&0x3fff))
#define NFC_CMD_N2M(size,ecc)          (N2M |(ecc<<14)|(1<<16)|(size&0x3fff))
#define NFC_CMD_DWR(data)              (DWR     |(data&0xff  ))
#define NFC_CMD_DRD(    )              (DRD                   )

/**
    Alias for CMD
*/
#define NFC_CMD_D_ADR(addr)         NFC_CMD_ADL(addr),NFC_CMD_ADH(addr)   
#define NFC_CMD_I_ADR(addr)         NFC_CMD_ADI(addr),NFC_CMD_ADI(addr)   


/**
    Register Operation and Controller Status 
*/
#define NFC_SEND_CMD(cmd)           (WRITE_CBUS_REG(NAND_CMD,cmd))
#define NFC_READ_INFO()             (READ_CBUS_REG(NAND_CMD))
/** ECC defination(M1) */
#define AML_NAND_ECC_NONE             0x0
#define NAND_ECC_REV0             0x1
#define NAND_ECC_REV1             0x2
#define NAND_ECC_REV2             0x3


#define NAND_ECC_BCH9             0x0
#define NAND_ECC_BCH8             0x1
#define NAND_ECC_BCH12            0x2
#define NAND_ECC_BCH16            0x3

/**
    Cmd FIFO control
*/
#define NFC_CMD_FIFO_GO()               (WRITE_CBUS_REG(NAND_CMD,(1<<30)))
#define NFC_CMD_FIFO_RESET()            (WRITE_CBUS_REG(NAND_CMD,(1<<31)))
/**
    ADDR operations
*/
#define NFC_SET_DADDR(a)         (WRITE_CBUS_REG(NAND_DADR,(unsigned)a))
#define NFC_SET_IADDR(a)         (WRITE_CBUS_REG(NAND_IADR,(unsigned)a))

/**
    Send command directly
*/
#define NFC_SEND_CMD_IDLE(ce,time)          NFC_SEND_CMD((ce)|IDLE|(time&0x3ff))
#define NFC_SEND_CMD_CLE(ce,cmd  )          NFC_SEND_CMD((ce)|CLE |(cmd &0x0ff))
#define NFC_SEND_CMD_ALE(ce,addr )          NFC_SEND_CMD((ce)|ALE |(addr&0x0ff))
#define NFC_SEND_CMD_RB(ce,time  )          NFC_SEND_CMD((ce)|RB  |(time&0x3ff))
#define NFC_SEND_CMD_STANDBY(time)          NFC_SEND_CMD(STANDBY  |(time&0x3ff))
#define NFC_SEND_CMD_ADL(addr)              NFC_SEND_CMD(ADL     |(addr&0xffff))
#define NFC_SEND_CMD_ADH(addr)              NFC_SEND_CMD(ADH|((addr>>16)&0xffff))
#define NFC_SEND_CMD_AIL(addr)              NFC_SEND_CMD(AIL     |(addr&0xffff))
#define NFC_SEND_CMD_AIH(addr)              NFC_SEND_CMD(AIH|((addr>>16)&0xffff))
#define NFC_SEND_CMD_M2N(size,ecc)          NFC_SEND_CMD(M2N |(ecc<<14)|(1<<16)|(size&0x3fff))
#define NFC_SEND_CMD_N2M(size,ecc)          NFC_SEND_CMD(N2M |(ecc<<14)|(1<<16)|(size&0x3fff))
#define NFC_SEND_CMD_DWR(data)              NFC_SEND_CMD(DWR     |(data&0xff  ))
#define NFC_SEND_CMD_DRD( size  )              NFC_SEND_CMD(DRD |  size   )
/**
    Cmd Info Macros
*/
#define NFC_INFO_GET()                      (READ_CBUS_REG(NAND_CMD))
#define NFC_CMDFIFO_SIZE()                  ((NFC_INFO_GET()>>20)&0x1f)
#define NFC_CHECEK_RB_TIMEOUT()             ((NFC_INFO_GET()>>25)&0x1)
#define NFC_GET_RB_STATUS(ce)               (((NFC_INFO_GET()>>26)&(~(ce>>10)))&0xf)
typedef unsigned    t_nfc_info;

#define NAND_INFO_DONE(a)         (((a)>>31)&1)
#define NAND_ECC_ENABLE(a)        (((a)>>30)&1)
#define NAND_ECC_FAIL(a)          (((a)>>29)&1)
#define NAND_ECC_CNT(a)           (((a)>>24)&0x1f)
#define NAND_INFO_DATA_2INFO(a)         ((a)&0xffff)
#define NAND_INFO_DATA_1INFO(a)         ((a)&0xff)


#define NFC_SET_SPARE_ONLY()			(SET_CBUS_REG_MASK(NAND_CFG,1<<15))
#define NFC_CLEAR_SPARE_ONLY()			(CLEAR_CBUS_REG_MASK(NAND_CFG,1<<15))
#define NFC_GET_BUF() 					READ_CBUS_REG(NAND_BUF)
#define NFC_GET_CFG() 					READ_CBUS_REG(NAND_CFG)
#define NFC_SET_CFG(val) 			    WRITE_CBUS_REG(NAND_CFG,(unsigned)val)


typedef unsigned  t_nf_ce;

typedef unsigned  t_ecc_mode;
#define NFC_GET_CE_CODE(a)				(((~(1<<(((unsigned)a)&3)))&0xf)<<10)




struct aml_m1_nand_platform
{
         unsigned int           page_size;
		 unsigned int 			spare_size;
		 unsigned int 			erase_size;
	 	 unsigned long long 	chip_size;
		 unsigned int 			ce_num;
		 unsigned int 			chip_num;
		 unsigned int 			timing_mode;
		 unsigned int 			bch_mode;
		 unsigned int 			encode_size;
		 unsigned int 			onfi_mode;
         unsigned int           nr_partitions;
         struct mtd_partition    *partitions;
};



#endif // NAND_H_INCLUDED

