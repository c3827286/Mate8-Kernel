


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_regdomain.h"
#include "hmac_main.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_mgmt_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"

#ifdef _PRE_WIFI_DMT
#include "hal_witp_dmt_if.h"
#include "dmt_stub.h"
#include "frame_public.h"
#endif
#include "hmac_p2p.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SME_STA_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
    g_handle_rsp_func_sta: 处理发给sme的消息
*****************************************************************************/
OAL_STATIC hmac_sme_handle_rsp_func g_handle_rsp_func_sta[HMAC_SME_RSP_BUTT] =
{
    hmac_handle_scan_rsp_sta,
    hmac_handle_join_rsp_sta,
    hmac_handle_auth_rsp_sta,
    hmac_handle_asoc_rsp_sta,
};


/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_uint32  hmac_send_connect_result_to_dmac_sta(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result);
OAL_STATIC oal_void  hmac_cfg80211_scan_comp_cb(void  *p_scan_record);
oal_void  hmac_send_rsp_to_sme_sta(hmac_vap_stru *pst_hmac_vap, hmac_sme_rsp_enum_uint8 en_type, oal_uint8 *puc_msg);


/*****************************************************************************
 函 数 名  : hmac_prepare_scan_req
 功能描述  : 准备扫描请求
 输入参数  : ul_chip_ver: 芯片版本
             pc_desired_ssid: mib中设置的ssid
 输出参数  : pst_scan_params: 扫描参数
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  hmac_prepare_scan_req(mac_scan_req_stru *pst_scan_params, oal_int8 *pc_desired_ssid)
{

#ifndef  _PRE_WLAN_FPGA_DIGITAL
    oal_uint8       uc_chan_idx;
    oal_uint32      ul_ret;
    oal_uint8       uc_2g_chan_num = 0;
    oal_uint8       uc_5g_chan_num = 0;
    oal_uint8       uc_chan_number;
#endif
    oal_uint8       auc_bcst_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    OAL_MEMZERO(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* 设置初始扫描请求的参数 */
    pst_scan_params->en_bss_type         = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type        = WLAN_SCAN_TYPE_ACTIVE;
    pst_scan_params->us_scan_time        = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    pst_scan_params->uc_probe_delay      = 0;
    pst_scan_params->uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* 默认扫描bss */
    pst_scan_params->p_fn_cb             = hmac_cfg80211_scan_comp_cb;      /* 使用cfg80211接口回调，因为ut/dmt都跟这个接口耦合 */
    pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;

    /* 设置初始扫描请求的ssid */
    /* 初始扫描请求只指定1个ssid, 要么是通配, 要么是mib值 */
    if ('\0' == pc_desired_ssid[0])
    {
        pst_scan_params->ac_ssid[0][0] = '\0';   /* 通配ssid */
    }
    else
    {
        oal_memcopy(pst_scan_params->ac_ssid[0], pc_desired_ssid, OAL_STRLEN(pc_desired_ssid) + 1);    /* copy包括字符串结尾的0 */
    }

    pst_scan_params->uc_ssid_num = 1;

    /* 设置初始扫描请求的bssid */
    /* 初始扫描请求只指定1个bssid，为广播地址 */
    oal_set_mac_addr(pst_scan_params->auc_bssid[0], auc_bcst_addr);
    pst_scan_params->uc_bssid_num = 1;

#ifndef  _PRE_WLAN_FPGA_DIGITAL

#ifdef _PRE_WIFI_DMT
    if (0 == g_ul_dmt_scan_flag)
    {
        OAL_IO_PRINT("2G scan \n");
        for (uc_chan_idx = 0; uc_chan_idx < 2; uc_chan_idx++)
#else
        /* 2G初始扫描信道, 全信道扫描 */
        for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
#endif
        {
            /* 判断信道是不是在管制域内 */
            ul_ret = mac_is_channel_idx_valid(WLAN_BAND_2G, uc_chan_idx);
            if (OAL_SUCC == ul_ret)
            {
                mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_chan_idx, &uc_chan_number);

                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_chan_number;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_idx         = uc_chan_idx;
                pst_scan_params->uc_channel_nums++;
                uc_2g_chan_num++;
            }
        }

#ifdef _PRE_WIFI_DMT
    }
#endif

    OAM_INFO_LOG1(0, OAM_SF_SCAN, "{hmac_prepare_scan_req::after regdomain filter, the 2g total channel num is %d", uc_2g_chan_num);

#ifdef _PRE_WIFI_DMT
    if (1 == g_ul_dmt_scan_flag)
    {
        OAL_IO_PRINT("5G scan \n");
        for (uc_chan_idx = 0; uc_chan_idx < 2; uc_chan_idx++)
#else
    /* 5G初始扫描信道, 全信道扫描 */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
#endif
    {
        /* 判断信道是不是在管制域内 */
        ul_ret = mac_is_channel_idx_valid(WLAN_BAND_5G, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx(WLAN_BAND_5G, uc_chan_idx, &uc_chan_number);

            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_number = uc_chan_number;
            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_band        = WLAN_BAND_5G;
            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_idx         = uc_chan_idx;
            pst_scan_params->uc_channel_nums++;
            uc_5g_chan_num++;
        }
    }
#ifdef _PRE_WIFI_DMT
    }
