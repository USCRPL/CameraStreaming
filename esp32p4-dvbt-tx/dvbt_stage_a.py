#!/usr/bin/env python3
"""
dvbt_stage_a.py - RTL-SDR capture + DVB-T OFDM detector (Stage A).

Tunes an RTL-SDR to the modulator frequency, captures IQ, and runs a
guard-interval (cyclic-prefix) self-correlation to answer, rigorously:

  * Is an OFDM signal actually present?
  * Which transmission mode - 2K or 8K?   (settles the FFT-mode question)
  * Which guard interval - 1/4, 1/8, 1/16, 1/32?
  * What is the real in-band SNR?

WHY THIS SNR IS TRUSTWORTHY (unlike the earlier in/out-of-band ratio):
The guard interval is a verbatim copy of the last Tu samples-worth of each OFDM
symbol.  Correlating the signal with a copy of itself delayed by Tu gives a
coefficient  rho = |E[x(n) x*(n+Tu)]| / E[|x|^2] = SNR / (SNR + 1).  It is a
pure time-domain self-correlation, so it does NOT depend on the SDR's passband
shape or roll-off - which is exactly what made the previous spectral method
report phantom signal on empty frequencies.  We invert it: SNR = rho/(1-rho).

The delay that maximises this correlation is the useful symbol length Tu, which
tells us the transmission mode directly.  The spacing of the correlation peaks
is the total symbol length Ts = Tu + Tg, which gives the guard interval.

USAGE
  # live - tune the dongle and capture:
  python dvbt_stage_a.py --freq 474e6 --bw 2e6 --fs 3.2e6 --seconds 2
  # push the DC spike off-signal with an LO offset (recommended):
  python dvbt_stage_a.py --freq 474e6 --lo-offset 400e3 --bw 2e6 --fs 3.2e6
  # analyse a recording made with:  rtl_sdr -f 474000000 -s 3200000 -n 32000000 cap.bin
  python dvbt_stage_a.py --file cap.bin --bw 2e6 --fs 3.2e6
  # validate the detector against synthetic signals of known truth:
  python dvbt_stage_a.py --selftest

REQUIRES: numpy         (pyrtlsdr only for live capture; matplotlib only for --plot)
"""
import argparse
import sys
import numpy as np

GI_FRACTIONS = [1/32, 1/16, 1/8, 1/4]
GI_NAMES = {1/32: "1/32", 1/16: "1/16", 1/8: "1/8", 1/4: "1/4"}


# --------------------------------------------------------------------------
# DVB-T timing
# --------------------------------------------------------------------------
def elementary_period(bw_hz):
    """DVB-T elementary period T_e (seconds). 8 MHz uses 7/64 us; scales with BW."""
    return (7.0 / 64.0e6) * (8.0e6 / bw_hz)


def nominal_tu_samples(bw_hz, fs, mode):
    """Useful-symbol length in capture samples for '2K' or '8K'."""
    Nu = 2048 if mode == "2K" else 8192
    return Nu * elementary_period(bw_hz) * fs      # may be fractional


# --------------------------------------------------------------------------
# core correlation
# --------------------------------------------------------------------------
def _sliding_sum(a, L):
    """Length-L moving sum; output length len(a)-L+1. Works for real or complex."""
    c = np.concatenate((np.zeros(1, dtype=a.dtype), np.cumsum(a)))
    return c[L:] - c[:-L]


def gi_metric(x, delay, L):
    """
    Normalised guard-interval correlation rho[n] for a given delay and window L.
    Returns (rho, P, E) where P is the complex correlation and E the geo-mean
    energy, so callers can re-combine them (e.g. average numerator/denominator).
    """
    n = len(x) - delay
    a = x[:n]
    b = x[delay:delay + n]
    P = _sliding_sum(a * np.conj(b), L)
    e1 = _sliding_sum((a.real**2 + a.imag**2).astype(np.float64), L)
    e2 = _sliding_sum((b.real**2 + b.imag**2).astype(np.float64), L)
    E = np.sqrt(e1 * e2) + 1e-30
    return np.abs(P) / E, P, E


def _score_delay(x, delay, L):
    """How guard-like is this delay? High percentile of rho (guard-aligned windows)."""
    rho, _, _ = gi_metric(x, delay, L)
    return np.percentile(rho, 99.0)


