"""MPEG-TS health measurement and PSI/SI parsing (channel discovery)."""
from __future__ import annotations

import threading
import time
from collections import defaultdict
from dataclasses import dataclass, field

import numpy as np

TS_SIZE = 188
SYNC = 0x47
NULL_PID = 0x1FFF
PAT_PID = 0x0000
SDT_PID = 0x0011

STREAM_TYPES = {
    0x01: ("video", "MPEG-1"), 0x02: ("video", "MPEG-2"), 0x10: ("video", "MPEG-4"),
    0x1B: ("video", "H.264"), 0x24: ("video", "HEVC"),
    0x03: ("audio", "MP1"), 0x04: ("audio", "MP2"), 0x0F: ("audio", "AAC"),
    0x11: ("audio", "AAC-LATM"), 0x81: ("audio", "AC-3"),
}

_CRC_TABLE = []
for _i in range(256):
    _c = _i << 24
    for _ in range(8):
        _c = ((_c << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if _c & 0x80000000 else (_c << 1) & 0xFFFFFFFF
    _CRC_TABLE.append(_c)


def crc32_mpeg(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for b in data:
        crc = ((crc << 8) & 0xFFFFFFFF) ^ _CRC_TABLE[((crc >> 24) ^ b) & 0xFF]
    return crc


@dataclass
class Stream:
    pid: int
    kind: str
    codec: str


@dataclass
class Program:
    number: int
    pmt_pid: int
    name: str = ""
    provider: str = ""
    pcr_pid: int = 0
    streams: list = field(default_factory=list)

    @property
    def video_pid(self):
        return next((s.pid for s in self.streams if s.kind == "video"), None)

    def pids(self):
        return {self.pmt_pid, self.pcr_pid, *(s.pid for s in self.streams)}


def find_sync(buf: bytes, start: int = 0):
    """Offset of the first position with three consecutive sync bytes, or None."""
    end = len(buf) - 2 * TS_SIZE
    pos = buf.find(SYNC, start)
    while pos != -1 and pos < end:
        if buf[pos + TS_SIZE] == SYNC and buf[pos + 2 * TS_SIZE] == SYNC:
            return pos
        pos = buf.find(SYNC, pos + 1)
    return None


class TsAnalyzer:
    """Feed arbitrary byte chunks with feed(); read rates with snapshot()."""

    def __init__(self):
        self._lock = threading.Lock()
        self.reset()

    def reset(self):
        with self._lock:
            self._buf = b""
            self._last_cc = {}
            self._sections = {}
            self._section_seen = {}
            self.programs = {}
            self.packets = self.nulls = self.tei = self.cc_errors = self.sync_losses = 0
            self._pid_packets = defaultdict(int)
            self._prev = None
            self._prev_time = time.monotonic()

    def feed(self, data: bytes):
        with self._lock:
            buf = self._buf + data
            pos = 0
            while True:
                aligned = find_sync(buf, pos)
                if aligned is None:
                    # Keep enough tail to find sync across the next chunk.
                    self._buf = buf[max(pos, len(buf) - 2 * TS_SIZE - 1):]
                    return
                if aligned != pos and self.packets:
                    self.sync_losses += 1
                count = (len(buf) - aligned) // TS_SIZE
                arr = np.frombuffer(buf, np.uint8, count * TS_SIZE, aligned).reshape(count, TS_SIZE)
                bad = np.flatnonzero(arr[:, 0] != SYNC)
                if bad.size:
                    good = int(bad[0])
                    self._process(arr[:good])
                    pos = aligned + good * TS_SIZE  # find_sync counts the loss on the next pass
                    continue
                self._process(arr)
                self._buf = buf[aligned + count * TS_SIZE:]
                return

    def _process(self, arr):
        if not len(arr):
            return
        pid = ((arr[:, 1].astype(np.int32) & 0x1F) << 8) | arr[:, 2]
        tei = (arr[:, 1] & 0x80) != 0
        afc = (arr[:, 3] >> 4) & 3
        cc = arr[:, 3] & 0x0F
        self.packets += len(arr)
        self.tei += int(tei.sum())
        self.nulls += int((pid == NULL_PID).sum())

        discont = ((afc & 2) != 0) & (arr[:, 4] > 0) & ((arr[:, 5] & 0x80) != 0)
        for p in np.unique(pid):
            mask = pid == p
            p = int(p)
            self._pid_packets[p] += int(mask.sum())
            if p == NULL_PID:
                continue
            usable = mask & ~tei & ((afc & 1) != 0)
            seq = cc[usable].astype(np.int16)
            if not seq.size:
                continue
            prev = self._last_cc.get(p)
            if prev is not None:
                seq = np.concatenate(([prev], seq))
                reset = np.concatenate(([False], discont[usable]))
            else:
                reset = discont[usable]
            step = (seq[1:] - seq[:-1]) & 0x0F
            self.cc_errors += int(((step != 1) & (step != 0) & ~reset[1:]).sum())
            self._last_cc[p] = int(seq[-1])

        # PAT first: a PMT or SDT in this same batch may depend on what it declares.
        clean = ~tei & ((afc & 1) != 0)
        for i in np.flatnonzero((pid == PAT_PID) & clean):
            self._psi_packet(PAT_PID, arr[i].tobytes())
        wanted = {pg.pmt_pid for pg in self.programs.values()}
        for i in np.flatnonzero(np.isin(pid, list(wanted)) & clean):
            self._psi_packet(int(pid[i]), arr[i].tobytes())
        for i in np.flatnonzero((pid == SDT_PID) & clean):
            self._psi_packet(SDT_PID, arr[i].tobytes())

    # ------------------------------------------------------------- PSI

    def _psi_packet(self, pid, pkt):
        offset = 4
        if pkt[3] & 0x20:
            offset += 1 + pkt[4]
        if offset >= TS_SIZE:
            return
        if pkt[1] & 0x40:
            pointer = pkt[offset]
            start = offset + 1 + pointer
            if start >= TS_SIZE:
                self._sections.pop(pid, None)
                return
            if pid in self._sections and pointer:
                self._sections[pid] += pkt[offset + 1:start]
                self._drain(pid)
            self._sections[pid] = pkt[start:]
            self._drain(pid)
        elif pid in self._sections:
            self._sections[pid] += pkt[offset:]
            self._drain(pid)

    def _drain(self, pid):
        buf = self._sections[pid]
        while len(buf) >= 3:
            if buf[0] == 0xFF:
                buf = b""
                break
            length = 3 + (((buf[1] & 0x0F) << 8) | buf[2])
            if len(buf) < length:
                break
            section = buf[:length]
            buf = buf[length:]
            if crc32_mpeg(section) == 0:
                self._section(pid, section)
        self._sections[pid] = buf

    def _section(self, pid, s):
        table = s[0]
        if pid == PAT_PID and table == 0x00:
            for i in range(8, len(s) - 4, 4):
                number = (s[i] << 8) | s[i + 1]
                pmt = ((s[i + 2] & 0x1F) << 8) | s[i + 3]
                if number and (number not in self.programs or self.programs[number].pmt_pid != pmt):
                    self.programs[number] = Program(number, pmt)
        elif pid == SDT_PID and table == 0x42:
            self._sdt(s)
        elif table == 0x02:
            number = (s[3] << 8) | s[4]
            if number in self.programs and self.programs[number].pmt_pid == pid:
                self._pmt(self.programs[number], s)

    def _pmt(self, program, s):
        program.pcr_pid = ((s[8] & 0x1F) << 8) | s[9]
        i = 12 + (((s[10] & 0x0F) << 8) | s[11])
        streams = []
        while i + 5 <= len(s) - 4:
            stype = s[i]
            pid = ((s[i + 1] & 0x1F) << 8) | s[i + 2]
            es_len = ((s[i + 3] & 0x0F) << 8) | s[i + 4]
            kind, codec = STREAM_TYPES.get(stype, ("other", f"0x{stype:02X}"))
            if stype == 0x06:
                tags = _descriptor_tags(s[i + 5:i + 5 + es_len])
                if 0x6A in tags:
                    kind, codec = "audio", "AC-3"
                elif 0x7A in tags:
                    kind, codec = "audio", "E-AC-3"
            streams.append(Stream(pid, kind, codec))
            i += 5 + es_len
        program.streams = streams

    def _sdt(self, s):
        i = 11
        while i + 5 <= len(s) - 4:
            number = (s[i] << 8) | s[i + 1]
            loop = ((s[i + 3] & 0x0F) << 8) | s[i + 4]
            d = i + 5
            end = d + loop
            while d + 2 <= end:
                tag, length = s[d], s[d + 1]
                if tag == 0x48 and number in self.programs:
                    prov_len = s[d + 3]
                    name_len = s[d + 4 + prov_len]
                    self.programs[number].provider = _text(s[d + 4:d + 4 + prov_len])
                    self.programs[number].name = _text(s[d + 5 + prov_len:d + 5 + prov_len + name_len])
                d += 2 + length
            i = end

    # ------------------------------------------------------------- stats

    def snapshot(self):
        """Cumulative counters plus per-second rates since the previous call."""
        with self._lock:
            now = time.monotonic()
            cur = {"packets": self.packets, "nulls": self.nulls, "tei": self.tei,
                   "cc": self.cc_errors, "sync": self.sync_losses,
                   "pids": dict(self._pid_packets)}
            prev = self._prev or {"packets": 0, "nulls": 0, "tei": 0, "cc": 0, "sync": 0, "pids": {}}
            dt = max(now - self._prev_time, 1e-3)
            self._prev, self._prev_time = cur, now
            packets = cur["packets"] - prev["packets"]
            nulls = cur["nulls"] - prev["nulls"]
            program_bps = {}
            for number, program in self.programs.items():
                delta = sum(cur["pids"].get(p, 0) - prev["pids"].get(p, 0) for p in program.pids())
                program_bps[number] = delta * TS_SIZE * 8 / dt
            return {
                "bitrate": packets * TS_SIZE * 8 / dt,
                "null_pct": 100.0 * nulls / packets if packets else 0.0,
                "tei_rate": (cur["tei"] - prev["tei"]) / dt,
                "cc_rate": (cur["cc"] - prev["cc"]) / dt,
                "sync_rate": (cur["sync"] - prev["sync"]) / dt,
                "program_bps": program_bps,
                "total_packets": cur["packets"], "total_tei": cur["tei"],
                "total_cc": cur["cc"], "total_sync": cur["sync"],
                "programs": list(self.programs.values()),
            }


def _descriptor_tags(data):
    tags, i = set(), 0
    while i + 2 <= len(data):
        tags.add(data[i])
        i += 2 + data[i + 1]
    return tags


def _text(raw: bytes) -> str:
    if raw and raw[0] < 0x20:
        raw = raw[1:]
    return raw.decode("latin-1", "replace").strip()


def assess(locked, tei_rate, cc_rate, quality, bitrate, recent_errors):
    """Reduce the link measurements to (verdict, reason). `quality` is 0-100, 0 if unreported."""
    if not locked and bitrate <= 0:
        return "NO SIGNAL", "Demodulator not locked, no data"
    if bitrate <= 0:
        return "NO SIGNAL", "Locked but no transport stream data"
    if tei_rate > 10 or cc_rate > 10:
        return "BAD", f"{tei_rate:.0f} corrupt / {cc_rate:.0f} lost packets per second"
    if tei_rate > 0 or cc_rate > 0 or recent_errors:
        return "MARGINAL", "Occasional packet errors in the last 10 s"
    if 0 < quality < 50:
        return "MARGINAL", f"Low reported signal quality ({quality})"
    return "GOOD", "No packet errors in the last 10 s"


def estimate_file_bitrate(path, sample=16 * 1024 * 1024, default=5_000_000):
    """Nominal bit rate of a recording from its PCRs, used to replay it in real time."""
    with open(path, "rb") as f:
        buf = f.read(sample)
    start = find_sync(buf)
    if start is None:
        return default
    count = (len(buf) - start) // TS_SIZE
    arr = np.frombuffer(buf, np.uint8, count * TS_SIZE, start).reshape(count, TS_SIZE)
    has_pcr = (((arr[:, 3] >> 4) & 2) != 0) & (arr[:, 4] >= 7) & ((arr[:, 5] & 0x10) != 0) & (arr[:, 0] == SYNC)
    idx = np.flatnonzero(has_pcr)
    if idx.size < 3:
        return default
    pid = ((arr[idx, 1].astype(np.int32) & 0x1F) << 8) | arr[idx, 2]
    # Null packets are 0xFF-stuffed, which reads as a valid PCR field.
    idx, pid = idx[pid != NULL_PID], pid[pid != NULL_PID]
    if idx.size < 3:
        return default
    idx = idx[pid == np.bincount(pid).argmax()]
    if idx.size < 3:
        return default
    r = arr[idx]
    base = (r[:, 6].astype(np.uint64) << np.uint64(25)) | (r[:, 7].astype(np.uint64) << np.uint64(17)) | \
           (r[:, 8].astype(np.uint64) << np.uint64(9)) | (r[:, 9].astype(np.uint64) << np.uint64(1)) | \
           (r[:, 10].astype(np.uint64) >> np.uint64(7))
    dt = np.diff(base.astype(np.float64)) / 90000.0
    db = np.diff(idx) * TS_SIZE * 8.0
    valid = (dt > 0) & (dt < 1.0)
    # Total bits over total time: PCR gaps are uneven, so a per-gap median is badly biased.
    return float(db[valid].sum() / dt[valid].sum()) if valid.any() else default
