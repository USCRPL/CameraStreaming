/* User-mode driver for the UT-120 (ITE IT9135/IT9137) over Microsoft's WinUSB.
 * Wraps ITE's demodulator API (itesdk/) so the engine never touches SDK types. */
#ifndef UT120_HW_H
#define UT120_HW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[128];
    char instance[256];
    char service[64];   /* kernel service bound to the device, "WinUSB" when usable */
    int ready;
} hw_device;

typedef struct {
    int chips;
    int locked[2], present[2];
    int quality[2], strength[2];  /* 0-100 from the demodulator */
    long dbm[2];
    int snr_db[2];
    double ber[2];                /* post-Viterbi bit error rate */
    unsigned aborts[2];           /* Reed-Solomon uncorrectable packets in the last window */
} hw_stats;

typedef void (*hw_ts_cb)(const unsigned char* data, unsigned long len, void* ctx);

int hw_list(hw_device* out, int max);
/* Opens, loads firmware and powers up. Returns 0, or writes a message to err. */
int hw_open(int index, char* err, int errlen);
void hw_close(void);
/* Short JSON object describing the chip(s), valid after hw_open. */
const char* hw_info(void);
unsigned long hw_tune(unsigned long khz, unsigned bw_mhz);
unsigned long hw_get_stats(hw_stats* s);
/* Blocks, calling cb for each TS buffer, until hw_stream_stop() or an error. */
int hw_stream(hw_ts_cb cb, void* ctx);
void hw_stream_stop(void);

#ifdef __cplusplus
}
#endif
#endif