def search_mode(x, bw, fs, mode, L=48, span=0.008, sub=400_000):
    """Search delays around a mode's nominal Tu; return (best_delay, best_score, tu0)."""
    tu0 = nominal_tu_samples(bw, fs, mode)
    lo = max(8, int(tu0 * (1 - span)))
    hi = int(tu0 * (1 + span))
    xs = x[:min(len(x), sub + hi)]
    best_d, best_s = lo, -1.0
    for d in range(lo, hi + 1):
        s = _score_delay(xs, d, L)
        if s > best_s:
            best_s, best_d = s, d
    return best_d, best_s, tu0


def fold_profile(rho, Ts, max_syms=40):
    """Average rho over up to max_syms symbol periods -> one Ts-long profile."""
    Ts = int(round(Ts))
    n = min(len(rho), Ts * max_syms)
    n -= n % Ts
    if n < Ts:
        return None
    return rho[:n].reshape(-1, Ts).mean(axis=0)


# --------------------------------------------------------------------------
# full Stage-A analysis
# --------------------------------------------------------------------------
def analyze(x, bw, fs, verbose=True):
    x = np.asarray(x, dtype=np.complex128)
    # ADC overload check on the RAW samples (before any scaling): a clean capture
    # has only its rare peaks near full-scale; a clipped one pins many samples there.
    mx = max(float(np.abs(x.real).max()), float(np.abs(x.imag).max())) + 1e-30
    clip_frac = float(np.mean((np.abs(x.real) >= 0.995 * mx) |
                              (np.abs(x.imag) >= 0.995 * mx)))
    x = x - x.mean()                                   # kill DC / carrier leakage bias
    x = x / (np.sqrt(np.mean(np.abs(x)**2)) + 1e-30)   # normalise

    # 1) which mode? pick the Tu delay with the strongest guard correlation
    cand = {m: search_mode(x, bw, fs, m) for m in ("2K", "8K")}
    mode = max(cand, key=lambda m: cand[m][1])
    delay, score, tu0 = cand[mode]

    # 2) guard interval: test the 4 fractions, keep the sharpest folded profile
    seg = x[:min(len(x), 800_000)]
    rho_det, _, _ = gi_metric(seg, delay, L=48)
    best = None
    for gi in GI_FRACTIONS:
        Ts = delay * (1 + gi)
        prof = fold_profile(rho_det, Ts)
        if prof is None:
            continue
        sharp = prof.max() - np.median(prof)
        if best is None or sharp > best[0]:
            best = (sharp, gi, Ts, prof)
    sharp, gi, Ts, prof = best
    Tg = delay * gi

    # 3) SNR from the guard-aligned correlation coefficient, with L sized to the
    #    guard so the window sits fully inside the cyclic prefix.
    L2 = int(np.clip(Tg * 0.5, 16, 512))
    rho2, _, _ = gi_metric(seg, delay, L2)
    prof2 = fold_profile(rho2, Ts)
    rho_peak = float(np.clip(prof2.max(), 1e-4, 0.999))  # plateau centre
    rho_floor = float(np.median(prof2))
    snr_lin = rho_peak / (1 - rho_peak)
    snr_db = 10 * np.log10(snr_lin)

    # detection confidence: guard peak must stand clearly above the off-guard floor
    present = (rho_peak - rho_floor) > 0.03 and rho_peak > 0.05

    res = dict(mode=mode, delay=delay, tu0=tu0, score=score, gi=gi, Ts=Ts,
               Tg=Tg, rho_peak=rho_peak, rho_floor=rho_floor, snr_db=snr_db,
               present=present, profile=prof2, other=cand, clip_frac=clip_frac)
    if verbose:
        _report(res, bw, fs)
    return res


