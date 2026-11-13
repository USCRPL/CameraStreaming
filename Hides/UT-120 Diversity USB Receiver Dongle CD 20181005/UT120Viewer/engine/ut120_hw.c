/* WinUSB transport + device bring-up for the UT-120, built on ITE's IT913x API.
 *
 * The bring-up order mirrors ITE's own Linux driver (it913x-drv.c: Device_init,
 * DRV_GetEEPROMConfig, DRV_Initialize, DRV_ApCtrl), which is the sequence ITE
 * tested on this hardware. Only the bus underneath is new. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <winusb.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "itesdk/demodulator.h"
#include "itesdk/demodulatorextend.h"
#include "itesdk/register.h"
#include "itesdk/variable.h"
#include "ut120_hw.h"

#define EP_CMD_OUT 0x02
#define EP_CMD_IN  0x81
#define EP_TS      0x84
#define TS_FRAME   65424          /* 348 TS packets: the frame size ITE's driver uses */
#define TS_QUEUE   8

#define EEPROM_FLB_OFS     8
#define EEPROM_IRMODE      (OVA_EEPROM_CFG + EEPROM_FLB_OFS + 0x10)
#define EEPROM_TSMODE      (OVA_EEPROM_CFG + EEPROM_FLB_OFS + 0x28 + 1)
#define EEPROM_TUNERID     (OVA_EEPROM_CFG + EEPROM_FLB_OFS + 0x30 + 4)
#define EEPROM_BOARDID     (OVA_EEPROM_CFG + EEPROM_FLB_OFS + 0x30 + 7)

static const char* kIds[] = {"VID_048D&PID_9135", "VID_048D&PID_9005", "VID_048D&PID_9006"};

static HANDLE g_file = INVALID_HANDLE_VALUE;
static WINUSB_INTERFACE_HANDLE g_usb;
static CRITICAL_SECTION g_bus;   /* one command/response at a time on EP 0x02/0x81 */
static CRITICAL_SECTION g_api;   /* one API sequence at a time (tune vs. stats poll) */
static int g_locks_ready;
static volatile LONG g_stop;
static IT9130 g_dev;
static Byte g_chip_version;
static Word g_tuner_id;
static Byte g_board_id;
static char g_info[256];
static int g_trace = -1;

static void trace(const char* dir, Dword n, const Byte* b, BOOL ok, DWORD err, DWORD ms) {
    if (g_trace < 0) g_trace = GetEnvironmentVariableA("UT120_TRACE", NULL, 0) > 0;
    if (!g_trace) return;
    fprintf(stderr, "{\"event\":\"trace\",\"dir\":\"%s\",\"len\":%lu,\"ok\":%d,\"err\":%lu,\"ms\":%lu,\"head\":\"", dir,
            (unsigned long)n, ok, err, ms);
    for (Dword i = 0; i < n && i < 12; ++i) fprintf(stderr, "%02X", b[i]);
    fprintf(stderr, "\"}\n");
}

static void init_locks(void) {
    if (!g_locks_ready) {
        InitializeCriticalSection(&g_bus);
        InitializeCriticalSection(&g_api);
        g_locks_ready = 1;
    }
}

void ut120_sleep_ms(unsigned long ms) { if (ms) Sleep(ms); }

/* ------------------------------------------------------------ SDK hooks */

Dword User_memoryCopy(Demodulator* d, void* dst, void* src, Dword n) { (void)d; memcpy(dst, src, n); return 0; }
Dword User_delay(Demodulator* d, Dword ms) { (void)d; ut120_sleep_ms(ms); return 0; }
Dword User_enterCriticalSection(Demodulator* d) { (void)d; EnterCriticalSection(&g_bus); return 0; }
Dword User_leaveCriticalSection(Demodulator* d) { (void)d; LeaveCriticalSection(&g_bus); return 0; }
Dword User_mpegConfig(Demodulator* d) { (void)d; return 0; }
Dword User_busTx(Demodulator* d, Dword n, Byte* b) { (void)d; (void)n; (void)b; return 0; }
Dword User_busRx(Demodulator* d, Dword n, Byte* b) { (void)d; (void)n; (void)b; return 0; }
Dword User_busRxData(Demodulator* d, Dword n, Byte* b) { (void)d; (void)n; (void)b; return 0; }

Dword Usb2_getDriver(Demodulator* d, Handle* h) { (void)d; (void)h; return 0; }
Dword Usb2_exitDriver(Demodulator* d) { (void)d; return 0; }
Dword Usb2_readDataBus(Demodulator* d, Dword n, Byte* b) { (void)d; (void)n; (void)b; return 0; }

