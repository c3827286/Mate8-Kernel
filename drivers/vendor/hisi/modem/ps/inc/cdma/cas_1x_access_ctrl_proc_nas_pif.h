/** ****************************************************************************

                    Huawei Technologies Sweden AB (C), 2001-2015

 ********************************************************************************
 * @author    Automatically generated by DAISY
 * @version
 * @date      2015-07-20
 * @file
 * @brief
 * Interface provided to Layer_3_NAS.
 * @copyright Huawei Technologies Sweden AB
 *******************************************************************************/
#ifndef CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_H
#define CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_H

/*******************************************************************************
 1. Other files included
*******************************************************************************/

#include "vos.h"
#include "PsTypeDef.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#pragma pack(4)

/*******************************************************************************
 2. Macro definitions
*******************************************************************************/

#define CAS_CNAS_1X_ORIG_QOS_MAX_LEN                        ( 31 )
#define CAS_CNAS_1X_MAX_DIGIT_NUM_LEN                       ( 64 )
#define CAS_CNAS_1X_MAX_SUB_ADDRESS_LEN                     ( 21 )

/*******************************************************************************
 3. Enumerations declarations
*******************************************************************************/

/** ****************************************************************************
 * Name        : CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16
 *
 * Description :
 *******************************************************************************/
enum CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM
{
    ID_CAS_CNAS_1X_ROAM_DISPLAY_IND                         = 0x8000, /**< @sa CAS_CNAS_1X_ROAM_DISPLAY_IND_STRU */
    ID_CAS_CNAS_1X_MSG_ACCESS_RSLT_IND                      = 0x8001, /**< @sa CAS_CNAS_1X_MSG_ACCESS_RSLT_IND_STRU */
    ID_CNAS_CAS_1X_REG_ABORT_REQ                            = 0x8002, /**< @sa CNAS_CAS_1X_REG_ABORT_REQ_STRU */
    ID_CAS_CNAS_1X_REG_ABORT_IND                            = 0x8003, /**< @sa CAS_CNAS_1X_REG_ABORT_IND_STRU */
    ID_CNAS_CAS_1X_EST_REQ                                  = 0x8004, /**< @sa CNAS_CAS_1X_EST_REQ_STRU */
    ID_CAS_CNAS_1X_EST_CNF                                  = 0x8005, /**< @sa CAS_CNAS_1X_EST_CNF_STRU */
    ID_CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_BUTT   = 0xFFFF
};
typedef VOS_UINT16 CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16;

/** ****************************************************************************
 * Name        : CNAS_CAS_1X_RETURN_CAUSE_ENUM_UINT8
 *
 * Description : Table 2.7.1.3.2.1-2.  RETURN_CAUSE Codes
 *******************************************************************************/
