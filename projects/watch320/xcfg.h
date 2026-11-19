#ifndef WATCH320_XCFG_CONFIG_H
#define WATCH320_XCFG_CONFIG_H

#include "Output/bin/xcfg.h"

/* 初值取自 Output/bin/Settings/watch.setting，缺少的配置项沿用 xcfg.xm 默认值。
 * 蓝牙基础地址、序列号和晶振产测值在编译时确定。
 */

/* 系统配置 */
#define XCFG_POWKEY_10S_RESET              0U
#define XCFG_SYS_SLEEP_TIME                0U
#define XCFG_VOL_MAX                       0U
#define XCFG_SYS_INIT_VOL                  16U
#define XCFG_WARNING_VOLUME                16U
#define XCFG_OSC_BOTH_CAP                  0U
#define XCFG_UOSCI_CAP                     43U
#define XCFG_UOSCO_CAP                     43U
#define XCFG_FT_OSC_CAP_EN                 1U
#define XCFG_EQ_DGB_SPP_EN                 0U
#define XCFG_HUART_DUMP_EN                 0U
#define XCFG_HUART_DUMP_SEL                1U
#define XCFG_FMRX_CFG_EN                   0U

/* 电源配置 */
#define XCFG_BUCK_MODE_EN                  1U
#define XCFG_VDDBT_CAPLESS_EN              0U
#define XCFG_VDDIO_SEL                     7U
#define XCFG_VDDBT_SEL                     10U

/* DAC配置 */
#define XCFG_DAC_SEL                       1U
#define XCFG_DACAUD_LDO_SEL                5U
#define XCFG_DACAUD_BYPASS_EN              0U
#define XCFG_DAC_MAX_GAIN                  0U
#define XCFG_BT_CALL_MAX_GAIN              1U

/* BT发射配置 */
#define XCFG_BT_CONNECT_NAME_EN            0U
#define XCFG_BT_CONNECT_NAME               "BT-BOX"

/* 充电配置 */
#define XCFG_CHARGE_EN                     1U
#define XCFG_CHARGE_TRICK_EN               1U
#define XCFG_CHARGE_DC_RESET               1U
#define XCFG_CHARGE_DC_NOT_PWRON           1U
#define XCFG_CHARGE_STOP_CURR              6U
#define XCFG_CHARGE_CONSTANT_CURR          21U
#define XCFG_CHARGE_TRICKLE_CURR           5U

/* 蓝牙配置 */
#define XCFG_BT_NAME                       "BT-CS32S123"
#define XCFG_BT_ADDR                       { 0x41U, 0x42U, 0x00U, 0x00U, 0x00U, 0x01U }
#define XCFG_BT_RF_TXPWR_RECON             0U
#define XCFG_BT_A2DP_EN                    1U
#define XCFG_BT_A2DP_VOL_CTRL_EN           1U
#define XCFG_BT_SCO_EN                     1U
#define XCFG_BT_HFP_PRIVATE_EN             0U
#define XCFG_BT_HFP_RING_NUMBER_EN         1U
#define XCFG_BT_HFP_INBAND_RING_EN         0U
#define XCFG_BT_SPP_EN                     0U
#define XCFG_BLE_EN                        1U
#define XCFG_LE_NAME                       "BT-CS32S123"

/* 蓝牙RF配置 */
#define XCFG_BT_RF_PWRDEC                  ((u8)(0))
#define XCFG_FT_RF_PARAM_EN                1U
#define XCFG_BT_RF_PARAM_EN                0U
#define XCFG_RF_PA_GAIN                    7U
#define XCFG_RF_MIX_GAIN                   5U
#define XCFG_RF_DIG_GAIN                   54U
#define XCFG_RF_PA_CAP                     0U
#define XCFG_RF_MIX_CAP                    8U
#define XCFG_RF_TXDBM                      8U

