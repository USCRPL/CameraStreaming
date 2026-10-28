/*
 * it9507_usb.c - USB-host transport for the IT9507 on ESP32-P4.
 *
 * Wraps ESP-IDF's asynchronous USB Host Library into the blocking
 * "send command / read reply" and "stream TS" model the modulator needs.
 *
 * GPL-2.0 (see it9507_regs.h for upstream attribution).
 */
#include <string.h>
#include <stdlib.h>
#include "it9507_private.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

static const char *TAG = "it9507_usb";

/* -------- transfer completion callbacks --------------------------------- */
static void cmd_out_cb(usb_transfer_t *t)
{
    it9507_handle_t h = (it9507_handle_t)t->context;
    h->cmd_out_status = t->status;
    h->cmd_out_actual = t->actual_num_bytes;
    xSemaphoreGive(h->cmd_out_done);
}

static void cmd_in_cb(usb_transfer_t *t)
{
    it9507_handle_t h = (it9507_handle_t)t->context;
    h->cmd_in_status = t->status;
    h->cmd_in_actual = t->actual_num_bytes;
    xSemaphoreGive(h->cmd_in_done);
}

static void ts_cb(usb_transfer_t *t)
{
    it9507_handle_t h = (it9507_handle_t)t->context;
    if (t->status != USB_TRANSFER_STATUS_COMPLETED) {
        ESP_LOGW(TAG, "TS xfer status=%d (actual=%d/%d)",
                 t->status, t->actual_num_bytes, t->num_bytes);
    }
    /* Return the transfer object to the free pool. */
    xQueueSend(h->ts_free, &t, 0);
}

/* -------- device matching / enumeration --------------------------------- */
static esp_err_t open_and_claim(it9507_handle_t h, uint8_t dev_addr)
{
    if (usb_host_device_open(h->client, dev_addr, &h->dev_hdl) != ESP_OK)
        return ESP_FAIL;

    const usb_device_desc_t *dd;
    if (usb_host_get_device_descriptor(h->dev_hdl, &dd) != ESP_OK)
        goto fail;
    if (dd->idVendor != IT9507_USB_VID || dd->idProduct != IT9507_USB_PID) {
        ESP_LOGD(TAG, "ignoring %04x:%04x", dd->idVendor, dd->idProduct);
        goto fail;
    }
    ESP_LOGI(TAG, "found IT9507 %04x:%04x @ addr %u",
             dd->idVendor, dd->idProduct, dev_addr);

    /* Walk the active config to find interface 0 and EP6's max packet size. */
    const usb_config_desc_t *cd;
    if (usb_host_get_active_config_descriptor(h->dev_hdl, &cd) != ESP_OK)
        goto fail;

    h->itf_num = 0;
    h->ts_mps  = IT9507_USB_MAX_PACKET;     /* default for HS bulk */
    int off = 0;
    const usb_standard_desc_t *d =
        (const usb_standard_desc_t *)usb_parse_next_descriptor_of_type(
            (const usb_standard_desc_t *)cd, cd->wTotalLength,
            USB_B_DESCRIPTOR_TYPE_ENDPOINT, &off);
    while (d) {
        const usb_ep_desc_t *ep = (const usb_ep_desc_t *)d;
        if (ep->bEndpointAddress == IT9507_EP_TS_OUT)
            h->ts_mps = USB_EP_DESC_GET_MPS(ep);
        d = (const usb_standard_desc_t *)usb_parse_next_descriptor_of_type(
                d, cd->wTotalLength, USB_B_DESCRIPTOR_TYPE_ENDPOINT, &off);
    }

    if (usb_host_interface_claim(h->client, h->dev_hdl, h->itf_num, 0) != ESP_OK) {
        ESP_LOGE(TAG, "interface_claim failed");
        goto fail;
    }
    ESP_LOGI(TAG, "claimed interface %u, TS EP mps=%u", h->itf_num, h->ts_mps);
    return ESP_OK;

fail:
    usb_host_device_close(h->client, h->dev_hdl);
    h->dev_hdl = NULL;
    return ESP_FAIL;
}

