/** ****************************************************************************

                    Huawei Technologies Sweden AB (C), 2001-2011

 ********************************************************************************
 * @author    Automatically generated by DAISY
 * @version
 * @date      2014-07-02
 * @file
 * @brief
 * Physical interface between XSD and MSCC
 * @copyright Huawei Technologies Sweden AB
 *******************************************************************************/
#ifndef XSD_MSCC_PIF_H
#define XSD_MSCC_PIF_H

/*******************************************************************************
 1. Other files included
*******************************************************************************/
#include "vos.h"
#include "PsTypeDef.h"
#include "NasMsccPifInterface.h"
/* Added by h00313353 for CDMA Iteration 15, 2015-5-30, begin */
#include "PsRrmInterface.h"
/* Added by h00313353 for CDMA Iteration 15, 2015-5-30, end */


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#pragma pack(4)

/*******************************************************************************
 2. Macro definitions
*******************************************************************************/
#define MSCC_XSD_INVALID_MNC                     (0xffff)
/* Added by y00322978 for CDMA Iteration 17, 2015-7-13, begin */
#define MSCC_XSD_MAX_WHITE_LOCK_SID_NUM           (20)
/* Added by y00322978 for CDMA Iteration 17, 2015-7-13, end */
/*******************************************************************************
 3. Enumerations declarations
*******************************************************************************/
/** ****************************************************************************
 * Name        : XSD_MSCC_PIF_MSG_TYPE_ENUM_UINT16
 * Description :
 *******************************************************************************/
enum XSD_MSCC_PIF_MSG_TYPE_ENUM
{
    ID_MSCC_XSD_START_REQ                        = 0x0000, /* _H2ASN_MsgChoice MSCC_XSD_START_REQ_STRU*/ /**< @sa MSCC_XSD_START_REQ_STRU */
    ID_XSD_MSCC_START_CNF                        = 0x0001, /* _H2ASN_MsgChoice XSD_MSCC_START_CNF_STRU*/ /**< @sa XSD_MSCC_START_CNF_STRU */
    ID_MSCC_XSD_POWER_OFF_REQ                    = 0x0002, /* _H2ASN_MsgChoice MSCC_XSD_POWER_OFF_REQ_STRU*/ /**< @sa MSCC_XSD_POWER_OFF_REQ_STRU */
    ID_XSD_MSCC_POWER_OFF_CNF                    = 0x0003, /* _H2ASN_MsgChoice XSD_MSCC_POWER_OFF_CNF_STRU*/ /**< @sa XSD_MSCC_POWER_OFF_CNF_STRU */
    ID_MSCC_XSD_MO_CALL_START_NTF                = 0x0004, /* _H2ASN_MsgChoice MSCC_XSD_MO_CALL_START_NTF_STRU*/ /**< @sa MSCC_XSD_MO_CALL_START_NTF_STRU */
    ID_MSCC_XSD_CALL_REDIAL_SYSTEM_ACQUIRE_NTF   = 0x0005, /* _H2ASN_MsgChoice MSCC_XSD_CALL_REDIAL_SYSTEM_ACQUIRE_NTF_STRU*/ /**< @sa MSCC_XSD_CALL_REDIAL_SYSTEM_ACQUIRE_NTF_STRU */
    ID_MSCC_XSD_MO_CALL_SUCCESS_NTF              = 0x0006, /* _H2ASN_MsgChoice MSCC_XSD_MO_CALL_SUCCESS_NTF_STRU*/ /**< @sa MSCC_XSD_CALL_SUCCESS_NTF_STRU */
    ID_MSCC_XSD_MO_CALL_END_NTF                  = 0x0007, /* _H2ASN_MsgChoice MSCC_XSD_1X_MO_CALL_END_NTF_STRU*/ /**< @sa MSCC_XSD_CALL_END_NTF_STRU */
    ID_MSCC_XSD_SYSTEM_ACQUIRE_REQ               = 0x0008, /* _H2ASN_MsgChoice MSCC_XSD_SYSTEM_ACQUIRE_REQ_STRU*/ /**< @sa MSCC_XSD_SYSTEM_ACQUIRE_REQ_STRU */
    ID_XSD_MSCC_SYSTEM_ACQUIRE_CNF               = 0x0009, /* _H2ASN_MsgChoice XSD_MSCC_SYSTEM_ACQUIRE_CNF_STRU*/ /**< @sa XSD_MSCC_SYSTEM_ACQUIRE_CNF_STRU */
    ID_XSD_MSCC_SYSTEM_ACQUIRE_IND               = 0x000A, /* _H2ASN_MsgChoice XSD_MSCC_SYSTEM_ACQUIRE_IND_STRU*/ /**< @sa XSD_MSCC_SYSTEM_ACQUIRE_IND_STRU */
    ID_XSD_MSCC_SYSTEM_ACQUIRE_START_IND         = 0x000B, /* _H2ASN_MsgChoice XSD_MSCC_SYSTEM_ACQUIRE_START_IND_STRU*/ /**< @sa XSD_MSCC_SYSTEM_ACQUIRE_START_IND_STRU */
    ID_XSD_MSCC_1X_SYSTEM_SERVICE_INFO_IND       = 0x000C, /* _H2ASN_MsgChoice XSD_MSCC_1X_SYSTEM_SERVICE_INFO_IND_STRU */
    ID_MSCC_XSD_CFREQLOCK_NTF                    = 0x000D, /* _H2ASN_MsgChoice MSCC_XSD_CFREQ_LOCK_NTF_STRU */
    ID_MSCC_XSD_CDMACSQ_SET_REQ                  = 0x000E, /* _H2ASN_MsgChoice MSCC_XSD_CDMACSQ_SET_REQ_STRU*/
    ID_XSD_MSCC_CDMACSQ_SET_CNF                  = 0x000F, /* _H2ASN_MsgChoice XSD_MSCC_CDMACSQ_SET_CNF_STRU*/
    ID_XSD_MSCC_CDMACSQ_SIGNAL_QUALITY_IND       = 0x0010, /* _H2ASN_MsgChoice XSD_MSCC_CDMACSQ_IND_STRU*/
    ID_XSD_MSCC_1X_SYSTEM_TIME_IND               = 0x0011, /* _H2ASN_MsgChoice XSD_MSCC_1X_SYSTEM_TIME_IND_STRU*/

