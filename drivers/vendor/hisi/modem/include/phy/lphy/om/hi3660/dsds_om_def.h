#ifndef __DSDS_OM_DEF_H__
#define __DSDS_OM_DEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "om_base_def.h"

typedef enum
{
	ID_RCM_LPHY_TASK_INIT_SEARCHING = 0,             /*  1  */
	ID_RCM_LPHY_TASK_INIT_PBCH_0,				/*  2  */
	ID_RCM_LPHY_TASK_INIT_PBCH_1,				/*  3  */
	ID_RCM_LPHY_TASK_INIT_SIB1,					/*  4  */
	ID_RCM_LPHY_TASK_INIT_SI_0,					/*  5  */
	ID_RCM_LPHY_TASK_INIT_SI_1,					/*  6  */

	ID_RCM_LPHY_TASK_IDLE_PAGE,					/*  7  */
	ID_RCM_LPHY_TASK_IDLE_MEA,					/*  8  */
	ID_RCM_LPHY_TASK_IDLE_BG_SEARCHING,		/*  9  */
	ID_RCM_LPHY_TASK_IDLE_PBCH_0,				/*  10  */
	ID_RCM_LPHY_TASK_IDLE_PBCH_1,				/*  11  */
	ID_RCM_LPHY_TASK_IDLE_SIB1,					/*  12  */
	ID_RCM_LPHY_TASK_IDLE_SI_0,					/*  13  */
	ID_RCM_LPHY_TASK_IDLE_SI_1,					/*  14  */
	ID_RCM_LPHY_TASK_IDLE_NCELL_PBCH_0,		/*  15  */
	ID_RCM_LPHY_TASK_IDLE_NCELL_PBCH_1,		/*  16  */
	ID_RCM_LPHY_TASK_IDLE_NCELL_SIB1,			/*  17  */
	ID_RCM_LPHY_TASK_IDLE_NCELL_SI_0,			/*  18  */
	ID_RCM_LPHY_TASK_IDLE_NCELL_SI_1,			/*  19  */

	ID_RCM_LPHY_TASK_CONN_TRAFFIC,				/*  20  */
	ID_RCM_LPHY_TASK_CONN_PMCH,				/*  21  */
	ID_RCM_LPHY_TASK_CONN_PBCH_0,				/*  22  */
	ID_RCM_LPHY_TASK_CONN_PBCH_1,				/*  23  */
	ID_RCM_LPHY_TASK_CONN_SIB1,					/*  24  */
	ID_RCM_LPHY_TASK_CONN_SI_0,					/*  25  */
	ID_RCM_LPHY_TASK_CONN_SI_1,					/*  26  */
	ID_RCM_LPHY_TASK_CONN_MEA,					/*  27  */
	ID_RCM_LPHY_TASK_CONN_PAGE,				    /*  28  */
	ID_RCM_LPHY_TASK_ANR,				        /*  29  */
	ID_RCM_LPHY_TASK_PRESYNC,                   /*30*/
	ID_RCM_LPHY_TASK_CAMPON,                   /*31*/
	ID_RCM_LPHY_TASK_BUTT,
}DSDS_RCM_LPHY_TASK_ID_ENUM;

typedef  UINT16 DSDS_RCM_LPHY_TASK_ID_ENUM_UINT16;

