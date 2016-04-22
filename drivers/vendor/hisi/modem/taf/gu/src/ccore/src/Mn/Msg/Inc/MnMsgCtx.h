
#ifndef _MN_MSG_CTX_H_
#define _MN_MSG_CTX_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/

#include  "vos.h"
#include  "PsTypeDef.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  1 全局变量定义
*****************************************************************************/

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


typedef struct
{
    VOS_UINT8                           ucGetCsmpParaFromUsimSupportFlg;        /*从(U)SIM卡中读取CSMP参数*/
    VOS_UINT8                           ucSmsPpDownlodSupportFlg;               /*表明是否支持短信的PP-DOWNLOAD功能*/
    VOS_UINT8                           ucSmsNvSmsRexitSupportFlg;              /*服务域设置为PS ONLY时，是否支持CS域短信和呼叫业务(紧急呼叫除外) */
    VOS_UINT8                           ucSmsStatusInEfsmsSupportFlg;           /* 表明NVIM中是否能保存短信状态报告*/
    VOS_UINT8                           ucPsOnlyCsServiceSupportFlg;            /*支持短信状态报告存到EFSMS文件*/
    VOS_UINT8                           ucLocalStoreFlg;                        /*modem local store message when no RP ACK*/

    VOS_UINT8                           ucGetScAddrIgnoreScIndication; /* 0x6f42文件中sc indication指示不存在短信中心号码时，如果短信中心号码合法是否读取，
                                                                       VOS_FALSE:sc indication指示短信中心号码不存在则不读取短信中心号码；
                                                                       VOS_TRUE: sc indication指示短信中心号码不存在读取短信中心号码 */
    VOS_UINT8                           aucReserve[1];

}MN_MSG_CUSTOM_CFG_INFO_STRU;


typedef struct
{
    MN_MSG_CUSTOM_CFG_INFO_STRU        stCustomCfg;                            /* NVIM中的定制信息 */
}MN_MSG_MS_CFG_INFO_STRU;
typedef struct
{
    MN_MSG_MS_CFG_INFO_STRU                  stMsCfgInfo;                      /* MS的配置信息 */
}MN_MSG_CONTEXT_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/



/*****************************************************************************
  10 函数声明
*****************************************************************************/
MN_MSG_CUSTOM_CFG_INFO_STRU* MN_MSG_GetCustomCfgInfo( VOS_VOID );


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

#endif /* end of MnCallCtx.h */