    ID_MSCC_XSD_POWER_SAVE_REQ                   = 0x0012, /* _H2ASN_MsgChoice MSCC_XSD_POWER_SAVE_REQ_STRU */
    ID_XSD_MSCC_POWER_SAVE_CNF                   = 0x0013, /* _H2ASN_MsgChoice XSD_MSCC_POWER_SAVE_CNF_STRU */
    ID_MSCC_XSD_CAS_STATUS_IND                   = 0x0014, /* _H2ASN_MsgChoice MSCC_XSD_CAS_STATUS_IND_STRU */
    ID_MSCC_XSD_SYS_CFG_REQ                      = 0x0015, /* _H2ASN_MsgChoice MSCC_XSD_SYS_CFG_REQ_STRU */
    ID_XSD_MSCC_SYS_CFG_CNF                      = 0x0016, /* _H2ASN_MsgChoice XSD_MSCC_SYS_CFG_CNF_STRU */

    ID_XSD_MSCC_RF_AVAILABLE_IND                 = 0x0017, /* _H2ASN_MsgChoice XSD_MSCC_RF_AVAILABLE_IND_STRU */

    ID_MSCC_XSD_SRV_ACQ_REQ                      = 0x0018, /* _H2ASN_MsgChoice MSCC_XSD_SRV_ACQ_REQ_STRU */
    ID_XSD_MSCC_SRV_ACQ_CNF                      = 0x0019, /* _H2ASN_MsgChoice XSD_MSCC_SRV_ACQ_CNF_STRU */
    ID_MSCC_XSD_BEGIN_SESSION_NOTIFY             = 0x001A, /* _H2ASN_MsgChoice MSCC_XSD_BEGIN_SESSION_NOTIFY_STRU */
    ID_MSCC_XSD_END_SESSION_NOTIFY               = 0x001B, /* _H2ASN_MsgChoice MSCC_XSD_END_SESSION_NOTIFY_STRU */

