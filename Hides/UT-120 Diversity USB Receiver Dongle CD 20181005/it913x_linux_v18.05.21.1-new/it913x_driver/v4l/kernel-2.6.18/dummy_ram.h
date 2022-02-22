#ifndef DUMMY_RAM_LIST
#define DUMMY_RAM_LIST


#define MERCURY_ROM_CODE

#define DUMMY_FW_VER0 4
#define DUMMY_FW_VER1 60
#define DUMMY_FW_VER2 0
#define DUMMY_FW_VER3 0



#define DUMMY_RAM_BASE 0x9BB9   //Attention: Reserved RAM   All are 8-bit for code size down.
#define DUMMY_RAM_LEN  71   	//d'64bytes  = 0x40 //(DUMMY_RAM_BASE + DUMMY_RAM_LEN - 1) is last dummy byte


#define    p_dummy_stes_last_case     0x9bb9
#define xd_p_dummy_stes_last_case	(*(volatile byte xdata *) 0x9bb9)
#define	dummy_stes_last_case_pos 0
#define	dummy_stes_last_case_len 4
#define	dummy_stes_last_case_lsb 0

#define    p_dummy_IF_max_8bits 	0x9bba				//INIT SCRIPT
#define xd_p_dummy_IF_max_8bits 	(*(volatile byte xdata *) 0x9bba)
#define	dummy_IF_max_8bits_pos 0
#define	dummy_IF_max_8bits_len 8
#define	dummy_IF_max_8bits_lsb 0

#define    p_dummy_ForceDropRfGainByGUI 	0x9bbb			//INIT SCRIPT
#define xd_p_dummy_ForceDropRfGainByGUI 	(*(volatile byte xdata *) 0x9bbb)
#define	dummy_ForceDropRfGainByGUI_pos 0
#define	dummy_ForceDropRfGainByGUI_len 1
#define	dummy_ForceDropRfGainByGUI_lsb 0

#define    p_dummy_antenna_unplugged     0x9bbc	
#define xd_p_dummy_antenna_unplugged 	(*(volatile byte xdata *) 0x9bbc)
#define	dummy_antenna_unplugged_pos 0
#define	dummy_antenna_unplugged_len 1
#define	dummy_antenna_unplugged_lsb 0

#define    p_dummy_ccid_bypass     0x9bbd
#define xd_p_dummy_ccid_bypass    (*(volatile byte xdata *) 0x9bbd)
#define	dummy_ccid_bypass_pos 0
#define	dummy_ccid_bypass_len 1
#define	dummy_ccid_bypass_lsb 0

#define    p_dummy_acid_bypass     0x9bbe
#define xd_p_dummy_acid_bypass    (*(volatile byte xdata *) 0x9bbe)
#define	dummy_acid_bypass_pos 0
#define	dummy_acid_bypass_len 1
#define	dummy_acid_bypass_lsb 0

#define    p_dummy_aci_0	 0x9bbf
#define xd_p_dummy_aci_0	(*(volatile byte xdata *) 0x9bbf)
#define	dummy_aci_0_pos 0
#define	dummy_aci_0_len 8
#define	dummy_aci_0_lsb 0

#define    p_dummy_aci_1	 0x9bc0
#define xd_p_dummy_aci_1	(*(volatile byte xdata *) 0x9bc0)
#define	dummy_aci_1_pos 0
#define	dummy_aci_1_len 8
#define	dummy_aci_1_lsb 0

#define    p_dummy_CallApShiftFreofTuner	 0x9bc1
#define xd_p_dummy_CallApShiftFreofTuner  (*(volatile byte xdata *) 0x9bc1)
#define	dummy_CallApShiftFreofTuner_pos 0
#define	dummy_CallApShiftFreofTuner_len 8
#define	dummy_CallApShiftFreofTuner_lsb 0

#define    p_dummy_antenna_unplug_thr 0x9bc2
#define xd_p_dummy_antenna_unplug_thr (*(volatile byte xdata *) 0x9bc2)
#define	dummy_antenna_unplug_thr_pos 0
#define	dummy_antenna_unplug_thr_len 1
#define	dummy_antenna_unplug_thr_lsb 0

