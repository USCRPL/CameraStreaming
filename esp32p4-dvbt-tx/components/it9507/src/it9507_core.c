/*
 * it9507_core.c - command framing, register access, firmware upload, tuner
 *                 math and the public it9507_* API for ESP32-P4.
 *
 * The register sequences and tuner math are ported from the GPL-2.0 upstream
 * drivers (ITE it950x SDK / linuxstb / fabled). GPL-2.0 applies.
 */
#include <string.h>
#include <stdlib.h>
#include "it9507_private.h"
#include "it9507_firmware.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "it9507";

/* ===================== command / checksum layer ========================= */
static inline uint16_t be16(uint16_t v){ return (uint16_t)((v<<8)|(v>>8)); }

static uint16_t cmd_csum(const void *p, size_t len)
{
    const uint8_t *b = (const uint8_t *)p;
    uint16_t s = 0;
    for (size_t i = 0; i < len/2; i++) s += (uint16_t)((b[2*i]<<8) | b[2*i+1]);
    if (len & 1) s += (uint16_t)(b[len-1] << 8);
    return (uint16_t)~s;
}

/* Frame, send and (optionally) validate a reply. Returns 0 on success. */
static int talk(it9507_handle_t h, it9507_req_t *req, it9507_reply_t *rep)
{
    if (h->last_error) return h->last_error;

    req->seq = h->seq++;
    uint16_t c = cmd_csum(&req->command, req->length + 3);
    req->payload[req->length]     = (uint8_t)(c >> 8);
    req->payload[req->length + 1] = (uint8_t)(c & 0xff);
    req->length += 3 + 2;

    int r = it9507_usb_cmd_xfer(h, req, (size_t)req->length + 1, rep);
    if (r < 0)               { h->last_error = ESP_FAIL; return -1; }
    if (!rep)                return 0;
    if (r < 3 + 2)           { h->last_error = ESP_FAIL; return -1; }
    if (rep->length > r)     { h->last_error = ESP_FAIL; return -1; }

    uint16_t cc = cmd_csum(&rep->seq, rep->length - 2);
    rep->length -= 2 + 2;
    if (((rep->payload[rep->length] << 8) | rep->payload[rep->length + 1]) != cc) {
        h->last_error = ESP_FAIL; return -1;
    }
    if (rep->status) { h->last_error = ESP_FAIL; return -1; }
    return 0;
}

/* ===================== register access ================================== */
int it9507_wr_regs(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   const uint8_t *vals, uint8_t num)
{
    it9507_req_t req; it9507_reply_t rep;
    req.command = be16(cpu | IT9507_CMD_REG_DEMOD_WRITE);
    req.length  = 6 + num;
    req.payload[0] = num;
    req.payload[1] = (cpu != IT9507_PROC_LINK || reg >= 0x100) ? 2 : 1;
    req.payload[2] = reg >> 24;
    req.payload[3] = reg >> 16;
    req.payload[4] = reg >> 8;
    req.payload[5] = reg;
    memcpy(&req.payload[6], vals, num);
    return talk(h, &req, &rep);
}

int it9507_rd_regs(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   uint8_t *vals, uint8_t num)
{
    it9507_req_t req; it9507_reply_t rep;
    req.command = be16(cpu | IT9507_CMD_REG_DEMOD_READ);
    req.length  = 6;
    req.payload[0] = num;
    req.payload[1] = (cpu != IT9507_PROC_LINK || reg >= 0x100) ? 2 : 1;
    req.payload[2] = reg >> 24;
    req.payload[3] = reg >> 16;
    req.payload[4] = reg >> 8;
    req.payload[5] = reg;
    if (talk(h, &req, &rep) < 0) return -1;
    if (rep.length != num) return -1;
    memcpy(vals, rep.payload, num);
    return 0;
}

int it9507_wr_reg(it9507_handle_t h, uint16_t cpu, uint32_t reg, uint8_t val)
{
    return it9507_wr_regs(h, cpu, reg, &val, 1);
}

int it9507_wr_bits(it9507_handle_t h, uint16_t cpu, uint32_t reg,
                   uint8_t pos, uint8_t len, uint8_t val)
{
    uint8_t old;
    if (it9507_rd_regs(h, cpu, reg, &old, 1) < 0) return -1;
    old &= ~((0xff >> (8 - len)) << pos);
    return it9507_wr_reg(h, cpu, reg, old | (val << pos));
}