// TDS DSDS
typedef enum
{
	ID_RCM_TPHY_TASK_ICS_CELL_SEARCH = 0,       /*  1  */
	ID_RCM_TPHY_TASK_ICS_SWEEP_FREQ,			/*  2  */
	ID_RCM_TPHY_TASK_ICS_CELL_MEASUREMENT,		/*  3  */
	ID_RCM_TPHY_TASK_ICS_PCCPCH,				/*  4  */

	ID_RCM_TPHY_TASK_IDLE_PICH,					/*  5  */
	ID_RCM_TPHY_TASK_IDLE_PCH,					/*  6  */
	ID_RCM_TPHY_TASK_IDLE_MEASUREMENT,			/*  7  */
	ID_RCM_TPHY_TASK_IDLE_CELL_SEARCH,			/*  8  */
	ID_RCM_TPHY_TASK_IDLE_PCCPCH,				/*  9  */
	ID_RCM_TPHY_TASK_IDLE_FORCE_WAKE,			/*  10  */

	ID_RCM_TPHY_TASK_ACC_FACH_TRAFFIC,			/*  11  */
	ID_RCM_TPHY_TASK_ACC_PCCPCH,				/*  12  */

	ID_RCM_TPHY_TASK_DCH_TRAFFIC,				/*  13  */
	ID_RCM_TPHY_TASK_BUTT,
}DSDS_RCM_TPHY_TASK_ID_ENUM;

typedef  UINT16 DSDS_RCM_TPHY_TASK_ID_ENUM_UINT16;

typedef enum
{
    DSDS_SCHED_STATE_NO_RF = 0,
	DSDS_SCHED_STATE_RF_APPLY,
	DSDS_SCHED_STATE_RF_APPLY_FAIL,
	DSDS_SCHED_STATE_RF_GRANT,
	DSDS_SCHED_STATE_RF_PREEMPT,
	DSDS_SCHED_STATE_BUTT,
}DSDS_SCHED_STATE_ENUM;

typedef UINT16 DSDS_SCHED_STATE_ENUM_UINT16;

typedef enum
{
	DSDS_RF_OFF =0,
	DSDS_RF_OFF2ON,
	DSDS_RF_ON,
	DSDS_RF_ON2OFF,
	DSDS_RF_ON_HYSTERESIS,
	DSDS_TASK_RF_BUTT,
} DSDS_TASK_RF_STATE_ENUM;

typedef UINT16 DSDS_TASK_RF_STATE_ENUM_UINT16;

typedef enum
{
	LPHY_DSDS_SCHED_APPLY_RF_REQ_IND         = OM_CMD_ID(LPHY_SCHED_MID, OM_TYPE_TRACE, 0x8),
	LPHY_DSDS_SCHED_ASSIGN_IND,
	LPHY_DSDS_SCHED_PREEMPT_IND,
	LPHY_DSDS_SCHED_RESUME_IND,
	LPHY_DSDS_SCHED_APPLY_RF_FAIL,
	LPHY_DSDS_SCHED_TASK_REL_REQ,
	LPHY_DSDS_SCHED_TIMER_EXPIRY,
	LPHY_DSDS_SCHED_TASK_END,
	LPHY_DSDS_TASK_RF_STATE_TRANSFER,
	LPHY_DSDS_SCHED_STATE_TRANSFER,
	LPHY_DSDS_SET_START_TIMER,
	LPHY_DSDS_SET_PREEMP_TIMER,
	LPHY_DSDS_SET_RESUME_TIMER,
	LPHY_DSDS_SET_END_TIMER,
	LPHY_DSDS_RF_SWITCH_ON,
	LPHY_DSDS_RF_USER_STATISTICS,
	LPHY_DSDS_RF_SWITCH_OFF,
	LPHY_DSDS_TIME_TRANSFER_IND,
	LPHY_DSDS_RELEASE_ALL,
	LPHY_DSDS_SCHED_PREEMPT_ACK,
	LPHY_DSDS_WARNING,
	LPHY_DSDS_RX_RCM_RECYCLE_ASSIGN,
	LPHY_DSDS_RX_RCM_ASSIGN_RF_FAIL,
	LPHY_DSDS_INFO,
}LPHY_OM_DSDS_REPORT_ENUM;

typedef UINT32 LPHY_OM_DSDS_REPORT_ENUM_UINT32;

typedef enum
{
	LPHY_DSDS_RF_ASSIGN_ERROR_BECAUSE_NOT_APPLY_RF  = 0,
	LPHY_DSDS_RF_ASSIGN_ERROR_BECAUSE_NOT_AT_APPLY_STATE,
	LPHY_DSDS_OM_WARNING_OVERTIMER_IND,
	LPHY_DSDS_ABNORMAL_BECAUSE_RF_ON_AT_PREEMPT_STATE,
	LPHY_DSDS_OM_WARNING_BUTT,
}LPHY_DSDS_OM_WARNING_ENUM;

