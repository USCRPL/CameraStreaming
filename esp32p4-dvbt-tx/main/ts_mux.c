/*
 * ts_mux.c - CBR single-program MPEG2-TS muxer. See ts_mux.h.
 *
 * Clock model: the transport is a constant byte stream at target_bps. The time
 * a given byte leaves the mux is (byte_offset * 8 / target_bps). PCR is stamped
 * from exactly that, so it is a faithful transport clock the receiver can lock
 * to. PTS (presentation) comes from the encoder and is offset ahead of PCR by
 * pts_delay so the decoder always has data buffered.
 */
#include <string.h>
#include "ts_mux.h"

#define PCR_HZ      27000000ULL
#define SYS_HZ      90000LL
#define AF_FLAG_PCR         0x10
#define AF_FLAG_RANDOM_ACC  0x40

/* ---- MPEG (ISO/IEC 13818-1) section CRC32 ------------------------------ */
static uint32_t crc32_mpeg(const uint8_t *p, size_t n)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++) {
        crc ^= (uint32_t)p[i] << 24;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80000000) ? (crc << 1) ^ 0x04C11DB7 : (crc << 1);
    }
    return crc;
}

static esp_err_t emit(ts_mux_t *m, const uint8_t *pkt)
{
    m->bytes_out += TS_PKT;
    return m->sink(m->ctx, pkt, TS_PKT);
}

/* ---- PCR: derive 33-bit base (90 kHz) + 9-bit ext (27 MHz) from the byte
 *           offset of the packet about to be sent. ------------------------ */
static void write_pcr_field(ts_mux_t *m, uint8_t *af6)
{
    /* transport time of the first byte of this packet, in 27 MHz ticks:
     *   t27 = bytes_out * 8 * PCR_HZ / target_bps, wrapped at 300 * 2^33.
     * RV32 (ESP32-P4) has no __int128, so bound bytes_out to one wrap period
     * first; then n*A and M*D both fit in uint64 for any target_bps up to
     * ~7 Mbit/s (covers all our QPSK/16QAM channel rates). Exact for runs under
     * one PCR wrap (~26.5 h); after that the wrap point rounds by < 1 byte. */
    const uint64_t A = 8ULL * (uint64_t)PCR_HZ;      /* 216,000,000 */
    const uint64_t M = 300ULL << 33;                 /* PCR wrap, 27 MHz ticks */
    uint64_t D = (uint64_t)m->target_bps;
    uint64_t wrap_bytes = (M * D) / A;
    uint64_t n    = wrap_bytes ? (m->bytes_out % wrap_bytes) : m->bytes_out;
    uint64_t t27  = n * A / D;                        /* < M, fits in uint64 */
    uint64_t base = (t27 / 300) & 0x1FFFFFFFFULL;
    uint32_t ext  = (uint32_t)(t27 % 300);
    af6[0] = base >> 25;
    af6[1] = base >> 17;
    af6[2] = base >> 9;
    af6[3] = base >> 1;
    af6[4] = ((base & 1) << 7) | 0x7E | ((ext >> 8) & 1);
    af6[5] = ext & 0xFF;
    m->last_pcr_bytes = m->bytes_out;
    m->have_pcr = true;
}

/* A standalone adaptation-only packet that carries just a PCR (no payload).
 * Continuity counter does NOT advance for payload-less packets. */
static esp_err_t send_pcr_only(ts_mux_t *m)
{
    uint8_t p[TS_PKT];
    memset(p, 0xFF, sizeof(p));
    p[0] = 0x47;
    p[1] = (TS_PID_VIDEO >> 8) & 0x1F;          /* no PUSI */
    p[2] = TS_PID_VIDEO & 0xFF;
    p[3] = 0x20 | (m->cc_vid & 0x0F);           /* AFC=10 (adaptation only) */
    p[4] = 183;                                 /* AF length fills the packet */
    p[5] = AF_FLAG_PCR;
    write_pcr_field(m, &p[6]);
    /* p[12..187] already 0xFF stuffing */
    return emit(m, p);
}

/* ---- PSI: PAT + PMT ---------------------------------------------------- */
static esp_err_t send_pat(ts_mux_t *m)
{
    uint8_t p[TS_PKT];
    memset(p, 0xFF, sizeof(p));
    int i = 0;
    p[i++] = 0x47;
    p[i++] = 0x40 | (TS_PID_PAT >> 8);
    p[i++] = TS_PID_PAT & 0xff;
    p[i++] = 0x10 | (m->cc_pat++ & 0x0f);
    p[i++] = 0x00;                              /* pointer field */
    uint8_t *sec = &p[i];
    int s = 0;
    sec[s++] = 0x00;                            /* table_id PAT */
    sec[s++] = 0xB0; sec[s++] = 13;             /* syntax + section_length */
    sec[s++] = 0x00; sec[s++] = 0x01;           /* TS id */
    sec[s++] = 0xC1; sec[s++] = 0x00; sec[s++] = 0x00;
    sec[s++] = (TS_PROGRAM_NUM >> 8); sec[s++] = TS_PROGRAM_NUM & 0xff;
    sec[s++] = 0xE0 | (TS_PID_PMT >> 8); sec[s++] = TS_PID_PMT & 0xff;
    uint32_t crc = crc32_mpeg(sec, s);
    sec[s++] = crc >> 24; sec[s++] = crc >> 16; sec[s++] = crc >> 8; sec[s++] = crc;
    return emit(m, p);
}

