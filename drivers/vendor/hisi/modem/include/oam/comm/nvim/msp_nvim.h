/*****************************************************************************
//
//                  版权所有 (C), 2001-2011, 华为技术有限公司
//
*****************************************************************************
//  文 件 名   : msp_nvim.h
//  版 本 号   : V1.0
//  生成日期   : 2008年07月07日
//  功能描述   : NV项管理接口，包括读写、删除、添加NV项的操作.
//  函数列表   : NVM_Read, NVM_Write
//  修改历史   :
//  1.日    期 : 2008年07月07日
//    修改内容 : 创建文件
//
*****************************************************************************/

#ifndef __MSP_NVIM_H__
#define __MSP_NVIM_H__

#include "vos.h"
#include "NVIM_Interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#if defined (INSTANCE_1)
#ifndef NVM_Read
#define NVM_Read(usID,pItem,ulLength)                NV_ReadEx(MODEM_ID_1,(VOS_UINT16)(usID),(pItem),(VOS_UINT32)(ulLength))
#endif
#ifndef NVM_Write
#define NVM_Write(usID,pItem,ulLength)               NV_WriteEx(MODEM_ID_1,(VOS_UINT16)(usID),(pItem),(VOS_UINT32)(ulLength))
#endif
#else
#ifndef NVM_Read
#define NVM_Read(usID,pItem,ulLength)                NV_ReadEx(MODEM_ID_0,(VOS_UINT16)(usID),(pItem),(VOS_UINT32)(ulLength))
#endif
#ifndef NVM_Write
#define NVM_Write(usID,pItem,ulLength)               NV_WriteEx(MODEM_ID_0,(VOS_UINT16)(usID),(pItem),(VOS_UINT32)(ulLength))
#endif
#endif
#ifndef NVM_GetItemLength
#define NVM_GetItemLength(usID, pulLength)           NV_Get16BitLength((usID), (pulLength))
#endif
#ifndef NVM_Flush
#define NVM_Flush()                                  NV_Flush()
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


#endif /* __MSP_DIAG_H__*/