static void client_cb(const usb_host_client_event_msg_t *msg, void *arg)
{
    it9507_handle_t h = (it9507_handle_t)arg;
    switch (msg->event) {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        if (!h->dev_hdl) {
            if (open_and_claim(h, msg->new_dev.address) == ESP_OK)
                xEventGroupSetBits(h->evt_group, IT9507_CONNECTED_BIT);
        }
        break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        ESP_LOGW(TAG, "device disconnected");
        h->last_error = ESP_ERR_INVALID_STATE;
        xEventGroupSetBits(h->evt_group, IT9507_FAILED_BIT);
        break;
    default:
        break;
    }
}

/* -------- USB event pump task ------------------------------------------- */
static void evt_task(void *arg)
{
    it9507_handle_t h = (it9507_handle_t)arg;
    while (h->evt_run) {
        uint32_t flags;
        if (h->own_host)
            usb_host_lib_handle_events(pdMS_TO_TICKS(10), &flags);
        usb_host_client_handle_events(h->client, pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

/* -------- public-to-component USB helpers ------------------------------- */
esp_err_t it9507_usb_start(it9507_handle_t h, const it9507_config_t *cfg)
{
    h->own_host  = cfg->install_usb_host;
    h->evt_group = xEventGroupCreate();
    h->cmd_mtx   = xSemaphoreCreateMutex();
    h->cmd_out_done = xSemaphoreCreateBinary();
    h->cmd_in_done  = xSemaphoreCreateBinary();
    if (!h->evt_group || !h->cmd_mtx || !h->cmd_out_done || !h->cmd_in_done)
        return ESP_ERR_NO_MEM;

    if (h->own_host) {
        const usb_host_config_t hc = {
            .skip_phy_setup = false,
            .intr_flags     = ESP_INTR_FLAG_LEVEL1,
        };
        ESP_RETURN_ON_ERROR(usb_host_install(&hc), TAG, "usb_host_install");
    }

    const usb_host_client_config_t cc = {
        .is_synchronous  = false,
        .max_num_event_msg = 8,
        .async = { .client_event_callback = client_cb, .callback_arg = h },
    };
    ESP_RETURN_ON_ERROR(usb_host_client_register(&cc, &h->client),
                        TAG, "client_register");

    /* Pre-allocate the command transfers. */
    ESP_RETURN_ON_ERROR(usb_host_transfer_alloc(320, 0, &h->cmd_out),
                        TAG, "alloc cmd_out");
    ESP_RETURN_ON_ERROR(usb_host_transfer_alloc(IT9507_USB_MAX_PACKET, 0, &h->cmd_in),
                        TAG, "alloc cmd_in");
    h->cmd_out->device_handle    = NULL;   /* filled after connect */
    h->cmd_out->bEndpointAddress = IT9507_EP_CMD_OUT;
    h->cmd_out->callback         = cmd_out_cb;
    h->cmd_out->context          = h;
    h->cmd_in->bEndpointAddress  = IT9507_EP_CMD_IN;
    h->cmd_in->callback          = cmd_in_cb;
    h->cmd_in->context           = h;

    /* TS pool. */
    h->ts_urb_size  = cfg->ts_urb_size  ? cfg->ts_urb_size  : IT9507_USB_FRAME_SIZE;
    h->ts_urb_count = cfg->ts_urb_count ? cfg->ts_urb_count : 4;
    h->ts_free = xQueueCreate(h->ts_urb_count, sizeof(usb_transfer_t *));
    h->ts_urbs = calloc(h->ts_urb_count, sizeof(usb_transfer_t *));
    if (!h->ts_free || !h->ts_urbs) return ESP_ERR_NO_MEM;
    for (size_t i = 0; i < h->ts_urb_count; i++) {
        ESP_RETURN_ON_ERROR(usb_host_transfer_alloc(h->ts_urb_size, 0, &h->ts_urbs[i]),
                            TAG, "alloc ts urb");
        h->ts_urbs[i]->bEndpointAddress = IT9507_EP_TS_OUT;
        h->ts_urbs[i]->callback         = ts_cb;
        h->ts_urbs[i]->context          = h;
        xQueueSend(h->ts_free, &h->ts_urbs[i], 0);
    }

    h->evt_run = true;
    BaseType_t ok = (cfg->task_core_id < 0)
        ? xTaskCreate(evt_task, "it9507_usb", 4096, h, cfg->task_priority, &h->evt_task)
        : xTaskCreatePinnedToCore(evt_task, "it9507_usb", 4096, h,
                                  cfg->task_priority, &h->evt_task, cfg->task_core_id);
    if (ok != pdPASS) return ESP_ERR_NO_MEM;

    /* Wait for enumeration. */
    uint32_t to = cfg->connect_timeout_ms ? cfg->connect_timeout_ms : 5000;
    EventBits_t b = xEventGroupWaitBits(h->evt_group,
                        IT9507_CONNECTED_BIT | IT9507_FAILED_BIT,
                        pdFALSE, pdFALSE, pdMS_TO_TICKS(to));
    if (!(b & IT9507_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "no IT9507 enumerated within %u ms", to);
        return ESP_ERR_TIMEOUT;
    }
    /* Bind device handle into the pre-allocated transfers. */
    h->cmd_out->device_handle = h->dev_hdl;
    h->cmd_in->device_handle  = h->dev_hdl;
    for (size_t i = 0; i < h->ts_urb_count; i++)
        h->ts_urbs[i]->device_handle = h->dev_hdl;
    return ESP_OK;
}

void it9507_usb_stop(it9507_handle_t h)
{
    h->evt_run = false;
    if (h->evt_task) { vTaskDelay(pdMS_TO_TICKS(30)); }
    if (h->dev_hdl) {
        usb_host_interface_release(h->client, h->dev_hdl, h->itf_num);
        usb_host_device_close(h->client, h->dev_hdl);
    }
    if (h->cmd_out) usb_host_transfer_free(h->cmd_out);
    if (h->cmd_in)  usb_host_transfer_free(h->cmd_in);
    if (h->ts_urbs) {
        for (size_t i = 0; i < h->ts_urb_count; i++)
            if (h->ts_urbs[i]) usb_host_transfer_free(h->ts_urbs[i]);
        free(h->ts_urbs);
    }
    if (h->client) usb_host_client_deregister(h->client);
    if (h->own_host) usb_host_uninstall();
}

/* Round a bulk-IN length up to the endpoint max packet size. */
static inline size_t round_mps(size_t n, uint16_t mps)
{
    return ((n + mps - 1) / mps) * mps;
}

int it9507_usb_cmd_xfer(it9507_handle_t h, it9507_req_t *req, size_t req_len,
                        it9507_reply_t *rep)
{
    if (req_len > 320) return -1;
    int ret = 0;
    xSemaphoreTake(h->cmd_mtx, portMAX_DELAY);

    /* ---- request (bulk OUT 0x02) ---- */
    memcpy(h->cmd_out->data_buffer, req, req_len);
    h->cmd_out->num_bytes = req_len;
    if (usb_host_transfer_submit(h->cmd_out) != ESP_OK) { ret = -1; goto out; }
    xSemaphoreTake(h->cmd_out_done, portMAX_DELAY);
    if (h->cmd_out_status != USB_TRANSFER_STATUS_COMPLETED ||
        (size_t)h->cmd_out_actual != req_len) { ret = -1; goto out; }

    if (!rep) goto out;

    /* ---- reply (bulk IN 0x81) ---- */
    h->cmd_in->num_bytes = round_mps(sizeof(*rep), IT9507_USB_MAX_PACKET);
    if (usb_host_transfer_submit(h->cmd_in) != ESP_OK) { ret = -1; goto out; }
    xSemaphoreTake(h->cmd_in_done, portMAX_DELAY);
    if (h->cmd_in_status != USB_TRANSFER_STATUS_COMPLETED) { ret = -1; goto out; }
    memcpy(rep, h->cmd_in->data_buffer,
           h->cmd_in_actual < (int)sizeof(*rep) ? h->cmd_in_actual : (int)sizeof(*rep));
    ret = h->cmd_in_actual;

out:
    xSemaphoreGive(h->cmd_mtx);
    return ret;
}

esp_err_t it9507_usb_ts_submit(it9507_handle_t h, const uint8_t *buf, size_t len,
                               TickType_t wait)
{
    while (len) {
        size_t chunk = len < h->ts_urb_size ? len : h->ts_urb_size;
        usb_transfer_t *t = NULL;
        if (xQueueReceive(h->ts_free, &t, wait) != pdTRUE)
            return ESP_ERR_TIMEOUT;                 /* channel can't keep up */
        memcpy(t->data_buffer, buf, chunk);
        t->num_bytes = chunk;
        if (usb_host_transfer_submit(t) != ESP_OK) {
            xQueueSend(h->ts_free, &t, 0);
            return ESP_FAIL;
        }
        buf += chunk;
        len -= chunk;
    }
    return ESP_OK;
}