Dword Usb2_writeControlBus(Demodulator* d, Dword n, Byte* b) {
    ULONG done = 0;
    DWORD t = GetTickCount();
    (void)d;
    BOOL ok = WinUsb_WritePipe(g_usb, EP_CMD_OUT, b, n, &done, NULL) && done == n;
    DWORD err = ok ? 0 : GetLastError();
    trace("out", n, b, ok, err, GetTickCount() - t);
    return ok ? 0 : 0x05000000 | (err & 0xFFFF);
}

Dword Usb2_readControlBus(Demodulator* d, Dword n, Byte* b) {
    ULONG done = 0;
    DWORD t = GetTickCount();
    (void)d;
    BOOL ok = WinUsb_ReadPipe(g_usb, EP_CMD_IN, b, n, &done, NULL);
    DWORD err = ok ? 0 : GetLastError();
    trace("in", done, b, ok, err, GetTickCount() - t);
    return ok ? 0 : 0x05010000 | (err & 0xFFFF);
}

/* ------------------------------------------------------------ discovery */

static int matches(const char* hwids) {
    for (const char* p = hwids; *p; p += strlen(p) + 1)
        for (unsigned i = 0; i < sizeof kIds / sizeof *kIds; ++i)
            if (strstr(_strupr((char*)p), kIds[i])) return 1;
    return 0;
}

int hw_list(hw_device* out, int max) {
    HDEVINFO set = SetupDiGetClassDevsA(NULL, "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) return 0;
    int n = 0;
    SP_DEVINFO_DATA info = {sizeof info};
    for (DWORD i = 0; n < max && SetupDiEnumDeviceInfo(set, i, &info); ++i) {
        char ids[1024] = {0};
        if (!SetupDiGetDeviceRegistryPropertyA(set, &info, SPDRP_HARDWAREID, NULL, (BYTE*)ids, sizeof ids - 2, NULL) || !matches(ids))
            continue;
        hw_device* d = &out[n];
        memset(d, 0, sizeof *d);
        SetupDiGetDeviceRegistryPropertyA(set, &info, SPDRP_SERVICE, NULL, (BYTE*)d->service, sizeof d->service, NULL);
        if (!_stricmp(d->service, "usbccgp")) continue;  /* composite parent; the interface child is listed separately */
        if (!SetupDiGetDeviceRegistryPropertyA(set, &info, SPDRP_FRIENDLYNAME, NULL, (BYTE*)d->name, sizeof d->name, NULL))
            SetupDiGetDeviceRegistryPropertyA(set, &info, SPDRP_DEVICEDESC, NULL, (BYTE*)d->name, sizeof d->name, NULL);
        SetupDiGetDeviceInstanceIdA(set, &info, d->instance, sizeof d->instance, NULL);
        d->ready = !_stricmp(d->service, "WinUSB");
        ++n;
    }
    SetupDiDestroyDeviceInfoList(set);
    return n;
}

static int open_path(const char* instance, char* path, size_t pathlen) {
    HDEVINFO set = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return 0;
    int found = 0;
    SP_DEVICE_INTERFACE_DATA ifd = {sizeof ifd};
    for (DWORD i = 0; !found && SetupDiEnumDeviceInterfaces(set, NULL, &GUID_DEVINTERFACE_USB_DEVICE, i, &ifd); ++i) {
        char buf[1024];
        SP_DEVICE_INTERFACE_DETAIL_DATA_A* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)buf;
        detail->cbSize = sizeof *detail;
        SP_DEVINFO_DATA info = {sizeof info};
        char id[256];
        if (SetupDiGetDeviceInterfaceDetailA(set, &ifd, detail, sizeof buf, NULL, &info) &&
            SetupDiGetDeviceInstanceIdA(set, &info, id, sizeof id, NULL) && !_stricmp(id, instance)) {
            snprintf(path, pathlen, "%s", detail->DevicePath);
            found = 1;
        }
    }
    SetupDiDestroyDeviceInfoList(set);
    return found;
}

/* ------------------------------------------------------------ bring-up */

#define DEV ((Demodulator*)&g_dev)
#define TRY(call) do { Dword e_ = (call); if (e_) { snprintf(err, errlen, "%s failed (0x%08lX)", #call, (unsigned long)e_); goto fail; } } while (0)

static Dword set_bus_tuner(Word tuner) {
    Dword version = 0;
    Dword e = Demodulator_setBusTuner(DEV, Bus_USB, tuner);
    if (e) return e;
    e = Demodulator_getFirmwareVersion(DEV, Processor_LINK, &version);
    g_dev.booted = version != 0 ? True : False;
    return e;
}