/* ===================== firmware upload ================================== */
int it9507_load_firmware(it9507_handle_t h)
{
    if (it9507_fw_codes_len == 0) {
        ESP_LOGE(TAG, "no firmware compiled in - see it9507_firmware.h");
        return -1;
    }
    if (it9507_fw_codes_len != 4560)
        ESP_LOGW(TAG, "firmware length %u, expected 4560", (unsigned)it9507_fw_codes_len);

    /* {chunk_size, count} pairs, terminated by {0,0}. */
    static const int chunks[][2] = {
        {48, 23}, {14, 1}, {48, 71}, {34, 1}, {0, 0}
    };
    it9507_wr_reg(h, IT9507_PROC_LINK, p_reg_lnk2ofdm_data_63_56, IT9507_IIC_SPEED);

    const uint8_t *p = it9507_fw_codes;
    for (int j = 0; chunks[j][0]; j++) {
        for (int i = 0; i < chunks[j][1]; i++) {
            it9507_req_t req; it9507_reply_t rep;
            req.command = be16(IT9507_PROC_LINK | IT9507_CMD_SCATTER_WRITE);
            req.length  = chunks[j][0];
            memcpy(req.payload, p, req.length);
            if (talk(h, &req, &rep) < 0) return h->last_error;
            p += chunks[j][0];
        }
    }
    it9507_req_t req; it9507_reply_t rep;
    req.command = be16(IT9507_PROC_LINK | IT9507_CMD_BOOT);
    req.length  = 0;
    int r = talk(h, &req, &rep);
    if (r == 0) vTaskDelay(pdMS_TO_TICKS(10));
    return r;
}

/* ===================== power / xmit ===================================== */
int it9507_power_up(it9507_handle_t h)
{
    it9507_wr_bits(h, IT9507_PROC_OFDM, p_reg_afe_mem0, 3, 1, 0);
    it9507_wr_reg (h, IT9507_PROC_OFDM, p_reg_dyn0_clk, 0);
    it9507_wr_bits(h, IT9507_PROC_LINK, p_reg_top_hostb_mpeg_ser_mode, 0, 1, 0);
    it9507_wr_bits(h, IT9507_PROC_LINK, p_reg_top_hostb_mpeg_par_mode, 0, 1, 0);
    return h->last_error;
}

int it9507_disable_xmit(it9507_handle_t h)
{
    it9507_wr_reg(h, IT9507_PROC_LINK, p_reg_tx_hw_disable, 1);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_fec_sw_rst, 1);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_afe_mem0, 1);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_afe_mem1, 0xFE);
    it9507_wr_reg(h, IT9507_PROC_LINK, IT9507_GPIO_O(IT9507_GPIOH2), 0); /* RF off */
    vTaskDelay(pdMS_TO_TICKS(100));
    return h->last_error;
}

int it9507_enable_xmit(it9507_handle_t h)
{
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_afe_mem0, 0);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_afe_mem1, 0xFC);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_fec_sw_rst, 0);
    it9507_wr_reg(h, IT9507_PROC_LINK, p_reg_tx_hw_disable, 0);
    it9507_wr_reg(h, IT9507_PROC_OFDM, reg_tx_fifo_overflow, 1);
    /* RF on -- GPIOH2 was configured as an output in chip_init; the UV-filter
     * path is selected in apply_modulation by frequency. */
    it9507_wr_reg(h, IT9507_PROC_LINK, IT9507_GPIO_O(IT9507_GPIOH2), 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    return h->last_error;
}

/* Recover the TS path after the modulator's tx_fifo_overflow latch trips (which
 * halts the FEC and freezes USB writes). Pulses the MPEG2-bridge resets to clear
 * the latch and reconnect FIFO->FEC, then re-arms the overflow flag. Brief glitch,
 * but the stream resumes instead of freezing permanently. Cheap: a few register
 * writes, no re-enumeration, no re-load of firmware/modulation. */
int it9507_ts_recover(it9507_handle_t h)
{
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF99D, 0, 1, 1);  /* mp2_sw_rst    = 1 */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A4, 0, 1, 1);  /* mp2if2_sw_rst = 1 */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF99D, 0, 1, 0);  /* mp2_sw_rst    = 0 */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A4, 0, 1, 0);  /* mp2if2_sw_rst = 0 */
    it9507_wr_reg (h, IT9507_PROC_OFDM, reg_tx_fifo_overflow, 1);  /* re-arm */
    return h->last_error;
}