static esp_err_t send_pmt(ts_mux_t *m)
{
    uint8_t p[TS_PKT];
    memset(p, 0xFF, sizeof(p));
    int i = 0;
    p[i++] = 0x47;
    p[i++] = 0x40 | (TS_PID_PMT >> 8);
    p[i++] = TS_PID_PMT & 0xff;
    p[i++] = 0x10 | (m->cc_pmt++ & 0x0f);
    p[i++] = 0x00;                              /* pointer field */
    uint8_t *sec = &p[i];
    int s = 0;
    sec[s++] = 0x02;                            /* table_id PMT */
    sec[s++] = 0x00; sec[s++] = 0x00;           /* syntax + length, patched */
    sec[s++] = (TS_PROGRAM_NUM >> 8); sec[s++] = TS_PROGRAM_NUM & 0xff;
    sec[s++] = 0xC1; sec[s++] = 0x00; sec[s++] = 0x00;
    sec[s++] = 0xE0 | (TS_PID_VIDEO >> 8); sec[s++] = TS_PID_VIDEO & 0xff; /* PCR PID */
    sec[s++] = 0xF0; sec[s++] = 0x00;           /* program_info_length = 0 */
    sec[s++] = 0x1B;                            /* stream_type H.264 */
    sec[s++] = 0xE0 | (TS_PID_VIDEO >> 8); sec[s++] = TS_PID_VIDEO & 0xff;
    sec[s++] = 0xF0; sec[s++] = 0x00;           /* ES_info_length = 0 */
    int sec_len = (s - 3) + 4;                  /* to end of CRC */
    sec[1] = 0xB0 | ((sec_len >> 8) & 0x0F);
    sec[2] = sec_len & 0xFF;
    uint32_t crc = crc32_mpeg(sec, s);
    sec[s++] = crc >> 24; sec[s++] = crc >> 16; sec[s++] = crc >> 8; sec[s++] = crc;
    return emit(m, p);
}

/* Minimal SDT (one running service) so a receiver's channel scan lists it. */
static esp_err_t send_sdt(ts_mux_t *m)
{
    static const char provider[] = "ESP32P4";
    static const char service[]  = "ESP32-DVBT";
    uint8_t p[TS_PKT];
    memset(p, 0xFF, sizeof(p));
    int i = 0;
    p[i++] = 0x47;
    p[i++] = 0x40 | (TS_PID_SDT >> 8);          /* PUSI */
    p[i++] = TS_PID_SDT & 0xff;
    p[i++] = 0x10 | (m->cc_sdt++ & 0x0f);
    p[i++] = 0x00;                              /* pointer field */
    uint8_t *sec = &p[i];
    int s = 0;
    sec[s++] = 0x42;                            /* table_id: SDT, actual TS */
    sec[s++] = 0x00; sec[s++] = 0x00;           /* syntax + length, patched below */
    sec[s++] = 0x00; sec[s++] = 0x01;           /* transport_stream_id (match PAT) */
    sec[s++] = 0xC1;                            /* reserved + version + current_next */
    sec[s++] = 0x00;                            /* section_number */
    sec[s++] = 0x00;                            /* last_section_number */
    sec[s++] = 0x00; sec[s++] = 0x01;           /* original_network_id */
    sec[s++] = 0xFF;                            /* reserved_future_use */

    /* one service entry */
    sec[s++] = (TS_PROGRAM_NUM >> 8); sec[s++] = TS_PROGRAM_NUM & 0xff; /* service_id */
    sec[s++] = 0xFC;                            /* reserved + EIT flags = 0 */

    /* service_descriptor (0x48) */
    uint8_t d[48]; int dl = 0;
    d[dl++] = 0x48;                             /* descriptor_tag */
    int dlen = dl++;                            /* descriptor_length placeholder */
    d[dl++] = 0x01;                             /* service_type: digital TV */
    d[dl++] = (uint8_t)(sizeof(provider) - 1);
    memcpy(&d[dl], provider, sizeof(provider) - 1); dl += sizeof(provider) - 1;
    d[dl++] = (uint8_t)(sizeof(service) - 1);
    memcpy(&d[dl], service, sizeof(service) - 1); dl += sizeof(service) - 1;
    d[dlen] = (uint8_t)(dl - dlen - 1);

    sec[s++] = 0x80 | ((dl >> 8) & 0x0F);       /* running_status=4 + free_CA=0 + len_hi */
    sec[s++] = dl & 0xFF;                        /* descriptors_loop_length lo */
    memcpy(&sec[s], d, dl); s += dl;

    int sec_len = (s - 3) + 4;                  /* to end of CRC */
    sec[1] = 0xB0 | ((sec_len >> 8) & 0x0F);
    sec[2] = sec_len & 0xFF;
    uint32_t crc = crc32_mpeg(sec, s);
    sec[s++] = crc >> 24; sec[s++] = crc >> 16; sec[s++] = crc >> 8; sec[s++] = crc;
    return emit(m, p);
}

