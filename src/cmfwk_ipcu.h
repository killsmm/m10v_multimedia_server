/******************************************************************************/
/**
 *  @file   cmfwk_ipcu.h
 *  @brief  Userland Definition
 */
/*
 *  Copyright 2015 Socionext Inc.
 ******************************************************************************/
#ifndef __CMFWK_IPCU_H__
#define __CMFWK_IPCU_H__

/**************************************************************** 
 *  include
 ****************************************************************/
#include <pthread.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include "ipcu_userland.h"
#include "cmfwk_std.h"

/******************************************************************** 
 *  Debug Flag
 ********************************************************************/
//#define DEBUG_ON_MLB01


/******************************************************************** 
 *  Common define definition
 ********************************************************************/
#define IPCU_SEND_ASYNC     (0)
#define IPCU_SEND_SYNC      (1)

#define IPCU_BUFF_SIZE 0x00008000UL

/******************************************************************** 
 *  Table structure
 ********************************************************************/


/******************************************************************** 
 *  Function prototype
 ********************************************************************/
FJ_ERR_CODE FJ_IPCU_Open(E_FJ_IPCU_MAILBOX_TYPE ipctype, UINT8 *id);
FJ_ERR_CODE FJ_IPCU_Close(UINT8 id);
FJ_ERR_CODE FJ_IPCU_Send(UINT8 id, UINT32 pdata, UINT32 length, UINT8 sync);
FJ_ERR_CODE FJ_IPCU_SetReceiverCB(UINT8 id, void(*req_fn_ptr)(UINT8, UINT32, UINT32, UINT32, UINT32));

#endif /* __CMFWK_IPCU_H__ */