/* ===================== tuner: LO frequency synthesis ==================== */
static unsigned it9507_lo_freq(unsigned rf_khz)
{
    static const unsigned char nvmap[] = { 48,32,24,16,12,8,6,4,2,2 };
    const unsigned fxtal = 2000, fdiv = 3, m_bdry = 3660;
    unsigned long tn = (unsigned long)fxtal * m_bdry;
    unsigned c[8];
    c[7]=tn/((unsigned long)fdiv*4);  c[6]=tn/((unsigned long)fdiv*6);
    c[5]=tn/((unsigned long)fdiv*8);  c[4]=tn/((unsigned long)fdiv*12);
    c[3]=tn/((unsigned long)fdiv*16); c[2]=tn/((unsigned long)fdiv*24);
    c[1]=tn/((unsigned long)fdiv*32); c[0]=0;

    unsigned nc;
    for (nc = 0; nc < 7 && rf_khz > c[nc+1]; nc++) ;
    unsigned nv = nvmap[nc];
    if (nc == 8) nc = 0;

    unsigned long tg = (unsigned long)rf_khz * nv * fdiv;
    unsigned long m  = tg / fxtal;
    if (tg - m*fxtal >= (fxtal >> 1)) m++;
    return (unsigned)((((nc) & 0x07) << 13) + (unsigned)m);
}

/* ===================== tuner: IQ calibration ============================ */
typedef struct { int frequency, dAmp, dPhi; } iq_t;

/* Full default IQ table from upstream (kHz, dAmp, dPhi). */
static const iq_t IQ_TABLE[] = {
 {50000,45,-1038},{60000,17,-664},{70000,3,-291},{80000,0,82},{90000,6,419},
 {100000,23,774},{110000,26,801},{120000,22,737},{130000,17,655},{140000,12,564},
 {150000,9,473},{160000,6,382},{170000,4,300},{180000,2,218},{190000,1,146},
 {200000,1,82},{210000,1,0},{220000,1,-55},{230000,1,-109},{240000,2,-155},
 {250000,2,-200},{260000,3,-237},{270000,3,-273},{280000,4,-300},{290000,4,-319},
 {300000,5,-337},{310000,5,-337},{320000,4,-346},{330000,4,-337},{340000,4,-328},
 {350000,4,-319},{360000,4,-300},{370000,3,-282},{380000,2,-273},{390000,2,-237},
 {400000,1,-228},{410000,1,-191},{420000,1,-173},{430000,0,-146},{440000,0,-118},
 {450000,0,-91},{460000,0,-73},{470000,0,-55},{480000,0,-36},{490000,0,-36},
 {500000,-1,-18},{510000,0,-9},{520000,0,0},{530000,-1,0},{540000,0,0},
 {550000,-1,-9},{560000,-1,18},{570000,-1,-9},{580000,0,9},{590000,0,18},
 {600000,0,0},{610000,0,18},{620000,0,0},{630000,-1,0},{640000,0,-9},
 {650000,0,9},{660000,0,-9},{670000,0,0},{680000,0,0},{690000,0,0},
 {700000,0,0},{710000,0,0},{720000,0,9},{730000,0,-9},{740000,0,9},
 {750000,0,-18},{760000,0,-27},{770000,2,-18},{780000,0,-18},{790000,2,-18},
 {800000,0,-18},{810000,0,-18},{820000,1,-18},{830000,0,-27},{840000,0,0},
 {850000,2,-27},{860000,0,-27},{870000,1,-27},{880000,0,-36},{890000,0,-27},
 {900000,0,0},{910000,1,-18},{920000,1,-27},{930000,0,-36},{940000,0,-27},
 {950000,0,-27},{1500000,0,-27},
};

static int iq_setIO(int THETA)
{
    int reverse, sign, x_in, x;
    if      (THETA < 4096)  { reverse=0; sign= 1; }
    else if (THETA < 8192)  { reverse=1; sign= 1; }
    else if (THETA < 12288) { reverse=0; sign=-1; }
    else                    { reverse=1; sign=-1; }
    x_in = THETA % 4096;
    x = reverse ? 4096 - x_in : x_in;

    static const int mi_t[16] = {25,25,24,24,23,22,20,19,17,15,13,11,8,6,4,1};
    static const int yi_t[16] = {0,400,801,1186,1564,1927,2277,2595,
                                 2894,3165,3405,3609,3788,3920,4015,4078};
    int sec = x >> 8; if (sec > 15) sec = 15;
    int mi = mi_t[sec]*16, xi = 256*sec, yi = yi_t[sec]*16;
    return sign * (mi * (x - xi) / 16 + yi);
}