enum CNAS_CAS_1X_RETURN_CAUSE_ENUM
{
    CNAS_CAS_1X_RETURN_CAUSE_NORMAL_ACCESS                  = 0x00,
    CNAS_CAS_1X_RETURN_CAUSE_SYSTEM_NOT_FOUND               = 0x01,
    CNAS_CAS_1X_RETURN_CAUSE_PROTOCOL_MISMATCH              = 0x02,
    CNAS_CAS_1X_RETURN_CAUSE_REGISTRATION_REJECTION         = 0x03,
    CNAS_CAS_1X_RETURN_CAUSE_WRONG_SID                      = 0x04,
    CNAS_CAS_1X_RETURN_CAUSE_WRONG_NID                      = 0x05,
    CNAS_CAS_1X_RETURN_CAUSE_BUTT                           = 0x06
};
typedef VOS_UINT8 CNAS_CAS_1X_RETURN_CAUSE_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_DATA_SYS_MODE_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_DATA_SYS_MODE
{
    CAS_CNAS_1X_DATA_SYS_MODE_1X        = 0x00,
    CAS_CNAS_1X_DATA_SYS_MODE_HRPD      = 0x01,
    CAS_CNAS_1X_DATA_SYS_MODE_BUTT      = 0x02
};
typedef VOS_UINT8 CAS_CNAS_1X_DATA_SYS_MODE_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_DIGIT_MODE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_DIGIT_MODE_ENUM
{
    CAS_CNAS_1X_DIGIT_MODE_DTMF         = 0x00,
    CAS_CNAS_1X_DIGIT_MODE_ASCII        = 0x01,
    CAS_CNAS_1X_DIGIT_MODE_BUTT         = 0x02
};
typedef VOS_UINT8 CAS_CNAS_1X_DIGIT_MODE_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_EST_RSLT_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_EST_RSLT_ENUM
{
    CAS_CNAS_1X_EST_RSLT_SUCCESS                            = 0x00, /**< SUCCESS */
    CAS_CNAS_1X_EST_RSLT_NO_SERVICE                         = 0x01, /**< No Service,Call Redial */
    CAS_CNAS_1X_EST_RSLT_MAX_ACCESS_PROBES                  = 0x02, /**< Max Access Probes, Call Redial */
    CAS_CNAS_1X_EST_RSLT_REORDER                            = 0x03, /**< Reorder Order, Call Redial */
    CAS_CNAS_1X_EST_RSLT_INTERCEPT                          = 0x04, /**< Intercept Order, Call Redial */
    CAS_CNAS_1X_EST_RSLT_ACCESS_DENYIED                     = 0x05, /**< Access Denied, Call Redial */
    CAS_CNAS_1X_EST_RSLT_LOCK                               = 0x06, /**< Lock until power cycle order, Call not Redial */
    CAS_CNAS_1X_EST_RSLT_ACCT_BLOCK                         = 0x07, /**< Acct Block, Call Redial */
    CAS_CNAS_1X_EST_RSLT_NDSS                               = 0x08, /**< Access Control based on Call Type, redial */
    CAS_CNAS_1X_EST_RSLT_REDIRECTION                        = 0x09, /**< Redirection, Call Redial */
    CAS_CNAS_1X_EST_RSLT_NOT_ACCEPT_BY_BS                   = 0x0A, /**< BS Reject, used in Register process */
    CAS_CNAS_1x_EST_RSLT_ACCESS_IN_PROGRESS                 = 0x0B, /**< Access In Progress, Call Redial */
    CAS_CNAS_1x_EST_RSLT_ACCESS_FAIL                        = 0x0C, /**< Access fail, Call Redial */
    CAS_CNAS_1X_EST_RSLT_ABORT                              = 0x0D, /**< State can not Process Call, Call Redial */
    CAS_CNAS_1X_EST_RSLT_RETRY_FAIL                         = 0x0E,
    CAS_CNAS_1X_EST_RSLT_NO_RF                              = 0x0F,
    CAS_CNAS_1X_EST_RSLT_BUTT                               = 0x10
};
typedef VOS_UINT8 CAS_CNAS_1X_EST_RSLT_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_EST_TYPE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_EST_TYPE_ENUM
{
    CAS_CNAS_1X_EST_TYPE_REGISTRATION   = 0x00,
    CAS_CNAS_1X_EST_TYPE_ORIGINATION    = 0x01,
    CAS_CNAS_1X_EST_TYPE_PAGING_RSP     = 0x02,
    CAS_CNAS_1X_EST_TYPE_BUTT           = 0x03
};
typedef VOS_UINT8 CAS_CNAS_1X_EST_TYPE_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_FAIL_LAYER_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_FAIL_LAYER_ENUM
{
    CAS_CNAS_1X_FAIL_LAYER_L2           = 0x00, /**< only MAX Access Probe is Layer2 abnormal */
    CAS_CNAS_1X_FAIL_LAYER_L3           = 0x01, /**< Layer3 abnormal */
    CAS_CNAS_1X_FAIL_LAYER_BUTT         = 0x02
};
typedef VOS_UINT8 CAS_CNAS_1X_FAIL_LAYER_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_NUMBER_PLAN_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_NUMBER_PLAN_ENUM
{
    CAS_CNAS_1X_NUMBER_PLAN_UNKNOW      = 0x00,
    CAS_CNAS_1X_NUMBER_PLAN_ISDN        = 0x01,
    CAS_CNAS_1X_NUMBER_PLAN_DATA        = 0x03,
    CAS_CNAS_1X_NUMBER_PLAN_TELEX       = 0x04,
    CAS_CNAS_1X_NUMBER_PLAN_PRIVATE     = 0x09,
    CAS_CNAS_1X_NUMBER_PLAN_BUTT        = 0x0A
};
typedef VOS_UINT8 CAS_CNAS_1X_NUMBER_PLAN_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_MSG_ACCESS_RSLT_ENUM_UINT8
 *
 * Description : Refers to  C.S0017 2.6.5.5.3
 *******************************************************************************/
