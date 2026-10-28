/*
 * app_main.c - ESP32-P4 wireless video pipeline, end to end:
 *
 *   OV5647 --MIPI-CSI--> ISP --YUV420--> H.264 HW encoder --Annex-B-->
 *       ts_mux --CBR MPEG2-TS--> it9507_write_ts() --USB--> IT9507 --> RF
 *
 * Threading model: a dedicated capture task blocks on the camera, copies each
 * frame into a shared buffer, and re-queues the V4L2 buffer IMMEDIATELY so the
 * CSI/ISP never starves. The main task encodes the latest frame at its own
 * (slower) rate, naturally dropping frames it can't keep up with. This decouples
 * the 20 fps sensor from the slower encode/mux/RF path - without it the camera
 * stalls after the buffers fill.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <string.h>

#include "it9507.h"
#include "ts_mux.h"
#include "cam_capture.h"
#include "h264_encoder.h"
#include "overlay.h"
#include "frame_dump.h"
#include "rgb_convert.h"

static const char *TAG = "app";

/* FCC Part 97.119 station identification. Replace with YOUR licensed callsign. */
#define CALLSIGN     "N0CALL"

/* The OV5647 ISP path streams RGB565 reliably (planar YUV420 stalls the CSI on
 * this sensor), so we capture RGB565 and convert to I420 for the encoder. If the
 * video comes out green/purple, the ISP is emitting big-endian RGB565: flip this. */
#define RGB565_BYTESWAP  false

/* resolution x fps must fit the CBR target, which must fit the DVB-T channel.
 * IMPORTANT: use a NATIVE OV5647 mode. The driver's RAW modes are 800x640,
 * 800x800, 800x1280, 1280x960, 1920x1080 - off-grid sizes (e.g. 640x480 or
 * 1280x720) make the ISP/CSI die after one frame. 800x640 streams cleanly
 * (the stock capture_stream example runs the same sensor at 800x800 @ 34fps). */
/* Capture at the sensor/ISP's NATIVE size -- requesting a non-supported size
 * makes S_FMT fail and the pipeline hangs. Instead we downscale 2:1 inside the
 * conversion pass we already run, so the ENCODER sees 400x320: a quarter of the
 * pixels, which quarters both the coded frame size (400 KB keyframes were
 * bursting the 3 Mbps channel) and the write-side PSRAM traffic, and skips every
 * other source row on the read side. Set ENC_DOWNSCALE to 1 for full size. */
#define VID_W        800
#define VID_H        640
#define VID_FPS      10
#define ENC_DOWNSCALE 2
#define VID_BPS      1200000      /* H.264 CBR. Must fit inside TS_BPS alongside PSI,
                                   * PCR and padding. Frames were arriving ~8 Mbps
                                   * (400 KB each) into a 3 Mbps channel, overflowing
                                   * the ring and shredding the picture: rate control
                                   * budgets per frame as bitrate/fps, so an honest
                                   * fps matters as much as the bitrate itself. */
#define TS_BPS       3000000      /* Comfortably under the ~3.0 Mbps effective drain so
                                   * the FIFO rarely fills. If it ever does, the tx_task
                                   * auto-recovers (it9507_ts_recover) instead of freezing,
                                   * so the exact value isn't critical anymore. */

static it9507_handle_t s_mod;

/* ---- capture relay: capture task -> encode loop ------------------------- */
static cam_capture_handle_t s_cam;
static SemaphoreHandle_t    s_lock;
/* Triple-buffered capture hand-off. The encode loop used to hold s_lock across
 * the whole conversion, so capture_task blocked waiting to publish and the two
 * ran in lockstep (both measured ~340 ms). With three buffers the lock is only
 * held for a pointer swap: capture always has a free buffer to fill, and encode
 * converts from its own buffer with the lock released. */
#define CAP_BUFS 3
static uint8_t             *s_buf[CAP_BUFS];   /* frame buffers (PSRAM)        */
static volatile int         s_ready = -1;      /* newest complete buffer index */
static volatile int         s_inuse = -1;      /* buffer the encoder is reading */
static uint8_t             *s_shared;          /* alias of s_buf[0] for setup   */
static size_t               s_shared_len;
static volatile uint32_t    s_seq;         /* bumped on every new frame       */
static volatile bool        s_run = true;

