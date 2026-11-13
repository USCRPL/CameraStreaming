import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from ut120.ts_analyzer import TS_SIZE, TsAnalyzer, assess, crc32_mpeg


def packet(pid, cc, tei=False, payload=b"", pusi=False):
    head = bytes([0x47, (0x80 if tei else 0) | (0x40 if pusi else 0) | (pid >> 8), pid & 0xFF, 0x10 | (cc & 15)])
    return head + payload + b"\xFF" * (TS_SIZE - 4 - len(payload))


def section(table, ext, body):
    length = 5 + len(body) + 4
    data = bytes([table, 0xB0 | (length >> 8), length & 0xFF, ext >> 8, ext & 0xFF, 0xC1, 0, 0]) + body
    crc = crc32_mpeg(data)
    return data + crc.to_bytes(4, "big")


class AnalyzerTests(unittest.TestCase):
    def test_clean_stream_has_no_errors(self):
        a = TsAnalyzer()
        a.feed(b"".join(packet(256, i) for i in range(40)))
        self.assertEqual((a.packets, a.cc_errors, a.tei, a.sync_losses), (40, 0, 0, 0))

    def test_detects_lost_packets_across_chunks(self):
        a = TsAnalyzer()
        data = b"".join(packet(256, i) for i in (0, 1, 2, 5, 6, 7, 8, 9))
        a.feed(data[:300])
        a.feed(data[300:])
        self.assertEqual(a.cc_errors, 1)
        self.assertEqual(a.packets, 8)

    def test_counts_tei_and_nulls(self):
        a = TsAnalyzer()
        a.feed(packet(256, 0) + packet(256, 1, tei=True) + packet(0x1FFF, 0) + packet(256, 1) + packet(256, 2))
        self.assertEqual((a.tei, a.nulls, a.cc_errors), (1, 1, 0))

    def test_resynchronises_after_garbage(self):
        a = TsAnalyzer()
        a.feed(b"".join(packet(256, i) for i in range(5)) + b"\x00" * 50 + b"".join(packet(256, i) for i in range(5, 12)))
        self.assertEqual(a.packets, 12)
        self.assertEqual(a.sync_losses, 1)

    def test_parses_pat_pmt_sdt(self):
        pat = section(0x00, 1, bytes([0, 7, 0xE1, 0x00]))
        pmt = section(0x02, 7, bytes([0xE1, 0x01, 0xF0, 0, 0x1B, 0xE1, 0x01, 0xF0, 0, 0x0F, 0xE1, 0x02, 0xF0, 0]))
        name = b"CAM1"
        desc = bytes([0x48, 3 + len(name), 1, 0]) + bytes([len(name)]) + name
        sdt = section(0x42, 1, bytes([0, 1, 0xFF]) + bytes([0, 7, 0xFC, 0x80 | (len(desc) >> 8), len(desc)]) + desc)
        a = TsAnalyzer()
        a.feed(packet(0, 0, payload=b"\0" + pat, pusi=True))
        a.feed(packet(0x100, 0, payload=b"\0" + pmt, pusi=True))
        a.feed(packet(0x11, 0, payload=b"\0" + sdt, pusi=True) + b"".join(packet(256, i) for i in range(3)))
        p = a.programs[7]
        self.assertEqual(p.name, "CAM1")
        self.assertEqual(p.video_pid, 0x101)
        self.assertEqual([s.codec for s in p.streams], ["H.264", "AAC"])

    def test_rejects_section_with_bad_crc(self):
        pat = bytearray(section(0x00, 1, bytes([0, 7, 0xE1, 0x00])))
        pat[9] ^= 1
        a = TsAnalyzer()
        a.feed(packet(0, 0, payload=b"\0" + bytes(pat), pusi=True) + packet(256, 0) + packet(256, 1))
        self.assertEqual(a.programs, {})


class AssessTests(unittest.TestCase):
    def test_verdicts(self):
        self.assertEqual(assess(False, 0, 0, 0, 0, False)[0], "NO SIGNAL")
        self.assertEqual(assess(True, 0, 0, 90, 5e6, False)[0], "GOOD")
        self.assertEqual(assess(True, 0, 0, 0, 5e6, False)[0], "GOOD")
        self.assertEqual(assess(True, 0, 0, 90, 5e6, True)[0], "MARGINAL")
        self.assertEqual(assess(True, 0, 0, 30, 5e6, False)[0], "MARGINAL")
        self.assertEqual(assess(True, 50, 0, 90, 5e6, True)[0], "BAD")


if __name__ == "__main__":
    unittest.main()
