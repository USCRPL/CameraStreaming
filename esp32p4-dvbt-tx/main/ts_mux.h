/*
 * ts_mux.h - H.264 -> single-program MPEG2-TS muxer for feeding the IT9507.
 *
 * Hardened for constant-bit-rate DVB-T:
 *   - PCR is derived from the TRANSPORT BYTE CLOCK (byte position / bitrate),
 *     not from PTS, so a receiver's clock-recovery PLL locks cleanly.
 *   - PCR is inserted at a guaranteed interval (default 30 ms <= DVB's 40 ms),
 *     using PCR-only packets between/within access units as needed.
 *   - PTS leads PCR by a fixed decode buffer delay so the decoder never starves.
 *   - Keyframes set random_access_indicator so receivers can join quickly.
 *   - The stream is null-padded (PID 0x1FFF) to exactly target_bps.
 *
 * target_bps must equal the DVB-T channel's usable TS rate (what you program
 * into the IT9507). The modulator drains at that rate; the muxer fills to it.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#define TS_PKT          188
#define TS_PID_PAT      0x0000
#define TS_PID_SDT      0x0011      /* service description -> scanner channel list */
#define TS_PID_PMT      0x1000
#define TS_PID_VIDEO    0x0100      /* also the PCR PID */
#define TS_PID_NULL     0x1FFF
#define TS_PROGRAM_NUM  1

/* Sink receives whole 188-byte TS packets. Wire it to it9507_write_ts(). */
typedef esp_err_t (*ts_sink_fn)(void *ctx, const uint8_t *pkts, size_t len);

typedef struct {
    uint32_t target_bps;       /* TS CBR rate = DVB-T channel usable rate     */
    uint32_t pcr_interval_ms;  /* max PCR spacing, 0 -> 30 ms                 */
    uint32_t psi_interval_ms;  /* PAT/PMT spacing, 0 -> 200 ms                */
    uint32_t pts_delay_ms;     /* PTS lead over PCR (decoder buffer), 0 ->200 */
} ts_mux_cfg_t;

typedef struct {
    ts_sink_fn sink;
    void      *ctx;
    uint32_t   target_bps;
    uint64_t   pcr_interval_bytes;
    uint64_t   psi_interval_bytes;
    int64_t    pts_delay_90k;

    uint8_t    cc_pat, cc_pmt, cc_vid, cc_sdt;   /* 4-bit continuity counters */
    uint64_t   bytes_out;                /* total TS bytes emitted            */
    bool       have_base;
    int64_t    first_pts_90k;            /* origin for the real-time clock    */
    uint64_t   last_psi_bytes;
    uint64_t   last_pcr_bytes;
    bool       have_pcr;
} ts_mux_t;

void ts_mux_init(ts_mux_t *m, ts_sink_fn sink, void *ctx, const ts_mux_cfg_t *cfg);

/* Mux one H.264 access unit (Annex-B, with start codes) at the given 90 kHz
 * PTS. `keyframe` marks an IDR so the AU is flagged as a random-access point.
 * Emits PSI/PCR as due, the PES, and CBR null padding. */
esp_err_t ts_mux_h264_au(ts_mux_t *m, const uint8_t *au, size_t len,
                         bool keyframe, int64_t pts_90k);