/* -------- decoupled TX path -----------------------------------------------
 * The CBR muxer pads null packets to fill the wall-clock PTS gap between access
 * units. If write_ts() blocks (channel momentarily full) INSIDE the encode
 * callback, the encode loop stalls, the next frame's PTS jumps further ahead,
 * the muxer emits even more nulls, which blocks write_ts even longer -> runaway
 * (the 1.5s -> 21s -> 292s gaps). Fix: the sink only drops TS into a ring and
 * never blocks; a dedicated task drains the ring to the modulator. The encode
 * loop then stays steady, so PTS gaps -- and null padding -- stay small. */
#define TS_RING_BYTES  (128 * 1024)   /* ~0.35 s at 3 Mbps. Deep rings ARE latency:
                                       * every byte waits behind whatever is already
                                       * queued, so 512 KB cost ~1.4 s of delay on a
                                       * 3 Mbps channel. Keep it just deep enough to
                                       * absorb bursts. */
#define TX_CHUNK_BYTES (32 * 1024)

static StreamBufferHandle_t s_ts_ring;
static StaticStreamBuffer_t s_ts_ring_ctrl;
static volatile uint64_t    s_tx_bytes;     /* actually written to the IT9507   */
static volatile uint32_t    s_tx_errors;    /* write_ts failures                */
static volatile uint64_t    s_ts_dropped;   /* bytes dropped because ring full  */

/* ts_mux sink: enqueue only, never block. Drop whole chunks if the ring is full
 * so TS packet alignment is preserved across the gap (better than a torn write).*/
static esp_err_t ts_sink(void *ctx, const uint8_t *pkts, size_t len)
{
    (void)ctx;
    if (xStreamBufferSpacesAvailable(s_ts_ring) >= len)
        xStreamBufferSend(s_ts_ring, pkts, len, 0);
    else
        s_ts_dropped += len;
    return ESP_OK;                       /* never stall the encoder */
}

/* Drains the ring to the modulator. write_ts BLOCKS (long timeout) so it pends
 * through the modulator's flow-control backpressure (mp2if_stop_en NAKs when its
 * FIFO is full) instead of timing out. That self-paces delivery to the drain
 * rate: the FEC stays continuously fed (our TS is transmitted, not nulls) and
 * the FIFO never accumulates to a stall. This is what the proven driver does --
 * plain blocking bulk writes gated by flow control, no manual rate matching. */
static void tx_task(void *arg)
{
    (void)arg;
    uint8_t *buf = heap_caps_malloc(TX_CHUNK_BYTES, MALLOC_CAP_SPIRAM);
    configASSERT(buf);
    uint32_t iters = 0, fails = 0, recoveries = 0;
    while (s_run) {
        size_t n = xStreamBufferReceive(s_ts_ring, buf, TX_CHUNK_BYTES, pdMS_TO_TICKS(100));
        if (!n) continue;
        esp_err_t r = it9507_write_ts(s_mod, buf, n, pdMS_TO_TICKS(2000));
        if (r == ESP_OK) { s_tx_bytes += n; fails = 0; }
        else {
            s_tx_errors++;
            /* A few write timeouts in a row means the modulator stopped accepting
             * TS -- its tx_fifo_overflow latch tripped and the FEC halted. Pulse
             * the MPEG2-bridge reset to clear it and resume, instead of freezing. */
            if (++fails >= 3) {
                it9507_ts_recover(s_mod);
                recoveries++;
                fails = 0;
                ESP_LOGW(TAG, "TS FIFO stalled -> recovered (%u total)", recoveries);
            }
        }
        if ((++iters & 0x3F) == 0)
            ESP_LOGI(TAG, "TX: %llu bytes to IT9507, %u errs | ring dropped %llu",
                     (unsigned long long)s_tx_bytes, s_tx_errors,
                     (unsigned long long)s_ts_dropped);
    }
    vTaskDelete(NULL);
}

/* encoder -> ts_mux. Called once per coded access unit. */
static esp_err_t on_access_unit(void *ctx, const uint8_t *au, size_t len,
                                bool keyframe, int64_t pts_90k)
{
    ts_mux_t *mux = ctx;
    return ts_mux_h264_au(mux, au, len, keyframe, pts_90k);
}