static int iq_interp(int fIn, int *dAmp, int *dPhi)
{
    const int n = sizeof(IQ_TABLE)/sizeof(IQ_TABLE[0]);
    int i;
    for (i = 0; IQ_TABLE[i].frequency && i+1 < n; i++)
        if (fIn >= IQ_TABLE[i].frequency && fIn < IQ_TABLE[i+1].frequency) break;
    if (fIn < IQ_TABLE[i].frequency || fIn > IQ_TABLE[i+1].frequency) return -1;
    if (fIn != IQ_TABLE[i].frequency &&
        IQ_TABLE[i+1].frequency - IQ_TABLE[i].frequency > 100000) return -1;

    int df = IQ_TABLE[i+1].frequency - IQ_TABLE[i].frequency;
    *dAmp = IQ_TABLE[i].dAmp +
            (fIn - IQ_TABLE[i].frequency) * (IQ_TABLE[i+1].dAmp - IQ_TABLE[i].dAmp) / df;

    int dp = IQ_TABLE[i+1].dPhi - IQ_TABLE[i].dPhi;
    if (dp <= -8192) dp += 16384; else if (dp >= 8192) dp -= 16384;
    int phi = IQ_TABLE[i].dPhi + (fIn - IQ_TABLE[i].frequency) * dp / df;
    phi %= 16384; while (phi < 0) phi += 16384;
    *dPhi = phi;
    return 0;
}

static int it9507_set_iq_calibration(it9507_handle_t h)
{
    uint8_t v[6];
    int dAmp = 0, dPhi = 0;
    if (iq_interp(h->mod.frequency_khz, &dAmp, &dPhi) < 0) {
        v[0]=0x00; v[1]=0x02; v[2]=0x00; v[3]=0x00; v[4]=0x00; v[5]=0x02;
        it9507_wr_regs(h, IT9507_PROC_OFDM, 0, v, 6);   /* default */
        return -1;
    }
    int alpha = 512 - dAmp, beta = 512 + dAmp;
    int t1 = iq_setIO(dPhi);
    dPhi = (dPhi + 4096) % 16384;
    int t2 = iq_setIO(dPhi);
    const int SH = 512;
    int c1 = alpha*beta/16 * t2 / 4096 / SH;
    int c2 = -1*alpha*beta/16 * t1 / 4096 / SH;
    int c3 = alpha*alpha / SH;
    if (c1 < 0) c1 += 2048;
    if (c2 < 0) c2 += 2048;
    if (c3 < 0) c3 += 2048;
    v[0]=c1&0xff; v[1]=c1>>8; v[2]=c2&0xff; v[3]=c2>>8; v[4]=c3&0xff; v[5]=c3>>8;
    /* Keep these as the unity-gain defaults for output-gain scaling. */
    h->iq_c1 = c1; h->iq_c2 = c2; h->iq_c3 = c3; h->output_gain = 0;
    return it9507_wr_regs(h, IT9507_PROC_OFDM, p_reg_iqik_c1_7_0, v, 6);
}

/* ===================== tuner: bandwidth / frequency ==================== */
static int it9507_set_bandwidth(it9507_handle_t h)
{
    static const struct { int bw; uint8_t r[5]; } cfg[] = {
        {1000,{0x5e,0x03,0x01,0x03,0x00}}, {1500,{0x6e,0x03,0x01,0x03,0x00}},
        {2000,{0x5e,0x01,0x01,0x03,0x00}}, {2500,{0x66,0x01,0x01,0x03,0x00}},
        {3000,{0x6e,0x01,0x01,0x03,0x00}}, {4000,{0x5e,0x01,0x01,0x03,0x01}},
        {5000,{0x66,0x01,0x01,0x03,0x01}}, {6000,{0x6e,0x01,0x01,0x03,0x01}},
        {7000,{0x76,0x02,0x01,0x03,0x02}}, {8000,{0x1e,0x02,0x01,0x03,0x02}},
    };
    int k;
    for (k = 0; k < (int)(sizeof(cfg)/sizeof(cfg[0])); k++)
        if (cfg[k].bw == (int)h->mod.bandwidth_khz) break;
    if (k == (int)(sizeof(cfg)/sizeof(cfg[0]))) return -1;

    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD812, 3);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFBB6, cfg[k].r[0]);
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xFBB7, 0, 2, cfg[k].r[1]);
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xFBB8, 2, 1, cfg[k].r[2]);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xF741, cfg[k].r[4]);
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD814, cfg[k].r[3]);
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xFBB8, 2, 1, 0);
    return h->last_error;
}