#define    p_dummy_rf_min_bak_7_0     0x9bc3
#define xd_p_dummy_rf_min_bak_7_0 	(*(volatile byte xdata *) 0x9bc3)
#define	dummy_rf_min_bak_7_0_pos 0
#define	dummy_rf_min_bak_7_0_len 8
#define	dummy_rf_min_bak_7_0_lsb 0

#define    p_dummy_rf_min_bak_9_8     0x9bc4
#define xd_p_dummy_rf_min_bak_9_8 	(*(volatile byte xdata *) 0x9bc4)
#define	dummy_rf_min_bak_9_8_pos 0
#define	dummy_rf_min_bak_9_8_len 8
#define	dummy_rf_min_bak_9_8_lsb 0

#define    p_dummy_if_min_bak_7_0     0x9bc5
#define xd_p_dummy_if_min_bak_7_0 	(*(volatile byte xdata *) 0x9bc5)
#define	dummy_if_min_bak_7_0_pos 0
#define	dummy_if_min_bak_7_0_len 8
#define	dummy_if_min_bak_7_0_lsb 0

#define    p_dummy_if_min_bak_9_8     0x9bc6
#define xd_p_dummy_if_min_bak_9_8 	(*(volatile byte xdata *) 0x9bc6)
#define	dummy_if_min_bak_9_8_pos 0
#define	dummy_if_min_bak_9_8_len 8
#define	dummy_if_min_bak_9_8_lsb 0

#define    p_dummy_rf_min_8bits_bak 	0x9bc7
#define xd_p_dummy_rf_min_8bits_bak	(*(volatile byte xdata *) 0x9bc7)
#define	dummy_rf_min_8bits_bak_pos 0
#define	dummy_rf_min_8bits_bak_len 8
#define	dummy_rf_min_8bits_bak_lsb 0

#define    p_dummy_reg_weak_signal_rfagc_thr	 0x9bc8
#define xd_p_dummy_reg_weak_signal_rfagc_thr 	(*(volatile byte xdata *) 0x9bc8)
#define	dummy_reg_weak_signal_rfagc_thr_pos 0
#define	dummy_reg_weak_signal_rfagc_thr_len 8
#define	dummy_reg_weak_signal_rfagc_thr_lsb 0

#define    p_dummy_reg_unplug_th	 0x9bc9
#define xd_p_dummy_reg_unplug_th 	(*(volatile byte xdata *) 0x9bc9)
#define	dummy_reg_unplug_th_pos 0
#define	dummy_reg_unplug_th_len 8
#define	dummy_reg_unplug_th_lsb 0

#define    p_dummy_strong_signal_detected     0x9bca
#define xd_p_dummy_strong_signal_detected 	(*(volatile byte xdata *) 0x9bca)
#define	dummy_strong_signal_detected_pos 0
#define	dummy_strong_signal_detected_len 1
#define	dummy_strong_signal_detected_lsb 0

#define    p_dummy_reg_unplug_flag     0x9bcb
#define xd_p_dummy_reg_unplug_flag 	(*(volatile byte xdata *) 0x9bcb)
#define	dummy_reg_unplug_flag_pos 0
#define	dummy_reg_unplug_flag_len 1
#define	dummy_reg_unplug_flag_lsb 0

#define    p_dummy_reg_CFI_th     0x9bcc
#define xd_p_dummy_reg_CFI_th 	(*(volatile byte xdata *) 0x9bcc)
#define	dummy_reg_CFI_th_pos 0
#define	dummy_reg_CFI_th_len 8
#define	dummy_reg_CFI_th_lsb 0

#define    p_dummy_cvi_reg_15_8	 0x9bcd
#define xd_p_dummy_cvi_reg_15_8  (*(volatile byte xdata *) 0x9bcd)
#define dummy_cvi_reg_15_8_pos 0
#define dummy_cvi_reg_15_8_len 8
#define dummy_cvi_reg_15_8_lsb 8

#define    p_dummy_cvi_reg_7_0 	0x9bce
#define xd_p_dummy_cvi_reg_7_0  (*(volatile byte xdata *) 0x9bce)
#define dummy_cvi_reg_7_0_pos 0
#define dummy_cvi_reg_7_0_len 8
#define dummy_cvi_reg_7_0_lsb 0