#endif
    OAM_INFO_LOG1(0, OAM_SF_SCAN, "{hmac_prepare_scan_req::after regdomain filter, the 5g total channel num is %d", uc_5g_chan_num);
#endif
}
OAL_STATIC oal_void  hmac_cfg80211_scan_comp_cb(void  *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    hmac_scan_rsp_stru               st_scan_rsp;

    OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_scan_comp_cb::cfg80211 scan comp, callback func proc.");

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_scan_comp_cb::pst_hmac_vap is null.");
        return;
    }

    /* 上报扫描结果结构体初始化 */
    OAL_MEMZERO(&st_scan_rsp, OAL_SIZEOF(st_scan_rsp));

    st_scan_rsp.uc_result_code = pst_scan_record->en_scan_rsp_status;

    /* 扫描结果发给sme */
    hmac_send_rsp_to_sme_sta(pst_hmac_vap, HMAC_SME_SCAN_RSP, (oal_uint8 *)&st_scan_rsp);

    return;
}
OAL_STATIC oal_void  hmac_cfg80211_prepare_scan_req_sta(mac_scan_req_stru *pst_scan_params, oal_int8 *puc_param)
{
    oal_uint8                       uc_loop;            /* for循环数组下标 */
    oal_uint8                       uc_channel_number;  /* 信道号 */
    oal_uint8                       uc_channel_idx;     /* 信道号索引 */
    oal_uint8                       auc_bcst_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    mac_cfg80211_scan_param_stru   *pst_cfg80211_scan_param;
    oal_uint32                      ul_ret;
    oal_uint8                       uc_chan_num_2g = 0;
    oal_uint8                       uc_chan_num_5g = 0;

    pst_cfg80211_scan_param = (mac_cfg80211_scan_param_stru *)puc_param;

    OAL_MEMZERO(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    pst_scan_params->en_bss_type  = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type = pst_cfg80211_scan_param->en_scan_type;

    /* 设置扫描时间 */
    if (WLAN_SCAN_TYPE_ACTIVE == pst_scan_params->en_scan_type)
    {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    }
    else
    {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_PASSIVE_SCAN_TIME;
    }

    pst_scan_params->uc_probe_delay      = 0;
    pst_scan_params->uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* 默认扫描bss */
    pst_scan_params->p_fn_cb             = hmac_cfg80211_scan_comp_cb;      /* 扫描完成回调函数 */

    if ('\0' == pst_cfg80211_scan_param->st_ssids[0].auc_ssid[0])
    {
        pst_scan_params->ac_ssid[0][0] = '\0';   /* 通配ssid */

        /* 如果上层未下发指定ssid，则采用默认值(后续根据bcm的行为做对应调整) */
        pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    }
    else
    {
        for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->l_ssid_num; uc_loop++)
        {
            oal_memcopy(pst_scan_params->ac_ssid[uc_loop], pst_cfg80211_scan_param->st_ssids[uc_loop].auc_ssid,
                        pst_cfg80211_scan_param->st_ssids[uc_loop].uc_ssid_len);
            pst_scan_params->ac_ssid[uc_loop][pst_cfg80211_scan_param->st_ssids[uc_loop].uc_ssid_len] = '\0';   /* ssid末尾置'\0' */
        }

        /* 如果上层下发了指定ssid，则每次扫描发送的probe req帧的个数为下发的ssid个数 */
        pst_scan_params->uc_max_send_probe_req_count_per_channel = (oal_uint8)pst_cfg80211_scan_param->l_ssid_num;
    }

    pst_scan_params->uc_ssid_num = (oal_uint8)pst_cfg80211_scan_param->l_ssid_num;

    oal_set_mac_addr(pst_scan_params->auc_bssid[0], auc_bcst_addr);
    pst_scan_params->uc_bssid_num = 1;  /* 初始扫描请求指定1个bssid，为广播地址 */

    /* 2G初始扫描信道 */
    for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->uc_num_channels_2G; uc_loop++)
    {
        uc_channel_number = (oal_uint8)pst_cfg80211_scan_param->pul_channels_2G[uc_loop];

        /* 判断信道是不是在管制域内 */
        ul_ret = mac_is_channel_num_valid(WLAN_BAND_2G, uc_channel_number);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_idx_from_num(WLAN_BAND_2G, uc_channel_number, &uc_channel_idx);

            pst_scan_params->ast_channel_list[uc_chan_num_2g].en_band        = WLAN_BAND_2G;
            pst_scan_params->ast_channel_list[uc_chan_num_2g].uc_chan_number = uc_channel_number;
            pst_scan_params->ast_channel_list[uc_chan_num_2g].uc_idx         = uc_channel_idx;

            pst_scan_params->uc_channel_nums++;
            uc_chan_num_2g++;
        }
    }

    OAM_INFO_LOG2(0, OAM_SF_SCAN, "{hmac_cfg80211_prepare_scan_req_sta:: 2g total channel num[%d], after regdomain filter is %d",
      pst_cfg80211_scan_param->uc_num_channels_2G, uc_chan_num_2g);

    /* 5G初始扫描信道*/
    for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->uc_num_channels_5G; uc_loop++)
    {
        uc_channel_number = (oal_uint8)pst_cfg80211_scan_param->pul_channels_5G[uc_loop];

        /* 判断信道是不是在管制域内 */
        ul_ret = mac_is_channel_num_valid(WLAN_BAND_5G, uc_channel_number);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_idx_from_num(WLAN_BAND_5G, uc_channel_number, &uc_channel_idx);

            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].en_band        = WLAN_BAND_5G;
            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].uc_chan_number = uc_channel_number;
            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].uc_idx         = uc_channel_idx;

            pst_scan_params->uc_channel_nums++;
            uc_chan_num_5g++;
        }
    }

    OAM_INFO_LOG2(0, OAM_SF_SCAN, "{hmac_cfg80211_prepare_scan_req_sta:: 5g total channel num[%d], after regdomain filter is %d",
      pst_cfg80211_scan_param->uc_num_channels_5G, uc_chan_num_5g);