static int it9507_set_frequency(it9507_handle_t h)
{
    unsigned code = it9507_lo_freq(h->mod.frequency_khz);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2A, code & 0xff);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2B, code >> 8);
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xFB2C, 2, 1, h->mod.frequency_khz > 950000 ? 1 : 0);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2D, 2);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2D, 1);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2D, 0);
    it9507_set_iq_calibration(h);
    return h->last_error;
}

/* ===================== TS input interface ============================== */
int it9507_set_ts_interface(it9507_handle_t h)
{
    /* Keep the EP6 endpoint setup that actually works on this USB host (it is the
     * register block bound to USB EP 0x06 here), and ADD ONLY the TS-input FIFO
     * block + FLOW CONTROL that the old version was missing. That missing flow
     * control is what let the FIFO overflow and latch at 13 Mbps.
     * NOTE: do NOT switch to the vendor's EP5 register names. On this host that
     * left EP 0x06 unenabled (ep6_rx_en / bit7 of 0xDD11 stayed 0), so the pipe
     * halted and every URB enqueue failed with ESP_ERR_INVALID_STATE. */
    it9507_wr_bits(h, IT9507_PROC_LINK, p_reg_dvbt_inten, reg_dvbt_inten_pos, 1, 1);
    it9507_wr_bits(h, IT9507_PROC_OFDM, p_reg_mpeg_full_speed, 0, 1, 0);
    it9507_wr_bits(h, IT9507_PROC_LINK, p_reg_dvbt_en, 0, 1, 1);
    it9507_wr_bits(h, IT9507_PROC_OFDM, p_mp2if_mpeg_ser_mode, 0, 1, 0);
    it9507_wr_bits(h, IT9507_PROC_OFDM, p_mp2if_mpeg_par_mode, 0, 1, 0);
    it9507_wr_bits(h, IT9507_PROC_LINK, 0xD91C, 0, 1, 0);  /* top_hostb ser (leak fix) */
    it9507_wr_bits(h, IT9507_PROC_LINK, 0xD91B, 0, 1, 0);  /* top_hostb par (leak fix) */

    /* ---- working EP6 endpoint (host OUT, USB EP 0x06) - unchanged ---- */
    it9507_wr_bits(h, IT9507_PROC_LINK, p_reg_ep6_rx_en, reg_ep6_rx_en_pos, 1, 1);
    it9507_wr_reg (h, IT9507_PROC_LINK, p_reg_ep6_max_pkt, IT9507_USB_MAX_PACKET / 4);
    {
        uint16_t fs_dw = IT9507_USB_FRAME_SIZE_DW;         /* 65424/4 = 16356 */
        it9507_wr_reg(h, IT9507_PROC_LINK, p_reg_ep6_cnt_num_7_0,     (uint8_t)(fs_dw & 0xFF));
        it9507_wr_reg(h, IT9507_PROC_LINK, p_reg_ep6_cnt_num_7_0 + 1, (uint8_t)(fs_dw >> 8));
    }

    /* ---- TS-input block + serial stream + FIFO FLOW CONTROL (the actual fix) ---- */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF99D, 0, 1, 1);  /* mp2_sw_rst   = 1 (reset MPEG2 bridge) */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A4, 0, 1, 1);  /* mp2if2_sw_rst = 1 (reset)  */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A3, 0, 1, 1);  /* mp2if2_en = 1              */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9CC, 0, 1, 0);  /* tsip_en = 0                */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9CD, 0, 1, 1);  /* tsis_en = 1 (serial)       */
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xF9D8, 0);        /* ts_in_src = 0              */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF99D, 0, 1, 0);  /* mp2_sw_rst   = 0 (release) - connects FIFO->FEC */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A4, 0, 1, 0);  /* mp2if2_sw_rst = 0 (release) */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9A5, 0, 1, 0);  /* mp2if2_half_psb = 0        */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xF9B5, 0, 1, 1);  /* mp2if_stop_en = 1  <<< FLOW CONTROL */
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD920, 0);        /* top_host_reverse = 0       */

    return h->last_error;
}

/* Program everything except the final carrier enable (left to it9507_start). */
int it9507_apply_modulation(it9507_handle_t h)
{
    it9507_set_bandwidth(h);
    it9507_set_frequency(h);
    /* UV filter select */
    it9507_wr_reg(h, IT9507_PROC_LINK, IT9507_GPIO_O(IT9507_GPIOH8),
                  h->mod.frequency_khz <= 300000 ? 0 : 1);
    it9507_disable_xmit(h);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF721, h->mod.constellation);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF723, h->mod.code_rate);
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_tps_gi, h->mod.guard_interval);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF726, h->mod.transmission_mode);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF7C1, 1 << (3 - h->mod.guard_interval));
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF7C6, 1);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF727, h->mod.cell_id >> 8);
    it9507_wr_reg(h, IT9507_PROC_OFDM, 0xF728, h->mod.cell_id);
    return h->last_error;
}