#define    p_dummy_fot_coeff_tuner_type 	0x9bcf
#define xd_p_dummy_fot_coeff_tuner_type  (*(volatile byte xdata *) 0x9bcf)
#define dummy_fot_coeff_tuner_type_pos 0
#define dummy_fot_coeff_tuner_type_len 8
#define dummy_fot_coeff_tuner_type_lsb 0

#define    p_dummy_rsda_trigger_delay_cnt  0x9bd0
#define xd_p_dummy_rsda_trigger_delay_cnt    (*(volatile byte xdata *) 0x9bd0)
#define dummy_rsda_trigger_delay_cnt_pos 0
#define dummy_rsda_trigger_delay_cnt_len 8
#define dummy_rsda_trigger_delay_cnt_lsb 0

#define    p_dummy_channelFlatnessInd     0x9bd1
#define xd_p_dummy_channelFlatnessInd     (*(volatile byte xdata *) 0x9bd1)
#define dummy_channelFlatnessInd_pos 0
#define dummy_channelFlatnessInd_len 1
#define dummy_channelFlatnessInd_lsb 0

#define    p_dummy_fadc_type	 0x9bd2
#define xd_p_dummy_fadc_type		(*(volatile byte xdata *) 0x9bd2)
#define	dummy_fadc_type_pos 0
#define	dummy_fadc_type_len 4
#define	dummy_fadc_type_lsb 0

#define    p_dummy_reg_unplug_rf_th     0x9bd3	//new
#define xd_p_dummy_reg_unplug_rf_th		(*(volatile byte xdata *) 0x9bd3)
#define	dummy_reg_unplug_rf_th_pos 0
#define	dummy_reg_unplug_rf_th_len 8
#define	dummy_reg_unplug_rf_th_lsb 0

#define    p_dummy_ce_is_mobile 	0x9bd4
#define xd_p_dummy_ce_is_mobile		(*(volatile byte xdata *) 0x9bd4)
#define	dummy_ce_is_mobile_pos 0
#define	dummy_ce_is_mobile_len 1
#define	dummy_ce_is_mobile_lsb 0

#define    p_dummy_tuner_ver_0		0x9bd5	//GUI
#define xd_p_dummy_tuner_ver_0		(*(volatile byte xdata *) 0x9bd5)
#define	dummy_tuner_ver_0_pos 0
#define	dummy_tuner_ver_0_len 8
#define	dummy_tuner_ver_0_lsb 0

#define    p_dummy_tuner_ver_1		0x9bd6	//GUI
#define xd_p_dummy_tuner_ver_1		(*(volatile byte xdata *) 0x9bd6)
#define	dummy_tuner_ver_1_pos 0
#define	dummy_tuner_ver_1_len 8
#define	dummy_tuner_ver_1_lsb 0

#define    p_dummy_aagc_speed_detect_count  	0x9bd7
#define xd_p_dummy_aagc_speed_detect_count    (*(volatile byte xdata *) 0x9bd7)
#define dummy_aagc_speed_detect_count_pos 0
#define dummy_aagc_speed_detect_count_len 8
#define dummy_aagc_speed_detect_count_lsb 0

#define    p_dummy_aagc_mobile_thr  	0x9bd8
#define xd_p_dummy_aagc_mobile_thr    (*(volatile byte xdata *) 0x9bd8)
#define dummy_aagc_mobile_thr_pos 0
#define dummy_aagc_mobile_thr_len 8
#define dummy_aagc_mobile_thr_lsb 0

#define    p_dummy_aagc_nonmobile_thr      0x9bd9
#define xd_p_dummy_aagc_nonmobile_thr    (*(volatile byte xdata *) 0x9bd9)
#define dummy_aagc_nonmobile_thr_pos 0
#define dummy_aagc_nonmobile_thr_len 8
#define dummy_aagc_nonmobile_thr_lsb 0

#define    p_dummy_retrain_dtop_bypass 	   0x9bda			//GUI
#define xd_p_dummy_retrain_dtop_bypass    (*(volatile byte xdata *) 0x9bda)
#define dummy_retrain_dtop_bypass_pos 0
#define dummy_retrain_dtop_bypass_len 8
#define dummy_retrain_dtop_bypass_lsb 0