static esp_err_t maybe_send_psi(ts_mux_t *m)
{
    if (m->have_base && (m->bytes_out - m->last_psi_bytes) < m->psi_interval_bytes)
        return ESP_OK;
    esp_err_t e = send_pat(m); if (e != ESP_OK) return e;
    e = send_pmt(m);           if (e != ESP_OK) return e;
    e = send_sdt(m);           if (e != ESP_OK) return e;
    m->last_psi_bytes = m->bytes_out;
    return ESP_OK;
}

/* ---- PES on the video PID --------------------------------------------- */
static void build_pes_header(uint8_t *hdr, int *hlen, int64_t pts_90k)
{
    int h = 0;
    hdr[h++] = 0x00; hdr[h++] = 0x00; hdr[h++] = 0x01; hdr[h++] = 0xE0; /* video */
    hdr[h++] = 0x00; hdr[h++] = 0x00;          /* PES_packet_length = 0 (unbounded) */
    hdr[h++] = 0x84;                           /* '10', data_alignment_indicator */
    hdr[h++] = 0x80;                           /* PTS flag */
    hdr[h++] = 0x05;                           /* PES header data length */
    int64_t p = pts_90k & 0x1FFFFFFFFLL;
    hdr[h++] = 0x20 | (((p >> 30) & 0x07) << 1) | 0x01;
    hdr[h++] = (p >> 22) & 0xFF;
    hdr[h++] = (((p >> 15) & 0x7F) << 1) | 0x01;
    hdr[h++] = (p >> 7) & 0xFF;
    hdr[h++] = ((p & 0x7F) << 1) | 0x01;
    *hlen = h;
}

/* Two-segment cursor: PES header followed by the access unit, so we never
 * allocate or concatenate per frame. */
typedef struct { const uint8_t *seg[2]; size_t len[2]; int idx; size_t off; } pes_cursor_t;

static size_t cur_remaining(const pes_cursor_t *c)
{
    size_t s = 0;
    for (int k = c->idx; k < 2; k++) s += c->len[k] - (k == c->idx ? c->off : 0);
    return s;
}

static int cur_copy(pes_cursor_t *c, uint8_t *dst, int n)
{
    int got = 0;
    while (n > 0 && c->idx < 2) {
        size_t avail = c->len[c->idx] - c->off;
        if (avail == 0) { c->idx++; c->off = 0; continue; }
        int t = (avail < (size_t)n) ? (int)avail : n;
        memcpy(dst, c->seg[c->idx] + c->off, t);
        dst += t; c->off += t; n -= t; got += t;
    }
    return got;
}

/* Emit exactly one 188-byte TS packet of the video PES, drawing payload from
 * the cursor. An adaptation field is added when the packet must carry a PCR or
 * random-access flag, or when the remaining payload is too small to fill the
 * packet (the leftover becomes AF stuffing). By construction the packet is
 * always filled to exactly TS_PKT - no null packets are ever needed mid-AU. */
static esp_err_t send_one_video_packet(ts_mux_t *m, pes_cursor_t *cur,
                                       bool start, bool want_pcr, bool rai)
{
    uint8_t pkt[TS_PKT];
    int i = 0;
    pkt[i++] = 0x47;
    pkt[i++] = (start ? 0x40 : 0x00) | (TS_PID_VIDEO >> 8);  /* PUSI on first */
    pkt[i++] = TS_PID_VIDEO & 0xFF;
    int afc_idx = i++;                       /* AFC + continuity byte */

    bool need_af = want_pcr || rai;
    int af_fixed = need_af ? (2 + (want_pcr ? 6 : 0)) : 0;   /* len+flags(+pcr) */
    int avail = TS_PKT - i - af_fixed;       /* payload bytes if AF stays as-is */
    size_t remaining = cur_remaining(cur);
    int n = (remaining < (size_t)avail) ? (int)remaining : avail;

    if (n < avail && !need_af) { need_af = true; af_fixed = 2; }  /* len+flags */
    int stuffing = TS_PKT - i - af_fixed - n;
    if (stuffing < 0) stuffing = 0;

    if (need_af) {
        pkt[afc_idx] = 0x30 | (m->cc_vid++ & 0x0F);          /* AF + payload */
        pkt[i++] = (uint8_t)(af_fixed - 1 + stuffing);       /* AF length */
        pkt[i++] = (want_pcr ? AF_FLAG_PCR : 0) | (rai ? AF_FLAG_RANDOM_ACC : 0);
        if (want_pcr) { write_pcr_field(m, &pkt[i]); i += 6; }
        if (stuffing) { memset(&pkt[i], 0xFF, stuffing); i += stuffing; }
    } else {
        pkt[afc_idx] = 0x10 | (m->cc_vid++ & 0x0F);          /* payload only */
    }

    if (n) i += cur_copy(cur, &pkt[i], n);
    /* i == TS_PKT here by construction */
    return emit(m, pkt);
}