enum CAS_CNAS_1X_MSG_ACCESS_RSLT_ENUM
{
    CAS_CNAS_1X_SUCC_ACCESS_ORIG        = 0x00,
    CAS_CNAS_1X_SUCC_ACCESS_PAGING_RSP  = 0x01,
    CAS_CNAS_1X_SUCC_ACCESS_REG         = 0x02,
    CAS_CNAS_1X_SUCC_ACCESS_OTHER_MSG   = 0x03,
    CAS_CNAS_1X_SUCC_ACCESS_BUTT        = 0x04
};
typedef VOS_UINT8 CAS_CNAS_1X_MSG_ACCESS_RSLT_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_NUMBER_TYPE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_NUMBER_TYPE_ENUM
{
    CAS_CNAS_1X_NUMBER_TYPE_UNKNOW                          = 0x00,
    CAS_CNAS_1X_NUMBER_TYPE_INTERNATIONAL                   = 0x01,
    CAS_CNAS_1X_NUMBER_TYPE_NATIONAL                        = 0x02,
    CAS_CNAS_1X_NUMBER_TYPE_NETWORK_SPEC                    = 0x03,
    CAS_CNAS_1X_NUMBER_TYPE_SUBSCRIBER                      = 0x04,
    CAS_CNAS_1X_NUMBER_TYPE_RESERVED                        = 0x05,
    CAS_CNAS_1X_NUMBER_TYPE_ABBREVIATED                     = 0x06,
    CAS_CNAS_1X_NUMBER_TYPE_RESERVED_FOR_EXT                = 0x07,
    CAS_CNAS_1X_NUMBER_TYPE_BUTT                            = 0x08
};
typedef VOS_UINT8 CAS_CNAS_1X_NUMBER_TYPE_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_REGISTRATION_TYPE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_REGISTRATION_TYPE_ENUM
{
    CAS_CNAS_1X_TIMER_BASED_REGISTRATION                    = 0x00,
    CAS_CNAS_1X_POWER_UP_REGISTRATION                       = 0x01,
    CAS_CNAS_1X_ZONE_BASED_REGISTRATION                     = 0x02,
    CAS_CNAS_1X_POWER_DOWN_REGISTRATION                     = 0x03,
    CAS_CNAS_1X_PARAMETER_CHANGED_REGISTRATION              = 0x04,
    CAS_CNAS_1X_ORDERED_REGISTRATION                        = 0x05,
    CAS_CNAS_1X_DISTANCE_BASED_REGISTRATION                 = 0x06,
    CAS_CNAS_1X_USER_ZONE_BASED_REGISTRATION                = 0x07,
    CAS_CNAS_1X_ENCRYPTION_RE_SYNC_REQUIRED_REGISTRATION    = 0x08,
    CAS_CNAS_1X_REGISTRATION_TYPE_BUTT                      = 0x09
};
typedef VOS_UINT8 CAS_CNAS_1X_REGISTRATION_TYPE_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_SERVICE_OPTION_ENUM_UINT16
 *
 * Description : Refer to C.S0017
 *******************************************************************************/
enum CAS_CNAS_1X_SERVICE_OPTION_ENUM
{
    CAS_CNAS_1X_SO_2_LOOPBACK                               = 0x0002,
    CAS_CNAS_1X_SO_3_EVRC                                   = 0x0003,
    CAS_CNAS_1X_SO_6_SMS_RS1                                = 0x0006,
    CAS_CNAS_1X_SO_7_PPP_PKT_DATA                           = 0x0007,
    CAS_CNAS_1X_SO_9_LOOPBACK                               = 0x0009,
    CAS_CNAS_1X_SO_14_SMS_RS2                               = 0x000E,
    CAS_CNAS_1X_SO_17_VOICE_13K                             = 0x0011,
    CAS_CNAS_1X_SO_18_OTASP                                 = 0x0012,
    CAS_CNAS_1X_SO_32_TDSO                                  = 0x0020,
    CAS_CNAS_1X_SO_33_PPP_PKT_DATA_CDMA2K                   = 0x0021,
    /* Added by y00314741 for AGPS_PROJECT, 2015-8-31, begin */
    CAS_CNAS_1X_SO_35_LOC_SERV                              = 0x0023,
    CAS_CNAS_1X_SO_36_LOC_SERV                              = 0x0024,
    /* Added by y00314741 for AGPS_PROJECT, 2015-8-31, end */
    CAS_CNAS_1X_SO_54_MARKOV                                = 0x0036,
    CAS_CNAS_1X_SO_55_LOOPBACK                              = 0x0037,
    CAS_CNAS_1X_SO_68_EVRC_B                                = 0x0044,
    CAS_CNAS_1X_SO_73_EVRC_NW                               = 0x0049,
    CAS_CNAS_1X_SO_74_LOOPBACK                              = 0x004A,
    CAS_CNAS_1X_SO_75_LOOPBACK                              = 0x004B,
    CAS_CNAS_1X_SO_BUTT                                     = 0x004C
};
typedef VOS_UINT16 CAS_CNAS_1X_SERVICE_OPTION_ENUM_UINT16;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_SUB_ADDRESS_EO_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_SUB_ADDRESS_EO_ENUM
{
    CAS_CNAS_1X_SUB_ADDRESS_EVEN        = 0x00,
    CAS_CNAS_1X_SUB_ADDRESS_ODD         = 0x01,
    CAS_CNAS_1X_SUB_ADDRESS_EO_BUTT     = 0x02
};
typedef VOS_UINT8 CAS_CNAS_1X_SUB_ADDRESS_EO_ENUM_UINT8;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_SUB_ADDRESS_TYPE_ENUM_UINT8
 *
 * Description :
 *******************************************************************************/