#define    p_dummy_strong_detect_bypass    0x9bdb
#define xd_p_dummy_strong_detect_bypass    (*(volatile byte xdata *) 0x9bdb)
#define dummy_strong_detect_bypass_pos 0
#define dummy_strong_detect_bypass_len 8
#define dummy_strong_detect_bypass_lsb 0

//#define    p_dummy_aaagc_bypass    (DUMMY_RAM_BASE+35)			//DEBUG
//#define xd_p_dummy_aaagc_bypass    (*(volatile byte xdata *) (DUMMY_RAM_BASE+35))
//#define dummy_aaagc_bypass_pos 0
//#define dummy_aaagc_bypass_len 8
//#define dummy_aaagc_bypass_lsb 0

#define    p_dummy_ofsm_state  0x9bdd			//DEBUG
#define xd_p_dummy_ofsm_state    (*(volatile byte xdata *) 0x9bdd)
#define dummy_ofsm_state_pos 0
#define dummy_ofsm_state_len 8
#define dummy_ofsm_state_lsb 0

#define    p_dummy_rsda_trigger_en  0x9bde			//DEBUG
#define xd_p_dummy_rsda_trigger_en    (*(volatile byte xdata *) 0x9bde)
#define dummy_rsda_trigger_en_pos 0
#define dummy_rsda_trigger_en_len 1
#define dummy_rsda_trigger_en_lsb 0

#define    p_dummy_ce_mobile_count_max  0x9bdf		//INIT
#define xd_p_dummy_ce_mobile_count_max    (*(volatile byte xdata *) 0x9bdf)
#define dummy_ce_mobile_count_max_pos 0
#define dummy_ce_mobile_count_max_len 8
#define dummy_ce_mobile_count_max_lsb 0

#define    p_dummy_ce_mobile_count_th  0x9be0		//INIT
#define xd_p_dummy_ce_mobile_count_th    (*(volatile byte xdata *) 0x9be0)
#define dummy_ce_mobile_count_th_pos 0
#define dummy_ce_mobile_count_th_len 8
#define dummy_ce_mobile_count_th_lsb 0

#define    p_dummy_timing_retrain_cnt			0x9be1
#define xd_p_dummy_timing_retrain_cnt			(*(volatile byte xdata *) 0x9be1)
#define	dummy_timing_retrain_cnt_pos 0
#define	dummy_timing_retrain_cnt_len 8
#define	dummy_timing_retrain_cnt_lsb 0

#define    p_dummy_err_timing_detect_en     0x9be2
#define xd_p_dummy_err_timing_detect_en	(*(volatile byte xdata *) 0x9be2)
#define	dummy_err_timing_detect_en_pos 0
#define	dummy_err_timing_detect_en_len 8
#define	dummy_err_timing_detect_en_lsb 0

#define    p_dummy_ss_dtop_bypass     0x9be3	//GUI
#define xd_p_dummy_ss_dtop_bypass	(*(volatile byte xdata *) 0x9be3)
#define	dummy_ss_dtop_bypass_pos 0
#define	dummy_ss_dtop_bypass_len 8
#define	dummy_ss_dtop_bypass_lsb 0

#define    p_dummy_stes_quit_case     0x9be4	//DEBUG
#define xd_p_dummy_stes_quit_case	(*(volatile byte xdata *) 0x9be4)
#define	dummy_stes_quit_case_pos 0
#define	dummy_stes_quit_case_len 4
#define	dummy_stes_quit_case_lsb 0

#define    p_dummy_rom_ver_1		0x9be5	//GUI
#define xd_p_dummy_rom_ver_1		(*(volatile byte xdata *) 0x9be5)
#define	dummy_rom_ver_1_pos 0
#define	dummy_rom_ver_1_len 8
#define	dummy_rom_ver_1_lsb 0

#define    p_dummy_spec_inv_detect_en		0x9be6	//GUI
#define xd_p_dummy_spec_inv_detect_en		(*(volatile byte xdata *) 0x9be6)
#define	dummy_spec_inv_detect_en_pos 0
#define	dummy_spec_inv_detect_en_len 8
#define	dummy_spec_inv_detect_en_lsb 0