	/* Added by z00316370 for UTS 2015-5-22 begin */
    ID_MSCC_XSD_HANDSET_INFO_QRY_REQ             = 0x001C, /* _H2ASN_MsgChoice MSCC_XSD_HANDSET_INFO_QRY_STRU*/
    ID_XSD_MSCC_HANDSET_INFO_QRY_CNF             = 0x001D, /* _H2ASN_MsgChoice XSD_MSCC_HANDSET_INFO_QRY_CNF_STRU*/
    /* Added by z00316370 for UTS 2015-5-22 end */

    /* Added by h00313353 for Iteration 17, 2015-7-4, begin */
    ID_XSD_MSCC_EMC_CALLBACK_IND                 = 0x001E, /* XSD_MSCC_EMC_CALLBACK_NTF_STRU */
    ID_MSCC_XSD_END_EMC_CALLBACK_NTF             = 0x001F, /* XSD_MSCC_END_EMC_CALLBACK_NTF_STRU */
    /* Added by h00313353 for Iteration 17, 2015-7-4, end */

    ID_XSD_MSCC_SID_IND                          = 0x0020, /* _H2ASN_MsgChoice MSCC_XSD_SID_IND_STRU*/

    /* Added by y00322978 for CDMA Iteration 17, 2015-7-13, begin */
    ID_MSCC_XSD_SET_CSIDLIST_REQ                 = 0x0021, /* _H2ASN_MsgChoice MSCC_XSD_SET_CSIDLIST_REQ_STRU*/
    ID_XSD_MSCC_SET_CSIDLIST_CNF                 = 0x0022, /* _H2ASN_MsgChoice XSD_MSCC_SET_CSIDLIST_CNF_STRU*/
    ID_XSD_MSCC_SYNC_SERVICE_AVAILABLE_IND       = 0x0023, /* _H2ASN_MsgChoice XSD_MSCC_SYNC_SERVICE_AVAILABLE_IND_STRU*/
    /* Added by y00322978 for CDMA Iteration 17, 2015-7-13, end */

    ID_XSD_MSCC_CDMA_UE_STATUS_IND               = 0x0024, /* _H2ASN_MsgChoice XSD_MSCC_UE_STATE_IND_STRU*/


    ID_XSD_MSCC_PIF_MSG_TYPE_ENUM_BUTT          = 0xFFFF
};
typedef VOS_UINT16 XSD_MSCC_PIF_MSG_TYPE_ENUM_UINT16;

/** ****************************************************************************
 * Name        : XSD_MSCC_START_RESULT_ENUM_UINT32
 * Description : Start result
 *******************************************************************************/
enum XSD_MSCC_START_RESULT_ENUM
{
    XSD_MSCC_START_RESULT_SUCC           = 0x00000000,
    XSD_MSCC_START_RESULT_FAIL           = 0x00000001,
    XSD_MSCC_START_RESULT_BUTT           = 0x00000002
};
typedef VOS_UINT32 XSD_MSCC_START_RESULT_ENUM_UINT32;


/** ****************************************************************************
 * Name        : MSCC_XSD_HRPD_CAS_STATUS_ENUM_UINT16
 *
 * Description :
 *******************************************************************************/
enum MSCC_XSD_HRPD_CAS_STATUS_ENUM
{
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_NONE                      = 0x0000,
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_INIT                      = 0x0001,
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_IDLE                      = 0x0002,
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_CONN                      = 0x0003,
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_BUTT                      = 0x0004
};
typedef VOS_UINT16 MSCC_XSD_HRPD_CAS_STATUS_ENUM_UINT16;

/** ****************************************************************************
 * Name        : MSCC_XSD_SYS_TYPE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum MSCC_XSD_SYS_TYPE_ENUM
{
    MSCC_XSD_SYS_TYPE_CDMA2000_AI       = 0x00,
    MSCC_XSD_SYS_TYPE_CDMA2000_1X       = 0x01,
    MSCC_XSD_SYS_TYPE_ENUM_BUTT         = 0x02
};
typedef VOS_UINT8 MSCC_XSD_SYS_TYPE_ENUM_UINT8;


/* Added by h00313353 for Iteration 17, 2015-7-16, begin */
typedef NAS_MSCC_PIF_EMC_CALLBACK_MODE_ENUM_UINT32 MSCC_XSD_EMC_CALLBACK_MODE_ENUM_UINT32;
/* Added by h00313353 for Iteration 17, 2015-7-16, end */