/* ===================== public API ====================================== */
/* ===================== chip init (from IT9507_initialize) ================
 * These writes were missing from this port. They configure the OFDM/analog
 * (RF) block after the firmware boots. Without them the digital modulator runs
 * -- TS drains at the channel rate -- but the analog path is left unconfigured,
 * so only LO leakage appears at the output and no OFDM is radiated.
 * Addresses/values are taken verbatim from the ITE SDK's IT9507_initialize().
 */
static int it9507_chip_init(it9507_handle_t h)
{
    /* UART -> GPIOH4 */
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD924, 0);
    /* I2C master clock 100k (re-asserted after firmware boot) */
    it9507_wr_reg (h, IT9507_PROC_LINK, p_reg_lnk2ofdm_data_63_56, IT9507_IIC_SPEED);

    /* --- OFDM / analog front-end configuration --- */
    it9507_wr_bits(h, IT9507_PROC_OFDM, 0xFB26, 7, 1, 1);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFBBD, 0xE0);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xF99A, 0x00);

    /* --- pad driving power (0xD830/31/32) --- */
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD830, 0);
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD831, 0);
    it9507_wr_reg (h, IT9507_PROC_LINK, 0xD832, 0);

    /* --- RF block: these two are the ones most likely to gate the output --- */
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFB2E, 0x11);
    it9507_wr_reg (h, IT9507_PROC_OFDM, 0xFBB3, 0x98);

    /* --- Board GPIO mapping (EagleUser_setSystemConfig + _Initialization) ---
     * THE missing piece: the RF-enable GPIO (GPIOH2) must be configured as an
     * OUTPUT (en=1, on=1) before driving it, or the later "RF on" write is a
     * silent no-op -- every chip register reads back correct yet nothing is
     * radiated except LO leakage. Same for the UHF/VHF filter select (GPIOH8).
     * HwDesc[] maps rfEnable=GPIOH2 on every board variant, so this is safe. */
    it9507_wr_reg (h, IT9507_PROC_LINK, IT9507_GPIO_EN(IT9507_GPIOH2), 1);  /* output enable */
    it9507_wr_reg (h, IT9507_PROC_LINK, IT9507_GPIO_ON(IT9507_GPIOH2), 1);  /* on enable     */
    it9507_wr_reg (h, IT9507_PROC_LINK, IT9507_GPIO_O (IT9507_GPIOH2), 0);  /* RF off (setup)*/
    it9507_wr_reg (h, IT9507_PROC_LINK, IT9507_GPIO_EN(IT9507_GPIOH8), 1);
    it9507_wr_reg (h, IT9507_PROC_LINK, IT9507_GPIO_ON(IT9507_GPIOH8), 1);

    return h->last_error;
}

esp_err_t it9507_install(const it9507_config_t *cfg, it9507_handle_t *out)
{
    ESP_RETURN_ON_FALSE(cfg && out, ESP_ERR_INVALID_ARG, TAG, "args");
    it9507_handle_t h = calloc(1, sizeof(*h));
    ESP_RETURN_ON_FALSE(h, ESP_ERR_NO_MEM, TAG, "no mem");
    h->seq = 0; h->last_error = 0;

    esp_err_t err = it9507_usb_start(h, cfg);
    if (err != ESP_OK) { free(h); return err; }

    /* Probe (optional) then upload firmware and bring the core up. */
    if (it9507_load_firmware(h) < 0) { err = ESP_FAIL; goto fail; }
    if (it9507_chip_init(h)     < 0) { err = ESP_FAIL; goto fail; }
    if (it9507_power_up(h)      < 0) { err = ESP_FAIL; goto fail; }
    if (it9507_set_ts_interface(h)<0){ err = ESP_FAIL; goto fail; }

    uint32_t link = 0, ofdm = 0;
    it9507_get_fw_versions(h, &link, &ofdm);
    ESP_LOGI(TAG, "fw link=0x%08x ofdm=0x%08x", (unsigned)link, (unsigned)ofdm);

    *out = h;
    return ESP_OK;
fail:
    it9507_usb_stop(h);
    free(h);
    return err;
}

