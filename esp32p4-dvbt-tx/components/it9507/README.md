# `it9507` — ESP32-P4 USB-host driver for the ITE IT9507 DVB-T modulator

This is an ESP-IDF component that lets an **ESP32-P4 act as the USB host** for an
**IT9507-based DVB-T modulator stick** (UT-100C and friends, USB `048d:9507`).
You feed it a constant-bit-rate MPEG2-TS; it emits a DVB-T RF signal.

It is a port of the GPL-2.0 Linux/libusb userspace drivers
(`linuxstb/it9507`, `fabled/sendts-it9507`). This port is GPL-2.0 as well.

```
 OV5647 ──MIPI-CSI──► ESP32-P4 ──H.264──► [your TS muxer] ──CBR TS──► it9507_write_ts()
                                                                          │  USB 2.0 HS host
                                                                          ▼
                                                                    IT9507 stick ──► RF
```

## What works out of the box vs. what needs hardware validation

| Area | Status |
|------|--------|
| USB Host enumeration / interface claim / bulk plumbing (EP `0x02`/`0x81` cmd, EP `0x06` TS) | Implemented against the ESP-IDF USB Host Library; matches the endpoints used by the Linux driver. **Needs on-target validation.** |
| Command framing, checksum, register r/w, firmware upload, BOOT | Ported verbatim from upstream. |
| Tuner math (LO synth, IQ calibration table + interpolation), bandwidth/frequency programming | Ported verbatim, full IQ table. |
| `set_ts_interface` EP6-RX enable | Ported. The **tail "frame byte-count" programming** is the most fragile part — if you see TS under-runs/over-runs, port that block verbatim from upstream `it950x.c`. |
| **Firmware blob** | **You must supply it** — not redistributed here. See below. |
| CBR TS muxer (`example/`) | **Skeleton.** Validate PCR/PTS and null-padding against a TS analyser before trusting RF output. |

## You must supply the firmware blob

The stick has no flash; the ~4560-byte "link" MCU image is uploaded on every
connect. It is **not** included here. Generate the header from either upstream:

```sh
# Option A: from the linuxstb firmware file
xxd -i dvb-mod-it9507.fw \
  | sed 's/unsigned char .*\[\]/static const uint8_t it9507_firmware[]/' \
  | sed 's/unsigned int .*_len/static const uint32_t it9507_firmware_len/' \
  > firmware_blob.inc
```

Drop the two symbols (`it9507_firmware[]`, `it9507_firmware_len`) into
`include/it9507_firmware.h` (replacing the stub), or `#include` the generated
file there. With the stub in place the driver refuses to start and tells you why.

## Build

In your project's `CMakeLists.txt` make the component visible (place it under
`components/it9507/`), then enable the firmware:

```cmake
# components/it9507/CMakeLists.txt already registers the sources; just define:
target_compile_definitions(${COMPONENT_LIB} PRIVATE IT9507_HAVE_FIRMWARE=1)
```

Requires ESP-IDF ≥ 5.3 and `target esp32p4`.

## Minimal usage

```c
it9507_config_t cfg = IT9507_CONFIG_DEFAULT();
it9507_handle_t mod;
ESP_ERROR_CHECK(it9507_install(&cfg, &mod));

it9507_modulation_t m = {
    .frequency_khz = 473000,   /* UHF ch 21 centre        */
    .bandwidth_khz = 2000,     /* narrow BW = low bitrate */
    .constellation = IT9507_QPSK,
    .code_rate     = IT9507_FEC_1_2,
    .guard_interval= IT9507_GI_1_4,
    .transmission_mode = IT9507_TX_MODE_2K,
    .cell_id = 0,
};
ESP_ERROR_CHECK(it9507_set_modulation(mod, &m));
ESP_ERROR_CHECK(it9507_start(mod));

for (;;) {
    size_t n; const uint8_t *ts = get_cbr_ts_chunk(&n);   /* your muxer */
    it9507_write_ts(mod, ts, n, pdMS_TO_TICKS(250));
}
```

## Bandwidth, bitrate, and the CBR rule

DVB-T capacity is set by **bandwidth × constellation × code-rate × guard
interval**. The TS you push **must be constant-bit-rate at or below that
capacity**, null-packet-padded (PID `0x1FFF`). USB bulk back-pressure paces you:
if `it9507_write_ts()` starts returning `ESP_ERR_TIMEOUT`, your TS rate exceeds
the channel — lower the H.264 bitrate or widen the channel.

Rough usable payloads: QPSK 1/2 GI 1/4 in **2 MHz ≈ 1.2 Mbit/s**, in
**8 MHz ≈ 5 Mbit/s**; QAM16 roughly doubles it. Budget the TS ~10–15 % below the
raw figure for PSI/PCR/null overhead. These are the knobs that make a 1–3 Mbit/s
H.264 stream fit.

## API

See `include/it9507.h`. Summary:

- `it9507_install()` — host up, wait for stick, upload firmware, power up.
- `it9507_set_modulation()` — program carrier + DVB-T params.
- `it9507_start()` / `it9507_stop()` — RF carrier on/off.
- `it9507_write_ts()` — push CBR TS bytes (multiple of 188).
- `it9507_get_fw_versions()` — liveness probe.
- `it9507_uninstall()` — tear down.