/*******************************************************************************
 4. Message Header declaration
*******************************************************************************/

/*******************************************************************************
 5. Message declaration
*******************************************************************************/

/*******************************************************************************
 6. STRUCT and UNION declaration
*******************************************************************************/

/*******************************************************************************
 7. OTHER declarations
*******************************************************************************/

/** ****************************************************************************
 * Name        : MSCC_XSD_START_REQ_STRU
 * Description : Start request
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
    MODEM_ID_ENUM_UINT16                        enModemId;
    NAS_MSCC_PIF_CARD_STATUS_ENUM_UINT8         enCardStatus;
    VOS_UINT8                                   ucSuppRatNum;
    VOS_RATMODE_ENUM_UINT32                     aenRatMode[VOS_RATMODE_BUTT];
    VOS_UINT8                                   ucIsCardChanged;                /* 卡的ICCID是否发生改变标识 */
    VOS_UINT8                                   aucReserved[3];
}MSCC_XSD_START_REQ_STRU;

/** ****************************************************************************
 * Name        : XSD_MSCC_START_CNF_STRU
 * Description : Start confirm
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
    XSD_MSCC_START_RESULT_ENUM_UINT32           enRslt;
}XSD_MSCC_START_CNF_STRU;
typedef NAS_MSCC_PIF_POWER_OFF_REQ_STRU                     MSCC_XSD_POWER_OFF_REQ_STRU;


typedef NAS_MSCC_PIF_POWER_OFF_CNF_STRU                     XSD_MSCC_POWER_OFF_CNF_STRU;


typedef NAS_MSCC_PIF_CDMA_MO_CALL_START_NTF_STRU            MSCC_XSD_MO_CALL_START_NTF_STRU;


typedef NAS_MSCC_PIF_CDMA_MO_CALL_REDIAL_SYS_ACQ_NTF_STRU   MSCC_XSD_CALL_REDIAL_SYSTEM_ACQUIRE_NTF_STRU;


typedef NAS_MSCC_PIF_CDMA_MO_CALL_SUCCESS_NTF_STRU          MSCC_XSD_MO_CALL_SUCCESS_NTF_STRU;


typedef NAS_MSCC_PIF_CDMA_MO_CALL_END_NTF_STRU              MSCC_XSD_1X_MO_CALL_END_NTF_STRU;


/** ****************************************************************************
 * Name        : MSCC_XSD_SYSTEM_ACQUIRE_REQ_STRU
 * Description : System acquire request
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
    VOS_UINT8                                   aucReserve[4];
}MSCC_XSD_SYSTEM_ACQUIRE_REQ_STRU;

/** ****************************************************************************
 * Name        : XSD_MSCC_SYSTEM_ACQUIRE_CNF_STRU
 * Description : System acquire confirm
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
    NAS_MSCC_PIF_ACQUIRED_RESULT_ENUM_UINT32    enRslt;

}XSD_MSCC_SYSTEM_ACQUIRE_CNF_STRU;

/** ****************************************************************************
 * Name        : XSD_MSCC_SYSTEM_ACQUIRE_IND_STRU
 * Description : System acquire indication
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
    NAS_MSCC_PIF_ACQUIRED_RESULT_ENUM_UINT32    enRslt;
}XSD_MSCC_SYSTEM_ACQUIRE_IND_STRU;
typedef NAS_MSCC_PIF_1X_SYSTEM_SERVICE_INFO_IND_STRU    XSD_MSCC_1X_SYSTEM_SERVICE_INFO_IND_STRU;



/** ****************************************************************************
 * Name        : XSD_MSCC_SYSTEM_ACQUIRE_START_IND_STRU
 * Description :
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;                    /*_H2ASN_Skip*//* 消息头                                   */
}XSD_MSCC_SYSTEM_ACQUIRE_START_IND_STRU;
typedef NAS_MSCC_PIF_CDMACSQ_SET_REQ_STRU    MSCC_XSD_CDMACSQ_SET_REQ_STRU;


typedef NAS_MSCC_PIF_CDMACSQ_SET_CNF_STRU    XSD_MSCC_CDMACSQ_SET_CNF_STRU;


typedef NAS_MSCC_PIF_CDMACSQ_IND_STRU    XSD_MSCC_CDMACSQ_IND_STRU;
/* Added by m00312079 for CDMA 1X Iteration 6 2014-12-25 end */