#ifdef _PRE_WLAN_FEATURE_P2P
    /* WLAN/P2P 共存时，判断是否p2p0 发起扫描 */
    pst_scan_params->bit_is_p2p0_scan = pst_cfg80211_scan_param->bit_is_p2p0_scan;
    if (pst_cfg80211_scan_param->bit_is_p2p0_scan)
    {
        pst_scan_params->en_bss_type = 0;
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */
}

/*****************************************************************************
 函 数 名  : hmac_prepare_join_req
 功能描述  : 准备加入请求
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  hmac_prepare_join_req(hmac_join_req_stru *pst_join_req, mac_bss_dscr_stru *pst_bss_dscr)
{
    OAL_MEMZERO(pst_join_req, OAL_SIZEOF(hmac_join_req_stru));

    oal_memcopy(&(pst_join_req->st_bss_dscr), pst_bss_dscr, OAL_SIZEOF(mac_bss_dscr_stru));

    pst_join_req->us_join_timeout = WLAN_JOIN_START_TIMEOUT;
    pst_join_req->us_probe_delay  = WLAN_PROBE_DELAY_TIME;
}

/*****************************************************************************
 函 数 名  : hmac_prepare_auth_req
 功能描述  : 准备认证请求
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  hmac_prepare_auth_req(mac_vap_stru *pst_mac_vap, hmac_auth_req_stru *pst_auth_req)
{
    OAL_MEMZERO(pst_auth_req, OAL_SIZEOF(hmac_auth_req_stru));

    /* 增加AUTH报文次数后，将超时时间减半 */
    pst_auth_req->us_timeout = (oal_uint16)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11AuthenticationResponseTimeOut >> 1;
}

/*****************************************************************************
 函 数 名  : hmac_prepare_asoc_req
 功能描述  : 准备关联请求
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  hmac_prepare_asoc_req(mac_vap_stru *pst_mac_vap, hmac_asoc_req_stru *pst_asoc_req)
{
    OAL_MEMZERO(pst_asoc_req, OAL_SIZEOF(hmac_asoc_req_stru));

    /* 增加ASOC报文次数后，将超时时间减半 */
    pst_asoc_req->us_assoc_timeout = (oal_uint16)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11AssociationResponseTimeOut >> 1;
}

/*****************************************************************************
 函 数 名  : hmac_sta_initiate_scan
 功能描述  : STA发起初始扫描
 输入参数  : pst_hmac_vap: HMAC VAP
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  hmac_sta_initiate_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32              ul_ret;
    mac_scan_req_stru       st_scan_params;
    hmac_vap_stru          *pst_hmac_vap;
    oal_int8               *pc_desired_ssid;

#ifdef  _PRE_WLAN_FPGA_DIGITAL
    oal_uint8               uc_chan_number = 0;
    mac_device_stru        *pst_mac_device;
#endif

#ifdef  _PRE_WLAN_FEATURE_P2P
    mac_device_stru        *pst_mac_device;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initial_scan: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 针对配置vap做保护 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_initiate_scan::this is config vap! can't get info.}");
        return OAL_FAIL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_scan::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_desired_ssid = (oal_int8 *)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;
    hmac_prepare_scan_req(&st_scan_params, pc_desired_ssid);
#ifdef _PRE_WLAN_FEATURE_P2P//TBD:P2P BUG
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    st_scan_params.bit_is_p2p0_scan = puc_param[0];
    /* 记录扫描参数信息到device结构体 */
    oal_memcopy(&(pst_mac_device->st_scan_params), &st_scan_params, OAL_SIZEOF(mac_scan_req_stru));
#endif  /* _PRE_WLAN_FEATURE_P2P */

#ifdef _PRE_WLAN_FPGA_DIGITAL
    /* 初始扫描列表清0 */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_scan::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 数字对通环境，只扫描1个信道 */
    st_scan_params.ast_channel_list[0].en_band = pst_hmac_vap->st_vap_base_info.st_channel.en_band;
    mac_get_channel_num_from_idx(pst_hmac_vap->st_vap_base_info.st_channel.en_band, 0, &uc_chan_number);
    st_scan_params.ast_channel_list[0].uc_chan_number = uc_chan_number;
    st_scan_params.uc_channel_nums++;
    /*数字对通环境，使用配置的信道宽度进行扫描，不像空口只使用20m频宽扫描*/
    st_scan_params.ast_channel_list[0].en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
#endif

    /* 状态机调用: hmac_mgmt_scan_req */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&st_scan_params));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_sta_initiate_scan::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
oal_uint32  hmac_cfg80211_check_can_start_sched_scan(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru       *pst_mac_device;
    mac_vap_stru          *pst_mac_vap_tmp = OAL_PTR_NULL;
    oal_uint32             ul_ret = OAL_SUCC;

    /* 获取mac device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果vap的模式不是STA，则返回，不支持其它模式的vap的调度扫描 */
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::vap mode[%d] don't support sched scan.}",
                         pst_mac_vap->en_vap_mode);

        pst_mac_device->st_wiphy_mgmt.pst_sched_scan_req = OAL_PTR_NULL;
        pst_mac_device->st_wiphy_mgmt.en_sched_scan_complete = OAL_TRUE;
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* 如果存在当前device存在up的vap，则不启动调度扫描 */
    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap_tmp);
    if (OAL_SUCC == ul_ret)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::exist up vap, don't start sched scan.}");

        pst_mac_device->st_wiphy_mgmt.pst_sched_scan_req = OAL_PTR_NULL;
        pst_mac_device->st_wiphy_mgmt.en_sched_scan_complete = OAL_TRUE;
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    return OAL_SUCC;
}
oal_uint32  hmac_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                       *pst_hmac_vap;
    mac_pno_scan_stru                   *pst_cfg80211_pno_scan_params;
    mac_pno_scan_stru                    st_pno_scan_params;
    oal_uint32                           ul_ret = OAL_FAIL;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap) || OAL_UNLIKELY(OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_cfg80211_start_sched_scan::input null %p %p.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 上层传的是地址，因此需要取值获取到真正的pno扫描参数所在的地址 */
    pst_cfg80211_pno_scan_params = (mac_pno_scan_stru *)(*(oal_uint *)puc_param);

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cfg80211_start_sched_scan::pst_hmac_vap null.}");

        OAL_MEM_FREE(pst_cfg80211_pno_scan_params, OAL_TRUE);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 拷贝上层下发的pno扫描参数 */
    oal_memcopy(&st_pno_scan_params, pst_cfg80211_pno_scan_params, OAL_SIZEOF(mac_pno_scan_stru));

    /* 释放本地内存池的内存 */
    OAL_MEM_FREE(pst_cfg80211_pno_scan_params, OAL_TRUE);

    /* 检测当前device是否可以启动调度扫描 */
    ul_ret = hmac_cfg80211_check_can_start_sched_scan(pst_mac_vap);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /* 设置PNO调度扫描结束时，如果有结果上报，则上报扫描结果的回调函数 */
    st_pno_scan_params.p_fn_cb = hmac_cfg80211_scan_comp_cb;

    /* 状态机调用: hmac_scan_proc_scan_req_event */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCHED_SCAN_REQ, (oal_void *)(&st_pno_scan_params));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cfg80211_start_sched_scan::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
                         抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap,
                                    WLAN_CFGID_STOP_SCHED_SCAN,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_cfg80211_stop_sched_scan::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_cfg80211_start_scan_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_scan_req_stru                    st_scan_params;
    hmac_vap_stru                       *pst_hmac_vap;
    mac_device_stru                     *pst_mac_device;
    mac_cfg80211_scan_param_pst_stru    *pst_cfg80211_scan_param_pst;
    oal_app_ie_stru                      st_app_ie;
    oal_uint32                           ul_ret = OAL_FAIL;
#ifdef _PRE_WLAN_FPGA_DIGITAL
    oal_uint8                            uc_channel = 0;
#endif

    OAL_MEMZERO(&st_scan_params, OAL_SIZEOF(mac_scan_req_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap) || OAL_UNLIKELY(OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::scan failed, null ptr, %p %p.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg80211_scan_param_pst = (mac_cfg80211_scan_param_pst_stru *)puc_param;

    /* 初始扫描列表清0 */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::scan failed, null ptr, pst_mac_device null.}");

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
        /* ut do noting */
#else
        /* 因为扫描命令下发修改为非阻塞方式，因此，信道申请的内存需要再此处释放 */
        oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_2G);
        oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_5G);
#endif
        OAL_MEM_FREE(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param, OAL_FALSE);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::scan failed, null ptr, pst_hmac_vap null.}");

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
        /* ut do noting */
#else
        /* 因为扫描命令下发修改为非阻塞方式，因此，信道申请的内存需要再此处释放 */
        oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_2G);
        oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_5G);
#endif
        OAL_MEM_FREE(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param, OAL_FALSE);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 将内核下发的扫描参数更新到驱动扫描请求结构体中 */
    hmac_cfg80211_prepare_scan_req_sta(&st_scan_params, (oal_int8 *)pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param);

    /* 设置P2P/WPS IE 信息到 vap 结构体中 */
    if (IS_LEGACY_VAP(pst_mac_vap))
    {
        hmac_config_del_p2p_ie((oal_uint8 *)(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->puc_ie), &(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->ul_ie_len));
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P0 扫描时记录P2P listen channel */
    if (OAL_TRUE == pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->bit_is_p2p0_scan)
    {
        hmac_find_p2p_listen_channel(pst_mac_vap,
                                     (oal_uint16)(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->ul_ie_len),
                                     (oal_uint8 *)(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->puc_ie));
        st_scan_params.uc_p2p0_listen_channel = pst_mac_vap->uc_p2p_listen_channel;
    }
#endif
    st_app_ie.ul_ie_len = pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->ul_ie_len;
    oal_memcopy(st_app_ie.auc_ie, pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->puc_ie, st_app_ie.ul_ie_len);
    st_app_ie.en_app_ie_type = OAL_APP_PROBE_REQ_IE;
    hmac_config_set_app_ie_to_vap(pst_mac_vap, &st_app_ie, st_app_ie.en_app_ie_type);

#ifdef _PRE_WLAN_FPGA_DIGITAL
    for (uc_channel = 0; uc_channel < WLAN_MAX_CHANNEL_NUM; uc_channel++)
    {
        st_scan_params.ast_channel_list[uc_channel].en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
    }
#endif

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
            /* ut do noting */
#else
    /* 因为扫描命令下发修改为非阻塞方式，因此，信道申请的内存需要再此处释放 */
    oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_2G);
    oal_free(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param->pul_channels_5G);
#endif
    OAL_MEM_FREE(pst_cfg80211_scan_param_pst->pst_mac_cfg80211_scan_param, OAL_FALSE);

    /* 状态机调用: hmac_scan_proc_scan_req_event */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&st_scan_params));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : hmac_sta_initiate_join
 功能描述  : 发起加入
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  hmac_sta_initiate_join(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    hmac_join_req_stru  st_join_req;
    hmac_vap_stru      *pst_hmac_vap;
    oal_uint32          ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_bss_dscr)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ASSOC, "{hmac_sta_initiate_join::input null %x %x.}", pst_mac_vap, pst_bss_dscr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_initiate_join::mac_res_get_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    oal_memcopy(pst_hmac_vap->auc_supp_rates,pst_bss_dscr->auc_supp_rates ,pst_bss_dscr->uc_num_supp_rates);
    pst_hmac_vap->uc_rs_nrates = pst_bss_dscr->uc_num_supp_rates;

    hmac_prepare_join_req(&st_join_req, pst_bss_dscr);

    /* 状态机调用  */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_JOIN_REQ, (oal_void *)(&st_join_req));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_initiate_join::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : hmac_sta_initiate_auth
 功能描述  : 发起认证
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  hmac_sta_initiate_auth(mac_vap_stru *pst_mac_vap)
{
    hmac_auth_req_stru  st_auth_req;
    hmac_vap_stru      *pst_hmac_vap;
    oal_uint32          ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_auth: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_auth: pst_hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_prepare_auth_req(pst_mac_vap, &st_auth_req);

    /* 状态机调用  */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_AUTH_REQ, (oal_void *)(&st_auth_req));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_initiate_join::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : hmac_sta_initiate_asoc
 功能描述  : 发起关联
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  hmac_sta_initiate_asoc(mac_vap_stru *pst_mac_vap)
{
    hmac_asoc_req_stru  st_asoc_req;
    hmac_vap_stru      *pst_hmac_vap;
    oal_uint32          ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_sta_initiate_asoc::pst_mac_vap null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "hmac_sta_initiate_asoc: pst_hmac_vap null!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_prepare_asoc_req(pst_mac_vap, &st_asoc_req);

    /* 状态机调用  */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ASOC_REQ, (oal_void *)(&st_asoc_req));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "hmac_sta_initiate_asoc::hmac_fsm_call_func_sta fail[%d]", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


#if  defined(_PRE_WIFI_DMT ) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
/*****************************************************************************
 函 数 名  : hmac_join_bss_sta
 功能描述  : 加入扫描到的期望的AP列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月3日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  hmac_join_bss_sta(hmac_vap_stru *pst_hmac_vap)
{
    mac_bss_dscr_stru   *pst_bss_dscr;
    oal_uint32           ul_bss_idx;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "hmac_join_bss_sta::pst_mac_device null");
        return;
    }

    /* 是否有期望的AP存在 */
    if (pst_hmac_device->uc_desired_bss_num > 0)
    {
        ul_bss_idx   = pst_hmac_device->auc_desired_bss_idx[pst_hmac_device->uc_desired_bss_num - 1];  /* 从后往前取期望的bss */
        pst_bss_dscr = hmac_scan_find_scanned_bss_dscr_by_index(pst_hmac_vap->st_vap_base_info.uc_device_id, ul_bss_idx);

        pst_hmac_device->uc_desired_bss_num--;      /* 期望加入的AP个数-1 */

        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_join_bss_sta::try to join bss[%d].}", ul_bss_idx);

        /* 加入请求 */
        hmac_sta_initiate_join(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr);
    }

}


