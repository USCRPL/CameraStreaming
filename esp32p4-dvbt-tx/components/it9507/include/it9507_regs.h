/*
 * it9507_regs.h - Register / command map for the ITE IT9507 DVB-T modulator.
 *
 * Register names and values are taken from the GPL-2.0 works:
 *   - ITE "it950x_linux" SDK (v13.06.27.1 / v13.09.13.1)
 *   - Dave Chapman's cleaned-up driver   (github.com/linuxstb/it9507)
 *   - Timo Teras' libusb port            (github.com/fabled/sendts-it9507)
 *
 * This file only contains hardware constants (no executable logic) so that the
 * .c files can stay readable. Keep the GPL-2.0 notice with any redistribution.
 */
#pragma once

/* ---- USB identity (UT-100C / generic IT9507 stick) ---------------------- */
#define IT9507_USB_VID                 0x048D
#define IT9507_USB_PID                 0x9507

/* Bulk endpoints (host point of view) */
#define IT9507_EP_CMD_OUT              0x02   /* command requests          */
#define IT9507_EP_CMD_IN               0x81   /* command replies           */
#define IT9507_EP_TS_OUT               0x06   /* MPEG2-TS payload          */

/* ---- Firmware versions the upstream driver expects ---------------------- */
#define IT9507_DVB_OFDM_VERSION        0xFF090800U
#define IT9507_DVB_LINK_VERSION        0xFF270200U

/* ---- Command opcodes ---------------------------------------------------- */
#define IT9507_CMD_REG_DEMOD_READ      0x0000
#define IT9507_CMD_REG_DEMOD_WRITE     0x0001
#define IT9507_CMD_REG_EEPROM_READ     0x0004
#define IT9507_CMD_REG_EEPROM_WRITE    0x0005
#define IT9507_CMD_IR_GET              0x0018
#define IT9507_CMD_QUERYINFO           0x0022
#define IT9507_CMD_BOOT                0x0023
#define IT9507_CMD_REBOOT              0x0023
#define IT9507_CMD_SCATTER_WRITE       0x0029
#define IT9507_CMD_GENERIC_READ        0x002A
#define IT9507_CMD_GENERIC_WRITE       0x002B

/* The high bit of the command word selects the target processor. */
#define IT9507_PROC_LINK               0x0000   /* "link" MCU            */
#define IT9507_PROC_OFDM               0x8000   /* OFDM / modulator core */

/* ---- GPIO register helpers --------------------------------------------- */
#define IT9507_GPIO_I(x)               ((x))
#define IT9507_GPIO_O(x)               ((x) + 1)
#define IT9507_GPIO_EN(x)              ((x) + 2)
#define IT9507_GPIO_ON(x)              ((x) + 3)
#define IT9507_GPIOH2                  0xD8B6   /* RF power enable       */
#define IT9507_GPIOH8                  0xD8CE   /* UV filter select      */

/* I2C master speed: 0x07 -> 366 kHz */
#define IT9507_IIC_SPEED               0x07

/* ---- OFDM / link registers used during bring-up ------------------------ */
#define p_reg_lnk2ofdm_data_63_56      0xF6A7
#define p_reg_fec_sw_rst               0xF702
#define p_reg_tps_gi                   0xF725
#define p_reg_iqik_c1_7_0              0xF752
#define p_reg_iqik_c2_7_0              0xF754
#define p_reg_iqik_c3_7_0              0xF756

#define p_mp2if_mpeg_ser_mode          0xF985
#define p_mp2if_mpeg_par_mode          0xF986
#define p_reg_mpeg_full_speed          0xF990
#define p_reg_afe_mem0                 0xFB24
#define p_reg_afe_mem1                 0xFB25
#define p_reg_dyn0_clk                 0xFBA8
#define reg_tx_fifo_overflow           0xF9E7

#define p_reg_top_hostb_mpeg_par_mode  0xD91B
#define p_reg_top_hostb_mpeg_ser_mode  0xD91C

#define p_reg_dvbt_en                  0xF41A
#define p_reg_dvbt_inten               0xF41F
#define reg_dvbt_inten_pos             2

/* USB endpoint configuration registers (link MCU) */
#define p_reg_ep6_max_pkt              0xDD0E
#define p_reg_ep6_rx_en                0xDD11   /* bit 7 */
#define reg_ep6_rx_en_pos              7
#define p_reg_ep6_cnt_num_7_0          0xDDA9
#define p_reg_tx_hw_disable            0xDDAB

/* ---- USB frame sizing (must stay a multiple of 188) -------------------- */
#define IT9507_USB_MAX_PACKET          512
#define IT9507_USB_FRAME_SIZE          (188 * 348)          /* 65424 bytes */
#define IT9507_USB_FRAME_SIZE_DW       (IT9507_USB_FRAME_SIZE / 4)

/* ---- Public enum values (mirror DVB-T) --------------------------------- */
#define IT9507_TX_MODE_2K              0
#define IT9507_TX_MODE_8K              1
#define IT9507_TX_MODE_4K              2

#define IT9507_QPSK                    0
#define IT9507_QAM16                   1
#define IT9507_QAM64                   2

#define IT9507_GI_1_32                 0
#define IT9507_GI_1_16                 1
#define IT9507_GI_1_8                  2
#define IT9507_GI_1_4                  3

#define IT9507_FEC_1_2                 0
#define IT9507_FEC_2_3                 1
#define IT9507_FEC_3_4                 2
#define IT9507_FEC_5_6                 3
#define IT9507_FEC_7_8                 4