/** ****************************************************************************
 * Name        : MSCC_XSD_CFREQ_LOCK_NTF_STRU
 * Description : Start confirm
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                                 stMsgHeader;                /*_H2ASN_Skip*//* 消息头                                   */
    NAS_MSCC_PIF_CFREQ_LOCK_SET_MODE_ENUM_UINT8     enFreqLockMode;
    VOS_UINT8                                       aucReserve[1];
    VOS_UINT16                                      usCdmaBandClass;
    VOS_UINT16                                      usSid;
    VOS_UINT16                                      usNid;
    VOS_UINT16                                      usCdmaFreq;
    VOS_UINT16                                      usCdmaPn;
}MSCC_XSD_CFREQ_LOCK_NTF_STRU;
typedef NAS_MSCC_PIF_1X_SYSTEM_TIME_IND_STRU    XSD_MSCC_1X_SYSTEM_TIME_IND_STRU;

/** ****************************************************************************
 * Name        : MSCC_XSD_CAS_STATUS_IND_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                             stMsgHeader;
    MSCC_XSD_HRPD_CAS_STATUS_ENUM_UINT16        enCasStatus;
    VOS_UINT8                                   aucReserve[2];
} MSCC_XSD_CAS_STATUS_IND_STRU;



typedef NAS_MSCC_PIF_POWER_SAVE_REQ_STRU    MSCC_XSD_POWER_SAVE_REQ_STRU;


typedef NAS_MSCC_PIF_POWER_SAVE_CNF_STRU    XSD_MSCC_POWER_SAVE_CNF_STRU;


typedef struct
{
    MSG_HEADER_STRU                                      stMsgHeader;          /* _H2ASN_Skip */
    VOS_UINT16                                           usOpId;
    VOS_UINT8                                            ucReserved[1];
    VOS_UINT8                                            ucSuppRatNum;
    VOS_RATMODE_ENUM_UINT32                              aenRatMode[VOS_RATMODE_BUTT];
    NAS_MSCC_PIF_CDMA_BAND_STRU                          stCdmaBand;
}MSCC_XSD_SYS_CFG_REQ_STRU;
typedef NAS_MSCC_PIF_SYS_CFG_SET_CNF_STRU   XSD_MSCC_SYS_CFG_CNF_STRU;



typedef NAS_MSCC_PIF_SRV_ACQ_REQ_STRU MSCC_XSD_SRV_ACQ_REQ_STRU;


typedef NAS_MSCC_PIF_SRV_ACQ_CNF_STRU XSD_MSCC_SRV_ACQ_CNF_STRU;


typedef NAS_MSCC_PIF_BEGIN_SESSION_NOTIFY_STRU MSCC_XSD_BEGIN_SESSION_NOTIFY_STRU;


typedef NAS_MSCC_PIF_END_SESSION_NOTIFY_STRU MSCC_XSD_END_SESSION_NOTIFY_STRU;


/* Added by z00316370 for UTS 2015-5-22 begin */
/** ****************************************************************************
 * Name        : MSCC_XSD_HANDSET_INFO_QRY_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                     stMsgHeader;                /*_H2ASN_Skip*//* 消息头                                   */
    VOS_UINT32                          ulInfoType;
    VOS_UINT16                          usOpId;
    VOS_UINT8                           aucReserve[2];
}MSCC_XSD_HANDSET_INFO_QRY_STRU;

/*****************************************************************************
 结构名    : XSD_MSCC_HANDSET_INFO_QRY_CNF_STRU
 结构说明  : XSD给MSCC回复的手机状态信息的消息结构
 1.日    期   : 2015年5月16日
   作    者   : z00316370
   修改内容   : 新建
*****************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                     stMsgHeader;        /*_H2ASN_Skip*//* 消息头    */
    VOS_UINT32                          ulInfoType;         /* 请求的信息类型 */
    VOS_UINT8                           ucCasState;         /* cas上报的主状态 */
    VOS_UINT8                           ucCasSubSta;        /* cas上报的子状态 */
    VOS_UINT8                           ucHighVer;          /* cas上报的所支持的最高的版本 */
    VOS_UINT8                           aucRsv[1];
}XSD_MSCC_HANDSET_INFO_QRY_CNF_STRU;
/* Added by z00316370 for UTS 2015-5-16 end */

