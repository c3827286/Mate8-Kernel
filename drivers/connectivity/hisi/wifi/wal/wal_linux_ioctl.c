/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : wal_linux_ioctl.c
  版 本 号   : 初稿
  作    者   : zhangheng
  生成日期   : 2012年12月10日
  最近修改   :
  功能描述   : linux ioctl配置命令
  函数列表   :
  修改历史   :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 创建文件

******************************************************************************/


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_profiling.h"
#include "oal_kernel_file.h"

#include "oam_ext_if.h"
#include "frw_ext_if.h"

#include "wlan_spec.h"
#include "wlan_types.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_ie.h"

#include "hmac_ext_if.h"
#include "hmac_chan_mgmt.h"

#include "wal_main.h"
#include "wal_config.h"
#include "wal_regdb.h"
#include "wal_linux_scan.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_bridge.h"
#include "wal_linux_flowctl.h"
#include "wal_linux_event.h"
#include "hmac_resource.h"
#include "hmac_p2p.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "oal_hcc_host_if.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/notifier.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>
#endif
#include "hmac_arp_offload.h"
#endif


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
oal_int8    *g_pac_wal_all_vap[] =
{
    "Hisilicon0",
    "wlan0",
    "wlan-va1",
    "wlan-va2",
    "wlan-va3",
    "Hisilicon1",
    "wlan1",
    "wlan-vai1",
    "wlan-vai2",
    "wlan-vai3"
};

oal_int8    *g_pac_wal_switch_info[] =
{
    "log_level",
    "feature_log_switch"
};

#endif

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    /* UT模式下调用frw_event_process_all_event */
    extern oal_void  frw_event_process_all_event(oal_uint ui_data);
#endif
#include "dmac_alg_if.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_C
#define MAX_PRIV_CMD_SIZE   2047

/*****************************************************************************
  2 结构体定义
*****************************************************************************/
typedef struct
{
    oal_int8                    *pc_country;          /* 国家字符串 */
    mac_dfs_domain_enum_uint8    en_dfs_domain;       /* DFS 雷达标准 */
}wal_dfs_domain_entry_stru;

typedef struct
{
    oal_uint32 ul_ap_max_user;                      /* ap最大用户数 */
    oal_int8   ac_ap_mac_filter_mode[257];          /* AP mac地址过滤命令参数,最长256 */
}wal_ap_config_stru;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
typedef struct
{
    oal_uint8                     uc_mode;          /* 模式*/
    oal_uint8                     uc_band;          /* 频段 */
}wal_atcmdsrv_mode_stru;

typedef struct
{
    oal_uint32                   ul_datarate;          /* at命令配置的速率值 */
    oal_int8                    *puc_datarate;          /* 速率字符串*/
}wal_atcmdsrv_datarate_stru;

#endif

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_STATIC oal_proc_dir_entry_stru *g_pst_proc_entry;

OAL_STATIC wal_ap_config_stru g_st_ap_config_info = {0};    /* AP配置信息,需要在vap 创建后下发的 */

/* hi1102-cb add sys for 51/02 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct kobject     *gp_sys_kobject;
#endif


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 wal_hipriv_inetaddr_notifier_call(struct notifier_block *this, oal_uint event, oal_void *ptr);

OAL_STATIC struct notifier_block wal_hipriv_notifier = {
    .notifier_call = wal_hipriv_inetaddr_notifier_call
};

oal_int32 wal_hipriv_inet6addr_notifier_call(struct notifier_block *this, oal_uint event, oal_void *ptr);

OAL_STATIC struct notifier_block wal_hipriv_notifier_ipv6 = {
    .notifier_call = wal_hipriv_inet6addr_notifier_call
};
#endif



/* 静态函数声明 */
OAL_STATIC oal_uint32  wal_hipriv_global_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_vap_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_vap_log_level(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_feature_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_log_ratelimit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
OAL_STATIC oal_uint32  wal_hipriv_set_mode_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_mcast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_bcast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_ucast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_mbcast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

OAL_STATIC oal_uint32  wal_hipriv_setcountry(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32   wal_hipriv_getcountry(oal_net_device_stru *pst_net_dev,oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_11D
OAL_STATIC oal_uint32   wal_hipriv_set_rd_by_ie_switch(oal_net_device_stru *pst_net_dev,oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_rate(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_mcs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_mcsac(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_rfch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_bw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_always_tx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
OAL_STATIC oal_uint32 wal_hipriv_always_tx_1102(oal_net_device_stru * pst_net_dev, oal_int8 * pc_param);
#endif

OAL_STATIC oal_uint32  wal_hipriv_always_rx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_thruput(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_freq_skew(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_adjust_ppm(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_user_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_add_vap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_event_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#if 0
OAL_STATIC oal_uint32  wal_hipriv_ota_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_hipriv_start_dpd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_hipriv_beacon_offload_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_ota_beacon_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ota_rx_dscr_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_set_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_tx_comp_cnt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_all_reg_value(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_DFT_EVENT
OAL_STATIC oal_void  wal_event_report_to_sdt(wal_msg_type_enum_uint8   en_msg_type, oal_uint8 *puc_param, wal_msg_stru *pst_cfg_msg);
#endif
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_80211_ucast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#ifdef _PRE_DEBUG_MODE_USER_TRACK
OAL_STATIC oal_uint32  wal_hipriv_report_thrput_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
OAL_STATIC oal_uint32  wal_hipriv_set_txop_ps_machw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
OAL_STATIC oal_uint32 wal_hipriv_btcoex_status_print(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32  wal_hipriv_set_80211_mcast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_all_80211_ucast(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_all_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_dhcp_arp_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_report_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32  wal_hipriv_set_rssi_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_random_mac_addr_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_probe_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_mpdu_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_all_ota(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_oam_output(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_add_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_del_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ampdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_auto_ba_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_profiling_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_addba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_delba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_mem_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_device_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_memory_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);

#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_hipriv_pci_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_pci_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_5115_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_5115_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
OAL_STATIC oal_uint32 wal_hipriv_max_user_num_each_device(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
OAL_STATIC oal_uint32  wal_hipriv_sdio_flowctrl(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_beacon_chain_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#if 0
OAL_STATIC oal_uint32  wal_hipriv_tdls_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_tdls_channel_switch_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_2040_channel_switch_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_FortyMHzIntolerant(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_2040_coext_support(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_rx_fcs_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_int32  wal_netdev_open(oal_net_device_stru *pst_net_dev);
OAL_STATIC oal_int32  wal_net_device_ioctl(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd);
OAL_STATIC oal_int32  wal_ioctl_get_assoc_req_ie(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32  wal_ioctl_set_auth_mode(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32  wal_ioctl_set_country_code(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_max_user(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32  wal_ioctl_nl80211_priv_connect(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32  wal_ioctl_nl80211_priv_disconnect(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_channel(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_wps_ie(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_frag(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_rts(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_net_device_stats_stru*  wal_netdev_get_stats(oal_net_device_stru *pst_net_dev);
OAL_STATIC oal_int32  wal_netdev_set_mac_addr(oal_net_device_stru *pst_net_dev, void *p_addr);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
OAL_STATIC oal_uint32  wal_ioctl_get_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_essid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_get_bss_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_bss_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_get_freq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_freq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_get_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_ioctl_set_beacon_interval(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_start_vap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#else
OAL_STATIC oal_int32  wal_ioctl_get_mode(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, void *p_param, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_mode(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, void *p_param, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_point_stru *pst_data, oal_int8 *pc_ssid);
OAL_STATIC oal_int32  wal_ioctl_set_essid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_point_stru *pst_data, oal_int8 *pc_ssid);
OAL_STATIC oal_int32  wal_ioctl_get_bss_type(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_uint32 *pul_type, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_bss_type(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_uint32 *pul_type, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_freq(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_freq_stru *pst_freq, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_freq(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_freq_stru *pst_freq, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_txpower(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_param_stru *pst_param, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_txpower(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_param_stru *pst_param, oal_int8 *pc_extra);
#endif
OAL_STATIC oal_int32  wal_ioctl_get_apaddr(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_sockaddr_stru           *pst_addr,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_iwrate(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_iwsense(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_rtsthres(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_fragthres(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_iwencode(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_point_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_iwrange(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_point_stru           *pst_param,
                oal_int8                    *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_param(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_param(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_iwname(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_int8* pc_name, oal_int8* pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_wme_params(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_wme_params(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_setcountry(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_w, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_getcountry(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_w, oal_int8 *pc_extra);
OAL_STATIC oal_uint32  wal_hipriv_amsdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_list_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_list_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_list_channel(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_regdomain_pwr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_regdomain_pwr_priv(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_start_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_start_join(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_start_deauth(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_timer(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_kick_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_pause_tid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_user_vip(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_vap_host(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_send_bar(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_ba_bitmap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_show_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_clear_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_user_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_show_vap_pkt_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_alg_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_timer_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_show_profiling(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ampdu_amsdu_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_reset_device(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_reset_operate(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
OAL_STATIC oal_uint32  wal_hipriv_set_oma(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_proxysta_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32  wal_hipriv_uapsd_debug(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32  wal_hipriv_set_phy_stat_en(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dbb_env_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_usr_queue_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_report_vap_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_report_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_report_ampdu_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_aggr_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_freq_adjust(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32  wal_hipriv_set_stbc_cap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_ldpc_cap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#ifdef _PRE_WLAN_DFT_REG
OAL_STATIC oal_uint32  wal_hipriv_dump_reg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_evt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_addr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif
OAL_STATIC oal_uint32  wal_hipriv_tpc_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ar_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_ar_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_rx_dscr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_tx_dscr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_memory(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_show_tx_dscr_addr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_alg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_event_queue_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_txbf_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_all_rx_dscr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_resume_rx_intr_fifo(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_hipriv_send_pspoll(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_send_nulldata(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

OAL_STATIC oal_uint32  wal_hipriv_frag_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_wmm_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_hide_ssid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_txbf_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#ifdef _PRE_WLAN_PERFORM_STAT
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_stat_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_stat_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

OAL_STATIC oal_uint32  wal_hipriv_display_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_display_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_display_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_display_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_display_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
OAL_STATIC oal_uint32   wal_hipriv_set_edca_opt_switch_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32   wal_hipriv_set_edca_opt_weight_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32   wal_hipriv_set_edca_opt_switch_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32   wal_hipriv_set_edca_opt_cycle_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_hipriv_lpm_soc_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_chip_state(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_psm_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_tx_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_wow_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

OAL_STATIC oal_uint32  wal_hipriv_remove_user_lut(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_data(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_probe_request(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

OAL_STATIC oal_uint32  wal_hipriv_send_frame(oal_net_device_stru * pst_net_dev, oal_int8 * pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_rx_pn(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_soft_retry(oal_net_device_stru * pst_net_dev, oal_int8 * pc_param);
OAL_STATIC oal_uint32  wal_hipriv_open_addr4(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_open_wmm_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_chip_test_open(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_coex(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_dfx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
OAL_STATIC oal_uint32 wal_hipriv_enable_pmf(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_clear_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC  oal_uint32 wal_hipriv_test_send_action(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
OAL_STATIC oal_uint32 wal_hipriv_set_default_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_test_add_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_mib(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_mib(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_thruput_bypass(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_auto_protection(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

OAL_STATIC oal_uint32  wal_hipriv_send_2040_coext(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_2040_coext_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_version(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

oal_int32 wal_ioctl_set_ssid(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
#ifdef _PRE_WLAN_FEATURE_DAQ
OAL_STATIC oal_uint32  wal_hipriv_data_acq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_get_smps_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
OAL_STATIC oal_uint32  wal_hipriv_dfs_radartool(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_SUPPORT_ACS
OAL_STATIC oal_uint32  wal_hipriv_acs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_DFT_REG
OAL_STATIC oal_uint32  wal_hipriv_dump_reg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_evt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_addr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
OAL_STATIC oal_uint32  wal_hipriv_set_opmode_notify(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_user_nssbw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif

OAL_STATIC oal_uint32  wal_hipriv_set_vap_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);


#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

/*{"blacklist_add",           wal_hipriv_blacklist_add},           1 */
OAL_STATIC oal_uint32  wal_hipriv_blacklist_add(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"blacklist_del",           wal_hipriv_blacklist_del},           2 */
OAL_STATIC oal_uint32  wal_hipriv_blacklist_del(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"blacklist_mode",          wal_hipriv_set_blacklist_mode},      3 */
OAL_STATIC oal_uint32  wal_hipriv_set_blacklist_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"blacklist_show",          wal_hipriv_blacklist_show},          4 wal_config_blacklist_show */
OAL_STATIC oal_uint32  wal_hipriv_blacklist_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"abl_on",                  wal_hipriv_set_abl_on},              5 */
OAL_STATIC oal_uint32  wal_hipriv_set_abl_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"abl_aging",               wal_hipriv_set_abl_aging},           6 */
OAL_STATIC oal_uint32  wal_hipriv_set_abl_aging(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"abl_threshold",           wal_hipriv_set_abl_threshold},       7 */
OAL_STATIC oal_uint32  wal_hipriv_set_abl_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"abl_reset",               wal_hipriv_set_abl_reset},           8 wal_config_set_autoblacklist_reset_time */
OAL_STATIC oal_uint32  wal_hipriv_set_abl_reset(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"isolation_mode",          wal_hipriv_set_isolation_mode},      9 */
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"isolation_type",          wal_hipriv_set_isolation_type},      10 */
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"isolation_fwd",           wal_hipriv_set_isolation_fwd},       11 */
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_fwd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"isolation_clear",         wal_hipriv_set_isolation_clear},     12 wal_config_set_isolation_clear */
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_clear(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
/*{"isolation_show",          wal_hipriv_set_isolation_show},      13 wal_config_isolation_show */
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
OAL_STATIC oal_uint32  wal_hipriv_m2u_snoop_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_m2u_add_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_m2u_cfg_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_m2u_show_snoop_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_igmp_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
OAL_STATIC oal_uint32  wal_hipriv_proxyarp_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_proxyarp_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32  wal_hipriv_wifi_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_pm_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);

#endif

OAL_STATIC oal_uint32  wal_hipriv_vap_classify_en(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_vap_classify_tid(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_scan_test(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC oal_uint32  wal_hipriv_sta_ps_mode(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_sta_ps_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32  wal_hipriv_sta_psm_param(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_psm_vote(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_sta_pm_open(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC oal_uint32  wal_hipriv_set_uapsd_para(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)&&(_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* hi1102-cb add sys for 51/02 */
/* OAL_STATIC ssize_t  wal_hipriv_sys_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t (*show)(struct device *dev, struct device_attribute *attr,char *buf);
ssize_t (*store)(struct device *dev, struct device_attribute *attr,const char *buf, size_t count); */
//OAL_STATIC oal_ssize_t  wal_hipriv_sys_write(oal_device_stru *dev, oal_device_attribute_stru *attr, const oal_int8 *buf, oal_size_t count);
//OAL_STATIC oal_ssize_t  wal_hipriv_sys_read(oal_device_stru *dev, oal_device_attribute_stru *attr, oal_int8 *buf);
OAL_STATIC oal_ssize_t  wal_hipriv_sys_write(oal_device_stru *dev, oal_device_attribute_stru *attr, const char *buf, oal_size_t count);
OAL_STATIC oal_ssize_t  wal_hipriv_sys_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *buf);
OAL_STATIC OAL_DEVICE_ATTR(hipriv, OAL_S_IRUGO|OAL_S_IWUSR, wal_hipriv_sys_read, wal_hipriv_sys_write);
/* hi1102-cb add sys for 51/02 */
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_hipriv_set_p2p_ps(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
OAL_STATIC oal_int32  wal_ioctl_set_p2p_noa(oal_net_device_stru * pst_net_dev, mac_cfg_p2p_noa_param_stru * pst_p2p_noa_param);
OAL_STATIC oal_int32  wal_ioctl_set_p2p_ops(oal_net_device_stru * pst_net_dev, mac_cfg_p2p_ops_param_stru * pst_p2p_ops_param);
OAL_STATIC oal_uint32  wal_hipriv_p2p_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif  /* _PRE_WLAN_FEATURE_P2P */

#ifdef _PRE_WLAN_FEATURE_WAPI
//OAL_STATIC oal_uint32  wal_hipriv_set_wapi(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32  wal_hipriv_show_wapi_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif/* #ifdef _PRE_WAPI_DEBUG */
#endif


oal_int32 wal_ioctl_set_wps_p2p_ie(oal_net_device_stru  *pst_net_dev,
                                    oal_uint8           *puc_buf,
                                    oal_uint32           ul_len,
                                    en_app_ie_type_uint8 en_type);

#ifdef _PRE_WLAN_PROFLING_MIPS
OAL_STATIC oal_uint32  wal_hipriv_set_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_show_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

OAL_STATIC oal_int32 wal_set_ap_max_user(oal_net_device_stru *pst_net_dev, oal_uint32 ul_ap_max_user);
OAL_STATIC oal_int32 wal_config_mac_filter(oal_net_device_stru *pst_net_dev, oal_int8 *pc_command);
OAL_STATIC oal_int32 wal_kick_sta(oal_net_device_stru *pst_net_dev, oal_uint8 *auc_mac_addr);
OAL_STATIC oal_int32  wal_ioctl_set_ap_config(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_get_assoc_list(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_mac_filters(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra);
OAL_STATIC oal_int32  wal_ioctl_set_ap_sta_disassoc(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra);
OAL_STATIC oal_uint32  wal_get_parameter_from_cmd(oal_int8 *pc_cmd, oal_int8 *pc_arg, OAL_CONST oal_int8 *puc_token, oal_uint32 *pul_cmd_offset, oal_uint32 ul_param_max_len);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_mmss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_show_arpoffload_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_uint32  wal_hipriv_get_tcp_ack_stream_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32  wal_hipriv_performance_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev);
oal_int32  wal_stop_vap(oal_net_device_stru *pst_net_dev);
OAL_STATIC oal_int32 wal_set_mac_addr(oal_net_device_stru *pst_net_dev);
oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, char *dev_name);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
OAL_STATIC oal_int32  wal_regdomain_update_for_dfs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country);
OAL_STATIC oal_int32  wal_regdomain_update(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country);
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
OAL_STATIC oal_uint32  wal_hipriv_send_cfg_uint32_data(oal_net_device_stru *pst_net_dev,
    oal_int8 *pc_param, wlan_cfgid_enum_uint16 cfgid);
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
OAL_STATIC oal_uint32 wal_hipriv_enable_2040bss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

/*****************************************************************************
  私有命令函数表. 私有命令格式:
         设备名 命令名 参数
  hipriv "Hisilicon0 create vap0"
*****************************************************************************/
/*Android private command strings*/
#define CMD_SET_AP_WPS_P2P_IE   "SET_AP_WPS_P2P_IE"
#define CMD_P2P_SET_NOA         "P2P_SET_NOA"
#define CMD_P2P_SET_PS          "P2P_SET_PS"
#define CMD_SET_POWER_ON        "SET_POWER_ON"
#define CMD_COUNTRY             "COUNTRY"

OAL_STATIC OAL_CONST wal_hipriv_cmd_entry_stru  g_ast_hipriv_cmd[] =
{
#ifdef _PRE_WLAN_FEATURE_PM
    {"pm_info",                 wal_hipriv_pm_info},                /* 输出低功耗PM信息 hipriv "Hisilicon0 pm_info"*/
    {"pm_enable",               wal_hipriv_pm_enable},                /* 输出低功耗PM信息 hipriv "Hisilicon0 pm_enable 0|1"*/
    {"enable",                  wal_hipriv_wifi_enable},            /* 开启或关闭wifi: hipriv "Hisilicon0 enable 0|1" */
#endif
    {"create",                  wal_hipriv_add_vap},                /* 创建vap私有命令为: hipriv "Hisilicon0 create vap0 ap|sta" */
    {"destroy",                 wal_hipriv_del_vap},                /* 删除vap私有命令为: hipriv "vap0 destroy" */
    {"info",                    wal_hipriv_vap_info},               /* 打印vap的所有参数信息: hipriv "vap0 info" */
    {"global_log_switch",       wal_hipriv_global_log_switch},      /* 全局日志开关:  hipriv "Hisilicon0 global_log_switch 0 | 1*/
    {"log_switch",              wal_hipriv_vap_log_switch},         /* VAP级别的日志开关: hipriv "Hisilicon0{VAPx} log_switch 0 | 1"，该命令针对所有的VAP */
    {"log_level",               wal_hipriv_vap_log_level},          /* VAP级别日志级别 hipriv "VAPX log_level {1|2}"  Warning与Error级别日志以VAP为维度 */
    {"feature_log_switch",      wal_hipriv_feature_log_switch},     /* 特性的INFO级别日志开关 hipriv "VAPX feature_name {0/1}"   */
    {"log_ratelimit",           wal_hipriv_log_ratelimit},          /* 特性的INFO级别日志开关 hipriv "Hisilicon0 log_ratelimit {type} {switch} {interval} {burst}"   */
    {"event_switch",            wal_hipriv_event_switch},           /* OAM event模块的开关的命令: hipriv "Hisilicon0 event_switch 0 | 1"，该命令针对所有的VAP */
#if 0
    {"ota_switch",              wal_hipriv_ota_switch},             /* 设置某一种具体的ota类型开关: hipriv "Hisilicon0 ota_switch ota_type(oam_ota_type_enum_uint8) 0 | 1"，该命令针对所有的VAP */
#endif
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
    {"start_dpd",               wal_hipriv_start_dpd},                /*Start DPD Calibration*/
#endif
#ifdef _PRE_WLAN_CHIP_TEST
    {"beacon_offload_test",     wal_hipriv_beacon_offload_test},    /* 手动设置host sleep状态，仅用于测试: hipriv "Hisilicon0 host_sleep 0 | 1" */
#endif
    {"ota_beacon_on",           wal_hipriv_ota_beacon_switch},      /* 设置是否上报beacon帧开关: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"，该命令针对所有的VAP */
    {"ota_switch",              wal_hipriv_ota_rx_dscr_switch},  /* 设置是否上报接收描述符帧开关: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1"，该命令针对所有的VAP */
    {"oam_output",              wal_hipriv_oam_output},             /* 设置oam模块的信息打印位置命令:hipriv "Hisilicon0 oam_output 0~4 (oam_output_type_enum_uint8)"，该命令针对所有的VAP */
    {"add_user",                wal_hipriv_add_user},               /* 设置添加用户的配置命令: hipriv "vap0 add_user xx xx xx xx xx xx(mac地址) 0 | 1(HT能力位) "  该命令针对某一个VAP */
    {"del_user",                wal_hipriv_del_user},               /* 设置删除用户的配置命令: hipriv "vap0 del_user xx xx xx xx xx xx(mac地址)" 该命令针对某一个VAP */
    {"ampdu_start",             wal_hipriv_ampdu_start},            /* 设置AMPDU开启的配置命令: hipriv "vap0  ampdu_start xx xx xx xx xx xx(mac地址) tidno" 该命令针对某一个VAP */
    {"auto_ba",                 wal_hipriv_auto_ba_switch},         /* 设置自动开始BA会话的开关:hipriv "vap0  auto_ba 0 | 1" 该命令针对某一个VAP */
    {"profiling",               wal_hipriv_profiling_switch},       /* 设置性能测试的开关:hipriv "vap0  profiling 0 | 1" 该命令针对某一个VAP */
    {"addba_req",               wal_hipriv_addba_req},              /* 设置建立BA会话的配置命令:hipriv "vap0 addba_req xx xx xx xx xx xx(mac地址) tidno ba_policy buffsize timeout" 该命令针对某一个VAP */
    {"delba_req",               wal_hipriv_delba_req},              /* 设置删除BA会话的配置命令: hipriv "vap0 delba_req xx xx xx xx xx xx(mac地址) tidno direction" 该命令针对某一个VAP */
    {"userinfo",                wal_hipriv_user_info},              /* 打印指定mac地址user的所有参数信息: hipriv "vap0 userinfo XX XX XX XX XX XX(16进制oal_strtohex)" */
    {"meminfo",                 wal_hipriv_mem_info},               /* 打印内存池信息: hipriv "Hisilicon0 meminfo poolid" */
    {"memleak",                 wal_hipriv_mem_leak},               /* 打印内存池信息: hipriv "Hisilicon0 memleak poolid" */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"devicememleak",           wal_hipriv_device_mem_leak},         /* 打印内存池信息: hipriv "Hisilicon0 devicememleak poolid" */
    {"memoryinfo",              wal_hipriv_memory_info},             /* 打印内存池信息: hipriv "Hisilicon0 memoryinfo host/device" */
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_CHIP_TEST
    {"pciregwrite",             wal_hipriv_pci_reg_write},          /* 写pci内部寄存器值: hipriv "Hisilicon0 pciregwrite <chip id>(芯片ID, 即PCIE编号) <addr>(寄存器地址) <val>(写入的4字节值)" */
    {"pciregread",              wal_hipriv_pci_reg_read},           /* 读pci内部寄存器值: hipriv "Hisilicon0 pciregread <chip id>(芯片ID, 即PCIE编号) <addr>(寄存器地址)" */
    {"regw5115",                wal_hipriv_5115_reg_write},         /* 写5115侧sys ctl or pci寄存器: hipriv "Hisilicon0 regw5115 <chip id>(芯片ID, 即PCIE编号) sys|pcie <addr>(寄存器地址) <val>(写入的4字节值)" */
    {"regr5115",                wal_hipriv_5115_reg_read},          /* 读5115侧sys ctl or pci寄存器: hipriv "Hisilicon0 regr5115 <chip id>(芯片ID, 即PCIE编号) sys|pcie <addr>(寄存器地址) */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
    {"max_user_num",            wal_hipriv_max_user_num_each_device},  /* 设置各设备关联用户个数: hipriv "Hisilicon0|1 max_user_num num"，0<=num<=32 */
#endif
    {"reginfo",                 wal_hipriv_reg_info},               /* 打印寄存器信息: hipriv "Hisilicon0 reginfo 16|32(51没有16位寄存器读取功能) regtype(soc/mac/phy) startaddr endaddr" */
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
    {"sdio_flowctrl",           wal_hipriv_sdio_flowctrl},
#endif
    {"beacon_chain_switch",     wal_hipriv_beacon_chain_switch},    /* 设置beacon帧发送策略配置命令: hipriv "vap0 beacon_chain_switch 0/1" 目前采取的是单通道模式(使用通道0)，0表示关闭双路轮流发送，1表示开启，该命令针对某一个VAP */
#if 0
    {"tdls_prohi",              wal_hipriv_tdls_prohibited},                       /* 设置tdls prohibited策略配置命令: hipriv "vap0  tdls_prohi 0/1" 0表示不禁用，1表示禁用 */
    {"tdls_chaswi_prohi",       wal_hipriv_tdls_channel_switch_prohibited},        /* 设置tdls channel switch prohibited策略配置命令: hipriv "vap0 tdls_chaswi_prohi 0/1" 0表示不禁用，1表示禁用 */
#endif
    {"2040_ch_swt_prohi",       wal_hipriv_2040_channel_switch_prohibited},        /* 设置20/40共存是否禁止信道切换: hipriv "vap0 2040_ch_swt_prohi 0|1" 0表示不禁止，1表示禁用 */
    {"2040_intolerant",         wal_hipriv_set_FortyMHzIntolerant},                /* 设置40MHz不允许位: hipriv "vap0 2040_intolerant 0|1" 0表示不允许运行40MHz，1表示允许运行40MHz */
    {"2040_coexistence",        wal_hipriv_set_2040_coext_support},                /* 设置20/40共存使能: hipriv "vap0 2040_coexistence 0|1" 0表示20/40MHz共存使能，1表示20/40MHz共存不使能 */
    {"rx_fcs_info",             wal_hipriv_rx_fcs_info},            /* 打印接收帧的FCS正确与错误信息:hipriv "vap0 rx_fcs_info 0/1" 0/1  0代表不清楚，1代表清楚 */
    {"set_ucast_data", wal_hipriv_set_ucast_data_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_ucast_data <param name> <value>" */
    {"set_mcast_data", wal_hipriv_set_mcast_data_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_mcast_data <param name> <value>" */
    {"set_bcast_data", wal_hipriv_set_bcast_data_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_bcast_data <param name> <value>" */
    {"set_ucast_mgmt", wal_hipriv_set_ucast_mgmt_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_ucast_mgmt <param name> <value>" */
    {"set_mbcast_mgmt",wal_hipriv_set_mbcast_mgmt_dscr_param},   /* 打印描述符信息: hipriv "vap0 set_mbcast_mgmt <param name> <value>" */
#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
    {"set_mode_ucast_data",     wal_hipriv_set_mode_ucast_data_dscr_param},         /* 设置指定模式单播数据帧描述符: hipriv "vap0 set_mode_ucast_data <protocol_mode> <param name> <value>" */
#endif
    {"rate",                    wal_hipriv_set_rate },               /* 设置non-HT模式下的速率:     hipriv "vap0 rate  <value>" */
    {"mcs",                     wal_hipriv_set_mcs  },               /* 设置HT模式下的速率:         hipriv "vap0 mcs   <value>" */
    {"mcsac",                   wal_hipriv_set_mcsac},               /* 设置VHT模式下的速率:        hipriv "vap0 mcsac <value>" */
    {"nss",                     wal_hipriv_set_nss  },               /* 设置HT模式下的空间流个数:   hipriv "vap0 nss   <value>" */
    {"rfch",                    wal_hipriv_set_rfch },               /* 设置发射通道:               hipriv "vap0 rfch  <value>" */
    {"bw",                      wal_hipriv_set_bw   },               /* 设置带宽:                   hipriv "vap0 bw    <value>" */
    {"al_tx",                   wal_hipriv_always_tx},               /* 设置常发模式:               hipriv "vap0 al_tx <value: 0/1/2> <ack policy: 0/1> <payload> <len>" value:0:关闭；1:RF；2:聚合。ack policy: 0:normal；1:no ack。*/
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    {"al_tx_1102",              wal_hipriv_always_tx_1102},          /* 设置常发模式:               hipriv "vap0 al_tx <value: 0/1/2>  <len>" 由于mac限制，11a,b,g下只支持4095以下数据发送,可以使用set_mcast_data对速率进行设置*/
#endif
    {"al_rx",                   wal_hipriv_always_rx},               /* 设置常收模式:               hipriv "vap0 al_rx <value: 0/1/2>" */
    {"get_thruput",             wal_hipriv_get_thruput},             /* 获取芯片的吞吐量数据        hipriv "vap0 get_thruput >" */
    {"set_freq_skew",           wal_hipriv_set_freq_skew},          /* 设置频偏数据                 hipriv "Hisilicon0 set_freq_skew
                                                                     * <idx chn T0Int20M T0Frac20M T1Int20M T1Frac20M T0Int40M T0Frac40M T1Int40M T1Frac40M>" */
    {"adjust_ppm",              wal_hipriv_adjust_ppm},             /* 设置ppm         hipriv "Hisilicon0 adjust_ppm ppm clock" */
    {"amsdu_start",             wal_hipriv_amsdu_start},            /* 打印寄存器信息: hipriv "vap0 amsdu_start xx xx xx xx xx xx(mac地址10进制oal_atoi) <max num> <max size> " */
    {"list_ap",                 wal_hipriv_list_ap},                /* 打印STA扫描到的AP列表: hipriv "sta0 list_ap" */
    {"list_sta",                wal_hipriv_list_sta},               /* 打印AP关联的STA列表: hipriv "sta0 list_sta" */
    {"start_scan",              wal_hipriv_start_scan},             /* 触发sta扫描: hipriv "sta0 start_scan" */
    {"start_join",              wal_hipriv_start_join},             /* 触发sta加入并认证关联: hipriv "sta0 start_join 1" 1表示扫描到的AP在device写数组下标号*/
    {"start_deauth",            wal_hipriv_start_deauth},           /* 触发sta去认证: hipriv "vap0 start_deauth" */
    {"dump_timer",              wal_hipriv_dump_timer},             /* 打印所有timer的维测信息 hipriv "vap0 dump_timer" */
    {"kick_user",               wal_hipriv_kick_user},              /* 删除1个用户 hipriv "vap0 kick_user xx xx xx xx xx xx(mac地址)" */
    {"pause_tid",               wal_hipriv_pause_tid},              /* 暂停指定用户的指定tid hipriv "vap0 pause_tid xx xx xx xx xx xx(mac地址) tid_num 0\1" */
    {"set_user_vip",            wal_hipriv_set_user_vip},           /* 设置某个用户为VIP或者非VIP，sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx(mac地址) 0\1" */
    {"set_vap_host",            wal_hipriv_set_vap_host},           /* 设置某个vap为host或者guest vap: sh hipriv.sh "vap0 st_vap_host 0\1" */
    {"ampdu_tx_on",             wal_hipriv_ampdu_tx_on},            /* 开启或关闭ampdu发送功能 hipriv "vap0 ampdu_tx_on 0\1" */
    {"amsdu_tx_on",             wal_hipriv_amsdu_tx_on},            /* 开启或关闭ampdu发送功能 hipriv "vap0 amsdu_tx_on 0\1" */
    {"send_bar",                wal_hipriv_send_bar},               /* 指定tid发送bar hipriv "vap0 send_bar A6C758662817(mac地址) tid_num" */
    {"regwrite",                wal_hipriv_reg_write},              /* 打印寄存器信息: hipriv "Hisilicon0 regwrite 32/16(51没有16位写寄存器功能) regtype(soc/mac/phy) addr val" addr val必须都是16进制0x开头 */
    {"packet_xmit",             wal_hipriv_packet_xmit},            /* 向目标STA/AP发送数据帧: hipriv "vap0 packet_xmit (tid_no) (报文个数) (报文长度) (RA MAC)" */
    {"dump_ba_bitmap",          wal_hipriv_dump_ba_bitmap},         /* 打印发送ba的bitmap hipriv "vap0 dump_ba_bitmap (tid_no) (RA)" */
    {"wifi_stat_info",          wal_hipriv_show_stat_info},         /* 获取所有维测统计信息: hipriv "Hisilicon0 wifi_stat_info" */
    {"vap_pkt_stat",            wal_hipriv_show_vap_pkt_stat},      /* 获取某一个vap下的收发包统计信息: sh hipriv.sh "vap_name vap_pkt_stat" */
    {"clear_stat_info",         wal_hipriv_clear_stat_info},        /* 清零所有维测统计信息: hipriv "Hisilicon0 clear_stat_info" */
    {"usr_stat_info",           wal_hipriv_user_stat_info},         /* 上报某个user下的维测统计信息: sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    {"timer_start",             wal_hipriv_timer_start},            /* 开启5115硬件定时器: hipriv "Hisilicon0 timer_start 0/1" */
    {"show_profiling",          wal_hipriv_show_profiling},         /* 开启5115硬件定时器: hipriv "Hisilicon0 show_profiling 0/1/2 (0是rx 1是tx 2是chipstart)" */
    {"ampdu_amsdu",             wal_hipriv_ampdu_amsdu_switch},     /* 设置amsdu ampdu联合聚合功能的开关:hipriv "vap0  ampdu_amsdu 0 | 1" 该命令针对某一个VAP */
   // {"reset_hw",                wal_hipriv_reset_device},           /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg) */
    {"reset_hw",                wal_hipriv_reset_device},           /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3|4|5|6|8|9|10|11
                                                                                                                    (all|phy|mac|debug|mac_tsf|mac_cripto|mac_non_cripto|phy_AGC|phy_HT_optional|phy_VHT_optional|phy_dadar )
                                                                                                                    0|1(reset phy reg) 0|1(reset mac reg) */
    {"reset_operate",           wal_hipriv_reset_operate},          /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg) */
    {"dump_rx_dscr",            wal_hipriv_dump_rx_dscr},           /* dump出来接收描述符队列，hipriv "Hisilicon0 dump_rx_dscr 0|1", 0:高优先级队列 1:普通优先级队列  */
    {"dump_tx_dscr",            wal_hipriv_dump_tx_dscr},           /* dump出来发送描述符队列，hipriv "Hisilicon0 dump_tx_dscr value", value取值0~3代表AC发送队列，4代表管理帧 */
    {"dump_memory",             wal_hipriv_dump_memory},            /* dump内存， hipriv "Hisilicon0 dump_memory 0xabcd len" */
    {"show_tx_dscr_addr",       wal_hipriv_show_tx_dscr_addr},      /* 打印内存池中所有发送描述符地址 hipriv "Hisilicon0 show_tx_dscr_addr" */
    {"list_channel",            wal_hipriv_list_channel},           /* 支持信道列表， hipriv "Hisilicon0 list_channel" */
    {"set_regdomain_pwr",       wal_hipriv_set_regdomain_pwr},      /* 设置管制域最大发送功率，hipriv "Hisilicon0 set_regdomain_pwr 20",单位dBm */
    {"set_regdomain_pwr_p",     wal_hipriv_set_regdomain_pwr_priv}, /* 设置管制域最大发送功率(可以突破管制域的限制)，hipriv "Hisilicon0 set_regdomain_pwr_priv 20",单位dBm */
    {"event_queue",             wal_hipriv_event_queue_info},       /* 打印事件队列信息，将打印出每一个非空事件队列中事件的个数，以及每一个事件头信息, hipriv "Hisilicon0 event_queue" */
    {"dump_all_dscr",           wal_hipriv_dump_all_rx_dscr},       /* 打印所有的接收描述符, hipriv "Hisilicon0 dump_all_dscr" */
    {"frag_threshold",          wal_hipriv_frag_threshold},         /* 设置分片门限的配置命令: hipriv "vap0 frag_threshold (len)" 该命令针对某一个VAP */
    {"wmm_switch",              wal_hipriv_wmm_switch},             /* 动态开启或者关闭wmm hipriv "vap0 wmm_switch 0|1"(0不使能，1使能)  */
    {"hide_ssid",               wal_hipriv_hide_ssid},              /*  隐藏ssid功能开启或者关闭 wmm hipriv "Hisilicon0 hide_ssid 0|1"(0不使能，1使能)  */
    {"ether_switch",            wal_hipriv_set_ether_switch},       /* 设置以太网帧上报的开关，sh hipriv.sh "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(帧方向) 0|1(开关)" */
    {"80211_uc_switch",         wal_hipriv_set_80211_ucast_switch}, /* 设置80211单播帧上报的开关，sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"80211_mc_switch",         wal_hipriv_set_80211_mcast_switch}, /* 设置80211组播\广播帧上报的开关，sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"probe_switch",            wal_hipriv_set_probe_switch},       /* 设置probe req与rsp上报的开关，sh hipriv.sh "Hisilicon0 probe_switch 0|1(帧方向tx|rx) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"rssi_switch",             wal_hipriv_set_rssi_switch},        /* 设置打印接收报文rssi信息的开关，sh hipriv.sh "Hisilicon0 rssi_switch 0|1(打开|关闭) N(间隔N个帧打印)" */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"report_vap_info",         wal_hipriv_report_vap_info},        /* 根据标记位上报vap的对应信息 sh hipriv.sh "wlan0 report_vap_info 1" */
#endif
    {"random_mac_addr_scan",    wal_hipriv_set_random_mac_addr_scan}, /* 设置随机mac addr扫描开关，sh hipriv.sh "Hisilicon0 random_mac_addr_scan 0|1(打开|关闭)" */
    {"mpdu_num",                wal_hipriv_get_mpdu_num},           /* 获取device下和每一个tid下当前mpdu个数，sh hipriv.sh "vap_name mpdu_num user_macaddr" */
    {"set_all_ota",             wal_hipriv_set_all_ota},            /* 设置所有ota上报，如果为1，则所有类型帧的cb描述符都报，如果为0，什么都不报，sh hipriv.sh "Hisilicon0 set_all_ota 0|1" */
    {"80211_uc_all",            wal_hipriv_set_all_80211_ucast},    /* 设置所有用户的单播开关，sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"ether_all",               wal_hipriv_set_all_ether_switch},   /* 设置所有用户的以太网开关，sh hipriv.sh "Hisilicon0 ether_all 0|1(帧方向tx|rx) 0|1(开关)" */
    {"dhcp_arp_switch",         wal_hipriv_set_dhcp_arp_switch},    /* 设置发送广播arp和dhcp开关，sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(开关)" */
#ifdef _PRE_DEBUG_MODE_USER_TRACK
    {"thrput_stat",             wal_hipriv_report_thrput_stat},     /* 上报或者停止上报反应user实时吞吐统计信息: sh hipriv.sh "vap_name thrput_stat  XX:XX:XX:XX:XX;XX 0|1" */
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    {"txopps_hw_en",            wal_hipriv_set_txop_ps_machw},      /* 设置mac txop ps使能寄存器，sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)" */
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    {"uapsd_debug",             wal_hipriv_uapsd_debug},            /* uapsd维测信息，sh hipriv "vap0 uapsd_debug 0|1|2(单用户|all user|清空统计计数器) xx:xx:xx:xx:xx:xx(mac地址)" */
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    {"coex_print",              wal_hipriv_btcoex_status_print},    /* 打印共存维测信息，sh hipriv.sh "coex_print" */
#endif
#ifdef _PRE_WLAN_DFT_STAT
    {"phy_stat_en",             wal_hipriv_set_phy_stat_en},        /* 设置phy统计使能节点编号，一次可以设置4个，参数范围1~16，sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4" */

    {"dbb_env_param",           wal_hipriv_dbb_env_param},          /* 上报或者停止上报空口环境类参数信息: sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */
    {"usr_queue_stat",          wal_hipriv_usr_queue_stat},         /* 上报或者清零用户队列统计信息: sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */
    {"vap_stat",                wal_hipriv_report_vap_stat},        /* 上报或者停止上报vap吞吐统计信息: sh hipriv.sh "vap_name vap _stat  0|1" */
    {"reprt_all_stat",          wal_hipriv_report_all_stat},        /* 上报或者清零所有维测统计信息: sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all)  0|1" */
#endif
#ifdef _PRE_DEBUG_MODE
    {"ampdu_stat",             wal_hipriv_report_ampdu_stat},      /* 上报或者清零ampdu维测统计信息: sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */
#endif

    {"ampdu_aggr_num",          wal_hipriv_set_ampdu_aggr_num},     /* 设置AMPDU聚合个数: sh hipriv.sh "Hisilicon0 ampdu_aggr_num aggr_num_switch aggr_num" ,aggr_num_switch非0时，aggr_num有效 */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"freq_adjust",             wal_hipriv_freq_adjust},            /* 频偏调整配置命令: sh hipriv.sh "Hisilicon0 freq_adjust pll_int pll_frac" ,pll_int整数分频系数，pll_frac小数分频系数 */
#endif

    {"set_stbc_cap",            wal_hipriv_set_stbc_cap},           /* 设置STBC能力 */
    {"set_ldpc_cap",            wal_hipriv_set_ldpc_cap},           /* 设置LDPC能力 */
#ifdef _PRE_WLAN_FEATURE_STA_PM
    {"set_ps_mode",             wal_hipriv_sta_ps_mode},           /* 设置PSPOLL能力 sh hipriv.sh 'wlan0 set_ps_mode 3 0'*/
#ifdef _PRE_PSM_DEBUG_MODE
    {"psm_info_debug",          wal_hipriv_sta_ps_info},            /* sta psm的维测统计信息 sh hipriv.sh 'wlan0 psm_info_debug 1' */
#endif
    {"set_psm_para",            wal_hipriv_sta_psm_param},          /* sh hipriv.sh 'wlan0 set_psm_para 100 40 */
    {"set_psm_vote",            wal_hipriv_psm_vote},               /* sh hipriv.sh 'wlan0 set_psm_vote xx xx xx xx */
    {"set_sta_pm_on",           wal_hipriv_sta_pm_open},            /* sh hipriv.sh 'wlan0 set_sta_pm_on xx xx xx xx */
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    {"set_uapsd_para",          wal_hipriv_set_uapsd_para},        /* 设置uapsd的参数信息 sh hipriv.sh 'wlan0 set_uapsd_para 3 1 1 1 1 */
#endif

#ifdef _PRE_WLAN_CHIP_TEST
    {"lpm_chip_state",          wal_hipriv_lpm_chip_state},         /* 睡眠或唤醒芯片, hipriv "Hisilicon0 lpm_chip_state 0|1|2(0:soft sleep，1:gpio sleep,2:work)" */
    {"lpm_soc_mode",            wal_hipriv_lpm_soc_mode},           /* 睡眠或唤醒芯片, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4(总线gating|PCIE RD BY PASS|mem precharge|PCIE L0-S|PCIE L1-0) 0|1(disable|enable)" */
    {"lpm_psm_param",           wal_hipriv_lpm_psm_param},          /* psm节能寄存器配置, hipriv "Hisilicon0 lpm_psm_param 0|1|2(ps off|ps on|debug) 0|1(DTIM|listen intval) xxx(listen interval值) xxx(TBTT offset)"*/
    {"lpm_smps_mode",           wal_hipriv_lpm_smps_mode},          /* smps节能模式配置, hipriv "Hisilicon0 lpm_smps_mode 0|1|2(off|static|dynamic)"*/
    {"lpm_smps_stub",           wal_hipriv_lpm_smps_stub},          /* smps ap发包打桩, hipriv "vap0 lpm_smps_stub 0|1|2(off|单流|双流) 0|1(是否发RTS)"*/
    {"lpm_txopps_set",          wal_hipriv_lpm_txopps_set},         /* txop ps节能模式配置, hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2 off|on)"*/
    {"lpm_txopps_tx_stub",      wal_hipriv_lpm_txopps_tx_stub},     /* txop ps发包测试打桩条件, hipriv "vap0 lpm_txopps_tx_stub 0|1|2(off|address|partial AID) xxx(第几个包打桩)"*/
    {"lpm_wow_en",              wal_hipriv_lpm_wow_en},             /* WOW模式开启, hipriv "Hisilicon0 lpm_wow_en 0|1|2(0不使能|1使能|2调试信息) 0|1(Null data是否唤醒)" "*/
    {"lpm_tx_data",             wal_hipriv_lpm_tx_data},            /* 测试发包, hipriv "vap0 lpm_tx_data xxx(个数) xxx(长度) xx:xx:xx:xx:xx:xx(目的mac) xxx(AC类型)"*/
    {"lpm_tx_probe_req",        wal_hipriv_lpm_tx_probe_request},   /* 测试发包, hipriv "vap0 lpm_tx_probe_req 0|1(被动|主动) xx:xx:xx:xx:xx:xx(主动模式下BSSID)"*/
    {"remove_lut",              wal_hipriv_remove_user_lut},        /* 删除恢复用户lut表, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac地址 16进制) 0|1(恢复/删除)" */
    {"send_frame",              wal_hipriv_send_frame},               /* 指定tid发送bar hipriv "vap0 send_frame (type) (num) (目的mac)" */
    {"set_rx_pn",               wal_hipriv_set_rx_pn},               /* 设置RX_PN_LUT_CONFIG寄存器 */
    {"set_sft_retry",           wal_hipriv_set_soft_retry},           /* 设置software_retry 描述符 hipriv "Hisilicon0 set_sft_retry 0|1(0不使能，1使能)"  */
    {"open_addr4",              wal_hipriv_open_addr4},             /* 设置mac头进入4地址 hipriv "Hisilicon0 open_addr4 0|1(0不使能，1使能)  */
    {"open_wmm_test",           wal_hipriv_open_wmm_test},         /* 设置芯片验证开关 hipriv "Hisilicon0 open_wmm_test 0|1|2|3  ()  */
    {"chip_test",               wal_hipriv_chip_test_open},         /* 设置芯片验证开关 hipriv "Hisilicon0 chip_test 0|1(0不使能，1使能)  */
    {"coex_ctrl",               wal_hipriv_set_coex},               /* 设置共存控制开关 hipriv "Hisilicon0 coex_ctrl xxx(mac ctrl值) xxx(rf ctrl值))  */
    {"dfx_en",                  wal_hipriv_set_dfx},                /* 设置DFX特性开关 sh hipriv.sh "Hisilicon0 dfx_en 0|1  */
    {"clear_all_stat",          wal_hipriv_clear_all_stat},         /* 清除中断和管理帧统计信息 hipriv "Hisilicon0 clear_all_stat" */

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    {"enable_pmf",              wal_hipriv_enable_pmf},     /* 设置chip test中强制使能pmf能力 (用于关联之后)sh hipriv.sh "vap0 enable_pmf 0|1|2(0不使能，1 enable, 2强制)  */
#endif
    {"send_action",             wal_hipriv_test_send_action},     /* 发送action帧接口 sh hipriv.sh "vap0 send_action XX(category) xx:xx:xx:xx:xx:xx(目的地址 16进制) " */
    {"send_pspoll",             wal_hipriv_send_pspoll},            /* sta发ps-poll给ap，sh hipriv "vap0 send_pspoll" */
    {"send_nulldata",           wal_hipriv_send_nulldata},          /* sta发null data给ap，通知节能状态，sh hipriv "vap0 send_nulldata 0|1(是否进入节能) 0|1(是否发qosnull) tid_no" */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
    {"set_default_key",         wal_hipriv_set_default_key},     /* 设置chip test中强制使能pmf能力 (用于关联之后)sh hipriv.sh "vap0 set_default_key x(key_index) 0|1(en_unicast) 0|1(multicast)"  */
    {"add_key",                 wal_hipriv_test_add_key},         /* chip test配置add key操作的私有配置命令接口
                          sh hipriv.sh "xxx(cipher) xx(en_pairwise) xx(key_len) xxx(key_index) xxxx:xx:xx:xx:xx:xx...(key 小于32字节) xx:xx:xx:xx:xx:xx(目的地址)  */

    /* 算法相关的命令 */
    {"alg_cfg",                 wal_hipriv_alg_cfg},                /* 算法参数配置: hipriv "vap0 alg_cfg sch_vi_limit 10"*/
    {"alg_ar_log",              wal_hipriv_ar_log},                 /* autorate算法日志参数配置:*/
    {"alg_ar_test",             wal_hipriv_ar_test},                /* autorate算法系统测试命令 */
    {"alg",                     wal_hipriv_alg},                    /* alg */
    {"alg_txbf_switch",         wal_hipriv_txbf_switch},            /* 开启或关闭tx beamforming能力 hipriv "vap0 alg_txbf_switch 0|1" */
    {"alg_txbf_log",            wal_hipriv_txbf_log},                 /* autorate算法日志参数配置:*/
    {"alg_tpc_log",             wal_hipriv_tpc_log},                /* tpc算法日志参数配置:*/

#ifdef _PRE_WLAN_FEATURE_DFS
    {"radartool",               wal_hipriv_dfs_radartool},
#endif
#ifdef _PRE_SUPPORT_ACS
    {"acs",                     wal_hipriv_acs},
#endif

#ifdef _PRE_WLAN_PERFORM_STAT
    /* 性能统计命令 */
    {"stat_tid_thrpt",          wal_hipriv_stat_tid_thrpt},        /* 统计指定tid的吞吐量: hipriv "vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_user_thrpt",         wal_hipriv_stat_user_thrpt},       /* 统计指定user的吞吐量: hipriv "vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_vap_thrpt",          wal_hipriv_stat_vap_thrpt},        /* 统计指定tid的吞吐量: hipriv "vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_tid_per",            wal_hipriv_stat_tid_per},          /* 统计指定tid的per: hipriv "vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_tid_delay",          wal_hipriv_stat_tid_delay},        /* 统计指定tid的delay: hipriv "vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */

    /* 性能显示命令 */
    {"dspl_tid_thrpt",          wal_hipriv_display_tid_thrpt},      /* 统计指定tid的吞吐量: hipriv "vap0 dspl_tid_thrpt xx xx xx xx xx xx(mac地址)" */
    {"dspl_user_thrpt",         wal_hipriv_display_user_thrpt},     /* 统计指定user的吞吐量: hipriv "vap0 dspl_user_thrpt xx xx xx xx xx xx(mac地址)" */
    {"dspl_vap_thrpt",          wal_hipriv_display_vap_thrpt},      /* 统计指定tid的吞吐量: hipriv "vap0 dspl_vap_thrpt" */
    {"dspl_tid_per",            wal_hipriv_display_tid_per},        /* 统计指定tid的per: hipriv "vap0 dspl_tid_per xx xx xx xx xx xx(mac地址) tid_num" */
    {"dspl_tid_delay",          wal_hipriv_display_tid_delay},      /* 统计指定tid的delay: hipriv "vap0 dspl_tid_delay xx xx xx xx xx xx(mac地址) tid_num" */
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    {"set_edca_switch_sta",        wal_hipriv_set_edca_opt_switch_sta},       /* STA是否开启私有edca参数优化机制 */
    {"set_edca_weight_sta",        wal_hipriv_set_edca_opt_weight_sta},       /* STA edca参数调整权重 */
    {"set_edca_switch_ap",         wal_hipriv_set_edca_opt_switch_ap},        /* 是否开启edca优化机制 */
    {"set_edca_cycle_ap",          wal_hipriv_set_edca_opt_cycle_ap},         /* 设置edca参数调整的周期 */
#endif

#ifdef _PRE_DEBUG_MODE
    /* 维测命令:设置某个值的某个类型*/
    {"debug_switch",            wal_hipriv_set_debug_switch},        /* 设置某一种具体的debug类型开关: hipriv "Hisilicon0 debug_switch debug_type debug_value"，该命令针对设备级别调试使用 */
    {"tx_comp_cnt",             wal_hipriv_get_tx_comp_cnt},         /* 统计发送完成中断是否丢失(关闭聚合) hipriv "Hisilicon0 tx_comp_cnt 0|1", 0表示清零统计次数， 1表示显示统计次数并且清零 */
    {"set_rx_filter_val",       wal_hipriv_set_rx_filter_val},       /* 设置接收帧过滤各状态下的配置值:hipriv "Hisilicon0 set_rx_filter_val 0-Normal/1-Repeater mode status value" */
    {"get_rx_filter_val",       wal_hipriv_get_rx_filter_val},       /* 设置接收帧过滤各状态下的配置值:hipriv "Hisilicon0 get_rx_filter_val 0-Normal/1-Repeater mode status" */
    {"set_rx_filter_en",        wal_hipriv_set_rx_filter_en},        /* 读取接收帧过滤各状态下的配置值:hipriv "Hisilicon0 set_rx_filter_en 0-打开/1-关闭 */
    {"get_rx_filter_en",        wal_hipriv_get_rx_filter_en},        /* 读取接收帧过滤各状态下的配置值:hipriv "Hisilicon0 get_rx_filter_en */
    {"get_all_regs",            wal_hipriv_get_all_reg_value},               /* 获取所有寄存器的值: hipriv "Hisilicon0 get_all_regs" */
#endif

    {"set_mib",                 wal_hipriv_set_mib},                   /* 设置VAP mib值 */
    {"get_mib",                 wal_hipriv_get_mib},                   /* 获取VAP mib值 */
    {"thruput_bypass",          wal_hipriv_set_thruput_bypass},        /* 设置thruput bypass维测点 */
    {"auto_protection",         wal_hipriv_set_auto_protection},       /* 设置自动保护开关 */

    /* 共存维测相关 */
    {"send_2040_coext",         wal_hipriv_send_2040_coext},           /* 发送20/40共存管理帧: hipriv "Hisilicon0 send_2040_coext coext_info chan_report" */
    {"2040_coext_info",         wal_hipriv_2040_coext_info},           /* 打印vap的所有20/40共存参数信息: hipriv "vap0 2040_coext_info" */
    {"get_version",             wal_hipriv_get_version},               /* 获取软件版本: hipriv "vap0 get_version" */

#ifdef _PRE_WLAN_FEATURE_DAQ
    {"data_acq",                wal_hipriv_data_acq},                  /* 获取软件版本: hipriv "Hisilicon0 data_acq 0/1/2/3/4 (length num depth) (channel mode data_th bit) (2) () ()" */
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    {"set_oma",                 wal_hipriv_set_oma},                   /* 设置Proxy STA的oma地址" */
    {"proxysta_switch",         wal_hipriv_proxysta_switch},           /* proxysta模块的开关的命令: hipriv "Hisilicon0 proxysta_switch 0 | 1"，该命令针对所有的VAP */
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    {"set_opmode_notify",       wal_hipriv_set_opmode_notify},         /* 设置VAP工作模式通知: hipriv "vap0 set_opmode_notify 0/1"  0-不支持; 1-支持 */
    {"get_user_nssbw",          wal_hipriv_get_user_nssbw},                   /* 设置添加用户的配置命令: hipriv "vap0 get_user_nssbw xx xx xx xx xx xx(mac地址) "  该命令针对某一个VAP */
#endif

#ifdef _PRE_WLAN_DFT_REG
    {"dump_reg",                 wal_hipriv_dump_reg},                 /* 设置需要读取的寄存器，hipriv "Hisilicon0 dump_reg phy/mac/soc/abb/rf (subtype-数字) (flag 0/1)"*/
    {"dump_reg_evt",             wal_hipriv_dump_reg_evt},             /* 设置触发寄存器读取的事件和事件跳数，hipriv "Hisilicon0 dump_reg_evt tx/rx/tbtt/prd (tick-数字)"*/
    {"dump_reg_addr",            wal_hipriv_dump_reg_addr},            /* 显示寄存器读取相关调试信息 */

    {"dump_reg_info",            wal_hipriv_dump_reg_info},            /* 显示寄存器读取相关调试信息 */
#endif

    {"set_vap_nss",              wal_hipriv_set_vap_nss},               /* 设置VAP的空间流个数:hipriv "vap0 set_vap_nss <value>" */

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

    {"blacklist_add",           wal_hipriv_blacklist_add},          /* 1 */
    {"blacklist_del",           wal_hipriv_blacklist_del},          /* 2 */
    {"blacklist_mode",          wal_hipriv_set_blacklist_mode},     /* 3 */
    {"blacklist_show",          wal_hipriv_blacklist_show},         /* 4 wal_config_blacklist_show */
    {"abl_on",                  wal_hipriv_set_abl_on},             /* 5 */
    {"abl_aging",               wal_hipriv_set_abl_aging},          /* 6 */
    {"abl_threshold",           wal_hipriv_set_abl_threshold},      /* 7 */
    {"abl_reset",               wal_hipriv_set_abl_reset},          /* 8 wal_config_set_autoblacklist_reset_time */
    {"isolation_mode",          wal_hipriv_set_isolation_mode},     /* 9 */
    {"isolation_type",          wal_hipriv_set_isolation_type},     /* 10 */
    {"isolation_fwd",           wal_hipriv_set_isolation_fwd},      /* 11 */
    {"isolation_clear",         wal_hipriv_set_isolation_clear},    /* 12 wal_config_set_isolation_clear */
    {"isolation_show",          wal_hipriv_set_isolation_show},     /* 13 wal_config_isolation_show */

#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
    {"m2u_snoop_on",            wal_hipriv_m2u_snoop_on},           /* 开启或关闭snoop开关功能 hipriv "vap0 m2u_snoop_on 0\1" */
    {"m2u_add_deny_table",      wal_hipriv_m2u_add_deny_table},         /* 增加组播组黑名单 hipriv "vap0 m2u_add_deny_table 224.1.1.1" */
    {"m2u_cfg_deny_table",      wal_hipriv_m2u_cfg_deny_table},         /* 增加组播组黑名单 hipriv "vap0 m2u_cfg_deny_table 1 0" */
    {"m2u_prt_sn_table",        wal_hipriv_m2u_show_snoop_table},       /* 打印组播组 hipriv "vap0 m2u_show_snoop_table 1" */
    {"m2u_igmp_pkt_xmit",       wal_hipriv_igmp_packet_xmit},           /* 向目标STA/AP发送数据帧: hipriv "vap0 m2u_igmp_pkt_xmit (tid_no) (报文个数) (报文长度) (RA MAC)" */
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    {"proxyarp_on",            wal_hipriv_proxyarp_on},           /* ?????proxyarp???? hipriv "vap0 proxyarp_on 0\1" */
#ifdef _PRE_DEBUG_MODE
    {"proxyarp_info",          wal_hipriv_proxyarp_info},         /* ??proxyarp???? hipriv "vap0 proxyarp_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif/* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
    {"smps_info",              wal_hipriv_get_smps_info},         /* ??proxyarp???? hipriv "vap0 smps_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif/* #ifdef _PRE_WLAN_FEATURE_SMPS */

    {"vap_classify_en",         wal_hipriv_vap_classify_en},        /* device级别配置命令 设置基于vap的业务分类是否使能 hipriv "Hisilicon0 vap_classify_en 0/1" */
    {"vap_classify_tid",        wal_hipriv_vap_classify_tid},       /* 设置vap的流等级 hipriv "vap0 classify_tid 0~7" */

#ifdef _PRE_DEBUG_MODE
    {"scan_test",               wal_hipriv_scan_test},              /* 扫描模块测试命令 hipriv "Hisilicon0 scan_test param1 param2" param1取值'2g' '5g' 'all' 1~14, 36~196; param2取值对应wlan_channel_bandwidth_enum_uint8 */
#endif
#ifdef _PRE_WLAN_PROFLING_MIPS
    {"set_mips",             wal_hipriv_set_mips},            /* 设置某流程的MIPS统计开关，sh hipriv.sh "Hisilicon0 set_mips wal_mips_param_enum 0|1" */
    {"show_mips",            wal_hipriv_show_mips},           /* 打印某流程的MIPS统计结果，sh hipriv.sh "Hisilicon0 show_mips wal_mips_param_enum" */
#endif
#if  (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
    {"freq",            wal_ioctl_set_freq},                    /* 设置AP 信道 */
    {"mode",            wal_ioctl_set_mode},                    /* 设置AP 协议模式 */
    {"essid",           wal_ioctl_set_essid},                   /* 设置AP ssid */
    {"txpower",         wal_ioctl_set_txpower},
    {"bintval",         wal_ioctl_set_beacon_interval},         /* 设置AP beacon 周期 */
    {"up",              wal_hipriv_start_vap},
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44)) */
    {"setcountry",              wal_hipriv_setcountry},            /*设置国家码命令 hipriv "Hisilicon0 setcountry param"param取值为大写的国家码字，例如 CN US*/
    {"getcountry",              wal_hipriv_getcountry},            /*查询国家码命令 hipriv "Hisilicon0 getcountry"*/
#ifdef _PRE_WLAN_FEATURE_11D
    {"set_rd_by_ie_switch",      wal_hipriv_set_rd_by_ie_switch},   /*设置是否根据关联ap更新国家码信息 hipriv "Hisilicon0 set_rd_by_ie_switch 0/1"*/
#endif
#ifdef  _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
    {"p2p_ps",                  wal_hipriv_set_p2p_ps},         /* 设置P2P 节能 sh hipriv.sh "vap0 p2p_ps noa/ops params */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
                                                                /* sh hipriv.sh "vap0 p2p_ps ops 0/1(0不使能，1使能) [0~255] 设置OPS 节能下ct_window 参数 */
                                                                /* sh hipriv.sh "vap0 p2p_ps noa start_time duration interval count 设置NOA 节能参数 */
                                                                /* sh hipriv.sh "vap0 p2p_ps statistics 0/1(0 清空统计，1查看统计) P2P 中断统计 */
    {"p2p_test",                wal_hipriv_p2p_test},
#endif  /* _PRE_WLAN_FEATURE_P2P */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"resume_rx_intr_fifo",     wal_hipriv_resume_rx_intr_fifo},    /* 使能恢复rx intr fifo命令，默认不是能 hipriv "Hisilicon0 resume_rxintr_fifo 0|1" 1使能 */
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"ampdu_mmss",              wal_hipriv_set_ampdu_mmss},         /* 设置AMPDU MMSS : sh hipriv.sh "vap0 ampdu_mmss 0~7" */
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    {"arp_offload_enable",      wal_hipriv_arp_offload_enable},    /* ARP/ND处理下移和广播/组播过滤开关:sh hipriv.sh "wlan0 arp_offload_enable  0/1(0关闭，1打开)" */
    {"show_arpoffload_info",    wal_hipriv_show_arpoffload_info},          /* 显示Device侧记录的IP地址:sh hipriv.sh "wlan0 show_ip_addr" */
#endif

#ifdef _PRE_WLAN_TCP_OPT
    {"get_tcp_ack_stream_info",                  wal_hipriv_get_tcp_ack_stream_info},         /* 显示TCP ACK 过滤统计值 sh hipriv.sh "vap0 get_tx_ack_stream_info*/
    {"tcp_tx_ack_opt_enable",                  wal_hipriv_tcp_tx_ack_opt_enable},         /*设置发送TCP ACK优化使能  sh hipriv.sh "vap0 tcp_tx_ack_opt_enable 0 | 1*/
    {"tcp_rx_ack_opt_enable",                  wal_hipriv_tcp_rx_ack_opt_enable},         /* 设置接收TCP ACK优化使能 sh hipriv.sh "vap0 tcp_rx_ack_opt_enable 0 | 1*/
    {"tcp_tx_ack_opt_limit",                  wal_hipriv_tcp_tx_ack_limit},         /* 设置发送TCP ACK LIMIT sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X*/
    {"tcp_rx_ack_opt_limit",                  wal_hipriv_tcp_rx_ack_limit},         /* 设置接收TCP ACKLIMIT  sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X*/

#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
    {"wapi_info",                             wal_hipriv_show_wapi_info},   /*wapi hipriv "vap0 wal_hipriv_show_wapi_info "*/
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


#ifdef _PRE_WLAN_DFT_STAT
    {"performance_log_debug",          wal_hipriv_performance_log_switch},        /* 设置性能打印控制开关 sh hipriv.sh "wlan0 performance_log_debug X Y,*/
                                                                                                                                                        /*其中X是打印点，见oal_performance_log_switch_enum定义，Y是使能开关,0关闭，1打开。*/                                                                                                                                                        /*X=255时，配置所有的打印开关*/
                                                                                                                                                        /*使用说明:												   		*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 0 0 :关闭聚合打印	*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 0 1 :打开聚合打印 	*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 1 0 :打印性能统计	*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 1 1 :聚合统计清0	*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 255 0 :清除所有控制开关*/
                                                                                                                                                        /*sh hipriv.sh "wlan0 performance_log_debug 255 1 :设置所有控制开关*/

#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    {"roam_enable",      wal_hipriv_roam_enable},   /* 设置漫游开关 */
    {"roam_org",         wal_hipriv_roam_org},      /* 设置漫游正交 */
    {"roam_band",        wal_hipriv_roam_band},     /* 设置漫游频段 */
    {"roam_start",       wal_hipriv_roam_start},    /* 漫游测试命令 */
    {"roam_info",        wal_hipriv_roam_info},     /* 漫游信息打印 */
#endif  //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    {"2040bss_enable",   wal_hipriv_enable_2040bss}, /* 设置20/40 bss使能: hipriv "Hisilicon0 2040bss_enable 0|1" 0表示20/40 bss判断关闭，1表示使能 */
#endif
};


/*****************************************************************************
  net_device上挂接的net_device_ops函数
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_net_device_ops_stru g_st_wal_net_dev_ops =
{
    .ndo_get_stats          = wal_netdev_get_stats,
    .ndo_open               = wal_netdev_open,
    .ndo_stop               = wal_netdev_stop,
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    .ndo_start_xmit         = wal_vap_start_xmit,
#else
    .ndo_start_xmit         = wal_bridge_vap_xmit,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
            /* TBD */
#else
            .ndo_set_multicast_list = OAL_PTR_NULL,
#endif

    .ndo_do_ioctl           = wal_net_device_ioctl,
    .ndo_change_mtu         = oal_net_device_change_mtu,
    .ndo_init               = oal_net_device_init,

#if (defined(_PRE_WLAN_FEATURE_FLOWCTL) || defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL))
    .ndo_select_queue       = wal_netdev_select_queue,
#endif

    .ndo_set_mac_address    = wal_netdev_set_mac_addr
};
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_net_device_ops_stru g_st_wal_net_dev_ops =
{
    oal_net_device_init,
    wal_netdev_open,
    wal_netdev_stop,
    wal_bridge_vap_xmit,
    OAL_PTR_NULL,
    wal_netdev_get_stats,
    wal_net_device_ioctl,
    oal_net_device_change_mtu,
    wal_netdev_set_mac_addr
};
#endif

/*****************************************************************************
  标准ioctl命令函数表.
*****************************************************************************/
OAL_STATIC OAL_CONST oal_iw_handler g_ast_iw_handlers[] =
{
    OAL_PTR_NULL,                               /* SIOCSIWCOMMIT, */
    (oal_iw_handler)wal_ioctl_get_iwname,       /* SIOCGIWNAME, */
    OAL_PTR_NULL,                               /* SIOCSIWNWID, */
    OAL_PTR_NULL,                               /* SIOCGIWNWID, */
    (oal_iw_handler)wal_ioctl_set_freq,         /* SIOCSIWFREQ, 设置频点/信道 */
    (oal_iw_handler)wal_ioctl_get_freq,         /* SIOCGIWFREQ, 获取频点/信道 */
    (oal_iw_handler)wal_ioctl_set_bss_type,     /* SIOCSIWMODE, 设置bss type */
    (oal_iw_handler)wal_ioctl_get_bss_type,     /* SIOCGIWMODE, 获取bss type */
    OAL_PTR_NULL,                               /* SIOCSIWSENS, */
    (oal_iw_handler)wal_ioctl_get_iwsense,      /* SIOCGIWSENS, */
    OAL_PTR_NULL,                               /* SIOCSIWRANGE, */ /* not used */
    (oal_iw_handler)wal_ioctl_get_iwrange,      /* SIOCGIWRANGE, */
    OAL_PTR_NULL,                               /* SIOCSIWPRIV, */  /* not used */
    OAL_PTR_NULL,                               /* SIOCGIWPRIV, */  /* kernel code */
    OAL_PTR_NULL,                               /* SIOCSIWSTATS, */ /* not used */
    OAL_PTR_NULL,                               /* SIOCGIWSTATS, */
    OAL_PTR_NULL,                               /* SIOCSIWSPY, */
    OAL_PTR_NULL,                               /* SIOCGIWSPY, */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* SIOCSIWAP, */
    (oal_iw_handler)wal_ioctl_get_apaddr,       /* SIOCGIWAP, */
    OAL_PTR_NULL,                               /* SIOCSIWMLME, */
    OAL_PTR_NULL,                               /* SIOCGIWAPLIST, */
    OAL_PTR_NULL,                               /* SIOCSIWSCAN, */
    OAL_PTR_NULL,                               /* SIOCGIWSCAN, */
    (oal_iw_handler)wal_ioctl_set_essid,        /* SIOCSIWESSID, 设置ssid */
    (oal_iw_handler)wal_ioctl_get_essid,        /* SIOCGIWESSID, 读取ssid */
    OAL_PTR_NULL,                               /* SIOCSIWNICKN */
    OAL_PTR_NULL,                               /* SIOCGIWNICKN */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* SIOCSIWRATE */
    (oal_iw_handler)wal_ioctl_get_iwrate,       /* SIOCGIWRATE */
    OAL_PTR_NULL,                               /* SIOCSIWRTS */
    (oal_iw_handler)wal_ioctl_get_rtsthres,     /* SIOCGIWRTS */
    OAL_PTR_NULL,                               /* SIOCSIWFRAG */
    (oal_iw_handler)wal_ioctl_get_fragthres,    /* SIOCGIWFRAG */
    (oal_iw_handler)wal_ioctl_set_txpower,      /* SIOCSIWTXPOW, 设置传输功率限制 */
    (oal_iw_handler)wal_ioctl_get_txpower,      /* SIOCGIWTXPOW, 设置传输功率限制 */
    OAL_PTR_NULL,                               /* SIOCSIWRETRY */
    OAL_PTR_NULL,                               /* SIOCGIWRETRY */
    OAL_PTR_NULL,                               /* SIOCSIWENCODE */
    (oal_iw_handler)wal_ioctl_get_iwencode,     /* SIOCGIWENCODE */
    OAL_PTR_NULL,                               /* SIOCSIWPOWER */
    OAL_PTR_NULL,                               /* SIOCGIWPOWER */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* -- hole -- */
    OAL_PTR_NULL,                               /* SIOCSIWGENIE */
    OAL_PTR_NULL,                               /* SIOCGIWGENIE */
    OAL_PTR_NULL,                               /* SIOCSIWAUTH */
    OAL_PTR_NULL,                               /* SIOCGIWAUTH */
    OAL_PTR_NULL,                               /* SIOCSIWENCODEEXT */
    OAL_PTR_NULL                                /* SIOCGIWENCODEEXT */
};

/*****************************************************************************
  私有ioctl命令参数定义定义
*****************************************************************************/
OAL_STATIC OAL_CONST oal_iw_priv_args_stru g_ast_iw_priv_args[] =
{
    {WAL_IOCTL_PRIV_SET_MODE,       OAL_IW_PRIV_TYPE_CHAR | 24, 0, "mode"},     /* 参数类型是char, 个数为24 */
    {WAL_IOCTL_PRIV_GET_MODE,       0, OAL_IW_PRIV_TYPE_CHAR | 24, "get_mode"},
    {WAL_IOCTL_PRIV_SET_COUNTRY,    OAL_IW_PRIV_TYPE_CHAR | 3, 0,  "setcountry"},
    {WAL_IOCTL_PRIV_GET_COUNTRY,    0, OAL_IW_PRIV_TYPE_CHAR | 3,  "getcountry"},

    {WAL_IOCTL_PRIV_SET_AP_CFG, OAL_IW_PRIV_TYPE_CHAR |  256, 0,"AP_SET_CFG"},
    {WAL_IOCTL_PRIV_AP_MAC_FLTR, OAL_IW_PRIV_TYPE_CHAR | 256, OAL_IW_PRIV_TYPE_CHAR | OAL_IW_PRIV_SIZE_FIXED | 0, "AP_SET_MAC_FLTR"},
    {WAL_IOCTL_PRIV_AP_GET_STA_LIST, 0, OAL_IW_PRIV_TYPE_CHAR | 1024, "AP_GET_STA_LIST"},
    {WAL_IOCTL_PRIV_AP_STA_DISASSOC, OAL_IW_PRIV_TYPE_CHAR | 256, OAL_IW_PRIV_TYPE_CHAR | 0, "AP_STA_DISASSOC"},

    /* sub-ioctl函数入口 */
    {WAL_IOCTL_PRIV_SETPARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "setparam"},
    {WAL_IOCTL_PRIV_GETPARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1,
                                    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "getparam"},

    /* sub-ioctl标志，name为'\0', 数字1表示set命令后面跟1个参数, get命令得到1个值 */
    {WAL_IOCTL_PRIV_SETPARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, ""},
    {WAL_IOCTL_PRIV_SETPARAM,       OAL_IW_PRIV_TYPE_BYTE | OAL_IW_PRIV_SIZE_FIXED | OAL_IW_PRIV_TYPE_ADDR, 0, ""},
    {WAL_IOCTL_PRIV_GETPARAM,       0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "" },
    {WAL_IOCTL_PRIV_GETPARAM,       0, OAL_IW_PRIV_TYPE_BYTE | OAL_IW_PRIV_SIZE_FIXED | OAL_IW_PRIV_TYPE_ADDR, ""},
    {WLAN_CFGID_SHORTGI,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "shortgi20"},
    {WLAN_CFGID_SHORTGI,            0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_shortgi20"},
    {WLAN_CFGID_SHORTGI_FORTY,      OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "shortgi40"},
    {WLAN_CFGID_SHORTGI_FORTY,      0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_shortgi40"},
    {WLAN_CFGID_SHORTGI_EIGHTY,     OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "shortgi80"},
    {WLAN_CFGID_SHORTGI_EIGHTY,     0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_shortgi80"},

    {WLAN_CFGID_SHORT_PREAMBLE,     OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "shpreamble"},
    {WLAN_CFGID_SHORT_PREAMBLE,     0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_shpreamble"},
#ifdef _PRE_WLAN_FEATURE_MONITOR
    {WLAN_CFGID_ADDR_FILTER,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "addr_filter"},
    {WLAN_CFGID_ADDR_FILTER,        0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_addr_filter"},
#endif
    {WLAN_CFGID_PROT_MODE,          OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "protmode"},
    {WLAN_CFGID_PROT_MODE,          0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_protmode"},
    {WLAN_CFGID_AUTH_MODE,          OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "authmode"},
    {WLAN_CFGID_AUTH_MODE,          0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_authmode"},
    {WLAN_CFGID_BEACON_INTERVAL,    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "bintval"},
    {WLAN_CFGID_BEACON_INTERVAL,    0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_bintval"},
    {WLAN_CFGID_NO_BEACON,          OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "nobeacon"},
    {WLAN_CFGID_NO_BEACON,          0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_nobeacon"},
    {WLAN_CFGID_TX_CHAIN,           OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "txchainmask"},
    {WLAN_CFGID_TX_CHAIN,           0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_txchainmask"},
    {WLAN_CFGID_RX_CHAIN,           OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "rxchainmask"},
    {WLAN_CFGID_RX_CHAIN,           0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_rxchainmask"},
    {WLAN_CFGID_CONCURRENT,         OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "concurrent"},
    {WLAN_CFGID_CONCURRENT,         0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_concurrent"},
    {WLAN_CFGID_TID,                0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_tid"},

#ifdef _PRE_WLAN_FEATURE_UAPSD
    /*U-APSD命令*/
    {WLAN_CFGID_UAPSD_EN ,          OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "uapsden"},
    {WLAN_CFGID_UAPSD_EN,           0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_uapsden"},
#endif
    {WLAN_CFGID_DTIM_PERIOD,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "dtim_period"},
    {WLAN_CFGID_DTIM_PERIOD,        0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_dtim_period"},

    /* EDCA参数配置命令 sub-ioctl入口 */
    {WAL_IOCTL_PRIV_SET_WMM_PARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 3, 0, "setwmmparam"},
    {WAL_IOCTL_PRIV_GET_WMM_PARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2,
                                         OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, "getwmmparam"},

    /* sub-ioctl标志，name为'\0', 2表示set命令后跟两个参数 */
    {WAL_IOCTL_PRIV_SET_WMM_PARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, ""},
    {WAL_IOCTL_PRIV_GET_WMM_PARAM,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1,
                                         OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "" },
    {WLAN_CFGID_EDCA_TABLE_CWMIN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "cwmin"},
    {WLAN_CFGID_EDCA_TABLE_CWMIN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_cwmin"},
    {WLAN_CFGID_EDCA_TABLE_CWMAX,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "cwmax"},
    {WLAN_CFGID_EDCA_TABLE_CWMAX,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_cwmax"},
    {WLAN_CFGID_EDCA_TABLE_AIFSN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "aifsn"},
    {WLAN_CFGID_EDCA_TABLE_AIFSN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_aifsn"},
    {WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "txoplimit"},
    {WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_txoplimit"},
    {WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME,    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "lifetime"},
    {WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME,    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_lifetime"},
    {WLAN_CFGID_EDCA_TABLE_MANDATORY,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "mandatory"},
    {WLAN_CFGID_EDCA_TABLE_MANDATORY,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_mandatory"},

    {WLAN_CFGID_QEDCA_TABLE_CWMIN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qcwmin"},
    {WLAN_CFGID_QEDCA_TABLE_CWMIN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qcwmin"},
    {WLAN_CFGID_QEDCA_TABLE_CWMAX,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qcwmax"},
    {WLAN_CFGID_QEDCA_TABLE_CWMAX,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qcwmax"},
    {WLAN_CFGID_QEDCA_TABLE_AIFSN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qaifsn"},
    {WLAN_CFGID_QEDCA_TABLE_AIFSN,            OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qaifsn"},
    {WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qtxoplimit"},
    {WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT,       OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qtxoplimit"},
    {WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME,    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qlifetime"},
    {WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME,    OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qlifetime"},
    {WLAN_CFGID_QEDCA_TABLE_MANDATORY,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "qmandatory"},
    {WLAN_CFGID_QEDCA_TABLE_MANDATORY,        OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_qmandatory"},
#ifdef _PRE_WLAN_FEATURE_SMPS
    {WLAN_CFGID_SMPS_MODE,          OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 2, 0, "smps_mode"},
    {WLAN_CFGID_SMPS_MODE,          0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_smps_mode"},
    {WLAN_CFGID_SMPS_EN,            0, OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, "get_smps_en"},
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    {WLAN_CFGID_PROXYARP_EN,   OAL_IW_PRIV_TYPE_INT | OAL_IW_PRIV_SIZE_FIXED | 1, 0, "proxyarp_en"}, /* 使能proxy arp */
#endif

};

/*****************************************************************************
  私有ioctl命令函数表.
*****************************************************************************/
OAL_STATIC OAL_CONST oal_iw_handler g_ast_iw_priv_handlers[] =
{
    (oal_iw_handler)wal_ioctl_set_param,                /* SIOCWFIRSTPRIV+0 */  /* sub-ioctl set 入口 */
    (oal_iw_handler)wal_ioctl_get_param,                /* SIOCWFIRSTPRIV+1 */  /* sub-ioctl get 入口 */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+2 */  /* setkey */
    (oal_iw_handler)wal_ioctl_set_wme_params,           /* SIOCWFIRSTPRIV+3 */  /* setwmmparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+4 */  /* delkey */
    (oal_iw_handler)wal_ioctl_get_wme_params,           /* SIOCWFIRSTPRIV+5 */  /* getwmmparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+6 */  /* setmlme */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+7 */  /* getchaninfo */
    (oal_iw_handler)wal_ioctl_setcountry,               /* SIOCWFIRSTPRIV+8 */  /* setcountry */
    (oal_iw_handler)wal_ioctl_getcountry,               /* SIOCWFIRSTPRIV+9 */  /* getcountry */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+10 */  /* addmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+11 */  /* getscanresults */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+12 */  /* delmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+13 */  /* getchanlist */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+14 */  /* setchanlist */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+15 */  /* kickmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+16 */  /* chanswitch */
    (oal_iw_handler)wal_ioctl_get_mode,                 /* SIOCWFIRSTPRIV+17 */  /* 获取模式, 例: iwpriv vapN get_mode */
    (oal_iw_handler)wal_ioctl_set_mode,                 /* SIOCWFIRSTPRIV+18 */  /* 设置模式, 例: iwpriv vapN mode 11g */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+19 */  /* getappiebuf */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+20 */  /* null */
    (oal_iw_handler)wal_ioctl_get_assoc_list,           /* SIOCWFIRSTPRIV+21 */  /* APUT取得关联STA列表 */
    (oal_iw_handler)wal_ioctl_set_mac_filters,          /* SIOCWFIRSTPRIV+22 */  /* APUT设置STA过滤 */
    (oal_iw_handler)wal_ioctl_set_ap_config,            /* SIOCWFIRSTPRIV+23 */  /* 设置APUT参数 */
    (oal_iw_handler)wal_ioctl_set_ap_sta_disassoc,      /* SIOCWFIRSTPRIV+24 */  /* APUT去关联STA */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+25 */  /* getStatistics */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+26 */  /* sendmgmt */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+27 */  /* null  */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+28 */  /* null */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+29 */  /* getaclmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+30 */  /* sethbrparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+29 */  /* getaclmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+30 */  /* sethbrparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+31 */  /* setrxtimeout */
};

/*****************************************************************************
  无线配置iw_handler_def定义
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_iw_handler_def_stru g_st_iw_handler_def =
{
    .standard           = g_ast_iw_handlers,
    .num_standard       = OAL_ARRAY_SIZE(g_ast_iw_handlers),
    .private            = g_ast_iw_priv_handlers,
    .num_private        = OAL_ARRAY_SIZE(g_ast_iw_priv_handlers),
    .private_args       = g_ast_iw_priv_args,
    .num_private_args   = OAL_ARRAY_SIZE(g_ast_iw_priv_args),
    .get_wireless_stats = OAL_PTR_NULL
};

#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_iw_handler_def_stru g_st_iw_handler_def =
{
    g_ast_iw_handlers,                      /* 标准ioctl handler */
    OAL_ARRAY_SIZE(g_ast_iw_handlers),
    OAL_ARRAY_SIZE(g_ast_iw_priv_handlers),
    {0, 0},                                 /* 字节对齐 */
    OAL_ARRAY_SIZE(g_ast_iw_priv_args),
    g_ast_iw_priv_handlers,                 /* 私有ioctl handler */
    g_ast_iw_priv_args,
    OAL_PTR_NULL
};
#endif

/*****************************************************************************
  协议模式字符串定义
*****************************************************************************/
OAL_CONST wal_ioctl_mode_map_stru g_ast_mode_map[] =
{
    /* legacy */
    {"11a",         WLAN_LEGACY_11A_MODE,       WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11b",         WLAN_LEGACY_11B_MODE,       WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11bg",        WLAN_MIXED_ONE_11G_MODE,    WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11g",         WLAN_MIXED_TWO_11G_MODE,    WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},

    /* 11n */
    {"11na20",      WLAN_HT_MODE,           WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11ng20",      WLAN_HT_MODE,           WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11na40plus",  WLAN_HT_MODE,           WLAN_BAND_5G,   WLAN_BAND_WIDTH_40PLUS},
    {"11na40minus", WLAN_HT_MODE,           WLAN_BAND_5G,   WLAN_BAND_WIDTH_40MINUS},
    {"11ng40plus",  WLAN_HT_MODE,           WLAN_BAND_2G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ng40minus", WLAN_HT_MODE,           WLAN_BAND_2G,   WLAN_BAND_WIDTH_40MINUS},

    /* 11ac */
    {"11ac20",              WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11ac40plus",          WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ac40minus",         WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_40MINUS},
    {"11ac80plusplus",      WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {"11ac80plusminus",     WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {"11ac80minusplus",     WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {"11ac80minusminus",    WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80MINUSMINUS},

    {"11ac2g20",            WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11ac2g40plus",        WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ac2g40minus",       WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_40MINUS},
    /* 11n only and 11ac only, 都是20M带宽 */
    {"11nonly2g",           WLAN_HT_ONLY_MODE,   WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11nonly5g",           WLAN_HT_ONLY_MODE,   WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11aconly",            WLAN_VHT_ONLY_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {OAL_PTR_NULL}
};

/* 注意! 这里的参数定义需要与 g_dmac_config_set_dscr_param中的函数顺序严格一致! */
OAL_STATIC oal_int8   *pauc_tx_dscr_param_name[WAL_DSCR_PARAM_BUTT] =
{
    "fbm",
    "pgl",
    "mtpgl",
    "sae",
    "ta",
    "ra",
    "cc",
    "data0",
    "data1",
    "data2",
    "data3",
    "power",
    "shortgi",
    "preamble",
    "rtscts",
    "lsigtxop",
    "smooth",
    "snding",
    "txbf",
    "stbc",
    "rd_ess",
    "dyn_bw",
    "dyn_bw_exist",
    "ch_bw_exist"
};

//#ifdef    _PRE_WLAN_CHIP_TEST
OAL_STATIC OAL_CONST oal_int8   *pauc_bw_tbl[WLAN_BAND_ASSEMBLE_AUTO] =
{
    "20",
    "rsv1",
    "rsv2",
    "rsv3",
    "40",
    "d40",
    "rsv6",
    "rsv7",
    "80",
    "d80",
    "rsv10",
    "rsv11",
    "160",
    "d160",
    "rsv14",
    "80_80"
};

OAL_STATIC OAL_CONST oal_int8   *pauc_non_ht_rate_tbl[WLAN_LEGACY_RATE_VALUE_BUTT] =
{
    "1",
    "2",
    "5.5",
    "11",
    "rsv0",
    "rsv1",
    "rsv2",
    "rsv3",
    "48",
    "24",
    "12",
    "6",
    "54",
    "36",
    "18",
    "9"
};
//#endif  /* _PRE_WLAN_CHIP_TEST */

OAL_STATIC OAL_CONST wal_ioctl_alg_cfg_stru g_ast_alg_cfg_map[] =
{
    {"sch_vi_ctrl_ena",         MAC_ALG_CFG_SCHEDULE_VI_CTRL_ENA},
    {"sch_bebk_minbw_ena",      MAC_ALG_CFG_SCHEDULE_BEBK_MIN_BW_ENA},
    {"sch_mvap_sch_ena",        MAC_ALG_CFG_SCHEDULE_MVAP_SCH_ENA},
    {"sch_vi_sch_ms",           MAC_ALG_CFG_SCHEDULE_VI_SCH_LIMIT},
    {"sch_vo_sch_ms",           MAC_ALG_CFG_SCHEDULE_VO_SCH_LIMIT},
    {"sch_vi_drop_ms",          MAC_ALG_CFG_SCHEDULE_VI_DROP_LIMIT},
    {"sch_vi_ctrl_ms",          MAC_ALG_CFG_SCHEDULE_VI_CTRL_MS},
    {"sch_vi_life_ms",          MAC_ALG_CFG_SCHEDULE_VI_MSDU_LIFE_MS},
    {"sch_vo_life_ms",          MAC_ALG_CFG_SCHEDULE_VO_MSDU_LIFE_MS},
    {"sch_be_life_ms",          MAC_ALG_CFG_SCHEDULE_BE_MSDU_LIFE_MS},
    {"sch_bk_life_ms",          MAC_ALG_CFG_SCHEDULE_BK_MSDU_LIFE_MS},
    {"sch_vi_low_delay",        MAC_ALG_CFG_SCHEDULE_VI_LOW_DELAY_MS},
    {"sch_vi_high_delay",       MAC_ALG_CFG_SCHEDULE_VI_HIGH_DELAY_MS},
    {"sch_cycle_ms",            MAC_ALG_CFG_SCHEDULE_SCH_CYCLE_MS},
    {"sch_ctrl_cycle_ms",       MAC_ALG_CFG_SCHEDULE_TRAFFIC_CTRL_CYCLE},
    {"sch_cir_nvip_kbps",       MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS},
    {"sch_cir_nvip_be",         MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS_BE},
    {"sch_cir_nvip_bk",         MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS_BK},
    {"sch_cir_vip_kbps",        MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS},
    {"sch_cir_vip_be",          MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS_BE},
    {"sch_cir_vip_bk",          MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS_BK},
    {"sch_cir_vap_kbps",        MAC_ALG_CFG_SCHEDULE_CIR_VAP_KBPS},
    {"sch_sm_delay_ms",         MAC_ALG_CFG_SCHEDULE_SM_TRAIN_DELAY},
    {"sch_drop_pkt_limit",      MAC_ALG_CFG_VIDEO_DROP_PKT_LIMIT},
    {"sch_flowctl_ena",         MAC_ALG_CFG_FLOWCTRL_ENABLE_FLAG},
    {"sch_log_start",           MAC_ALG_CFG_SCHEDULE_LOG_START},
    {"sch_log_end",             MAC_ALG_CFG_SCHEDULE_LOG_END},
    {"sch_vap_prio",            MAC_ALG_CFG_SCHEDULE_VAP_SCH_PRIO},

    {"txbf_switch",             MAC_ALG_CFG_TXBF_MASTER_SWITCH},
    {"txbf_txmode_enb",         MAC_ALG_CFG_TXBF_TXMODE_ENABLE},
    {"txbf_bfer_enb",           MAC_ALG_CFG_TXBF_TXBFER_ENABLE},
    {"txbf_bfee_enb",           MAC_ALG_CFG_TXBF_TXBFEE_ENABLE},
    {"txbf_txstbc_enb",         MAC_ALG_CFG_TXBF_TXSTBC_ENABLE},
    {"txbf_rxstbc_enb",         MAC_ALG_CFG_TXBF_RXSTBC_ENABLE},
    {"txbf_2g_bfer",            MAC_ALG_CFG_TXBF_2G_BFER_ENABLE},
    {"txbf_2nss_bfer",          MAC_ALG_CFG_TXBF_2NSS_BFER_ENABLE},
    {"txbf_fix_mode",           MAC_ALG_CFG_TXBF_FIX_MODE},
    {"txbf_fix_sound",          MAC_ALG_CFG_TXBF_FIX_SOUNDING},
    {"txbf_log_enb",            MAC_ALG_CFG_TXBF_LOG_ENABLE},
    {"txbf_log_sta",            MAC_ALG_CFG_TXBF_RECORD_LOG_START},
    {"txbf_log_out",            MAC_ALG_CFG_TXBF_LOG_OUTPUT},
    {"ar_enable",               MAC_ALG_CFG_AUTORATE_ENABLE},                   /* 开启或关闭速率自适应算法: sh hipriv.sh "vap0 alg_cfg ar_enable [1|0]" */
    {"ar_use_lowest",           MAC_ALG_CFG_AUTORATE_USE_LOWEST_RATE},          /* 开启或关闭使用最低速率: sh hipriv.sh "vap0 alg_cfg ar_use_lowest [1|0]" */
    {"ar_short_num",            MAC_ALG_CFG_AUTORATE_SHORT_STAT_NUM},           /* 设置短期统计的包数目:sh hipriv.sh "vap0 alg_cfg ar_short_num [包数目]" */
    {"ar_short_shift",          MAC_ALG_CFG_AUTORATE_SHORT_STAT_SHIFT},         /* 设置短期统计的包位移值:sh hipriv.sh "vap0 alg_cfg ar_short_shift [位移值]" */
    {"ar_long_num",             MAC_ALG_CFG_AUTORATE_LONG_STAT_NUM},            /* 设置长期统计的包数目:sh hipriv.sh "vap0 alg_cfg ar_long_num [包数目]" */
    {"ar_long_shift",           MAC_ALG_CFG_AUTORATE_LONG_STAT_SHIFT},          /* 设置长期统计的包位移值:sh hipriv.sh "vap0 alg_cfg ar_long_shift [位移值]" */
    {"ar_min_probe_no",         MAC_ALG_CFG_AUTORATE_MIN_PROBE_INTVL_PKTNUM},   /* 设置最小探测包间隔:sh hipriv.sh "vap0 alg_cfg ar_min_probe_no [包数目]" */
    {"ar_max_probe_no",         MAC_ALG_CFG_AUTORATE_MAX_PROBE_INTVL_PKTNUM},   /* 设置最大探测包间隔:sh hipriv.sh "vap0 alg_cfg ar_max_probe_no [包数目]" */
    {"ar_keep_times",           MAC_ALG_CFG_AUTORATE_PROBE_INTVL_KEEP_TIMES},   /* 设置探测间隔保持次数:sh hipriv.sh "vap0 alg_cfg ar_keep_times [次数]" */
    {"ar_delta_ratio",          MAC_ALG_CFG_AUTORATE_DELTA_GOODPUT_RATIO},      /* 设置goodput突变门限(千分比，如300):sh hipriv.sh "vap0 alg_cfg ar_delta_ratio [千分比]" */
    {"ar_vi_per_limit",         MAC_ALG_CFG_AUTORATE_VI_PROBE_PER_LIMIT},       /* 设置vi的per门限(千分比，如300):sh hipriv.sh "vap0 alg_cfg ar_vi_per_limit [千分比]" */
    {"ar_vo_per_limit",         MAC_ALG_CFG_AUTORATE_VO_PROBE_PER_LIMIT},       /* 设置vo的per门限(千分比，如300):sh hipriv.sh "vap0 alg_cfg ar_vo_per_limit [千分比]" */
    {"ar_ampdu_time",           MAC_ALG_CFG_AUTORATE_AMPDU_DURATION},           /* 设置ampdu的durattion值:sh hipriv.sh "vap0 alg_cfg ar_ampdu_time [时间值]" */
    {"ar_cont_loss_num",        MAC_ALG_CFG_AUTORATE_MCS0_CONT_LOSS_NUM},       /* 设置mcs0的传输失败门限:sh hipriv.sh "vap0 alg_cfg ar_cont_loss_num [包数目]" */
    {"ar_11b_diff_rssi",        MAC_ALG_CFG_AUTORATE_UP_PROTOCOL_DIFF_RSSI},    /* 设置升回11b的rssi门限:sh hipriv.sh "vap0 alg_cfg ar_11b_diff_rssi [数值]" */
    {"ar_rts_mode",             MAC_ALG_CFG_AUTORATE_RTS_MODE},                 /* 设置rts模式:sh hipriv.sh "vap0 alg_cfg ar_rts_mode [0(都不开)|1(都开)|2(rate[0]动态RTS, rate[1..3]都开RTS)|3(rate[0]不开RTS, rate[1..3]都开RTS)]" */
    {"ar_legacy_loss",          MAC_ALG_CFG_AUTORATE_LEGACY_1ST_LOSS_RATIO_TH}, /* 设置Legacy首包错误率门限:sh hipriv.sh "vap0 alg_cfg ar_legacy_loss [数值]" */
    {"ar_ht_vht_loss",          MAC_ALG_CFG_AUTORATE_HT_VHT_1ST_LOSS_RATIO_TH}, /* 设置Legacy首包错误率门限:sh hipriv.sh "vap0 alg_cfg ar_ht_vht_loss [数值]" */
    {"ar_stat_log_do",          MAC_ALG_CFG_AUTORATE_STAT_LOG_START},           /* 开始速率统计日志:sh hipriv.sh "vap0 alg_cfg ar_stat_log_do [mac地址] [业务类别] [包数目]" 如: sh hipriv.sh "vap0 alg_cfg ar_stat_log_do 06:31:04:E3:81:02 1 1000" */
    {"ar_sel_log_do",           MAC_ALG_CFG_AUTORATE_SELECTION_LOG_START},      /* 开始速率选择日志:sh hipriv.sh "vap0 alg_cfg ar_sel_log_do [mac地址] [业务类别] [包数目]" 如: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 200" */
    {"ar_fix_log_do",           MAC_ALG_CFG_AUTORATE_FIX_RATE_LOG_START},       /* 开始固定速率日志:sh hipriv.sh "vap0 alg_cfg ar_fix_log_do [mac地址] [tidno] [per门限]" 如: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 200" */
    {"ar_aggr_log_do",          MAC_ALG_CFG_AUTORATE_AGGR_STAT_LOG_START},      /* 开始聚合自适应日志:sh hipriv.sh "vap0 alg_cfg ar_fix_log_do [mac地址] [tidno]" 如: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 " */
    {"ar_st_log_out",           MAC_ALG_CFG_AUTORATE_STAT_LOG_WRITE},           /* 打印速率统计日志:sh hipriv.sh "vap0 alg_cfg ar_st_log_out 06:31:04:E3:81:02" */
    {"ar_sel_log_out",          MAC_ALG_CFG_AUTORATE_SELECTION_LOG_WRITE},      /* 打印速率选择日志:sh hipriv.sh "vap0 alg_cfg ar_sel_log_out 06:31:04:E3:81:02" */
    {"ar_fix_log_out",          MAC_ALG_CFG_AUTORATE_FIX_RATE_LOG_WRITE},       /* 打印固定速率日志:sh hipriv.sh "vap0 alg_cfg ar_fix_log_out 06:31:04:E3:81:02" */
    {"ar_aggr_log_out",         MAC_ALG_CFG_AUTORATE_AGGR_STAT_LOG_WRITE},      /* 打印固定速率日志:sh hipriv.sh "vap0 alg_cfg ar_fix_log_out 06:31:04:E3:81:02" */
    {"ar_disp_rateset",         MAC_ALG_CFG_AUTORATE_DISPLAY_RATE_SET},         /* 打印速率集合:sh hipriv.sh "vap0 alg_cfg ar_disp_rateset 06:31:04:E3:81:02" */
    {"ar_cfg_fix_rate",         MAC_ALG_CFG_AUTORATE_CONFIG_FIX_RATE},          /* 配置固定速率:sh hipriv.sh "vap0 alg_cfg ar_cfg_fix_rate 06:31:04:E3:81:02 0" */
    {"ar_disp_rx_rate",         MAC_ALG_CFG_AUTORATE_DISPLAY_RX_RATE},          /* 打印接收速率集合:sh hipriv.sh "vap0 alg_cfg ar_disp_rx_rate 06:31:04:E3:81:02" */
    {"ar_log_enable",           MAC_ALG_CFG_AUTORATE_LOG_ENABLE},               /* 开启或关闭速率自适应日志: sh hipriv.sh "vap0 alg_cfg ar_log_enable [1|0]" */
    {"ar_max_vo_rate",          MAC_ALG_CFG_AUTORATE_VO_RATE_LIMIT},            /* 设置最大的VO速率: sh hipriv.sh "vap0 alg_cfg ar_max_vo_rate [速率值]" */
    {"ar_fading_per_th",        MAC_ALG_CFG_AUTORATE_JUDGE_FADING_PER_TH},      /* 设置深衰弱的per门限值: sh hipriv.sh "vap0 alg_cfg ar_fading_per_th [per门限值(千分数)]"*/
    {"ar_aggr_opt",             MAC_ALG_CFG_AUTORATE_AGGR_OPT},                 /* 设置聚合自适应开关: sh hipriv.sh "vap0 alg_cfg ar_aggr_opt [1|0]"*/
    {"ar_aggr_pb_intvl",        MAC_ALG_CFG_AUTORATE_AGGR_PROBE_INTVL_NUM},     /* 设置聚合自适应探测间隔: sh hipriv.sh "vap0 alg_cfg ar_aggr_pb_intvl [探测间隔]"*/
    {"ar_aggr_st_shift",        MAC_ALG_CFG_AUTORATE_AGGR_STAT_SHIFT},          /* 设置聚合自适应统计移位值: sh hipriv.sh "vap0 alg_cfg ar_aggr_st_shift [统计移位值]"*/
    {"ar_dbac_aggrtime",        MAC_ALG_CFG_AUTORATE_DBAC_AGGR_TIME},           /* 设置DBAC模式下的最大聚合时间: sh hipriv.sh "vap0 alg_cfg ar_dbac_aggrtime [最大聚合时间(us)]"*/
    {"ar_dbg_vi_status",        MAC_ALG_CFG_AUTORATE_DBG_VI_STATUS},            /* 设置调试用的VI状态: sh hipriv.sh "vap0 alg_cfg ar_dbg_vi_status [0/1/2]"*/
    {"ar_dbg_aggr_log",         MAC_ALG_CFG_AUTORATE_DBG_AGGR_LOG},             /* 聚合自适应log开关: sh hipriv.sh "vap0 alg_cfg ar_dbg_aggr_log [0/1]"*/
    {"ar_aggr_pck_num",         MAC_ALG_CFG_AUTORATE_AGGR_NON_PROBE_PCK_NUM},   /* 最优速率变化时不进行聚合探测的报文数: sh hipriv.sh "vap0 alg_cfg ar_aggr_pck_num [报文数]"*/
    {"ar_min_aggr_idx",         MAC_ALG_CFG_AUTORATE_AGGR_MIN_AGGR_TIME_IDX},   /* 最小聚合时间索引: sh hipriv.sh "vap0 alg_cfg ar_aggr_min_idx [索引值]"*/
    {"ar_max_aggr_num",         MAC_ALG_CFG_AUTORATE_MAX_AGGR_NUM},             /* 设置最大聚合数目: sh hipriv.sh "vap0 alg_cfg ar_max_aggr_num [聚合数目]" */
    {"ar_1mpdu_per_th",         MAC_ALG_CFG_AUTORATE_LIMIT_1MPDU_PER_TH},       /* 设置最低阶MCS限制聚合为1的PER门限: sh hipriv.sh "vap0 alg_cfg ar_1mpdu_per_th [per门限值(千分数)]" */

    {"ar_btcoxe_probe",         MAC_ALG_CFG_AUTORATE_BTCOEX_PROBE_ENABLE},      /* 开启或关闭共存探测机制: sh hipriv.sh "vap0 alg_cfg ar_btcoxe_probe [1|0]" */
    {"ar_btcoxe_aggr",          MAC_ALG_CFG_AUTORATE_BTCOEX_AGGR_ENABLE},       /* 开启或关闭共存聚合机制: sh hipriv.sh "vap0 alg_cfg ar_btcoxe_aggr [1|0]" */
    {"ar_coxe_intvl",           MAC_ALG_CFG_AUTORATE_COEX_STAT_INTVL},          /* 设置共存统计时间间隔参数: sh hipriv.sh "vap0 alg_cfg ar_coxe_intvl [统计周期ms]"*/
    {"ar_coxe_low_th",          MAC_ALG_CFG_AUTORATE_COEX_LOW_ABORT_TH},        /* 设置共存abort低比例门限参数: sh hipriv.sh "vap0 alg_cfg ar_coxe_low_th [千分数]"*/
    {"ar_coxe_high_th",         MAC_ALG_CFG_AUTORATE_COEX_HIGH_ABORT_TH},       /* 设置共存abort高比例门限参数: sh hipriv.sh "vap0 alg_cfg ar_coxe_high_th [千分数]"*/
    {"ar_coxe_agrr_th",         MAC_ALG_CFG_AUTORATE_COEX_AGRR_NUM_ONE_TH},     /* 设置共存聚合数目为1的门限参数: sh hipriv.sh "vap0 alg_cfg ar_coxe_agrr_th [千分数]"*/

    {"ar_dyn_bw_en",            MAC_ALG_CFG_AUTORATE_DYNAMIC_BW_ENABLE},        /* 动态带宽特性使能开关: sh hipriv.sh "vap0 alg_cfg ar_dyn_bw_en [0/1]" */
    {"ar_thpt_wave_opt",        MAC_ALG_CFG_AUTORATE_THRPT_WAVE_OPT},           /* 吞吐量波动优化开关: sh hipriv.sh "vap0 alg_cfg ar_thpt_wave_opt [0/1]" */
    {"ar_gdpt_diff_th",         MAC_ALG_CFG_AUTORATE_GOODPUT_DIFF_TH},          /* 设置判断吞吐量波动的goodput差异比例门限(千分数): sh hipriv.sh "vap0 alg_cfg ar_gdpt_diff_th [goodput相差比例门限(千分数)]" */
    {"ar_per_worse_th",         MAC_ALG_CFG_AUTORATE_PER_WORSE_TH},             /* 设置判断吞吐量波动的PER变差的门限(千分数): sh hipriv.sh "vap0 alg_cfg ar_per_worse_th [PER变差门限(千分数)]" */

    {"sm_train_num",            MAC_ALG_CFG_SMARTANT_TRAINING_PACKET_NUMBER},

    {"anti_inf_imm_en",         MAC_ALG_CFG_ANTI_INTF_IMM_ENABLE},      /* 弱干扰免疫中non-direct使能: sh hipriv.sh "vap0 alg_cfg anti_inf_imm_en 0|1" */
    {"anti_inf_unlock_en",      MAC_ALG_CFG_ANTI_INTF_UNLOCK_ENABLE},   /* 弱干扰免疫中dynamic unlock使能: sh hipriv.sh "vap0 alg_cfg anti_inf_unlock_en 0|1" */
    {"anti_inf_stat_time",      MAC_ALG_CFG_ANTI_INTF_RSSI_STAT_CYCLE}, /* 弱干扰免疫中rssi统计周期: sh hipriv.sh "vap0 anti_inf_stat_time [time]" */
    {"anti_inf_off_time",       MAC_ALG_CFG_ANTI_INTF_UNLOCK_CYCLE},    /* 弱干扰免疫中unlock关闭周期: sh hipriv.sh "vap0 anti_inf_off_time [time]" */
    {"anti_inf_off_dur",        MAC_ALG_CFG_ANTI_INTF_UNLOCK_DUR_TIME}, /* 弱干扰免疫中unlock关闭持续时间: sh hipriv.sh "vap0 anti_inf_off_dur [time]" */
    {"anti_inf_stat_mode",      MAC_ALG_CFG_ANTI_INTF_RSSI_STAT_MODE},  /* 弱干扰免疫中rssi统计方式: sh hipriv.sh "vap0 anti_inf_stat_mode 0|1" */
    {"anti_inf_nav_en",         MAC_ALG_CFG_ANTI_INTF_NAV_IMM_ENABLE},  /* 抗干扰nav免疫使能: sh hipriv.sh "vap0 alg_cfg anti_inf_nav_en 0|1" */
    {"edca_opt_co_ch_time",     MAC_ALG_CFG_EDCA_OPT_CO_CH_DET_CYCLE},  /* 同频干扰检测周期: sh hipriv.sh "vap0 alg_cfg edca_opt_co_ch_time [time]" */
    {"edca_opt_en_ap",          MAC_ALG_CFG_EDCA_OPT_AP_EN_MODE},       /* ap模式下edca优化使能模式: sh hipriv.sh "vap0 alg_cfg edca_opt_en_ap 0|1|2" */
    {"edca_opt_en_sta",         MAC_ALG_CFG_EDCA_OPT_STA_EN},           /* sta模式下edca优化使能模式: sh hipriv.sh "vap0 alg_cfg edca_opt_en_sta 0|1" */
    {"edca_opt_sta_weight",     MAC_ALG_CFG_EDCA_OPT_STA_WEIGHT},       /* sta模式下edca优化的weighting系数: sh hipriv.sh "vap0 alg_cfg edca_opt_sta_weight 0~3"*/
    {"edca_opt_nondir_th",      MAC_ALG_CFG_EDCA_OPT_NONDIR_TH},        /* non-direct包占空比门限 sh hipriv.sh "vap0 alg_cfg edca_opt_nondir_th [val]" */
    {"edca_opt_th_udp",         MAC_ALG_CFG_EDCA_OPT_TH_UDP},           /* ap模式下UDP业务对应包判别门限 sh hipriv.sh "vap0 alg_cfg edca_opt_th_udp [val]" */
    {"edca_opt_th_tcp",         MAC_ALG_CFG_EDCA_OPT_TH_TCP},           /* ap模式下tcP业务对应包判别门限 sh hipriv.sh "vap0 alg_cfg edca_opt_th_tcp [val]" */
    {"edca_opt_debug_mode",     MAC_ALG_CFG_EDCA_OPT_DEBUG_MODE},       /* 是否打印相关信息，仅用于本地版本调试 */

    {"tpc_mode",                MAC_ALG_CFG_TPC_MODE},                          /* 设置TPC工作模式 */
    {"tpc_dbg",                 MAC_ALG_CFG_TPC_DEBUG},                         /* 设置TPC的debug开关 */
    {"tpc_pow_lvl",             MAC_ALG_CFG_TPC_POWER_LEVEL},                   /* 设置TPC功率等级(0,1,2,3), 在固定功率模式下使用 */
    {"tpc_log",                 MAC_ALG_CFG_TPC_LOG},                           /* 设置TPC的log开关:sh hipriv.sh "vap0 alg_cfg tpc_log 1 */
    {"tpc_stat_log_do",         MAC_ALG_CFG_TPC_STAT_LOG_START},                /* 开始功率统计日志:sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_do [mac地址] [业务类别] [包数目]" 如: sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_do 06:31:04:E3:81:02 1 1000" */
    {"tpc_stat_log_out",        MAC_ALG_CFG_TPC_STAT_LOG_WRITE},                /* 打印功率统计日志:sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_out 06:31:04:E3:81:02" */
    {"tpc_pkt_log_do",          MAC_ALG_CFG_TPC_PER_PKT_LOG_START},             /* 开始每包统计日志:sh hipriv.sh "vap0 alg_tpc_log tpc_pkt_log_do [mac地址] [业务类别] [包数目]" 如: sh hipriv.sh "vap0 alg_tpc_log tpc_pkt_log_do 06:31:04:E3:81:02 1 1000" */
    {"tpc_get_frame_pow",       MAC_ALG_CFG_TPC_GET_FRAME_POW},                 /* 获取特殊帧功率:sh hipriv.sh "vap0 alg_tpc_log tpc_get_frame_pow beacon_pow" */
    {"tpc_mag_frm_pow_lvl",     MAC_ALG_CFG_TPC_MANAGEMENT_MCAST_FRM_POWER_LEVEL}, /*TPC管理帧和多播帧功率等级*/
    {"tpc_ctl_frm_pow_lvl",     MAC_ALG_CFG_TPC_CONTROL_FRM_POWER_LEVEL},          /*TPC控制帧功率等级*/
    {"tpc_reset_stat",          MAC_ALG_CFG_TPC_RESET_STAT},                    /*释放统计内存*/
    {"tpc_reset_pkt",           MAC_ALG_CFG_TPC_RESET_PKT},                     /*释放每包内存*/
    {"tpc_over_temp_th",        MAC_ALG_CFG_TPC_OVER_TMP_TH},                   /* TPC过温门限 */
    {"tpc_dpd_enable_rate",     MAC_ALG_CFG_TPC_DPD_ENABLE_RATE},               /* 配置DPD生效的速率INDEX */
    {"tpc_target_rate_11b",     MAC_ALG_CFG_TPC_TARGET_RATE_11B},               /* 11b目标速率设置 */
    {"tpc_target_rate_11ag",    MAC_ALG_CFG_TPC_TARGET_RATE_11AG},              /* 11ag目标速率设置 */
    {"tpc_target_rate_11n20",   MAC_ALG_CFG_TPC_TARGET_RATE_HT40},              /* 11n20目标速率设置 */
    {"tpc_target_rate_11n40",   MAC_ALG_CFG_TPC_TARGET_RATE_HT40},              /* 11n40目标速率设置 */
    {"tpc_target_rate_11ac20",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT20},             /* 11ac20目标速率设置 */
    {"tpc_target_rate_11ac40",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT40},             /* 11ac40目标速率设置 */
    {"tpc_target_rate_11ac80",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT80},             /* 11ac80目标速率设置 */
    {"tpc_show_log_info",       MAC_ALG_CFG_TPC_SHOW_LOG_INFO},                 /* 打印TPC的日志信息:sh hipriv.sh "vap0 alg_cfg tpc_show_log_info */

    {OAL_PTR_NULL}
};

OAL_CONST wal_dfs_domain_entry_stru g_ast_dfs_domain_table[] =
{
    {"AE", MAC_DFS_DOMAIN_ETSI},
    {"AL", MAC_DFS_DOMAIN_NULL},
    {"AM", MAC_DFS_DOMAIN_ETSI},
    {"AN", MAC_DFS_DOMAIN_ETSI},
    {"AR", MAC_DFS_DOMAIN_FCC},
    {"AT", MAC_DFS_DOMAIN_ETSI},
    {"AU", MAC_DFS_DOMAIN_FCC},
    {"AZ", MAC_DFS_DOMAIN_ETSI},
    {"BA", MAC_DFS_DOMAIN_ETSI},
    {"BE", MAC_DFS_DOMAIN_ETSI},
    {"BG", MAC_DFS_DOMAIN_ETSI},
    {"BH", MAC_DFS_DOMAIN_ETSI},
    {"BL", MAC_DFS_DOMAIN_NULL},
    {"BN", MAC_DFS_DOMAIN_ETSI},
    {"BO", MAC_DFS_DOMAIN_ETSI},
    {"BR", MAC_DFS_DOMAIN_FCC},
    {"BY", MAC_DFS_DOMAIN_ETSI},
    {"BZ", MAC_DFS_DOMAIN_ETSI},
    {"CA", MAC_DFS_DOMAIN_FCC},
    {"CH", MAC_DFS_DOMAIN_ETSI},
    {"CL", MAC_DFS_DOMAIN_NULL},
    {"CN", MAC_DFS_DOMAIN_NULL},
    {"CO", MAC_DFS_DOMAIN_FCC},
    {"CR", MAC_DFS_DOMAIN_FCC},
    {"CS", MAC_DFS_DOMAIN_ETSI},
    {"CY", MAC_DFS_DOMAIN_ETSI},
    {"CZ", MAC_DFS_DOMAIN_ETSI},
    {"DE", MAC_DFS_DOMAIN_ETSI},
    {"DK", MAC_DFS_DOMAIN_ETSI},
    {"DO", MAC_DFS_DOMAIN_FCC},
    {"DZ", MAC_DFS_DOMAIN_NULL},
    {"EC", MAC_DFS_DOMAIN_FCC},
    {"EE", MAC_DFS_DOMAIN_ETSI},
    {"EG", MAC_DFS_DOMAIN_ETSI},
    {"ES", MAC_DFS_DOMAIN_ETSI},
    {"FI", MAC_DFS_DOMAIN_ETSI},
    {"FR", MAC_DFS_DOMAIN_ETSI},
    {"GB", MAC_DFS_DOMAIN_ETSI},
    {"GE", MAC_DFS_DOMAIN_ETSI},
    {"GR", MAC_DFS_DOMAIN_ETSI},
    {"GT", MAC_DFS_DOMAIN_FCC},
    {"HK", MAC_DFS_DOMAIN_FCC},
    {"HN", MAC_DFS_DOMAIN_FCC},
    {"HR", MAC_DFS_DOMAIN_ETSI},
    {"HU", MAC_DFS_DOMAIN_ETSI},
    {"ID", MAC_DFS_DOMAIN_NULL},
    {"IE", MAC_DFS_DOMAIN_ETSI},
    {"IL", MAC_DFS_DOMAIN_ETSI},
    {"IN", MAC_DFS_DOMAIN_NULL},
    {"IQ", MAC_DFS_DOMAIN_NULL},
    {"IR", MAC_DFS_DOMAIN_NULL},
    {"IS", MAC_DFS_DOMAIN_ETSI},
    {"IT", MAC_DFS_DOMAIN_ETSI},
    {"JM", MAC_DFS_DOMAIN_FCC},
    {"JO", MAC_DFS_DOMAIN_ETSI},
    {"JP", MAC_DFS_DOMAIN_MKK},
    {"KP", MAC_DFS_DOMAIN_NULL},
    {"KR", MAC_DFS_DOMAIN_KOREA},
    {"KW", MAC_DFS_DOMAIN_ETSI},
    {"KZ", MAC_DFS_DOMAIN_NULL},
    {"LB", MAC_DFS_DOMAIN_NULL},
    {"LI", MAC_DFS_DOMAIN_ETSI},
    {"LK", MAC_DFS_DOMAIN_FCC},
    {"LT", MAC_DFS_DOMAIN_ETSI},
    {"LU", MAC_DFS_DOMAIN_ETSI},
    {"LV", MAC_DFS_DOMAIN_ETSI},
    {"MA", MAC_DFS_DOMAIN_NULL},
    {"MC", MAC_DFS_DOMAIN_ETSI},
    {"MK", MAC_DFS_DOMAIN_ETSI},
    {"MO", MAC_DFS_DOMAIN_FCC},
    {"MT", MAC_DFS_DOMAIN_ETSI},
    {"MX", MAC_DFS_DOMAIN_FCC},
    {"MY", MAC_DFS_DOMAIN_FCC},
    {"NG", MAC_DFS_DOMAIN_NULL},
    {"NL", MAC_DFS_DOMAIN_ETSI},
    {"NO", MAC_DFS_DOMAIN_ETSI},
    {"NP", MAC_DFS_DOMAIN_NULL},
    {"NZ", MAC_DFS_DOMAIN_FCC},
    {"OM", MAC_DFS_DOMAIN_FCC},
    {"PA", MAC_DFS_DOMAIN_FCC},
    {"PE", MAC_DFS_DOMAIN_FCC},
    {"PG", MAC_DFS_DOMAIN_FCC},
    {"PH", MAC_DFS_DOMAIN_FCC},
    {"PK", MAC_DFS_DOMAIN_NULL},
    {"PL", MAC_DFS_DOMAIN_ETSI},
    {"PR", MAC_DFS_DOMAIN_FCC},
    {"PT", MAC_DFS_DOMAIN_ETSI},
    {"QA", MAC_DFS_DOMAIN_NULL},
    {"RO", MAC_DFS_DOMAIN_ETSI},
    {"RU", MAC_DFS_DOMAIN_FCC},
    {"SA", MAC_DFS_DOMAIN_FCC},
    {"SE", MAC_DFS_DOMAIN_ETSI},
    {"SG", MAC_DFS_DOMAIN_NULL},
    {"SI", MAC_DFS_DOMAIN_ETSI},
    {"SK", MAC_DFS_DOMAIN_ETSI},
    {"SV", MAC_DFS_DOMAIN_FCC},
    {"SY", MAC_DFS_DOMAIN_NULL},
    {"TH", MAC_DFS_DOMAIN_FCC},
    {"TN", MAC_DFS_DOMAIN_ETSI},
    {"TR", MAC_DFS_DOMAIN_ETSI},
    {"TT", MAC_DFS_DOMAIN_FCC},
    {"TW", MAC_DFS_DOMAIN_NULL},
    {"UA", MAC_DFS_DOMAIN_NULL},
    {"US", MAC_DFS_DOMAIN_FCC},
    {"UY", MAC_DFS_DOMAIN_FCC},
    {"UZ", MAC_DFS_DOMAIN_FCC},
    {"VE", MAC_DFS_DOMAIN_FCC},
    {"VN", MAC_DFS_DOMAIN_ETSI},
    {"YE", MAC_DFS_DOMAIN_NULL},
    {"ZA", MAC_DFS_DOMAIN_FCC},
    {"ZW", MAC_DFS_DOMAIN_NULL},
};

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))

OAL_CONST wal_atcmdsrv_mode_stru g_ast_atcmdsrv_mode_table[] =
{
    {WLAN_LEGACY_11A_MODE, WLAN_BAND_5G},    /* 11a, 5G, OFDM */
    {WLAN_LEGACY_11B_MODE, WLAN_BAND_2G},    /* 11b, 2.4G */
    {WLAN_LEGACY_11G_MODE, WLAN_BAND_2G},    /* 旧的11g only已废弃, 2.4G, OFDM */
    {WLAN_MIXED_ONE_11G_MODE, WLAN_BAND_2G},    /* 11bg, 2.4G */
    {WLAN_MIXED_TWO_11G_MODE, WLAN_BAND_2G},    /* 11g only, 2.4G */
    {WLAN_HT_MODE, WLAN_BAND_5G},    /* 11n(11bgn或者11an，根据频段判断) */
    {WLAN_VHT_MODE, WLAN_BAND_2G},    /* 11ac */
    {WLAN_HT_ONLY_MODE, WLAN_BAND_5G},    /* 11n only mode,只有带HT的设备才可以接入 */
    {WLAN_VHT_ONLY_MODE, WLAN_BAND_5G},    /* 11ac only mode 只有带VHT的设备才可以接入 */
    {WLAN_HT_11G_MODE, WLAN_BAND_2G},    /* 11ng,不包括11b*/
    {WLAN_PROTOCOL_BUTT,WLAN_BAND_2G},
};

OAL_STATIC OAL_CONST wal_atcmdsrv_datarate_stru   past_atcmdsrv_non_ht_rate_table[] =
{
    {1," 1 "},
    {2," 2 "},
    {5," 5.5 "},
    {6," 6 "},
    {9," 9 "},
    {11," 11 "},
    {12," 12 "},
    {18," 18 "},
    {24," 24 "},
    {36," 36 "},
    {48," 48 "},
    {54," 54 "},
};

#endif


/*****************************************************************************
  3 函数实现
*****************************************************************************/
/*****************************************************************************
 函 数 名  : wal_get_cmd_one_arg
 功能描述  : 获取字符串第一个参数 以空格为参数区分标识
 输入参数  : pc_cmd         : 传入的字符串
 输出参数  : pc_arg         : 第一个参数
             pul_cmd_offset : 第一个参数的长度
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月13日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_get_cmd_one_arg(oal_int8 *pc_cmd, oal_int8 *pc_arg, oal_uint32 *pul_cmd_offset)
{
    oal_int8   *pc_cmd_copy;
    oal_uint32  ul_pos = 0;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd) || (OAL_PTR_NULL == pc_arg) || (OAL_PTR_NULL == pul_cmd_offset)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_get_cmd_one_arg::pc_cmd/pc_arg/pul_cmd_offset null ptr error %d, %d, %d, %d!}\r\n", pc_cmd, pc_arg, pul_cmd_offset);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_cmd_copy = pc_cmd;

    /* 去掉字符串开始的空格 */
    while (' ' == *pc_cmd_copy)
    {
        ++pc_cmd_copy;
    }

    while ((' ' != *pc_cmd_copy) && ('\0' != *pc_cmd_copy))
    {
        pc_arg[ul_pos] = *pc_cmd_copy;
        ++ul_pos;
        ++pc_cmd_copy;

        if (OAL_UNLIKELY(ul_pos >= WAL_HIPRIV_CMD_NAME_MAX_LEN))
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_get_cmd_one_arg::ul_pos >= WAL_HIPRIV_CMD_NAME_MAX_LEN, ul_pos %d!}\r\n", ul_pos);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }

    pc_arg[ul_pos]  = '\0';

    /* 字符串到结尾，返回错误码 */
    if (0 == ul_pos)
    {
        OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_get_cmd_one_arg::return param pc_arg is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pul_cmd_offset = (oal_uint32)(pc_cmd_copy - pc_cmd);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_alloc_cfg_event
 功能描述  : WAL申请事件，并填充事件头
 输入参数  : pst_net_dev: net_device
 输出参数  : ppst_event_mem: 指向事件内存
             ppst_cfg_priv : 指向私有配置结构
             ppst_cfg_msg  : 指向配置消息
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_alloc_cfg_event
                (oal_net_device_stru     *pst_net_dev,
                 frw_event_mem_stru     **ppst_event_mem,
                 hmac_vap_cfg_priv_stru **ppst_cfg_priv,
                 wal_msg_stru           **ppst_cfg_msg,
                 oal_uint16               us_len)
{
    mac_vap_stru               *pst_vap;
    oal_uint32                  ul_ret;
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_alloc_cfg_event::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取配置私有结构 */
    ul_ret = hmac_vap_get_priv_cfg(pst_vap, ppst_cfg_priv);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{wal_alloc_cfg_event::hmac_vap_get_priv_cfg return err code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_event_mem = FRW_EVENT_ALLOC(us_len);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_ANY, "{wal_alloc_cfg_event::pst_event_mem null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *ppst_event_mem = pst_event_mem;    /* 出参赋值 */

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       WAL_HOST_CRX_SUBTYPE_CFG,
                       us_len,
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_vap->uc_chip_id,
                       pst_vap->uc_device_id,
                       pst_vap->uc_vap_id);

    *ppst_cfg_msg = (wal_msg_stru *)pst_event->auc_event_data;  /* 出参赋值 */

    return OAL_SUCC;
}


 oal_int32 wal_send_cfg_event(oal_net_device_stru      *pst_net_dev,
                                   wal_msg_type_enum_uint8   en_msg_type,
                                   oal_uint16                us_len,
                                   oal_uint8                *puc_param,
                                   oal_bool_enum_uint8       en_need_rsp,
                                   wal_msg_stru            **ppst_rsp_msg)
{
    wal_msg_stru                *pst_cfg_msg;
    frw_event_mem_stru          *pst_event_mem;
    hmac_vap_cfg_priv_stru      *pst_cfg_priv;
    oal_uint32                   ul_ret;
    oal_int32                    l_ret;
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    mac_vap_stru                *pst_mac_vap;
#endif

    /* 申请事件 */
    ul_ret = wal_alloc_cfg_event(pst_net_dev, &pst_event_mem, &pst_cfg_priv, &pst_cfg_msg, us_len);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_send_cfg_event::wal_alloc_cfg_event return err code %d!}\r\n", ul_ret);
        return -OAL_ENOMEM;
    }

    /* 填写配置消息 */
    WAL_CFG_MSG_HDR_INIT(&(pst_cfg_msg->st_msg_hdr),
                         en_msg_type,
                         us_len,
                         WAL_GET_MSG_SN(pst_cfg_priv->uc_msg_sn));

    /* 填写WID消息 */
    oal_memcopy(pst_cfg_msg->auc_msg_data, puc_param, us_len);

#ifdef _PRE_WLAN_DFT_EVENT
    wal_event_report_to_sdt(en_msg_type, puc_param, pst_cfg_msg);
#endif

    pst_cfg_priv->en_wait_ack = OAL_FALSE;          /* 设置等待事件处理返回状态 */

/* 分发事件 */
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
        pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
        frw_event_post_event(pst_event_mem, pst_mac_vap->ul_core_id);
#else
        frw_event_dispatch_event(pst_event_mem);
#endif

    FRW_EVENT_FREE(pst_event_mem);

    if (OAL_FALSE == en_need_rsp)
    {
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == ppst_rsp_msg)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event::OAL_PTR_NULL == ppst_rsp_msg!}\r\n");
        return -OAL_ENOMEM;
    }

    /* win32 UT模式，触发一次事件调度 */
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    frw_event_process_all_event(0);
#endif

    /***************************************************************************
        等待事件返回
    ***************************************************************************/
    
    /*lint -e730*//* info, boolean argument to function */
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_cfg_priv->st_wait_queue, (OAL_TRUE == pst_cfg_priv->en_wait_ack), 6 * OAL_TIME_HZ);
    /*lint +e730*/
    /*lint -e774*//* 规避WIN32版本下的PC LINT错误 */

    if (0 == l_ret)
    {
        /* 超时 */
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event:: wait queue timeout!}\r\n");
        return -OAL_EINVAL;
    }
    else if (l_ret < 0)
    {
        /* 异常 */
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event:: wait queue error!}\r\n");
        return -OAL_EINVAL;
    }
    /*lint +e774*/

    /* 发送配置事件返回的状态信息 */
    *ppst_rsp_msg  = (wal_msg_stru *)(pst_cfg_priv->ac_rsp_msg);

    return OAL_SUCC;

}
#ifdef _PRE_WLAN_FEATURE_P2P
wlan_p2p_mode_enum_uint8 wal_wireless_iftype_to_mac_p2p_mode(enum nl80211_iftype en_iftype)
{
    wlan_p2p_mode_enum_uint8 en_p2p_mode = WLAN_LEGACY_VAP_MODE;

    switch(en_iftype)
    {
        case NL80211_IFTYPE_P2P_CLIENT:
            en_p2p_mode = WLAN_P2P_CL_MODE;
            break;
        case NL80211_IFTYPE_P2P_GO:
            en_p2p_mode = WLAN_P2P_GO_MODE;
            break;
        case NL80211_IFTYPE_P2P_DEVICE:
            en_p2p_mode = WLAN_P2P_DEV_MODE;
            break;
        case NL80211_IFTYPE_AP:
        case NL80211_IFTYPE_STATION:
            en_p2p_mode = WLAN_LEGACY_VAP_MODE;
            break;
        default:
            en_p2p_mode = WLAN_P2P_BUTT;
    }
    return en_p2p_mode;
}
#endif
/*****************************************************************************
 函 数 名  : wal_cfg_vap_h2d_event
 功能描述  : cfg vap h2d
 输入参数  : pst_net_dev: net_device
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日
    作    者   : liuzhengqi
    修改内容   : 新生成函数

*****************************************************************************/

oal_int32  wal_cfg_vap_h2d_event(oal_net_device_stru *pst_net_dev)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_wireless_dev_stru      *pst_wdev;
    mac_wiphy_priv_stru        *pst_wiphy_priv;
    hmac_vap_stru              *pst_cfg_hmac_vap;
    mac_device_stru            *pst_mac_device;
    oal_net_device_stru        *pst_cfg_net_dev;

    oal_int32                   l_ret;
    wal_msg_stru                *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru      *pst_write_rsp_msg;
    wal_msg_write_stru          st_write_msg;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if(NULL == pst_wdev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_wdev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wiphy_priv  = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    pst_mac_device  = pst_wiphy_priv->pst_mac_device;
    if(NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_hmac_vap= (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;
    if(NULL == pst_cfg_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_net_dev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
    抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG_VAP_H2D, OAL_SIZEOF(mac_cfg_vap_h2d_stru));
    ((mac_cfg_vap_h2d_stru *)st_write_msg.auc_value)->pst_net_dev = pst_cfg_net_dev;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
             WAL_MSG_TYPE_WRITE,
             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_vap_h2d_stru),
             (oal_uint8 *)&st_write_msg,
             OAL_TRUE,
             &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::hmac cfg vap h2d fail,err code[%d]\r\n", pst_write_rsp_msg->ul_err_code);
            return -OAL_EINVAL;
        }
    }
#endif

    return OAL_SUCC;

}

/*****************************************************************************
 函 数 名  : wal_netdev_open
 功能描述  : 启用VAP
 输入参数  : pst_net_dev: net_device
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月11日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_int32  wal_netdev_open(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru      st_write_msg;
    oal_int32               l_ret;
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru *pst_write_rsp_msg;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru   *pst_wdev;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    OAL_IO_PRINT("wal_netdev_open,dev_name is:%s\n", pst_net_dev->name);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::iftype:%d.!}\r\n", pst_net_dev->ieee80211_ptr->iftype);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

       l_ret = wlan_pm_open();
       if (OAL_FAIL == l_ret)
       {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::wlan_pm_open Fail!}\r\n");
            return -OAL_EFAIL;
       }
       else if(OAL_ERR_CODE_ALREADY_OPEN != l_ret)
       {
            // 重新上电场景，下发配置VAP
            l_ret = wal_cfg_vap_h2d_event(pst_net_dev);
            if(OAL_SUCC != l_ret)
            {
                OAL_IO_PRINT("wal_cfg_vap_h2d_event FAIL %d \r\n",l_ret);
                return -OAL_EFAIL;
            }
            OAL_IO_PRINT("wal_cfg_vap_h2d_event succ \r\n");
       }

       wal_send_cali_data(pst_net_dev);

#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    if (((NL80211_IFTYPE_STATION == pst_net_dev->ieee80211_ptr->iftype) || (NL80211_IFTYPE_P2P_DEVICE == pst_net_dev->ieee80211_ptr->iftype))
         &&((0 == (oal_strcmp("wlan0", pst_net_dev->name))) || (0 == (oal_strcmp("p2p0", pst_net_dev->name)))))
    {
        l_ret = wal_init_wlan_vap(pst_net_dev);
        if(OAL_SUCC != l_ret)
        {
            return -OAL_EFAIL;
        }
    }
    else if(NL80211_IFTYPE_AP == pst_net_dev->ieee80211_ptr->iftype)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::ap mode,no need to start vap.!}\r\n");
        return OAL_SUCC;
    }
#endif

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_VAP, OAL_SIZEOF(mac_cfg_start_vap_param_stru));
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_mgmt_rate_init_flag = OAL_TRUE;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::hmac start vap fail,err code[%d]!}\r\n", pst_write_rsp_msg->ul_err_code);
            return -OAL_EINVAL;
        }
    }

    if (0 == (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING))
    {
        OAL_NETDEVICE_FLAGS(pst_net_dev) |= OAL_IFF_RUNNING;
    }

    return OAL_SUCC;
}
oal_int32  wal_netdev_stop(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    oal_int32                   l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode;
    oal_wireless_dev_stru      *pst_wdev;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    OAL_IO_PRINT("wal_netdev_stop,dev_name is:%s\n", pst_net_dev->name);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::iftype:%d.!}\r\n", pst_net_dev->ieee80211_ptr->iftype);

    /* AP模式下,在模式切换时down和删除 vap */
    if(NL80211_IFTYPE_AP == pst_net_dev->ieee80211_ptr->iftype)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::ap mode,no need to stop vap.!}\r\n");
        return OAL_SUCC;
    }
#endif

    /* 如果不是up状态，则直接返回成功 */
    if (0 == (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::vap is already down!}\r\n");
        return OAL_SUCC;
    }

    wal_force_scan_complete(pst_net_dev, OAL_TRUE);

    /***************************************************************************
                           抛事件到wal层处理
    ***************************************************************************/
    /* 填写WID消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DOWN_VAP, OAL_SIZEOF(mac_cfg_down_vap_param_stru));
    ((mac_cfg_down_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::hmac stop vap fail!err code [%d]}\r\n", pst_write_rsp_msg->ul_err_code);
            return -OAL_EFAIL;
        }
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* wlan0/p2p0 down时 删除VAP */
    if (((NL80211_IFTYPE_STATION == pst_net_dev->ieee80211_ptr->iftype) || (NL80211_IFTYPE_P2P_DEVICE == pst_net_dev->ieee80211_ptr->iftype))
         &&((0 == (oal_strcmp("wlan0", pst_net_dev->name))) || (0 == (oal_strcmp("p2p0", pst_net_dev->name)))))
    {
        l_ret = wal_deinit_wlan_vap(pst_net_dev);
        if(OAL_SUCC!=l_ret)
        {
            return l_ret;
        }

        return OAL_SUCC;
    }
#endif

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_netdev_get_stats
 功能描述  : 获取统计信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月1日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_net_device_stats_stru*  wal_netdev_get_stats(oal_net_device_stru *pst_net_dev)
{
    oal_net_device_stats_stru  *pst_stats = &(pst_net_dev->stats);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

#ifdef _PRE_DEBUG_MODE
    mac_vap_stru               *pst_mac_vap;
    oam_stat_info_stru         *pst_oam_stat;

    oam_vap_stat_info_stru     *pst_oam_vap_stat;

    pst_mac_vap  = OAL_NET_DEV_PRIV(pst_net_dev);
    pst_oam_stat = OAM_STAT_GET_STAT_ALL();

    pst_oam_vap_stat = &(pst_oam_stat->ast_vap_stat_info[pst_mac_vap->uc_vap_id]);

    /* 更新统计信息到net_device */
    pst_stats->rx_packets   = pst_oam_vap_stat->ul_rx_pkt_to_lan;
    pst_stats->rx_bytes     = pst_oam_vap_stat->ul_rx_bytes_to_lan;
    pst_stats->rx_dropped   = pst_oam_vap_stat->ul_rx_ta_check_dropped +
                              pst_oam_vap_stat->ul_rx_da_check_dropped +
                              pst_oam_vap_stat->ul_rx_key_search_fail_dropped +
                              pst_oam_vap_stat->ul_rx_tkip_mic_fail_dropped +
                              pst_oam_vap_stat->ul_rx_replay_fail_dropped +
                              pst_oam_vap_stat->ul_rx_11i_check_fail_dropped +
                              pst_oam_vap_stat->ul_rx_wep_check_fail_dropped +
                              pst_oam_vap_stat->ul_rx_alg_process_dropped +
                              pst_oam_vap_stat->ul_rx_null_frame_dropped +
                              pst_oam_vap_stat->ul_rx_abnormal_dropped +
                              pst_oam_vap_stat->ul_rx_mgmt_abnormal_dropped +
                              pst_oam_vap_stat->ul_rx_pspoll_process_dropped +
                              pst_oam_vap_stat->ul_rx_bar_process_dropped +
                              pst_oam_vap_stat->ul_rx_no_buff_dropped +
                              pst_oam_vap_stat->ul_rx_defrag_process_dropped +
                              pst_oam_vap_stat->ul_rx_de_mic_fail_dropped +
                              pst_oam_vap_stat->ul_rx_portvalid_check_fail_dropped;

    pst_stats->tx_packets   = pst_oam_vap_stat->ul_tx_pkt_num_from_lan;
    pst_stats->tx_bytes     = pst_oam_vap_stat->ul_tx_bytes_from_lan;
    pst_stats->tx_dropped   = pst_oam_vap_stat->ul_tx_abnormal_msdu_dropped +
                              pst_oam_vap_stat->ul_tx_security_check_faild +
                              pst_oam_vap_stat->ul_tx_abnormal_mpdu_dropped +
                              pst_oam_vap_stat->ul_tx_uapsd_process_dropped +
                              pst_oam_vap_stat->ul_tx_psm_process_dropped +
                              pst_oam_vap_stat->ul_tx_alg_process_dropped;
#endif
#endif
    return pst_stats;
}

/*****************************************************************************
 函 数 名  : wal_netdev_set_mac_addr
 功能描述  : 设置mac地址
 输入参数  : pst_dev: 网络设备
             p_addr : 地址
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月24日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
/*lint -e527*//* 规避WIN32版本下的PC LINT错误 */
OAL_STATIC oal_int32  wal_netdev_set_mac_addr(oal_net_device_stru *pst_net_dev, void *p_addr)
{
    oal_sockaddr_stru            *pst_mac_addr;
    oal_uint32                    ul_ret;
    frw_event_mem_stru           *pst_event_mem;
    wal_msg_stru                 *pst_cfg_msg;
    wal_msg_write_stru           *pst_write_msg;
    hmac_vap_cfg_priv_stru       *pst_cfg_priv;
    mac_cfg_staion_id_param_stru *pst_param;
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    mac_vap_stru                 *pst_mac_vap;
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_wireless_dev_stru        *pst_wdev; /* 对于P2P 场景，p2p0 和 p2p-p2p0 MAC 地址从wlan0 获取 */
    wlan_p2p_mode_enum_uint8      en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == p_addr)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::pst_net_dev or p_addr null ptr error %d, %d!}\r\n", pst_net_dev, p_addr);

        return -OAL_EFAUL;
    }

    /*lint -e774*//*lint -e506*//* 规避WIN32版本下的PC LINT错误 */
    if (oal_netif_running(pst_net_dev))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::cannot set address; device running!}\r\n");

        return -OAL_EBUSY;
    }
    /*lint +e774*//*lint +e506*/

    pst_mac_addr = (oal_sockaddr_stru *)p_addr;

    if (ETHER_IS_MULTICAST(pst_mac_addr->sa_data))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::can not set group/broadcast addr!}\r\n");

        return -OAL_EINVAL;
    }

    oal_set_mac_addr((oal_uint8 *)(pst_net_dev->dev_addr), (oal_uint8 *)(pst_mac_addr->sa_data));

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    return OAL_SUCC;
#endif

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_alloc_cfg_event(pst_net_dev, &pst_event_mem, &pst_cfg_priv, &pst_cfg_msg, (WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru)));     /* 申请事件 */
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::wal_alloc_cfg_event return err code %d!}\r\n", ul_ret);
        return -OAL_ENOMEM;
    }

    /* 填写配置消息 */
    WAL_CFG_MSG_HDR_INIT(&(pst_cfg_msg->st_msg_hdr),
                         WAL_MSG_TYPE_WRITE,
                         WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru),
                         WAL_GET_MSG_SN(pst_cfg_priv->uc_msg_sn));

    /* 填写WID消息 */
    pst_write_msg = (wal_msg_write_stru *)pst_cfg_msg->auc_msg_data;
    WAL_WRITE_MSG_HDR_INIT(pst_write_msg, WLAN_CFGID_STATION_ID, OAL_SIZEOF(mac_cfg_staion_id_param_stru));

    pst_param = (mac_cfg_staion_id_param_stru *)pst_write_msg->auc_value;   /* 填写WID对应的参数 */
    oal_set_mac_addr(pst_param->auc_station_id, (oal_uint8 *)(pst_mac_addr->sa_data));
#ifdef _PRE_WLAN_FEATURE_P2P
    /* 填写下发net_device 对应p2p 模式 */
    pst_wdev = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    pst_param->en_p2p_mode = en_p2p_mode;
#endif

#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    frw_event_post_event(pst_event_mem, pst_mac_vap->ul_core_id);
#else
    frw_event_dispatch_event(pst_event_mem);
#endif

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
/*lint +e527*//* 规避WIN32版本下的PC LINT错误 */

/*****************************************************************************
 函 数 名  : wal_android_priv_cmd
 功能描述  : 私有接口函数
 输入参数  : net device指针
 输出参数  : 无
 返 回 值  : 统计结果指针
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月24日
    作    者   : xiaoyuren x00305155
    修改内容   : 新增函数

*****************************************************************************/
oal_int32 wal_android_priv_cmd(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    wal_android_wifi_priv_cmd_stru  st_priv_cmd;
    oal_uint8   *puc_command        = OAL_PTR_NULL;
    oal_int32    l_ret              = 0;


    if (OAL_PTR_NULL == pst_ifr->ifr_data)
    {
        l_ret = -OAL_EINVAL;
        return l_ret;
    }

    if (oal_copy_from_user(&st_priv_cmd, pst_ifr->ifr_data, sizeof(wal_android_wifi_priv_cmd_stru)))
    {
        l_ret = -OAL_EINVAL;
        return l_ret;
    }

    if (st_priv_cmd.l_total_len > MAX_PRIV_CMD_SIZE)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::too long priavte command. len:%d. }\r\n", st_priv_cmd.l_total_len);
        l_ret = -OAL_EINVAL;
        return l_ret;
    }

    /* 申请内存保存wpa_supplicant 下发的命令和数据 */
    puc_command = oal_memalloc((oal_uint32)st_priv_cmd.l_total_len + 1);/* total len 为priv cmd 后面buffer 长度 */
    if (OAL_PTR_NULL == puc_command)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::mem alloc failed.}\r\n");

        l_ret = -OAL_ENOMEM;
        return l_ret;
    }

    /* 拷贝wpa_supplicant 命令到内核态中 */
    oal_memset(puc_command, 0, (oal_uint32)(st_priv_cmd.l_total_len + 1));

    l_ret = (oal_int32)oal_copy_from_user(puc_command, pst_ifr->ifr_data + 8, (oal_uint32)(st_priv_cmd.l_total_len));

    if (l_ret != 0)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::oal_copy_from_user: -OAL_EFAIL }\r\n");
        l_ret = -OAL_EFAIL;
        oal_free(puc_command);
        return l_ret;
    }
    puc_command[st_priv_cmd.l_total_len] = '\0';

    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_android_priv_cmd::Android private cmd total_len:%d, used_len:%d, CMD:0x%x %x}\r\n",
                    st_priv_cmd.l_total_len, st_priv_cmd.l_used_len, *((oal_uint32*)puc_command),*((oal_uint32*)(puc_command+4)) );

    if (oal_memcmp(puc_command, CMD_SET_AP_WPS_P2P_IE, OAL_STRLEN(CMD_SET_AP_WPS_P2P_IE)) == 0)
    {
        oal_uint32 skip = OAL_STRLEN(CMD_SET_AP_WPS_P2P_IE) + 1;
        /* 结构体类型 */
        oal_app_ie_stru *pst_wps_p2p_ie;
        pst_wps_p2p_ie = (oal_app_ie_stru *)(puc_command + skip);

        l_ret = wal_ioctl_set_wps_p2p_ie(pst_net_dev,
                                            pst_wps_p2p_ie->auc_ie,
                                            pst_wps_p2p_ie->ul_ie_len,
                                            pst_wps_p2p_ie->en_app_ie_type);
    }
#ifdef _PRE_WLAN_FEATURE_P2P
    else if(oal_memcmp(puc_command, CMD_P2P_SET_NOA, OAL_STRLEN(CMD_P2P_SET_NOA)) == 0)
    {
        oal_uint32 skip = OAL_STRLEN(CMD_P2P_SET_NOA) + 1;
        mac_cfg_p2p_noa_param_stru  st_p2p_noa_param;
        oal_memcopy(&st_p2p_noa_param, puc_command + skip, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));

        l_ret = wal_ioctl_set_p2p_noa(pst_net_dev,&st_p2p_noa_param);
    }
    else if(oal_memcmp(puc_command, CMD_P2P_SET_PS, OAL_STRLEN(CMD_P2P_SET_PS)) == 0)
    {
        oal_uint32 skip = OAL_STRLEN(CMD_P2P_SET_PS) + 1;
        mac_cfg_p2p_ops_param_stru  st_p2p_ops_param;
        oal_memcopy(&st_p2p_ops_param, puc_command + skip, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));

        l_ret = wal_ioctl_set_p2p_ops(pst_net_dev,&st_p2p_ops_param);
    }
#endif
    else if(0 == oal_memcmp(puc_command, CMD_SET_POWER_ON, OAL_STRLEN(CMD_SET_POWER_ON)))
    {
        oal_int32 power_flag = -1;
        oal_uint32 command_len = OAL_STRLEN((oal_int8 *)puc_command);
        /* 格式:SET_POWER_ON 1 or SET_POWER_ON 0 */
        if(command_len < (OAL_STRLEN((oal_int8 *)CMD_SET_POWER_ON) + 2))
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_SET_POWER_ON cmd len must equal or larger than 18. Now the cmd len:%d.}\r\n", command_len);

            oal_free(puc_command);
            return -OAL_FAIL;
        }

        power_flag = oal_atoi((oal_int8 *)puc_command + OAL_STRLEN((oal_int8 *)CMD_SET_POWER_ON) + 1);

        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_SET_POWER_ON command,power flag:%d}\r\n", power_flag);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        //ap上下电，配置VAP
        if (0 == power_flag)//下电
        {

            wlan_pm_close();

        }
        else if (1 == power_flag) //上电
        {
            l_ret = wlan_pm_open();

            if (OAL_FAIL==l_ret)
            {
                 OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::wlan_pm_open Fail!}\r\n");
                 oal_free(puc_command);
                 return -OAL_EFAIL;
            }
            else if(OAL_ERR_CODE_ALREADY_OPEN != l_ret)
            {
                // 重新上电场景，下发配置VAP
                l_ret = wal_cfg_vap_h2d_event(pst_net_dev);

                if(OAL_SUCC!= l_ret)
                {
                    oal_free(puc_command);
                    return -OAL_EFAIL;
                }
            }
        }
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::pupower_flag:%d error.}\r\n", power_flag);
            oal_free(puc_command);
            return -OAL_FAIL;
        }
#endif
    }
    else if(0 == oal_memcmp(puc_command, CMD_COUNTRY, OAL_STRLEN(CMD_COUNTRY)))
    {
    #ifdef _PRE_WLAN_FEATURE_11D
        const oal_int8 *country_code;
        oal_int8        auc_country_code[3] = {0};
        oal_int32       l_ret;

        /* 格式:COUNTRY CN */
        if(OAL_STRLEN((oal_int8 *)puc_command) < (OAL_STRLEN((oal_int8 *)CMD_COUNTRY) + 3))
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::puc_command len error.}\r\n");

            oal_free(puc_command);
            return -OAL_FAIL;
        }

        country_code = (oal_int8 *)puc_command + OAL_STRLEN((oal_int8 *)CMD_COUNTRY) + 1;

        oal_memcopy(auc_country_code, country_code, 2);

        l_ret = wal_regdomain_update_for_dfs(pst_net_dev, auc_country_code);

        if (OAL_UNLIKELY(OAL_SUCC != l_ret))
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::return err code [%d]!}\r\n", l_ret);

            oal_free(puc_command);
            return -OAL_EFAIL;
        }

        l_ret = wal_regdomain_update(pst_net_dev, auc_country_code);

        if (OAL_UNLIKELY(OAL_SUCC != l_ret))
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::return err code [%d]!}\r\n", l_ret);

            oal_free(puc_command);
            return -OAL_EFAIL;
        }
    #else
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
    #endif
    }
    else
    {
        /* 驱动对于不支持的命令，返回成功，否则上层wpa_supplicant认为ioctl失败，导致异常重启wifi */
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::Unknown PRIVATE command - ignore. Check it in applog:wpa_driver_nl80211_driver_cmd.}\r\n");
        l_ret = OAL_SUCC;
    }

    oal_free(puc_command);
    return l_ret;
#else
    return  OAL_SUCC;
#endif
}

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_freq(oal_net_device_stru *pst_net_dev, oal_int32 l_freq)
{
    wal_msg_write_stru          st_write_msg;

    oal_int32                   l_ret;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_freq:l_freq[%d]", l_freq);
    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CURRENT_CHANEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_freq;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_freq::return err code %d!}", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_txpower(oal_net_device_stru *pst_net_dev, oal_int32 l_txpower)
{
    wal_msg_write_stru          st_write_msg;

    oal_int32                   l_ret;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_txpower:l_txpower[%d]", l_txpower);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_POWER, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_txpower;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_txpower::return err code %d!}", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_mode(oal_net_device_stru *pst_net_dev, oal_int32 l_mode)
{
    wal_msg_write_stru          st_write_msg;
    mac_cfg_mode_param_stru    *pst_mode_param;
    oal_uint8                   uc_prot_idx;

    oal_int32                   l_ret;

    /*获取模式对应的band*/
    for (uc_prot_idx = 0; uc_prot_idx < WAL_ATCMDSRV_IOCTL_MODE_NUM; uc_prot_idx++)
    {
        if (g_ast_atcmdsrv_mode_table[uc_prot_idx].uc_mode == (oal_uint8)l_mode)
        {
            break;
        }
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru));

    /*设置模式，在配置模式的时候将带宽默认成20M*/
    pst_mode_param = (mac_cfg_mode_param_stru *)(st_write_msg.auc_value);
    pst_mode_param->en_protocol  = (oal_uint8)l_mode;
    pst_mode_param->en_band      = g_ast_atcmdsrv_mode_table[uc_prot_idx].uc_band;
    pst_mode_param->en_bandwidth = WLAN_BAND_WIDTH_20M;
    /*未测使用，后续将删除*/
    OAM_WARNING_LOG3(0, OAM_SF_CFG, "{wal_atcmsrv_ioctl_set_mode::protocol[%d],band[%d],bandwidth[%d]!}\r\n",
                            pst_mode_param->en_protocol, pst_mode_param->en_band, pst_mode_param->en_bandwidth);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_datarate(oal_net_device_stru *pst_net_dev, oal_int32 l_datarate)
{
    oal_uint8                   uc_prot_idx;
    oal_uint32                  ul_ret;
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_datarate:l_datarate[%d]", l_datarate);

    /*获取速率对应的字符，方便调用设置速率的相应接口*/
    for (uc_prot_idx = 0; uc_prot_idx < WAL_ATCMDSRV_IOCTL_DATARATE_NUM; uc_prot_idx++)
    {
        if (past_atcmdsrv_non_ht_rate_table[uc_prot_idx].ul_datarate == (oal_uint32)l_datarate)
        {
            break;
        }
    }
    /*根据设置的速率值来选择相应的函数，目前先只考虑非HT和VHT,装备只使用这种模式*/
    ul_ret = wal_hipriv_set_rate(pst_net_dev,(oal_int8 *)past_atcmdsrv_non_ht_rate_table[uc_prot_idx].puc_datarate);

    if (OAL_SUCC != ul_ret)
    {
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_bandwidth(oal_net_device_stru *pst_net_dev, oal_int32 l_bandwidth)
{
    wal_msg_write_stru          st_write_msg;
    mac_cfg_mode_param_stru    *pst_mode_param;

    oal_int32                   l_ret;


    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_bandwidth:l_bandwidth[%d]", l_bandwidth);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BANDWIDTH, OAL_SIZEOF(oal_int32));

    /*设置带宽时，模式不做修改，还是按照之前的值配置*/
    pst_mode_param = (mac_cfg_mode_param_stru *)(st_write_msg.auc_value);

    pst_mode_param->en_bandwidth = (oal_uint8)l_bandwidth;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;

}
OAL_STATIC oal_int32  wal_atcmsrv_ioctl_set_always_tx(oal_net_device_stru *pst_net_dev,oal_int32 l_always_tx)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_bcast_param;
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_always_tx:l_always_tx[%d]", l_always_tx);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_TX_1102, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_bcast_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 装备测试的情况下直接将长发参数设置好 */
    pst_set_bcast_param->en_payload_flag = RF_PAYLOAD_ALL_ONE;
    pst_set_bcast_param->ul_payload_len = WAL_ATCMDSRB_IOCTL_AL_TX_LEN;
    pst_set_bcast_param->uc_param = (oal_uint8)l_always_tx;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_always_tx::return err code [%d]!}", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_void wal_atcmdsrv_ioctl_convert_dbb_num(oal_uint32 ul_dbb_num,oal_uint8 *pc_dbb_num)
{
    oal_uint8  uc_temp          = 0;

    /* MAC H/w version register format                  */
    /* ------------------------------------------------ */
    /* | 31 - 24 | 23 - 16 | 15 - 12 | 11 - 0 | */
    /* ------------------------------------------------ */
    /* | BN      | Y1      | Y2      |   Y3   | */
    /* ------------------------------------------------ */

    /* Format the version as BN.Y1.Y2.Y3 with all values in hex i.e. the  */
    /* version string would be XX.XX.X.XXX.                                 */
    /* For e.g. 0225020A saved in the version register would translate to */
    /* the configuration interface version number 02.25.0.20A             */

    uc_temp = (ul_dbb_num & 0xF0000000) >> 28;
    pc_dbb_num[0] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    uc_temp = (ul_dbb_num & 0x0F000000) >> 24;
    pc_dbb_num[1] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);

    pc_dbb_num[2] = '.';

    uc_temp = (ul_dbb_num & 0x00F00000) >> 20;
    pc_dbb_num[3] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    uc_temp = (ul_dbb_num & 0x000F0000) >> 16;
    pc_dbb_num[4] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    pc_dbb_num[5] = '.';

    uc_temp = (ul_dbb_num & 0x0000F000) >> 12;
    pc_dbb_num[6] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    pc_dbb_num[7] = '.';

    uc_temp = (ul_dbb_num & 0x00000F00) >> 8;
    pc_dbb_num[8] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    uc_temp = (ul_dbb_num & 0x000000F0) >> 4;
    pc_dbb_num[9] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);
    uc_temp = (ul_dbb_num & 0x0000000F) >> 0;
    pc_dbb_num[10] = WAL_ATCMDSRV_GET_HEX_CHAR(uc_temp);


    return ;
}


OAL_STATIC oal_int32  wal_atcmsrv_ioctl_get_dbb_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_dbb_num)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int                         i_leftime;
    mac_vap_stru                          *pst_mac_vap;
    hmac_vap_stru                         *pst_hmac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,"{wal_atcmsrv_ioctl_get_dbb_num::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_VERSION, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::wal_send_cfg_event return err_code [%d]!}", l_ret);
        return l_ret;
    }
    /*阻塞等待dmac上报*/
    /*lint -e730*/
    i_leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q,(OAL_TRUE == pst_hmac_vap->st_atcmdsrv_get_status.uc_get_dbb_completed_flag),WAL_ATCMDSRB_DBB_NUM_TIME);
    /*lint +e730*/
    if ( 0 == i_leftime)
    {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::dbb_num wait for %ld ms timeout!}",
                         ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000)/OAL_TIME_HZ));
        return -OAL_EINVAL;
    }
    else if (i_leftime < 0)
    {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::dbb_num wait for %ld ms error!}",
                         ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000)/OAL_TIME_HZ));
        return -OAL_EINVAL;
    }
    else
    {
        /* 正常结束  */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::dbb_num wait for %ld ms error!}",
                      ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000)/OAL_TIME_HZ));
        if(0x0225020a != pst_hmac_vap->st_atcmdsrv_get_status.ul_dbb_num)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_dbb_num:ul_dbb_num[0x%x],not match 0x0225020a", pst_hmac_vap->st_atcmdsrv_get_status.ul_dbb_num);
            return -OAL_EINVAL;
        }
        wal_atcmdsrv_ioctl_convert_dbb_num(pst_hmac_vap->st_atcmdsrv_get_status.ul_dbb_num,(oal_uint8 *)pc_dbb_num);
        return OAL_SUCC;
    }


}
oal_uint64                      ul_gpio_wakeup_host_int_get_save;

OAL_STATIC oal_int32  wal_atcmsrv_ioctl_get_fem_pa_status(oal_net_device_stru *pst_net_dev, oal_int32 *pl_fem_pa_status)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int                         i_leftime;
    mac_vap_stru                   *pst_mac_vap;
    hmac_vap_stru                  *pst_hmac_vap;
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
    oal_uint64                      ul_gpio_wakeup_host_int_get;
    oal_uint32                      ul_check_gpio_wakeup_host_status = 0;
#endif
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_fem_pa_status::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,"{wal_atcmsrv_ioctl_get_fem_pa_status::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
    /*device唤醒host gpio引脚检测*/
    ul_gpio_wakeup_host_int_get = oal_get_gpio_int_count_para();

    if(ul_gpio_wakeup_host_int_get_save == ul_gpio_wakeup_host_int_get)
    {
        ul_check_gpio_wakeup_host_status = 1;

        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_fem_pa_status:check wl_host_wake_up gpio fail!");
    }
    ul_gpio_wakeup_host_int_get_save = ul_gpio_wakeup_host_int_get;
#endif
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHECK_FEM_PA, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_fem_pa_status::wal_send_cfg_event return err_code [%d]!}", l_ret);
        return l_ret;
    }
    /*阻塞等待dmac上报*/
    /*lint -e730*/
    i_leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q,(OAL_TRUE == pst_hmac_vap->st_atcmdsrv_get_status.uc_check_fem_pa_flag),WAL_ATCMDSRB_CHECK_FEM_PA);
    /*lint +e730*/

    if ( 0 == i_leftime)
    {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_fem_pa_status::dbb_num wait for %ld ms timeout!}",
                         ((WAL_ATCMDSRB_CHECK_FEM_PA * 1000)/OAL_TIME_HZ));
        return -OAL_EINVAL;
    }
    else if (i_leftime < 0)
    {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_fem_pa_status::dbb_num wait for %ld ms error!}",
                         ((WAL_ATCMDSRB_CHECK_FEM_PA * 1000)/OAL_TIME_HZ));
        return -OAL_EINVAL;
    }
    else
    {
        /* 正常结束  */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_fem_pa_status::dbb_num wait for %ld ms error!}",
                      ((WAL_ATCMDSRB_CHECK_FEM_PA * 1000)/OAL_TIME_HZ));
#if 0
        ul_gpio_wakeup_host_int_get = oal_get_gpio_int_count_para();

        if(ul_gpio_wakeup_host_int_get_save == ul_gpio_wakeup_host_int_get)
        {
            ul_check_gpio_wakeup_host_status = 1;

            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_fem_pa_status:check wl_host_wake_up gpio fail!");
        }
        ul_gpio_wakeup_host_int_get_save = ul_gpio_wakeup_host_int_get;
#endif
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
        *pl_fem_pa_status = (oal_int32)(pst_hmac_vap->st_atcmdsrv_get_status.ul_check_fem_pa_status|(ul_check_gpio_wakeup_host_status << 4));

        if(0 != pst_hmac_vap->st_atcmdsrv_get_status.ul_check_fem_pa_status)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_fem_pa_status:check fem_pa_status fail[*pl_fem_pa_status]!",*pl_fem_pa_status);
        }

#endif
        return OAL_SUCC;

    }


}


oal_int32 wal_atcmdsrv_wifi_priv_cmd(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd)
{
    wal_atcmdsrv_wifi_priv_cmd_stru  st_priv_cmd;
    oal_int32    l_ret              = 0;


    if ((OAL_PTR_NULL == pst_ifr->ifr_data)||(OAL_PTR_NULL == pst_net_dev))
    {
        l_ret = -OAL_EINVAL;
        return l_ret;
    }
    /*将用户态数据拷贝到内核态*/
    if (oal_copy_from_user(&st_priv_cmd, pst_ifr->ifr_data, sizeof(wal_android_wifi_priv_cmd_stru)))
    {
		l_ret = -OAL_EINVAL;
		return l_ret;
	}
    if(st_priv_cmd.l_verify != WAL_ATCMDSRV_IOCTL_VERIFY_CODE)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY,"wal_atcmdsrv_wifi_priv_cmd:ioctl verify failed,verify code is:%d(not equal %d)", st_priv_cmd.l_verify, WAL_ATCMDSRV_IOCTL_VERIFY_CODE);
        return -OAL_EINVAL;
    }
    switch(st_priv_cmd.ul_cmd)
    {
        case WAL_ATCMDSRV_IOCTL_CMD_WI_FREQ_SET:
             l_ret = wal_atcmsrv_ioctl_set_freq(pst_net_dev,st_priv_cmd.pri_data.l_freq);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_WI_POWER_SET:
             l_ret = wal_atcmsrv_ioctl_set_txpower(pst_net_dev,st_priv_cmd.pri_data.l_pow);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_MODE_SET:
             l_ret = wal_atcmsrv_ioctl_set_mode(pst_net_dev,st_priv_cmd.pri_data.l_mode);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_DATARATE_SET:
             l_ret = wal_atcmsrv_ioctl_set_datarate(pst_net_dev,st_priv_cmd.pri_data.l_datarate);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_BAND_SET:
             l_ret = wal_atcmsrv_ioctl_set_bandwidth(pst_net_dev,st_priv_cmd.pri_data.l_bandwidth);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_ALWAYS_TX_SET:
             l_ret = wal_atcmsrv_ioctl_set_always_tx(pst_net_dev,st_priv_cmd.pri_data.l_awalys_tx);
             break;
        case WAL_ATCMDSRV_IOCTL_CMD_DBB_GET:
             l_ret = wal_atcmsrv_ioctl_get_dbb_num(pst_net_dev,st_priv_cmd.pri_data.auc_dbb);
            if(oal_copy_to_user(pst_ifr->ifr_data,&st_priv_cmd,sizeof(wal_atcmdsrv_wifi_priv_cmd_stru)))
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY,"wal_atcmdsrv_wifi_priv_cmd:Failed to copy ioctl_data to user !");
                l_ret = -OAL_EINVAL;
            }
            break;
        case WAL_ATCMDSRV_IOCTL_CMD_FEM_PA_STATUS_GET:
            l_ret = wal_atcmsrv_ioctl_get_fem_pa_status(pst_net_dev,&st_priv_cmd.pri_data.l_fem_pa_status);
            if(oal_copy_to_user(pst_ifr->ifr_data,&st_priv_cmd,sizeof(wal_atcmdsrv_wifi_priv_cmd_stru)))
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY,"wal_atcmdsrv_wifi_priv_cmd:Failed to copy ioctl_data to user !");
                l_ret = -OAL_EINVAL;
            }
            break;
        default:
             break;
    }
    return l_ret;
}
#endif

/*****************************************************************************
 函 数 名  : wal_net_device_ioctl
 功能描述  : net device的ioctl函数
 输入参数  : net device指针
 输出参数  : 无
 返 回 值  : 统计结果指针
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月27日
    作    者   : 段开勇
    修改内容   : 新增函数

*****************************************************************************/
oal_int32 wal_net_device_ioctl(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd)
{
    oal_net_dev_ioctl_data_stru        *pst_ioctl_data;
    oal_int32                           l_ret   = 0;

    if ((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pst_ifr))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_net_device_ioctl::pst_dev %p, pst_ifr %p!}\r\n",
                       pst_net_dev, pst_ifr);
        return -OAL_EFAUL;
    }

    if (OAL_PTR_NULL == pst_ifr->ifr_data)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_net_device_ioctl::pst_ifr->ifr_data is NULL!}\r\n");
        return -OAL_EFAUL;
    }

    /* 1102 wpa_supplicant 通过ioctl 下发命令 */
    if (WAL_SIOCDEVPRIVATE+1 == ul_cmd)
    {
        l_ret = wal_android_priv_cmd(pst_net_dev, pst_ifr, ul_cmd);
        return l_ret;
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /*atcmdsrv 通过ioctl下发命令*/
        else if ( (WAL_SIOCDEVPRIVATE + 2) == ul_cmd )
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,"atcmdsrv_ioctl_cmd,cmd=0x%x", ul_cmd);
            l_ret = wal_atcmdsrv_wifi_priv_cmd(pst_net_dev, pst_ifr, ul_cmd);
            return l_ret;
        }
#endif

    pst_ioctl_data = (oal_net_dev_ioctl_data_stru *)pst_ifr->ifr_data;

    if (WAL_SIOCDEVPRIVATE != ul_cmd)
    {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_net_device_ioctl::unrecognised cmd, %d!}\r\n", ul_cmd);
        return OAL_SUCC;
    }

    switch(pst_ioctl_data->l_cmd)
    {
        case HWIFI_IOCTL_CMD_GET_STA_ASSOC_REQ_IE:
        {
            l_ret = wal_ioctl_get_assoc_req_ie(pst_net_dev, pst_ioctl_data);

            break;
        }
        case HWIFI_IOCTL_CMD_SET_AP_AUTH_ALG:
        {
            l_ret = wal_ioctl_set_auth_mode(pst_net_dev, pst_ioctl_data);

            break;
        }
        case HWIFI_IOCTL_CMD_SET_COUNTRY:
        {
            l_ret = wal_ioctl_set_country_code(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_WPS_IE:
        {
            l_ret = wal_ioctl_set_wps_ie(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_SSID:
        {
            l_ret = wal_ioctl_set_ssid(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_MAX_USER:
        {
            l_ret = wal_ioctl_set_max_user(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_PRIV_CONNECT:
        {
            l_ret = wal_ioctl_nl80211_priv_connect(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_PRIV_DISCONNECT:
        {
            l_ret = wal_ioctl_nl80211_priv_disconnect(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_FREQ:
        {
            l_ret = wal_ioctl_set_channel(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_FRAG:
        {
            l_ret = wal_ioctl_set_frag(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_RTS:
        {
            l_ret = wal_ioctl_set_rts(pst_net_dev, pst_ioctl_data);
            break;
        }

        default:
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_net_device_ioctl::no matched CMD !}\r\n");
            l_ret = -OAL_EFAIL;
            break;
    }

    return (l_ret);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
/* 1102 不使用iwconfig iwpriv ，采用hipriv 接口 */

OAL_STATIC oal_uint32  wal_ioctl_get_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}
OAL_STATIC oal_uint32  wal_ioctl_set_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                    ac_mode_str[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};     /* 预留协议模式字符串空间 */
    oal_uint8                   uc_prot_idx;
    mac_cfg_mode_param_stru    *pst_mode_param;
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_ioctl_set_mode::pst_net_dev/p_param null ptr error %d!}\r\n", pst_net_dev, pc_param);
        return -OAL_EFAUL;
    }

#if 0
    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }
#endif
    /* pc_param指向传入模式参数, 将其取出存放到ac_mode_str中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_mode_str, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    ac_mode_str[OAL_SIZEOF(ac_mode_str) - 1] = '\0';    /* 确保以null结尾 */

    for (uc_prot_idx = 0; OAL_PTR_NULL != g_ast_mode_map[uc_prot_idx].pc_name; uc_prot_idx++)
    {
        l_ret = oal_strcmp(g_ast_mode_map[uc_prot_idx].pc_name, ac_mode_str);

        if (0 == l_ret)
        {
            break;
        }
    }

    if (OAL_PTR_NULL == g_ast_mode_map[uc_prot_idx].pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mode::unrecognized protocol string!}\r\n");
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru));

    pst_mode_param = (mac_cfg_mode_param_stru *)(st_write_msg.auc_value);
    pst_mode_param->en_protocol  = g_ast_mode_map[uc_prot_idx].en_mode;
    pst_mode_param->en_band      = g_ast_mode_map[uc_prot_idx].en_band;
    pst_mode_param->en_bandwidth = g_ast_mode_map[uc_prot_idx].en_bandwidth;

    OAM_INFO_LOG3(0, OAM_SF_CFG, "{wal_ioctl_set_mode::protocol[%d],band[%d],bandwidth[%d]!}\r\n",
                            pst_mode_param->en_protocol, pst_mode_param->en_band, pst_mode_param->en_bandwidth);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}
OAL_STATIC oal_uint32  wal_ioctl_set_essid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint8                       uc_ssid_len;
    oal_int32                       l_ret;
    wal_msg_write_stru              st_write_msg;
    mac_cfg_ssid_param_stru        *pst_param;
    mac_vap_stru                   *pst_mac_vap;
    oal_uint32                      ul_off_set;
    oal_int8                       *pc_ssid;
    oal_int8                        ac_ssid[WLAN_SSID_MAX_LEN] = {0};
    oal_uint32                      ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_essid::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        /* 设备在up状态且是AP时，不允许配置，必须先down */
        if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid::device is busy, please down it firste %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
            return -OAL_EBUSY;
        }
    }

    /* pc_param指向传入模式参数, 将其取出存放到ac_mode_str中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_ssid, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_essid::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_ssid       = ac_ssid;
    pc_ssid       = oal_strim(ac_ssid);                   /* 去掉字符串开始结尾的空格 */
    uc_ssid_len = (oal_uint8)OAL_STRLEN(pc_ssid);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: ssid length %d!}\r\n", uc_ssid_len);

    if (uc_ssid_len > WLAN_SSID_MAX_LEN - 1)        /* -1为\0预留空间 */
    {
        uc_ssid_len = WLAN_SSID_MAX_LEN - 1;
    }

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: ssid length is %d!}\r\n", uc_ssid_len);
    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SSID, OAL_SIZEOF(mac_cfg_ssid_param_stru));

    /* 填写WID对应的参数 */
    pst_param = (mac_cfg_ssid_param_stru *)(st_write_msg.auc_value);
    pst_param->uc_ssid_len = uc_ssid_len;
    oal_memcopy(pst_param->ac_ssid, pc_ssid, uc_ssid_len);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ssid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_ioctl_get_bss_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}
OAL_STATIC oal_uint32  wal_ioctl_set_bss_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}
OAL_STATIC oal_uint32  wal_ioctl_get_freq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}


OAL_STATIC oal_uint32  wal_ioctl_set_freq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_freq;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_freq[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;

#if 0
    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::device is busy, please down it firs %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }
#endif
    /* pc_param指向新创建的net_device的name, 将其取出存放到ac_name中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_freq, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    l_freq = oal_atoi(ac_freq);
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::l_freq = %d!}\r\n", l_freq);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CURRENT_CHANEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_freq;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_ioctl_get_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}
OAL_STATIC oal_uint32  wal_ioctl_set_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return OAL_FAIL;
}


OAL_STATIC oal_uint32  wal_ioctl_set_beacon_interval(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_beacon_interval;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_beacon_interval[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::device is busy, please down it firs %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    /* pc_param指向新创建的net_device的name, 将其取出存放到ac_name中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_beacon_interval, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    l_beacon_interval = oal_atoi(ac_beacon_interval);
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_beacon_interval::l_beacon_interval = %d!}\r\n", l_beacon_interval);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BEACON_INTERVAL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_beacon_interval;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_start_vap
 功能描述  : 私有up命令，代替ifconfig up。3sta场景时，第三个sta无法up，可用此命令up
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月6日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_start_vap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    OAM_ERROR_LOG0(0, OAM_SF_CFG, "DEBUG:: priv start enter.");

    wal_netdev_open(pst_net_dev);

    return OAL_SUCC;
}

#else
/*****************************************************************************
 函 数 名  : wal_ioctl_get_mode
 功能描述  : 读取vap模式: 包括协议 频点 带宽
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月18日
    作    者   : zhangheng
    修改内容   : 新生成函数
  2.日    期   : 2013年6月9日
    作    者   : zhangyu
    修改内容   : 统一发送消息接口函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_mode(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_param, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_query_stru              st_query_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    oal_iw_point_stru              *pst_point;
    oal_uint8                       uc_prot_idx;
    mac_cfg_mode_param_stru        *pst_mode_param;
    oal_int8                       *pc_err_str = "Error protocal";
    mac_vap_stru                   *pst_mac_vap;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/

    st_query_msg.en_wid = WLAN_CFGID_MODE;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_mode::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pst_point = (oal_iw_point_stru *)p_param;

    pst_mode_param = (mac_cfg_mode_param_stru *)(pst_query_rsp_msg->auc_value);

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_mode::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        for (uc_prot_idx = 0; OAL_PTR_NULL != g_ast_mode_map[uc_prot_idx].pc_name; uc_prot_idx++)
        {
            if ((g_ast_mode_map[uc_prot_idx].en_mode == pst_mode_param->en_protocol) &&
                (g_ast_mode_map[uc_prot_idx].en_band == pst_mode_param->en_band) &&
                (g_ast_mode_map[uc_prot_idx].en_bandwidth == pst_mode_param->en_bandwidth))
            {
                break;
            }
        }
    }
    /* STA模式下频段和频段在和AP关联之后自适应，此处仅比较协议模式 */
    else if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        for (uc_prot_idx = 0; OAL_PTR_NULL != g_ast_mode_map[uc_prot_idx].pc_name; uc_prot_idx++)
        {
            if (g_ast_mode_map[uc_prot_idx].en_mode == pst_mode_param->en_protocol)
            {
                break;
            }
        }
    }
    else
    {
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == g_ast_mode_map[uc_prot_idx].pc_name)
    {
        pst_point->length = (oal_uint16)OAL_STRLEN(pc_err_str);
        oal_memcopy(pc_extra, pc_err_str, pst_point->length);
        return OAL_SUCC;
    }

    pst_point->length = (oal_uint16)OAL_STRLEN(g_ast_mode_map[uc_prot_idx].pc_name);
    oal_memcopy(pc_extra, g_ast_mode_map[uc_prot_idx].pc_name, pst_point->length);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_mode
 功能描述  : 设置模式: 包括协议、频段、带宽
 输入参数  : pst_net_dev: net device
             p_param    : 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32 wal_ioctl_set_mode(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, void *p_param, oal_int8 *pc_extra)
{
    oal_iw_point_stru          *pst_param;
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    oal_int8                    ac_mode_str[24] = {0};     /* 预留协议模式字符串空间 */
    oal_uint8                   uc_prot_idx;
    mac_cfg_mode_param_stru    *pst_mode_param;
    wal_msg_write_stru          st_write_msg;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == p_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_ioctl_set_mode::pst_net_dev/p_param null ptr error %d!}\r\n", pst_net_dev, p_param);
        return -OAL_EFAUL;
    }

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }
    pst_param = (oal_iw_point_stru *)p_param;
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::input str length is %d!}\r\n", pst_param->length);

    if (pst_param->length > OAL_SIZEOF(ac_mode_str))    /* 字符串切断, 长度包括\0 */
    {
        pst_param->length =  OAL_SIZEOF(ac_mode_str);
    }

    ul_ret = oal_copy_from_user(ac_mode_str, pst_param->pointer, pst_param->length);

    /* copy_from_user函数的目的是从用户空间拷贝数据到内核空间，失败返回没有被拷贝的字节数，成功返回0 */
    if (ul_ret > 0)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::oal_copy_from_user return error code %d!}\r\n", ul_ret);
        return -OAL_EFAUL;
    }

    ac_mode_str[OAL_SIZEOF(ac_mode_str) - 1] = '\0';    /* 确保以null结尾 */

    for (uc_prot_idx = 0; OAL_PTR_NULL != g_ast_mode_map[uc_prot_idx].pc_name; uc_prot_idx++)
    {
        l_ret = oal_strcmp(g_ast_mode_map[uc_prot_idx].pc_name, ac_mode_str);

        if (0 == l_ret)
        {
            break;
        }
    }

    if (OAL_PTR_NULL == g_ast_mode_map[uc_prot_idx].pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mode::unrecognized protocol string!}\r\n");
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru));

    pst_mode_param = (mac_cfg_mode_param_stru *)(st_write_msg.auc_value);
    pst_mode_param->en_protocol  = g_ast_mode_map[uc_prot_idx].en_mode;
    pst_mode_param->en_band      = g_ast_mode_map[uc_prot_idx].en_band;
    pst_mode_param->en_bandwidth = g_ast_mode_map[uc_prot_idx].en_bandwidth;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mode::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_essid
 功能描述  : 设置ssid
 输入参数  : pst_net_dev: net device
             pst_info   : iw info
             pst_data   : ioctl payload
             pc_ssid    : ssid
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月14日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_essid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_point_stru *pst_data, oal_int8 *pc_ssid)
{
    oal_uint8                       uc_ssid_len;
    oal_int32                       l_ret;
    wal_msg_write_stru              st_write_msg;
    mac_cfg_ssid_param_stru        *pst_param;
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_essid::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        /* 设备在up状态且是AP时，不允许配置，必须先down */
        if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid::device is busy, please down it firste %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
            return -OAL_EBUSY;
        }
    }

    pc_ssid = oal_strim(pc_ssid);                   /* 去掉字符串开始结尾的空格 */

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: pst_data->flags, pst_data->lengt %d, %d!}\r\n", pst_data->flags, pst_data->length);

    //uc_ssid_len = (oal_uint8)pst_data->length;    /* 长度不包括 \0 */
    uc_ssid_len = (oal_uint8)OAL_STRLEN(pc_ssid);

    if (uc_ssid_len > WLAN_SSID_MAX_LEN - 1)        /* -1为\0预留空间 */
    {
        uc_ssid_len = WLAN_SSID_MAX_LEN - 1;
    }

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: ssid length is %d!}\r\n", uc_ssid_len);
    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SSID, OAL_SIZEOF(mac_cfg_ssid_param_stru));

    /* 填写WID对应的参数 */
    pst_param = (mac_cfg_ssid_param_stru *)(st_write_msg.auc_value);
    pst_param->uc_ssid_len = uc_ssid_len;
    oal_memcopy(pst_param->ac_ssid, pc_ssid, uc_ssid_len);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ssid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_essid:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_octl_get_essid
 功能描述  : 设置ssid
 输入参数  : pst_net_dev: net device
             pst_info   : iw info
             pst_data   : ioctl payload
             pc_ssid    : ssid
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月14日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_point_stru *pst_data, oal_int8 *pc_ssid)
{
    oal_int32                       l_ret;
    wal_msg_query_stru              st_query_msg;
    mac_cfg_ssid_param_stru        *pst_ssid;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru               *pst_query_rsp_msg;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_SSID;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_essid:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    /* 业务处理 */
    pst_ssid = (mac_cfg_ssid_param_stru *)(pst_query_rsp_msg->auc_value);

    pst_data->flags = 1;    /* 设置出参标志为有效 */
    pst_data->length = pst_ssid->uc_ssid_len;

    oal_memcopy(pc_ssid, pst_ssid->ac_ssid, pst_ssid->uc_ssid_len);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_bss_type
 功能描述  : get bss type
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_bss_type(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_uint32 *pul_type, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_query_stru              st_query_msg;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    oal_uint32                      ul_type;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_BSS_TYPE;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_bss_type::wal_ioctl_get_bss_type return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    ul_type = *((oal_uint32 *)pst_query_rsp_msg->auc_value);

    *pul_type = OAL_IW_MODE_AUTO;

    if (WLAN_MIB_DESIRED_BSSTYPE_INFRA == ul_type)
    {
        *pul_type = OAL_IW_MODE_INFRA;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_bss_type
 功能描述  : set bss type
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_bss_type(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_uint32 *pul_type, oal_int8 *pc_extra)
{
    oal_uint32                      ul_type;
    oal_int32                       l_ret;
    wal_msg_write_stru              st_write_msg;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_bss_type::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    ul_type = *pul_type;

    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_bss_type::type value is %d!}\r\n", ul_type);

    if (ul_type < WLAN_MIB_DESIRED_BSSTYPE_INFRA || ul_type >= WLAN_MIB_DESIRED_BSSTYPE_BUTT)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_bss_type::input type is invalid!}\r\n");
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BSS_TYPE, OAL_SIZEOF(ul_type));
    *((oal_uint32 *)st_write_msg.auc_value) = ul_type;    /* 填写WID对应的参数 */

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_type),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_bss_type::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_freq
 功能描述  : 获取频点/信道
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_freq(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_freq_stru *pst_freq, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_query_stru              st_query_msg;
    wal_msg_rsp_stru               *pst_queue_rsp_msg;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_CURRENT_CHANEL;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_freq::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_queue_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pst_freq->m = *((oal_int32 *)(pst_queue_rsp_msg->auc_value));
    pst_freq->e = 0;

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_freq
 功能描述  : 设置频点/信道
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_freq(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_freq_stru *pst_freq, oal_int8 *pc_extra)
{
    oal_int32                   l_ret;
    wal_msg_write_stru          st_write_msg;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::device is busy, please down it firs %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    OAM_INFO_LOG4(0, OAM_SF_ANY, "{wal_ioctl_set_freq::pst_freq: m = %u, e = %u, i = %u, flags = %u!}\r\n",
                 (oal_uint32)pst_freq->m, (oal_uint16)pst_freq->e, pst_freq->i, pst_freq->flags);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CURRENT_CHANEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = pst_freq->m;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_freq::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_txpower
 功能描述  : 获得传输功率
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_txpower(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_param_stru *pst_param, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_query_stru              st_query_msg;
    wal_msg_rsp_stru               *pst_query_rsp_msg = OAL_PTR_NULL;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_TX_POWER;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_txpower::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息*/
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pst_param->value    = *((oal_int32 *)(pst_query_rsp_msg->auc_value));
    pst_param->fixed    = 1;
    pst_param->disabled = 0;
    pst_param->flags    = OAL_IW_TXPOW_DBM;

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_txpower
 功能描述  : 设置传输功率
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_txpower(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iw_param_stru *pst_param, oal_int8 *pc_extra)
{
    oal_int32                   l_ret;
    wal_msg_write_stru          st_write_msg;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_txpower::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    OAM_INFO_LOG4(0, OAM_SF_ANY, "{wal_ioctl_set_txpower::pst_param: value= %d, fixed = %d, disabled = %d, flags = %d!}\r\n",
                 pst_param->value, pst_param->fixed, pst_param->disabled, pst_param->flags);

    if (pst_param->flags != OAL_IW_TXPOW_DBM)       /* 如果参数输入类型不是dBm，则返回错误 */
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_txpower::invalid argument!}\r\n");
        return -OAL_EINVAL;
    }

    if (pst_param->value > WLAN_MAX_TXPOWER || pst_param->value < 0)   /* 参数异常: 功率限制大于1W */
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_txpower::invalid argument!}\r\n");
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_POWER, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = pst_param->value;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_txpower::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44)) *//* 1102 不使用iwconfig ，采用hipriv 接口 */

/*****************************************************************************
 函 数 名  : wal_ioctl_get_apaddr
 功能描述  : 获取BSSID
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_apaddr(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_sockaddr_stru           *pst_addr,
                oal_int8                    *pc_extra)
{
    mac_vap_stru   *pst_mac_vap;
    oal_uint8       auc_zero_addr[WLAN_MAC_ADDR_LEN] = {0};

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_apaddr::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
    {
        oal_set_mac_addr((oal_uint8 *)pst_addr->sa_data, pst_mac_vap->auc_bssid);
    }
    else
    {
        oal_set_mac_addr((oal_uint8 *)pst_addr->sa_data, auc_zero_addr);
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_iwrate
 功能描述  : iwconfig获取rate，不支持，返回-1
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_iwrate(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra)
{
    /* iwconfig获取rate，不支持此命令，则返回-1 */

    return -1;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_iwrate
 功能描述  : iwconfig获取sense，不支持，返回-1
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_iwsense(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra)
{
    /* iwconfig获取sense，不支持此命令，则返回-1 */

    return -1;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_rtsthres
 功能描述  : iwconfig获取rtsthres
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_rtsthres(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra)
{
    mac_vap_stru   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_rtsthres::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param->value    = (oal_int32)pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11RTSThreshold;
    pst_param->disabled = (WLAN_RTS_MAX == pst_param->value);
    pst_param->fixed    = 1;

    return 0;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_fragthres
 功能描述  : iwconfig获取fragthres
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_fragthres(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_param_stru           *pst_param,
                oal_int8                    *pc_extra)
{
    mac_vap_stru   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_fragthres::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param->value    = (oal_int32)pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11FragmentationThreshold;
    pst_param->disabled = (WLAN_FRAG_THRESHOLD_MAX == pst_param->value);
    pst_param->fixed    = 1;

    return 0;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_iwencode
 功能描述  : iwconfig获取encode, 不支持
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月22日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_iwencode(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_point_stru           *pst_param,
                oal_int8                    *pc_extra)
{
    /* 不支持iwconfig获取encode，直接返回-1 */

    return -1;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_iwrange
 功能描述  : iwconfig获取iwrange, 不支持
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_iwrange(
                oal_net_device_stru         *pst_net_dev,
                oal_iw_request_info_stru    *pst_info,
                oal_iw_point_stru           *pst_param,
                oal_int8                    *pc_extra)
{

    return -1;
}


/*****************************************************************************
 函 数 名  : wal_ioctl_get_param
 功能描述  : iwpriv私有获取参数命令入口
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_param(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_query_stru              st_query_msg;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    oal_int32                      *pl_param;

    pl_param = (oal_int32 *)pc_extra;
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_param::return err code %d!}\r\n", pl_param[0]);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = (oal_uint16)pl_param[0];

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_param::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pl_param[0] = *((oal_int32 *)(pst_query_rsp_msg->auc_value));

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_param
 功能描述  : iwpriv私有设置参数命令入口
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_param(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra)
{
    oal_int32                       l_error = 0;
    oal_int32                       l_ret;
    oal_int32                      *pl_param;
    oal_int32                       l_subioctl_id;
    oal_int32                       l_value;
    wal_msg_write_stru              st_write_msg;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_param::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    pl_param      = (oal_int32 *)pc_extra;
    l_subioctl_id = pl_param[0];    /* 获取sub-ioctl的ID */
    l_value       = pl_param[1];    /* 获取要设置的值 */
    OAM_INFO_LOG2(0, OAM_SF_ANY, "{wal_ioctl_set_param::the subioctl_id and value is %d, %d!}\r\n", l_subioctl_id, l_value);

    if (l_value < 0)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_param::input value is negative %d!}\r\n", l_value);
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, (oal_uint16)l_subioctl_id, OAL_SIZEOF(oal_int32));

    switch (l_subioctl_id)                                                      /* 根据sub-ioctl id填写WID */
    {
        case WLAN_CFGID_PROT_MODE:
            if (l_value >= WLAN_PROT_BUTT)  /* 参数检查 */
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_AUTH_MODE:
            if (l_value >= WLAN_WITP_ALG_AUTH_BUTT)  /* 参数检查 */
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_BEACON_INTERVAL:
            if (l_value > WLAN_BEACON_INTVAL_MAX || l_value < WLAN_BEACON_INTVAL_MIN)
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_TX_CHAIN:
            if (l_value > 0xF)
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_RX_CHAIN:
            if (l_value > 0xF)
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_CONCURRENT:
            if (l_value > WLAN_MAX_ASSOC_USER_CFG || l_value < 1)  /* 参数检查，最大关联用户数1~200 */
            {
                l_error = -OAL_EINVAL;
            }

            break;

        case WLAN_CFGID_DTIM_PERIOD:
            if (l_value > WLAN_DTIM_PERIOD_MAX || l_value < WLAN_DTIM_PERIOD_MIN)
            {
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_param::input dtim_period invalid %d!}\r\n", l_value);
                l_error = -OAL_EINVAL;
            }

            break;

        default:
            break;
    }

    if (0 != l_error)           /* 参数异常 */
    {
        return l_error;
    }

    *((oal_int32 *)(st_write_msg.auc_value)) = l_value;   /* 填写set消息的payload */

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_param::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}



/*****************************************************************************
 函 数 名  : wal_ioctl_set_wme_params
 功能描述  : iwpriv私有设置参数命令入口
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月9日
    作    者   : 康国昌
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_wme_params(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra)
{
    oal_int32                       l_error = 0;
    oal_int32                       l_ret;
    oal_int32                      *pl_param;
    oal_int32                       l_subioctl_id;
    oal_int32                       l_ac;
    oal_int32                       l_value;
    wal_msg_write_stru              st_write_msg;
    wal_msg_wmm_stru               *pst_wmm_params;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_wme_params::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    pl_param      = (oal_int32 *)pc_extra;
    l_subioctl_id = pl_param[0];    /* 获取sub-ioctl的ID */
    l_ac          = pl_param[1];
    l_value       = pl_param[2];    /* 获取要设置的值 */

    OAM_INFO_LOG3(0, OAM_SF_ANY, "{wal_ioctl_set_wme_params::the subioctl_id,l_ac,value is %d, %d, %d!}\r\n", l_subioctl_id, l_ac, l_value);

    /* ac取值0~3, value不能为负值 */
    if ((l_value < 0) || (l_ac < 0) || (l_ac >= WLAN_WME_AC_BUTT))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_ioctl_set_wme_params::input value is negative %d, %d!}\r\n", l_value, l_ac);
        return -OAL_EINVAL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    switch (l_subioctl_id)                                                      /* 根据sub-ioctl id填写WID */
    {
        case WLAN_CFGID_EDCA_TABLE_CWMIN:
            if ((l_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (l_value < WLAN_QEDCA_TABLE_CWMIN_MIN))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_EDCA_TABLE_CWMAX:
            if ((l_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (l_value < WLAN_QEDCA_TABLE_CWMAX_MIN))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_EDCA_TABLE_AIFSN:
            if ((l_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (l_value > WLAN_QEDCA_TABLE_AIFSN_MAX))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT:
            if ((l_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX) || (l_value < WLAN_QEDCA_TABLE_TXOP_LIMIT_MIN))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME:
            if (l_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX)
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_EDCA_TABLE_MANDATORY:
            if ((OAL_TRUE != l_value) &&  (OAL_FALSE != l_value))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMIN:
            if ((l_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (l_value < WLAN_QEDCA_TABLE_CWMIN_MIN))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMAX:
            if ((l_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (l_value < WLAN_QEDCA_TABLE_CWMAX_MIN))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_AIFSN:
            if ((l_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (l_value > WLAN_QEDCA_TABLE_AIFSN_MAX))
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT:
            if (l_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX)
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME:
            if (l_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX)
            {
                l_error = -OAL_EINVAL;
            }
            break;

        case WLAN_CFGID_QEDCA_TABLE_MANDATORY:
            if ((OAL_TRUE != l_value) &&  (OAL_FALSE != l_value))
            {
                l_error = -OAL_EINVAL;
            }

            break;

        default:
            break;
    }

    if (0 != l_error)           /* 参数异常 */
    {
        return l_error;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, (oal_uint16)l_subioctl_id, OAL_SIZEOF(wal_msg_wmm_stru));

    pst_wmm_params               = (wal_msg_wmm_stru *)(st_write_msg.auc_value);
    pst_wmm_params->en_cfg_id    = (oal_uint16)l_subioctl_id;
    pst_wmm_params->ul_ac        = (oal_uint32)l_ac;                     /* 填写set消息的payload */
    pst_wmm_params->ul_value     = (oal_uint32)l_value;                  /* 填写set消息的payload */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(wal_msg_wmm_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_wme_params::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_ioctl_get_wme_params
 功能描述  : iwpriv私有获取参数命令入口
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月9日
    作    者   : 康国昌
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_wme_params(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_iw, oal_int8 *pc_extra)
{
    oal_int *param;

    param = (oal_int *)pc_extra;

    param[0] = (oal_int)wal_config_get_wmm_params(pst_net_dev, (oal_uint8 *)pc_extra);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11D
/*****************************************************************************
 函 数 名  : wal_is_alpha_upper
 功能描述  : 判断是否是大写字母
 输入参数  : c_letter: 国家字符串字母
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_bool_enum_uint8  wal_is_alpha_upper(oal_int8 c_letter)
{
    if (c_letter >= 'A' && c_letter <= 'Z')
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}

/*****************************************************************************
 函 数 名  : wal_regdomain_get_band
 功能描述  : 获取一个管制类的起始频带
 输入参数  : ul_start_freq: 起始频率
             ul_end_freq  : 结束频率
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint8  wal_regdomain_get_band(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    if (ul_start_freq > 2400 && ul_end_freq < 2500)
    {
        return MAC_RC_START_FREQ_2;
    }
    else if (ul_start_freq > 5000 && ul_end_freq < 5870)
    {
        return MAC_RC_START_FREQ_5;
    }
    else if (ul_start_freq > 4900 && ul_end_freq < 4999)
    {
        return MAC_RC_START_FREQ_5;
    }
    else
    {
        return MAC_RC_START_FREQ_BUTT;
    }
}

/*****************************************************************************
 函 数 名  : wal_regdomain_get_bw
 功能描述  : 获取一个管制类的带宽
 输入参数  : uc_bw: linux管制类中的带宽值
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint8  wal_regdomain_get_bw(oal_uint8 uc_bw)
{
    oal_uint8 uc_bw_map;

    switch (uc_bw)
    {
        case 40:
                uc_bw_map = MAC_CH_SPACING_40MHZ;
                break;
        case 20:
                uc_bw_map = MAC_CH_SPACING_20MHZ;
                break;
        default:
                uc_bw_map = MAC_CH_SPACING_BUTT;
                break;
    };

    return uc_bw_map;
}

/*****************************************************************************
 函 数 名  : wal_regdomain_get_channel_2g
 功能描述  : 获取管制类信道位图，信道在2g频段上
 输入参数  : ul_start_freq: 起始频率
             ul_end_freq  : 结束频率
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_regdomain_get_channel_2g(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_freq;
    oal_uint32 ul_i;
    oal_uint32 ul_ch_bmap = 0;

    for (ul_freq = ul_start_freq + 10; ul_freq <= (ul_end_freq - 10); ul_freq++)
    {
        for (ul_i = 0; ul_i < MAC_CHANNEL_FREQ_2_BUTT; ul_i++)
        {
            if (ul_freq == g_ast_freq_map_2g[ul_i].us_freq)
            {
                ul_ch_bmap |= (1 << ul_i);
            }
        }
    }

    return ul_ch_bmap;
}

/*****************************************************************************
 函 数 名  : wal_regdomain_get_channel_5g
 功能描述  : 获取管制类信道位图，信道在5g频段上
 输入参数  : ul_start_freq: 起始频率
             ul_end_freq  : 结束频率
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_regdomain_get_channel_5g(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_freq;
    oal_uint32 ul_i;
    oal_uint32 ul_ch_bmap = 0;

    for (ul_freq = ul_start_freq + 10; ul_freq <= (ul_end_freq - 10); ul_freq += 5)
    {
        for (ul_i = 0; ul_i < MAC_CHANNEL_FREQ_5_BUTT; ul_i++)
        {
            if (ul_freq == g_ast_freq_map_5g[ul_i].us_freq)
            {
                ul_ch_bmap |= (1 << ul_i);
            }
        }
    }

    return ul_ch_bmap;

}

/*****************************************************************************
 函 数 名  : wal_regdomain_get_channel
 功能描述  : 获取1个管制类的信道位图
 输入参数  : uc_band      : 频段
             ul_start_freq: 起始频率
             ul_end_freq  : 中止频率
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_regdomain_get_channel(oal_uint8 uc_band, oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_ch_bmap = 0;;

    switch (uc_band)
    {
        case MAC_RC_START_FREQ_2:
            ul_ch_bmap = wal_regdomain_get_channel_2g(ul_start_freq, ul_end_freq);
            break;

        case MAC_RC_START_FREQ_5:
            ul_ch_bmap = wal_regdomain_get_channel_5g(ul_start_freq, ul_end_freq);
            break;

        default:
            break;
    }

    return ul_ch_bmap;
}

/*****************************************************************************
 函 数 名  : wal_get_dfs_domain
 功能描述  : 根据国家码，获取对应的雷达检测标准
 输入参数  : pst_mac_regdom: 管制域指针
             pc_country    : 国家码
 输出参数  : pst_mac_regdom: 管制域指针
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月31日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  wal_get_dfs_domain(mac_regdomain_info_stru *pst_mac_regdom, OAL_CONST oal_int8 *pc_country)
{
    oal_uint32    u_idx;
    oal_uint32    ul_size = OAL_ARRAY_SIZE(g_ast_dfs_domain_table);

    for (u_idx = 0; u_idx < ul_size; u_idx++)
    {
        if (0 == oal_strcmp(g_ast_dfs_domain_table[u_idx].pc_country, pc_country))
        {
            pst_mac_regdom->en_dfs_domain = g_ast_dfs_domain_table[u_idx].en_dfs_domain;

            return;
        }
    }

    pst_mac_regdom->en_dfs_domain = MAC_DFS_DOMAIN_NULL;
}

/*****************************************************************************
 函 数 名  : wal_regdomain_fill_info
 功能描述  : 填充管制下发的管制域信息
 输入参数  : pst_regdom    : 指向linux的管制域信息
             pst_mac_regdom: 指向要下发的管制域信息
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_void  wal_regdomain_fill_info(OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom, mac_regdomain_info_stru *pst_mac_regdom)
{
    oal_uint32  ul_i;
    oal_uint32  ul_start;
    oal_uint32  ul_end;
    oal_uint8   uc_band;
    oal_uint8   uc_bw;

    /* 复制国家字符串 */
    pst_mac_regdom->ac_country[0] = pst_regdom->alpha2[0];
    pst_mac_regdom->ac_country[1] = pst_regdom->alpha2[1];
    pst_mac_regdom->ac_country[2] = 0;

    /* 获取DFS认证标准类型 */
    wal_get_dfs_domain(pst_mac_regdom, pst_regdom->alpha2);

    /* 填充管制类个数 */
    pst_mac_regdom->uc_regclass_num = (oal_uint8)pst_regdom->n_reg_rules;

    /* 填充管制类信息 */
    for (ul_i = 0; ul_i < pst_regdom->n_reg_rules; ul_i++)
    {
        /* 填写管制类的频段(2.4G或5G) */
        ul_start = pst_regdom->reg_rules[ul_i].freq_range.start_freq_khz / 1000;
        ul_end   = pst_regdom->reg_rules[ul_i].freq_range.end_freq_khz / 1000;
        uc_band  = wal_regdomain_get_band(ul_start, ul_end);
        pst_mac_regdom->ast_regclass[ul_i].en_start_freq = uc_band;

        /* 填写管制类允许的最大带宽 */
        uc_bw = (oal_uint8)(pst_regdom->reg_rules[ul_i].freq_range.max_bandwidth_khz / 1000);
        pst_mac_regdom->ast_regclass[ul_i].en_ch_spacing = wal_regdomain_get_bw(uc_bw);

        /* 填写管制类信道位图 */
        pst_mac_regdom->ast_regclass[ul_i].ul_channel_bmap = wal_regdomain_get_channel(uc_band, ul_start, ul_end);

        /* 标记管制类行为 */
        pst_mac_regdom->ast_regclass[ul_i].uc_behaviour_bmap = 0;

        if (pst_regdom->reg_rules[ul_i].flags & NL80211_RRF_DFS)
        {
            pst_mac_regdom->ast_regclass[ul_i].uc_behaviour_bmap |= MAC_RC_DFS;
        }

        /* 填充覆盖类和最大发送功率 */
        pst_mac_regdom->ast_regclass[ul_i].uc_coverage_class = 0;
        pst_mac_regdom->ast_regclass[ul_i].uc_max_reg_tx_pwr = (oal_uint8)(pst_regdom->reg_rules[ul_i].power_rule.max_eirp / 100);
        pst_mac_regdom->ast_regclass[ul_i].uc_max_tx_pwr     = (oal_uint8)(pst_regdom->reg_rules[ul_i].power_rule.max_eirp / 100);

    }
}

/*****************************************************************************
 函 数 名  : wal_regdomain_update
 功能描述  : 下发配置管制域信息
 输入参数  : pst_net_dev: net_device
             pc_country : 要设置的国家字符串
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月22日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_regdomain_update(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country)
{
    OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom;
    oal_uint16                              us_size;
    mac_regdomain_info_stru                *pst_mac_regdom;
    wal_msg_write_stru                      st_write_msg;
    mac_cfg_country_stru                   *pst_param;
    oal_int32                               l_ret;
    oal_int8                               *pc_current_country;

    if (!wal_is_alpha_upper(pc_country[0]) || !wal_is_alpha_upper(pc_country[1]))
    {
        if (('9' == pc_country[0]) && ('9' == pc_country[1]))
        {
            OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::set regdomain to 99!}\r\n");
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::country str is invalid!}\r\n");
            return -OAL_EINVAL;
        }
    }

    pc_current_country = mac_regdomain_get_country();

    /* 当前国家码与要设置的国家码一致，直接返回 */
    if ((pc_country[0] == pc_current_country[0])
        && (pc_country[1] == pc_current_country[1]))
    {
        return OAL_SUCC;
    }

    pst_regdom = wal_regdb_find_db(pc_country);
    if (OAL_PTR_NULL == pst_regdom)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::no regdomain db was found!}\r\n");
        return -OAL_EINVAL;
    }

    us_size = (oal_uint16)(OAL_SIZEOF(mac_regclass_info_stru) * pst_regdom->n_reg_rules + MAC_RD_INFO_LEN);

    /* 申请内存存放管制域信息，将内存指针作为事件payload抛下去 */
    /* 此处申请的内存在事件处理函数释放(hmac_config_set_country) */
    pst_mac_regdom = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, us_size, OAL_TRUE);
    if (OAL_PTR_NULL == pst_mac_regdom)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::alloc regdom mem fail, return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    wal_regdomain_fill_info(pst_regdom, pst_mac_regdom);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_COUNTRY, OAL_SIZEOF(mac_cfg_country_stru));

    /* 填写WID对应的参数 */
    pst_param = (mac_cfg_country_stru *)(st_write_msg.auc_value);
    pst_param->p_mac_regdom = pst_mac_regdom;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_country_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_regdomain_update::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
/*****************************************************************************
 函 数 名  : wal_regdomain_update_for_dfs
 功能描述  : 下发配置管制域信息
 输入参数  : pst_net_dev: net_device
             pc_country : 要设置的国家字符串
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月19日
    作    者   : zhangxiang
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_regdomain_update_for_dfs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country)
{
    OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom;
    oal_uint16                              us_size;
    mac_regdomain_info_stru                *pst_mac_regdom;
    wal_msg_write_stru                      st_write_msg;
    mac_dfs_domain_enum_uint8              *pst_param;
    oal_int32                               l_ret;
    oal_int8                               *pc_current_country;

    if (!wal_is_alpha_upper(pc_country[0]) || !wal_is_alpha_upper(pc_country[1]))
    {
        if (('9' == pc_country[0]) && ('9' == pc_country[1]))
        {
            OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::set regdomain to 99!}\r\n");
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::country str is invalid!}\r\n");
            return -OAL_EINVAL;
        }
    }

    pc_current_country = mac_regdomain_get_country();

    /* 当前国家码与要设置的国家码一致，直接返回 */
    if ((pc_country[0] == pc_current_country[0])
        && (pc_country[1] == pc_current_country[1]))
    {
        return OAL_SUCC;
    }

    pst_regdom = wal_regdb_find_db(pc_country);
    if (OAL_PTR_NULL == pst_regdom)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::no regdomain db was found!}\r\n");
        return -OAL_EINVAL;
    }

    us_size = (oal_uint16)(OAL_SIZEOF(mac_regclass_info_stru) * pst_regdom->n_reg_rules + MAC_RD_INFO_LEN);

    /* 申请内存存放管制域信息,在本函数结束后释放 */
    pst_mac_regdom = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, us_size, OAL_TRUE);
    if (OAL_PTR_NULL == pst_mac_regdom)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::alloc regdom mem fail, return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    wal_regdomain_fill_info(pst_regdom, pst_mac_regdom);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_COUNTRY_FOR_DFS, OAL_SIZEOF(mac_dfs_domain_enum_uint8));

    /* 填写WID对应的参数 */
    pst_param = (mac_dfs_domain_enum_uint8 *)(st_write_msg.auc_value);
    *pst_param = pst_mac_regdom->en_dfs_domain;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_dfs_domain_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        /*pst_mac_regdom内存，此处释放 */
        OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::return err code %d!}\r\n", l_ret);
        return l_ret;
    }
    /*pst_mac_regdom内存，此处释放 */
    OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_regdomain_update_sta
 功能描述  : sta关联成功后，更新自己的管制域信息
 输入参数  : uc_vap_id: sta的vap id
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月22日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_regdomain_update_sta(oal_uint8 uc_vap_id)
{
    oal_int8                      *pc_desired_country;

    oal_net_device_stru           *pst_net_dev;
    oal_int32                      l_ret;
    oal_bool_enum_uint8            us_updata_rd_by_ie_switch;

    hmac_vap_get_updata_rd_by_ie_switch(uc_vap_id,&us_updata_rd_by_ie_switch);

    if(OAL_TRUE== us_updata_rd_by_ie_switch)
    {
        pc_desired_country = hmac_vap_get_desired_country(uc_vap_id);

        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_desired_country))
        {
            OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::pc_desired_country is null ptr!}\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }


        /* 期望的国家码全为0，表示对端AP的国家码不存在，采用sta当前默认的国家码 */
        if ((0 == pc_desired_country[0]) && (0 == pc_desired_country[1]))
        {
            OAM_INFO_LOG0(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::ap does not have country ie, use default!}\r\n");
            return OAL_SUCC;
        }

        pst_net_dev = hmac_vap_get_net_device(uc_vap_id);

        l_ret = wal_regdomain_update_for_dfs(pst_net_dev, pc_desired_country);
        if (l_ret != OAL_SUCC)
        {
            OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::wal_regdomain_update err code %d!}\r\n", l_ret);
            return OAL_FAIL;
        }

        l_ret = wal_regdomain_update(pst_net_dev, pc_desired_country);
        if (l_ret != OAL_SUCC)
        {
            OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::wal_regdomain_update err code %d!}\r\n", l_ret);
            return OAL_FAIL;
        }


        OAM_INFO_LOG2(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::country is %u, %u!}\r\n", (oal_uint8)pc_desired_country[0], (oal_uint8)pc_desired_country[1]);

    }
    else
    {
        OAM_INFO_LOG0(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::us_updata_rd_by_ie_switch is OAL_FALSE!}\r\n");
    }
    return OAL_SUCC;
}
/*****************************************************************************
 函 数 名  : wal_hipriv_set_rd_by_ie_switch
 功能描述  :设置是否根据获取的关联ap的国家码更新自己的国家码
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月17日
    作    者   : zhangxiang
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_set_rd_by_ie_switch(oal_net_device_stru *pst_net_dev,oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_switch_enum_uint8           *pst_set_rd_by_ie_switch;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_switch_enum_uint8            en_rd_by_ie_switch = OAL_SWITCH_OFF;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RD_IE_SWITCH, OAL_SIZEOF(oal_switch_enum_uint8));

    /* 解析并设置配置命令参数 */
    pst_set_rd_by_ie_switch = (oal_switch_enum_uint8 *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rd_by_ie_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_rd_by_ie_switch = (oal_uint8)oal_atoi(ac_name);
    *pst_set_rd_by_ie_switch = en_rd_by_ie_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rd_by_ie_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

/*****************************************************************************
 函 数 名  : wal_ioctl_setcountry
 功能描述  : 设置管制域国家码
 输入参数  : pst_net_dev: net device
             pst_info   : 请求的信息
             p_w        : 请求的信息
             pc_extra   : 国家字符串
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_setcountry(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_w, oal_int8 *pc_extra)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int32  l_ret;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_setcountry::country is %d, %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return -OAL_EBUSY;
    }

    l_ret = wal_regdomain_update(pst_net_dev, pc_extra);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_setcountry::regdomain_update return err code %d!}\r\n", l_ret);
        return l_ret;
    }
#else
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_ioctl_setcountry:_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_getcountry
 功能描述  : 读取国家码
 输入参数  : pst_net_dev: net device
             pst_info   : 请求的信息
             p_w        : 请求的信息
 输出参数  : pc_extra   : 读取到的国家码
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_getcountry(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_w, oal_int8 *pc_extra)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int32                       l_ret;
    wal_msg_query_stru              st_query_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    mac_cfg_get_country_stru       *pst_get_country;
    oal_iw_point_stru              *pst_w = (oal_iw_point_stru *)p_w;

    /***************************************************************************
       抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_COUNTRY;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_QUERY,
                              WAL_MSG_WID_LENGTH,
                              (oal_uint8 *)&st_query_msg,
                              OAL_TRUE,
                              &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_getcountry:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pst_get_country = (mac_cfg_get_country_stru*)(pst_query_rsp_msg->auc_value);

    oal_memcopy(pc_extra, pst_get_country->ac_country, WLAN_COUNTRY_STR_LEN);
    pst_w->length = WLAN_COUNTRY_STR_LEN;

#endif

    return OAL_SUCC;
}

#if 0
/*****************************************************************************
 函 数 名  : wal_ioctl_gettid
 功能描述  : 读取最新接收到数据包的tid
 输入参数  : pst_net_dev: net device
             pst_info   : 请求的信息
             p_w        : 请求的信息
 输出参数  : pc_extra   : 读取到的国家码
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月16日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_gettid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_w, oal_int8 *pc_extra)
{

    oal_int32                       l_ret;
    wal_msg_query_stru              st_query_msg;
    wal_msg_stru                   *pst_rsp_msg;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    mac_cfg_get_tid_stru           *pst_get_tid;
    oal_iw_point_stru              *pst_w = (oal_iw_point_stru *)p_w;

    /***************************************************************************
       抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_TID;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_QUERY,
                              WAL_MSG_WID_LENGTH,
                              (oal_uint8 *)&st_query_msg,
                              OAL_TRUE,
                              &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
       return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* 业务处理 */
    pst_get_tid = (mac_cfg_get_tid_stru*)(pst_query_rsp_msg->auc_value);

    oal_memcopy(pc_extra, &pst_get_tid->en_tid, OAL_SIZEOF(pst_get_tid->en_tid));
    pst_w->length = OAL_SIZEOF(pst_get_tid->en_tid);


    return OAL_SUCC;
}
#endif
/*****************************************************************************
 函 数 名  : wal_hipriv_wifi_enable
 功能描述  : 私有命令，使能/去使能wifi
 输入参数  : pst_cfg_net_dev: net_device
             pc_param: 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32  wal_hipriv_wifi_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM log模块的开关的命令: hipriv "Hisilicon0 enable 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
       return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WIFI_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_pm_info
 功能描述  : 私有命令，PM状态机信息维测输出
 输入参数  : pst_cfg_net_dev: net_device
             pc_param: 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/

OAL_STATIC oal_uint32  wal_hipriv_pm_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_INFO, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

/*****************************************************************************
 函 数 名  : wal_hipriv_pm_enable
 功能描述  : 私有命令，PM功能关闭开启
 输入参数  : pst_cfg_net_dev: net_device
             pc_param: 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;


    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
       return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_int32  wal_set_random_mac_to_mib(oal_net_device_stru *pst_net_dev)
{
    oal_uint32                    ul_ret;
    frw_event_mem_stru           *pst_event_mem;
    wal_msg_stru                 *pst_cfg_msg;
    wal_msg_write_stru           *pst_write_msg;
    hmac_vap_cfg_priv_stru       *pst_cfg_priv;
    mac_cfg_staion_id_param_stru *pst_param;
    mac_vap_stru                 *pst_mac_vap;
    oal_uint8                    *puc_mac_addr;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_wireless_dev_stru        *pst_wdev; /* 对于P2P 场景，p2p0 和 p2p-p2p0 MAC 地址从wlan0 获取 */
    mac_device_stru              *pst_mac_device;
    wlan_p2p_mode_enum_uint8      en_p2p_mode = WLAN_LEGACY_VAP_MODE;
    oal_uint8                     auc_primary_mac_addr[WLAN_MAC_ADDR_LEN] = {0};    /* MAC地址 */
#endif

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

#ifdef _PRE_WLAN_FEATURE_P2P
    /* 获取wlan0 MAC 地址，生成p2p0/p2p-p2p0 MAC 地址 */
    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_mac_vap->uc_device_id);
    pst_wdev = pst_net_dev->ieee80211_ptr;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device->st_p2p_info.pst_primary_net_device))
    {
        /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
        oal_random_ether_addr(auc_primary_mac_addr);
        auc_primary_mac_addr[0] &= (~0x02);
        auc_primary_mac_addr[1] = 0x11;
        auc_primary_mac_addr[2] = 0x02;
    }
    else
    {
#ifndef _PRE_PC_LINT
        if(OAL_LIKELY(OAL_PTR_NULL != OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device)))
        {
            oal_memcopy(auc_primary_mac_addr, OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device), WLAN_MAC_ADDR_LEN);
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib() pst_primary_net_device; addr is null}\r\n");
            return OAL_FAIL;
        }
#endif
    }

    switch (pst_wdev->iftype)
    {
        case NL80211_IFTYPE_P2P_DEVICE:
        {
           en_p2p_mode = WLAN_P2P_DEV_MODE;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
            /* 产生P2P device MAC 地址，将本地mac 地址bit 设置为1 */
            oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                        OAL_NETDEVICE_MAC_ADDR(pst_net_dev),
                        WLAN_MAC_ADDR_LEN);

#else
            /* 产生P2P device MAC 地址，将本地mac 地址bit 设置为1 */
            oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                        auc_primary_mac_addr,
                        WLAN_MAC_ADDR_LEN);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID[0] |= 0x02;
#endif
            break;
        }
        case NL80211_IFTYPE_P2P_CLIENT:
        {
            en_p2p_mode = WLAN_P2P_CL_MODE;
            /* 产生P2P interface MAC 地址 */
            oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID,
                        auc_primary_mac_addr,
                        WLAN_MAC_ADDR_LEN);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0] |= 0x02;
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[4] ^= 0x80;
            break;
        }
        case NL80211_IFTYPE_P2P_GO:
        {
            en_p2p_mode = WLAN_P2P_GO_MODE;
            /* 产生P2P interface MAC 地址 */
            oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID,
                        auc_primary_mac_addr,
                        WLAN_MAC_ADDR_LEN);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0] |= 0x02;
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[4] ^= 0x80;
            break;
        }
        default:
        {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
            if(0 == (oal_strcmp("p2p0", pst_net_dev->name)))
            {
                en_p2p_mode = WLAN_P2P_DEV_MODE;
                 /* 产生P2P device MAC 地址，将本地mac 地址bit 设置为1 */
                oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                            OAL_NETDEVICE_MAC_ADDR(pst_net_dev),
                            WLAN_MAC_ADDR_LEN);
                break;
            }

            oal_memcopy(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID,
                        OAL_NETDEVICE_MAC_ADDR(pst_net_dev),
                        WLAN_MAC_ADDR_LEN);
#else
            /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
            oal_random_ether_addr(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0]&=(~0x02);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[1]=0x11;
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[2]=0x02;
#endif
            break;
        }
    }
#else
    /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
    oal_random_ether_addr(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0]&=(~0x02);
    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[1]=0x11;
    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[2]=0x02;
#endif

    /* send the random mac to dmac */
    /***************************************************************************
        抛事件到wal层处理   copy from wal_netdev_set_mac_addr()
        gong TBD : 改为调用通用的config接口
    ***************************************************************************/
    ul_ret = wal_alloc_cfg_event(pst_net_dev, &pst_event_mem, &pst_cfg_priv, &pst_cfg_msg, (WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru)));     /* 申请事件 */
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib() fail; return %d!}\r\n", ul_ret);
        return -OAL_ENOMEM;
    }

    /* 填写配置消息 */
    WAL_CFG_MSG_HDR_INIT(&(pst_cfg_msg->st_msg_hdr),
                         WAL_MSG_TYPE_WRITE,
                         WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru),
                         WAL_GET_MSG_SN(pst_cfg_priv->uc_msg_sn));

    /* 填写WID消息 */
    pst_write_msg = (wal_msg_write_stru *)pst_cfg_msg->auc_msg_data;
    WAL_WRITE_MSG_HDR_INIT(pst_write_msg, WLAN_CFGID_STATION_ID, OAL_SIZEOF(mac_cfg_staion_id_param_stru));

    pst_param = (mac_cfg_staion_id_param_stru *)pst_write_msg->auc_value;   /* 填写WID对应的参数 */
#ifdef _PRE_WLAN_FEATURE_P2P
    /* 如果使能P2P，需要将netdevice 对应的P2P 模式在配置参数中配置到hmac 和dmac */
    /* 以便底层识别配到p2p0 或p2p-p2p0 cl */
    pst_param->en_p2p_mode = en_p2p_mode;
    if (en_p2p_mode == WLAN_P2P_DEV_MODE)
    {
        puc_mac_addr = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID;
    }
    else
#endif
    {
        puc_mac_addr = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID;
    }
    oal_set_mac_addr(pst_param->auc_station_id, puc_mac_addr);

    OAL_IO_PRINT("wal_set_random_mac_to_mib,mac is:%.2x:%.2x:%.2x\n",
                  puc_mac_addr[0], puc_mac_addr[1], puc_mac_addr[2]);

    frw_event_dispatch_event(pst_event_mem);    /* 分发事件 */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
#endif


/*****************************************************************************
 函 数 名  : wal_hipriv_setcountry
 功能描述  : 设置管制域国家码
 输入参数  : pst_net_dev: net device
             pc_param   : 国家字符串
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月16日
    作    者   : zhangxiang
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_setcountry(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int32                        l_ret;
    oal_uint32                       ul_ret;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        *puc_para;

    /* 设备在up状态不允许配置，必须先down */
    if (0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)))
    {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::country is %d, %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_EBUSY;
    }
    /* 获取国家码字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    puc_para = &ac_arg[0];

    l_ret = wal_regdomain_update_for_dfs(pst_net_dev, puc_para);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::regdomain_update return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    l_ret = wal_regdomain_update(pst_net_dev, puc_para);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::regdomain_update return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }


#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_setcountry::_PRE_WLAN_FEATURE_11D is not define!}\r\n");

#endif
    return OAL_SUCC;

}

/*****************************************************************************
 函 数 名  : wal_hipriv_getcountry
 功能描述  : 读取国家码
 输入参数  : pst_net_dev: net device
 输出参数  : pc_extra   : 读取到的国家码
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月16日
    作    者   : zhangxiang
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_getcountry(oal_net_device_stru *pst_net_dev,oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_FEATURE_11D

    wal_msg_query_stru           st_query_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_COUNTRY;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH ,
                               (oal_uint8 *)&st_query_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
       OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_getcountry::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
       return (oal_uint32)l_ret;
    }

#else
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_hipriv_getcountry::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_add_vap
 功能描述  : 私有命令，创建VAP
 输入参数  : pst_cfg_net_dev: net_device
             pc_param: 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_add_vap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_net_device_stru        *pst_net_dev;
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                    ac_mode[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wlan_vap_mode_enum_uint8    en_mode;
    mac_vap_stru               *pst_mac_vap;

    oal_wireless_dev_stru      *pst_wdev;

    mac_vap_stru               *pst_cfg_mac_vap;
    mac_device_stru            *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_vap_stru               *pst_cfg_mac_vap_proxysta;
    mac_device_stru            *pst_mac_device_proxysta;
    oal_bool_enum_uint8         en_is_proxysta      = OAL_FALSE;
    oal_bool_enum_uint8         en_is_main_proxysta = OAL_FALSE;
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif


    /* pc_param指向新创建的net_device的name, 将其取出存放到ac_name中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;

    /* pc_param 指向'ap|sta', 将其取出放到ac_mode中 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_mode, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    /* 获取mac device */
    pst_cfg_mac_vap_proxysta = OAL_NET_DEV_PRIV(pst_cfg_net_dev);
    pst_mac_device_proxysta  = mac_res_get_dev(pst_cfg_mac_vap_proxysta->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device_proxysta))
    {
        OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_hipriv_add_vap:: pst_mac_device is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    /* 解析ac_mode字符串对应的模式 */
    if (0 == (oal_strcmp("ap", ac_mode)))
    {
        en_mode = WLAN_VAP_MODE_BSS_AP;
    }
    else if (0 == (oal_strcmp("sta", ac_mode)))
    {
        en_mode = WLAN_VAP_MODE_BSS_STA;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (OAL_TRUE == pst_mac_device_proxysta->st_cap_flag.bit_proxysta)
        {
            en_is_proxysta       = OAL_TRUE;
            en_is_main_proxysta  = OAL_TRUE;
        }
#endif
    }
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    else if (0 == (oal_strcmp("proxysta", ac_mode)))
    {
        en_mode              = WLAN_VAP_MODE_BSS_STA;
        if (OAL_TRUE == pst_mac_device_proxysta->st_cap_flag.bit_proxysta)
        {
            en_is_proxysta       = OAL_TRUE;
            en_is_main_proxysta  = OAL_FALSE;
            OAM_INFO_LOG2(0, OAM_SF_PROXYSTA, "{wal_hipriv_add_vap::en_is_proxysta:%d,en_is_main_proxysta:%d}",en_is_proxysta,en_is_main_proxysta);

        }
    }
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    /* 创建P2P 相关VAP */
    else if (0 == (oal_strcmp("p2p_device", ac_mode)))
    {
        en_mode = WLAN_VAP_MODE_BSS_STA;
        en_p2p_mode = WLAN_P2P_DEV_MODE;
    }
    else if (0 == (oal_strcmp("p2p_cl", ac_mode)))
    {
        en_mode = WLAN_VAP_MODE_BSS_STA;
        en_p2p_mode = WLAN_P2P_CL_MODE;
    }
    else if (0 == (oal_strcmp("p2p_go", ac_mode)))
    {
        en_mode = WLAN_VAP_MODE_BSS_AP;
        en_p2p_mode = WLAN_P2P_GO_MODE;
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_vap::the mode param is invalid!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 如果创建的net device已经存在，直接返回 */
    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_name);
    if (OAL_PTR_NULL != pst_net_dev)
    {
        /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
        oal_dev_put(pst_net_dev);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_vap::the net_device is already exist!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac device */
    pst_cfg_mac_vap = OAL_NET_DEV_PRIV(pst_cfg_net_dev);
    pst_mac_device  = mac_res_get_dev(pst_cfg_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::pst_mac_device is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_WLAN_FEATURE_FLOWCTL)
    pst_net_dev = oal_net_alloc_netdev_mqs(0, ac_name, oal_ether_setup, WAL_NETDEV_SUBQUEUE_MAX_NUM, 1);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
#elif defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL)
    pst_net_dev = oal_net_alloc_netdev_mqs(0, ac_name, oal_ether_setup, WLAN_NET_QUEUE_BUTT, 1);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
    OAM_INFO_LOG1(0,OAM_SF_ANY, "after alloc_netdev: txq_num = %d\r\n", OAL_NETDEVICE_TX_QUEUE_NUM(pst_net_dev));
#else
    pst_net_dev = oal_net_alloc_netdev(0, ac_name, oal_ether_setup);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::pst_net_dev null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wdev = (oal_wireless_dev_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(oal_wireless_dev_stru), OAL_FALSE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_wdev))
    {
        OAM_ERROR_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::alloc mem, pst_wdev is null ptr!}\r\n");
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memset(pst_wdev, 0, OAL_SIZEOF(oal_wireless_dev_stru));

    /* 对netdevice进行赋值 */
    pst_net_dev->wireless_handlers             = &g_st_iw_handler_def;
    /* OAL_NETDEVICE_OPS(pst_net_dev)             = &g_st_wal_net_dev_ops; */
    pst_net_dev->netdev_ops                    = &g_st_wal_net_dev_ops;
    OAL_NETDEVICE_DESTRUCTOR(pst_net_dev)      = oal_net_free_netdev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,44))
    OAL_NETDEVICE_MASTER(pst_net_dev)          = OAL_PTR_NULL;
#endif

    OAL_NETDEVICE_IFALIAS(pst_net_dev)         = OAL_PTR_NULL;
    OAL_NETDEVICE_WATCHDOG_TIMEO(pst_net_dev)  = 5;
    OAL_NETDEVICE_WDEV(pst_net_dev)            = pst_wdev;
    OAL_NETDEVICE_QDISC(pst_net_dev, OAL_PTR_NULL);
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
//    OAL_NETDEVICE_TX_QUEUE_LEN(pst_net_dev) = 0;
#endif

    pst_wdev->netdev = pst_net_dev;

    if (WLAN_VAP_MODE_BSS_AP == en_mode)
    {
        pst_wdev->iftype = NL80211_IFTYPE_AP;
    }
    else if (WLAN_VAP_MODE_BSS_STA == en_mode)
    {
        pst_wdev->iftype = NL80211_IFTYPE_STATION;
    }
#ifdef _PRE_WLAN_FEATURE_P2P
    if (WLAN_P2P_DEV_MODE == en_p2p_mode)
    {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_DEVICE;
    }
    else if (WLAN_P2P_CL_MODE == en_p2p_mode)
    {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_CLIENT;
    }
    else if (WLAN_P2P_GO_MODE == en_p2p_mode)
    {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_GO;
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */

    pst_wdev->wiphy = pst_mac_device->pst_wiphy;

    OAL_NETDEVICE_FLAGS(pst_net_dev) &= ~OAL_IFF_RUNNING;   /* 将net device的flag设为down */


    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_VAP, OAL_SIZEOF(mac_cfg_add_vap_param_stru));
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_vap_mode  = en_mode;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->uc_cfg_vap_indx = pst_cfg_mac_vap->uc_vap_id;
#ifdef  _PRE_WLAN_FEATURE_PROXYSTA
    if (OAL_TRUE == pst_mac_device_proxysta->st_cap_flag.bit_proxysta)
    {
        ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_is_proxysta      = en_is_proxysta;
        ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_is_main_proxysta = en_is_main_proxysta;
    }
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
   ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode  = en_p2p_mode;
#endif
    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    /* 读取返回的错误码 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::hmac add vap fail,err code[%d]!}\r\n", pst_write_rsp_msg->ul_err_code);
            /* 异常处理，释放内存 */
            OAL_MEM_FREE(pst_wdev, OAL_FALSE);
            oal_net_free_netdev(pst_net_dev);
            return pst_write_rsp_msg->ul_err_code;
        }
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    if ((WLAN_LEGACY_VAP_MODE == en_p2p_mode) && (OAL_PTR_NULL == pst_mac_device->st_p2p_info.pst_primary_net_device))
    {
        /* 如果创建wlan0， 则保存wlan0 为主net_device,p2p0 和p2p-p2p0 MAC 地址从主netdevice 获取 */
        pst_mac_device->st_p2p_info.pst_primary_net_device = pst_net_dev;
    }

    if(OAL_SUCC != wal_set_random_mac_to_mib(pst_net_dev))
    {
        /* 异常处理，释放内存 */
        /* 异常处理，释放内存 */
        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    } /* set random mac to mib ; for hi1102-cb */
#endif

    /* 设置netdevice的MAC地址，MAC地址在HMAC层被初始化到MIB中 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_p2p_mode == WLAN_P2P_DEV_MODE)
    {
        oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID);

        pst_mac_device->st_p2p_info.uc_p2p0_vap_idx = pst_mac_vap->uc_vap_id;
    }
    else
#endif
    {
        oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
    }


    /* 注册net_device */
    ul_ret = (oal_uint32)oal_net_register_netdev(pst_net_dev);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::oal_net_register_netdev return error code %d!}\r\n", ul_ret);

        /* 异常处理，释放内存 */
        /* 抛删除vap事件释放刚申请的vap  */
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));

        l_ret = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_TRUE,
                                   &pst_rsp_msg);

        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_del_vap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    wal_msg_stru               *pst_rsp_msg;
    oal_int32                    l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_wireless_dev_stru       *pst_wdev;
    wlan_p2p_mode_enum_uint8     en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_del_vap::pst_net_dev or pc_param null ptr error %d, %d!}\r\n",
                       pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设备在up状态不允许删除，必须先down */
    if (OAL_UNLIKELY(0 != (OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev))))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_vap::device is busy, please down it first %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    //删除vap 时需要将参数赋值。
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_del_vap::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

#endif

    OAL_MEM_FREE(OAL_NETDEVICE_WDEV(pst_net_dev), OAL_TRUE);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_vap::return err code %d}\r\n", l_ret);
        /* 去注册 */
        oal_net_unregister_netdev(pst_net_dev);
        return (oal_uint32)l_ret;
    }

    /* 去注册 */
    oal_net_unregister_netdev(pst_net_dev);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_vap_info
 功能描述  : 打印vap的所有参数信息
 输入参数  : pst_net_dev : net_device
             pc_param    : 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月28日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_INFO, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_vap_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_beacon_chain_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::pst_net_dev or pc_param null ptr error %d, %d!}\r\n",  pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* beacon通道(0/1)切换开关的命令: hipriv "vap0 beacon_chain_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，配置不同的通道 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::the beacon chain switch command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BEACON_CHAIN_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#if 0
OAL_STATIC oal_uint32  wal_hipriv_tdls_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_tdls_prohibited::pst_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* TDLS禁用开关的命令: hipriv "vap0 tdls_prohi 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_prohibited::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_prohibited::the tdls_prohibited command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TDLS_PROHI, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_prohibited::err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_tdls_channel_switch_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_tdls_channel_switch_prohibited::pst_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* TDLS禁用开关的命令: hipriv "vap0 tdls_chaswi_prohi 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_channel_switch_prohibited::wal_get_cmd_one_arg return err_code, %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS信道切换禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_channel_switch_prohibited::the channel_switch_prohibited switch command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TDLS_CHASWI_PROHI, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tdls_channel_switch_prohibited::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
/*****************************************************************************
 函 数 名  : wal_hipriv_2040_channel_switch_prohibited
 功能描述  : 设置20/40共存是否禁止信道切换
 输入参数  : pst_net_dev : net_device
             pc_param    : 参数
 输出参数  : 无
 返 回 值  : OAL_SUCC或其它错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_2040_channel_switch_prohibited(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_channel_switch_prohibited::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
         return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        uc_csp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        uc_csp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_channel_switch_prohibited::the channel_switch_prohibited switch command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_CHASWI_PROHI, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_channel_switch_prohibited::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_FortyMHzIntolerant
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
OAL_STATIC oal_uint32  wal_hipriv_set_FortyMHzIntolerant(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_FortyMHzIntolerant::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
         return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        uc_csp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        uc_csp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_FortyMHzIntolerant::the 2040_intolerant command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_INTOLERANT, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_FortyMHzIntolerant::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_2040_coext_support
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
OAL_STATIC oal_uint32  wal_hipriv_set_2040_coext_support(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_2040_coext_support::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
         return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        uc_csp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        uc_csp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_2040_coext_support::the 2040_coexistence command is erro %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_COEXISTENCE, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_2040_coext_support::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_rx_fcs_info
 功能描述  : 打印对应vap的接收FCS的信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月8日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC  oal_uint32  wal_hipriv_rx_fcs_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    mac_cfg_rx_fcs_info_stru   *pst_rx_fcs_info;
    mac_cfg_rx_fcs_info_stru    st_rx_fcs_info;  /* 临时保存获取的use的信息 */

    /* 打印接收帧的FCS正确与错误信息:sh hipriv.sh "vap0 rx_fcs_info 0/1 1-4" 0/1  0代表不清楚，1代表清楚 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    st_rx_fcs_info.ul_data_op = (oal_uint32)oal_atoi(ac_name);

    if (st_rx_fcs_info.ul_data_op > 1)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::the ul_data_op command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    st_rx_fcs_info.ul_print_info = (oal_uint32)oal_atoi(ac_name);

    if (st_rx_fcs_info.ul_print_info > 4)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::the ul_print_info command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FCS_INFO, OAL_SIZEOF(mac_cfg_rx_fcs_info_stru));

    /* 设置配置命令参数 */
    pst_rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)(st_write_msg.auc_value);
    pst_rx_fcs_info->ul_data_op    = st_rx_fcs_info.ul_data_op;
    pst_rx_fcs_info->ul_print_info = st_rx_fcs_info.ul_print_info;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_rx_fcs_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_global_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                   l_switch_val;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    /* 获取开关状态值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_global_switch((oal_switch_enum_uint8)l_switch_val);
}

/*****************************************************************************
 函 数 名  : wal_hipriv_vap_log_switch
 功能描述  : 配置VAP级别日志开关
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月3日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_vap_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru               *pst_mac_vap;
    oal_int32                   l_switch_val;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_switch::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取开关状态值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_vap_switch(pst_mac_vap->uc_vap_id, (oal_switch_enum_uint8)l_switch_val);
}

/*****************************************************************************
 函 数 名  : wal_hipriv_vap_log_level
 功能描述  : 设置vap的LOG打印级别
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月3日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_vap_log_level(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru               *pst_mac_vap;
    oam_log_level_enum_uint8    en_level_val;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    wal_msg_write_stru          st_write_msg;
#endif

    /* OAM log模块的开关的命令: hipriv "Hisilicon0[vapx] log_level {1/2}"
       1-2(error与warning)级别日志以vap级别为维度；
    */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_level::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取日志级别 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    en_level_val = (oam_log_level_enum_uint8)oal_atoi(ac_param);
    if ((en_level_val<OAM_LOG_LEVEL_ERROR) || (en_level_val>OAM_LOG_LEVEL_INFO))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_feature_log_level::invalid switch value[%d].}", en_level_val);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#if 0  /* 产品发布前支持info级别全局开 */
    if ((0 != oal_strcmp("1", ac_param)) && (0 != oal_strcmp("2", ac_param)))
    {
        /* INFO级别 */
        if (0 == oal_strcmp("3", ac_param))
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_level::not support info level config. use[feature_log_switch]command}\r\n");
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
        }
        else
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_level::invalid switch value}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }
#endif
    ul_ret = oam_log_set_vap_level(pst_mac_vap->uc_vap_id, en_level_val);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
// 目前支持02 device 设置log 级别， 遗留后续的合并问题
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LOG_LEVEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = en_level_val;
    ul_ret |= (oal_uint32)wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_vap_log_level::return err code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

#endif
    return ul_ret;
}
OAL_STATIC oal_uint32  wal_hipriv_feature_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru                       *pst_mac_vap;
    oam_feature_enum_uint8              en_feature_id;
    oal_uint8                           uc_switch_vl;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oam_log_level_enum_uint8            en_log_lvl;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        wal_msg_write_stru          st_write_msg;
#endif

    /* OAM log模块的开关的命令: hipriv "Hisilicon0[vapx] feature_log_switch {feature_name} {0/1}"
       1-2(error与warning)级别日志以vap级别为维度；
    */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取特性名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 提供特性名的帮助信息 */
    if ('?' == ac_param[0])
    {
        OAL_IO_PRINT("please input abbr feature name. \r\n");
        oam_show_feature_list();
        return OAL_SUCC;
    }

    /* 获取特性ID */
    ul_ret = oam_get_feature_id((oal_uint8 *)ac_param, &en_feature_id);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::invalid feature name}\r\n");
        return ul_ret;
    }

    /* 获取开关值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取INFO级别开关状态 */
    if ((0 != oal_strcmp("0", ac_param)) && (0 != oal_strcmp("1", ac_param)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    uc_switch_vl = (oal_uint8)oal_atoi(ac_param);

     /* 关闭INFO日志级别时，恢复成默认的日志级别 */
    en_log_lvl = (OAL_SWITCH_ON == uc_switch_vl) ? OAM_LOG_LEVEL_INFO : OAM_LOG_DEFAULT_LEVEL;
    ul_ret = oam_log_set_feature_level(pst_mac_vap->uc_vap_id, en_feature_id, en_log_lvl) ;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FEATURE_LOG, OAL_SIZEOF(oal_int32));
    *((oal_uint16 *)(st_write_msg.auc_value)) = ((en_feature_id<<8) | en_log_lvl);
    ul_ret |= (oal_uint32)wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::return err code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
#endif

    return ul_ret;

}


OAL_STATIC oal_uint32  wal_hipriv_log_ratelimit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oam_ratelimit_stru                  st_ratelimit;
    oam_ratelimit_type_enum_uint8       en_ratelimit_type;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;

    /* OAM log printk流控配置命令: hipriv "Hisilicon0[vapx] {log_ratelimit} {printk(0)/sdt(1)}{switch(0/1)} {interval} {burst}" */

    st_ratelimit.en_ratelimit_switch    = OAL_SWITCH_OFF;
    st_ratelimit.ul_interval            = OAM_RATELIMIT_DEFAULT_INTERVAL;
    st_ratelimit.ul_burst               = OAM_RATELIMIT_DEFAULT_BURST;

    /* 获取限速类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    en_ratelimit_type =  (oam_ratelimit_type_enum_uint8)oal_atoi(ac_param);

    /* 获取开关状态 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_ratelimit.en_ratelimit_switch =  (oal_switch_enum_uint8)oal_atoi(ac_param);

    if (OAL_SWITCH_ON == st_ratelimit.en_ratelimit_switch)
    {
        /* 获取interval值 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_interval = (oal_uint32)oal_atoi(ac_param);

        /* 获取burst值 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_burst = (oal_uint32)oal_atoi(ac_param);
    }

    return oam_log_set_ratelimit_param(en_ratelimit_type, &st_ratelimit);
}
OAL_STATIC oal_uint32  wal_hipriv_event_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM event模块的开关的命令: hipriv "Hisilicon0 event_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对event模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EVENT_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_eth_switch_param_stru   st_eth_switch_param;

    /* "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(帧方向) 0|1(开关)" */
    OAL_MEMZERO(&st_eth_switch_param, OAL_SIZEOF(mac_cfg_eth_switch_param_stru));

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_eth_switch_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_hipriv_get_mac_addr return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取以太网帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_eth_switch_param.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    st_eth_switch_param.en_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ETH_SWITCH, OAL_SIZEOF(st_eth_switch_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_eth_switch_param,
                OAL_SIZEOF(st_eth_switch_param));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_eth_switch_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_80211_ucast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;

    /* sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                                       0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */
    OAL_MEMZERO(&st_80211_ucast_switch, OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru));

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_80211_ucast_switch.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::wal_hipriv_get_mac_addr return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get 80211 ucast frame direction return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame type return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame content switch  return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame cb switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame dscr switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(st_80211_ucast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_ucast_switch,
                OAL_SIZEOF(st_80211_ucast_switch));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_ucast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#ifdef _PRE_DEBUG_MODE_USER_TRACK
OAL_STATIC oal_uint32  wal_hipriv_report_thrput_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_usr_thrput_stru         st_usr_thrput;

    /* sh hipriv.sh "vap_name thrput_stat  XX:XX:XX:XX:XX;XX 0|1" */

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_usr_thrput.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_hipriv_get_mac_addr return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_usr_thrput.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USR_THRPUT_STAT, OAL_SIZEOF(st_usr_thrput));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_usr_thrput, OAL_SIZEOF(st_usr_thrput));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_usr_thrput),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS
OAL_STATIC oal_uint32  wal_hipriv_set_txop_ps_machw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_txopps_machw_param_stru     st_txopps_machw_param;

    /* sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)" */

    /* 获取txop ps使能开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps en return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_en = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* 获取txop ps condition1使能开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition1 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition1= (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* 获取txop ps condition2使能开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition2 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition2 = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TXOP_PS_MACHW, OAL_SIZEOF(st_txopps_machw_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_txopps_machw_param,
                OAL_SIZEOF(st_txopps_machw_param));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_txopps_machw_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
/*****************************************************************************
 函 数 名  : wal_hipriv_btcoex_status_print
 功能描述  : 打印共存维测信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月17日
    作    者   : g00306640
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32 wal_hipriv_btcoex_status_print(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    /* sh hipriv.sh "vap_name coex_print" */

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BTCOEX_STATUS_PRINT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_btcoex_status_print::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif



OAL_STATIC oal_uint32  wal_hipriv_set_80211_mcast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_80211_mcast_switch_stru st_80211_mcast_switch;

    /* sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                                0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get 80211 mcast frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_MCAST_SWITCH, OAL_SIZEOF(st_80211_mcast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_mcast_switch,
                OAL_SIZEOF(st_80211_mcast_switch));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_mcast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_all_80211_ucast(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;
    wal_msg_write_stru              st_write_msg;
    oal_uint8                       auc_broadcast_addr[WLAN_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    oal_int32                       l_ret;


    /* sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                             0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */

    OAL_MEMZERO(&st_80211_ucast_switch, OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru));

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get 80211 ucast frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);


    /* 设置广播mac地址 */
    oal_memcopy(st_80211_ucast_switch.auc_user_macaddr, auc_broadcast_addr, WLAN_MAC_ADDR_LEN);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(st_80211_ucast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_ucast_switch,
                OAL_SIZEOF(st_80211_ucast_switch));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_ucast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_all_ucast_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_all_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint8                       uc_user_num;
    oal_uint8                       uc_frame_direction;
    oal_uint8                       uc_switch;

    /* sh hipriv.sh "Hisilicon0 ether_all 0|1(帧方向tx|rx) 0|1(开关)" */

    /* 获取以太网帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ether_switch::get eth frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ether_switch::get eth type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /* 设置开关 */
    for (uc_user_num = 0; uc_user_num < WLAN_ACTIVE_USER_MAX_NUM + WLAN_MAX_MULTI_USER_NUM_SPEC; uc_user_num++)
    {
        oam_report_eth_frame_set_switch(uc_user_num, uc_switch, uc_frame_direction);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_dhcp_arp_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_switch;

    /* sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(开关)" */

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::get switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DHCP_ARP, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = (oal_uint32)uc_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_report_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_uint32                      ul_flag_value;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;

    /* sh hipriv.sh "wlan0 report_vap_info  flags_value" */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_flag_value = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_VAP_INFO, OAL_SIZEOF(ul_flag_value));

    /* 填写消息体，参数 */
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_flag_value;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_flag_value),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_rssi_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_rssi_debug_switch_stru      st_rssi_switch;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;

    /* sh hipriv.sh "Hisilicon0 rssi_switch  0|1(关闭|打开) N(每个N个报文打印一次)" */

    OAL_MEMZERO(&st_rssi_switch, OAL_SIZEOF(st_rssi_switch));

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::get rssi switch on, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_rssi_switch.ul_rssi_debug_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::get rx comp isr interval switch info, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_rssi_switch.ul_rx_comp_isr_interval = (oal_uint8)oal_atoi(ac_name);

    OAM_INFO_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::rssi_switch: %d, interval: %d.}",
                  st_rssi_switch.ul_rssi_debug_switch, st_rssi_switch.ul_rx_comp_isr_interval);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RSSI_SWITCH, OAL_SIZEOF(st_rssi_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_rssi_switch,
                OAL_SIZEOF(st_rssi_switch));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_rssi_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::return err code[%d]!}", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_random_mac_addr_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_rand_mac_addr_scan_switch;

    /* sh hipriv.sh "Hisilicon0 random_mac_addr_scan 0|1(开关)" */

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_random_mac_addr_scan::get switch return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_rand_mac_addr_scan_switch = (oal_uint8)oal_atoi(ac_name);

    /* 开关的取值范围为0|1,做参数合法性判断 */
    if (uc_rand_mac_addr_scan_switch > 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_random_mac_addr_scan::param is error, switch_value[%d]!}",
                         uc_rand_mac_addr_scan_switch);
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RANDOM_MAC_ADDR_SCAN, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = (oal_uint32)uc_rand_mac_addr_scan_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_random_mac_addr_scan::return err code[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_set_probe_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_probe_switch_stru       st_probe_switch;

    /* sh hipriv.sh "Hisilicon0 probe_switch 0|1(帧方向tx|rx) 0|1(帧内容开关)
                                             0|1(CB开关) 0|1(描述符开关)"
    */

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROBE_SWITCH, OAL_SIZEOF(st_probe_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_probe_switch,
                OAL_SIZEOF(st_probe_switch));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_probe_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_get_mpdu_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_get_mpdu_num_stru       st_param;

    /* sh hipriv.sh "vap_name mpdu_num user_macaddr" */

    OAL_MEMZERO(&st_param, OAL_SIZEOF(mac_cfg_get_mpdu_num_stru));

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_MPDU_NUM, OAL_SIZEOF(st_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_param,
                OAL_SIZEOF(st_param));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_all_ota(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_param;
    wal_msg_write_stru              st_write_msg;

    /* 获取开关 sh hipriv.sh "Hisilicon0 set_all_ota 0|1"*/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    l_param = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALL_OTA, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ota::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#if 0

OAL_STATIC oal_uint32  wal_hipriv_ota_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_ota_type;
    oal_int32                           l_param;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;
    wal_specific_event_type_param_stru *pst_specific_event_param;

    /* OAM ota模块的开关的命令: hipriv "Hisilicon0 ota_switch event_type 0 | 1",
       此处将首先解析出ota类型(oam_ota_type_enum_uint8),然后解析开关参数"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_ota_type = oal_atoi((const oal_int8 *)ac_name);

    /* 解析参数 */
    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);



    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_SWITCH, OAL_SIZEOF(wal_specific_event_type_param_stru));
    pst_specific_event_param = (wal_specific_event_type_param_stru *)st_write_msg.auc_value;
    pst_specific_event_param->l_event_type = l_ota_type;
    pst_specific_event_param->l_param      = l_param;

    OAL_IO_PRINT("wal_hipriv_ota_switch: en_ota_type:%d  en_switch_type:%d \n", l_ota_type, l_param);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(wal_specific_event_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_switch::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_hipriv_start_dpd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_DPD, OAL_SIZEOF(wal_specific_event_type_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(wal_specific_event_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_CHIP_TEST
/*****************************************************************************
 函 数 名  : wal_hipriv_beacon_offload_test
 功能描述  : 手动配置host sleep, 仅用于测试
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月3日
    作    者   : l00311403
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_beacon_offload_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru                  st_write_msg;
    oal_uint8                           *uc_param;
    oal_uint32                          ul_off_set = 0;
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;
    oal_uint8                           i;

    uc_param = (oal_uint8*)st_write_msg.auc_value;

    /* hipriv "Hisilicon0 beacon_offload_test param0 param1 param2 param3", */
    for (i=0; i<4; i++)
    {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_offload_test::wal_get_cmd_one_arg param[%d] fails!}", i);
            return ul_ret;
        }
        *uc_param = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
        uc_param++;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, OAL_SIZEOF(wal_specific_event_type_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_ota_beacon_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_param;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;

    /* OAM ota模块的开关的命令: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_BEACON_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_ota_rx_dscr_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_param;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;

    /* OAM ota模块的开关的命令: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1"
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);

    /* 解析参数 */
    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_RX_DSCR_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_hipriv_get_tx_comp_cnt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32      ul_ret          = OAL_SUCC;
    oal_uint32      ul_off_set      = 0;
    oal_uint8       uc_stat_flag    = 0;
    oal_int8        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_vap_stru    *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 统计发送完成中断是否丢失(关闭聚合) sh hipriv.sh "Hisilicon0 tx_comp_cnt 0|1",
       0表示清零统计次数， 1表示显示统计次数并且清零",
    */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    uc_stat_flag = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if( 0 == uc_stat_flag)
    {
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_data_num            = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_mgnt_num            = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_total_num  = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num    = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num    = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num    = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num    = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num    = 0;

    }
    else
    {
        OAL_IO_PRINT("ul_tx_data_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_data_num);
        OAL_IO_PRINT("ul_tx_mgnt_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_mgnt_num);
        OAL_IO_PRINT("ul_tx_complete_total_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_total_num);
        OAL_IO_PRINT("ul_tx_complete_uh1_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num);
        OAL_IO_PRINT("ul_tx_complete_uh2_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh1_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num);
        OAL_IO_PRINT("ul_tx_complete_bh2_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh3_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num);
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_off_set = 0;
    oal_uint8                           uc_debug_type = 0;
    oal_int8                            ac_debug_type[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                          ul_ret;
    oal_uint8                           uc_idx;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_debug_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg return error code[%d]! \n", __LINE__, ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    if (ac_debug_type[0] == '?')
    {
        OAL_IO_PRINT("debug_switch <debug_type> <switch_state> \n");
        OAL_IO_PRINT("               0      0/1     -- when set register echo read value \n");
        return OAL_SUCC;
    }

    uc_debug_type = (oal_uint8)oal_atoi((const oal_int8 *)ac_debug_type);
    if (uc_debug_type >= MAX_DEBUG_TYPE_NUM)
    {
        OAL_IO_PRINT("Info: <debug_type> should be less than %d. \n", MAX_DEBUG_TYPE_NUM);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_debug_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg return error code[%d]! \n", __LINE__, ul_ret);
        return ul_ret;
    }

    g_aul_debug_feature_switch[uc_debug_type] = (oal_uint32)oal_atoi((const oal_int8 *)ac_debug_type);
    if ((g_aul_debug_feature_switch[uc_debug_type] != OAL_SWITCH_ON) && (g_aul_debug_feature_switch[uc_debug_type] != OAL_SWITCH_OFF))
    {
        OAL_IO_PRINT("Error:switch_value must be 0 or 1. \n");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    OAL_IO_PRINT("<debug_type>   <switch_value> \n");
    for (uc_idx = 0; uc_idx < MAX_DEBUG_TYPE_NUM; uc_idx++)
    {
        OAL_IO_PRINT("  %d          %d \n", uc_idx, g_aul_debug_feature_switch[uc_idx]);
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_DAQ

OAL_STATIC oal_uint32  wal_hipriv_data_acq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    while (' ' == *pc_param)
    {
        ++pc_param;
    }
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DATA_ACQ, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
       OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_data_acq::return err code[%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_get_smps_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_SMPS_INFO, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_smps_info::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_oam_output
 功能描述  : 设置oam模块的输出位置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月3日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_oam_output(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;

    /* OAM log模块的开关的命令: hipriv "Hisilicon0 log_level 0~3"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 取值:oam_output_type_enum_uint8 */
    l_tmp = oal_atoi(ac_name);
    if (l_tmp >= OAM_OUTPUT_TYPE_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::output type invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OAM_OUTPUT_TYPE,  OAL_SIZEOF(oal_int32));
    /* 设置配置命令参数 */
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_add_user
 功能描述  : 设置添加用户的配置命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月5日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_add_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_add_user_param_stru    *pst_add_user_param;
    mac_cfg_add_user_param_stru     st_add_user_param;  /* 临时保存获取的use的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置添加用户的配置命令: hipriv "vap0 add_user xx xx xx xx xx xx(mac地址) 0 | 1(HT能力位) "
        该命令针对某一个VAP
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_add_user_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取用户的HT标识 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对user的HT字段进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        st_add_user_param.en_ht_cap = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        st_add_user_param.en_ht_cap = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::the mod switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_USER, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    /* 设置配置命令参数 */
    pst_add_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_add_user_param->auc_mac_addr[ul_get_addr_idx] = st_add_user_param.auc_mac_addr[ul_get_addr_idx];
    }
    pst_add_user_param->en_ht_cap = st_add_user_param.en_ht_cap;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_del_user
 功能描述  : 设置删除用户的配置命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月5日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_del_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_del_user_param_stru    *pst_del_user_param;
    mac_cfg_del_user_param_stru     st_del_user_param;  /* 临时保存获取的use的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置删除用户的配置命令: hipriv "vap0 del_user xx xx xx xx xx xx(mac地址)"
        该命令针对某一个VAP
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_del_user_param.auc_mac_addr);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_USER, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    /* 设置配置命令参数 */
    pst_del_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_del_user_param->auc_mac_addr[ul_get_addr_idx] = st_del_user_param.auc_mac_addr[ul_get_addr_idx];
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_ampdu_start
 功能描述  : 开启ampdu调测命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月7日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_ampdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                 st_write_msg;
    oal_uint32                         ul_off_set;
    oal_int8                           ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                         ul_ret;
    oal_int32                          l_ret;
    mac_cfg_ampdu_start_param_stru    *pst_ampdu_start_param;
    mac_cfg_ampdu_start_param_stru     st_ampdu_start_param;  /* 临时保存获取的use的信息 */
    oal_uint32                         ul_get_addr_idx;

    /*
        设置AMPDU开启的配置命令: hipriv "Hisilicon0  ampdu_start xx xx xx xx xx xx(mac地址) tidno ack_policy"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_ampdu_start_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::the ampdu start command is erro [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_ampdu_start_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_start_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::the ampdu start command is error! uc_tidno is  [%d]!}\r\n", st_ampdu_start_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_START, OAL_SIZEOF(mac_cfg_ampdu_start_param_stru));

    /* 设置配置命令参数 */
    pst_ampdu_start_param = (mac_cfg_ampdu_start_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_ampdu_start_param->auc_mac_addr[ul_get_addr_idx] = st_ampdu_start_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_ampdu_start_param->uc_tidno = st_ampdu_start_param.uc_tidno;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_auto_ba_switch
 功能描述  : 开启或关闭amsdu ampdu联合发送
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : tanghuichao
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_ampdu_amsdu_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* 设置自动开始BA会话的开关:hipriv "vap0  auto_ba 0 | 1" 该命令针对某一个VAP */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_amsdu_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对AUTO BA进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_amsdu_switch::the auto ba switch command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_AMPDU_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_amsdu_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_auto_ba_switch
 功能描述  : 开启自动BA会话的开关
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_auto_ba_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* 设置自动开始BA会话的开关:hipriv "vap0  auto_ba 0 | 1" 该命令针对某一个VAP */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对AUTO BA进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::the auto ba switch command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AUTO_BA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_profiling_switch
 功能描述  : 开启自动BA会话的开关
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_profiling_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* 设置自动开始BA会话的开关:hipriv "vap0  profiling 0 | 1" 该命令针对某一个VAP */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对AUTO BA进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::the profiling switch command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROFILING_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_addba_req
 功能描述  : 建立BA会话的调测命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月7日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_addba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_addba_req_param_stru   *pst_addba_req_param;
    mac_cfg_addba_req_param_stru    st_addba_req_param;     /* 临时保存获取的addba req的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置AMPDU关闭的配置命令:
        hipriv "Hisilicon0 addba_req xx xx xx xx xx xx(mac地址) tidno ba_policy buffsize timeout"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_addba_req_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_addba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_addba_req_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error!uc_tidno is [%d]!}\r\n", st_addba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取ba_policy */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.en_ba_policy = (oal_uint8)oal_atoi(ac_name);
    if (MAC_BA_POLICY_IMMEDIATE != st_addba_req_param.en_ba_policy)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the ba policy is not correct! ba_policy is[%d]!}\r\n", st_addba_req_param.en_ba_policy);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取buffsize */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_buff_size = (oal_uint16)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /* 获取timeout时间 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_timeout = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADDBA_REQ, OAL_SIZEOF(mac_cfg_addba_req_param_stru));

    /* 设置配置命令参数 */
    pst_addba_req_param = (mac_cfg_addba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_addba_req_param->auc_mac_addr[ul_get_addr_idx] = st_addba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_addba_req_param->uc_tidno     = st_addba_req_param.uc_tidno;
    pst_addba_req_param->en_ba_policy = st_addba_req_param.en_ba_policy;
    pst_addba_req_param->us_buff_size = st_addba_req_param.us_buff_size;
    pst_addba_req_param->us_timeout   = st_addba_req_param.us_timeout;


    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_addba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_delba_req
 功能描述  : 删除BA会话的调测命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月7日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_delba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_delba_req_param_stru   *pst_delba_req_param;
    mac_cfg_delba_req_param_stru    st_delba_req_param;     /* 临时保存获取的addba req的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置AMPDU关闭的配置命令:
        hipriv "Hisilicon0 delba_req xx xx xx xx xx xx(mac地址) tidno direction reason_code"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_delba_req_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the delba_req req command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_delba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the delba_req req command is error! uc_tidno is[%d]!}\r\n", st_delba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取direction */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_delba_req_param.en_direction = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.en_direction >= MAC_BUTT_DELBA)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the direction is not correct! direction is[%d]!}\r\n", st_delba_req_param.en_direction);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DELBA_REQ, OAL_SIZEOF(mac_cfg_delba_req_param_stru));

    /* 设置配置命令参数 */
    pst_delba_req_param = (mac_cfg_delba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_delba_req_param->auc_mac_addr[ul_get_addr_idx] = st_delba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_delba_req_param->uc_tidno       = st_delba_req_param.uc_tidno;
    pst_delba_req_param->en_direction   = st_delba_req_param.en_direction;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_delba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_vap_info
 功能描述  : 打印vap的所有参数信息
 输入参数  : pst_net_dev : net_device
             pc_param    : 参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月28日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_user_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru                    *pst_mac_vap;
    wal_msg_write_stru              st_write_msg;
    oal_int32                      l_ret;
    mac_cfg_user_info_param_stru    *pst_user_info_param;
    oal_uint8                       auc_mac_addr[6] = {0};    /* 临时保存获取的use的mac地址信息 */
    oal_uint8                       uc_char_index;
    oal_uint16                      us_user_idx;

    /* 去除字符串的空格 */
    pc_param++;

    /* 获取mac地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++)
    {
        if (':' == *pc_param)
        {
            pc_param++;
            if (0 != uc_char_index)
            {
                uc_char_index--;
            }

            continue;
        }

        auc_mac_addr[uc_char_index/2] =
        (oal_uint8)(auc_mac_addr[uc_char_index/2] * 16 * (uc_char_index % 2) +
                                        oal_strtohex(pc_param));
        pc_param++;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USER_INFO, OAL_SIZEOF(mac_cfg_user_info_param_stru));

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    l_ret = (oal_int32)mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_user_info::no such user!}\r\n");
        return OAL_FAIL;
    }

    /* 设置配置命令参数 */
    pst_user_info_param              = (mac_cfg_user_info_param_stru *)(st_write_msg.auc_value);
    pst_user_info_param->us_user_idx = us_user_idx;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_info_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_user_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为多大的值 */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* 单播数据帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
/*****************************************************************************
 函 数 名  : wal_hipriv_set_mode_ucast_data_dscr_param
 功能描述  : 针对指定协议模式设置单播数据帧描述符参数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月1日
    作    者   : l00330043
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_mode_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析配置的协议模式 */
    if (!oal_strcmp("11ac", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_VHT_UCAST_DATA;
    }
    else if (!oal_strcmp("11n", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_HT_UCAST_DATA;
    }
    else if (!oal_strcmp("11ag", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11AG_UCAST_DATA;
    }
    else if (!oal_strcmp("11b", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11B_UCAST_DATA;
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param:: no such param for protocol_mode!}\r\n");
        return OAL_FAIL;
    }

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为多大的值 */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_mcast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为多大的值 */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* 组播数据帧描述符设置 tpye = MAC_VAP_CONFIG_MCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_bcast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为多大的值 */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* 广播数据帧描述符设置 tpye = MAC_VAP_CONFIG_BCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_BCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_ucast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_band;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /***************************************************************************
             sh hipriv.sh "vap0 set_ucast_mgmt data0 2 8389137"
    ***************************************************************************/
    /* 解析data0 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为哪个频段的单播管理帧 2G or 5G*/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);

    /* 单播管理帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_MGMT 2为2G,否则为5G  */
    if (WLAN_BAND_2G == uc_band)
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_2G;
    }
    else
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_5G;
    }

    /* 解析要设置为多大的速率 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_mbcast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_band;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /***************************************************************************
             sh hipriv.sh "vap0 set_mcast_mgmt data0 5 8389137"
    ***************************************************************************/
    /* 解析data0 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为哪个频段的单播管理帧 2G or 5G*/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);

    /* 单播管理帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_MGMT 2为2G,否则为5G  */
    if (WLAN_BAND_2G == uc_band)
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_2G;
    }
    else
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_5G;
    }

    /* 解析要设置为多大的速率 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  wal_hipriv_set_rate(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
//#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_non_ht_rate_stru         *pst_set_rate_param;
    wlan_legacy_rate_value_enum_uint8  en_rate_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RATE, OAL_SIZEOF(mac_cfg_non_ht_rate_stru));

    /* 解析并设置配置命令参数 */
    pst_set_rate_param = (mac_cfg_non_ht_rate_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rate::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析是设置为哪一级速率 */
    for (en_rate_index = 0; en_rate_index < WLAN_LEGACY_RATE_VALUE_BUTT; en_rate_index++)
    {
        if(!oal_strcmp(pauc_non_ht_rate_tbl[en_rate_index], ac_arg))
        {
            break;
        }
    }

    /* 根据速率配置TX描述符中的协议模式 */
    if (en_rate_index <= WLAN_SHORT_11b_11_M_BPS)
    {
        pst_set_rate_param->en_protocol_mode = WLAN_11B_PHY_PROTOCOL_MODE;
    }
    else if (en_rate_index >= WLAN_LEGACY_OFDM_48M_BPS && en_rate_index <= WLAN_LEGACY_OFDM_9M_BPS)
    {
        pst_set_rate_param->en_protocol_mode = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
    }
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rate::invalid rate!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 解析要设置为多大的值 */
    pst_set_rate_param->en_rate = en_rate_index;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_non_ht_rate_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rate::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
//#endif  /* _PRE_WLAN_CHIP_TEST */
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_mcs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
//#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_mcs_param;
    oal_int32                        l_mcs;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_idx = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MCS, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_mcs_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 解析要设置为多大的值 */
    l_mcs = oal_atoi(ac_arg);

    if (l_mcs < WAL_HIPRIV_HT_MCS_MIN || l_mcs > WAL_HIPRIV_HT_MCS_MAX)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input val out of range [%d]!}\r\n", l_mcs);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_mcs_param->uc_param = (oal_uint8)l_mcs;
    pst_set_mcs_param->en_protocol_mode = WLAN_HT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
//#endif  /* _PRE_WLAN_CHIP_TEST */
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_mcsac(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
//#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_mcs_param;
    oal_int32                        l_mcs;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_idx = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MCSAC, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_mcs_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 解析要设置为多大的值 */
    l_mcs = oal_atoi(ac_arg);

    if (l_mcs < WAL_HIPRIV_VHT_MCS_MIN || l_mcs > WAL_HIPRIV_VHT_MCS_MAX)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input val out of range [%d]!}\r\n", l_mcs);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_mcs_param->uc_param = (oal_uint8)l_mcs;
    pst_set_mcs_param->en_protocol_mode = WLAN_VHT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
//#endif  /* _PRE_WLAN_CHIP_TEST */
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_nss_param;
    oal_int32                        l_nss;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_idx = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_NSS, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_nss_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {

        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_nss::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 解析要设置为多大的值 */
    l_nss = oal_atoi(ac_arg);

    if (l_nss < WAL_HIPRIV_NSS_MIN || l_nss > WAL_HIPRIV_NSS_MAX)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::input val out of range [%d]!}\r\n", l_nss);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_nss_param->uc_param = (oal_uint8)(l_nss - 1);
    pst_set_nss_param->en_protocol_mode = WLAN_VHT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_rfch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_rfch_param;
    oal_uint8                        uc_ch;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                         c_ch_idx;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RFCH, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_rfch_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析要设置为多大的值 */
    uc_ch = 0;
    for (c_ch_idx = 0; c_ch_idx < WAL_HIPRIV_CH_NUM; c_ch_idx++)
    {
        if ('0' == ac_arg[c_ch_idx])
        {
            continue;
        }
        else if ('1' == ac_arg[c_ch_idx])
        {
            uc_ch += (oal_uint8)(1 << (WAL_HIPRIV_CH_NUM - c_ch_idx - 1));
        }
        /* 输入数据有非01数字，或数字少于4位，异常 */
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 输入参数多于四位，异常 */
    if ('\0' != ac_arg[c_ch_idx])
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_rfch_param->uc_param = uc_ch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
#endif
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_bw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
//#ifdef _PRE_WLAN_CHIP_TEST
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_bw_param;
    hal_channel_assemble_enum_uint8  en_bw_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BW, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_bw_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取带宽值字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析要设置为多大的值 */
    for (en_bw_index = 0; en_bw_index < WLAN_BAND_ASSEMBLE_AUTO; en_bw_index++)
    {
        if(!oal_strcmp(pauc_bw_tbl[en_bw_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (en_bw_index >= WLAN_BAND_ASSEMBLE_AUTO)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bw::not support this bandwidth!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_bw_param->uc_param = (oal_uint8)(en_bw_index);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
//#endif  /* _PRE_WLAN_CHIP_TEST */
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_always_tx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_bcast_param;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_switch_enum_uint8            en_tx_flag;
    wlan_tx_ack_policy_enum_uint8    en_ack_policy;
    mac_rf_payload_enum_uint8        en_payload_flag;
    oal_uint32                       ul_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_TX, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_bcast_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取常发模式开关标志 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::get tx_flag return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_tx_flag = (oal_uint8)oal_atoi(ac_arg);
    pst_set_bcast_param->uc_param = en_tx_flag;

    /* 获取ack_policy参数 */
    pc_param = pc_param + ul_off_set;
    ul_ret   = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);

    en_ack_policy = (wlan_tx_ack_policy_enum_uint8)oal_atoi(ac_arg);
    if (en_ack_policy >= WLAN_TX_NO_EXPLICIT_ACK)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::ack policy flag err[%d]!}\r\n", en_ack_policy);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    pst_set_bcast_param->en_ack_policy = en_ack_policy;

    /* 获取payload_flag参数 */
    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    en_payload_flag = (mac_rf_payload_enum_uint8)oal_atoi(ac_arg);
    if (en_payload_flag >= RF_PAYLOAD_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::payload flag err[%d]!}\r\n", en_payload_flag);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    pst_set_bcast_param->en_payload_flag = en_payload_flag;

    /* 获取len参数 */
    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    ul_len = (oal_uint32)oal_atoi(ac_arg);
    if (ul_len >= 65535)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::len [%u] overflow!}\r\n", ul_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    pst_set_bcast_param->ul_payload_len = ul_len;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
#endif
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
OAL_STATIC oal_uint32  wal_hipriv_always_tx_1102(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_bcast_param;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_switch_enum_uint8            en_tx_flag = OAL_SWITCH_OFF;
    mac_rf_payload_enum_uint8        en_payload_flag = RF_PAYLOAD_ALL_ZERO;
    oal_uint32                       ul_len = 0;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_TX_1102, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_bcast_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_tx_flag = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    if (OAL_SWITCH_OFF != en_tx_flag)
    {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        pc_param = pc_param + ul_off_set;
        en_payload_flag = (oal_uint8)oal_atoi(ac_name);
        if(en_payload_flag >= RF_PAYLOAD_BUTT)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::payload flag err[%d]!}\r\n", en_payload_flag);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        ul_len = (oal_uint16)oal_atoi(ac_name);
        if(65535 < ul_len)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::len [%u] overflow!}\r\n", ul_len);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
        pc_param += ul_off_set;


    }

    /* 关闭的情况下不需要解析后面的参数 */
    pst_set_bcast_param->en_payload_flag = en_payload_flag;
    pst_set_bcast_param->ul_payload_len = ul_len;
    pst_set_bcast_param->uc_param = en_tx_flag;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_always_rx
 功能描述  : 数据常收
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月28日
    作    者   : 曹海彬
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_always_rx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_rx_flag;
    oal_int32                        l_idx = 0;

    /* 获取常收模式开关标志 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_rx::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_always_rx::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 将命令参数值字符串转化为整数 */
    uc_rx_flag = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rx_flag;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_RX, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_rx::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

/*****************************************************************************
 函 数 名  : wal_hipriv_get_thruput
 功能描述  : 获取芯片吞吐量信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月28日
    作    者   : 曹海彬
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_get_thruput(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_stage;
    oal_int32                        l_idx = 0;

    /* 获取参数 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 将命令参数值字符串转化为整数 */
    uc_stage = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_stage;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_THRUPUT, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_freq_skew(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int8                    ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint16                  us_len;
    oal_uint16                  i;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    mac_cfg_freq_skew_stru      *pst_freq_skew;

    /*             命令格式: hipriv "Hisilicon0 set_freq_skew <>"
     * <idx chn T0Int20M T0Frac20M T1Int20M T1Frac20M T0Int40M T0Frac40M T1Int40M T1Frac40M>
     */
    pst_freq_skew = (mac_cfg_freq_skew_stru*)st_write_msg.auc_value;

    /* 索引值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_freq_skew->us_idx = (oal_uint16)oal_atoi(ac_arg);

    /* 信道 */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_freq_skew->us_chn = (oal_uint16)oal_atoi(ac_arg);

    /* 获取8个校正数据 */
    for (i = 0; i < WAL_HIPRIV_FREQ_SKEW_ARG_NUM; i++)
    {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pst_freq_skew->as_corr_data[i] = (oal_int16)oal_atoi(ac_arg);
    }

    us_len = OAL_SIZEOF(mac_cfg_freq_skew_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FREQ_SKEW, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_adjust_ppm
 功能描述  : 设置ppm
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月9日
    作    者   : pwx287475
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_adjust_ppm(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int8                    ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint16                  us_len;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    mac_cfg_adjust_ppm_stru     *pst_adjust_ppm;

    /* 命令格式: hipriv "Hisilicon0 adjust_ppm ppm band clock" */
    pst_adjust_ppm = (mac_cfg_adjust_ppm_stru*)st_write_msg.auc_value;

    /* ppm */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_adjust_ppm::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    if ('-' == ac_arg[0])
    {
        pst_adjust_ppm->c_ppm_val = -(oal_int8)oal_atoi(ac_arg+1);
    }
    else
    {
        pst_adjust_ppm->c_ppm_val = (oal_int8)oal_atoi(ac_arg);
    }


    /* clock */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        /* 此参数不配置，采用默认时钟配置，5G 26M 2G 40M */
        pst_adjust_ppm->uc_clock_freq = 0;
    }
    else
    {
        pst_adjust_ppm->uc_clock_freq = (oal_uint8)oal_atoi(ac_arg);
    }

    us_len = OAL_SIZEOF(mac_cfg_adjust_ppm_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADJUST_PPM, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_adjust_ppm::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_amsdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_amsdu_start_param_stru  *pst_amsdu_start_param;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_START, OAL_SIZEOF(mac_cfg_amsdu_start_param_stru));

    /* 解析并设置配置命令参数 */
    pst_amsdu_start_param = (mac_cfg_amsdu_start_param_stru *)(st_write_msg.auc_value);

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, pst_amsdu_start_param->auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->uc_amsdu_max_num     = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->us_amsdu_max_size    = (oal_uint16)oal_atoi(ac_name);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_amsdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_iwname
 功能描述  : 获得无线的名字, 用来确认无限扩展的存在.
 输入参数  : pst_net_dev: 指向net_device的指针
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年2月21日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_iwname(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_int8* pc_name, oal_int8* pc_extra)
{
    oal_int8    ac_iwname[] = "IEEE 802.11";
    oal_memcopy(pc_name, ac_iwname, OAL_SIZEOF(ac_iwname));

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_find_cmd
 功能描述  : 查找私有命令表
 输入参数  : pc_cmd_name: 命令名
 输出参数  : puc_cmd_id : 命令id
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月13日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_find_cmd(oal_int8 *pc_cmd_name, oal_uint8 *puc_cmd_id)
{
    oal_uint8                 en_cmd_idx;
    int                       l_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd_name) || (OAL_PTR_NULL == puc_cmd_id)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_find_cmd::pc_cmd_name/puc_cmd_id null ptr error [%d] [%d]!}\r\n", pc_cmd_name, puc_cmd_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (en_cmd_idx = 0; en_cmd_idx < OAL_ARRAY_SIZE(g_ast_hipriv_cmd); en_cmd_idx++)
    {
        l_ret = oal_strcmp(g_ast_hipriv_cmd[en_cmd_idx].pc_cmd_name, pc_cmd_name);

        if (0 == l_ret)
        {
            *puc_cmd_id = en_cmd_idx;

            return OAL_SUCC;
        }
    }

    OAM_IO_PRINTK("cmd name[%s] is not exist. \r\n", pc_cmd_name);
    return OAL_FAIL;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_get_cmd_net_dev
 功能描述  : 获取命令对应的net_dev
 输入参数  : pc_cmd: 命令字符串
 输出参数  : ppst_net_dev: 得到net_device
            pul_off_set: 取net_device偏移的字节
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月18日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_get_cmd_net_dev(oal_int8 *pc_cmd, oal_net_device_stru **ppst_net_dev, oal_uint32 *pul_off_set)
{
    oal_net_device_stru  *pst_net_dev;
    oal_int8              ac_dev_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32            ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd) || (OAL_PTR_NULL == ppst_net_dev) || (OAL_PTR_NULL == pul_off_set)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_net_dev::pc_cmd/ppst_net_dev/pul_off_set null ptr error [%d] [%d] [%d]!}\r\n", pc_cmd, ppst_net_dev, pul_off_set);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg(pc_cmd, ac_dev_name, pul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_net_dev::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_dev_name);
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_net_dev::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    oal_dev_put(pst_net_dev);

    *ppst_net_dev = pst_net_dev;

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_get_cmd_id
 功能描述  : 获取cmd id
 输入参数  : pc_cmd: 命令字符串
 输出参数  : ppst_net_dev: 得到net_device
             pul_off_set: 取net_device偏移的字节
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月18日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_get_cmd_id(oal_int8 *pc_cmd, oal_uint8 *puc_cmd_id, oal_uint32 *pul_off_set)
{
    oal_uint8                   en_cmd_id;
    oal_int8                    ac_cmd_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd) || (OAL_PTR_NULL == puc_cmd_id) || (OAL_PTR_NULL == pul_off_set)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::pc_cmd/puc_cmd_id/pul_off_set null ptr error [%d] [%d] [%d]!}\r\n", pc_cmd, puc_cmd_id, pul_off_set);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg(pc_cmd, ac_cmd_name, pul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 根据命令名找到命令枚举 */
    ul_ret = wal_hipriv_find_cmd(ac_cmd_name, &en_cmd_id);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::wal_hipriv_find_cmd return error cod [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    *puc_cmd_id = en_cmd_id;

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_parse_cmd
 功能描述  : 解析私有配置命令
 输入参数  : pc_cmd: 命令
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月11日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_parse_cmd(oal_int8 *pc_cmd)
{
    oal_net_device_stru        *pst_net_dev;
    oal_uint8                   en_cmd;
    oal_uint32                  ul_off_set = 0;
    oal_uint32                  ul_ret;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_uint32                  ul_num;
    oal_int8                    cmd_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
#endif
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_cmd))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::pc_cmd null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
        cmd格式约束
        网络设备名 命令      参数   Hisilicon0 create vap0
        1~15Byte   1~15Byte
    **************************** ***********************************************/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    //OAL_IO_PRINT("test_by_minos init\n");

    ul_ret = wal_hipriv_get_cmd_net_dev(pc_cmd, &pst_net_dev, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_net_dev return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_cmd += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_cmd, cmd_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if((0 == oal_strcmp(cmd_name, g_pac_wal_switch_info[0])) ||(0 == oal_strcmp(cmd_name, g_pac_wal_switch_info[1])) )
    {
        ul_ret  = wal_hipriv_get_cmd_id(pc_cmd, &en_cmd, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_id return error code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        pc_cmd += ul_off_set;

        for(ul_num=0; ul_num<(sizeof(g_pac_wal_all_vap)/sizeof(g_pac_wal_all_vap[0])); ul_num++ )
        {

            ul_ret = wal_hipriv_get_cmd_net_dev(g_pac_wal_all_vap[ul_num], &pst_net_dev, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_net_dev return error code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }
                    /* 调用命令对应的函数 */
            ul_ret = g_ast_hipriv_cmd[en_cmd].p_func(pst_net_dev, pc_cmd);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::g_ast_hipriv_cmd return error code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }
        }
    }
    else
    {
        ul_ret  = wal_hipriv_get_cmd_id(pc_cmd, &en_cmd, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_id return error code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        pc_cmd += ul_off_set;

        /* 调用命令对应的函数 */
        ul_ret = g_ast_hipriv_cmd[en_cmd].p_func(pst_net_dev, pc_cmd);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::g_ast_hipriv_cmd return error code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }

#else
    ul_ret = wal_hipriv_get_cmd_net_dev(pc_cmd, &pst_net_dev, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_net_dev return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_cmd += ul_off_set;
    ul_ret  = wal_hipriv_get_cmd_id(pc_cmd, &en_cmd, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_id return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_cmd += ul_off_set;

    /* 调用命令对应的函数 */
    ul_ret = g_ast_hipriv_cmd[en_cmd].p_func(pst_net_dev, pc_cmd);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::g_ast_hipriv_cmd return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
#endif
    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)&&(_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/*****************************************************************************
 函 数 名  : wal_hipriv_sys_write    hi1102-cb add sys for 51/02
 功能描述  : sys write函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 处理字节的长度
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月17日
    作    者   : chenchongbao
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_ssize_t  wal_hipriv_sys_write(oal_device_stru *dev, oal_device_attribute_stru *attr, const char *pc_buffer, oal_size_t count)
//OAL_STATIC oal_ssize_t  wal_hipriv_sys_write(oal_device_stru *dev, oal_device_attribute_stru *attr, const oal_int8 *pc_buffer, oal_size_t count)
{
    oal_int8  *pc_cmd;
    oal_uint32 ul_ret;
    oal_uint32 ul_len = (oal_uint32)count;

    if (ul_len > WAL_HIPRIV_CMD_MAX_LEN)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sys_write::ul_len>WAL_HIPRIV_CMD_MAX_LEN, ul_len [%d]!}\r\n", ul_len);
        return -OAL_EINVAL;
    }

    pc_cmd = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WAL_HIPRIV_CMD_MAX_LEN, OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_cmd))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_proc_write::alloc mem return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    OAL_MEMZERO(pc_cmd, WAL_HIPRIV_CMD_MAX_LEN);

    /*ul_ret = oal_copy_from_user(pc_cmd, pc_buffer, ul_len);*/
    oal_memcopy(pc_cmd, pc_buffer, ul_len);

    pc_cmd[ul_len - 1] = '\0';

    //printk("sys write: %s\n",pc_cmd);
    OAM_IO_PRINTK(" %s\n", pc_cmd);

    ul_ret = wal_hipriv_parse_cmd(pc_cmd);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::parse cmd return err code[%d]!}\r\n", ul_ret);
    }

    OAL_MEM_FREE(pc_cmd, OAL_TRUE);

    return (oal_int32)ul_len;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_sys_read    hi1102-cb add sys for 51/02
 功能描述  : sys read函数 空函数；防止编译警告
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 处理字节的长度
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月17日
    作    者   : chenchongbao
    修改内容   : 新生成函数

*****************************************************************************/
#define SYS_READ_MAX_STRING_LEN (4096-40)   /* 当前命令字符长度20字节内，预留40保证不会超出 */
OAL_STATIC oal_ssize_t  wal_hipriv_sys_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *pc_buffer)
{
    oal_uint32              ul_cmd_idx;
    oal_uint32              buff_index = 0;

    for (ul_cmd_idx = 0; ul_cmd_idx < OAL_ARRAY_SIZE(g_ast_hipriv_cmd); ul_cmd_idx++)
    {
        buff_index += OAL_SPRINTF(pc_buffer+buff_index,(SYS_READ_MAX_STRING_LEN-buff_index),
            "\t%s\n",g_ast_hipriv_cmd[ul_cmd_idx].pc_cmd_name);

        if(buff_index > SYS_READ_MAX_STRING_LEN){
            buff_index += OAL_SPRINTF(pc_buffer+buff_index,(SYS_READ_MAX_STRING_LEN-buff_index),"\tmore...\n");
            break;
        }
    }

    return buff_index;
}

#endif  /* _PRE_OS_VERSION_LINUX */

/*****************************************************************************
 函 数 名  : wal_hipriv_proc_write
 功能描述  : proc write函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 处理字节的长度
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_hipriv_proc_write(oal_file_stru *pst_file, const oal_int8 *pc_buffer, oal_uint32 ul_len, oal_void *p_data)
{
    oal_int8  *pc_cmd;
    oal_uint32 ul_ret;

    if (ul_len > WAL_HIPRIV_CMD_MAX_LEN)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::ul_len>WAL_HIPRIV_CMD_MAX_LEN, ul_len [%d]!}\r\n", ul_len);
        return -OAL_EINVAL;
    }

    pc_cmd = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WAL_HIPRIV_CMD_MAX_LEN, OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_cmd))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_proc_write::alloc mem return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    OAL_MEMZERO(pc_cmd, WAL_HIPRIV_CMD_MAX_LEN);

    ul_ret = oal_copy_from_user(pc_cmd, pc_buffer, ul_len);

    /* copy_from_user函数的目的是从用户空间拷贝数据到内核空间，失败返回没有被拷贝的字节数，成功返回0 */
    if (ul_ret > 0)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::oal_copy_from_user return ul_ret[%d]!}\r\n", ul_ret);
        OAL_MEM_FREE(pc_cmd, OAL_TRUE);

        return -OAL_EFAUL;
    }

    pc_cmd[ul_len - 1] = '\0';

    ul_ret = wal_hipriv_parse_cmd(pc_cmd);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::parse cmd return err code[%d]!}\r\n", ul_ret);
    }

    OAL_MEM_FREE(pc_cmd, OAL_TRUE);

    return (oal_int32)ul_len;
}

/*****************************************************************************
 函 数 名  : wal_create_hipriv_proc
 功能描述  : 创建proc入口
 输入参数  : p_proc_arg: 创建proc参数，此处不使用
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_create_proc(oal_void *p_proc_arg)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32           l_ret;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
        /* TBD */
        g_pst_proc_entry = OAL_PTR_NULL;
#else

    /* 420十进制对应八进制是0644 linux模式定义 S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); */
    /* S_IRUSR文件所有者具可读取权限, S_IWUSR文件所有者具可写入权限, S_IRGRP用户组具可读取权限, S_IROTH其他用户具可读取权限 */
    g_pst_proc_entry = oal_create_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, 420, NULL);
    if (OAL_PTR_NULL == g_pst_proc_entry)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_create_proc::oal_create_proc_entry return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    g_pst_proc_entry->data  = p_proc_arg;
    g_pst_proc_entry->nlink = 1;        /* linux创建proc默认值 */
    g_pst_proc_entry->read_proc  = OAL_PTR_NULL;

    g_pst_proc_entry->write_proc = (write_proc_t *)wal_hipriv_proc_write;

#endif
    /* hi1102-cb add sys for 51/02 */
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    gp_sys_kobject = oal_get_sysfs_root_object();
    if(NULL == gp_sys_kobject)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_create_proc::get sysfs root object failed!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    l_ret = sysfs_create_file(gp_sys_kobject,&dev_attr_hipriv.attr);
    return l_ret;
#else
    return OAL_SUCC;
#endif
}

/*****************************************************************************
 函 数 名  : wal_hipriv_remove_proc
 功能描述  : 删除proc
 输入参数  : 无
 输出参数  : 无
 返 回 值  : OAL_SUCC
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_remove_proc(void)
{
/* 卸载时删除sysfs */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    sysfs_remove_file(gp_sys_kobject,&dev_attr_hipriv.attr);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

    oal_remove_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, NULL);
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

    oal_remove_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, g_pst_proc_entry);
#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_mem_info
 功能描述  : 打印执行内存池信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_mem_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_int8                     auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_mem_pool_id_enum_uint8   en_pool_id;
    oal_uint32                   ul_off_set;
    oal_uint32                   ul_ret;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_mem_info::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取内存池ID */
    ul_ret = wal_get_cmd_one_arg(pc_param, auc_token, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_info::wal_get_cmd_one_arg return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);

    /* 打印内存池信息 */
    oal_mem_info(en_pool_id);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_mem_leak
 功能描述  : 检查指定内存池泄漏内存块(hipriv "Hisilicon0 memleak poolid")
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_int8                     auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_mem_pool_id_enum_uint8   en_pool_id;
    oal_uint32                   ul_off_set;
    oal_uint32                   ul_ret;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取内存池ID */
    ul_ret = wal_get_cmd_one_arg(pc_param, auc_token, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::wal_get_cmd_one_arg return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);
    if (en_pool_id > OAL_MEM_POOL_ID_SDT_NETBUF)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::mem pool id exceeds,en_pool_id[%d]!}\r\n", en_pool_id);
        return OAL_SUCC;
    }

    /* 检查内存池泄漏内存块 */
    oal_mem_leak(en_pool_id);

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_device_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                st_write_msg;
    oal_uint32                        ul_off_set;
    oal_int8                          ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                        ul_ret;
    oal_int32                         l_ret;
    oal_uint8                         uc_pool_id;
    mac_device_pool_id_stru          *pst_pool_id_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_device_mem_leak::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_pool_id = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEVICE_MEM_LEAK, OAL_SIZEOF(mac_device_pool_id_stru));

    /* 设置配置命令参数 */
    pst_pool_id_param = (mac_device_pool_id_stru *)(st_write_msg.auc_value);
    pst_pool_id_param->uc_pool_id   = uc_pool_id;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_device_pool_id_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_device_mem_leak::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_memory_info
 功能描述  : 检查host侧与device侧内存池使用状况
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月23日
    作    者   : l00324381
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_memory_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_uint32                        ul_off_set;
    oal_int8                          ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                        ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_device_mem_leak::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    if(0 == (oal_strcmp("host", ac_name)))
    {
        oal_mem_print_pool_info();
    }
    else if(0 == (oal_strcmp("device", ac_name)))
    {
        hcc_print_device_mem_info();
    }

    return OAL_SUCC;
}

#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32  wal_hipriv_pci_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8            *pc_token;
    oal_int8            *pc_end;
    oal_int8            *pc_ctx;
    oal_int8            *pc_sep = " ";
    oal_int32            ul_addr;
    oal_uint32           ul_val = 0;
    oal_uint8            uc_pci_device_id = 0;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pul_addr null ptr errorr [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_addr = (oal_int32)oal_strtol(pc_token, &pc_end, 16);

    /* 获取需要写入的值 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::ul_val null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    /* OAL_IO_PRINT("pci_reg_write: addr = 0x%x, val = 0x%x.\n", ul_addr, ul_val); */
    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "pci_reg_write: addr = 0x%08x, val = 0x%08x.\n", ul_addr, ul_val);
    //oam_print(ac_buf);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

    oal_pci_write_config_dword(pst_bus_chip->pst_pci_device, ul_addr, ul_val);

    OAL_IO_PRINT("pci_reg_write succ: addr = 0x%x, val = 0x%x.\n", ul_addr, ul_val);

    return OAL_SUCC;
}
oal_uint32  wal_hipriv_pci_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8       *pc_token;
    oal_int8       *pc_end;
    oal_int8       *pc_ctx;
    oal_int8       *pc_sep = " ";
    oal_int32       ul_addr;
    oal_uint32      ul_val = 0;
    oal_uint8       uc_pci_device_id = 0;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    /* 读取那个芯片的pcie */
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::ul_addr null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_addr = (oal_int32)oal_strtol(pc_token, &pc_end, 16);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pst_bus_chip is null，uc_pci_device_id[0].}\r\n", uc_pci_device_id);
        return OAL_FAIL;
    }

    oal_pci_read_config_dword(pst_bus_chip->pst_pci_device, ul_addr, &ul_val);

    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "pci_reg_read: addr=0x%08x, val=0x%08x.\n", ul_addr, ul_val);
    //oam_print(ac_buf);
    OAL_IO_PRINT("pci_reg_read succ:addr = 0x%x, val = 0x%x\n", ul_addr, ul_val);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_5115_reg_write
 功能描述  : 写5115侧的sys/pcie寄存器
             命令:sh hipriv.sh "Hisilicon0 regw5115 sys|pcie 0/1(如果是pcie的话要增加哪个芯片) <addr>(寄存器地址) <val>(写入的4字节值)"
             sys不区分芯片，只有一颗5115或者5610芯片，因此输入0和1都一样
 输入参数  : pst_net_dev: net_device
             pc_param   : 命令行输入参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月4日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_5115_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";
    oal_uint8             uc_pci_device_id = 0;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint32            ul_val = 0;
    oal_uint              ul_flag;
#endif

    struct wal_reg_write_stru
    {
        oal_int8     *pc_reg_type;
        oal_uint32    ul_addr;
        oal_uint32    ul_val;
    }st_reg_write = {0};

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

      /* 参数检查 */
    /*lint -e960*/
    if ((0 != oal_strcmp(pc_token, "0")) && (0 != oal_strcmp(pc_token, "1")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err pci_device_id!}\r\n");
        return OAL_FAIL;
    }

    /* 获取要读取的寄存器类型 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

      /* 参数检查 */
    if ((0 != oal_strcmp(pc_token, "sys")) && (0 != oal_strcmp(pc_token, "pcie")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err reg typer!}\r\n");
        return OAL_FAIL;
    }
    /*lint +e960*/
    st_reg_write.pc_reg_type = pc_token;

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_write.ul_addr = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    /* 获取需要写入的值 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_write.ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    //OAL_IO_PRINT("write 5115/5610 reg, addr = 0x%x, val = 0x%x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);
    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "write 5115/5610 reg, addr = 0x%08x, val = 0x%08x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);
    //oam_print(ac_buf);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if ((0 == oal_strcmp(st_reg_write.pc_reg_type, "sys")))   /* sys ctl */
    {
        oal_writel(st_reg_write.ul_val, g_pst_5115_sys_ctl + (st_reg_write.ul_addr - OAL_PCIE_SYS_BASE_PHYS));
    }
    else   /* chip id 0 */
    {
        oal_irq_save(&ul_flag, OAL_5115IRQ_WH5RW);

        if(0 == uc_pci_device_id)
        {
            /* 配置工作模式，写cpu侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_W_PCIE0);
            ul_val |= BIT21;
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_W_PCIE0);

            oal_writel(st_reg_write.ul_val, pst_bus_chip->p_pci_dbi_base + (st_reg_write.ul_addr - OAL_DBI_BASE_ADDR_0));

            /* 配置工作模式，恢复写wifi侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_W_PCIE0);
            ul_val &= (~BIT21);
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_W_PCIE0);
        }
        else
/* chip id 1 */
        {
            /* 配置工作模式，写cpu侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_W_PCIE1);
            ul_val |= BIT21;
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_W_PCIE1);

            oal_writel(st_reg_write.ul_val, pst_bus_chip->p_pci_dbi_base + (st_reg_write.ul_addr - OAL_DBI_BASE_ADDR_1));

            /* 配置工作模式，恢复写wifi侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_W_PCIE1);
            ul_val &= (~BIT21);
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_W_PCIE1);
        }

        oal_irq_restore(&ul_flag, OAL_5115IRQ_WH5RW);
    }
#endif

    OAL_IO_PRINT("write 5115/5610 reg succ, addr = 0x%x, val = 0x%x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_5115_reg_read
 功能描述  : 读5115侧的sys/pcie寄存器
             命令:sh hipriv.sh "Hisilicon0 regw5115 sys|pcie 0/1(如果是pcie的话要增加哪个芯片) <addr>(寄存器地址) <val>(写入的4字节值)"
             sys不区分芯片，只有一颗5115或者5610芯片，因此输入0和1都一样
 输入参数  : pst_net_dev: net_device
             pc_param   : 命令行输入参数
 输出参数  : 无
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月4日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_5115_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";
    oal_uint8            uc_pci_device_id = 0;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        oal_uint32            ul_val = 0;
        oal_uint              ul_flag;
#endif

    struct wal_reg_info_stru
    {
        oal_int8     *pc_reg_type;
        oal_uint32    ul_addr;
        oal_uint32    ul_val;
    }st_reg_info = {0};

     /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_read::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

      /* 参数检查 */
    /*lint -e960*/
    if ((0 != oal_strcmp(pc_token, "0")) && (0 != oal_strcmp(pc_token, "1")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err pci_device_id!}\r\n");
        return OAL_FAIL;
    }

    /* 获取要读取的寄存器类型 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    /* 参数检查 */
    if ((0 != oal_strcmp(pc_token, "sys")) && (0 != oal_strcmp(pc_token, "pcie")))
    {
        return OAL_FAIL;
    }
    /*lint +e960*/

    st_reg_info.pc_reg_type = pc_token;

    /* 获取地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_info.ul_addr = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if ((0 == oal_strcmp(st_reg_info.pc_reg_type, "sys")))   /* sys ctl */
    {
        st_reg_info.ul_val = oal_readl(g_pst_5115_sys_ctl + (st_reg_info.ul_addr - OAL_PCIE_SYS_BASE_PHYS));
    }
    else   /* pcie0 */
    {
        oal_irq_save(&ul_flag, OAL_5115IRQ_WH5RR);

        if(0 == uc_pci_device_id)
        {
            /* 配置工作模式，读cpu侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_R_PCIE0);
            ul_val |= BIT21;
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_R_PCIE0);

            st_reg_info.ul_val = oal_readl(pst_bus_chip->p_pci_dbi_base + (st_reg_info.ul_addr - OAL_DBI_BASE_ADDR_0));

            /* 配置工作模式，恢复读wifi侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_R_PCIE0);
            ul_val &= (~BIT21);
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_R_PCIE0);
        }
        else
        {
            /* 配置工作模式，读cpu侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_R_PCIE1);
            ul_val |= BIT21;
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_R_PCIE1);

            st_reg_info.ul_val = oal_readl(pst_bus_chip->p_pci_dbi_base + (st_reg_info.ul_addr - OAL_DBI_BASE_ADDR_1));

            /* 配置工作模式，恢复读wifi侧 */
            ul_val = oal_readl(g_pst_5115_sys_ctl + OAL_PERI_R_PCIE1);
            ul_val &= (~BIT21);
            oal_writel(ul_val, g_pst_5115_sys_ctl + OAL_PERI_R_PCIE1);
        }

        oal_irq_restore(&ul_flag, OAL_5115IRQ_WH5RR);
    }
#endif

    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "read 5115/5610 reg, addr = 0x%08x, val = 0x%08x.\n", st_reg_info.ul_addr, st_reg_info.ul_val);
    //oam_print(ac_buf);
    OAL_IO_PRINT("read 5115 reg succ, addr = 0x%x, val = 0x%x.\n", st_reg_info.ul_addr, st_reg_info.ul_val);

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_reg_info
 功能描述  : 查询寄存器值(hipriv "Hisilicon0 reginfo regtype(soc/mac/phy) startaddr endaddr")
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REG_INFO, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
/*****************************************************************************
 函 数 名  : wal_hipriv_sdio_flowctrl
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月30日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_sdio_flowctrl(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SDIO_FLOWCTRL, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sdio_flowctrl::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_list_ap
 功能描述  : 打印STA扫描到的AP列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_list_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_AP, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_ap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_list_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_STA, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_sta::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_list_channel(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_CHAN, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_channel::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_pwr;
    wal_msg_write_stru          st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_hipriv_set_regdomain_pwr, get arg return err %d", ul_ret);
        return ul_ret;
    }

    l_pwr = oal_atoi(ac_name);
    if (l_pwr <= 0 || l_pwr > 100)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "invalid value, %d", l_pwr);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REGDOMAIN_PWR, OAL_SIZEOF(mac_cfg_regdomain_max_pwr_stru));

    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->uc_pwr        = (oal_uint8)l_pwr;
    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->en_exceed_reg = OAL_FALSE;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr_priv(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_uint32                  ul_pwr;
    wal_msg_write_stru          st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_hipriv_set_regdomain_pwr, get arg return err %d", ul_ret);
        return ul_ret;
    }

    ul_pwr = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REGDOMAIN_PWR, OAL_SIZEOF(oal_int32));

    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->uc_pwr        = (oal_uint8)ul_pwr;
    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->en_exceed_reg = OAL_TRUE;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_event_queue_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return frw_event_queue_info();
}


OAL_STATIC oal_uint32  wal_hipriv_dump_all_rx_dscr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_ALL_RX_DSCR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_all_rx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_start_scan
 功能描述  : 触发sta初始扫描
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_start_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                    uc_is_p2p0_scan;
#endif  /* _PRE_WLAN_FEATURE_P2P */

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_SCAN, OAL_SIZEOF(oal_int32));

#ifdef _PRE_WLAN_FEATURE_P2P
    uc_is_p2p0_scan = (oal_memcmp(pst_net_dev->name, "p2p0", OAL_STRLEN("p2p0")) == 0)?1:0;
    //uc_is_p2p0_scan = (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_P2P_DEVICE)?1:0;
    st_write_msg.auc_value[0] = uc_is_p2p0_scan;
#endif  /* _PRE_WLAN_FEATURE_P2P */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_scan::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_start_scan
 功能描述  : 触发sta初始扫描
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_start_join(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    oal_uint32                       ul_ret;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_JOIN, OAL_SIZEOF(oal_int32));

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_memcopy(st_write_msg.auc_value, ac_name, WAL_MSG_WRITE_MAX_LEN);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::return err codereturn err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_start_deauth
 功能描述  : 触发去认证
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_start_deauth(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_DEAUTH, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_deauth::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_start_deauth
 功能描述  : 触发sta初始扫描
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_dump_timer(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TIEMR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_timer::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_kick_user
 功能描述  : 删除1个用户
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_kick_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};

    /* 去关联1个用户的命令 hipriv "vap0 kick_user xx:xx:xx:xx:xx:xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* 设置配置命令参数 */
    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, auc_mac_addr);

    /* 填写去关联reason code */
    pst_kick_user_param->us_reason_code = MAC_UNSPEC_REASON;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_pause_tid
 功能描述  : 删除1个用户
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_pause_tid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    oal_uint8                       uc_tid;
    /* 去关联1个用户的命令 hipriv "vap0 kick_user xx xx xx xx xx xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PAUSE_TID, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_pause_tid_param->uc_is_paused = (oal_uint8)oal_atoi(ac_name);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_user_vip
 功能描述  : 设置用户的vip flag;
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年07月22日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_user_vip(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_user_vip_param_stru    *pst_user_vip_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8                       uc_vip_flag;

    /* 设置用户为vip用户: 0 代表非VIP用户，1代表VIP用户
       sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx 0|1" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_vip_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_USER_VIP, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_user_vip_param = (mac_cfg_user_vip_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_user_vip_param->auc_mac_addr, auc_mac_addr);
    pst_user_vip_param->uc_vip_flag = uc_vip_flag;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_vip_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_set_vap_host
 功能描述  : 设置vap的host flag;
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年07月22日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_vap_host(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_host_flag;

    /* 设置vap的host flag: 0 代表guest vap, 1代表host vap
       sh hipriv.sh "vap0 set_host 0|1" */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_host_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VAP_HOST, OAL_SIZEOF(oal_uint8));

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_host_flag;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_send_bar
 功能描述  : 指定用户指定tid 发送bar
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_send_bar(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    oal_uint8                       uc_tid;

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_BAR, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_ampdu_tx_on
 功能描述  : 开启或关闭ampdu发送功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
OAL_STATIC oal_uint32  wal_hipriv_set_oma(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_set_oma_param_stru     *pst_set_oma_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];

    /* 设置Proxy STA 的OMA地址命令 sh hipriv.sh "vap0 set_vma xx xx xx xx xx xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_PROXYSTA, "{wal_hipriv_set_oma::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OMA, OAL_SIZEOF(mac_cfg_set_oma_param_stru));

    /* 设置配置命令参数 */
    pst_set_oma_param = (mac_cfg_set_oma_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_set_oma_param->auc_mac_addr, auc_mac_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_oma_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_oma::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_proxysta_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* proxysta模块的开关的命令: hipriv "Hisilicon0 proxysta_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /* 针对解析出的不同命令，对proxysta模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYSTA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_DFT_REG
OAL_STATIC oal_uint32  wal_hipriv_dump_reg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_type_enum_uint8         en_reg_type = 0;
    oal_uint32                      ul_reg_subtype = 0;
    oal_uint8                       uc_flag = 0;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    if ((0 != oal_strcmp(ac_name, "soc"))
    && (0 != oal_strcmp(ac_name, "mac"))
    && (0 != oal_strcmp(ac_name, "phy"))
    && (0 != oal_strcmp(ac_name, "abb"))
    && (0 != oal_strcmp(ac_name, "rf")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "soc"))
    {
        en_reg_type = OAM_REG_SOC;
    }
    if (0 == oal_strcmp(ac_name, "mac"))
    {
        en_reg_type = OAM_REG_MAC;
    }
    if (0 == oal_strcmp(ac_name, "phy"))
    {
        en_reg_type = OAM_REG_PHY;
    }
    if (0 == oal_strcmp(ac_name, "abb"))
    {
        en_reg_type = OAM_REG_ABB;
    }
    if (0 == oal_strcmp(ac_name, "rf"))
    {
        en_reg_type = OAM_REG_RF;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    ul_reg_subtype = (oal_uint32)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_flag = (oal_uint8)oal_atoi(ac_name);

    oam_reg_set_flag(en_reg_type, ul_reg_subtype, uc_flag);
    OAL_IO_PRINT("dump reg: regtype %u, subtype %u,uc flag %u\n",
                        en_reg_type,
                        ul_reg_subtype,
                        uc_flag);
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_evt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_evt_enum_uint32         en_evt_type = 0;
    oal_uint32                      ul_tick = 0;
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    /* 获取事件类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    if ((0 != oal_strcmp(ac_name, "tbtt"))
    && (0 != oal_strcmp(ac_name, "rx"))
    && (0 != oal_strcmp(ac_name, "tx"))
    && (0 != oal_strcmp(ac_name, "prd"))
    && (0 != oal_strcmp(ac_name, "usr"))
    && (0 != oal_strcmp(ac_name, "all"))
    && (0 != oal_strcmp(ac_name, "err")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "tbtt"))
    {
        en_evt_type = OAM_REG_EVT_TBTT;
    }
    if (0 == oal_strcmp(ac_name, "rx"))
    {
        en_evt_type = OAM_REG_EVT_RX;
    }
    if (0 == oal_strcmp(ac_name, "tx"))
    {
        en_evt_type = OAM_REG_EVT_TX;
    }
    if (0 == oal_strcmp(ac_name, "prd"))
    {
        en_evt_type = OAM_REG_EVT_PRD;
    }
    if (0 == oal_strcmp(ac_name, "usr"))
    {
        en_evt_type = OAM_REG_EVT_USR;
    }
    if (0 == oal_strcmp(ac_name, "err"))
    {
        en_evt_type = OAM_REG_EVT_ERR;
    }
    if (0 == oal_strcmp(ac_name, "all"))
    {
        en_evt_type = OAM_REG_EVT_BUTT;
    }

    /* 获取tick */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    ul_tick = (oal_uint32)oal_atoi(ac_name);

    oam_reg_set_evt(en_evt_type, ul_tick);

    if(OAM_REG_EVT_USR != en_evt_type)
    {
        return OAL_SUCC;
    }

    /***************************************************************************
                             抛事件到wal层触发数据刷新和上报
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_REG, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_dump_reg_addr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_type_enum_uint8         en_reg_type = 0;
    oal_uint8                       uc_flag = 0;
    oal_uint32                      ul_addr_start = 0;
    oal_uint32                      ul_addr_end = 0;
    oal_int8                       *pc_end;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    if ((0 != oal_strcmp(ac_name, "soc"))
    && (0 != oal_strcmp(ac_name, "mac"))
    && (0 != oal_strcmp(ac_name, "phy"))
    && (0 != oal_strcmp(ac_name, "abb"))
    && (0 != oal_strcmp(ac_name, "rf")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "soc"))
    {
        en_reg_type = OAM_REG_SOC;
    }
    if (0 == oal_strcmp(ac_name, "mac"))
    {
        en_reg_type = OAM_REG_MAC;
    }
    if (0 == oal_strcmp(ac_name, "phy"))
    {
        en_reg_type = OAM_REG_PHY;
    }
    if (0 == oal_strcmp(ac_name, "abb"))
    {
        en_reg_type = OAM_REG_ABB;
    }
    if (0 == oal_strcmp(ac_name, "rf"))
    {
        en_reg_type = OAM_REG_RF;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    //ul_addr_start = (oal_uint32)oal_atoi(ac_name);
    ul_addr_start = (oal_uint32)oal_strtol(ac_name, &pc_end, 16);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    //ul_addr_end = (oal_uint32)oal_atoi(ac_name);
    ul_addr_end = (oal_uint32)oal_strtol(ac_name, &pc_end, 16);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_flag = (oal_uint8)oal_atoi(ac_name);

    if((ul_addr_start % 4)
        || (ul_addr_end % 4)
        || (ul_addr_start > ul_addr_end))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::start %u, end %u Err [%d] [%d]!}\r\n", ul_addr_start, ul_addr_end);
        return OAL_FAIL;
    }
    oam_reg_set_flag_addr(en_reg_type, ul_addr_start, ul_addr_end, uc_flag);
    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_dump_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oam_reg_info();
    return OAL_SUCC;
}


#endif


OAL_STATIC oal_uint32  wal_hipriv_frag_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    mac_cfg_frag_threshold_stru    *pst_threshold;
    oal_uint32                      ul_threshold = 0;

    /* 获取分片门限 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_threshold = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    if ((WLAN_FRAG_THRESHOLD_MIN > ul_threshold) ||
        (WLAN_FRAG_THRESHOLD_MAX < ul_threshold))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::ul_threshold value error [%d]!}\r\n", ul_threshold);
        return OAL_FAIL;
    }

    pst_threshold = (mac_cfg_frag_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_frag_threshold = ul_threshold;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_frag_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FRAG_THRESHOLD_REG, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_wmm_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_wmm = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WMM_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::return err code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_hide_ssid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_hide_ssid = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_hide_ssid = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_hide_ssid;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_HIDE_SSID, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
/*****************************************************************************
 函 数 名  : wal_hipriv_resume_rx_intr_fifo
 功能描述  : 使能rx intr fifo命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_resume_rx_intr_fifo(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    mac_cfg_resume_rx_intr_fifo_stru   *pst_param;
    oal_uint8                           uc_is_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_resume_rx_intr_fifo::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_is_on = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESUME_RX_INTR_FIFO, OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru));

    /* 设置配置命令参数 */
    pst_param = (mac_cfg_resume_rx_intr_fifo_stru *)(st_write_msg.auc_value);
    pst_param->uc_is_on = uc_is_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_resume_rx_intr_fifo::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif
/*****************************************************************************
 函 数 名  : wal_hipriv_ampdu_tx_on
 功能描述  : 开启或关闭ampdu发送功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_ampdu_tx_on
 功能描述  : 开启或关闭ampdu发送功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_txbf_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TXBF_SWITCH, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_reset_device
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月6日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_reset_device(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW,us_len );

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_device::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_reset_operate(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);


    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW_OPERATE, us_len);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_operate::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_UAPSD
/*****************************************************************************
 函 数 名  : wal_hipriv_uapsd_debug
 功能描述  : uapsd维测命令
 输入参数  : WLAN_CFGID_UAPSD_DEBUG
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月6日
    作    者   : zourong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_uapsd_debug(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_UAPSD_DEBUG,us_len );

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_uapsd_debug::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_hipriv_set_phy_stat_en(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oam_stats_phy_node_idx_stru     st_phy_stat_node_idx;
    oal_uint8                       uc_loop;

    /* sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4" */
    for (uc_loop = 0; uc_loop < OAM_PHY_STAT_NODE_ENABLED_MAX_NUM; uc_loop++)
    {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_stat_en::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        st_phy_stat_node_idx.auc_node_idx[uc_loop] = (oal_uint8)oal_atoi(ac_name);

        /* 检查参数是否合法，参数范围是1~16 */
        if (st_phy_stat_node_idx.auc_node_idx[uc_loop] < OAM_PHY_STAT_ITEM_MIN_IDX
            || st_phy_stat_node_idx.auc_node_idx[uc_loop] > OAM_PHY_STAT_ITEM_MAX_IDX)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                             "{wal_hipriv_set_phy_stat_en::stat_item_idx invalid! should between 1 and 16!}.",
                             st_phy_stat_node_idx.auc_node_idx[uc_loop]);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        pc_param += ul_off_set;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PHY_STAT_EN, OAL_SIZEOF(st_phy_stat_node_idx));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_phy_stat_node_idx, OAL_SIZEOF(st_phy_stat_node_idx));

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_phy_stat_node_idx),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_stat_en::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_dbb_env_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_param;

    /* sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DBB_ENV_PARAM, OAL_SIZEOF(uc_param));

    /* 填写消息体，参数 */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_usr_queue_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_usr_queue_param_stru    st_usr_queue_param;

    /* sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_usr_queue_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_hipriv_get_mac_addr return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_usr_queue_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USR_QUEUE_STAT, OAL_SIZEOF(st_usr_queue_param));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_usr_queue_param, OAL_SIZEOF(st_usr_queue_param));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_usr_queue_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_report_vap_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_param;

    /* sh hipriv.sh "vap_name vap _stat  0|1" */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_STAT, OAL_SIZEOF(uc_param));

    /* 填写消息体，参数 */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_report_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /* sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all)  0|1" */
    /* 获取repot类型 */
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));
    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';
    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALL_STAT, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_all_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_report_ampdu_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_stat_stru         st_ampdu_param;

    /* sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_ampdu_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_hipriv_get_mac_addr return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_param.uc_tid_no = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_param.uc_tid_no > WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::input tid_no invalid. tid_no = [%d]!}\r\n", st_ampdu_param.uc_tid_no);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_ampdu_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_AMPDU_STAT, OAL_SIZEOF(st_ampdu_param));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_ampdu_param, OAL_SIZEOF(st_ampdu_param));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif
OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_aggr_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru       st_write_msg;
    oal_uint32               ul_off_set;
    oal_int8                 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_aggr_num_stru    st_aggr_num_ctl;
    oal_uint32               ul_ret;
    oal_int32                l_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_aggr_num_ctl.uc_aggr_num_switch = (oal_uint8)oal_atoi(ac_name);
    if (0 == st_aggr_num_ctl.uc_aggr_num_switch)
    {
        /* 不指定聚合个数时，聚合个数恢复为0 */
        st_aggr_num_ctl.uc_aggr_num = 0;
    }
    else
    {
        /* 获取聚合个数 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        st_aggr_num_ctl.uc_aggr_num = (oal_uint8)oal_atoi(ac_name);

        /* 超过聚合最大限制判断 */
        if (st_aggr_num_ctl.uc_aggr_num > WLAN_AMPDU_TX_MAX_NUM)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::exceed max aggr num [%d]!}\r\n", st_aggr_num_ctl.uc_aggr_num);
            return ul_ret;
        }
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AGGR_NUM, OAL_SIZEOF(st_aggr_num_ctl));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_aggr_num_ctl, OAL_SIZEOF(st_aggr_num_ctl));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_aggr_num_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_hipriv_freq_adjust(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                       *pc_token;
    oal_int8                       *pc_end;
    oal_int8                       *pc_ctx;
    oal_int8                       *pc_sep = " ";
    mac_cfg_freq_adjust_stru        st_freq_adjust_ctl;

    /* 获取整数分频 */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_int = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /* 获取小数分频 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_frac = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FREQ_ADJUST, OAL_SIZEOF(st_freq_adjust_ctl));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_freq_adjust_ctl, OAL_SIZEOF(st_freq_adjust_ctl));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_freq_adjust_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_freq_adjust::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
OAL_STATIC oal_uint32  wal_hipriv_set_stbc_cap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* STBC设置开关的命令: hipriv "vap0 set_stbc_cap 0 | 1"
            此处将解析出"1"或"0"存入ac_name
    */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::the set stbc command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STBC_CAP, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
OAL_STATIC oal_uint32  wal_hipriv_set_ldpc_cap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* LDPC设置开关的命令: hipriv "vap0 set_ldpc_cap 0 | 1"
            此处将解析出"1"或"0"存入ac_name
    */

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::the set ldpc command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LDPC_CAP, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_dump_rx_dscr
 功能描述  : dump接收描述符队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月2日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_dump_rx_dscr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_RX_DSCR, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_dump_tx_dscr
 功能描述  : dump发送描述符
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月4日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_dump_tx_dscr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TX_DSCR, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_dump_memory
 功能描述  : dump内存
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月4日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_dump_memory(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_addr[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        ac_len[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_len;
    oal_uint32                      ul_addr;
    mac_cfg_dump_memory_stru       *pst_cfg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_len, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 地址字符串转成16位地址 */
    ul_addr = (oal_uint32)oal_strtol(ac_addr, 0, 16);
    ul_len  = (oal_uint32)oal_atoi(ac_len);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_MEMORY, OAL_SIZEOF(mac_cfg_dump_memory_stru));

    /* 设置配置命令参数 */

    pst_cfg = (mac_cfg_dump_memory_stru *)(st_write_msg.auc_value);

    pst_cfg->ul_addr = ul_addr;
    pst_cfg->ul_len  = ul_len;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_dump_memory_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_show_tx_dscr_addr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_DEBUG_MODE
    oal_uint32                   ul_mem_idx;
    oal_uint16                   us_tx_dscr_idx;
    oal_mempool_tx_dscr_addr     *pst_tx_dscr_addr;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tx_dscr_addr = oal_mem_get_tx_dscr_addr();
    if (OAL_PTR_NULL == pst_tx_dscr_addr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_tx_dscr_addr is NULL!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("Allocated addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < pst_tx_dscr_addr->us_tx_dscr_cnt; ul_mem_idx++)
    {
        OAL_IO_PRINT("[%d]0x%x\t", ul_mem_idx, (oal_uint32)pst_tx_dscr_addr->ul_tx_dscr_addr[ul_mem_idx]);
    }
    OAL_IO_PRINT("\n");

    OAL_IO_PRINT("Released addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++)
    {
        if (0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr)
        {
            OAL_IO_PRINT("Addr:0x%x\tFileId:%d\tLineNum:%d\tTimeStamp:%u\n",
                        (oal_uint32)pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_file_id,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_line_num,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_ts);
        }
    }

    OAL_IO_PRINT("Tx complete int:\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++)
    {
        if (0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr)
        {
            OAL_IO_PRINT("Up tx addr:0x%x\tts:%u  |  Dn tx addr:0x%x\tts:%u\n",
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_up_intr_ts,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_dn_intr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_dn_intr_ts);
            OAL_IO_PRINT("tx dscr in q[%d] mpdu_num[%d]:", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_q_num,
                                                            pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_mpdu_num);
            for (us_tx_dscr_idx = 0; 0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[us_tx_dscr_idx] && us_tx_dscr_idx < OAL_MAX_TX_DSCR_CNT_IN_LIST; us_tx_dscr_idx++)
            {
                OAL_IO_PRINT("0x%x\t", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[us_tx_dscr_idx]);
            }
            OAL_IO_PRINT("\n-------------------------------------------\n");
        }
    }
#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_reg_write
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月6日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REG_WRITE, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_write::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dump_ba_bitmap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_tid;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN];

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_BA_BITMAP, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_packet_xmit
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月6日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_packet_num;
    oal_uint8                       uc_tid;
    oal_uint16                      uc_packet_len;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN];

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    if(uc_tid >= WLAN_TID_MAX_NUM)
    {
         return OAL_FAIL;
    }
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_packet_num = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_packet_len = (oal_uint16)oal_atoi(ac_name);
    if(uc_packet_len < 30)
    {
        return OAL_FAIL;
    }
    pc_param += ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PACKET_XMIT, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_packet_num = uc_packet_num;
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    pst_aggr_tx_on_param->us_packet_len = uc_packet_len;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_alg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;  //FIXME : st_write_msg can only carry bytes less than 48
    oal_int32           l_ret;
    oal_uint32          ul_off_set;
    oal_int8            ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8           *pc_tmp = (oal_int8 *)pc_param;
    oal_uint16          us_config_len;
    oal_uint16          us_param_len;

    mac_ioctl_alg_config_stru   st_alg_config;

    st_alg_config.uc_argc = 0;
    while(OAL_SUCC == wal_get_cmd_one_arg(pc_tmp, ac_arg, &ul_off_set))
    {
        st_alg_config.auc_argv_offset[st_alg_config.uc_argc] = (oal_uint8)((oal_uint8)(pc_tmp - pc_param) + (oal_uint8)ul_off_set - (oal_uint8)OAL_STRLEN(ac_arg));
        pc_tmp += ul_off_set;
        st_alg_config.uc_argc++;

        if(st_alg_config.uc_argc > DMAC_ALG_CONFIG_MAX_ARG)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_hipriv_alg error, argc too big [%d]!}\r\n", st_alg_config.uc_argc);
            return OAL_FAIL;
        }
    }

    if(0 == st_alg_config.uc_argc)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_alg::argc=0!}\r\n");
        return OAL_FAIL;
    }

    us_param_len = (oal_uint16)OAL_STRLEN(pc_param);
    if(us_param_len > WAL_MSG_WRITE_MAX_LEN - 1 - OAL_SIZEOF(mac_ioctl_alg_config_stru) )
    {
        return OAL_FAIL;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    us_config_len = OAL_SIZEOF(mac_ioctl_alg_config_stru) + us_param_len+ 1;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG, us_config_len);
    oal_memcopy(st_write_msg.auc_value, &st_alg_config, OAL_SIZEOF(mac_ioctl_alg_config_stru));
    oal_memcopy(st_write_msg.auc_value + OAL_SIZEOF(mac_ioctl_alg_config_stru), pc_param, us_param_len + 1);
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_config_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFS
/*****************************************************************************
 函 数 名  : wal_hipriv_dfs_radartool
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月5日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_dfs_radartool(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_uint16            us_len;
    oal_int32             l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RADARTOOL, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dfs_radartool::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif   /* end of _PRE_WLAN_FEATURE_DFS */
#ifdef _PRE_SUPPORT_ACS
/*****************************************************************************
 函 数 名  : wal_hipriv_acs
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月22日
    作    者   : gaolin
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_acs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_uint16            us_len;
    oal_int32             l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ACS_CONFIG, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_acs::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_show_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

    oam_stats_report_info_to_sdt(OAM_OTA_TYPE_DEV_STAT_INFO);
    oam_stats_report_info_to_sdt(OAM_OTA_TYPE_VAP_STAT_INFO);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_show_vap_pkt_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    /***************************************************************************
                                 抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_PKT_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_vap_pkt_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_clear_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oam_stats_clear_stat_info();
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_user_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    /* sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    l_tmp = oal_atoi(ac_name);
    if ((l_tmp < 0) || (l_tmp >= WLAN_ACTIVE_USER_MAX_NUM))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::user id invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    oam_stats_report_usr_info((oal_uint16)l_tmp);
#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_show_irq_info
 功能描述  : 打印各类中断次数信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月8日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_timer_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_timer_switch;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);

         return ul_ret;
    }

    uc_timer_switch = (oal_uint8)oal_atoi(ac_name);
    if (uc_timer_switch >= 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::invalid choicee [%d]!}\r\n", uc_timer_switch);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TIMER_START, OAL_SIZEOF(oal_uint8));

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_timer_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_show_irq_info
 功能描述  : 打印各类中断次数信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月8日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_show_profiling(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_PROFILING_MODE

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);

         return ul_ret;
    }

    ul_value = (oal_uint32)oal_atoi(ac_name);
    if (ul_value >= OAM_PROFILING_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::invalid choicee [%d]!}\r\n", ul_value);

        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_PROFILING, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#endif

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_alg_cfg
 功能描述  : 算法模块参数配置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

        配置算法的配置命令: hipriv "vap0 alg_cfg vi_sch_limit 10"
        该命令针对某一个VAP

 修改历史      :
  1.日    期   : 2013年10月25日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_alg_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    mac_ioctl_alg_param_stru       *pst_alg_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru          st_alg_cfg;
    oal_uint8                       uc_map_index = 0;
    oal_int32                       l_ret;

    pst_alg_param = (mac_ioctl_alg_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    /* 获取参数配置值 */
    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 记录参数配置值 */
    pst_alg_param->ul_value = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_tpc_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_tpc_log_param_stru       *pst_alg_tpc_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;

    pst_alg_tpc_log_param = (mac_ioctl_alg_tpc_log_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_tpc_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    /* 区分获取特定帧功率和统计日志命令处理:获取功率只需获取帧名字 */
    if (MAC_ALG_CFG_TPC_GET_FRAME_POW == pst_alg_tpc_log_param->en_alg_cfg)
    {
        /* 获取配置参数名称 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);

        /* 记录命令对应的帧名字 */
        pst_alg_tpc_log_param->pc_frame_name = ac_name;
        en_stop_flag = OAL_TRUE;
    }
    else
    {
        ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_tpc_log_param->auc_mac_addr, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_hipriv_get_mac_addr failed!}\r\n");
            return ul_ret;
        }
        pc_param += ul_off_set;

        while ((' ' == *pc_param) || ('\0' == *pc_param))
        {
            if ('\0' == *pc_param)
            {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++ pc_param;
        }

        /* 获取业务类型值 */
        if (OAL_TRUE != en_stop_flag)
        {
            ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_alg_tpc_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
            pc_param = pc_param + ul_off_set;

            en_stop_flag = OAL_FALSE;
            while ((' ' == *pc_param) || ('\0' == *pc_param))
            {
                if ('\0' == *pc_param)
                {
                    en_stop_flag = OAL_TRUE;
                    break;
                }
                ++ pc_param;
            }

            if (OAL_TRUE != en_stop_flag)
            {
                /* 获取参数配置值 */
                ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
                if (OAL_SUCC != ul_ret)
                {
                    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
                    return ul_ret;
                }

                /* 记录参数配置值 */
                pst_alg_tpc_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
            }
        }
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_tpc_log_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_tpc_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_ar_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_ar_log_param_stru         *pst_alg_ar_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;

    pst_alg_ar_log_param = (mac_ioctl_alg_ar_log_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_ar_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_ar_log_param->auc_mac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_off_set;

    while ((' ' == *pc_param) || ('\0' == *pc_param))
    {
        if ('\0' == *pc_param)
        {
            en_stop_flag = OAL_TRUE;
            break;
        }
        ++ pc_param;
    }

    /* 获取业务类型值 */
    if (OAL_TRUE != en_stop_flag)
    {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        pst_alg_ar_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
        pc_param = pc_param + ul_off_set;

        en_stop_flag = OAL_FALSE;
        while ((' ' == *pc_param) || ('\0' == *pc_param))
        {
            if ('\0' == *pc_param)
            {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++ pc_param;
        }

        if (OAL_TRUE != en_stop_flag)
        {
            /* 获取参数配置值 */
            ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            /* 记录参数配置值 */
            pst_alg_ar_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        }
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

OAL_STATIC oal_uint32  wal_hipriv_txbf_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_txbf_log_param_stru      *pst_alg_txbf_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;
    pst_alg_txbf_log_param = (mac_ioctl_alg_txbf_log_param_stru *)(st_write_msg.auc_value);
    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }
    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }
    /* 记录命令对应的枚举值 */
    pst_alg_txbf_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;
    ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_txbf_log_param->auc_mac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_off_set;
    while ((' ' == *pc_param))
    {
        ++ pc_param;
        if ('\0' == *pc_param)
        {
            en_stop_flag = OAL_TRUE;
            break;
        }
    }
    /* 获取参数配置值 */
    if (OAL_TRUE != en_stop_flag)
    {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        pst_alg_txbf_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
        pc_param = pc_param + ul_off_set;

        en_stop_flag = OAL_FALSE;
        while ((' ' == *pc_param) || ('\0' == *pc_param))
        {
            if ('\0' == *pc_param)
            {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++ pc_param;
        }

        if (OAL_TRUE != en_stop_flag)
        {
            /* 获取参数配置值 */
            ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            /* 记录参数配置值 */
            pst_alg_txbf_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        }
    }
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_txbf_log_param_stru));
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_txbf_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_ar_test
 功能描述  : autorate算法测试配置命令:
             hipriv "vap0 alg_ar_test cycle_rate 11:22:33:44:55:66 1"
             该命令针对某一个USER
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

        配置算法的配置命令: hipriv "vap0 alg_ar_test cycle_rate 11:22:33:44:55:66 1"
        该命令针对某一个USER

 修改历史      :
  1.日    期   : 2013年11月14日
    作    者   : liwenjun 68207
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_ar_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_offset;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_ar_test_param_stru       *pst_alg_ar_test_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;

    pst_alg_ar_test_param = (mac_ioctl_alg_ar_test_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_offset;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_ar_test_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;


    ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_ar_test_param->auc_mac_addr, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_offset;

    /* 获取参数配置值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 记录参数配置值 */
    pst_alg_ar_test_param->us_value = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru));
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


oal_int32 wal_ioctl_get_assoc_req_ie(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_len;
    oal_int32                       l_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_req_ie::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }

    l_ret = (oal_int32)wal_config_get_assoc_req_ie(pst_mac_vap, &us_len, (oal_uint8 *)pst_ioctl_data);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_req_ie::return err code [%d]!}\r\n", l_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_int32 wal_ioctl_set_auth_mode(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_len = sizeof(oal_uint32);
    oal_uint32                      ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_auth_mode::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }

    ul_ret = hmac_config_set_auth_mode(pst_mac_vap, us_len, (oal_uint8 *)&(pst_ioctl_data->pri_data.auth_params));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_auth_mode::return err code [%d]!}\r\n", ul_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_ioctl_set_max_user(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_len = sizeof(oal_uint32);
    oal_uint32                      ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_max_user::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }

    ul_ret = hmac_config_set_max_user(pst_mac_vap, us_len, (pst_ioctl_data->pri_data.ul_vap_max_user));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_max_user::hmac_config_set_max_user return err code [%d]!}\r\n", ul_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_ssid(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret;
    mac_cfg_ssid_param_stru         st_ssid_param;
    oal_uint8                        uc_ssid_temp;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ssid::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }
    /* 保证后面的strlen不会溢出 */
    pst_ioctl_data->pri_data.ssid[OAL_IEEE80211_MAX_SSID_LEN+3] = '\0';
    uc_ssid_temp = (oal_uint8)OAL_STRLEN((oal_int8 *)pst_ioctl_data->pri_data.ssid);
    us_len = OAL_MIN(uc_ssid_temp, OAL_IEEE80211_MAX_SSID_LEN);

    st_ssid_param.uc_ssid_len = (oal_uint8)us_len;
    oal_memcopy(st_ssid_param.ac_ssid, (const oal_void *)(pst_ioctl_data->pri_data.ssid), (oal_uint32)us_len);
    ul_ret = hmac_config_set_ssid(pst_mac_vap, OAL_SIZEOF(st_ssid_param), (oal_uint8 *)&st_ssid_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_ssid::hmac_config_set_ssid return err code [%d]!}\r\n", ul_ret);
        return -OAL_EFAIL;
    }

    /* OAM日志中不能使用%s
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_ssid::set ssid[%s] ok [%d]!}\r\n", (pst_ioctl_data->pri_data.ssid));*/
    return OAL_SUCC;
}




oal_int32 wal_ioctl_set_country_code(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int8                        auc_country_code[4] = {0};
    oal_int32                       l_ret;

    oal_memcopy(auc_country_code, pst_ioctl_data->pri_data.country_code.auc_country_code, 3);

    l_ret = wal_regdomain_update_for_dfs(pst_net_dev, auc_country_code);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::return err code [%d]!}\r\n", l_ret);
        return -OAL_EFAIL;
    }

    l_ret = wal_regdomain_update(pst_net_dev, auc_country_code);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::return err code [%d]!}\r\n", l_ret);
        return -OAL_EFAIL;
    }

#else
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif

    return OAL_SUCC;
}

oal_int32  wal_ioctl_nl80211_priv_connect(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg80211_connect_param_stru     st_mac_cfg80211_connect_param;
    oal_uint8                           uc_loop                 = 0;
    oal_uint8                           uc_akm_suite_num        = 0;
    oal_uint8                           uc_pairwise_cipher_num  = 0;
    oal_int32                           l_ret                   = 0;

    /* 初始化驱动连接参数 */
    oal_memset(&st_mac_cfg80211_connect_param, 0, OAL_SIZEOF(mac_cfg80211_connect_param_stru));

    /* 解析内核下发的 freq to channel_number eg.1,2,36,40...  */
    st_mac_cfg80211_connect_param.uc_channel    = (oal_uint8)oal_ieee80211_frequency_to_channel(pst_ioctl_data->pri_data.cfg80211_connect_params.l_freq);

    /* 解析内核下发的 ssid */
    st_mac_cfg80211_connect_param.puc_ssid      = (oal_uint8 *)pst_ioctl_data->pri_data.cfg80211_connect_params.puc_ssid;
    st_mac_cfg80211_connect_param.uc_ssid_len   = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.ssid_len;

    /* 解析内核下发的 bssid */
    st_mac_cfg80211_connect_param.puc_bssid     = (oal_uint8 *)pst_ioctl_data->pri_data.cfg80211_connect_params.puc_bssid;

    /* 解析内核下发的安全相关参数 */
    /* 认证类型 */
    st_mac_cfg80211_connect_param.en_auth_type  = pst_ioctl_data->pri_data.cfg80211_connect_params.en_auth_type;

    /* 加密能力 */
    st_mac_cfg80211_connect_param.en_privacy    = pst_ioctl_data->pri_data.cfg80211_connect_params.en_privacy;

    /* IE下发 */
    st_mac_cfg80211_connect_param.puc_ie        = pst_ioctl_data->pri_data.cfg80211_connect_params.puc_ie;
    st_mac_cfg80211_connect_param.ul_ie_len     = (oal_uint32)pst_ioctl_data->pri_data.cfg80211_connect_params.ie_len;

    /* 设置加密参数 */
    if (0 != pst_ioctl_data->pri_data.cfg80211_connect_params.en_privacy)
    {
        if ((0 != pst_ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len) && (0 == pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites))
        {
            /* 设置wep加密信息 */
            st_mac_cfg80211_connect_param.puc_wep_key            = pst_ioctl_data->pri_data.cfg80211_connect_params.puc_wep_key;
            st_mac_cfg80211_connect_param.uc_wep_key_len         = pst_ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len;
            st_mac_cfg80211_connect_param.uc_wep_key_index       = pst_ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_index;
            st_mac_cfg80211_connect_param.st_crypto.cipher_group = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.cipher_group;
        }
        else if (0 != pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites)
        {
            /* 设置WPA/WPA2 加密信息 */
            st_mac_cfg80211_connect_param.st_crypto.wpa_versions = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.wpa_versions;
            st_mac_cfg80211_connect_param.st_crypto.cipher_group = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.cipher_group;
            st_mac_cfg80211_connect_param.st_crypto.n_ciphers_pairwise = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_ciphers_pairwise;
            st_mac_cfg80211_connect_param.st_crypto.n_akm_suites = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites;
            st_mac_cfg80211_connect_param.st_crypto.control_port = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.control_port;

            uc_pairwise_cipher_num = st_mac_cfg80211_connect_param.st_crypto.n_ciphers_pairwise;
            for (uc_loop = 0; uc_loop < uc_pairwise_cipher_num; uc_loop++)
            {
                st_mac_cfg80211_connect_param.st_crypto.ciphers_pairwise[uc_loop] = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.ciphers_pairwise[uc_loop];
            }

            uc_akm_suite_num = st_mac_cfg80211_connect_param.st_crypto.n_akm_suites;
            for (uc_loop = 0; uc_loop < uc_akm_suite_num; uc_loop++)
            {
                st_mac_cfg80211_connect_param.st_crypto.akm_suites[uc_loop] = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.akm_suites[uc_loop];
            }
        }
        else
        {
            OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_ioctl_nl80211_priv_connect::set_key fail! uc_wep_key_len [%d] n_akm_suites [%d]} puc_wep_key[0x%x]\r\n",
                           pst_ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len,
                           pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites,
                           pst_ioctl_data->pri_data.cfg80211_connect_params.puc_wep_key);
            return -OAL_EFAIL;
        }

    }

    /* 抛事件给驱动，启动关联 */
    l_ret = wal_cfg80211_start_connect(pst_net_dev, &st_mac_cfg80211_connect_param);
    if( OAL_SUCC != l_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_nl80211_priv_connect::wal_cfg80211_start_connect fail!}\r\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32  wal_ioctl_nl80211_priv_disconnect(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg_kick_user_param_stru    st_mac_cfg_kick_user_param;
    oal_int32                       l_ret;
    mac_user_stru                   *pst_mac_user;
    mac_vap_stru                    *pst_mac_vap;


    /* 解析内核下发的connect参数 */
    oal_memset(&st_mac_cfg_kick_user_param, 0, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* 解析内核下发的去关联原因  */
    st_mac_cfg_kick_user_param.us_reason_code = pst_ioctl_data->pri_data.kick_user_params.us_reason_code;

    /* 填写和sta关联的ap mac 地址*/
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    pst_mac_user = mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_nl80211_priv_disconnect:: mac_res_get_mac_user pst_mac_user  is nul!}\r\n");
        return OAL_SUCC;
    }

    oal_memcopy(st_mac_cfg_kick_user_param.auc_mac_addr, pst_mac_user->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);

    l_ret = wal_cfg80211_start_disconnect(pst_net_dev, &st_mac_cfg_kick_user_param);
    if( OAL_SUCC != l_ret)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_nl80211_priv_disconnect::wal_cfg80211_start_disconnect fail!}\r\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
oal_int32 wal_ioctl_set_channel(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg_channel_param_stru          *pst_channel_param;
    wal_msg_write_stru                   st_write_msg;
    mac_device_stru                     *pst_device;
    mac_vap_stru                        *pst_mac_vap;
    oal_wiphy_stru                      *pst_wiphy;
    oal_ieee80211_channel_stru          *pst_channel;
    wlan_channel_bandwidth_enum_uint8    en_bandwidth;
    oal_int32                            l_freq;
    oal_int32                            l_channel;
    oal_int32                            l_sec_channel_offset;
    oal_int32                            l_center_freq1;
    oal_int32                            l_bandwidth;
    oal_int32                            l_ret;
    wal_msg_stru                        *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru              *pst_write_rsp_msg;

    l_freq               = pst_ioctl_data->pri_data.freq.l_freq;
    l_channel            = pst_ioctl_data->pri_data.freq.l_channel;
    l_sec_channel_offset = pst_ioctl_data->pri_data.freq.l_sec_channel_offset;
    l_center_freq1       = pst_ioctl_data->pri_data.freq.l_center_freq1;
    l_bandwidth          = pst_ioctl_data->pri_data.freq.l_bandwidth;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    pst_device  = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_channel::pst_device is null!}\r\n");
        return -OAL_EINVAL;
    }

    pst_wiphy   = pst_device->pst_wiphy;
    pst_channel = oal_ieee80211_get_channel(pst_wiphy, l_freq);
    l_channel   = pst_channel->hw_value;

    /* 判断信道在不在管制域内 */
    l_ret = (oal_int32)mac_is_channel_num_valid(pst_channel->band, (oal_uint8)l_channel);
    if (l_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_channel::channel num is invalid. band, ch num [%d] [%d]!}\r\n", pst_channel->band, l_channel);
        return -OAL_EINVAL;
    }

    /* 进行内核带宽值和WITP 带宽值转换 */
    if (80 == l_bandwidth)
    {
        en_bandwidth = mac_get_bandwith_from_center_freq_seg0((oal_uint8)l_channel, (oal_uint8)((l_center_freq1 - 5000)/5));
    }
    else if (40 == l_bandwidth)
    {
        switch (l_sec_channel_offset) {
            case -1:
                en_bandwidth     = WLAN_BAND_WIDTH_40MINUS;
                break;
            case 1:
                en_bandwidth     = WLAN_BAND_WIDTH_40PLUS;
                break;
            default:
                en_bandwidth     = WLAN_BAND_WIDTH_20M;
                break;
        }
    }
    else
    {
        en_bandwidth     = WLAN_BAND_WIDTH_20M;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/

    /* 填写消息 */
    pst_channel_param = (mac_cfg_channel_param_stru *)(st_write_msg.auc_value);
    pst_channel_param->uc_channel   = (oal_uint8)pst_channel->hw_value;
    pst_channel_param->en_band      = pst_channel->band;
    pst_channel_param->en_bandwidth = en_bandwidth;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG80211_SET_CHANNEL, OAL_SIZEOF(mac_cfg_channel_param_stru));

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_channel_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_channel:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_channel::wal_send_cfg_event return err code:[%d]!}\r\n",
                        pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_wps_p2p_ie
 功能描述  : 设置WPS p2p信息元素
 输入参数  : oal_net_device_stru *pst_net_dev
             char *buf
             int len
 输出参数  : 无
 返 回 值  : oal_int32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日
    作    者   : xiaoyuren x00305155
    修改内容   : 新生成函数

*****************************************************************************/
oal_int32 wal_ioctl_set_wps_p2p_ie(oal_net_device_stru  *pst_net_dev,
                                    oal_uint8           *puc_buf,
                                    oal_uint32           ul_len,
                                    en_app_ie_type_uint8 en_type)
{
    wal_msg_write_stru              st_write_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru         *pst_write_rsp_msg;
    oal_app_ie_stru                 st_wps_p2p_ie;
    oal_int32                       l_ret = 0;

    OAL_MEMZERO(&st_wps_p2p_ie, OAL_SIZEOF(st_wps_p2p_ie));
    switch (en_type) {
        case OAL_APP_BEACON_IE:
        case OAL_APP_PROBE_RSP_IE:
        case OAL_APP_ASSOC_RSP_IE:
            st_wps_p2p_ie.en_app_ie_type = en_type;
            st_wps_p2p_ie.ul_ie_len      = ul_len;
            oal_memcopy(st_wps_p2p_ie.auc_ie, puc_buf, ul_len);
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_wps_p2p_ie:: wrong type: [%x]!}\r\n",
                            en_type);
            return -OAL_EFAIL;
    }


    oal_memcopy(st_write_msg.auc_value, &st_wps_p2p_ie, OAL_SIZEOF(st_wps_p2p_ie));

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_WPS_P2P_IE, OAL_SIZEOF(st_wps_p2p_ie));

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_wps_p2p_ie),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_p2p_ie:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_p2p_ie::wal_send_cfg_event return err code: [%d]!}\r\n",
                        pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_P2P
/*****************************************************************************
 函 数 名  : wal_ioctl_set_p2p_noa
 功能描述  : 设置p2p noa节能参数
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_paramm
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月22日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_p2p_noa(oal_net_device_stru *pst_net_dev, mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param)
{
    wal_msg_write_stru              st_write_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru         *pst_write_rsp_msg;
    oal_int32                       l_ret = 0;

    oal_memcopy(st_write_msg.auc_value, pst_p2p_noa_param, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_NOA, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_p2p_noa_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_noa:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_noa::wal_send_cfg_event return err code:  [%d]!}\r\n",
                           pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_p2p_ops
 功能描述  : 设置p2p ops节能参数
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_paramm
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月22日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_p2p_ops(oal_net_device_stru *pst_net_dev, mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param)
{
    wal_msg_write_stru              st_write_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru         *pst_write_rsp_msg;
    oal_int32                       l_ret = 0;

    oal_memcopy(st_write_msg.auc_value, pst_p2p_ops_param, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_OPS, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_p2p_ops_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_ops:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_ops::wal_send_cfg_event return err code:[%d]!}\r\n",
                        pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif



oal_int32 wal_ioctl_set_wps_ie(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru              st_write_msg;
    oal_app_ie_stru                 st_wps_ie;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru         *pst_write_rsp_msg;
    oal_int32                       l_ret = 0;

    OAL_MEMZERO(&st_wps_ie, OAL_SIZEOF(oal_app_ie_stru));
    st_wps_ie.ul_ie_len      = pst_ioctl_data->pri_data.st_app_ie.ul_ie_len;
    st_wps_ie.en_app_ie_type = pst_ioctl_data->pri_data.st_app_ie.en_app_ie_type;
    l_ret = (oal_int32)oal_copy_from_user(st_wps_ie.auc_ie, pst_ioctl_data->pri_data.st_app_ie.auc_ie, st_wps_ie.ul_ie_len);

    /* copy_from_user函数的目的是从用户空间拷贝数据到内核空间，失败返回没有被拷贝的字节数，成功返回0 */
    if(l_ret != 0)
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_ioctl_set_wps_ie::copy app ie fail.ie_type[%d], ie_len[%d]}",
                        st_wps_ie.en_app_ie_type,
                        st_wps_ie.ul_ie_len);
        return -OAL_EFAIL;
    }

    oal_memcopy(st_write_msg.auc_value, &st_wps_ie, OAL_SIZEOF(oal_app_ie_stru));

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_WPS_IE, OAL_SIZEOF(oal_app_ie_stru));

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_app_ie_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_ie:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_ie::wal_send_cfg_event return err code:[%x]!}\r\n",
                        pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_frag(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru              st_write_msg;
    mac_cfg_frag_threshold_stru    *pst_threshold;
    oal_uint32                      ul_threshold = 0;
    oal_uint16                      us_len;
    oal_int32                       l_ret = 0;

    /* 获取分片门限 */
    ul_threshold = (oal_uint32)pst_ioctl_data->pri_data.l_frag;

    pst_threshold = (mac_cfg_frag_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_frag_threshold = ul_threshold;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_frag_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FRAG_THRESHOLD_REG, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_frag::return err code [%d]!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_rts(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru              st_write_msg;
    mac_cfg_rts_threshold_stru     *pst_threshold;
    oal_uint32                      ul_threshold = 0;
    oal_uint16                      us_len;
    oal_int32                       l_ret = 0;

    /* 获取分片门限 */
    ul_threshold = (oal_uint32)pst_ioctl_data->pri_data.l_rts;

    pst_threshold = (mac_cfg_rts_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_rts_threshold = ul_threshold;

    OAM_INFO_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_rts::rts [%d]!}\r\n", ul_threshold);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_rts_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RTS_THRESHHOLD, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_rts::return err code [%d]!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_get_cmd_one_arg
 功能描述  : 获取字符串第一个参数，以逗号为命令区分标识，并扣除命令前置字符
 输入参数  : pc_cmd         : 传入的字符串
             puc_token      : 命令前置字串
 输出参数  : pc_arg         : 扣掉命令前置字串后的参数
             pul_cmd_offset : 第一个参数的长度
 返 回 值  : 错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月22日
    作    者   : xiaoyuren
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32 wal_get_parameter_from_cmd(oal_int8 *pc_cmd, oal_int8 *pc_arg, OAL_CONST oal_int8 *puc_token, oal_uint32 *pul_cmd_offset, oal_uint32 ul_param_max_len)
{
    oal_int8   *pc_cmd_copy;
    oal_int8    ac_cmd_copy[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN];
    oal_uint32  ul_pos = 0;
    oal_uint32  ul_arg_len;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd) || (OAL_PTR_NULL == pc_arg) || (OAL_PTR_NULL == pul_cmd_offset)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_get_parameter_from_cmd::pc_cmd/pc_arg/pul_cmd_offset null ptr error %d, %d, %d, %d!}\r\n", pc_cmd, pc_arg, pul_cmd_offset);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_cmd_copy = pc_cmd;

    /* 去掉字符串开始的逗号 */
    while (',' == *pc_cmd_copy)
    {
        ++pc_cmd_copy;
    }
    /* 取得逗号前的字符串 */
    while ((',' != *pc_cmd_copy) && ('\0' != *pc_cmd_copy))
    {
        ac_cmd_copy[ul_pos] = *pc_cmd_copy;
        ++ul_pos;
        ++pc_cmd_copy;

        if (OAL_UNLIKELY(ul_pos >= ul_param_max_len))
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_get_parameter_from_cmd::ul_pos >= WAL_HIPRIV_CMD_NAME_MAX_LEN, ul_pos %d!}\r\n", ul_pos);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }
    ac_cmd_copy[ul_pos]  = '\0';
    /* 字符串到结尾，返回错误码 */
    if (0 == ul_pos)
    {
        OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_get_parameter_from_cmd::return param pc_arg is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    *pul_cmd_offset = (oal_uint32)(pc_cmd_copy - pc_cmd);

    /* 检查字符串是否包含期望的前置命令字符*/
    if (0 != oal_memcmp(ac_cmd_copy, puc_token, OAL_STRLEN(puc_token)))
    {
        return OAL_FAIL;
    }
    else
    {
        /* 扣除前置命令字符，回传参数*/
        ul_arg_len = OAL_STRLEN(ac_cmd_copy) - OAL_STRLEN(puc_token);
        oal_memcopy(pc_arg, ac_cmd_copy + OAL_STRLEN(puc_token), ul_arg_len);
        pc_arg[ul_arg_len]  = '\0';
    }
    return OAL_SUCC;
}
OAL_STATIC oal_int32 wal_set_ap_max_user(oal_net_device_stru *pst_net_dev, oal_uint32 ul_ap_max_user)
{
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    oal_int32                   l_ret;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user:: ap_max_user is : %u.}\r\n", ul_ap_max_user);

    if(ul_ap_max_user == 0)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user::invalid ap max user(%u),ignore this set.}\r\n", ul_ap_max_user);
        return OAL_SUCC;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MAX_USER, OAL_SIZEOF(ul_ap_max_user));
     *((oal_uint32 *)st_write_msg.auc_value) = ul_ap_max_user;
    l_ret = wal_send_cfg_event(pst_net_dev,
                                WAL_MSG_TYPE_WRITE,
                                WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_ap_max_user),
                                (oal_uint8 *)&st_write_msg,
                                OAL_TRUE,
                                &pst_rsp_msg);

    if ((OAL_SUCC != l_ret) || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user:: wal_send_cfg_event return err code %d!}\r\n", l_ret);

        return l_ret;
    }

    /* 读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_set_ap_max_user::wal_send_cfg_event return err code: [%d]!}\r\n",
                           pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

    /* 每次设置最大用户数完成后，都清空为非法值0 */
    g_st_ap_config_info.ul_ap_max_user = 0;

    return l_ret;
}


OAL_STATIC oal_int32 wal_config_mac_filter(oal_net_device_stru *pst_net_dev, oal_int8 *pc_command)
{
    oal_int8                    ac_parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN];
    oal_int8                   *pc_parse_command;
    oal_uint32                  ul_mac_mode;
    oal_uint32                  ul_mac_cnt = 0;
    oal_uint32                  ul_i;
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    wal_msg_write_stru          st_write_msg;
    oal_uint16                  us_len;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    oal_int32                   l_ret = 0;
#endif
    oal_uint32                  ul_ret = 0;
    oal_uint32                  ul_off_set;


    if (OAL_PTR_NULL == pc_command)
    {
        return -OAL_EINVAL;
    }

    pc_parse_command = pc_command;

    /* 解析MAC_MODE*/
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_MODE=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code %u.}\r\n", ul_ret);
        return -OAL_EINVAL;
    }
    /* 检查参数是否合法 0,1,2*/
    ul_mac_mode = (oal_uint32)oal_atoi(ac_parsed_command);
    if(ul_mac_mode > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::invalid MAC_MODE[%s]!}\r\n", ac_parsed_command);
        return -OAL_EINVAL;
    }

    /* 设置过滤模式*/
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    ul_ret = wal_hipriv_send_cfg_uint32_data(pst_net_dev,ac_parsed_command, WLAN_CFGID_BLACKLIST_MODE);
    if(OAL_SUCC != ul_ret)
    {
        return (oal_int32)ul_ret;
    }
#endif
    /* 解析MAC_CNT*/
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_CNT=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ul_ret);
        return -OAL_EINVAL;
    }
    ul_mac_cnt = (oal_uint32)oal_atoi(ac_parsed_command);

    for (ul_i = 0; ul_i < ul_mac_cnt; ul_i++)
    {
        pc_parse_command += ul_off_set;
        ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ul_ret);
            return -OAL_EINVAL;
        }
        /* 5.1  检查参数是否符合MAC长度*/
        if(WLAN_MAC_ADDR_LEN * 2 != OAL_STRLEN(ac_parsed_command))
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_mac_filter::invalid MAC format}\r\n");
            return -OAL_EINVAL;
        }
        /*6. 添加过滤设备*/
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
        /***************************************************************************
                             抛事件到wal层处理
        ***************************************************************************/
        oal_strtoaddr(ac_parsed_command, st_write_msg.auc_value); /* 将字符 ac_name 转换成数组 mac_add[6] */

        us_len = OAL_MAC_ADDR_LEN; /* OAL_SIZEOF(oal_uint8); */

        if(ul_i == (ul_mac_cnt - 1))
        {
            /* 等所有的mac地址都添加完成后，才进行关联用户确认，是否需要删除 */
            WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);
        }
        else
        {
            WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST_ONLY, us_len);
        }

        /* 6.1  发送消息*/
        l_ret = wal_send_cfg_event(pst_net_dev,
                                    WAL_MSG_TYPE_WRITE,
                                    WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                    (oal_uint8 *)&st_write_msg,
                                    OAL_TRUE,
                                    &pst_rsp_msg);

        if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter:: wal_send_cfg_event return err code %d!}\r\n", l_ret);
            return l_ret;
        }

        /* 6.2  读取返回的错误码 */
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_config_mac_filter::wal_send_cfg_event return err code:[%x]!}\r\n",
                               pst_write_rsp_msg->ul_err_code);
            return -OAL_EFAIL;
        }
#endif
    }

    /* 每次设置完成mac地址过滤后，清空此中间变量 */
    oal_memset(g_st_ap_config_info.ac_ap_mac_filter_mode, 0 ,OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode));

    return OAL_SUCC;
}



OAL_STATIC oal_int32 wal_kick_sta(oal_net_device_stru *pst_net_dev, oal_uint8 *auc_mac_addr)
{
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    oal_int32                       l_ret = 0;
#endif


    if(NULL == auc_mac_addr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_kick_sta::argument auc_mac_addr is null.\n");
        return -OAL_EFAIL;
    }

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, auc_mac_addr);

    pst_kick_user_param->us_reason_code = MAC_AUTH_NOT_VALID;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_kick_sta:: wal_send_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 4.4  读取返回的错误码 */
    pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_kick_sta::wal_send_cfg_event return err code: [%x]!}\r\n",
                               pst_write_rsp_msg->ul_err_code);
        return -OAL_EFAIL;
    }

#endif

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_ap_config
 功能描述  : 设置netd下发APUT config参数 (最大用户数)
 输入参数  : oal_iw_request_info_stru *pst_info
             oal_iwreq_data_union *pst_wrqu
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32 wal_ioctl_set_ap_config(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra)
{
    oal_int8                   *pc_command        = OAL_PTR_NULL;
    oal_int8                   *pc_parse_command  = OAL_PTR_NULL;
    oal_int32                   l_ret             = OAL_SUCC;
    oal_uint32                  ul_ret            = OAL_SUCC;
    oal_int8                    ac_parse_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN];
    oal_uint32                  ul_off_set;

    /* 1. 申请内存保存netd 下发的命令和数据 */
    pc_command = oal_memalloc((oal_int32)(pst_wrqu->data.length + 1));
    if (OAL_PTR_NULL == pc_command)
    {
        return -OAL_ENOMEM;
    }
    /* 2. 拷贝netd 命令到内核态中 */
    oal_memset(pc_command, 0, (oal_uint32)(pst_wrqu->data.length + 1));
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer , (oal_uint32)(pst_wrqu->data.length));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::oal_copy_from_user: -OAL_EFAIL }\r\n");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }
    pc_command[pst_wrqu->data.length] = '\0';

    OAL_IO_PRINT("wal_ioctl_set_ap_config,data len:%u,command is:%s\n", (oal_uint32)pst_wrqu->data.length, pc_command);

    pc_parse_command = pc_command;
    /* 3.   解析参数 */
    /* 3.1  解析ASCII_CMD*/
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "ASCII_CMD=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd ASCII_CMD return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    if ((0 != oal_strcmp("AP_CFG", ac_parse_command)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::sub_command %s != 'AP_CFG' }\r\n",ac_parse_command);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    /* 3.2  解析CHANNEL，目前不处理netd下发的channel信息*/
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "CHANNEL=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd CHANNEL return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    /* 3.3  解析MAX_SCB*/
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "MAX_SCB=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd MAX_SCB return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    g_st_ap_config_info.ul_ap_max_user = (oal_uint32)oal_atoi(ac_parse_command);

    if (OAL_PTR_NULL != OAL_NET_DEV_PRIV(pst_net_dev))
    {
        l_ret = wal_set_ap_max_user(pst_net_dev, (oal_uint32)oal_atoi(ac_parse_command));
    }

    /* 5. 结束释放内存*/
    oal_free(pc_command);
    return l_ret;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_get_assoc_list
 功能描述  : netd下发命令取得关联设备列表
 输入参数  : oal_iw_request_info_stru *pst_info
             oal_iwreq_data_union *pst_wrqu
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月22日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_get_assoc_list(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra)
{
    oal_int32                       l_ret;
    wal_msg_query_stru              st_query_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru               *pst_query_rsp_msg;
    oal_int8                       *pc_sta_list;
    oal_netbuf_stru                *pst_response_netbuf;

    if(OAL_PTR_NULL == pc_extra)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list:: return buf is null,return faild.}\r\n");
        return -OAL_EFAIL;
    }

    /* 上层在任何时候都可能下发此命令，需要先判断当前netdev的状态并及时返回 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev)))
    {
        return -OAL_EFAIL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    st_query_msg.en_wid = WLAN_CFGID_GET_STA_LIST;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if ((OAL_SUCC != l_ret)  || (OAL_PTR_NULL == pst_rsp_msg))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    /* 业务处理 */
    if(pst_query_rsp_msg->us_len >= OAL_SIZEOF(pst_response_netbuf))
    {
        oal_memcopy(&pst_response_netbuf,pst_query_rsp_msg->auc_value,OAL_SIZEOF(pst_response_netbuf));
        if(NULL != pst_response_netbuf)
        {
            pc_sta_list = (oal_int8*)OAL_NETBUF_DATA(pst_response_netbuf);
            pst_wrqu->data.length = (oal_uint16)(OAL_NETBUF_LEN(pst_response_netbuf) + 1);
            oal_memcopy(pc_extra, pc_sta_list, pst_wrqu->data.length);
            pc_extra[OAL_NETBUF_LEN(pst_response_netbuf)] = '\0';
            oal_netbuf_free(pst_response_netbuf);
        }
        else
        {
            l_ret = -OAL_ENOMEM;
        }
    }
    else
    {
        oal_print_hex_dump((oal_uint8 * )pst_rsp_msg->auc_msg_data, pst_query_rsp_msg->us_len, 32, "query msg: ");
        l_ret = -OAL_EINVAL;
    }

    if(OAL_SUCC != l_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list::process failed,ret=%d}", l_ret);
    }
    else
    {
        OAL_IO_PRINT("wal_ioctl_get_assoc_list,pc_sta_list is:%s,len:%d\n", pc_extra, pst_wrqu->data.length);
    }

    return l_ret;

}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_mac_filters
 功能描述  : netd下发命令设置黑名单或白名单
 输入参数  : oal_iw_request_info_stru *pst_info
             oal_iwreq_data_union *pst_wrqu
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月22日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_mac_filters(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra)
{
    mac_vap_stru               *pst_vap           = OAL_PTR_NULL;
    oal_int8                   *pc_command        = OAL_PTR_NULL;
    oal_int32                   l_ret             = 0;
    oal_uint32                  ul_ret            = 0;
    oal_int8                    ac_parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN];
    oal_int8                   *pc_parse_command;
    oal_uint32                  ul_mac_mode;
    oal_uint32                  ul_mac_cnt = 0;
    oal_uint8                   auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                  ul_off_set;

    /* 1. 申请内存保存netd 下发的命令和数据 */
    pc_command = oal_memalloc((oal_int32)(pst_wrqu->data.length + 1));
    if (OAL_PTR_NULL == pc_command)
    {
        return -OAL_ENOMEM;
    }

    /* 2. 拷贝netd 命令到内核态中 */
    oal_memset(pc_command, 0, (oal_uint32)(pst_wrqu->data.length + 1));
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer , (oal_uint32)(pst_wrqu->data.length));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::oal_copy_from_user: -OAL_EFAIL }\r\n");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }
    pc_command[pst_wrqu->data.length] = '\0';

    OAL_IO_PRINT("wal_ioctl_set_mac_filters,data len:%d, command is:%s\n", pst_wrqu->data.length, pc_command);

    pc_parse_command = pc_command;

    oal_memset(g_st_ap_config_info.ac_ap_mac_filter_mode, 0, OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode));
    oal_strcpy(g_st_ap_config_info.ac_ap_mac_filter_mode, pc_command);

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_PTR_NULL == pst_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::netdevice vap is null,just save it.}\r\n");
        oal_free(pc_command);
        return OAL_SUCC;
    }

    /* 3  解析MAC_MODE*/
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_MODE=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    /* 3.1 检查参数是否合法 0,1,2*/
    ul_mac_mode = (oal_uint32)oal_atoi(ac_parsed_command);
    if(ul_mac_mode > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::invalid MAC_MODE[%s]!}\r\n", ac_parsed_command);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    /* 5 解析MAC_CNT*/
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_CNT=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    ul_mac_cnt = (oal_uint32)oal_atoi(ac_parsed_command);

    wal_config_mac_filter(pst_net_dev, pc_command);

    /* 如果是白名单模式，且下发允许MAC地址为空，即不允许任何设备关联，需要去关联所有已经关联的STA */
    if((0 == ul_mac_cnt) && (2 == ul_mac_mode))
    {
        OAL_IO_PRINT("wal_ioctl_set_mac_filters,delete all stations.\n");

        oal_memset(auc_mac_addr, 0xff, OAL_ETH_ALEN);
        l_ret = wal_kick_sta(pst_net_dev, auc_mac_addr);
    }

    oal_free(pc_command);
    return l_ret;
}

/*****************************************************************************
 函 数 名  : wal_ioctl_set_ap_sta_disassoc
 功能描述  : netd下发命令去关联STA
 输入参数  : oal_iw_request_info_stru *pst_info
             oal_iwreq_data_union *pst_wrqu
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月22日
    作    者   : xiaoyuren 00305155
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_int32  wal_ioctl_set_ap_sta_disassoc(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_iwreq_data_union *pst_wrqu, oal_int8 *pc_extra)
{
    oal_int8                       *pc_command        = OAL_PTR_NULL;
    oal_int32                       l_ret             = 0;
    oal_uint32                      ul_ret            = 0;
    oal_int8                        ac_parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_off_set;

    /* 1. 申请内存保存netd 下发的命令和数据 */
    pc_command = oal_memalloc((oal_int32)(pst_wrqu->data.length + 1));
    if (OAL_PTR_NULL == pc_command)
    {
        return -OAL_ENOMEM;
    }

    /* 2. 拷贝netd 命令到内核态中 */
    oal_memset(pc_command, 0, (oal_uint32)(pst_wrqu->data.length + 1));
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer , (oal_uint32)(pst_wrqu->data.length));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::oal_copy_from_user: -OAL_EFAIL }\r\n");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }
    pc_command[pst_wrqu->data.length] = '\0';

    OAL_IO_PRINT("wal_ioctl_set_ap_sta_disassoc,command is:%s\n", pc_command);

    /* 3. 解析命令获取MAC */
    ul_ret = wal_get_parameter_from_cmd(pc_command, ac_parsed_command, "MAC=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::wal_get_parameter_from_cmd MAC return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    /* 3.1  检查参数是否符合MAC长度*/
    if(WLAN_MAC_ADDR_LEN * 2 != OAL_STRLEN(ac_parsed_command))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::invalid MAC format}\r\n");
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    oal_strtoaddr(ac_parsed_command, auc_mac_addr); /* 将字符 ac_name 转换成数组 mac_add[6] */

    l_ret = wal_kick_sta(pst_net_dev, auc_mac_addr);

    /* 5. 结束释放内存*/
    oal_free(pc_command);
    return l_ret;

}

#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_DFT_EVENT

OAL_STATIC oal_void  wal_event_report_to_sdt(wal_msg_type_enum_uint8   en_msg_type,
                                       oal_uint8                *puc_param,
                                       wal_msg_stru             *pst_cfg_msg)
{
    oam_event_type_enum_uint16   en_event_type = OAM_EVENT_TYPE_BUTT;
    oal_uint8                    auc_event[50] = {0};
    oal_uint8                    auc_macaddr[WLAN_MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (WAL_MSG_TYPE_QUERY == en_msg_type)
    {
        en_event_type = OAM_EVENT_WID_QUERY;
    }
    else if (WAL_MSG_TYPE_WRITE == en_msg_type)
    {
        en_event_type = OAM_EVENT_WID_WRITE;
    }

    /* 复制WID,参数的前两个字节是WID */
    oal_memcopy((oal_void *)auc_event, (const oal_void *)puc_param, OAL_SIZEOF(oal_uint16));

    /* 复制消息头 */
    oal_memcopy((oal_void *)&auc_event[2], (const oal_void *)&(pst_cfg_msg->st_msg_hdr), OAL_SIZEOF(wal_msg_hdr_stru));

    WAL_EVENT_WID(auc_macaddr, 0, en_event_type, auc_event);
}
#endif

OAL_STATIC oal_uint32  wal_hipriv_get_all_reg_value(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_ALL_REG_VALUE, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_all_reg_value::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_get_mac_addr
 功能描述  : 统计指定tid的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_get_mac_addr(oal_int8 *pc_param, oal_uint8 auc_mac_addr[], oal_uint32 *pul_total_offset)
{
    oal_uint32                      ul_off_set      = 0;
    oal_uint32                      ul_ret          = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mac_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);

    *pul_total_offset = ul_off_set;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
/*****************************************************************************
 函 数 名  : wal_hipriv_set_edca_opt_switch_sta
 功能描述  : 指定打开或者关闭sta的edca优化功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_switch_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8        uc_flag         = 0;
    oal_uint8       *puc_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    oal_int32        l_ret           = OAL_SUCC;
    mac_vap_stru    *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_switch_sta 1/0"

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
       OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_sta:: only STA_MODE support}");
       return OAL_FAIL;
    }

    /* 获取配置参数 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_sta::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);

    /* 非法配置参数 */
    if (uc_flag > 1)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_switch_sta, invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_SWITCH_STA, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_sta:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_edca_opt_weight_sta
 功能描述  : 调整sta的edca参数权重，权重取值为0-3,3表示完全根据beacon进行调整，0表示采用配值的edca参数(此时竞争能力最强)
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_weight_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8           uc_weight       = 0;
    oal_uint8          *puc_value       = 0;
    oal_uint32          ul_ret          = OAL_SUCC;
    oal_int32           l_ret           = OAL_SUCC;
    oal_uint32          ul_off_set      = 0;
    mac_vap_stru       *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_weight_sta 1"

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* 获取参数值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /*lint -e734*/
    uc_weight = (oal_uint32)oal_atoi(ac_name);
   /*lint +e734*/
    /* 最大权重为3 */
    if (uc_weight > 3)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_weight_sta: valid value is between 0 and %d", 3);
        return OAL_FAIL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_WEIGHT_STA, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_weight;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_weight_sta:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_set_edca_opt_switch_ap
 功能描述  : 指定打开或者关闭ap的edca优化功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_switch_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8        uc_flag         = 0;
    oal_uint8       *puc_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    oal_int32        l_ret           = OAL_SUCC;
    mac_vap_stru    *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_switch_ap 1/0"

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
       OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
       return OAL_FAIL;
    }

    /* 获取配置参数 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);

    /* 非法配置参数 */
    if (uc_flag > 1)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_cycle_ap, invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_SWITCH_AP, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_edca_opt_cycle_ap
 功能描述  : 调整edca参数调整的周期
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_cycle_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint32       ul_cycle_ms     = 0;
    oal_uint32      *pul_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_int32        l_ret           = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    mac_vap_stru    *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_cycle_ap 200"

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* 获取参数值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_cycle_ms = (oal_uint32)oal_atoi(ac_name);

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_CYCLE_AP, OAL_SIZEOF(oal_uint32));
    pul_value = (oal_uint32 *)(st_write_msg.auc_value);
    *pul_value = ul_cycle_ms;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


#ifdef _PRE_WLAN_PERFORM_STAT

/*****************************************************************************
 函 数 名  : wal_hipriv_stat_tid_thrpt
 功能描述  : 统计指定tid的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_total_offset = 0;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_stat_user_thrpt
 功能描述  : 统计指定用户的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_stat_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_total_offset = 0;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_USER_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取统计周期 */
    pc_param = pc_param + ul_total_offset;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_stat_vap_thrpt
 功能描述  : 统计指定用户的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_stat_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_VAP_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取统计周期 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_stat_tid_per
 功能描述  : 统计指定tid的per
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_PER;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_stat_tid_delay
 功能描述  : 统计指定tid的delay
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_stat_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_DELAY;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_display_tid_thrpt
 功能描述  : 统计指定tid的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_display_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_display_user_thrpt
 功能描述  : 统计指定用户的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_display_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_USER_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_display_vap_thrpt
 功能描述  : 统计指定用户的吞吐量
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_display_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;

    /* vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_VAP_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_vap_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_display_tid_per
 功能描述  : 统计指定tid的per
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_display_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_PER;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_display_tid_delay
 功能描述  : 统计指定tid的delay
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月14日
    作    者   : xiechunhui
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_display_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_DELAY;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_hipriv_lpm_soc_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_soc_set_stru       *pst_set_para;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* SOC节能测试模式配置, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4(总线gating|PCIE RD BY PASS|mem precharge|PCIE L0-S|PCIE L1-0)
                        0|1(disable|enable) pcie_idle(PCIE低功耗空闲时间1~7us) "*/

    pst_set_para = (mac_cfg_lpm_soc_set_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取测试模式*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->en_mode= (mac_lpm_soc_set_enum_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取开启还是关闭*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->uc_on_off = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取PCIE空闲时间配置*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->uc_pcie_idle = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SOC_MODE,OAL_SIZEOF(mac_cfg_lpm_soc_set_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_soc_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_lpm_chip_state(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;
    mac_cfg_lpm_sleep_para_stru     *pst_set_para;

    pst_set_para = (mac_cfg_lpm_sleep_para_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    pst_set_para->uc_pm_switch = (mac_lpm_state_enum_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取定时睡眠参数*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->us_sleep_ms = (oal_uint16)oal_atoi(ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_CHIP_STATE,OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_chip_state::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_psm_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_psm_param_stru     *pst_psm_para;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* psm节能寄存器配置, hipriv "Hisilicon0 lpm_psm_param 0|1(ps off|ps on) 0|1(DTIM|listen intval) xxx(listen interval值) xxx(TBTT offset)"*/

    pst_psm_para = (mac_cfg_lpm_psm_param_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取节能是否开启*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->uc_psm_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /* 获取是DTIM唤醒还是listen interval唤醒 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->uc_psm_wakeup_mode = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取listen interval的值 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->us_psm_listen_interval = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取TBTT中断提前量的值 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->us_psm_tbtt_offset = (oal_uint8)oal_atoi(ac_name);
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_PSM_PARAM,OAL_SIZEOF(mac_cfg_lpm_psm_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_psm_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_psm_param::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;
    oal_uint8                       uc_smps_mode;

    wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);

    uc_smps_mode = (oal_uint8)oal_atoi(ac_name);
    if (uc_smps_mode >= 3)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::invalid choice [%d]!}\r\n", uc_smps_mode);

        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_smps_mode;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_MODE,OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_smps_stub_stru     *pst_smps_stub;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /*smps ap发包打桩, hipriv "vap0 lpm_smps_stub 0|1|2(off|单流|双流) 0|1(是否发RTS)*/
    /*设置配置命令参数 */
    pst_smps_stub = (mac_cfg_lpm_smps_stub_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取桩类型*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_smps_stub->uc_stub_type = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* RTS */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_smps_stub->uc_rts_en= (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_STUB,OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru) );

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_stub::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_txopps_set_stru    *pst_txopps_set;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* txop ps节能寄存器配置, hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2 off|on)"*/
    /* 设置配置命令参数 */
    pst_txopps_set = (mac_cfg_lpm_txopps_set_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取节能是否开启*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_txop_ps_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /* 获取condition1 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_conditon1 = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取condition2*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_conditon2 = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_PS_SET,OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru) );

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_set::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_tx_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_ret;
    mac_cfg_lpm_txopps_tx_stub_stru    *pst_txopps_tx_stub;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* txop ps发包测试打桩条件, hipriv "vap0 lpm_txopps_tx_stub 0|1(off|on) xxx(第几个包打桩)"*/
    /* 设置配置命令参数 */
    pst_txopps_tx_stub = (mac_cfg_lpm_txopps_tx_stub_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取桩类型*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_tx_stub->uc_stub_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取第几个报文打桩 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_tx_stub->us_begin_num = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_TX_STUB,OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru) );

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_tx_stub::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_data(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_tx_data_stru       *pst_lpm_tx_data;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* 测试发包, hipriv "vap0 lpm_tx_data xxx(个数) xxx(长度) xx:xx:xx:xx:xx:xx(目的mac) xxx(AC类型)"*/
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取发包个数*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->us_num= (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取发包长度 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->us_len = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取目的地址 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /* 获取发包AC类型 */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->uc_ac = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_DATA,OAL_SIZEOF(mac_cfg_lpm_tx_data_stru) );

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_tx_data::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_probe_request(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_tx_data_stru       *pst_lpm_tx_data;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* 测试发包, hipriv "vap0 lpm_tx_probe_request 0|1(被动|主动) xx:xx:xx:xx:xx:xx(主动模式下BSSID)"*/
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取主动or被动probe request*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->uc_positive = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取bssid */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_PROBE_REQUEST,OAL_SIZEOF(mac_cfg_lpm_tx_data_stru) );

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_tx_probe_request::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  wal_hipriv_lpm_wow_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;
    mac_cfg_lpm_wow_en_stru         *pst_lpm_wow_en;


    pst_lpm_wow_en = (mac_cfg_lpm_wow_en_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    pst_lpm_wow_en->uc_en = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    pst_lpm_wow_en->uc_null_wake = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    /* 设置配置命令参数 */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_WOW_EN,OAL_SIZEOF(mac_cfg_lpm_wow_en_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_wow_en_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_wow_en::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_remove_user_lut
 功能描述  : 删除回复用户lut表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月24日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32  wal_hipriv_remove_user_lut(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_remove_lut_stru        *pst_param;          /* 这里复用删除用户配置命令的结构体 */
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_user_idx;


    /* 删除恢复用户lut表, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac地址) 0|1(恢复/删除)" */
    pst_param = (mac_cfg_remove_lut_stru *)(st_write_msg.auc_value);

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
    }

    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* 获取 恢复/删除 标识 */
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_param->uc_is_remove = (oal_uint8)oal_atoi(ac_name);

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::no such user!}\r\n");

        return ul_ret;
    }

    pst_param->us_user_idx = us_user_idx;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REMOVE_LUT, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_send_frame(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_offset;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    mac_cfg_send_frame_param_stru      *pst_test_send_frame;
    oal_uint8                           auc_mac_addr[WLAN_MAC_ADDR_LEN];

    mac_test_frame_type_enum_uint8      en_frame_type;
    oal_uint8                           uc_pkt_num = 0;

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame type err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_frame_type = (mac_test_frame_type_enum_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_offset;

    /* 获取帧数目 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame num err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_pkt_num = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_offset;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_offset;

    /***************************************************************************
                                 抛事件到dmac层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_FRAME, OAL_SIZEOF(mac_cfg_send_frame_param_stru));

    /* 设置配置命令参数 */
    pst_test_send_frame = (mac_cfg_send_frame_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_test_send_frame->auc_mac_ra, auc_mac_addr);
    pst_test_send_frame->en_frame_type = en_frame_type;
    pst_test_send_frame->uc_pkt_num = uc_pkt_num;

    ul_ret = (oal_uint32)wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_frame_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::wal_send_cfg_event return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_rx_pn(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       auc_mac_addr[OAL_MAC_ADDR_LEN];
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_user_idx;
    mac_cfg_set_rx_pn_stru         *pst_rx_pn;
    oal_uint16                      us_pn = 0;
    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_off_set;

    /* 获取pn号 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    us_pn = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::no such user!}\r\n");

        return ul_ret;
    }

    pst_rx_pn = (mac_cfg_set_rx_pn_stru *)(st_write_msg.auc_value);
    pst_rx_pn->us_rx_pn = us_pn;
    pst_rx_pn->us_user_idx = us_user_idx;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_set_rx_pn_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_PN_REG, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_soft_retry(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_software_retry = 0;
    oal_uint8                       uc_retry_test = 0;
    mac_cfg_set_soft_retry_stru    *pst_soft_retry;
    /* 是否为test所设的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_retry_test = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_software_retry = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    pst_soft_retry = (mac_cfg_set_soft_retry_stru *)(st_write_msg.auc_value);
    pst_soft_retry->uc_retry_test = uc_retry_test;
    pst_soft_retry->uc_software_retry = uc_software_retry;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_set_soft_retry_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_SOFT_RETRY, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_open_addr4(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_addr4 = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_addr4 = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_addr4;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_ADDR4, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_open_wmm_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_wmm = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_WMM_TEST, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_chip_test_open(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_chip_test_open = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHIP_TEST_OPEN, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_coex(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_mac_ctrl = 0;
    oal_uint32                      ul_rf_ctrl  = 0;
    mac_cfg_coex_ctrl_param_stru   *pst_coex_ctrl;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_mac_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param   += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_2nd_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_rf_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param  += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    pst_coex_ctrl = (mac_cfg_coex_ctrl_param_stru *)(st_write_msg.auc_value);
    pst_coex_ctrl->ul_mac_ctrl = ul_mac_ctrl;
    pst_coex_ctrl->ul_rf_ctrl  = ul_rf_ctrl;

    us_len = OAL_SIZEOF(mac_cfg_coex_ctrl_param_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_COEX, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_dfx
 功能描述  : 设置DFX特性开关
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月3日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_dfx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DFX_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
OAL_STATIC oal_uint32 wal_hipriv_enable_pmf(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;

    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_chip_test_open = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PMF_ENABLE, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC  oal_uint32 wal_hipriv_test_send_action(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    mac_cfg_send_action_param_stru  st_action_param;

    OAL_MEMZERO(&st_action_param, OAL_SIZEOF(mac_cfg_send_action_param_stru));

    /* 获取uc_category设定的值 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_action_param.uc_category = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 获取目的地址 */
    ul_ret = wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_action_param.auc_mac_da);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, &st_action_param, OAL_SIZEOF(mac_cfg_send_action_param_stru));
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_ACTION, OAL_SIZEOF(mac_cfg_send_action_param_stru));

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_action_param_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;

}


/*****************************************************************************
 函 数 名  : wal_hipriv_send_pspoll
 功能描述  : sta发ps-poll
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_send_pspoll(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_PSPOLL, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_pspoll::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_send_nulldata
 功能描述  : sta发nulldata
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_send_nulldata(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    mac_cfg_tx_nulldata_stru    *pst_tx_nulldata;
    oal_uint32                   ul_off_set = 0;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                   ul_ret;

    pst_tx_nulldata = (mac_cfg_tx_nulldata_stru *)st_write_msg.auc_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_psm = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_qos = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_tidno = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_NULLDATA, OAL_SIZEOF(mac_cfg_tx_nulldata_stru));
    oal_memcopy((oal_void *)st_write_msg.auc_value,
                (const oal_void *)pst_tx_nulldata,
                OAL_SIZEOF(mac_cfg_tx_nulldata_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_nulldata_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_show_mgmt_stat
 功能描述  : 打印管理帧统计信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月10日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_clear_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    /***************************************************************************
                                 抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CLEAR_ALL_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_clear_all_stat::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32 wal_hipriv_set_default_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_setdefaultkey_param_stru  st_payload_params  = {0};
    oal_uint32                     ul_off_set;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                      l_ret;
    wal_msg_write_stru             st_write_msg;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_net_dev)|| (OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_default_key::Param Check ERROR,pst_netdev, pst_params %d, %d!}\r\n",pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 获取key_index*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.uc_key_index = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取en_unicast*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.en_unicast = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取multicast*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.en_multicast = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    /*3.2 填写 msg 消息体 */
    oal_memcopy(st_write_msg.auc_value, &st_payload_params, OAL_SIZEOF(mac_setdefaultkey_param_stru));
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEFAULT_KEY, OAL_SIZEOF(mac_setdefaultkey_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_setdefaultkey_param_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_default_key::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }
    return OAL_SUCC;


}
OAL_STATIC oal_uint32 wal_hipriv_test_add_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru             st_write_msg;
    mac_addkey_param_stru          st_payload_params;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                      l_ret;
    oal_uint32                     ul_off_set;
    oal_uint8                      auc_key[OAL_WPA_KEY_LEN] = {0};
    oal_int8                      *pc_key;
    oal_uint32                     ul_char_index;
    oal_uint16                     us_len;
    wal_msg_stru                  *pst_rsp_msg;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_net_dev)|| (OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::Param Check ERROR,pst_netdev, pst_params %d, %d!}\r\n",pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /*xxx(cipher) xx(en_pairwise) xx(key_len) xxx(key_index) xxxx:xx:xx:xx:xx:xx...(key 小于32字节) xx:xx:xx:xx:xx:xx(目的地址)  */

    oal_memset(&st_payload_params, 0, OAL_SIZEOF(st_payload_params));
    oal_memset(&st_payload_params.st_key, 0, OAL_SIZEOF(mac_key_params_stru));
    st_payload_params.st_key.seq_len = 6;
    OAL_MEMZERO(st_payload_params.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* 获取cipher*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.st_key.cipher = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;


    /* 获取en_pairwise*/
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.en_pairwise = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取key_len */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pc_param = pc_param + ul_off_set;
    st_payload_params.st_key.key_len = (oal_uint8)oal_atoi(ac_name);
    if ((st_payload_params.st_key.key_len > OAL_WPA_KEY_LEN) || (st_payload_params.st_key.key_len < 0) )
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::Param Check ERROR! key_len[%x]  }\r\n",
                      (oal_int32)st_payload_params.st_key.key_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 获取key_index */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    st_payload_params.uc_key_index = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取key */
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    pc_param = pc_param + ul_off_set;
    pc_key = ac_name;
    /* 16进制转换 */
    for (ul_char_index = 0; ul_char_index < ul_off_set; ul_char_index++)
    {
        if ('-' == *pc_key)
        {
            pc_key++;
            if (0 != ul_char_index)
            {
                ul_char_index--;
            }

            continue;
        }

        auc_key[ul_char_index/2] = (oal_uint8)(auc_key[ul_char_index/2] * 16 * (ul_char_index % 2) + oal_strtohex(pc_key));
        pc_key++;
    }
    oal_memcopy(st_payload_params.st_key.auc_key, auc_key, (oal_uint32)st_payload_params.st_key.key_len);


    /* 获取目的地址 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);
    wal_get_cmd_one_arg((oal_int8*)pc_param, ac_name, &ul_off_set);
    oal_strtoaddr(ac_name, st_payload_params.auc_mac_addr);
    pc_param = pc_param + ul_off_set;

    OAM_INFO_LOG3(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::key_len:%d, seq_len:%d, cipher:0x%08x!}\r\n",
                  st_payload_params.st_key.key_len, st_payload_params.st_key.seq_len, st_payload_params.st_key.cipher);


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    /*3.2 填写 msg 消息体 */
    us_len = (oal_uint32)OAL_SIZEOF(mac_addkey_param_stru);
    oal_memcopy(st_write_msg.auc_value, &st_payload_params, (oal_uint32)us_len);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_KEY, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_TRUE,
                             &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }
    return OAL_SUCC;

}
OAL_STATIC oal_uint32  wal_hipriv_set_mib(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_mib_idx = 0;
    oal_uint32                      ul_mib_value = 0;
    mac_cfg_set_mib_stru           *pst_set_mib;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_idx = (oal_uint32)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_value = (oal_uint32)oal_atoi(ac_name);

    pst_set_mib = (mac_cfg_set_mib_stru *)(st_write_msg.auc_value);
    pst_set_mib->ul_mib_idx   = ul_mib_idx;
    pst_set_mib->ul_mib_value = ul_mib_value;
    us_len = OAL_SIZEOF(mac_cfg_set_mib_stru);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MIB, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_mib(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_mib_idx = 0;

    /* 获取mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_idx = (oal_uint32)oal_atoi(ac_name);

    us_len = OAL_SIZEOF(oal_uint32);
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_mib_idx;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_MIB, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mib::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_set_thruput_bypass
 功能描述  : 设置thruput_bypass维测点
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月15日
    作    者   : s00304087
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_thruput_bypass(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                        l_cfg_rst;
    oal_uint16                       us_len;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_off_set = 0;
    oal_uint8                        uc_bypass_type = 0;
    oal_uint8                        uc_value = 0;
    mac_cfg_set_thruput_bypass_stru *pst_set_bypass;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_bypass_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_bypass = (mac_cfg_set_thruput_bypass_stru *)(st_write_msg.auc_value);
    pst_set_bypass->uc_bypass_type   = uc_bypass_type;
    pst_set_bypass->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_thruput_bypass_stru);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_hipriv_performance_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                        l_cfg_rst;
    oal_uint16                       us_len;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_off_set = 0;
    oal_uint8                        uc_performance_switch_type = 0;
    oal_uint8                        uc_value = 0;
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_performance_switch_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_performance_log_switch = (mac_cfg_set_performance_log_switch_stru *)(st_write_msg.auc_value);
    pst_set_performance_log_switch->uc_performance_log_switch_type   = uc_performance_switch_type;
    pst_set_performance_log_switch->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_performance_log_switch_stru);
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::uc_performance_switch_type = %d, uc_value = %d!}\r\n", uc_performance_switch_type,uc_value);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_auto_protection(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_auto_protection_flag = 0;

    /* 获取mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_auto_protection::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_auto_protection_flag = (oal_uint32)oal_atoi(ac_name);

    us_len = OAL_SIZEOF(oal_uint32);
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_auto_protection_flag;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AUTO_PROTECTION, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_auto_protection::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC oal_uint32  wal_hipriv_proxyarp_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_bool_enum_uint8              en_proxyarp_on;
    mac_proxyarp_en_stru            *pst_proxyarp_on_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::get cmd  err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    en_proxyarp_on = (oal_uint8)oal_atoi(ac_name);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_EN, OAL_SIZEOF(mac_proxyarp_en_stru));

    /* 设置配置命令参数 */
    pst_proxyarp_on_param = (mac_proxyarp_en_stru *)(st_write_msg.auc_value);
    pst_proxyarp_on_param->en_proxyarp = en_proxyarp_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_proxyarp_en_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_proxyarp_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_INFO, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_WAPI

#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32  wal_hipriv_show_wapi_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru                    *pst_mac_vap;
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    mac_cfg_user_info_param_stru    *pst_user_info_param;
    oal_uint8                        auc_mac_addr[6] = {0};    /* 临时保存获取的use的mac地址信息 */
    oal_uint8                        uc_char_index;
    oal_uint16                       us_user_idx;
    //OAL_IO_PRINT("wal_hipriv_show_wapi_info::enter\r\n");
    /* 去除字符串的空格 */
    pc_param++;

    /* 获取mac地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++)
    {
        if (':' == *pc_param)
        {
            pc_param++;
            if (0 != uc_char_index)
            {
                uc_char_index--;
            }

            continue;
        }

        auc_mac_addr[uc_char_index/2] =
        (oal_uint8)(auc_mac_addr[uc_char_index/2] * 16 * (uc_char_index % 2) +
                                        oal_strtohex(pc_param));
        pc_param++;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WAPI_INFO, OAL_SIZEOF(mac_cfg_user_info_param_stru));

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    l_ret = (oal_int32)mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != l_ret)
    {
        //OAL_IO_PRINT("wal_hipriv_show_wapi_info::no such user\r\n");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_user_info::no such user!}\r\n");
        return OAL_FAIL;
    }

    /* 设置配置命令参数 */
    pst_user_info_param              = (mac_cfg_user_info_param_stru *)(st_write_msg.auc_value);
    pst_user_info_param->us_user_idx = us_user_idx;
    //OAL_IO_PRINT("wal_hipriv_show_wapi_info::us_user_idx %u\r\n", us_user_idx);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_show_wapi_info::us_user_idx %u", us_user_idx);
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_info_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_user_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef WAPI_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


/*****************************************************************************
 函 数 名  : wal_hipriv_send_2040_coext
 功能描述  : 发送20/40共存管理帧
 输入参数  : pst_net_dev:
             pc_param:
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月25日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_send_2040_coext(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    oal_uint32                      ul_off_set = 0;
    mac_cfg_set_2040_coexist_stru   *pst_2040_coexist;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    pst_2040_coexist = (mac_cfg_set_2040_coexist_stru*)st_write_msg.auc_value;
    /* 获取mib名称 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_coext_info = (oal_uint32)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_channel_report = (oal_uint32)oal_atoi(ac_name);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_2040_COEXT,
                    OAL_SIZEOF(mac_cfg_set_2040_coexist_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_2040_coexist_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_2040_coext_info
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月10日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_2040_coext_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_COEXT_INFO, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_coext_info::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_get_version
 功能描述  : 获取版本
 输入参数  : pst_net_dev:
             pc_param:
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月25日
    作    者   : mayuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_get_version(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_VERSION, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
/*****************************************************************************
 函 数 名  : wal_hipriv_set_opmode_notify
 功能描述  : 设置工作模式通知能力
 输入参数  : pst_net_dev: net device指针
             pc_param: 参数
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月12日
    作    者   : zhangyu
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_opmode_notify(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_value = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_opmode_notify::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_value;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OPMODE_NOTIFY, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_opmode_notify::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_get_user_nssbw
 功能描述  : 设置添加用户的配置命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月5日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_get_user_nssbw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_add_user_param_stru    *pst_add_user_param;
    mac_cfg_add_user_param_stru     st_add_user_param;  /* 临时保存获取的use的信息 */
    oal_uint32                      ul_get_addr_idx;

    /* 获取用户带宽和空间流信息: hipriv "vap0 add_user xx xx xx xx xx xx(mac地址)" */
    OAL_MEMZERO((oal_void *)&st_add_user_param, OAL_SIZEOF(mac_cfg_add_user_param_stru));
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_add_user_param.auc_mac_addr);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    /* 设置配置命令参数 */
    pst_add_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_add_user_param->auc_mac_addr[ul_get_addr_idx] = st_add_user_param.auc_mac_addr[ul_get_addr_idx];
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_USER_RSSBW, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif
OAL_STATIC oal_uint32  wal_hipriv_set_vap_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_vap_nss = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_vap_nss::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_vap_nss = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_vap_nss;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VAP_NSS, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_vap_nss::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_rx_filter_val(oal_int8                **pc_param,
                                                hmac_cfg_rx_filter_stru *pst_rx_filter_val)
{
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                          ul_ret;

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_dev_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    if (pst_rx_filter_val->uc_dev_mode > 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::st_rx_filter_val.uc_dev_mode is exceed.[%d]}\r\n", pst_rx_filter_val->uc_dev_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_vap_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_mode >= WLAN_VAP_MODE_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n", pst_rx_filter_val->uc_vap_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;
    pst_rx_filter_val->uc_vap_status = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_status >= MAC_VAP_STATE_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n", pst_rx_filter_val->uc_vap_status);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    hmac_cfg_rx_filter_stru             st_rx_filter_val = {0};
    wal_msg_write_stru                  st_write_msg;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 0--写某一VAP状态的帧过滤值 */
    st_rx_filter_val.uc_write_read = 0;

    /* 获取需要写入的值 */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::pc_token is null}\r\n");
        return OAL_FAIL;
    }

    st_rx_filter_val.ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value, &st_rx_filter_val, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    hmac_cfg_rx_filter_stru             st_rx_filter_val = {0};
    wal_msg_write_stru                  st_write_msg;

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 1--读某一VAP状态的帧过滤值 */
    st_rx_filter_val.uc_write_read = 1;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value, &st_rx_filter_val, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_rx_filter_en = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_vap_nss::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_rx_filter_en = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rx_filter_en;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_rx_filter_en::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_blacklist_add(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }

    oal_strtoaddr(ac_name, st_write_msg.auc_value); /* 将字符 ac_name 转换成数组 mac_add[6] */

    us_len = OAL_MAC_ADDR_LEN; /* OAL_SIZEOF(oal_uint8); */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_blacklist_del(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }

    oal_strtoaddr(ac_name, st_write_msg.auc_value); /* 将字符 ac_name 转换成数组 mac_add[6] */

    us_len = OAL_MAC_ADDR_LEN; /* OAL_SIZEOF(oal_uint8); */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_send_cfg_uint32_data(oal_net_device_stru *pst_net_dev,
    oal_int8 *pc_param, wlan_cfgid_enum_uint16 cfgid)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      set_value = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_send_cfg_uint32_data:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    set_value = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    us_len = 4; /* OAL_SIZEOF(oal_uint32) */
    *(oal_uint32 *)(st_write_msg.auc_value) = set_value;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, cfgid, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_send_cfg_uint32_data:wal_send_cfg_event return [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_blacklist_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_BLACKLIST_MODE);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_blacklist_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_BLACKLIST_SHOW);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_abl_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_ON);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_abl_aging(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_AGING);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_abl_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_THRESHOLD);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_abl_reset(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_RESET);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_MODE);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_TYPE);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_fwd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_FORWARD);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_clear(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_CLEAR);
}
/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
   1.日    期   : 2014年7月29日
     作    者   : chenchongbao
     修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_isolation_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_SHOW);
}

#endif  /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */
#ifdef _PRE_WLAN_FEATURE_MCAST
OAL_STATIC oal_uint32  wal_hipriv_m2u_snoop_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_snoop_on;
    oal_uint8                        uc_m2u_mcast_mode;
    mac_cfg_m2u_snoop_on_param_stru *pst_m2u_snoop_on_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_snoop_on = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_mcast_mode = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_SNOOP_ON, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    /* 设置配置命令参数 */
    pst_m2u_snoop_on_param = (mac_cfg_m2u_snoop_on_param_stru *)(st_write_msg.auc_value);
    pst_m2u_snoop_on_param->uc_m2u_snoop_on   = uc_m2u_snoop_on;
    pst_m2u_snoop_on_param->uc_m2u_mcast_mode = uc_m2u_mcast_mode;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_m2u_add_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint32                       ul_deny_group_addr;
    mac_add_m2u_deny_table_stru     *pst_m2u_deny_table_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_add_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    ul_deny_group_addr = oal_in_aton((oal_uint8 *)ac_name);

    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_ADD_M2U_DENY_TABLE, OAL_SIZEOF(mac_add_m2u_deny_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_deny_table_param = (mac_add_m2u_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_deny_table_param->ul_deny_group_addr = ul_deny_group_addr;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_add_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_add_deny_table::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_m2u_cfg_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_clear_deny_table;
    oal_uint8                        uc_m2u_show_deny_table;
    mac_clg_m2u_deny_table_stru     *pst_m2u_deny_table_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_clear_deny_table = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_show_deny_table = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_DENY_TABLE, OAL_SIZEOF(mac_clg_m2u_deny_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_deny_table_param = (mac_clg_m2u_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_deny_table_param->uc_m2u_clear_deny_table   = uc_m2u_clear_deny_table;
    pst_m2u_deny_table_param->uc_m2u_show_deny_table    = uc_m2u_show_deny_table;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_clg_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_m2u_show_snoop_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_show_snoop_table;
    mac_show_m2u_snoop_table_stru   *pst_m2u_show_snoop_table_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_show_snoop_table = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_SHOW_M2U_SNOOP_TABLE, OAL_SIZEOF(mac_show_m2u_snoop_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_show_snoop_table_param = (mac_show_m2u_snoop_table_stru *)(st_write_msg.auc_value);
    pst_m2u_show_snoop_table_param->uc_m2u_show_snoop_table   = uc_m2u_show_snoop_table;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_show_m2u_snoop_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_igmp_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_packet_num;
    oal_uint8                       uc_tid;
    oal_uint16                      uc_packet_len;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN];

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_packet_num = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_packet_len = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_IGMP_PACKET_XMIT, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_packet_num = uc_packet_num;
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    pst_aggr_tx_on_param->us_packet_len = uc_packet_len;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

/*****************************************************************************
 函 数 名  : wal_hipriv_vap_classify_en
 功能描述  : 是否使能基于vap的流分类功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月24日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_vap_classify_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC == ul_ret)
    {
        ul_val = (oal_uint32)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_CLASSIFY_EN, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_vap_classify_tid
 功能描述  : 设置vap的流等级
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月24日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_vap_classify_tid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC == ul_ret)
    {
        ul_val = (oal_uint32)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_CLASSIFY_TID, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_scan_test
 功能描述  : 扫描测试命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月29日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_scan_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[15];
    oal_int8                        ac_scan_type[15];
    oal_uint32                      ul_ret;
    oal_uint8                       uc_bandwidth;
    wal_msg_write_stru              st_write_msg;
    mac_ioctl_scan_test_config_stru *pst_scan_test;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_scan_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get first arg fail.");
        return OAL_FAIL;
    }

    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get second arg fail.");
        return OAL_FAIL;
    }
    uc_bandwidth = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                            抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SCAN_TEST, OAL_SIZEOF(mac_ioctl_scan_test_config_stru));

    /* 设置配置命令参数 */
    pst_scan_test = (mac_ioctl_scan_test_config_stru *)(st_write_msg.auc_value);
    oal_memcopy(pst_scan_test->ac_scan_type, ac_scan_type, sizeof(ac_scan_type));
    pst_scan_test->en_bandwidth = uc_bandwidth;

    wal_send_cfg_event(pst_net_dev,
                      WAL_MSG_TYPE_WRITE,
                      WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_scan_test_config_stru),
                      (oal_uint8 *)&st_write_msg,
                      OAL_FALSE,
                      OAL_PTR_NULL);

    return OAL_SUCC;

}
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
extern    oal_uint32 wlan_pm_enable(oal_void);
extern    oal_uint32 wlan_pm_disable(oal_void);
OAL_STATIC oal_uint32  wal_hipriv_sta_ps_mode(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_vap_ps_mode;
    oal_uint8                        uc_hw_ps_enabled;
    oal_uint8                        uc_sdt_enabled;
    oal_uint8                        uc_cfg_pm_mode_manual;
    mac_cfg_ps_mode_param_stru       *pst_ps_on_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_enable::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_vap_ps_mode = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_mode::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_hw_ps_enabled = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_mode::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sdt_enabled = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_mode::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_cfg_pm_mode_manual = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PS_MODE, OAL_SIZEOF(mac_cfg_ps_mode_param_stru));

    /* 设置配置命令参数 */
    pst_ps_on_param = (mac_cfg_ps_mode_param_stru *)(st_write_msg.auc_value);
    pst_ps_on_param->uc_vap_ps_mode   = uc_vap_ps_mode;
    pst_ps_on_param->uc_hw_ps_enabled = uc_hw_ps_enabled;
    pst_ps_on_param->uc_sdt_enabled   = uc_sdt_enabled;
    pst_ps_on_param->uc_cfg_pm_mode_manual = uc_cfg_pm_mode_manual;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(1 == uc_hw_ps_enabled)
    {
        wlan_pm_enable();
    }
    else
    {
        wlan_pm_disable();
    }
#endif
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_ps_enable::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_sta_ps_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_psm_info_enable;
    oal_uint8                        uc_psm_debug_mode;
    mac_cfg_ps_info_stru            *pst_ps_info;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_psm_info_enable = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_psm_debug_mode = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_PS_INFO, OAL_SIZEOF(mac_cfg_ps_info_stru));

    /* 设置配置命令参数 */
    pst_ps_info = (mac_cfg_ps_info_stru *)(st_write_msg.auc_value);
    pst_ps_info->uc_psm_info_enable   = uc_psm_info_enable;
    pst_ps_info->uc_psm_debug_mode    = uc_psm_debug_mode;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_ps_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif
OAL_STATIC oal_uint32  wal_hipriv_sta_psm_param(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    oal_uint16                          us_listen_interval;
    oal_uint16                          us_tbtt_offset;
    oal_uint16                          us_ext_tbtt_offset;
    mac_cfg_ps_param_stru               *pst_ps_para;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_listen_interval = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_tbtt_offset = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_ext_tbtt_offset = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PSM_PARAM, OAL_SIZEOF(mac_cfg_ps_param_stru));

    /* 设置配置命令参数 */
    pst_ps_para = (mac_cfg_ps_param_stru *)(st_write_msg.auc_value);
    pst_ps_para->us_listen_interval     = us_listen_interval;
    pst_ps_para->us_tbtt_offset         = us_tbtt_offset;
    pst_ps_para->us_ext_tbtt_offset     = us_ext_tbtt_offset;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_psm_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_psm_vote(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    oal_uint8                           uc_sta0_vote;
    oal_uint8                           uc_sta1_vote;
    oal_uint8                           uc_sta2_vote;
    oal_uint8                           uc_ap_vote;
    mac_cfg_ps_vote_stru                *pst_ps_vote;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_psm_vote::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sta0_vote = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_psm_vote::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sta1_vote = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_psm_vote::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sta2_vote = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_psm_vote::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_ap_vote = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PSM_VOTE, OAL_SIZEOF(mac_cfg_ps_vote_stru));

    /* 设置配置命令参数 */
    pst_ps_vote = (mac_cfg_ps_vote_stru *)(st_write_msg.auc_value);
    pst_ps_vote->uc_sta0_vote     = uc_sta0_vote;
    pst_ps_vote->uc_sta1_vote     = uc_sta1_vote;
    pst_ps_vote->uc_sta2_vote     = uc_sta2_vote;
    pst_ps_vote->uc_ap_vote       = uc_ap_vote;


    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_vote_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_psm_vote::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_sta_pm_open(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    oal_uint8                           uc_sta_pm_open;
    mac_cfg_ps_open_stru               *pst_sta_pm_open;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_pm_open::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sta_pm_open = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PSM_OPEN, OAL_SIZEOF(mac_cfg_ps_open_stru));

    /* 设置配置命令参数 */
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)(st_write_msg.auc_value);
    pst_sta_pm_open->uc_sta_pm_open = uc_sta_pm_open;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(1 == uc_sta_pm_open)
    {
        wlan_pm_enable();
    }
    else
    {
        wlan_pm_disable();
    }
#endif

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_open_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_pm_open::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC oal_uint32  wal_hipriv_set_uapsd_para(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_uapsd_sta_stru          *pst_uapsd_param;
    oal_uint8                       uc_max_sp_len;
    oal_uint8                       uc_ac;
    oal_uint8                       uc_delivery_enabled[WLAN_WME_AC_BUTT];
    oal_uint8                       uc_trigger_enabled[WLAN_WME_AC_BUTT];

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_uapsd_para::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_max_sp_len = (oal_uint8)oal_atoi(ac_name);

    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++)
    {
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_uapsd_para::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        /* delivery_enabled的参数设置 */
        uc_delivery_enabled[uc_ac] = (oal_uint8)oal_atoi(ac_name);

        /* trigger_enabled 参数的设置 */
        uc_trigger_enabled[uc_ac] = (oal_uint8)oal_atoi(ac_name);
    }
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_UAPSD_PARA, OAL_SIZEOF(mac_cfg_uapsd_sta_stru));

    /* 设置配置命令参数 */
    pst_uapsd_param = (mac_cfg_uapsd_sta_stru *)(st_write_msg.auc_value);
    pst_uapsd_param->uc_max_sp_len   = uc_max_sp_len;
    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++)
    {
        pst_uapsd_param->uc_delivery_enabled[uc_ac] = uc_delivery_enabled[uc_ac];
        pst_uapsd_param->uc_trigger_enabled[uc_ac]  = uc_trigger_enabled[uc_ac];
    }

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_uapsd_sta_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_set_uapsd_para::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32   wal_parse_ops_param(oal_int8 *pc_param, mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param)
{
    oal_uint32                  ul_ret;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_off_set;
    oal_int32                   l_ct_window;

    /* 解析第一个参数，是否使能OPS 节能 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 1 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    if (0 == (oal_strcmp("0", ac_name)))
    {
        pst_p2p_ops_param->en_ops_ctrl  = OAL_FALSE;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        pst_p2p_ops_param->en_ops_ctrl = OAL_TRUE;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::the log switch command is error [%s]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 解析第二个参数，OPS 节能CT Window */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    OAL_IO_PRINT("wal_parse_ops_param:ct window %s\r\n", ac_name);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 2 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    l_ct_window = oal_atoi(ac_name);
    if (l_ct_window < 0 || l_ct_window > 255)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_ct_window);
        return OAL_FAIL;
    }
    else
    {
        pst_p2p_ops_param->uc_ct_window = (oal_uint8)l_ct_window;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32   wal_parse_noa_param(oal_int8 *pc_param, mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param)
{
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_off_set;
    oal_int32                   l_count;
    oal_uint32                  ul_ret;


    /* 解析第一个参数，start_time */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_start_time = (oal_uint32)oal_atoi(ac_name);

    /* 解析第二个参数，dulration */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_duration = (oal_uint32)oal_atoi(ac_name);

    /* 解析第三个参数，interval */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_interval = (oal_uint32)oal_atoi(ac_name);

    /* 解析第四个参数，count */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    l_count = oal_atoi(ac_name);

    if (l_count < 0 || l_count > 255)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_count);
        return OAL_FAIL;
    }
    else
    {
        pst_p2p_noa_param->uc_count  = (oal_uint8)l_count;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_p2p_ps(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_p2p_ops_param_stru  st_p2p_ops_param;
    mac_cfg_p2p_noa_param_stru  st_p2p_noa_param;
    mac_cfg_p2p_stat_param_stru st_p2p_stat_param;
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    oal_uint16                  us_len;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("ops", ac_name)))
    {
        /* 设置P2P OPS 节能参数 */
        ul_ret = wal_parse_ops_param(pc_param, &st_p2p_ops_param);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps ops::ctrl[%d], ct_window[%d]!}\r\n",
                        st_p2p_ops_param.en_ops_ctrl,
                        st_p2p_ops_param.uc_ct_window);
        us_len = OAL_SIZEOF(mac_cfg_p2p_ops_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_OPS, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
        oal_memcopy(st_write_msg.auc_value, &st_p2p_ops_param, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
    }
    else if (0 == (oal_strcmp("noa", ac_name)))
    {
        /* 设置P2P NOA 节能参数 */
        ul_ret = wal_parse_noa_param(pc_param, &st_p2p_noa_param);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        OAM_INFO_LOG4(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps noa::start_time[%d], duration[%d], interval[%d], count[%d]!}\r\n",
                        st_p2p_noa_param.ul_start_time,
                        st_p2p_noa_param.ul_duration,
                        st_p2p_noa_param.ul_interval,
                        st_p2p_noa_param.uc_count);
        us_len = OAL_SIZEOF(mac_cfg_p2p_noa_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_NOA, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
        oal_memcopy(st_write_msg.auc_value, &st_p2p_noa_param, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
    }
    else if (0 == (oal_strcmp("statistics", ac_name)))
    {
        /* 获取P2P节能统计 */
        /* 解析参数，查看节能统计 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 1 return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param   += ul_off_set;
        if (0 == (oal_strcmp("0", ac_name)))
        {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 0;
        }
        else if (0 == (oal_strcmp("1", ac_name)))
        {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 1;
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::wrong parm\r\n}");
            return OAL_FAIL;
        }

        us_len = OAL_SIZEOF(mac_cfg_p2p_stat_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_STAT, us_len);
        oal_memcopy(st_write_msg.auc_value, &st_p2p_stat_param, us_len);
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::ctrl[%d], len:%d!}\r\n",
                        st_p2p_stat_param.uc_p2p_statistics_ctrl, us_len);
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

OAL_STATIC oal_uint32  wal_hipriv_p2p_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_p2p_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("del_intf", ac_name)))
    {
        oal_uint32              ul_del_intf = 0;
        mac_vap_stru           *pst_mac_vap;
        hmac_device_stru       *pst_hmac_device;

        /* 获取参数 */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::wal_get_cmd_one_arg 1 return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param   += ul_off_set;
        if (0 == (oal_strcmp("0", ac_name)))
        {
            ul_del_intf = 0;
        }
        else if (0 == (oal_strcmp("1", ac_name)))
        {
            ul_del_intf = 1;
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_p2p_test ::wrong parm.ul_del_intf\r\n}");
            return OAL_FAIL;
        }

        pst_mac_vap     = OAL_NET_DEV_PRIV(pst_net_dev);
        pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
        if (pst_hmac_device == OAL_PTR_NULL)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{wal_hipriv_p2p_test::pst_hmac_device is null !device_id[%d]}",
                        pst_mac_vap->uc_device_id);
            return OAL_FAIL;
        }

        if (ul_del_intf == 1)
        {
            hmac_set_p2p_status(&pst_hmac_device->ul_p2p_intf_status, P2P_STATUS_IF_DELETING);
        }
        else
        {
            hmac_clr_p2p_status(&pst_hmac_device->ul_p2p_intf_status, P2P_STATUS_IF_DELETING);
        }

        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_hipriv_p2p_test ::ctrl[%d], len:%d!}\r\n",
                        ul_del_intf, pst_hmac_device->ul_p2p_intf_status);

        OAL_SMP_MB();
        OAL_WAIT_QUEUE_WAKE_UP(&pst_hmac_device->st_netif_change_event);

    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    #if 0
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    #endif

    return OAL_SUCC;
}


#endif  /* _PRE_WLAN_FEATURE_P2P */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
/*****************************************************************************
 函 数 名  : wal_hipriv_set_ampdu_mmss
 功能描述  : 设置AMPDU的mmss字段
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月20日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_mmss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru       st_write_msg;
    oal_uint32               ul_off_set;
    oal_int8                 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_ampdu_mmss_stru  st_ampdu_mmss_cfg;
    oal_uint32               ul_ret;
    oal_int32                l_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_mmss_cfg.uc_mmss_val = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_mmss_cfg.uc_mmss_val > 7)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n", st_ampdu_mmss_cfg.uc_mmss_val);
        OAL_IO_PRINT("{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n", st_ampdu_mmss_cfg.uc_mmss_val);

        return OAL_FAIL;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_MMSS, OAL_SIZEOF(st_ampdu_mmss_cfg));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_ampdu_mmss_cfg, OAL_SIZEOF(st_ampdu_mmss_cfg));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_mmss_cfg),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
/*****************************************************************************
 函 数 名  : wal_hipriv_set_mips
 功能描述  :
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月12日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_set_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_mips_type;
    oal_int32                       l_switch;
    wal_msg_write_stru              st_write_msg;
    oal_mips_type_param_stru       *pst_mips_type_param;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_switch = oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MIPS, OAL_SIZEOF(oal_mips_type_param_stru));
    pst_mips_type_param = (oal_mips_type_param_stru *)st_write_msg.auc_value;
    pst_mips_type_param->l_mips_type = l_mips_type;
    pst_mips_type_param->l_switch    = l_switch;


    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_mips_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_show_mips
 功能描述  :
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月12日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32  wal_hipriv_show_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_mips_type;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_MIPS, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_mips_type;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
/*****************************************************************************
 函 数 名  : wal_hipriv_arp_offload_enable
 功能描述  : 用命令打开/关闭ARP offload功能
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                     ul_ret;
    oal_int32                      l_ret;
    oal_uint32                     ul_off_set;
    oal_switch_enum_uint8          en_switch;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru             st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "wal_hipriv_arp_offload_enable return err_code: %d", ul_ret);
        return ul_ret;
    }
    en_switch = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ENABLE_ARP_OFFLOAD, OAL_SIZEOF(oal_switch_enum_uint8));
    *(oal_switch_enum_uint8 *)(st_write_msg.auc_value) = en_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_arp_offload_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_show_arpoffload_info
 功能描述  : 显示Device侧记录的IP地址
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月10日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_show_arpoffload_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_show_ip_addr;
    oal_uint8                        uc_show_arpoffload_info;
    mac_cfg_arpoffload_info_stru     *pst_arpoffload_info;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_show_ip_addr = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_show_arpoffload_info = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_ARPOFFLOAD_INFO, OAL_SIZEOF(mac_cfg_arpoffload_info_stru));

    /* 设置配置命令参数 */
    pst_arpoffload_info = (mac_cfg_arpoffload_info_stru *)(st_write_msg.auc_value);
    pst_arpoffload_info->uc_show_ip_addr            = uc_show_ip_addr;
    pst_arpoffload_info->uc_show_arpoffload_info    = uc_show_arpoffload_info;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_arpoffload_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_show_arpoffload_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_uint32  wal_hipriv_get_tcp_ack_stream_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_TCP_ACK_STREAM_INFO, OAL_SIZEOF(oal_uint32));


    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_opt_enable::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_rx_ack_opt_enable::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    wal_send_cfg_event(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    return OAL_SUCC;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru      st_write_msg;
    oal_int32               l_ret;
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru *pst_write_rsp_msg;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru   *pst_wdev;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    OAL_IO_PRINT("wal_start_vap,dev_name is:%s\n", pst_net_dev->name);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_VAP, OAL_SIZEOF(mac_cfg_start_vap_param_stru));
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_start_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_mgmt_rate_init_flag = OAL_TRUE;

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 处理返回消息 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::hmac start vap fail, err code[%d]!}\r\n", pst_write_rsp_msg->ul_err_code);
            return -OAL_EINVAL;
        }
    }

    if (0 == (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING))
    {
        OAL_NETDEVICE_FLAGS(pst_net_dev) |= OAL_IFF_RUNNING;
    }

    return OAL_SUCC;
}
oal_int32  wal_stop_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru      st_write_msg;
    oal_int32               l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru   *pst_wdev;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    /* 如果不是up状态，则直接返回成功 */
    if (0 == (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING))
    {
        OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::vap is already down!}\r\n");
        return OAL_SUCC;
    }

    OAL_IO_PRINT("wal_stop_vap,dev_name is:%s\n", pst_net_dev->name);

    /***************************************************************************
                           抛事件到wal层处理
    ***************************************************************************/
    /* 填写WID消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DOWN_VAP, OAL_SIZEOF(mac_cfg_down_vap_param_stru));
    ((mac_cfg_down_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (WLAN_P2P_BUTT == en_p2p_mode)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_stop_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}



oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev)
{
    oal_net_device_stru        *pst_cfg_net_dev;
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru     *pst_write_rsp_msg;
    mac_vap_stru               *pst_mac_vap;
    oal_wireless_dev_stru      *pst_wdev;
    mac_wiphy_priv_stru        *pst_wiphy_priv;
    mac_vap_stru               *pst_cfg_mac_vap;
    hmac_vap_stru              *pst_cfg_hmac_vap;
    mac_device_stru            *pst_mac_device;
    oal_int32                   l_ret;

    wlan_vap_mode_enum_uint8    en_vap_mode;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if(NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_net_dev is null!}\r\n");
        return -OAL_EINVAL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL != pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_vap is already exist}\r\n");
        return OAL_SUCC;
    }

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if(NULL == pst_wdev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_wdev is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_wiphy_priv  = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    pst_mac_device  = pst_wiphy_priv->pst_mac_device;
    if(NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_device is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_cfg_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->uc_cfg_vap_id);
    pst_cfg_hmac_vap= (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;
    if(NULL == pst_cfg_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_net_dev is null!}\r\n");
        return -OAL_EFAUL;
    }

    /* 仅用于WIFI和AP打开时创建VAP */
    if((NL80211_IFTYPE_STATION == pst_wdev->iftype) || (NL80211_IFTYPE_P2P_DEVICE == pst_wdev->iftype))
    {
        if(0 == (oal_strcmp("wlan0", pst_net_dev->name)))
        {
            en_vap_mode = WLAN_VAP_MODE_BSS_STA;
        }
#ifdef _PRE_WLAN_FEATURE_P2P
        else if(0 == (oal_strcmp("p2p0", pst_net_dev->name)))
        {
            en_vap_mode = WLAN_VAP_MODE_BSS_STA;
            en_p2p_mode= WLAN_P2P_DEV_MODE;
        }
#endif
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::net_dev is not wlan0 or p2p0!}\r\n");
            return OAL_SUCC;
        }
    }
    else if(NL80211_IFTYPE_AP == pst_wdev->iftype)
    {
        en_vap_mode = WLAN_VAP_MODE_BSS_AP;
    }
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::net_dev is not wlan0 or p2p0!}\r\n");
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_init_wlan_vap::en_vap_mode:%d,en_p2p_mode:%d}\r\n",
                     en_vap_mode, en_p2p_mode);
#endif

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_VAP, OAL_SIZEOF(mac_cfg_add_vap_param_stru));
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev     = pst_net_dev;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_vap_mode     = en_vap_mode;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->uc_cfg_vap_indx = pst_cfg_mac_vap->uc_vap_id;
#ifdef _PRE_WLAN_FEATURE_P2P
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode      = en_p2p_mode;
#endif
    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::return err code %d!}\r\n", l_ret);
        return -OAL_EFAIL;
    }

    /* 读取返回的错误码 */
    if(OAL_PTR_NULL != pst_rsp_msg)
    {
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        if (OAL_SUCC != pst_write_rsp_msg->ul_err_code)
        {
            OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::hmac add vap fail, err code[%d]!}\r\n", pst_write_rsp_msg->ul_err_code);
            return -OAL_EFAIL;
        }
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if(OAL_SUCC != wal_set_random_mac_to_mib(pst_net_dev))
    {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::wal_set_random_mac_to_mib fail!}\r\n");
        return -OAL_EFAUL;
    }
#endif

    /* 设置netdevice的MAC地址，MAC地址在HMAC层被初始化到MIB中 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_p2p_mode == WLAN_P2P_DEV_MODE)
    {
        pst_mac_device->st_p2p_info.uc_p2p0_vap_idx = pst_mac_vap->uc_vap_id;
    }
#endif

    if(NL80211_IFTYPE_AP == pst_wdev->iftype)
    {
        /* AP模式初始化，初始化配置最大用户数和mac地址过滤模式 */
        if(g_st_ap_config_info.ul_ap_max_user > 0)
        {
            wal_set_ap_max_user(pst_net_dev, g_st_ap_config_info.ul_ap_max_user);
        }

        if(OAL_STRLEN(g_st_ap_config_info.ac_ap_mac_filter_mode) > 0)
        {
            wal_config_mac_filter(pst_net_dev, g_st_ap_config_info.ac_ap_mac_filter_mode);
        }
    }

    return OAL_SUCC;
}
oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru           st_write_msg;
    wal_msg_stru               *pst_rsp_msg;
    mac_vap_stru                *pst_mac_vap;
    oal_int32                    l_ret;

#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::pst_del_vap_param null ptr !}\r\n");
        return -OAL_EINVAL;
    }


    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::pst_mac_vap is already null}\r\n");
        return OAL_SUCC;
    }

    /* 仅用于WIFI和AP关闭时删除VAP */
    if ((0 != (oal_strcmp("wlan0", pst_net_dev->name))) && (0 != (oal_strcmp("p2p0", pst_net_dev->name))))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::net_dev is not wlan0 or p2p0!}\r\n");
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    if(0 == oal_strcmp("p2p0", pst_net_dev->name))
    {
        en_p2p_mode = WLAN_P2P_DEV_MODE;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    //删除vap 时需要将参数赋值。
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::return error code %d}\r\n", l_ret);
    }

    OAL_NET_DEV_PRIV(pst_net_dev) = NULL;

    return l_ret;
}



OAL_STATIC oal_int32 wal_set_mac_addr(oal_net_device_stru *pst_net_dev)
{
    oal_uint8                     auc_primary_mac_addr[WLAN_MAC_ADDR_LEN] = {0};    /* MAC地址 */
    oal_wireless_dev_stru        *pst_wdev;
    mac_wiphy_priv_stru          *pst_wiphy_priv;
    mac_device_stru              *pst_mac_device;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    pst_wiphy_priv = (mac_wiphy_priv_stru *)(oal_wiphy_priv(pst_wdev->wiphy));
    pst_mac_device = pst_wiphy_priv->pst_mac_device;

#ifdef _PRE_WLAN_FEATURE_P2P
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device->st_p2p_info.pst_primary_net_device))
    {
        /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
        oal_random_ether_addr(auc_primary_mac_addr);
        auc_primary_mac_addr[0] &= (~0x02);
        auc_primary_mac_addr[1] = 0x11;
        auc_primary_mac_addr[2] = 0x02;
    }
    else
    {
#ifndef _PRE_PC_LINT
        if(OAL_LIKELY(OAL_PTR_NULL != OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device)))
        {
            oal_memcopy(auc_primary_mac_addr, OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device), WLAN_MAC_ADDR_LEN);
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_mac_addr() pst_primary_net_device; addr is null}\r\n");
            return OAL_FAIL;
        }
#endif
    }

    switch (pst_wdev->iftype)
    {
        case NL80211_IFTYPE_P2P_DEVICE:
        {
            /* 产生P2P device MAC 地址，将本地mac 地址bit 设置为1 */
            auc_primary_mac_addr[0] |= 0x02;
            oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);

            break;
        }
        default:
        {
            oal_random_ether_addr(auc_primary_mac_addr);
            auc_primary_mac_addr[0]&=(~0x02);
            auc_primary_mac_addr[1]=0x11;
            auc_primary_mac_addr[2]=0x02;

            oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);
            break;
        }
    }
#else
    oal_random_ether_addr(auc_primary_mac_addr);
    auc_primary_mac_addr[0]&=(~0x02);
    auc_primary_mac_addr[1]=0x11;
    auc_primary_mac_addr[2]=0x02;

    oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);
#endif

    return OAL_SUCC;
}
oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, char *dev_name)
{
    oal_net_device_stru        *pst_net_dev;
    oal_wireless_dev_stru      *pst_wdev;
    mac_wiphy_priv_stru        *pst_wiphy_priv;
    enum nl80211_iftype         en_type;
    oal_int32                   l_ret;

    if((NULL == pst_wiphy) || (NULL == dev_name))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::pst_wiphy or dev_name is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (0 == (oal_strcmp("wlan0", dev_name)))
    {
        en_type = NL80211_IFTYPE_STATION;
    }
    else if (0 == (oal_strcmp("p2p0", dev_name)))
    {
        en_type = NL80211_IFTYPE_P2P_DEVICE;
    }
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::dev name is not wlan0 or p2p0}\r\n");
        return OAL_SUCC;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_netdev::en_type is %d}\r\n", en_type);

    /* 如果创建的net device已经存在，直接返回 */
    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(dev_name);
    if (OAL_PTR_NULL != pst_net_dev)
    {
        /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
        oal_dev_put(pst_net_dev);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::the net_device is already exist!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_WLAN_FEATURE_FLOWCTL)
    pst_net_dev = oal_net_alloc_netdev_mqs(0, dev_name, oal_ether_setup, WAL_NETDEV_SUBQUEUE_MAX_NUM, 1);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
#elif defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL)
    pst_net_dev = oal_net_alloc_netdev_mqs(0, dev_name, oal_ether_setup, WLAN_NET_QUEUE_BUTT, 1);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
    OAM_INFO_LOG1(0,OAM_SF_ANY, "after alloc_netdev: txq_num = %d\r\n", OAL_NETDEVICE_TX_QUEUE_NUM(pst_net_dev));
#else
    pst_net_dev = oal_net_alloc_netdev(0, dev_name, oal_ether_setup);    /* 此函数第一个入参代表私有长度，此处不涉及为0 */
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::oal_net_alloc_netdev return null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wdev = (oal_wireless_dev_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(oal_wireless_dev_stru), OAL_FALSE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_wdev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::alloc mem, pst_wdev is null ptr!}\r\n");
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memset(pst_wdev, 0, OAL_SIZEOF(oal_wireless_dev_stru));

    /* 对netdevice进行赋值 */
    pst_net_dev->wireless_handlers             = &g_st_iw_handler_def;
    pst_net_dev->netdev_ops                    = &g_st_wal_net_dev_ops;
    OAL_NETDEVICE_DESTRUCTOR(pst_net_dev)      = oal_net_free_netdev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,44))
    OAL_NETDEVICE_MASTER(pst_net_dev)          = OAL_PTR_NULL;
#endif

    OAL_NETDEVICE_IFALIAS(pst_net_dev)         = OAL_PTR_NULL;
    OAL_NETDEVICE_WATCHDOG_TIMEO(pst_net_dev)  = 5;
    OAL_NETDEVICE_WDEV(pst_net_dev)            = pst_wdev;
    OAL_NETDEVICE_QDISC(pst_net_dev, OAL_PTR_NULL);

    pst_wdev->netdev = pst_net_dev;
    pst_wdev->iftype = en_type;
    pst_wdev->wiphy = pst_wiphy;
    pst_wiphy_priv = (mac_wiphy_priv_stru *)(oal_wiphy_priv(pst_wiphy));

#ifdef _PRE_WLAN_FEATURE_P2P
    if ((NL80211_IFTYPE_STATION == en_type))
    {
        /* 如果创建wlan0， 则保存wlan0 为主net_device,p2p0 和p2p-p2p0 MAC 地址从主netdevice 获取 */
        pst_wiphy_priv->pst_mac_device->st_p2p_info.pst_primary_net_device = pst_net_dev;
    }
    else if(NL80211_IFTYPE_P2P_DEVICE == en_type)
    {
        pst_wiphy_priv->pst_mac_device->st_p2p_info.pst_p2p_net_device = pst_net_dev;
    }
#endif
    OAL_NETDEVICE_FLAGS(pst_net_dev) &= ~OAL_IFF_RUNNING;   /* 将net device的flag设为down */

    wal_set_mac_addr(pst_net_dev);

    /* 注册net_device */
    l_ret = oal_net_register_netdev(pst_net_dev);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {

        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_netdev::oal_net_register_netdev return error code %d!}\r\n", l_ret);

        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);

        return l_ret;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
/*****************************************************************************
 函 数 名  : wal_hipriv_max_user_num_each_device
 功能描述  :  设置各设备关联用户个数
 输入参数  : oal_net_device_stru *pst_net_dev
             oal_int8 *pc_param
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月20日
    作    者   : wwx293929
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC oal_uint32 wal_hipriv_max_user_num_each_device(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru             *pst_mac_vap;
    mac_device_stru         *pst_mac_device;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    oal_int8                      ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint16                  us_user_val;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_max_user_num_each_device::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_max_user_num_each_device::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_max_user_num_each_device::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    us_user_val = (oal_uint16)oal_atoi(ac_param);
    if (us_user_val > WLAN_ASSOC_USER_MAX_NUM_SPEC)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_max_user_num_each_device::uc_user_val[%d].}", us_user_val);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_mac_device->us_user_nums_max = us_user_val;
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_bool_enum_uint8        en_enable;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam enable type return err_code [%d]", ul_ret);
        return ul_ret;
    }
    en_enable = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ENABLE, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_org;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam org type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_org = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ORG, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_org;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_org::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_band;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam band type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_band = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_BAND, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_band;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_band::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    en_enable = 1;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_START, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    en_enable = 1;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_INFO, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32  wal_hipriv_enable_2040bss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_uint8                       uc_2040bss_switch;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
         return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::invalid parameter.}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    uc_2040bss_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040BSS_ENABLE, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_2040bss_switch;  /* 设置配置命令参数 */

    ul_ret = (oal_uint32)wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::return err code %d!}\r\n", ul_ret);
    }

    return ul_ret;
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
/*****************************************************************************
 函 数 名  : wal_hipriv_register_inetaddr_notifier
 功能描述  : 注册网络接口的通知链
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_register_inetaddr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (0 == register_inetaddr_notifier(&wal_hipriv_notifier))
    {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_register_inetaddr_notifier::register inetaddr notifier failed.}");
    return OAL_FAIL;

#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_unregister_inetaddr_notifier
 功能描述  : 注销网络接口的通知链
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_unregister_inetaddr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (0 == unregister_inetaddr_notifier(&wal_hipriv_notifier))
    {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_unregister_inetaddr_notifier::hmac_unregister inetaddr notifier failed.}");
    return OAL_FAIL;

#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_register_inet6addr_notifier
 功能描述  : 注册IPv6网络接口的通知链
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月15日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_register_inet6addr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (0 == register_inet6addr_notifier(&wal_hipriv_notifier_ipv6))
    {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_register_inet6addr_notifier::register inetaddr6 notifier failed.}");
    return OAL_FAIL;

#endif
    return OAL_SUCC;
}

/*****************************************************************************
 函 数 名  : wal_hipriv_unregister_inet6addr_notifier
 功能描述  : 注销IPv6网络接口的通知链
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_uint32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月15日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_uint32 wal_hipriv_unregister_inet6addr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (0 == unregister_inet6addr_notifier(&wal_hipriv_notifier_ipv6))
    {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_unregister_inet6addr_notifier::hmac_unregister inetaddr6 notifier failed.}");
    return OAL_FAIL;

#endif
    return OAL_SUCC;
}


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*****************************************************************************
 函 数 名  : wal_hipriv_inetaddr_notifier_call
 功能描述  : 通知链回调函数
 输入参数  : struct notifier_block *this
             oal_uint event
             oal_void *ptr
 输出参数  : 无
 返 回 值  : oal_int32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月21日
    作    者   : w00316376
    修改内容   : 新生成函数

*****************************************************************************/
oal_int32 wal_hipriv_inetaddr_notifier_call(struct notifier_block *this, oal_uint event, oal_void *ptr)
{
    /*
     * Notification mechanism from kernel to our driver. This function is called by the Linux kernel
     * whenever there is an event related to an IP address.
     * ptr : kernel provided pointer to IP address that has changed
     */
    struct in_ifaddr    *pst_ifa       = (struct in_ifaddr *)ptr;
    mac_vap_stru        *pst_mac_vap;

    if (OAL_UNLIKELY(NULL == pst_ifa) || OAL_UNLIKELY(NULL == pst_ifa->ifa_dev->dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::pst_ifa is NULL.}");
        return NOTIFY_DONE;
    }

    /* Filter notifications meant for non Hislicon devices */
    if (pst_ifa->ifa_dev->dev->netdev_ops != &g_st_wal_net_dev_ops)
    {
        return NOTIFY_DONE;
    }

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_ifa->ifa_dev->dev);
    if (OAL_UNLIKELY(NULL == pst_mac_vap))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::Get mac vap failed, when %d(UP:1 DOWN:2 UNKNOWN:others) ipv4 address.}", event);
        return NOTIFY_DONE;
    }

    switch (event)
    {
        case NETDEV_UP:
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::Up IPv4[%d.X.X.%d].}",
                             ((oal_uint8 *)&(pst_ifa->ifa_address))[0],
                             ((oal_uint8 *)&(pst_ifa->ifa_address))[3]);
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_ADD, &(pst_ifa->ifa_address));

            /* 获取到IP地址的时候开启低功耗 */
            if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
            {
               hmac_vap_stru  *pst_hmac_vap;

#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
               wlan_pm_set_timeout(DEFAULT_WDG_TIMEOUT);
#endif
               pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
               FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_ps_sw_timer));
            }
            break;
        }


        case NETDEV_DOWN:
        {
#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
            wlan_pm_set_timeout(LONG_WDG_TIMETOUT);
#endif
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::Down IPv4[%d.X.X.%d].}",
                             ((oal_uint8 *)&(pst_ifa->ifa_address))[0],
                             ((oal_uint8 *)&(pst_ifa->ifa_address))[3]);
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_DEL, &(pst_ifa->ifa_address));
            break;
        }

        default:
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::Unknown notifier event[%d].}", event);
            break;
        }
    }

    return NOTIFY_DONE;
}


/*****************************************************************************
 函 数 名  : wal_hipriv_inet6addr_notifier_call
 功能描述  : IPv6通知链回调函数
 输入参数  : struct notifier_block *this
             oal_uint event
             oal_void *ptr
 输出参数  : 无
 返 回 值  : oal_int32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月15日
    作    者   : Wlan_mib_temp
    修改内容   : 新生成函数

*****************************************************************************/
oal_int32 wal_hipriv_inet6addr_notifier_call(struct notifier_block *this, oal_uint event, oal_void *ptr)
{
    /*
     * Notification mechanism from kernel to our driver. This function is called by the Linux kernel
     * whenever there is an event related to an IP address.
     * ptr : kernel provided pointer to IP address that has changed
     */
    struct inet6_ifaddr    *pst_ifa       = (struct inet6_ifaddr *)ptr;
    mac_vap_stru           *pst_mac_vap;

    if (OAL_UNLIKELY(NULL == pst_ifa) || OAL_UNLIKELY(NULL == pst_ifa->idev->dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::pst_ifa is NULL.}");
        return NOTIFY_DONE;
    }

    /* Filter notifications meant for non Hislicon devices */
    if (pst_ifa->idev->dev->netdev_ops != &g_st_wal_net_dev_ops)
    {
        return NOTIFY_DONE;
    }

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_ifa->idev->dev);
    if (OAL_UNLIKELY(NULL == pst_mac_vap))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::Get mac vap failed, when %d(UP:1 DOWN:2 UNKNOWN:others) ipv6 address.}", event);
        return NOTIFY_DONE;
    }

    switch (event)
    {
        case NETDEV_UP:
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::UP IPv6[%04x:%04x:XXXX:XXXX:XXXX:XXXX:%04x:%04x].}",
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[0]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[1]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[6]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[7]));
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV6, DMAC_IP_ADDR_ADD, &(pst_ifa->addr));
            break;
        }

        case NETDEV_DOWN:
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::DOWN IPv6[%04x:%04x:XXXX:XXXX:XXXX:XXXX:%04x:%04x].}",
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[0]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[1]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[6]),
                             OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[7]));
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV6, DMAC_IP_ADDR_DEL, &(pst_ifa->addr));
            break;
        }

        default:
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::Unknown notifier event[%d].}", event);
            break;
        }
    }

    return NOTIFY_DONE;
}
#endif /* #if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
#endif /* #ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD */

/*lint -e19*/
oal_module_symbol(wal_hipriv_proc_write);
oal_module_symbol(wal_hipriv_get_mac_addr);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