static esp_err_t send_video_pes(ts_mux_t *m, const uint8_t *es, size_t len,
                                int64_t pts_90k, bool keyframe)
{
    uint8_t hdr[19]; int hlen = 0;
    build_pes_header(hdr, &hlen, pts_90k);

    pes_cursor_t cur = { .seg = { hdr, es },
                         .len = { (size_t)hlen, len }, .idx = 0, .off = 0 };
    bool start = true;
    while (cur_remaining(&cur) > 0) {
        /* first packet always carries PCR; later packets only when the PCR
         * interval has elapsed, so big AUs keep the clock fresh. */
        bool want_pcr = start ||
            (m->bytes_out - m->last_pcr_bytes) >= m->pcr_interval_bytes;
        bool rai = start && keyframe;
        esp_err_t e = send_one_video_packet(m, &cur, start, want_pcr, rai);
        if (e != ESP_OK) return e;
        start = false;
    }
    return ESP_OK;
}

/* ---- CBR null padding, with PCR kept alive between AUs ------------------ */
static esp_err_t send_null(ts_mux_t *m)
{
    uint8_t n[TS_PKT];
    memset(n, 0xFF, sizeof(n));
    n[0] = 0x47;
    n[1] = (TS_PID_NULL >> 8);
    n[2] = TS_PID_NULL & 0xff;
    n[3] = 0x10;                                /* payload only, CC irrelevant */
    return emit(m, n);
}

static esp_err_t pad_to_cbr(ts_mux_t *m, int64_t now_pts_90k)
{
    int64_t elapsed = now_pts_90k - m->first_pts_90k;     /* 90 kHz */
    if (elapsed < 0) return ESP_OK;
    /* bytes that should have left the mux by 'now' at the target rate.
     * uint64 (no __int128 on RV32) is exact for ~500 days of continuous runtime
     * at these rates -- far beyond any real session. */
    uint64_t want = (uint64_t)m->target_bps * (uint64_t)elapsed / SYS_HZ / 8;
    while (m->bytes_out + TS_PKT <= want) {
        esp_err_t e;
        if (m->have_pcr && (m->bytes_out - m->last_pcr_bytes) >= m->pcr_interval_bytes)
            e = send_pcr_only(m);
        else
            e = send_null(m);
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}

/* ---- public ------------------------------------------------------------ */
void ts_mux_init(ts_mux_t *m, ts_sink_fn sink, void *ctx, const ts_mux_cfg_t *cfg)
{
    memset(m, 0, sizeof(*m));
    m->sink = sink; m->ctx = ctx;
    m->target_bps = cfg->target_bps;
    uint32_t pcr_ms = cfg->pcr_interval_ms ? cfg->pcr_interval_ms : 30;
    uint32_t psi_ms = cfg->psi_interval_ms ? cfg->psi_interval_ms : 200;
    uint32_t pts_ms = cfg->pts_delay_ms    ? cfg->pts_delay_ms    : 200;
    m->pcr_interval_bytes = (uint64_t)m->target_bps / 8 * pcr_ms / 1000;
    m->psi_interval_bytes = (uint64_t)m->target_bps / 8 * psi_ms / 1000;
    m->pts_delay_90k      = (int64_t)pts_ms * SYS_HZ / 1000;
    if (m->pcr_interval_bytes < TS_PKT) m->pcr_interval_bytes = TS_PKT;
}

esp_err_t ts_mux_h264_au(ts_mux_t *m, const uint8_t *au, size_t len,
                         bool keyframe, int64_t pts_90k)
{
    if (!m->have_base) { m->first_pts_90k = pts_90k; m->have_base = true; }
    int64_t pts_out = (pts_90k - m->first_pts_90k) + m->pts_delay_90k;

    esp_err_t e = maybe_send_psi(m);                  if (e != ESP_OK) return e;
    e = send_video_pes(m, au, len, pts_out, keyframe); if (e != ESP_OK) return e;
    return pad_to_cbr(m, pts_90k);
}