esp_err_t it9507_set_output_gain(it9507_handle_t h, int gain_db)
{
    ESP_RETURN_ON_FALSE(h, ESP_ERR_INVALID_ARG, TAG, "args");

    /* Output level is controlled by scaling the IQ-cal coefficients c1/c2/c3
     * (ported from IT9507_adjustOutputGain). +1 dB multiplies amplitude by 1.122,
     * -1 dB by 0.891, clamped so no coefficient leaves the 10-bit range. This
     * scales the OFDM but NOT the DC/carrier leakage, so it also improves the
     * OFDM-to-carrier ratio. Needs the IQ defaults set during modulation. */
    int c1d = h->iq_c1, c2d = h->iq_c2, c3d = h->iq_c3;
    if (c1d == 0 && c2d == 0 && c3d == 0) {
        ESP_LOGW(TAG, "output gain: IQ defaults unset; call set_modulation first");
        return ESP_ERR_INVALID_STATE;
    }
    if (c1d > 1023) c1d -= 2048;
    if (c2d > 1023) c2d -= 2048;
    if (c3d > 1023) c3d -= 2048;
    #define IABS(x) ((x) < 0 ? -(x) : (x))

    long mm1 = c1d ? (10000L * 1023 / IABS(c1d)) : 0x7fffffffL;
    long mm2 = c2d ? (10000L * 1023 / IABS(c2d)) : 0x7fffffffL;
    long mm3 = c3d ? (10000L * 1023 / IABS(c3d)) : 0x7fffffffL;
    long amp_mul_max = mm1;
    if (mm2 < amp_mul_max) amp_mul_max = mm2;
    if (mm3 < amp_mul_max) amp_mul_max = mm3;

    int  gain_X10 = gain_db * 10;
    int  c1 = c1d, c2 = c2d, c3 = c3d, i = 0;
    long amp_mul = 10000;
    bool overflow = false;

    if (gain_X10 > 0) {
        for (i = 0; i < gain_X10; i += 10) {
            if (amp_mul_max > amp_mul) {
                amp_mul = amp_mul * 11220 / 10000;
                c1 = (int)((long)c1d * amp_mul / 10000);
                c2 = (int)((long)c2d * amp_mul / 10000);
                c3 = (int)((long)c3d * amp_mul / 10000);
            }
            if (c1 > 0x3ff) { c1 = 0x3ff; overflow = true; }
            if (c3 > 0x3ff) { c3 = 0x3ff; overflow = true; }
            if (overflow) break;
        }
    } else if (gain_X10 < 0) {
        for (i = 0; i > gain_X10; i -= 10) {
            if (amp_mul_max > amp_mul) {
                amp_mul = amp_mul * 8910 / 10000;
                c1 = (int)((long)c1d * amp_mul / 10000);
                c2 = (int)((long)c2d * amp_mul / 10000);
                c3 = (int)((long)c3d * amp_mul / 10000);
            }
            if (c1 == 0 || c3 == 0) overflow = true;
            if (overflow) break;
        }
    }
    #undef IABS

    if (c1 < 0) c1 += 2048;
    if (c2 < 0) c2 += 2048;
    if (c3 < 0) c3 += 2048;
    c1 %= 2048; c2 %= 2048; c3 %= 2048;
    h->output_gain = i / 10;

    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c1_7_0,     (uint8_t)(c1 & 0xff));
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c1_7_0 + 1, (uint8_t)(c1 >> 8));
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c2_7_0,     (uint8_t)(c2 & 0xff));
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c2_7_0 + 1, (uint8_t)(c2 >> 8));
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c3_7_0,     (uint8_t)(c3 & 0xff));
    it9507_wr_reg(h, IT9507_PROC_OFDM, p_reg_iqik_c3_7_0 + 1, (uint8_t)(c3 >> 8));

    ESP_LOGI(TAG, "output gain: requested %d dB, applied %d dB (c1=%d c2=%d c3=%d)",
             gain_db, h->output_gain, c1, c2, c3);
    return h->last_error ? ESP_FAIL : ESP_OK;
}

esp_err_t it9507_set_modulation(it9507_handle_t h, const it9507_modulation_t *mod)
{
    ESP_RETURN_ON_FALSE(h && mod, ESP_ERR_INVALID_ARG, TAG, "args");
    h->mod = *mod;
    h->last_error = 0;
    if (it9507_apply_modulation(h) != 0) return ESP_FAIL;
    it9507_set_output_gain(h, mod->gain_db);   /* no-op at 0; wire real gain once ported */
    return ESP_OK;
}

