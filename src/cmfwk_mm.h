/******************************************************************************/
/**
 *  @file   cmfwk_mm.h
 *  @brief  Userland Definition
 */
/*
 *  Copyright 2015 Socionext Inc.
 ******************************************************************************/
#ifndef __CMFWK_MM_H__
#define __CMFWK_MM_H__

/**************************************************************** 
 *  include
 ****************************************************************/
#include "ipcu_userland.h"
#include "cmfwk_std.h"

/******************************************************************** 
 *  Debug Flag
 ********************************************************************/


/******************************************************************** 
 *  Common define definition
 ********************************************************************/
#define PAGE_SHIFT 12
#define PAGE_MASK  (~((1 << PAGE_SHIFT) - 1))

#define D_NON_SHARED                (0x00000000)   /* 0:Non-shared memory mode  */
#define D_SHARED                    (0x00000001)   /* 1:Shared memory mode      */

enum {
	IPCU_MM_BUFF = 0,//Each ipcu ch 
	IPCU_MM_SYNC,    //for ?
	IPCU_MM_RDPT,    //for TS response
	IPCU_MM_WTPT,    //for TS response
	IPCU_MM_MREC,    //TS encoder
	IPCU_MM_TERMIO,  //for ?
	IPCU_MM_STR,     //for string
	IPCU_MM_RBRY,    //for ribery
	IPCU_MM_AUDI,    //for audio
	IPCU_MM_RAW,     //for RAW
	IPCU_MM_YUV,     //for YUV
	IPCU_MM_HEVC,    //for YUV
	IPCU_MM_JPEG,    //for jpeg
	IPCU_MM_CAP,     //for ?
	IPCU_MM_OSD,     //for OSD
	IPCU_MM_AOUT,    //for Audio Out
	IPCU_MM_META,    //for META
	IPCU_MM_LCD_OSD,    // for fb driver
	IPCU_MM_HDMI_OSD,    //for fb driver
	IPCU_MM_RIBERY_S,    //for second ribery area
	IPCU_MM_FW_UPDATE,	//for fw update
	IPCU_MM_ID_MAX,
};

/******************************************************************** 
 *  Table structure
 ********************************************************************/


/******************************************************************** 
 *  Function prototype
 ********************************************************************/
UINT32 FJ_MM_virt_to_phys(UINT32 vaddr);
UINT32 FJ_MM_phys_to_virt(UINT32 paddr);
UINT32 FJ_MM_getPhysAddrInfo(UINT32 id, UINT32* pa, UINT32* sz);
UINT32 FJ_MM_Get_SharedMemory_Max(VOID);


#endif /* __CMFWK_MM_H__ */