enum CAS_CNAS_1X_SUB_ADDRESS_TYPE_ENUM
{
    CAS_CNAS_1X_SUB_ADDRESS_TYPE_NSAP   = 0x00,
    CAS_CNAS_1X_SUB_ADDRESS_TYPE_USER   = 0x01,
    CAS_CNAS_1X_SUB_ADDRESS_TYPE_BUTT   = 0x02
};
typedef VOS_UINT8 CAS_CNAS_1X_SUB_ADDRESS_TYPE_ENUM_UINT8;

/*******************************************************************************
 4. Message Header declaration
*******************************************************************************/

/*******************************************************************************
 5. Message declaration
*******************************************************************************/

/*******************************************************************************
 6. STRUCT and UNION declaration
*******************************************************************************/

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_CALLING_PARTY_NUM_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    CAS_CNAS_1X_NUMBER_TYPE_ENUM_UINT8  enNumType;
    CAS_CNAS_1X_NUMBER_PLAN_ENUM_UINT8  enNumPlan;
    VOS_UINT8                           ucNumLen;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT8                           aucDigitNum[CAS_CNAS_1X_MAX_DIGIT_NUM_LEN];
} CAS_CNAS_1X_CALLING_PARTY_NUM_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_SUB_ADDRESS_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_UINT8                                               ucIsExtBit;
    CAS_CNAS_1X_SUB_ADDRESS_TYPE_ENUM_UINT8                 enSubAddrType;
    CAS_CNAS_1X_SUB_ADDRESS_EO_ENUM_UINT8                   enEoInd;
    VOS_UINT8                                               ucNumLen;
    VOS_UINT8                                               aucDigitNum[CAS_CNAS_1X_MAX_SUB_ADDRESS_LEN];
    VOS_UINT8                                               aucReserved[3];
} CAS_CNAS_1X_SUB_ADDRESS_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_LAST_ACT_DATA_NET_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    CAS_CNAS_1X_DATA_SYS_MODE_UINT8     enDataSysMode;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT16                          usPzid;
    VOS_UINT16                          usSid;
    VOS_UINT16                          usNid;
} CAS_CNAS_1X_LAST_ACT_DATA_NET_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_SERVICE_OPTION_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    CAS_CNAS_1X_SERVICE_OPTION_ENUM_UINT16                  enSo;
    VOS_UINT8                                               aucReserved1[2];
} CAS_CNAS_1X_SERVICE_OPTION_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_ORIG_NUM_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    CAS_CNAS_1X_DIGIT_MODE_ENUM_UINT8   enDigitMode;
    CAS_CNAS_1X_NUMBER_TYPE_ENUM_UINT8  enNumType;
    CAS_CNAS_1X_NUMBER_PLAN_ENUM_UINT8  enNumPlan;
    VOS_UINT8                           ucDigitNum;
    VOS_UINT8                           aucDigit[CAS_CNAS_1X_MAX_DIGIT_NUM_LEN];
} CAS_CNAS_1X_ORIG_NUM_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_ORIG_QOS_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_UINT8                           ucQosParaLength;
    VOS_UINT8                           aucQosPara[CAS_CNAS_1X_ORIG_QOS_MAX_LEN];
} CAS_CNAS_1X_ORIG_QOS_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_REGISTRATION_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    CAS_CNAS_1X_REGISTRATION_TYPE_ENUM_UINT8                enRegType;
    VOS_UINT8                                               ucIsMtCallInRoamingAcc;
    CNAS_CAS_1X_RETURN_CAUSE_ENUM_UINT8                     enReturnCause;
    VOS_UINT8                                               aucReserved[1];
} CAS_CNAS_1X_REGISTRATION_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_ORIGINATION_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_UINT32                                              bitOpQosInfo:1;
    VOS_UINT32                                              bitOpCallingPartyNum:1;
    VOS_UINT32                                              bitOpCallingPartySubAddr:1;
    VOS_UINT32                                              bitOpCalledPartySubAddr:1;
    VOS_UINT32                                              bitOpSpare:28;
    VOS_UINT8                                               ucConnectId;
    VOS_UINT8                                               ucIsMtCallInRoamingAcc;
    VOS_UINT8                                               ucIsDataReady;
    VOS_UINT8                                               ucIsEmergencyCall;
    CAS_CNAS_1X_SERVICE_OPTION_INFO_STRU                    stSoInfo;
    CAS_CNAS_1X_ORIG_NUM_INFO_STRU                          stOrigNumInfo;
    VOS_UINT8                                               ucIsReOrigReason;
    VOS_UINT8                                               ucReOrigCount;
    VOS_UINT8                                               ucSrId;
    PS_BOOL_ENUM_UINT8                                      enDormantDataCall;
    CNAS_CAS_1X_RETURN_CAUSE_ENUM_UINT8                     enReturnCause;              /**< Refer cs0005, */
    VOS_UINT8                                               aucRsvd[3];
    CAS_CNAS_1X_ORIG_QOS_INFO_STRU                          stOrigQosInfo;
    CAS_CNAS_1X_CALLING_PARTY_NUM_INFO_STRU                 stCallingPartyNum;
    CAS_CNAS_1X_SUB_ADDRESS_INFO_STRU                       stCallingPartySubAddr;
    CAS_CNAS_1X_SUB_ADDRESS_INFO_STRU                       stCalledPartySubAddr;
    CAS_CNAS_1X_LAST_ACT_DATA_NET_INFO_STRU                 stLastActDataNetInfo;
} CAS_CNAS_1X_ORIGINATION_INFO_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_PAGING_RSP_INFO_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_UINT8                                               ucIsMtCallInRoamingAcc;
    VOS_UINT8                                               ucConnectId;
    VOS_UINT8                                               aucReserved[2];
    CAS_CNAS_1X_SERVICE_OPTION_INFO_STRU                    stSoInfo;
} CAS_CNAS_1X_PAGING_RSP_INFO_STRU;

