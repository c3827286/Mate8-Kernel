
/************************************************************************
 *                                                                      *
 *                             ipcp_fun.h                               *
 *                                                                      *
 *  Project Code:       VRP3.0                                          *
 *  Create Date:        2000/03/27                                      *
 *  Author:             Deng Yi Ou                                      *
 *  Modify Date:                                                        *
 *  Document:                                                           *
 *  Function:           IPCP模块私有函数说明                            *
 *  Others:                                                             *
 *----------------------------------------------------------------------*
 *                                                                      *
 *  Copyright 2000-2002 VRP3.0 Team Beijing Institute HuaWei Tech, Inc. *
 *                      ALL RIGHTS RESERVED                             *
 *                                                                      *
 *----------------------------------------------------------------------*
 *                                                                      *
 *   这个文件包含了IPCP模块所有私有函数的申明                           *
 *                                                                      *
 ************************************************************************/

#ifndef      _IPCP_FUNC_H_
#define      _IPCP_FUNC_H_

#ifdef      __cplusplus
extern "C" {
#endif		/* end of __cplusplus */

/* 接口函数 */
VOID PPP_IPCP_Init(PPPINFO_S *, ULONG, ULONG) ;
VOID PPP_IPCP_ReceiveEventFromCore (VOID *, ULONG , char *) ;
VOID PPP_IPCP_ReceivePacket (VOID *, UCHAR* , UCHAR* , ULONG ) ;

/* 回调函数 */
VOID PPP_IPCP_resetci(PPPFSM_S *f) ;
USHORT PPP_IPCP_cilen(PPPFSM_S *) ;
VOID PPP_IPCP_addci(PPPFSM_S *, UCHAR *) ;
USHORT PPP_IPCP_ackci(PPPFSM_S *, UCHAR *, ULONG) ;
USHORT PPP_IPCP_nakci(PPPFSM_S *, UCHAR *, ULONG) ;
USHORT PPP_IPCP_rejci(PPPFSM_S *, UCHAR *, ULONG) ;
UCHAR PPP_IPCP_reqci(PPPFSM_S *, UCHAR *, ULONG *) ;
VOID PPP_IPCP_up(PPPFSM_S *) ;
VOID PPP_IPCP_down(PPPFSM_S *) ;
VOID PPP_IPCP_finished(PPPFSM_S *) ;
VOID PPP_IPCP_starting(PPPFSM_S *) ;

/* 内部函数 */
ULONG PPP_IPCP_UpResetCi(PPPINFO_S *) ;

#ifdef      __cplusplus
}
#endif      /* end of __cplusplus */

#endif      /* end of _IPCP_FUNC_H_ */