def _report(r, bw, fs):
    to_us = 1e6 / fs
    print("\n================ DVB-T Stage-A detector ================")
    if not r["present"]:
        print("  RESULT: no clear OFDM guard-interval structure found.")
        print(f"    best guard correlation rho_peak = {r['rho_peak']:.3f}"
              f" (floor {r['rho_floor']:.3f}) - not a confident detection.")
        print("    -> either nothing is transmitting on this frequency, the")
        print("       capture is too short/noisy, or bw/fs are wrong.")
        _report_modes(r)
        return
    print(f"  OFDM DETECTED.")
    print(f"    transmission mode : {r['mode']}   "
          f"(useful-symbol delay Tu = {r['delay']} samples = {r['delay']*to_us:.1f} us)")
    print(f"    guard interval    : {GI_NAMES[r['gi']]}   "
          f"(Tg = {r['Tg']:.0f} samples = {r['Tg']*to_us:.1f} us)")
    print(f"    symbol period Ts  : {r['Ts']:.0f} samples = {r['Ts']*to_us:.1f} us   "
          f"({fs/r['Ts']:.0f} symbols/s)")
    print(f"    guard corr rho    : peak {r['rho_peak']:.3f}  floor {r['rho_floor']:.3f}")
    print(f"    ESTIMATED SNR     : {r['snr_db']:.1f} dB   (LOWER BOUND - see notes)")
    print("  ------------------------------------------------------")
    # overload check first: a clipped capture makes every other number garbage
    if r["clip_frac"] > 0.01:
        print(f"  !! ADC OVERLOAD: {100*r['clip_frac']:.1f}% of samples are at full")
        print("     scale. The capture is CLIPPING - the SNR above is meaningless.")
        print("     RE-CAPTURE WITH LOWER RTL GAIN (try -g 15, or -g 0..20) and")
        print("     re-run. This is the most likely cause of a low reading on a")
        print("     strong bench link.")
        print("  ------------------------------------------------------")
    print("  NOTE: this GI-correlation SNR is a pessimistic LOWER BOUND for a")
    print("  narrowband (2 MHz) signal on an RTL-SDR: the guard lag is long")
    print(f"  (Tu={r['delay']*to_us:.0f} us), so the dongle's own oscillator phase")
    print("  noise decorrelates the guard and subtracts from the reading. A real")
    print("  receiver tracks that out with pilots. True SNR is likely HIGHER;")
    print("  Stage B (constellation) will measure it without this bias.")
    print("  ------------------------------------------------------")
    need = 5.5
    if r["clip_frac"] > 0.01:
        print("  VERDICT: fix the overload and re-capture before trusting any SNR.")
    elif r["snr_db"] >= need + 2:
        print(f"  VERDICT: even this lower bound clears the ~{need:.0f} dB QPSK-1/2")
        print("           needs. If the RX won't lock, suspect a parameter mismatch")
        print("           (mode/GI/inversion), not signal level.")
    elif r["snr_db"] >= need:
        print(f"  VERDICT: lower-bound SNR is near the ~{need:.0f} dB threshold; true")
        print("           SNR is probably adequate. Chase parameters, then level.")
    else:
        print(f"  VERDICT: lower-bound SNR reads low, but for this signal that is")
        print("           expected from the phase-noise bias above - it does NOT")
        print("           prove the link is weak. Confirm by (a) capturing at lower")
        print("           RTL gain, (b) building Stage B to read the true SNR.")
    _report_modes(r)
    print("========================================================\n")


def _report_modes(r):
    print("  mode search (higher guard score = better match):")
    for m, (d, s, tu0) in r["other"].items():
        mark = "  <- picked" if m == r.get("mode") and r["present"] else ""
        print(f"    {m}: nominal Tu~{tu0:7.1f}  best delay {d:6d}  score {s:.3f}{mark}")


# --------------------------------------------------------------------------
# capture back-ends
# --------------------------------------------------------------------------
def capture_rtlsdr(freq, fs, seconds, gain, lo_offset):
    try:
        from rtlsdr import RtlSdr
    except ImportError:
        sys.exit("pyrtlsdr not installed. Use:  pip install pyrtlsdr\n"
                 "or (more reliable on Windows) capture with rtl_sdr and pass --file:\n"
                 "  rtl_sdr -f %d -s %d -g 40 -n %d cap.bin\n"
                 "  python dvbt_stage_a.py --file cap.bin --bw 2e6 --fs %g"
                 % (int(freq), int(fs), int(fs * 10), fs))
    except (AttributeError, OSError) as e:
        sys.exit(
            "pyrtlsdr loaded but its librtlsdr DLL is incompatible (%s).\n"
            "This is a version mismatch (e.g. missing 'rtlsdr_set_dithering'),\n"
            "common on Windows with the RTL-SDR Blog V4. Easiest fix: skip\n"
            "pyrtlsdr and capture with rtl_sdr.exe, then analyse the file:\n"
            "  rtl_sdr -f %d -s %d -g 40 -n %d cap.bin\n"
            "  python dvbt_stage_a.py --file cap.bin --bw 2e6 --fs %g\n"
            "(Or fix live capture: pip install pyrtlsdr==0.2.93, or install the\n"
            " RTL-SDR Blog librtlsdr build so the DLL matches pyrtlsdr.)"
            % (str(e), int(freq), int(fs), int(fs * 10), fs))
    try:
        sdr = RtlSdr()
    except (AttributeError, OSError) as e:
        sys.exit(
            "Could not open the RTL-SDR via pyrtlsdr (%s).\n"
            "Capture with rtl_sdr.exe instead and analyse the file:\n"
            "  rtl_sdr -f %d -s %d -g 40 -n %d cap.bin\n"
            "  python dvbt_stage_a.py --file cap.bin --bw 2e6 --fs %g"
            % (str(e), int(freq), int(fs), int(fs * 10), fs))
    sdr.sample_rate = fs
    sdr.center_freq = freq - lo_offset            # tune off so DC spike misses signal
    sdr.gain = "auto" if gain is None else gain
    n = int(fs * seconds)
    n -= n % 512
    print(f"tuning {sdr.center_freq/1e6:.3f} MHz (signal at {freq/1e6:.3f}), "
          f"fs={fs/1e6:.3f} Msps, {seconds}s, gain={sdr.gain}")
    x = sdr.read_samples(n)
    sdr.close()
    if lo_offset:                                 # mix signal back to baseband
        t = np.arange(len(x))
        x = x * np.exp(-2j * np.pi * (lo_offset / fs) * t)
    return np.asarray(x, dtype=np.complex128)