esp_err_t it9507_start(it9507_handle_t h)
{
    ESP_RETURN_ON_FALSE(h, ESP_ERR_INVALID_ARG, TAG, "args");
    h->last_error = 0;
    return it9507_enable_xmit(h) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t it9507_stop(it9507_handle_t h)
{
    ESP_RETURN_ON_FALSE(h, ESP_ERR_INVALID_ARG, TAG, "args");
    h->last_error = 0;
    return it9507_disable_xmit(h) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t it9507_write_ts(it9507_handle_t h, const uint8_t *ts, size_t len,
                          TickType_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(h && ts, ESP_ERR_INVALID_ARG, TAG, "args");
    return it9507_usb_ts_submit(h, ts, len, ticks_to_wait);
}

esp_err_t it9507_get_fw_versions(it9507_handle_t h, uint32_t *link, uint32_t *ofdm)
{
    ESP_RETURN_ON_FALSE(h, ESP_ERR_INVALID_ARG, TAG, "args");
    for (int pass = 0; pass < 2; pass++) {
        it9507_req_t req; it9507_reply_t rep;
        uint16_t proc = pass ? IT9507_PROC_OFDM : IT9507_PROC_LINK;
        req.command = be16(proc | IT9507_CMD_QUERYINFO);
        req.length  = 1;
        req.payload[0] = 1;
        uint32_t v = 0;
        if (talk(h, &req, &rep) == 0 && rep.length == 4)
            v = (rep.payload[0]<<24)|(rep.payload[1]<<16)|(rep.payload[2]<<8)|rep.payload[3];
        if (pass == 0 && link) *link = v;
        if (pass == 1 && ofdm) *ofdm = v;
    }
    return ESP_OK;
}

esp_err_t it9507_dump_status(it9507_handle_t h)
{
    ESP_RETURN_ON_FALSE(h, ESP_ERR_INVALID_ARG, TAG, "args");
    uint8_t ovf=0xFF, txdis=0xFF, dvbt=0xFF;
    uint8_t cons=0xFF, cr=0xFF, gi=0xFF, mode=0xFF;
    uint8_t bwA=0xFF, bwB=0xFF, lo0=0xFF, lo1=0xFF;
    /* Read back with the SAME processor prefix each value was written with. */
    it9507_rd_regs(h, IT9507_PROC_OFDM, reg_tx_fifo_overflow, &ovf,  1);
    it9507_rd_regs(h, IT9507_PROC_LINK, p_reg_tx_hw_disable,  &txdis,1);
    it9507_rd_regs(h, IT9507_PROC_LINK, p_reg_dvbt_en,        &dvbt, 1);
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xF721,               &cons, 1);
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xF723,               &cr,   1);
    it9507_rd_regs(h, IT9507_PROC_OFDM, p_reg_tps_gi,         &gi,   1);
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xF726,               &mode, 1);
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xFBB6,               &bwA,  1); /* bandwidth coef  */
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xF741,               &bwB,  1); /* bandwidth mode  */
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xFB2A,               &lo0,  1); /* LO synth code lo*/
    it9507_rd_regs(h, IT9507_PROC_OFDM, 0xFB2B,               &lo1,  1); /* LO synth code hi*/

    ESP_LOGI(TAG, "IT9507 status: fifo_ovf=%u tx_hw_disable=%u dvbt_en=%u", ovf, txdis, dvbt);
    ESP_LOGI(TAG, "  modulation : cons=%u cr=%u gi=%u mode=%u", cons, cr, gi, mode);
    /* For 2 MHz the bandwidth table programs 0xFBB6=0x5e, 0xF741=0x00. LO code is
     * nonzero if the frequency synth was programmed. All-0xFF = read failed. */
    ESP_LOGI(TAG, "  carrier    : bw[FBB6]=0x%02x bw[F741]=0x%02x LOcode=0x%02x%02x",
             bwA, bwB, lo1, lo0);
    return h->last_error ? ESP_FAIL : ESP_OK;
}

esp_err_t it9507_uninstall(it9507_handle_t h)
{
    if (!h) return ESP_ERR_INVALID_ARG;
    it9507_disable_xmit(h);
    it9507_usb_stop(h);
    if (h->evt_group)    vEventGroupDelete(h->evt_group);
    if (h->cmd_mtx)      vSemaphoreDelete(h->cmd_mtx);
    if (h->cmd_out_done) vSemaphoreDelete(h->cmd_out_done);
    if (h->cmd_in_done)  vSemaphoreDelete(h->cmd_in_done);
    if (h->ts_free)      vQueueDelete(h->ts_free);
    free(h);
    return ESP_OK;
}
