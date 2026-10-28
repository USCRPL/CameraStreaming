/*
 * it9507.h - ESP-IDF (ESP32-P4) USB-host driver for the ITE IT9507 DVB-T
 *            modulator (UT-100C and compatible sticks, USB 048d:9507).
 *
 * The ESP32-P4 acts as USB *host*; the IT9507 stick is the USB *device*.
 * The application feeds a constant-bit-rate MPEG2-TS into it9507_write_ts();
 * the modulator turns it into a DVB-T RF signal.
 *
 * Derived from GPL-2.0 upstream drivers (see it9507_regs.h). This port is
 * therefore GPL-2.0 as well.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "it9507_regs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct it9507_dev_s *it9507_handle_t;

/* DVB-T modulation parameters. NOTE: bandwidth is in kHz and must be one of
 * the supported values {1000,1500,2000,2500,3000,4000,5000,6000,7000,8000}.
 * Narrow bandwidths are how you match a low H.264 bit-rate. */
typedef struct {
    uint32_t frequency_khz;     /* RF centre frequency, e.g. 473000          */
    uint32_t bandwidth_khz;     /* channel bandwidth in kHz (see note above) */
    int      gain_db;           /* software attenuation, negative = quieter  */
    uint8_t  constellation;     /* IT9507_QPSK / QAM16 / QAM64               */
    uint8_t  code_rate;         /* IT9507_FEC_*                              */
    uint8_t  guard_interval;    /* IT9507_GI_*                               */
    uint8_t  transmission_mode; /* IT9507_TX_MODE_*                          */
    uint16_t cell_id;           /* DVB-T cell id (0 is fine)                 */
} it9507_modulation_t;

typedef struct {
    bool        install_usb_host;  /* true: driver calls usb_host_install().
                                    * false: app already installed the host. */
    size_t      ts_urb_size;       /* bytes per TS bulk transfer (mult of 188).
                                    * 0 -> IT9507_USB_FRAME_SIZE (65424).     */
    size_t      ts_urb_count;      /* in-flight TS transfers, 0 -> 4.         */
    UBaseType_t task_priority;     /* USB event task priority, 0 -> 5.        */
    int         task_core_id;      /* core for the event task, -1 -> no affin */
    uint32_t    connect_timeout_ms;/* wait for the stick, 0 -> 5000.          */
} it9507_config_t;

#define IT9507_CONFIG_DEFAULT() (it9507_config_t){     \
    .install_usb_host    = true,                       \
    .ts_urb_size         = IT9507_USB_FRAME_SIZE,      \
    .ts_urb_count        = 4,                          \
    .task_priority       = 5,                          \
    .task_core_id        = -1,                         \
    .connect_timeout_ms  = 5000,                       \
}

/* Bring up the USB host, wait for the modulator, upload firmware, power it up.
 * On success *out is a ready (but not yet transmitting) handle. */
esp_err_t it9507_install(const it9507_config_t *cfg, it9507_handle_t *out);

/* Program the carrier and DVB-T parameters. May be called before start or
 * while stopped. Re-tuning while transmitting is allowed but will glitch. */
esp_err_t it9507_set_modulation(it9507_handle_t h, const it9507_modulation_t *mod);

/* Enable / disable the RF carrier (afe + PLL + GPIO power). */
esp_err_t it9507_start(it9507_handle_t h);
esp_err_t it9507_stop(it9507_handle_t h);

/* Push CBR transport-stream bytes. len SHOULD be a multiple of 188.
 * Blocks up to ticks_to_wait for a free transfer slot; returns ESP_ERR_TIMEOUT
 * if the modulator cannot keep up (your TS bit-rate exceeds the channel). */
esp_err_t it9507_write_ts(it9507_handle_t h, const uint8_t *ts, size_t len,
                          TickType_t ticks_to_wait);

/* Clear the modulator's tx_fifo_overflow latch and reconnect the FIFO->FEC path
 * after a stall (writes timing out). Call when it9507_write_ts() has failed
 * repeatedly; the TS stream resumes after a brief glitch. */
int it9507_ts_recover(it9507_handle_t h);

/* Read the link / OFDM firmware versions (handy as a "is it alive" probe). */
esp_err_t it9507_get_fw_versions(it9507_handle_t h, uint32_t *link, uint32_t *ofdm);

/* Set RF output level. gain_db is relative attenuation (0 = chip default level,
 * negative = quieter). Applied automatically by it9507_set_modulation() from
 * it9507_modulation_t.gain_db. See the implementation note about porting the
 * upstream amplitude registers before non-zero values take effect. */
esp_err_t it9507_set_output_gain(it9507_handle_t h, int gain_db);

/* Read back FIFO-overflow flag, TX-enable, and the programmed modulation config
 * and log them. Use to diagnose a modulator that accepts TS then stops draining. */
esp_err_t it9507_dump_status(it9507_handle_t h);

/* Stop, release the interface, optionally uninstall the host. */
esp_err_t it9507_uninstall(it9507_handle_t h);

#ifdef __cplusplus
}
#endif
