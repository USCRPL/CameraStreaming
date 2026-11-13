# UT-120 Viewer

Tune the Hides UT-120 (ITE IT913x) dongle, browse channels, watch video and monitor link quality on Windows, without BDAViewer and without ITE's Windows driver.

Start it with `run.bat`. To replay a recording: `run.bat path\to\file.ts`.

## How it talks to the dongle

ITE's kernel driver (`IT9135BDA.sys`) does not load on current Windows 11 builds: the 2019 version is blocked as incompatible and the 2017 version is rejected as unsigned. This app does not use it. Instead:

- The dongle runs on **WinUSB**, Microsoft's built-in generic USB driver.
- `engine/ut120_hw.c` is a user-mode driver: it loads the chip firmware, configures the demodulator(s) and reads the transport stream over USB.
- The chip logic in `engine/itesdk/` is ITE's own IT913x API, copied unchanged from the Linux SDK on the UT-120 CD (`it913x_linux_v18.05.21.1`). The bring-up order follows ITE's Linux driver.

| Part | Role |
| --- | --- |
| `engine/ut120_engine.cpp` | Process the GUI talks to: tune, statistics, TS on stdout |
| `engine/ut120_hw.c` | WinUSB transport and device bring-up |
| `ut120/ts_analyzer.py` | Packet error counts, bitrates, channel discovery from PAT/PMT/SDT |
| `ut120/video.py` | H.264/HEVC/MPEG-2 decoding with PyAV |
| `ut120_viewer.py` | GUI |

## One-time setup: switch the dongle to WinUSB

Needs an administrator account. Everything used here is part of Windows.

1. Remove ITE's tuner driver packages so Windows stops assigning them. In a terminal opened with **Run as administrator**:
   ```
   pnputil /delete-driver oem4.inf /uninstall /force
   pnputil /delete-driver oem22.inf /uninstall /force
   ```
   The `oemNN` numbers are specific to this PC. Check with `pnputil /enum-drivers` that the ones you delete have Original Name `it9135bda.inf`. Leave `it9507tx.inf` alone; that is the UT-100 transmitter driver.
2. Unplug the UT-120 and plug it back in. It should now appear in Device Manager as an unknown device, with no driver.
3. In Device Manager, right-click it and choose **Update driver**, then **Browse my computer for drivers**, then **Let me pick from a list of available drivers on my computer**.
4. Choose **Universal Serial Bus devices**, then manufacturer **WinUsb Device** and model **WinUsb Device**. Accept the warning.
5. Check that it worked: `engine\ut120_engine.exe selftest` should show `ready`, then load the firmware and print the chip details.

If Windows Update reinstalls ITE's driver later, repeat steps 2 to 4.

## Reading the quality panel

- **Corrupt packets**: arrived with the transport error flag set, meaning error correction failed. This is the most direct measure of a poor RF link.
- **RS uncorrectable**: the demodulator's own count of packets Reed-Solomon could not repair (BDAViewer calls this RSD abort).
- **Bit error rate**: post-Viterbi BER, the same figure BDAViewer shows.
- **Signal strength**: dBm from the tuner, plus the chip's 0 to 100 indication.
- **Diversity**: per-chip lock and level when the dongle has two demodulators combined.
- **Lost packets**: continuity counter gaps.
- **Verdict**: GOOD means no corrupt or lost packets in the last 10 s. BAD means more than 10 per second.

## Limits

- Video only; audio is not played. Recordings keep all streams.
- The first picture after tuning waits for a keyframe, which can take several seconds depending on the encoder's GOP length.

## Building and tests

`build_engine.bat` builds the engine with MSYS2's UCRT64 GCC. `run.bat` builds it automatically if it is missing.

```bash
.venv/Scripts/python -m unittest discover -s tests
```
