#!/usr/bin/env python3
"""
dvbt_stage_b.py - DVB-T Stage B: recover the QPSK constellation and the TRUE SNR.

Stage A proved OFDM is present and found the mode/GI. Stage B goes further: it
synchronises, FFTs each OFDM symbol, equalises, and plots the data-carrier
constellation - then measures SNR as MER (modulation error ratio) from how
tightly the points cluster. Because equalisation tracks out the slow phase
drift, this MER is NOT corrupted by the oscillator phase noise that made the
Stage-A guard-correlation SNR a pessimistic lower bound.

What a clean 4-point QPSK scatter means: the transmitted signal is genuinely
demodulable and the only thing between you and a lock is level/parameters.
A fuzzy blob means low real SNR, and the MER number says exactly how low.

Pipeline: resample capture -> native rate (2048 samples/useful symbol for 2K)
-> guard-interval timing + fractional CFO -> per-symbol FFT -> integer-CFO
(active-band) alignment -> blind flat-channel equalise (common phase + a small
timing-ramp correction; valid for a short bench link) -> constellation + MER.

USAGE
  python dvbt_stage_b.py --file cap.bin --bw 2e6 --fs 3.2e6 --mode 2K --gi 1/4
  python dvbt_stage_b.py --selftest          # validate against known-SNR signals

REQUIRES: numpy   (matplotlib for the --plot constellation image)
"""
import argparse
import sys
import numpy as np

# DVB-T 2K non-data carrier positions (to exclude from the data constellation).
CONTINUAL_2K = [0, 48, 54, 87, 141, 156, 192, 201, 255, 279, 282, 333, 432, 450,
    483, 525, 531, 618, 636, 714, 759, 765, 780, 804, 873, 888, 918, 939, 942,
    969, 984, 1050, 1101, 1107, 1110, 1137, 1140, 1146, 1206, 1269, 1323, 1377,
    1491, 1683, 1704]
TPS_2K = [34, 50, 209, 346, 413, 569, 595, 688, 790, 901, 1073, 1219, 1262,
    1286, 1469, 1594, 1687]
GI_MAP = {"1/32": 1/32, "1/16": 1/16, "1/8": 1/8, "1/4": 1/4}


def elementary_period(bw):
    return (7.0 / 64.0e6) * (8.0e6 / bw)


def fft_resample(x, new_len):
    """Resample by spectral trunc/zero-pad (works for complex)."""
    n = len(x)
    X = np.fft.fft(x)
    if new_len < n:
        h = (new_len + 1) // 2
        Xn = np.concatenate((X[:h], X[n - (new_len - h):]))
    else:
        h = (n + 1) // 2
        Xn = np.concatenate((X[:h], np.zeros(new_len - n, complex), X[h:]))
    return np.fft.ifft(Xn) * (new_len / n)


def _slide(a, L):
    c = np.concatenate((np.zeros(1, dtype=a.dtype), np.cumsum(a)))
    return c[L:] - c[:-L]