/*****************************************************************************
 函 数 名  : hmac_prepare_bss_list
 功能描述  : 生成期望加入的bss列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_prepare_bss_list(hmac_vap_stru *pst_hmac_vap)
{
    oal_int8               *pc_mib_ssid;
    oal_uint32              ul_bss_idx = 0;
    hmac_device_stru       *pst_hmac_device;
    hmac_bss_mgmt_stru     *pst_bss_mgmt;
    hmac_scanned_bss_info  *pst_scanned_bss;
    mac_bss_dscr_stru      *pst_bss_dscr;
    oal_dlist_head_stru    *pst_entry;
    oal_uint8               uc_device_id;

    /* 获取device id */
    uc_device_id = pst_hmac_vap->st_vap_base_info.uc_device_id;

    /* 获取hmac device结构体 */
    pst_hmac_device = hmac_res_get_mac_dev(uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ASSOC, "hmac_prepare_bss_list::pst_hmac_device null");
        return;
    }

    pc_mib_ssid = (oal_int8 *)pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;

    /* 没有设置期望的ssid, 继续发起扫描 */
    if (0 == pc_mib_ssid[0])
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_prepare_bss_list::desired ssid is not set.}");
        return;
    }

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 更新期望的ap列表，遍历扫描到的bss, 看是否是期望的 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (0 == oal_strcmp(pc_mib_ssid, pst_bss_dscr->ac_ssid))
        {
            /* 检测期望join 的AP能力信息是否匹配 */
            if(OAL_SUCC != hmac_check_capability_mac_phy_supplicant(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr))
            {
                continue;
            }

            pst_bss_dscr->en_desired = OAL_TRUE;

            pst_hmac_device->auc_desired_bss_idx[pst_hmac_device->uc_desired_bss_num] = (oal_uint8)ul_bss_idx;  /* 记录期望的AP在bss list中的索引号 */
            pst_hmac_device->uc_desired_bss_num++;
        }
        else
        {
            pst_bss_dscr->en_desired = OAL_FALSE;
        }

        ul_bss_idx++;
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    hmac_join_bss_sta(pst_hmac_vap);
}
#endif

/*****************************************************************************
 函 数 名  : hmac_handle_scan_rsp_sta
 功能描述  : 处理扫描结果
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_handle_scan_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event;
    hmac_scan_rsp_stru  *pst_scan_rsp;


    /* 抛扫描完成事件到WAL */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_scan_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_handle_scan_rsp_sta::FRW_EVENT_ALLOC fail.}");
        return;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA,
                       OAL_SIZEOF(hmac_scan_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_scan_rsp = (hmac_scan_rsp_stru *)pst_event->auc_event_data;

    oal_memcopy(pst_scan_rsp, puc_msg , OAL_SIZEOF(hmac_scan_rsp_stru));

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

#if  defined(_PRE_WIFI_DMT ) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* 准备期望加入的bss列表*/
    hmac_prepare_bss_list(pst_hmac_vap);
#endif

}

/*****************************************************************************
 函 数 名  : hmac_handle_join_rsp_sta
 功能描述  : 处理加入结果
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_handle_join_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    hmac_join_rsp_stru  *pst_join_rsp = (hmac_join_rsp_stru *)puc_msg;

    if (HMAC_MGMT_SUCCESS == pst_join_rsp->en_result_code)
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_join_rsp_sta::join succ.}");

        /* 初始化AUTH次数 */
        pst_hmac_vap->uc_auth_cnt = 1;
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));
    }
    else
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "hmac_handle_join_rsp_sta::join fail[%d]", pst_join_rsp->en_result_code);

        hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);
    }
}
oal_void hmac_report_connect_failed_result(hmac_vap_stru *pst_hmac_vap, mac_status_code_enum_uint16 reason_code)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event;
    hmac_asoc_rsp_stru   pst_asoc_rsp;

    memset(&pst_asoc_rsp, 0 ,sizeof(pst_asoc_rsp));
    pst_asoc_rsp.en_result_code = HMAC_MGMT_TIMEOUT;
    pst_asoc_rsp.en_status_code = reason_code;

    /* 抛加入完成事件到WAL */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_asoc_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_report_connect_failed_result::FRW_EVENT_ALLOC fail!}");
        return;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA,
                       OAL_SIZEOF(hmac_asoc_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    oal_memcopy(pst_event->auc_event_data, &pst_asoc_rsp, OAL_SIZEOF(hmac_asoc_rsp_stru));

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

}

