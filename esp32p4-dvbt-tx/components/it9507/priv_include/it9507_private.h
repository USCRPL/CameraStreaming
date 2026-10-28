/*
 * it9507_private.h - internal definitions shared between it9507_usb.c and
 *                    it9507_core.c. Not part of the public API.
 */
#pragma once

#include "it9507.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "usb/usb_host.h"

/* On-the-wire command framing (little-endian host; command word is stored
 * big-endian so the struct can be memcpy'd straight to the bulk endpoint). */
typedef struct __attribute__((packed)) {
    uint8_t  length;        /* number of bytes that FOLLOW this byte         */
    uint16_t command;       /* big-endian: PROC_* | CMD_*                    */
    uint8_t  seq;
    uint8_t  payload[251];  /* last two bytes become the checksum            */
} it9507_req_t;

typedef struct __attribute__((packed)) {
    uint8_t  length;
    uint8_t  seq;
    uint8_t  status;
    uint8_t  payload[251];
} it9507_reply_t;

#define IT9507_CONNECTED_BIT  (1 << 0)
#define IT9507_FAILED_BIT     (1 << 1)

struct it9507_dev_s {
    /* USB host plumbing */
    usb_host_client_handle_t client;
    usb_device_handle_t      dev_hdl;
    uint8_t                  itf_num;
    uint16_t                 ts_mps;        /* EP6 max packet size           */
    bool                     own_host;      /* did we call usb_host_install? */
    TaskHandle_t             evt_task;
    volatile bool            evt_run;
    EventGroupHandle_t       evt_group;

    /* Command channel (serialised) */
    SemaphoreHandle_t        cmd_mtx;
    usb_transfer_t          *cmd_out;
    usb_transfer_t          *cmd_in;
    SemaphoreHandle_t        cmd_out_done;
    SemaphoreHandle_t        cmd_in_done;
    int                      cmd_out_status, cmd_out_actual;
    int                      cmd_in_status,  cmd_in_actual;
    uint8_t                  seq;

    /* TS streaming pool (EP6 bulk OUT) */
    size_t                   ts_urb_size;
    size_t                   ts_urb_count;
    QueueHandle_t            ts_free;       /* queue of free usb_transfer_t* */
    usb_transfer_t         **ts_urbs;

    int                      last_error;    /* sticky, like upstream dev->error */
    it9507_modulation_t      mod;

    /* Output-gain state: the IQ-cal coefficients act as the unity-gain default;
     * adjust_output_gain scales them to change RF level (see it9507_core.c). */
    int                      iq_c1, iq_c2, iq_c3;
    int                      output_gain;   /* last applied gain, dB */
};

/* ---- it9507_usb.c ------------------------------------------------------- */
esp_err_t it9507_usb_start(it9507_handle_t h, const it9507_config_t *cfg);
void      it9507_usb_stop(it9507_handle_t h);
/* Synchronous command exchange on EP 0x02 / 0x81. rep may be NULL. */
int       it9507_usb_cmd_xfer(it9507_handle_t h, it9507_req_t *req,
                              size_t req_len, it9507_reply_t *rep);
/* Submit one TS chunk on EP 0x06, blocking for a free slot. */
esp_err_t it9507_usb_ts_submit(it9507_handle_t h, const uint8_t *buf, size_t len,
                               TickType_t wait);

/* ---- it9507_core.c (register / tuner layer) ---------------------------- */
int it9507_wr_regs(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   const uint8_t *vals, uint8_t num);
int it9507_rd_regs(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   uint8_t *vals, uint8_t num);
int it9507_wr_reg (it9507_handle_t h, uint16_t cpu, uint32_t reg, uint8_t val);
int it9507_wr_bits(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   uint8_t pos, uint8_t len, uint8_t val);

int it9507_load_firmware(it9507_handle_t h);
int it9507_power_up(it9507_handle_t h);
int it9507_enable_xmit(it9507_handle_t h);
int it9507_disable_xmit(it9507_handle_t h);
int it9507_apply_modulation(it9507_handle_t h);
int it9507_set_ts_interface(it9507_handle_t h);