/* Capture task: block for a frame, publish it, recycle the buffer at once. */
static void capture_task(void *arg)
{
    (void)arg;
    uint32_t got = 0;
    while (s_run) {
        cam_frame_t f;
        esp_err_t r = cam_capture_get(s_cam, &f, -1 /*block, no select*/);
        if (r != ESP_OK) { ESP_LOGW(TAG, "capture get 0x%x", r); vTaskDelay(1); continue; }

        if (got < 10) ESP_LOGI(TAG, "capture: frame %u (idx %u, %uB; RGB565 expects %uB%s)",
                               got, f.index, (unsigned)f.len, (unsigned)((size_t)VID_W*VID_H*2),
                               f.len==(size_t)VID_W*VID_H*2 ? "" : " <-- MISMATCH: ISP not RGB565!");

        /* Pick a buffer that is neither the one the encoder is reading nor the
         * one already published, copy into it, then publish with a short lock. */
        int slot = -1;
        xSemaphoreTake(s_lock, portMAX_DELAY);
        for (int i = 0; i < CAP_BUFS; i++)
            if (i != s_inuse && i != s_ready) { slot = i; break; }
        xSemaphoreGive(s_lock);
        if (slot < 0) { cam_capture_put(s_cam, f.index); continue; }  /* shouldn't happen */

        memcpy(s_buf[slot], f.data, f.len);

        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_ready = slot;
        s_shared_len = f.len;
        s_seq++;
        xSemaphoreGive(s_lock);

        cam_capture_put(s_cam, f.index);   /* immediate -> CSI never starves */
        if (++got % 100 == 0) ESP_LOGI(TAG, "capture: %u frames (seq %u)", got, s_seq);

        /* Yield a tick so IDLE0 (and the watchdog) run. Blocking DQBUF returns
         * immediately while buffers are backed up, so without this the task
         * busy-loops and starves the core. ~10ms cap is well above the rate the
         * encoder consumes, and DQBUF still paces us to the sensor otherwise. */
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

static esp_err_t modulator_up(void)
{
    it9507_config_t cfg = IT9507_CONFIG_DEFAULT();
    cfg.ts_urb_size  = 188 * 174;   /* ~32 KB bulk transfers */
    cfg.ts_urb_count = 6;
    cfg.task_core_id = 1;           /* keep USB IRQs off the capture/encode core */
    ESP_RETURN_ON_ERROR(it9507_install(&cfg, &s_mod), TAG, "it9507_install");

    it9507_modulation_t mod = {
        .frequency_khz     = 425000,
        .bandwidth_khz     = 6000,       /* standard 6 MHz (UT-120 locks it)     */
        .gain_db           = -20,        /* reduce output power to run cooler     */
        .constellation     = IT9507_QPSK,    /* lower channel rate so it fits the */
        .code_rate         = IT9507_FEC_1_2, /* ~5.7 Mbps our USB/TS path sustains */
        .guard_interval    = IT9507_GI_1_4,  /* -> QPSK 1/2 1/4 6MHz = 3.73 Mbps  */
        .transmission_mode = IT9507_TX_MODE_8K,
        .cell_id           = 0,
    };
    ESP_RETURN_ON_ERROR(it9507_set_modulation(s_mod, &mod), TAG, "set_modulation");
    ESP_RETURN_ON_ERROR(it9507_start(s_mod), TAG, "it9507_start");
    return ESP_OK;
}

void app_main(void)
{
    /* (4) RF first, so the TS sink has somewhere to go. */
    ESP_ERROR_CHECK(modulator_up());

    /* Decoupled TX path: ring buffer + drain task, created before the muxer so
     * the very first packets have somewhere to go and the encode loop can never
     * block on write_ts(). */
    uint8_t *ring_buf = heap_caps_malloc(TS_RING_BYTES + 1, MALLOC_CAP_SPIRAM);
    ESP_ERROR_CHECK(ring_buf ? ESP_OK : ESP_ERR_NO_MEM);
    s_ts_ring = xStreamBufferCreateStatic(TS_RING_BYTES, 1, ring_buf, &s_ts_ring_ctrl);
    ESP_ERROR_CHECK(s_ts_ring ? ESP_OK : ESP_ERR_NO_MEM);
    xTaskCreatePinnedToCore(tx_task, "ts_tx", 4096, NULL, 5, NULL, 1);
    it9507_dump_status(s_mod);   /* confirm modulation took + FIFO state at start */

    /* (3) Muxer. PCR every 30 ms (inside DVB's 40 ms limit), PSI every 200 ms. */
    static ts_mux_t mux;
    ts_mux_cfg_t mcfg = {
        .target_bps      = TS_BPS,
        .pcr_interval_ms = 30,
        .psi_interval_ms = 200,
        .pts_delay_ms    = 200,
    };
    ts_mux_init(&mux, ts_sink, NULL, &mcfg);

    /* (1) Camera. Capture RGB565 (the ISP path that streams reliably on the
     * OV5647); we convert to I420 for the encoder in the loop below. */
    cam_capture_cfg_t ccfg = CAM_CAPTURE_CFG_DEFAULT();
    ccfg.width = VID_W; ccfg.height = VID_H; ccfg.buf_count = 6;
    ccfg.format = CAM_FMT_RGB565;
    ESP_ERROR_CHECK(cam_capture_start(&ccfg, &s_cam));

    uint32_t w, h, rgb_bytes;
    cam_capture_get_format(s_cam, &w, &h, &rgb_bytes);
    /* Encoder-side geometry after the 2:1 downscale done in the conversion pass.
     * Capture stays at the ISP's native size (w,h); the encoder sees (ew,eh). */
    const uint32_t ew = w / ENC_DOWNSCALE, eh = h / ENC_DOWNSCALE;
    size_t i420_bytes = (size_t)ew * eh * 3 / 2;

    /* s_shared holds the RGB565 frame; yuv holds the converted I420 we encode.
     * We convert straight out of s_shared (no intermediate copy), so only these
     * two buffers are needed. Start the capture pump before the encoder init so
     * the CSI never overflows. */
    for (int i = 0; i < CAP_BUFS; i++) {
        s_buf[i] = heap_caps_malloc(rgb_bytes, MALLOC_CAP_SPIRAM);
        ESP_ERROR_CHECK(s_buf[i] ? ESP_OK : ESP_ERR_NO_MEM);
    }
    s_shared = s_buf[0];
    uint8_t *yuv = heap_caps_malloc(i420_bytes, MALLOC_CAP_SPIRAM);
    /* Encoder-native packed buffer (O_UYY_E_VYY). Same size as I420. We keep the
     * I420 buffer because the overlay and the frame dump both work on planes. */
    uint8_t *packed = heap_caps_malloc(i420_bytes, MALLOC_CAP_SPIRAM);
    ESP_ERROR_CHECK((s_shared && yuv && packed) ? ESP_OK : ESP_ERR_NO_MEM);
    s_lock = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_lock ? ESP_OK : ESP_ERR_NO_MEM);
    /* Capture pump on core 1 (with USB), leaving core 0 for the encode loop -
     * its 1MB/frame memcpy was starving the encoder when co-located. */
    xTaskCreatePinnedToCore(capture_task, "cam", 4096, NULL, 6, NULL, 1);

    /* (2) Encoder, sized to the negotiated geometry. */
    h264_encoder_handle_t enc;
    h264_encoder_cfg_t ecfg = H264_ENCODER_CFG_DEFAULT();
    ecfg.width = ew; ecfg.height = eh;
    /* GOP of VID_FPS assumed ~20 fps -> a keyframe every second. At the low real
     * frame rate that became a keyframe every ~16 s, so any loss persisted for
     * ages and a newly-tuned receiver waited that long for a picture. Cap it. */
    /* GOP ~1 s of real time. gop=4 at a low frame rate meant a 400 KB keyframe
     * every ~1.5 s, which alone exceeded the channel. */
    ecfg.bitrate_bps = VID_BPS; ecfg.fps = VID_FPS; ecfg.gop = VID_FPS;
    /* qp_min is a QUALITY FLOOR: at 20 the encoder refuses to compress harder
     * than "visually near-lossless", so it overshot the bitrate target ~3x
     * (60 KB frames at 6.7 fps = 3.3 Mbps into a 3 Mbps channel) and the ring
     * kept dropping. Letting QP rise lets rate control actually hit its budget.
     * Lower qp_min for a crisper picture if the channel has headroom. */
    ecfg.qp_min = 28; ecfg.qp_max = 46;
    ecfg.qp_min = 20; ecfg.qp_max = 40;
    ESP_ERROR_CHECK(h264_encoder_start(&ecfg, &enc));

    overlay_style_t ovl = OVERLAY_STYLE_DEFAULT();
    ovl.anchor = OVL_BOTTOM_RIGHT; ovl.scale = (w >= 1280) ? 3 : 2;

    ESP_LOGI(TAG, "pipeline live: capture %ux%u -> encode %ux%u -> %u bps -> DVB-T",
             w, h, ew, eh, VID_BPS);

    /* Encode loop: take the newest frame, encode at our own rate, drop the rest.
     * Wall-clock PTS keeps the CBR muxer correct despite frame dropping. */
    uint32_t last_seq = 0, frames = 0, drops = 0;
    int64_t  t0 = esp_timer_get_time();

    while (1) {
        /* Convert straight out of the shared capture buffer -- no intermediate
         * copy. We hold the lock across the (few-tens-of-ms) conversion; the
         * capture task has 4 CSI buffers of headroom so it won't overflow while
         * it waits to publish the next frame. */
        /* Claim the newest complete frame, then release the lock immediately so
         * capture_task can keep filling other buffers while we convert. */
        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (s_seq == last_seq || s_ready < 0) {
            xSemaphoreGive(s_lock);
            vTaskDelay(1);                 /* no new frame yet; yield a tick */
            continue;
        }
        last_seq = s_seq;
        int myslot = s_ready;
        s_inuse = myslot;
        xSemaphoreGive(s_lock);

        /* One pass: RGB565 -> packed encoder format, written straight into the
         * encoder's own input buffer. Previously this was three passes over
         * PSRAM per frame (convert to I420, pack, then copy into the encoder),
         * which cost ~450 ms/frame -- far more than the 26 ms encode itself. */
        uint8_t *inbuf = h264_encoder_input_buffer(enc, NULL);
        if (!inbuf) inbuf = packed;
        rgb565_to_uyy_vyy(s_buf[myslot], inbuf, (int)w, (int)h,
                          RGB565_BYTESWAP, ENC_DOWNSCALE);

        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_inuse = -1;                      /* release the buffer back to capture */
        xSemaphoreGive(s_lock);

        /* Burn in the callsign (packed-layout aware). */
        overlay_draw_text_packed(inbuf, (int)ew, (int)eh, CALLSIGN, &ovl);

        /* ---- geometry probe -------------------------------------------------
         * Set TEST_PATTERN 1 to replace the camera frame with a known synthetic
         * I420 image. The encoder input is PROVEN-correct I420 (frame dump) and
         * the encoder accepts only YU12, yet the received picture repeats
         * horizontally -- so the encoder is mapping a correct buffer to the wrong
         * output geometry. A known pattern makes that mapping readable directly
         * off the receiver:
         *   - luma = smooth left-to-right ramp (black -> white)
         *   - a white square in the TOP-LEFT corner
         *   - a bright vertical line at the exact horizontal centre
         *   - chroma pinned neutral (128) so the image must appear GREYSCALE
         * Read the received picture:
         *   how many ramps  -> exact horizontal repeat factor
         *   square count/position -> row-start mapping
         *   any colour at all -> chroma planes are being misread
         * Set back to 0 for live video. */
#define TEST_PATTERN 0
#if TEST_PATTERN
        {
            uint8_t *Y = yuv, *U = yuv + (size_t)w * h;
            uint8_t *V = U + (size_t)(w / 2) * (h / 2);
            for (int y = 0; y < (int)h; y++) {
                uint8_t *row = Y + (size_t)y * w;
                for (int x = 0; x < (int)w; x++) row[x] = (uint8_t)(x * 255 / (int)(w - 1));
                row[w / 2] = 255;                       /* centre line */
            }
            for (int y = 0; y < 60 && y < (int)h; y++)  /* top-left square */
                memset(Y + (size_t)y * w, 255, 60);
            memset(U, 128, (size_t)(w / 2) * (h / 2));  /* neutral chroma */
            memset(V, 128, (size_t)(w / 2) * (h / 2));
        }
#endif

        /* One-shot diagnostic: dump the exact I420 buffer we hand the encoder.
         * The overlay is drawn ONCE here, so if the reconstructed PNG shows one
         * callsign and sane colors, the capture+conversion are correct and the
         * 3x tiling/color error is introduced by the encoder; if the PNG itself
         * is tiled, the fault is upstream. Set DUMP_ONE_FRAME 0 to disable. */
#define DUMP_ONE_FRAME 0
#if DUMP_ONE_FRAME
        static bool dumped = false;
        if (!dumped && frames == 30) {      /* let AE settle, then dump one */
            dumped = true;
            frame_dump_base64(yuv, i420_bytes, (int)ew, (int)eh, "i420");
        }
#endif

        int64_t pts = (esp_timer_get_time() - t0) * 9 / 100;    /* us -> 90 kHz */
        esp_err_t r = (inbuf != packed)
            ? h264_encoder_encode_filled(enc, inbuf, NULL, i420_bytes, pts,
                                         on_access_unit, &mux)
            : h264_encoder_encode(enc, packed, i420_bytes, pts, on_access_unit, &mux);
        if (r == ESP_ERR_TIMEOUT) drops++;
        else if (r != ESP_OK)     ESP_LOGW(TAG, "encode/mux err 0x%x", r);

        if (++frames % 50 == 0) {
            ESP_LOGI(TAG, "encode: %u frames, %u drops | TX %llu B, %u errs, dropped %llu B",
                     frames, drops, (unsigned long long)s_tx_bytes, s_tx_errors,
                     (unsigned long long)s_ts_dropped);
            it9507_dump_status(s_mod);   /* so we see FIFO/PLL state alongside TX */
        }
    }
}