/* MIC参数 */
#define XCFG_BT_MMIC_CFG                   0U
#define XCFG_BT_SMIC_CFG                   1U
#define XCFG_MIC_POST_GAIN                 1U
#define XCFG_MIC0_EN                       1U
#define XCFG_MIC0_BIAS_METHOD              0U
#define XCFG_MIC0_PWR_SEL                  2U
#define XCFG_BT_MIC0_DIG_GAIN              21U
#define XCFG_MIC1_EN                       0U
#define XCFG_MIC1_BIAS_METHOD              0U
#define XCFG_MIC1_PWR_SEL                  0U
#define XCFG_BT_MIC1_DIG_GAIN              10U

/* 通话参数 */
#define XCFG_BT_SCO_NR_EN                  1U
#define XCFG_BT_SCO_NR_LEVEL               6U
#define XCFG_BT_AEC_EN                     1U
#define XCFG_BT_ECHO_LEVEL                 3U
#define XCFG_BT_AEC_DIG_GAIN               9U
#define XCFG_BT_FAR_OFFSET                 42U
#define XCFG_BT_ALC_EN                     0U

/* FM收音配置 */
#define XCFG_FMRX_R_VAL                    128U
#define XCFG_FMRX_Z_VAL                    1100U
#define XCFG_FMRX_FZ_VAL                   600U
#define XCFG_FMRX_D_VAL                    3000U
#define XCFG_FMRX_CS_FILTER_FIXED          1U
#define XCFG_FMRX_CS_FILTER_SEL            2U
#define XCFG_FMRX_AUDIO_CH                 0U

/* 生产配置 */
#define XCFG_SERIAL_2                      "ABAB"
#define XCFG_SERIAL_NUM                    { 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U }

/* 产测参数 */
#define XCFG_OSCI_CAP                      31U
#define XCFG_OSCO_CAP                      31U
#define XCFG_SOFT_KEY                      { 0 }
#define XCFG_ASR_SOFT_KEY                  { 0 }