typedef UINT16 LPHY_DSDS_OM_WARNING_ENUM_UINT16;

typedef enum
{
	DSDS_TASK_TIMER_TYPE_TASK_START =0,
	DSDS_TASK_TIMER_TYPE_TASK_PREEMPTY,
	DSDS_TASK_TIMER_TYPE_TASK_RESUME,
	DSDS_TASK_TIMER_TYPE_TASK_END,
	DSDS_TASK_TIMER_TYPE_BUTT,
}DSDS_TASK_TIMER_TYPE_ENUM;

typedef UINT16 DSDS_TASK_TIMER_TYPE_ENUM_UINT16;

typedef struct
{
    UINT16 usLteSfn;
	UINT16 usLteSbfn;
}DSDS_LTE_TIME_STRU;

typedef struct
{
    UINT16 usTdsFn;
    UINT16 usTdschip;
}DSDS_TDS_TIME_STRU;

typedef struct
{
    union
    {
        DSDS_LTE_TIME_STRU stLteTime;
        DSDS_TDS_TIME_STRU stTdsTime;
    }u;
}DSDS_TIME_STRU;

typedef struct
{
    DSDS_RCM_LPHY_TASK_ID_ENUM_UINT16		enTaskID;
    DSDS_TIME_STRU                          stDsdsOmTime;
    UINT16									usCurrSymNum;
    DSDS_SCHED_STATE_ENUM_UINT16			enCurrDsdsState;
    DSDS_TASK_RF_STATE_ENUM_UINT16			enCurrRfState;
	UINT32									ulCurrent32kTime;
}DSDS_OM_COMM_STRU;


typedef struct
{
    DSDS_OM_COMM_STRU   stDsdsOmComm;
	DSDS_TIME_STRU      stExpectStartTime;
	UINT32 		        ulExpectStartTime;
}DSDS_OM_APPLY_RF_REQ_STRU;

typedef struct
{
	DSDS_OM_COMM_STRU						stDsdsOmComm;
	DSDS_TASK_RF_STATE_ENUM_UINT16			enRfStateFrom;
    DSDS_TASK_RF_STATE_ENUM_UINT16 			enRfStateTo;
}DSDS_OM_RF_STATE_TRANSFER_STRU;

typedef struct
{
	DSDS_OM_COMM_STRU						stDsdsOmComm;
	DSDS_SCHED_STATE_ENUM_UINT16			enDsdsStateFrom;
    DSDS_SCHED_STATE_ENUM_UINT16 			enDsdsStateTo;
}DSDS_OM_STATE_TRANSFER_STRU;

typedef struct
{
	DSDS_OM_COMM_STRU   stDsdsOmComm;
	DSDS_LTE_TIME_STRU  stExpectStartTime;
	UINT32 		ulExpectExpiryTime;
}DSDS_OM_SET_TIMER_STRU;

typedef struct
{
	DSDS_OM_COMM_STRU						stDsdsOmComm;
	UINT32 										ulCurrentTime;
	DSDS_TASK_TIMER_TYPE_ENUM_UINT16			enTimerType;
	UINT16										pad;
}DSDS_OM_TIMER_EXPIRY_STRU;

typedef struct
{
	DSDS_OM_COMM_STRU						stDsdsOmComm;
	LPHY_DSDS_OM_WARNING_ENUM_UINT16 		enWarningType;
	UINT16 										pad;
}DSDS_OM_WARNING_STRU;

typedef struct
{
    DSDS_TIME_STRU 							stLteDsdsTime;
    UINT32 									ulTransferTo32kTime;
    UINT32									ulLocal32kTime;
    UINT16									usSbfnDiff;
	UINT16 									us32kTimeType;
}DSDS_OM_TIME_TRANSFER_STRU;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