def load_file(path, fmt, fs, lo_offset):
    if fmt == "u8":
        raw = np.fromfile(path, dtype=np.uint8).astype(np.float32)
        raw = (raw - 127.5) / 127.5
        raw = raw[:len(raw) - (len(raw) % 2)]
        x = raw[0::2] + 1j * raw[1::2]
    elif fmt == "cs8":                       # SatDump / signed 8-bit interleaved
        raw = np.fromfile(path, dtype=np.int8).astype(np.float32) / 128.0
        raw = raw[:len(raw) - (len(raw) % 2)]
        x = raw[0::2] + 1j * raw[1::2]
    else:
        x = np.fromfile(path, dtype=np.complex64)
    x = x.astype(np.complex128)
    if lo_offset:
        t = np.arange(len(x))
        x = x * np.exp(-2j * np.pi * (lo_offset / fs) * t)
    return x


# --------------------------------------------------------------------------
# synthetic signal + self-test  (validate BEFORE trusting real data)
# --------------------------------------------------------------------------
def _resample_fft(x, new_len):
    X = np.fft.fftshift(np.fft.fft(x))
    old = len(x)
    if new_len >= old:
        pad = new_len - old
        Xn = np.concatenate((np.zeros(pad // 2, complex), X,
                             np.zeros(pad - pad // 2, complex)))
    else:
        cut = old - new_len
        Xn = X[cut // 2: cut // 2 + new_len]
    return np.fft.ifft(np.fft.ifftshift(Xn)) * (new_len / old)


def make_ofdm(mode, gi, n_syms, bw, fs, snr_db, cfo_hz=1500.0,
              dc=0.3, seed=0):
    """Synthesize a DVB-T-like OFDM signal at capture rate fs with known truth."""
    rng = np.random.default_rng(seed)
    Nu = 2048 if mode == "2K" else 8192
    Ng = int(Nu * gi)
    active = int(Nu * 1705 / 2048)            # DVB-T active-carrier fraction
    native = np.zeros(0, complex)
    blk = []
    for _ in range(n_syms):
        car = np.zeros(Nu, complex)
        idx = np.arange((Nu - active) // 2, (Nu + active) // 2)
        car[idx] = ((rng.integers(0, 2, active) * 2 - 1) +
                    1j * (rng.integers(0, 2, active) * 2 - 1)) / np.sqrt(2)
        t = np.fft.ifft(np.fft.ifftshift(car)) * np.sqrt(Nu)
        blk.append(np.concatenate((t[-Ng:], t)))       # prepend cyclic prefix
    native = np.concatenate(blk)

    native_fs = 1.0 / elementary_period(bw)            # signal's own rate
    new_len = int(round(len(native) * fs / native_fs))
    x = _resample_fft(native, new_len)

    x = x / np.sqrt(np.mean(np.abs(x)**2))
    npow = 10 ** (-snr_db / 10)
    x = x + np.sqrt(npow / 2) * (rng.standard_normal(len(x)) +
                                 1j * rng.standard_normal(len(x)))
    if cfo_hz:
        x = x * np.exp(2j * np.pi * (cfo_hz / fs) * np.arange(len(x)))
    x = x + dc                                          # DC / LO-leakage term
    return x


def selftest():
    bw, fs = 2e6, 3.2e6
    print("Validating detector against synthetic signals of KNOWN truth...")
    cases = [
        ("2K", 1/4, 12.0), ("2K", 1/4, 6.0), ("2K", 1/4, 3.0),
        ("2K", 1/8, 8.0), ("8K", 1/4, 8.0),
    ]
    ok = True
    for mode, gi, snr in cases:
        nsy = 320 if mode == "2K" else 90
        x = make_ofdm(mode, gi, nsy, bw, fs, snr, seed=int(snr * 7) + len(mode))
        r = analyze(x, bw, fs, verbose=False)
        mode_ok = (r["mode"] == mode)
        gi_ok = (r["gi"] == gi)
        snr_ok = abs(r["snr_db"] - snr) <= 2.0
        good = mode_ok and gi_ok and snr_ok
        ok = ok and good and r["present"]
        print(f"  truth {mode:>2} GI {GI_NAMES[gi]:>4} SNR {snr:4.1f} dB  ->  "
              f"got {r['mode']:>2} GI {GI_NAMES[r['gi']]:>4} SNR {r['snr_db']:5.1f} dB "
              f"[mode {'OK' if mode_ok else 'X'} | gi {'OK' if gi_ok else 'X'} | "
              f"snr {'OK' if snr_ok else 'X'}]  {'PASS' if good else 'FAIL'}")
    # negative control: pure noise must NOT be called a detection
    rng = np.random.default_rng(1)
    noise = (rng.standard_normal(600_000) + 1j * rng.standard_normal(600_000)) + 0.3
    rn = analyze(noise, bw, fs, verbose=False)
    neg_ok = not rn["present"]
    print(f"  noise-only negative control -> present={rn['present']} "
          f"(rho_peak {rn['rho_peak']:.3f})  {'PASS' if neg_ok else 'FAIL'}")
    ok = ok and neg_ok
    print("\n  SELF-TEST:", "PASS - detector trustworthy" if ok else "FAIL - do not trust")
    return 0 if ok else 1


# --------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="DVB-T Stage-A OFDM detector")
    ap.add_argument("--file", help="analyse a recording instead of live capture")
    ap.add_argument("--fmt", choices=["u8", "cs8", "c64"], default="u8",
                    help="file format: rtl_sdr u8 (default) or complex64")
    ap.add_argument("--freq", type=float, default=474e6, help="signal frequency, Hz")
    ap.add_argument("--bw", type=float, default=2e6, help="signal bandwidth, Hz")
    ap.add_argument("--fs", type=float, default=3.2e6, help="sample rate, Hz")
    ap.add_argument("--seconds", type=float, default=2.0, help="capture length, s")
    ap.add_argument("--gain", type=float, default=None, help="tuner gain dB (default auto)")
    ap.add_argument("--lo-offset", type=float, default=0.0,
                    help="tune this far below freq, mix back (dodges the DC spike)")
    ap.add_argument("--plot", action="store_true", help="save rho profile to stage_a.png")
    ap.add_argument("--selftest", action="store_true", help="validate on synthetic data")
    a = ap.parse_args()

    if a.selftest:
        sys.exit(selftest())

    if a.file:
        x = load_file(a.file, a.fmt, a.fs, a.lo_offset)
        print(f"loaded {len(x)} samples ({len(x)/a.fs:.2f} s) from {a.file}")
    else:
        x = capture_rtlsdr(a.freq, a.fs, a.seconds, a.gain, a.lo_offset)

    if len(x) < 200_000:
        sys.exit("capture too short; need >~0.1 s")

    r = analyze(x, a.bw, a.fs, verbose=True)

    if a.plot and r["profile"] is not None:
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
            plt.figure(figsize=(10, 4))
            plt.plot(r["profile"], lw=0.8)
            plt.axhline(r["rho_peak"], color="g", ls="--", label=f"peak {r['rho_peak']:.3f}")
            plt.axhline(r["rho_floor"], color="gray", ls="--", label=f"floor {r['rho_floor']:.3f}")
            plt.title(f"guard correlation profile - {r['mode']} GI {GI_NAMES[r['gi']]} "
                      f"SNR {r['snr_db']:.1f} dB")
            plt.xlabel("phase within symbol (samples)"); plt.ylabel("rho")
            plt.legend(); plt.tight_layout(); plt.savefig("stage_a.png", dpi=130)
            print("wrote stage_a.png")
        except ImportError:
            print("(matplotlib not installed; skipping plot)")


if __name__ == "__main__":
    main()