#define XCFG_STATIC_INIT { \
    .powkey_10s_reset = XCFG_POWKEY_10S_RESET, \
    .sys_sleep_time = XCFG_SYS_SLEEP_TIME, \
    .vol_max = XCFG_VOL_MAX, \
    .sys_init_vol = XCFG_SYS_INIT_VOL, \
    .warning_volume = XCFG_WARNING_VOLUME, \
    .osc_both_cap = XCFG_OSC_BOTH_CAP, \
    .uosci_cap = XCFG_UOSCI_CAP, \
    .uosco_cap = XCFG_UOSCO_CAP, \
    .ft_osc_cap_en = XCFG_FT_OSC_CAP_EN, \
    .eq_dgb_spp_en = XCFG_EQ_DGB_SPP_EN, \
    .huart_dump_en = XCFG_HUART_DUMP_EN, \
    .huart_dump_sel = XCFG_HUART_DUMP_SEL, \
    .fmrx_cfg_en = XCFG_FMRX_CFG_EN, \
    .buck_mode_en = XCFG_BUCK_MODE_EN, \
    .vddbt_capless_en = XCFG_VDDBT_CAPLESS_EN, \
    .vddio_sel = XCFG_VDDIO_SEL, \
    .vddbt_sel = XCFG_VDDBT_SEL, \
    .dac_sel = XCFG_DAC_SEL, \
    .dacaud_ldo_sel = XCFG_DACAUD_LDO_SEL, \
    .dacaud_bypass_en = XCFG_DACAUD_BYPASS_EN, \
    .dac_max_gain = XCFG_DAC_MAX_GAIN, \
    .bt_call_max_gain = XCFG_BT_CALL_MAX_GAIN, \
    .bt_connect_name_en = XCFG_BT_CONNECT_NAME_EN, \
    .bt_connect_name = XCFG_BT_CONNECT_NAME, \
    .charge_en = XCFG_CHARGE_EN, \
    .charge_trick_en = XCFG_CHARGE_TRICK_EN, \
    .charge_dc_reset = XCFG_CHARGE_DC_RESET, \
    .charge_dc_not_pwron = XCFG_CHARGE_DC_NOT_PWRON, \
    .charge_stop_curr = XCFG_CHARGE_STOP_CURR, \
    .charge_constant_curr = XCFG_CHARGE_CONSTANT_CURR, \
    .charge_trickle_curr = XCFG_CHARGE_TRICKLE_CURR, \
    .bt_name = XCFG_BT_NAME, \
    .bt_addr = XCFG_BT_ADDR, \
    .bt_rf_txpwr_recon = XCFG_BT_RF_TXPWR_RECON, \
    .bt_a2dp_en = XCFG_BT_A2DP_EN, \
    .bt_a2dp_vol_ctrl_en = XCFG_BT_A2DP_VOL_CTRL_EN, \
    .bt_sco_en = XCFG_BT_SCO_EN, \
    .bt_hfp_private_en = XCFG_BT_HFP_PRIVATE_EN, \
    .bt_hfp_ring_number_en = XCFG_BT_HFP_RING_NUMBER_EN, \
    .bt_hfp_inband_ring_en = XCFG_BT_HFP_INBAND_RING_EN, \
    .bt_spp_en = XCFG_BT_SPP_EN, \
    .ble_en = XCFG_BLE_EN, \
    .le_name = XCFG_LE_NAME, \
    .bt_rf_pwrdec = XCFG_BT_RF_PWRDEC, \
    .ft_rf_param_en = XCFG_FT_RF_PARAM_EN, \
    .bt_rf_param_en = XCFG_BT_RF_PARAM_EN, \
    .rf_pa_gain = XCFG_RF_PA_GAIN, \
    .rf_mix_gain = XCFG_RF_MIX_GAIN, \
    .rf_dig_gain = XCFG_RF_DIG_GAIN, \
    .rf_pa_cap = XCFG_RF_PA_CAP, \
    .rf_mix_cap = XCFG_RF_MIX_CAP, \
    .rf_txdbm = XCFG_RF_TXDBM, \
    .bt_mmic_cfg = XCFG_BT_MMIC_CFG, \
    .bt_smic_cfg = XCFG_BT_SMIC_CFG, \
    .mic_post_gain = XCFG_MIC_POST_GAIN, \
    .mic0_en = XCFG_MIC0_EN, \
    .mic0_bias_method = XCFG_MIC0_BIAS_METHOD, \
    .mic0_pwr_sel = XCFG_MIC0_PWR_SEL, \
    .bt_mic0_dig_gain = XCFG_BT_MIC0_DIG_GAIN, \
    .mic1_en = XCFG_MIC1_EN, \
    .mic1_bias_method = XCFG_MIC1_BIAS_METHOD, \
    .mic1_pwr_sel = XCFG_MIC1_PWR_SEL, \
    .bt_mic1_dig_gain = XCFG_BT_MIC1_DIG_GAIN, \
    .bt_sco_nr_en = XCFG_BT_SCO_NR_EN, \
    .bt_sco_nr_level = XCFG_BT_SCO_NR_LEVEL, \
    .bt_aec_en = XCFG_BT_AEC_EN, \
    .bt_echo_level = XCFG_BT_ECHO_LEVEL, \
    .bt_aec_dig_gain = XCFG_BT_AEC_DIG_GAIN, \
    .bt_far_offset = XCFG_BT_FAR_OFFSET, \
    .bt_alc_en = XCFG_BT_ALC_EN, \
    .fmrx_r_val = XCFG_FMRX_R_VAL, \
    .fmrx_z_val = XCFG_FMRX_Z_VAL, \
    .fmrx_fz_val = XCFG_FMRX_FZ_VAL, \
    .fmrx_d_val = XCFG_FMRX_D_VAL, \
    .fmrx_cs_filter_fixed = XCFG_FMRX_CS_FILTER_FIXED, \
    .fmrx_cs_filter_sel = XCFG_FMRX_CS_FILTER_SEL, \
    .fmrx_audio_ch = XCFG_FMRX_AUDIO_CH, \
    .serial_2 = XCFG_SERIAL_2, \
    .serial_num = XCFG_SERIAL_NUM, \
    .osci_cap = XCFG_OSCI_CAP, \
    .osco_cap = XCFG_OSCO_CAP, \
    .soft_key = XCFG_SOFT_KEY, \
    .asr_soft_key = XCFG_ASR_SOFT_KEY, \
}

#endif