/** ****************************************************************************
 * Name        : XSD_MSCC_UE_STATE_IND_STRU
 * Description : UE STATE indication
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                                      /* _H2ASN_Skip */
    XSD_MSCC_PIF_MSG_TYPE_ENUM_UINT16                       enMsgId;    /* _H2ASN_Skip */
    VOS_UINT16                                              usOpId;
    VOS_UINT8                                               ucUeMainState;
    VOS_UINT8                                               ucUeSubState;
    VOS_UINT8                                               aucReserve[2];
}XSD_MSCC_UE_STATE_IND_STRU;



typedef NAS_MSCC_PIF_SID_IND_STRU MSCC_XSD_SID_IND_STRU;

/* Added by h00313353 for Iteration 17, 2015-7-4, begin */
/*****************************************************************************
 结构名    : XSD_MSCC_EMC_CALLBACK_IND_STRU
 结构说明  : XSD给MSCC回复紧急呼CALLBACK模式状态的消息结构
 1.日    期   : 2015年07月02日
   作    者   : h00313353
   修改内容   : 新建
*****************************************************************************/
typedef NAS_MSCC_PIF_EMC_CALLBACK_IND_STRU XSD_MSCC_EMC_CALLBACK_IND_STRU;

/*****************************************************************************
 结构名    : MSCC_XSD_END_EMC_CALLBACK_REQ_STRU
 结构说明  : MSCC给XSD回复退出紧急呼CallBack请求的消息结构
 1.日    期   : 2015年07月02日
   作    者   : h00313353
   修改内容   : 新建
*****************************************************************************/
typedef NAS_MSCC_PIF_END_EMC_CALLBACK_IND_STRU MSCC_XSD_END_EMC_CALLBACK_NTF_STRU;
/* Added by h00313353 for Iteration 17, 2015-7-4, end */
/* Added by y00322978 for CDMA Iteration 17, 2015-7-13, begin */
/*****************************************************************************
 结构名    : MSCC_XSD_OPER_LOCK_WHITE_SID_STRU
 结构说明  : 保存的SID白名单信息
  1.日    期   : 2015年7月2日
    作    者   : y00322978
    修改内容   : 新建
*****************************************************************************/
typedef NAS_MSCC_PIF_OPER_LOCK_SYS_WHITE_STRU MSCC_XSD_OPER_LOCK_SYS_WHITE_STRU;

/*****************************************************************************
 结构名    : MSCC_XSD_SET_CSIDLIST_REQ_STRU
 结构说明  : MSCC给XSD sid white list 设置请求结构体
 1.日    期   : 2015年07月02日
   作    者   : y00322978
   修改内容   : 新建
*****************************************************************************/
typedef NAS_MSCC_PIF_SET_CSIDLIST_REQ_STRU MSCC_XSD_SET_CSIDLIST_REQ_STRU;

typedef NAS_MSCC_PIF_SET_CSIDLIST_CNF_STRU XSD_MSCC_SET_CSIDLIST_CNF_STRU;

typedef NAS_MSCC_PIF_SYNC_SERVICE_AVAILABLE_IND_STRU XSD_MSCC_SYNC_SERVICE_AVAILABLE_IND_STRU;

/* Added by y00322978 for CDMA Iteration 17, 2015-7-13, end */
/*******************************************************************************
 8. Global  declaration
*******************************************************************************/

/*******************************************************************************
 9. Function declarations
*******************************************************************************/
/*****************************************************************************
  H2ASN顶级消息结构定义
*****************************************************************************/
typedef struct
{
    VOS_UINT16      enMsgID;                /*_H2ASN_MsgChoice_Export XSD_MSCC_PIF_MSG_TYPE_ENUM_UINT16*/
    VOS_UINT8       aucMsgBlock[2];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          XSD_MSCC_PIF_MSG_TYPE_ENUM_UINT16
        ****************************************************************************/
}XSD_MSCC_PIF_MSG_DATA;
/*_H2ASN_Length UINT16*/


typedef struct
{
    VOS_MSG_HEADER
    XSD_MSCC_PIF_MSG_DATA       stMsgData;
}Xsd_mscc_pif_MSG;



#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif
