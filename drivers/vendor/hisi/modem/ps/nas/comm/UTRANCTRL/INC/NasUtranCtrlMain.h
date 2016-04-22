
#ifndef _NAS_UTRANCTRL_MAIN_H_
#define _NAS_UTRANCTRL_MAIN_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "PsTypeDef.h"


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



/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
VOS_UINT32 NAS_UTRANCTRL_ProcessMmcMsg(
    struct MsgCB                       *pstMsg
);

VOS_VOID NAS_UTRANCTRL_RegFsm( VOS_VOID  );

VOS_UINT32 NAS_UTRANCTRL_ProcessMsgInFsm(
    VOS_UINT32                          ulEventType,
    struct MsgCB                       *pstMsg
);


VOS_VOID  NAS_UTRANCTRL_PostProcMsg(
    struct MsgCB                       *pstSrcMsg,
    struct MsgCB                      **ppstDestMsg
);



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

#endif /* end of NAS_UTRANCTRLMain.h */