static Dword read_config(void) {
    Byte var[2] = {0}, reg = 0;
    Dword e = Demodulator_readRegister(DEV, 0, Processor_LINK, chip_version_7_0, &g_chip_version);
    if (!e) e = Demodulator_readRegisters(DEV, 0, Processor_LINK, chip_version_7_0 + 1, 2, var);
    if (e) return e;
    Word chip_type = (Word)(var[1] << 8 | var[0]);
    int omega2 = chip_type == 0x9135 && g_chip_version == 2;
    g_chip_version = omega2 ? 2 : 1;
    e = Demodulator_readRegisters(DEV, 0, Processor_LINK, omega2 ? 0x461d : 0x461b, 1, &reg);
    if (e) return e;

    g_dev.chipNumber = 1;
    g_dev.architecture = Architecture_DCA;
    g_tuner_id = Tuner_Afatech_OMEGA;
    if (reg == 0) return 0;  /* EEPROM not valid: single chip defaults */

    Demodulator_readRegister(DEV, 0, Processor_LINK, EEPROM_BOARDID, &g_board_id);
    e = Demodulator_readRegisters(DEV, 0, Processor_LINK, EEPROM_TSMODE, 1, &reg);
    if (e) return e;
    if (reg == 1 || reg == 2) {
        g_dev.chipNumber = 2;   /* diversity: two demodulators combined into one TS */
    } else if (reg == 3) {
        g_dev.chipNumber = 2;   /* dual TS; we still read only the first stream */
        g_dev.architecture = Architecture_PIP;
    }
    e = Demodulator_readRegisters(DEV, 0, Processor_LINK, EEPROM_TUNERID, 1, &reg);
    if (e) return e;
    switch (reg) {
    case 0x51: case 0x52: case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:
        g_tuner_id = reg;
        break;
    default:
        break;
    }
    return 0;
}

static Dword bits(Byte chip, Dword reg, Byte pos, Byte len, Byte v) {
    return Demodulator_writeRegisterBits(DEV, chip, Processor_LINK, reg, pos, len, v);
}

static void nim_reset(void) {           /* pulse GPIOH1 to reset the second chip */
    bits(0, p_reg_top_gpioh1_en, reg_top_gpioh1_en_pos, reg_top_gpioh1_en_len, 1);
    bits(0, p_reg_top_gpioh1_on, reg_top_gpioh1_on_pos, reg_top_gpioh1_on_len, 1);
    bits(0, p_reg_top_gpioh1_o, reg_top_gpioh1_o_pos, reg_top_gpioh1_o_len, 0);
    Sleep(50);
    bits(0, p_reg_top_gpioh1_o, reg_top_gpioh1_o_pos, reg_top_gpioh1_o_len, 1);
}

static void nim_suspend(int on) {        /* GPIOH5 gates the second chip */
    if (g_dev.chipNumber < 2) return;
    bits(0, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, on ? 1 : 0);
    Sleep(on ? 50 : 100);
}

static void tuner_wakeup(void) {
    bits(0, p_reg_top_gpioh7_o, reg_top_gpioh7_o_pos, reg_top_gpioh7_o_len, 1);
}

static Dword power(Byte chip, int on) {
    Dword e;
    if (on) {
        if (chip == 1) {
            nim_suspend(0);
        } else {
            Dword v;
            for (int i = 0; i < 5; ++i) { Demodulator_getFirmwareVersion(DEV, Processor_LINK, &v); Sleep(1); }
        }
        e = Demodulator_controlTunerPowerSaving(DEV, chip, 1);
        if (!e) e = Demodulator_controlPowerSaving(DEV, chip, 1);
    } else {
        if (chip == 0 && g_dev.chipNumber == 2) nim_suspend(1);
        e = Demodulator_controlPowerSaving(DEV, chip, 0);
        if (!e) e = Demodulator_controlTunerPowerSaving(DEV, chip, 0);
    }
    return e;
}