/*******************************************************************************
 7. OTHER declarations
*******************************************************************************/

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_ROAM_DISPLAY_IND_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
    VOS_UINT8                                               ucRoamInd;
    VOS_UINT8                                               aucReserve[3];
} CAS_CNAS_1X_ROAM_DISPLAY_IND_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_MSG_ACCESS_RSLT_IND_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
    CAS_CNAS_1X_MSG_ACCESS_RSLT_ENUM_UINT8                  enMsgAccessInfo;
    VOS_UINT8                                               ucAcsSuccFlag;
    VOS_UINT8                                               aucReserve[2];
} CAS_CNAS_1X_MSG_ACCESS_RSLT_IND_STRU;

/** ****************************************************************************
 * Name        : CNAS_CAS_1X_REG_ABORT_REQ_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
} CNAS_CAS_1X_REG_ABORT_REQ_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_REG_ABORT_IND_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
} CAS_CNAS_1X_REG_ABORT_IND_STRU;

/** ****************************************************************************
 * Name        : CNAS_CAS_1X_EST_REQ_STRU
 *
 * Description : NAS inform AS to establish the uplink common channel to send
 * the registation message or origination message.
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
    CAS_CNAS_1X_EST_TYPE_ENUM_UINT8                         enEstType;
    VOS_UINT8                                               aucReserved[3];
    CAS_CNAS_1X_REGISTRATION_INFO_STRU                      stRegInfo;
    CAS_CNAS_1X_ORIGINATION_INFO_STRU                       stOriginationInfo;
    CAS_CNAS_1X_PAGING_RSP_INFO_STRU                        stPagingRspInfo;
} CNAS_CAS_1X_EST_REQ_STRU;

/** ****************************************************************************
 * Name        : CAS_CNAS_1X_EST_CNF_STRU
 *
 * Description : AS inform NAS the establish result.
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CAS_1X_ACCESS_CTRL_PROC_NAS_PIF_MSG_TYPE_ENUM_UINT16    enMsgId;
    VOS_UINT16                                              usOpId;
    CAS_CNAS_1X_EST_TYPE_ENUM_UINT8                         enEstType;
    CAS_CNAS_1X_EST_RSLT_ENUM_UINT8                         enEstRslt;
    CAS_CNAS_1X_FAIL_LAYER_ENUM_UINT8                       enFailLayer;
    VOS_UINT8                                               ucConnectId;
} CAS_CNAS_1X_EST_CNF_STRU;

/*******************************************************************************
 8. Global  declaration
*******************************************************************************/

/*******************************************************************************
 9. Function declarations
*******************************************************************************/

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