/*****************************************************************************
 函 数 名  : hmac_handle_auth_rsp_sta
 功能描述  : 处理认证结果
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_handle_auth_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    hmac_auth_rsp_stru     *pst_auth_rsp = (hmac_auth_rsp_stru *)puc_msg;
    hmac_user_stru         *pst_hmac_user;

    if (HMAC_MGMT_SUCCESS == pst_auth_rsp->us_status_code)
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_handle_auth_rsp_sta::auth succ.}");

        /* 初始化AOSC次数 */
        pst_hmac_vap->uc_asoc_cnt = 1;
        hmac_sta_initiate_asoc(&pst_hmac_vap->st_vap_base_info);
        return;
    }

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "hmac_handle_auth_rsp_sta::auth fail[%d],cnt[%d]",
                     pst_auth_rsp->us_status_code, pst_hmac_vap->uc_auth_cnt);

    if ((MAC_UNSUPT_ALG == pst_auth_rsp->us_status_code ||
         MAX_AUTH_CNT <= pst_hmac_vap->uc_auth_cnt) &&
        (WLAN_WITP_AUTH_AUTOMATIC == pst_hmac_vap->en_auth_mode))
    {
        pst_hmac_vap->en_auth_mode = WLAN_WITP_AUTH_SHARED_KEY;
        /* 需要将状态机设置为 */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_JOIN_COMP);

        /* 更新AUTH的次数 */
        pst_hmac_vap->uc_auth_cnt = 0;

        /* 重新发起关联动作 */
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));

        return;
    }

    if (pst_hmac_vap->uc_auth_cnt < MAX_AUTH_CNT)
    {
        /* 需要将状态机设置为 */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_JOIN_COMP);

        /* 更新AUTH的次数 */
        pst_hmac_vap->uc_auth_cnt++;

        /* 重新发起关联动作 */
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));

        return;
    }

    hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);

    /* The MAC state is changed to fake up state. Further MLME     */
    /* requests are processed in this state.                       */
    hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);

    /* 获取用户指针 */
    pst_hmac_user = mac_res_get_hmac_user((oal_uint16)pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (OAL_PTR_NULL != pst_hmac_user)
    {
        /* 删除对应用户 */
        hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
    }
    else
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "auth pst_hmac_user NULL.");
    }

    /* 上报关联失败到wpa_supplicant */
    hmac_report_connect_failed_result(pst_hmac_vap, MAC_AUTH_TIMEOUT);

}

/*****************************************************************************
 函 数 名  : hmac_handle_asoc_rsp_sta
 功能描述  : 处理认证结果
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_handle_asoc_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event;
    hmac_asoc_rsp_stru  *pst_asoc_rsp = (hmac_asoc_rsp_stru *)puc_msg;
    hmac_user_stru      *pst_hmac_user;

    if(HMAC_MGMT_SUCCESS == pst_asoc_rsp->en_result_code)
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta::asoc succ.}");
        /* 抛加入完成事件到WAL */
        pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_asoc_rsp_stru));
        if (OAL_PTR_NULL == pst_event_mem)
        {
            OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_handle_asoc_rsp_sta::FRW_EVENT_ALLOC fail!}");
            return;
        }

        /* 填写事件 */
        pst_event = (frw_event_stru *)pst_event_mem->puc_data;

        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_CTX,
                           HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA,
                           OAL_SIZEOF(hmac_asoc_rsp_stru),
                           FRW_EVENT_PIPELINE_STAGE_0,
                           pst_hmac_vap->st_vap_base_info.uc_chip_id,
                           pst_hmac_vap->st_vap_base_info.uc_device_id,
                           pst_hmac_vap->st_vap_base_info.uc_vap_id);

        oal_memcopy(pst_event->auc_event_data, puc_msg, OAL_SIZEOF(hmac_asoc_rsp_stru));

        /* 分发事件 */
        frw_event_dispatch_event(pst_event_mem);
        FRW_EVENT_FREE(pst_event_mem);
    }
    else
    {
        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "hmac_handle_asoc_rsp_sta::asoc fail[%d]",
          pst_asoc_rsp->en_result_code, pst_hmac_vap->uc_asoc_cnt);

        if (pst_hmac_vap->uc_asoc_cnt >= MAX_ASOC_CNT)
        {
            /* 获取用户指针 */
            pst_hmac_user = mac_res_get_hmac_user((oal_uint16)pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
            if (NULL != pst_hmac_user)
            {
                /* 发送去认证帧到AP */
                hmac_mgmt_send_deauth_frame(&pst_hmac_vap->st_vap_base_info, pst_hmac_user->st_user_base_info.auc_user_mac_addr, MAC_AUTH_NOT_VALID, OAL_FALSE);

                /* 删除对应用户 */
                hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
            }
            else
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "asoc:pst_hmac_user NULL.");
            }

            /* 设置状态为FAKE UP */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);

            /* 同步DMAC状态 */
            hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);

            /* 上报关联失败到wpa_supplicant */
            hmac_report_connect_failed_result(pst_hmac_vap, pst_asoc_rsp->en_status_code);
        }
        else
        {
            /* 需要将状态机设置为 */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_AUTH_COMP);

            /* 发起ASOC的次数 */
            pst_hmac_vap->uc_asoc_cnt++;

            /* 重新发起关联动作 */
            hmac_sta_initiate_asoc(&pst_hmac_vap->st_vap_base_info);
        }
    }

}