int hw_open(int index, char* err, int errlen) {
    init_locks();
    hw_close();
    hw_device devs[8];
    int n = hw_list(devs, 8);
    if (index < 0 || index >= n) { snprintf(err, errlen, "UT-120 not found. Is it plugged in?"); return -1; }
    if (!devs[index].ready) {
        snprintf(err, errlen, "UT-120 is using driver '%s', not WinUSB. See README: Switch the dongle to WinUSB.",
                 devs[index].service[0] ? devs[index].service : "none");
        return -1;
    }
    char path[1024];
    if (!open_path(devs[index].instance, path, sizeof path)) { snprintf(err, errlen, "No USB interface for %s", devs[index].instance); return -1; }
    g_file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (g_file == INVALID_HANDLE_VALUE) {
        snprintf(err, errlen, "Cannot open the dongle (error %lu). Is another program using it?", GetLastError());
        return -1;
    }
    if (!WinUsb_Initialize(g_file, &g_usb)) {
        snprintf(err, errlen, "WinUsb_Initialize failed (error %lu)", GetLastError());
        CloseHandle(g_file);
        g_file = INVALID_HANDLE_VALUE;
        return -1;
    }
    ULONG timeout = 3000;
    WinUsb_SetPipePolicy(g_usb, EP_CMD_OUT, PIPE_TRANSFER_TIMEOUT, sizeof timeout, &timeout);
    WinUsb_SetPipePolicy(g_usb, EP_CMD_IN, PIPE_TRANSFER_TIMEOUT, sizeof timeout, &timeout);
    WinUsb_FlushPipe(g_usb, EP_CMD_IN);
    WinUsb_FlushPipe(g_usb, EP_TS);

    EnterCriticalSection(&g_api);
    memset(&g_dev, 0, sizeof g_dev);
    g_dev.userData = &g_dev;
    g_dev.driver = &g_dev;
    g_dev.chipNumber = 1;
    g_dev.frequency[0] = 666000;
    g_dev.bandwidth[0] = 8000;
    g_dev.user_usb20_frame_size_dw = TS_FRAME / 4;

    for (int i = 0; i < 5; ++i) { set_bus_tuner(Tuner_Afatech_OMEGA); Sleep(1); }
    TRY(set_bus_tuner(Tuner_Afatech_OMEGA));
    TRY(read_config());
    TRY(set_bus_tuner(g_tuner_id));

    if (g_dev.chipNumber == 2 && !g_dev.booted) {
        nim_reset();
    } else if (g_dev.chipNumber == 2) {
        nim_suspend(0);
        tuner_wakeup();
    } else if (g_dev.booted) {
        tuner_wakeup();
    }
    if (g_dev.booted)
        for (Byte c = 0; c < g_dev.chipNumber; ++c) power(c, 1);

    TRY(Demodulator_initialize(DEV, g_dev.chipNumber, 8000, StreamType_DVBT_DATAGRAM, g_dev.architecture));

    for (Byte c = 0; c < g_dev.chipNumber; ++c) {  /* ITE patch for 0-byte USB packets */
        Byte r = 0;
        Demodulator_readRegister(DEV, c, Processor_LINK, 0xdd8d, &r);
        Demodulator_writeRegister(DEV, c, Processor_LINK, 0xdd8d, (Byte)(r | 0x08));
    }
    if (g_dev.chipNumber == 2) {
        bits(0, p_reg_top_gpioh5_en, reg_top_gpioh5_en_pos, reg_top_gpioh5_en_len, 1);
        bits(0, p_reg_top_gpioh5_on, reg_top_gpioh5_on_pos, reg_top_gpioh5_on_len, 1);
        bits(0, p_reg_top_gpioh5_o, reg_top_gpioh5_o_pos, reg_top_gpioh5_o_len, 0);
        Sleep(10);
        bits(1, p_reg_top_pwrdw, reg_top_pwrdw_pos, reg_top_pwrdw_len, 1);
        bits(1, p_reg_top_pwrdw_hwen, reg_top_pwrdw_hwen_pos, reg_top_pwrdw_hwen_len, 1);
    }
    /* Same as ITE's driver: park every chip, then power up for use. */
    for (int c = g_dev.chipNumber - 1; c >= 0; --c) power((Byte)c, 0);
    for (Byte c = 0; c < g_dev.chipNumber; ++c) TRY(power(c, 1));
    for (Byte c = 0; c < g_dev.chipNumber; ++c) Demodulator_controlPidFilter(DEV, c, 0);

    {
        Dword link = 0, ofdm = 0;
        Demodulator_getFirmwareVersion(DEV, Processor_LINK, &link);
        Demodulator_getFirmwareVersion(DEV, Processor_OFDM, &ofdm);
        snprintf(g_info, sizeof g_info,
                 "{\"chips\":%d,\"omega\":%d,\"tuner_id\":\"0x%02X\",\"board_id\":%d,\"architecture\":\"%s\","
                 "\"fw_link\":\"%08lX\",\"fw_ofdm\":\"%08lX\"}",
                 g_dev.chipNumber, g_chip_version, g_tuner_id, g_board_id,
                 g_dev.architecture == Architecture_PIP ? "dual-ts" : (g_dev.chipNumber == 2 ? "diversity" : "single"),
                 (unsigned long)link, (unsigned long)ofdm);
    }
    LeaveCriticalSection(&g_api);
    return 0;

fail:
    LeaveCriticalSection(&g_api);
    hw_close();
    return -1;
}