#define    p_dummy_bfs_fcw_def_7_0		0x9be7		//GUI
#define xd_p_dummy_bfs_fcw_def_7_0		(*(volatile byte xdata *) 0x9be7)
#define	dummy_bfs_fcw_def_7_0_pos 0
#define	dummy_bfs_fcw_def_7_0_len 8
#define	dummy_bfs_fcw_def_7_0_lsb 0


#define    p_dummy_bfs_fcw_def_15_8		0x9be8		//GUI
#define xd_p_dummy_bfs_fcw_def_15_8		(*(volatile byte xdata *) 0x9be8)
#define	dummy_bfs_fcw_def_15_8_pos 0
#define	dummy_bfs_fcw_def_15_8_len 8
#define	dummy_bfs_fcw_def_15_8_lsb 0


#define    p_dummy_bfs_fcw_def_22_16		0x9be9		//GUI
#define xd_p_dummy_bfs_fcw_def_22_16		(*(volatile byte xdata *) 0x9be9)
#define	dummy_bfs_fcw_def_22_16_pos 0
#define	dummy_bfs_fcw_def_22_16_len 7
#define	dummy_bfs_fcw_def_22_16_lsb 0

#define    p_dummy_bfs_fcw_inv_7_0		0x9bea		//GUI
#define xd_p_dummy_bfs_fcw_inv_7_0		(*(volatile byte xdata *) 0x9bea)
#define	dummy_bfs_fcw_inv_7_0_pos 0
#define	dummy_bfs_fcw_inv_7_0_len 8
#define	dummy_bfs_fcw_inv_7_0_lsb 0

#define    p_dummy_bfs_fcw_inv_15_8		0x9beb		//GUI
#define xd_p_dummy_bfs_fcw_inv_15_8		(*(volatile byte xdata *) 0x9beb)
#define	dummy_bfs_fcw_inv_15_8_pos 0
#define	dummy_bfs_fcw_inv_15_8_len 8
#define	dummy_bfs_fcw_inv_15_8_lsb 0

#define    p_dummy_bfs_fcw_inv_22_16		0x9bec		//GUI
#define xd_p_dummy_bfs_fcw_inv_22_16		(*(volatile byte xdata *) 0x9bec)
#define	dummy_bfs_fcw_inv_22_16_pos 0
#define	dummy_bfs_fcw_inv_22_16_len 7
#define	dummy_bfs_fcw_inv_22_16_lsb 0

#define    p_dummy_spec_inv_detected		0x9bed	//GUI
#define xd_p_dummy_spec_inv_detected		(*(volatile byte xdata *) 0x9bed)
#define	dummy_spec_inv_detected_pos 0
#define	dummy_spec_inv_detected_len 1
#define	dummy_spec_inv_detected_lsb 0

//#define    p_dummy_adc_attenu_en		(DUMMY_RAM_BASE+53)	
//#define xd_p_dummy_adc_attenu_en		(*(volatile byte xdata *) (DUMMY_RAM_BASE+53))
//#define	dummy_adc_attenu_en_pos 0
//#define	dummy_adc_attenu_en_len 1
//#define	dummy_adc_attenu_en_lsb 0

#define    p_dummy_adc_attenu_on_if_thr		0x9bef	
#define xd_p_dummy_adc_attenu_on_if_thr		(*(volatile byte xdata *) 0x9bef)
#define	dummy_adc_attenu_on_if_thr_pos 0
#define	dummy_adc_attenu_on_if_thr_len 8
#define	dummy_adc_attenu_on_if_thr_lsb 0

#define    p_dummy_adc_attenu_off_if_thr		0x9bf0	
#define xd_p_dummy_adc_attenu_off_if_thr		(*(volatile byte xdata *) 0x9bf0)
#define	dummy_adc_attenu_off_if_thr_pos 0
#define	dummy_adc_attenu_off_if_thr_len 8
#define	dummy_adc_attenu_off_if_thr_lsb 0

#define    p_dummy_rom_ver_0		0x9bf1	//GUI
#define xd_p_dummy_rom_ver_0		(*(volatile byte xdata *) 0x9bf1)
#define	dummy_rom_ver_0_pos 0
#define	dummy_rom_ver_0_len 8
#define	dummy_rom_ver_0_lsb 0

