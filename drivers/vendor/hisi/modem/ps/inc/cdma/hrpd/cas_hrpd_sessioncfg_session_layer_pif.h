/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : cas_hrpd_sessioncfg_session_layer_pif.h
  版 本 号   : 初稿
  作    者   : W00200446
  生成日期   : 2015年2月27日
  最近修改   :
  功能描述   : 配置协商相关处理公共的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2015年2月27日
    作    者   : W00200446
    修改内容   : 创建文件

******************************************************************************/

#ifndef __CAS_HRPD_SESSIONCFG_SESSION_LAYER_PIF_H__
#define __CAS_HRPD_SESSIONCFG_SESSION_LAYER_PIF_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif



#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/******************************************************************************
结构名      : CAS_HRPD_DEFAULT_SMP_ATTRIB_STRU
协议表格    :
ASN.1描述   :
结构说明    :
******************************************************************************/
typedef struct
{
    VOS_UINT16                                  usTsmpClose;
    VOS_UINT8                                   ucResv[2];
}CAS_HRPD_DEFAULT_SMP_ATTRIB_STRU;

/******************************************************************************
结构名      : CAS_HRPD_DEFAULT_AMP_ATTRIB_STRU
协议表格    :
ASN.1描述   :
结构说明    :
******************************************************************************/
typedef struct
{
    VOS_UINT16                                  usMaxNoMonitorDistance;
    VOS_UINT16                                  usHardwareSeparableFromSession;
    VOS_UINT16                                  usSupportGAUPMaxNoMonitorDistance;
    VOS_UINT16                                  usReducedSubnetMaskOffset;
    VOS_UINT16                                  usSupportSecondaryColorCodes;
    VOS_UINT8                                   ucResv[2];
}CAS_HRPD_DEFAULT_AMP_ATTRIB_STRU;

/******************************************************************************
结构名      : CAS_HRPD_GENERIC_MULTI_CDP_STRU
协议表格    :
ASN.1描述   :
结构说明    :
******************************************************************************/
typedef struct
{
    VOS_UINT8                                   ucSimultaneousCommonChannelTransmit;
    VOS_UINT8                                   ucSimultaneousDedicatedChannelTransmit;
    VOS_UINT8                                   ucSimultaneousCommonChannelReceive;
    VOS_UINT8                                   ucSimultaneousDedicatedChannelReceive;
    VOS_UINT8                                   ucHybridMSAT;
    VOS_UINT8                                   ucReceiveDiversity;
    VOS_UINT8                                   ucResv[2];
}CAS_HRPD_GENERIC_MULTI_CDP_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
#ifndef INFUSION_VERSION

/*****************************************************************************
 函 数 名  : CAS_HRPD_HSCP_GetDefaultAmpAttr
 功能描述  : 获取default address management protocol的属性，输入参数不能为空
 输入参数  : CAS_HRPD_DEFAULT_AMP_ATTRIB_STRU *pstDeAmp
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月26日
    作    者   : W00200446
    修改内容   : 新生成函数

*****************************************************************************/
extern VOS_VOID CAS_HRPD_HSCP_GetDefaultAmpAttr(CAS_HRPD_DEFAULT_AMP_ATTRIB_STRU *pstDeAmp);

/*****************************************************************************
 函 数 名  : CAS_HRPD_HSCP_GetDefaultSmpAttr
 功能描述  : 获取default session management protocol的属性，输入参数不能为空
 输入参数  : CAS_HRPD_DEFAULT_SMP_ATTRIB_STRU *pstDeSmp
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月26日
    作    者   : W00200446
    修改内容   : 新生成函数

*****************************************************************************/
extern VOS_VOID CAS_HRPD_HSCP_GetDefaultSmpAttr(CAS_HRPD_DEFAULT_SMP_ATTRIB_STRU *pstDeSmp);

#endif


#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of cas_hrpd_sessioncfg_session_layer_pif.h */