/*****************************************************************************
 函 数 名  : hmac_send_rsp_to_sme
 功能描述  : 将连接状态机结果上报给SME
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_send_rsp_to_sme_sta(hmac_vap_stru *pst_hmac_vap, hmac_sme_rsp_enum_uint8 en_type, oal_uint8 *puc_msg)
{

#ifdef _PRE_WIFI_DMT
    /* 进行DMT结果上报，用例验证预期结果 */
    hmac_scan_rsp_stru              *pst_scan_rsp;
    hmac_join_rsp_stru              *pst_join_rsp;
    hmac_auth_rsp_stru              *pst_auth_rsp;
    hmac_asoc_rsp_stru              *pst_asoc_rsp;

    EXPECT_EVENT_STATUS              stEventStatus;

    stEventStatus.ucMsgType   = en_type;
    stEventStatus.usMsgStatus = pst_hmac_vap->st_vap_base_info.en_vap_state;
    stEventStatus.usMsgLength= sizeof(EXPECT_EVENT_STATUS);

    if (HMAC_SME_SCAN_RSP == en_type)
    {
        pst_scan_rsp = (hmac_scan_rsp_stru *)puc_msg;
        stEventStatus.usMsgSubStatus = pst_scan_rsp->uc_result_code;
    }
    else if (HMAC_SME_JOIN_RSP == en_type)
    {
        pst_join_rsp = (hmac_join_rsp_stru *)puc_msg;
        stEventStatus.usMsgSubStatus = pst_join_rsp->en_result_code;
    }
    else if (HMAC_SME_AUTH_RSP == en_type)
    {
        pst_auth_rsp = (hmac_auth_rsp_stru *)puc_msg;
        stEventStatus.usMsgSubStatus = pst_auth_rsp->us_status_code;
    }
    else if (HMAC_SME_ASOC_RSP == en_type)
    {
        pst_asoc_rsp = (hmac_asoc_rsp_stru *)puc_msg;
        stEventStatus.usMsgSubStatus = pst_asoc_rsp->en_result_code;
    }

    OAL_IO_PRINT("[EVENT_REPORT] RspType=%d VapState=%d ResultCode=%d \n", \
                  stEventStatus.ucMsgType, stEventStatus.usMsgStatus, stEventStatus.usMsgSubStatus);

    DmtStub_Sta2DmtSendMsg(DEV_DMT_EVENT_STATUS_MSG, (oal_uint8 *)&stEventStatus, sizeof(EXPECT_EVENT_STATUS));
#endif

    g_handle_rsp_func_sta[en_type](pst_hmac_vap, puc_msg);
}

/*****************************************************************************
 函 数 名  : hmac_send_rsp_to_sme_ap
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月5日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
oal_void  hmac_send_rsp_to_sme_ap(hmac_vap_stru *pst_hmac_vap, hmac_ap_sme_rsp_enum_uint8 en_type, oal_uint8 *puc_msg)
{
    /* 目前只有DMT使用 */

#ifdef _PRE_WIFI_DMT
    /* 进行DMT结果上报，用例验证预期结果 */
    hmac_ap_start_rsp_stru   *pst_ap_start_rsp;
    EXPECT_EVENT_STATUS       stEventStatus;

    stEventStatus.ucMsgType   = en_type;
    stEventStatus.usMsgStatus = pst_hmac_vap->st_vap_base_info.en_vap_state;
    stEventStatus.usMsgLength= sizeof(EXPECT_EVENT_STATUS);

    if (HMAC_AP_SME_START_RSP == en_type)
    {
        pst_ap_start_rsp = (hmac_ap_start_rsp_stru *)puc_msg;
        stEventStatus.usMsgSubStatus = pst_ap_start_rsp->en_result_code;
    }

    OAL_IO_PRINT("[EVENT_REPORT] RspType=%d VapState=%d ResultCode=%d \n", \
                  stEventStatus.ucMsgType, stEventStatus.usMsgStatus, stEventStatus.usMsgSubStatus);

    DmtStub_Sta2DmtSendMsg(DEV_DMT_EVENT_STATUS_MSG, (oal_uint8 *)&stEventStatus, sizeof(EXPECT_EVENT_STATUS));
#endif
}


/*****************************************************************************
 函 数 名  : hmac_send_connect_fail_to_dmac_sta
 功能描述  : 入网失败通知DMAC
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月15日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  hmac_send_connect_result_to_dmac_sta(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result)
{
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event;

    /* 抛事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(oal_uint32));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "pst_event_mem null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_CONN_RESULT,
                       OAL_SIZEOF(oal_uint32),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    *((oal_uint32 *)(pst_event->auc_event_data)) = ul_result;

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}





/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_sta_initiate_scan);
oal_module_symbol(hmac_sta_initiate_join);
oal_module_symbol(hmac_cfg80211_start_scan_sta);
oal_module_symbol(hmac_cfg80211_start_sched_scan);
oal_module_symbol(hmac_cfg80211_stop_sched_scan);
/*lint +e578*//*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