def gi_sync(x, Tu, Tg):
    """Symbol phase within Ts and fractional CFO (cycles/sample) from the CP."""
    Ts = Tu + Tg
    P = _slide(x[:-Tu] * np.conj(x[Tu:]), Tg)
    m = (len(P) // Ts) * Ts
    grid = np.abs(P[:m]).reshape(-1, Ts)
    off = int(np.argmax(grid.mean(0)))
    col = P[:m].reshape(-1, Ts)[:, off]
    # P(guard) phase = -2*pi*df*Tu, so the TRUE CFO is the negative of that angle.
    cfo = -np.angle(np.mean(col)) / (2 * np.pi * Tu)
    return off, cfo


def demodulate(x, fs, bw, mode, gi, max_syms=400):
    Nu = 2048 if mode == "2K" else 8192
    Ng = int(round(Nu * gi))
    Ts = Nu + Ng
    active = 1705 if mode == "2K" else 6817
    native = 1.0 / elementary_period(bw)

    # resample capture to the signal's native rate (Nu samples per useful symbol)
    need = int((max_syms + 8) * Ts * fs / native)
    seg = x[:min(len(x), need)]
    xr = fft_resample(seg, int(round(len(seg) * native / fs)))
    xr = xr - xr.mean()

    # data-carrier mask: drop scattered pilots (k%3==0), continual pilots, TPS
    kk = np.arange(active)
    dm = (kk % 3 != 0)
    for p in CONTINUAL_2K + TPS_2K:
        if p < active:
            dm[p] = False
    kcd = (kk[dm] - active / 2.0)

    def sync_and_fft(sig):
        off, cfo = gi_sync(sig, Nu, Ng)
        s2 = sig * np.exp(-2j * np.pi * cfo * np.arange(len(sig)))
        F, m = [], 0
        while off + Ng + m * Ts + Nu <= len(s2) and m < max_syms:
            st = off + Ng + m * Ts
            F.append(np.fft.fftshift(np.fft.fft(s2[st:st + Nu])))
            m += 1
        return np.asarray(F), cfo

    def per_symbol_tau_idx(car, taus):
        Rd4 = np.exp(-1j * 4 * 2 * np.pi * np.outer(taus, kcd) / Nu)
        D4 = car[:, dm] ** 4
        return np.argmax(np.abs(D4 @ Rd4.conj().T), axis=1), Rd4

    # Pass 1: measure the timing DRIFT across the capture. A sample-clock offset
    # (SFO) between the SDR and the transmitter makes each symbol land a little
    # later/earlier than the last; left uncorrected it smears the constellation
    # (even ~1 ppm ruins a narrowband capture). We estimate it and resample the
    # whole signal to correct the clock - what a real receiver's timing loop does.
    F, cfo = sync_and_fft(xr)
    if len(F) < 8:
        return None
    pw = np.mean(np.abs(F) ** 2, axis=0)
    cs = np.concatenate(([0.0], np.cumsum(pw)))
    b0 = int(np.argmax(cs[active:] - cs[:-active]))
    ti, _ = per_symbol_tau_idx(F[:, b0:b0 + active], np.arange(-8.0, 8.01, 0.25))
    tau1 = np.arange(-8.0, 8.01, 0.25)[ti]
    mm = np.arange(len(tau1))
    slope = float(np.polyfit(mm, tau1, 1)[0])
    sfo = slope / Ts
    tau_drift = slope * len(tau1)

    # correct the clock and re-demodulate
    xr = fft_resample(xr, int(round(len(xr) / (1 + sfo))))
    F, cfo = sync_and_fft(xr)
    if len(F) < 8:
        return None
    car = F[:, b0:b0 + active]

    # per-symbol fine timing + common-phase de-rotation (blind: QPSK^4 = -1)
    taus = np.arange(-3.0, 3.01, 0.05)
    bi, Rd4 = per_symbol_tau_idx(car, taus)
    D = car[:, dm]
    D4 = D ** 4
    cons = []
    for s in range(len(D)):
        val = np.sum(D4[s] * np.conj(Rd4[bi[s]]))
        ph = (np.angle(val) - np.pi) / 4.0
        d = D[s] * np.exp(-1j * (2 * np.pi * taus[bi[s]] * kcd / Nu + ph))
        d = d / (np.sqrt(np.mean(np.abs(d) ** 2)) + 1e-12)
        cons.append(d)
    cons = np.concatenate(cons)
    cons = cons / (np.sqrt(np.mean(np.abs(cons) ** 2)) + 1e-12)
    tau = float(np.mean(taus[bi]))

    # MER / SNR from cluster tightness
    ideal = (np.sign(cons.real) + 1j * np.sign(cons.imag)) / np.sqrt(2)
    evm2 = np.mean(np.abs(cons - ideal) ** 2)
    mer = 10 * np.log10(1.0 / (evm2 + 1e-30))
    return dict(cons=cons, mer=mer, cfo_hz=cfo * native, int_cfo=b0 - (Nu // 2 - active // 2),
                tau=tau, tau_drift=tau_drift, sfo_ppm=sfo * 1e6, nsym=len(car))


# ---------------------------------------------------------------- synthetic
def make_dvbt(mode, gi, n_syms, bw, fs, snr_db, cfo_hz=2500.0, dc=0.2, seed=0):
    """Random-QPSK OFDM (DVB-T timing) at capture rate with known truth."""
    rng = np.random.default_rng(seed)
    Nu = 2048 if mode == "2K" else 8192
    Ng = int(Nu * gi)
    active = 1705 if mode == "2K" else 6817
    base = Nu // 2 - active // 2
    blk = []
    for _ in range(n_syms):
        car = np.zeros(Nu, complex)
        qpsk = ((rng.integers(0, 2, active) * 2 - 1) +
                1j * (rng.integers(0, 2, active) * 2 - 1)) / np.sqrt(2)
        sh = np.zeros(Nu, complex)
        sh[base:base + active] = qpsk
        car = np.fft.ifftshift(sh)
        t = np.fft.ifft(car) * np.sqrt(Nu)
        blk.append(np.concatenate((t[-Ng:], t)))
    native = np.concatenate(blk)
    nat_fs = 1.0 / elementary_period(bw)
    x = fft_resample(native, int(round(len(native) * fs / nat_fs)))
    x = x / np.sqrt(np.mean(np.abs(x) ** 2))
    npow = 10 ** (-snr_db / 10)
    x = x + np.sqrt(npow / 2) * (rng.standard_normal(len(x)) + 1j * rng.standard_normal(len(x)))
    x = x * np.exp(2j * np.pi * (cfo_hz / fs) * np.arange(len(x))) + dc
    return x


def selftest():
    bw, fs = 2e6, 3.2e6
    native = 1.0 / elementary_period(bw)
    Nu, active = 2048, 1705
    # MER is per-carrier C/N; make_dvbt sets capture-band SNR. The two differ by
    # the OFDM processing gain: resample to signal BW (fs/native) x FFT carrier
    # concentration (Nu/active). Compare MER against that expectation.
    gain = 10 * np.log10((fs / native) * (Nu / active))
    print("Validating Stage B against synthetic signals of KNOWN SNR...")
    print(f"  (per-carrier C/N = capture SNR + {gain:.1f} dB OFDM processing gain)")
    ok = True
    for snr in (15.0, 10.0, 7.0, 4.0):
        x = make_dvbt("2K", 1/4, 260, bw, fs, snr, cfo_hz=2500.0, seed=int(snr))
        r = demodulate(x, fs, bw, "2K", 1/4)
        expect = snr + gain
        d = r["mer"] - expect
        good = abs(d) <= 2.0
        ok = ok and good
        print(f"  capture SNR {snr:4.1f} -> expect C/N {expect:4.1f} -> "
              f"measured MER {r['mer']:5.1f} dB  (int-CFO {r['int_cfo']:+d})  "
              f"{'PASS' if good else 'FAIL (%.1f)' % d}")
    print("\n  SELF-TEST:", "PASS - MER tracks true per-carrier C/N" if ok else "FAIL")

    # SFO robustness: a clock offset must not wreck MER (it did before correction)
    print("\n  SFO robustness (clean 20 dB signal + sample-clock offset):")
    for sfo in (1.0, 5.0):
        x = make_dvbt("2K", 1/4, 400, bw, fs, 20.0, cfo_hz=510.0, seed=1)
        x = fft_resample(x, int(round(len(x) * (1 + sfo / 1e6))))
        r = demodulate(x, fs, bw, "2K", 1/4)
        good = r["mer"] >= 18.0
        ok = ok and good
        print(f"    +{sfo:.0f} ppm SFO -> MER {r['mer']:.1f} dB (est {r['sfo_ppm']:+.1f})  "
              f"{'PASS' if good else 'FAIL'}")
    print("\n  OVERALL:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


# ---------------------------------------------------------------- io / main
def load_file(path, fmt, fs, lo_offset):
    if fmt == "u8":
        b = np.fromfile(path, dtype=np.uint8)
        clip = float(np.mean((b == 0) | (b == 255)))
        raw = (b.astype(np.float32) - 127.5) / 127.5
        raw = raw[:len(raw) - (len(raw) % 2)]
        x = (raw[0::2] + 1j * raw[1::2]).astype(np.complex128)
    elif fmt == "cs8":                       # SatDump / signed 8-bit interleaved
        b = np.fromfile(path, dtype=np.int8)
        clip = float(np.mean((b == -128) | (b == 127)))
        raw = b.astype(np.float32) / 128.0
        raw = raw[:len(raw) - (len(raw) % 2)]
        x = (raw[0::2] + 1j * raw[1::2]).astype(np.complex128)
    else:
        x = np.fromfile(path, dtype=np.complex64).astype(np.complex128)
        clip = 0.0
    if lo_offset:
        x = x * np.exp(-2j * np.pi * (lo_offset / fs) * np.arange(len(x)))
    return x, clip


def main():
    ap = argparse.ArgumentParser(description="DVB-T Stage B: constellation + true SNR")
    ap.add_argument("--file")
    ap.add_argument("--fmt", choices=["u8", "cs8", "c64"], default="u8")
    ap.add_argument("--fs", type=float, default=3.2e6)
    ap.add_argument("--bw", type=float, default=2e6)
    ap.add_argument("--mode", choices=["2K", "8K"], default="2K")
    ap.add_argument("--gi", choices=list(GI_MAP), default="1/4")
    ap.add_argument("--lo-offset", type=float, default=0.0)
    ap.add_argument("--plot", action="store_true", help="save constellation_b.png")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()

    if a.selftest:
        sys.exit(selftest())
    if not a.file:
        sys.exit("give --file cap.bin (capture with rtl_sdr) or --selftest")

    x, clip = load_file(a.file, a.fmt, a.fs, a.lo_offset)
    print(f"loaded {len(x)} samples ({len(x)/a.fs:.2f} s); clipping {100*clip:.1f}%")
    if clip > 0.01:
        print("  WARNING: capture is clipping - MER will be pessimistic; add pad.")
    r = demodulate(x, a.fs, a.bw, a.mode, GI_MAP[a.gi])
    if r is None:
        sys.exit("could not demodulate enough symbols; check mode/gi/fs/bw.")

    print("\n================ DVB-T Stage-B ================")
    print(f"  symbols processed : {r['nsym']}")
    print(f"  fractional CFO    : {r['cfo_hz']:+.0f} Hz")
    print(f"  integer CFO       : {r['int_cfo']:+d} carriers")
    print(f"  sample-clock (SFO): {r['sfo_ppm']:+.1f} ppm (estimated + corrected)")
    print(f"  timing trim       : {r['tau']:+.2f} samples")
    print(f"  >>> MER / true SNR: {r['mer']:.1f} dB <<<")
    need = 6.0
    if r["mer"] >= need + 3:
        print(f"  VERDICT: clean, demodulable QPSK well above the ~{need:.0f} dB")
        print("           needed. If the RX won't lock, it's a PARAMETER problem")
        print("           (mode/GI/spectral inversion) or the RX itself - not the")
        print("           signal. The transmitter is doing its job.")
    elif r["mer"] >= need:
        print(f"  VERDICT: marginal (~{need:.0f} dB needed). Should lock with more")
        print("           level headroom; add TX power or reduce path loss.")
    else:
        print(f"  VERDICT: real SNR is genuinely low ({r['mer']:.1f} dB). This is a")
        print("           level/link problem: more TX power, less path loss, less")
        print("           clipping. Not a parameter issue.")
    print("===============================================\n")

    if a.plot:
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
            c = r["cons"][:20000]
            plt.figure(figsize=(6, 6))
            plt.plot(c.real, c.imag, ".", ms=1, alpha=0.3)
            for pt in [(1, 1), (1, -1), (-1, 1), (-1, -1)]:
                plt.plot(pt[0]/np.sqrt(2), pt[1]/np.sqrt(2), "r+", ms=14, mew=2)
            lim = 2.0
            plt.xlim(-lim, lim); plt.ylim(-lim, lim); plt.gca().set_aspect("equal")
            plt.grid(alpha=0.3)
            plt.title(f"DVB-T QPSK constellation - MER {r['mer']:.1f} dB")
            plt.tight_layout(); plt.savefig("constellation_b.png", dpi=130)
            print("wrote constellation_b.png")
        except ImportError:
            print("(matplotlib not installed; skipping plot)")


if __name__ == "__main__":
    main()