void hw_close(void) {
    if (g_file == INVALID_HANDLE_VALUE) return;
    hw_stream_stop();
    WinUsb_Free(g_usb);
    CloseHandle(g_file);
    g_file = INVALID_HANDLE_VALUE;
    g_usb = NULL;
}

const char* hw_info(void) { return g_info; }

unsigned long hw_tune(unsigned long khz, unsigned bw_mhz) {
    if (g_file == INVALID_HANDLE_VALUE) return 0xFFFFFFFF;
    EnterCriticalSection(&g_api);
    Dword e = 0;
    /* In diversity mode both demodulators must sit on the same channel to be combined. */
    for (Byte c = 0; c < g_dev.chipNumber && !e; ++c)
        e = Demodulator_acquireChannel(DEV, c, (Word)(bw_mhz * 1000), khz);
    /* No WinUsb_FlushPipe(EP_TS) here: it blocks while hw_stream has reads queued.
     * The viewer discards data from the previous channel itself. */
    LeaveCriticalSection(&g_api);
    return e;
}

unsigned long hw_get_stats(hw_stats* s) {
    memset(s, 0, sizeof *s);
    if (g_file == INVALID_HANDLE_VALUE) return 0xFFFFFFFF;
    EnterCriticalSection(&g_api);
    Dword first = 0;
    s->chips = g_dev.chipNumber;
    for (Byte c = 0; c < g_dev.chipNumber; ++c) {
        Statistic st;
        ChannelStatistic ch;
        Long dbm = 0;
        Byte snr = 0;
        memset(&st, 0, sizeof st);
        memset(&ch, 0, sizeof ch);
        Dword e = Demodulator_getStatistic(DEV, c, &st);
        if (e && !first) first = e;
        s->present[c] = st.signalPresented;
        s->locked[c] = st.signalLocked;
        s->quality[c] = st.signalQuality;
        s->strength[c] = st.signalStrength;
        if (!Demodulator_getSignalStrengthDbm(DEV, c, &dbm)) s->dbm[c] = dbm;
        if (!Demodulator_getSNR(DEV, c, &snr)) s->snr_db[c] = snr;
        if (st.signalLocked && !Demodulator_getChannelStatistic(DEV, c, &ch)) {
            s->aborts[c] = ch.abortCount;
            s->ber[c] = ch.postVitBitCount ? (double)ch.postVitErrorCount / ch.postVitBitCount : 0.0;
        }
    }
    LeaveCriticalSection(&g_api);
    return first;
}

/* ------------------------------------------------------------ TS */

int hw_stream(hw_ts_cb cb, void* ctx) {
    static unsigned char buf[TS_QUEUE][TS_FRAME];
    OVERLAPPED ov[TS_QUEUE];
    int rc = 0;
    g_stop = 0;
    ULONG zero = 0;
    WinUsb_SetPipePolicy(g_usb, EP_TS, PIPE_TRANSFER_TIMEOUT, sizeof zero, &zero);
    for (int i = 0; i < TS_QUEUE; ++i) {
        memset(&ov[i], 0, sizeof ov[i]);
        ov[i].hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!WinUsb_ReadPipe(g_usb, EP_TS, buf[i], TS_FRAME, NULL, &ov[i]) && GetLastError() != ERROR_IO_PENDING) {
            rc = (int)GetLastError();
            g_stop = 1;
        }
    }
    for (int i = 0; !g_stop; i = (i + 1) % TS_QUEUE) {
        ULONG got = 0;
        if (!WinUsb_GetOverlappedResult(g_usb, &ov[i], &got, TRUE)) {
            DWORD e = GetLastError();
            if (e != ERROR_OPERATION_ABORTED && !g_stop) rc = (int)e;
            break;
        }
        if (got) cb(buf[i], got, ctx);
        ResetEvent(ov[i].hEvent);
        if (!WinUsb_ReadPipe(g_usb, EP_TS, buf[i], TS_FRAME, NULL, &ov[i]) && GetLastError() != ERROR_IO_PENDING) {
            rc = (int)GetLastError();
            break;
        }
    }
    WinUsb_AbortPipe(g_usb, EP_TS);
    for (int i = 0; i < TS_QUEUE; ++i) {
        ULONG got;
        WinUsb_GetOverlappedResult(g_usb, &ov[i], &got, TRUE);
        CloseHandle(ov[i].hEvent);
    }
    return rc;
}

void hw_stream_stop(void) {
    g_stop = 1;
    if (g_usb) WinUsb_AbortPipe(g_usb, EP_TS);
}