#define    p_dummy_api_ver_0		0x9bf2	//GUI
#define xd_p_dummy_api_ver_0		(*(volatile byte xdata *) 0x9bf2)
#define	dummy_api_ver_0_pos 0
#define	dummy_api_ver_0_len 8
#define	dummy_api_ver_0_lsb 0

#define    p_dummy_api_ver_1		0x9bf3	//GUI
#define xd_p_dummy_api_ver_1		(*(volatile byte xdata *) 0x9bf3)
#define	dummy_api_ver_1_pos 0
#define	dummy_api_ver_1_len 8
#define	dummy_api_ver_1_lsb 0

#define    p_dummy_api_ver_2		0x9bf4	//GUI
#define xd_p_dummy_api_ver_2		(*(volatile byte xdata *) 0x9bf4)
#define	dummy_api_ver_2_pos 0
#define	dummy_api_ver_2_len 8
#define	dummy_api_ver_2_lsb 0

#define    p_dummy_api_ver_3		0x9bf5	//GUI
#define xd_p_dummy_api_ver_3		(*(volatile byte xdata *) 0x9bf5)
#define	dummy_api_ver_3_pos 0
#define	dummy_api_ver_3_len 8
#define	dummy_api_ver_3_lsb 0

#define    p_dummy_qef_drop_num		0x9bf6
#define xd_p_dummy_qef_drop_num			(*(volatile byte xdata *) 0x9bf6)
#define	dummy_qef_drop_num_pos 0
#define	dummy_qef_drop_num_len 8
#define	dummy_qef_drop_num_lsb 0

#define    p_dummy_terr_drop_num		0x9bf7
#define xd_p_dummy_terr_drop_num			(*(volatile byte xdata *) 0x9bf7)
#define	dummy_terr_drop_num_pos 0
#define	dummy_terr_drop_num_len 8
#define	dummy_terr_drop_num_lsb 0

#define    p_dummy_qef_drop_value		0x9bf8
#define xd_p_dummy_qef_drop_value			(*(volatile byte xdata *) 0x9bf8)
#define	dummy_qef_drop_value_pos 0
#define	dummy_qef_drop_value_len 8
#define	dummy_qef_drop_value_lsb 0

#define    p_dummy_terr_drop_value		0x9bf9
#define xd_p_dummy_terr_drop_value			(*(volatile byte xdata *) 0x9bf9)
#define	dummy_terr_drop_value_pos 0
#define	dummy_terr_drop_value_len 8
#define	dummy_terr_drop_value_lsb 0

#define    p_dummy_ber_times		0x9bfa
#define xd_p_dummy_ber_times			(*(volatile byte xdata *) 0x9bfa)
#define	dummy_ber_times_pos 0
#define	dummy_ber_times_len 8
#define	dummy_ber_times_lsb 0

#define    p_dummy_good_performance_th		0x9bfb
#define xd_p_dummy_good_performance_th			(*(volatile byte xdata *) 0x9bfb)
#define	dummy_good_performance_th_pos 0
#define	dummy_good_performance_th_len 8
#define	dummy_good_performance_th_lsb 0

#define    p_dummy_one_shot_drop		0x9bfc
#define xd_p_dummy_one_shot_drop			(*(volatile byte xdata *) 0x9bfc)
#define	dummy_one_shot_drop_pos 0
#define	dummy_one_shot_drop_len 8
#define	dummy_one_shot_drop_lsb 0

#define    p_dummy_tps_trial_times		0x9bfd
#define xd_p_dummy_tps_trial_times			(*(volatile byte xdata *) 0x9bfd)
#define	dummy_tps_trial_times_pos 0
#define	dummy_tps_trial_times_len 8
#define	dummy_tps_trial_times_lsb 0

#define    p_dummy_was_steady_trial_times		0x9bfe
#define xd_p_dummy_was_steady_trial_times			(*(volatile byte xdata *) 0x9bfe)
#define	dummy_was_steady_trial_times_pos 0
#define	dummy_was_steady_trial_times_len 8
#define	dummy_was_steady_trial_times_lsb 0
	
#define    p_dummy_timing_err_level			0x9bff
#define xd_p_dummy_timing_err_level			(*(volatile byte xdata *) 0x9bff)
#define	dummy_timing_err_level_pos 0
#define	dummy_timing_err_level_len 4
#define	dummy_timing_err_level_lsb 0


#endif
