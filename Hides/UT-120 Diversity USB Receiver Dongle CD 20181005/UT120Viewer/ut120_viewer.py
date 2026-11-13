"""UT-120 Viewer: tune, browse channels, watch video and monitor link quality."""
from __future__ import annotations

import sys
import threading
import time
from collections import deque
from pathlib import Path

from PySide6.QtCore import QPointF, QRectF, QSettings, Qt, QTimer, Signal
from PySide6.QtGui import QColor, QFont, QImage, QPainter, QPalette, QPen, QPolygonF
from PySide6.QtWidgets import (QAbstractItemView, QApplication, QComboBox, QDoubleSpinBox, QFileDialog, QFormLayout,
                               QFrame, QGridLayout, QGroupBox, QHBoxLayout, QHeaderView, QLabel, QListWidget,
                               QListWidgetItem, QMainWindow, QMessageBox, QPushButton, QSplitter, QTableWidget,
                               QTableWidgetItem, QToolTip, QVBoxLayout, QWidget)

from ut120.sources import ENGINE, FileSource, LiveSource, list_tuners
from ut120.ts_analyzer import TsAnalyzer, assess
from ut120.video import VideoDecoder

ACCENT = "#5AA2F0"
STATUS = {"GOOD": "#3FB56F", "MARGINAL": "#E0A030", "BAD": "#E5534B", "NO SIGNAL": "#8A9099", "IDLE": "#8A9099"}
SCAN_NO_LOCK_S = 2.0
SCAN_PSI_S = 5.0


class VideoView(QWidget):
    def __init__(self):
        super().__init__()
        self.image = None
        self.message = "No video"
        self.setMinimumSize(480, 270)

    def show_image(self, image):
        self.image = image
        self.update()

    def clear(self, message):
        self.image, self.message = None, message
        self.update()

    def paintEvent(self, _):
        p = QPainter(self)
        p.fillRect(self.rect(), Qt.GlobalColor.black)
        if self.image is None:
            p.setPen(QColor("#8A9099"))
            p.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, self.message)
            return
        size = self.image.size().scaled(self.size(), Qt.AspectRatioMode.KeepAspectRatio)
        target = QRectF((self.width() - size.width()) / 2, (self.height() - size.height()) / 2,
                        size.width(), size.height())
        p.setRenderHint(QPainter.RenderHint.SmoothPixmapTransform)
        p.drawImage(target, self.image)


class History(QWidget):
    """One measurement over the last two minutes, baseline at zero."""

    def __init__(self, title, fmt, unit_floor=1.0):
        super().__init__()
        self.title, self.fmt, self.floor = title, fmt, unit_floor
        self.values = deque(maxlen=120)
        self.setMinimumHeight(66)
        self.setMouseTracking(True)

    def push(self, value):
        self.values.append(value)
        self.update()

    def clear(self):
        self.values.clear()
        self.update()

    def mouseMoveEvent(self, e):
        if self.values:
            step = max(self.width() - 1, 1) / (self.values.maxlen - 1)
            back = int(round((self.width() - e.position().x()) / step))
            if back < len(self.values):
                QToolTip.showText(e.globalPosition().toPoint(),
                                  f"{back} s ago: {self.fmt(self.values[-1 - back])}", self)

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)
        palette = self.palette()
        top, base = 20, self.height() - 3
        p.setPen(palette.color(QPalette.ColorRole.PlaceholderText))
        p.drawText(0, 13, self.title)
        p.setPen(palette.color(QPalette.ColorRole.WindowText))
        now = self.fmt(self.values[-1]) if self.values else "-"
        p.drawText(QRectF(0, 0, self.width(), 16), Qt.AlignmentFlag.AlignRight, now)
        p.setPen(QPen(QColor(128, 128, 128, 90), 1))
        p.drawLine(0, base, self.width(), base)
        if len(self.values) < 2:
            return
        peak = max(max(self.values), self.floor)
        step = (self.width() - 1) / (self.values.maxlen - 1)
        x0 = self.width() - 1 - step * (len(self.values) - 1)
        line = QPolygonF([QPointF(x0 + i * step, base - (v / peak) * (base - top))
                          for i, v in enumerate(self.values)])
        p.setPen(QPen(QColor(ACCENT), 2))
        p.drawPolyline(line)


def mbps(v):
    return f"{v / 1e6:.2f} Mb/s"


class MainWindow(QMainWindow):
    engine_event = Signal(dict)

    def __init__(self):
        super().__init__()
        self.setWindowTitle("UT-120 Viewer")
        self.resize(1360, 800)
        self.settings = QSettings("CameraStreaming", "UT120Viewer")
        self.analyzer = TsAnalyzer()
        self.decoder = VideoDecoder()
        self.source = None
        self.pending_tune = None
        self.signal = {}
        self.video_info = {}
        self.selected_program = None
        self.channel_keys = []
        self.recent_errors = deque(maxlen=10)
        self.record_file = None
        self.record_lock = threading.Lock()
        self.record_bytes = 0
        self.scan_queue = []
        self.scan_started = 0.0
        self.scan_flushed = False
        self.decoding = True

        self._build_ui()
        self.engine_event.connect(self.on_engine_event)
        self.decoder.frame.connect(self.video.show_image)
        self.decoder.info.connect(self.on_video_info)

        self.tick_timer = QTimer(self, interval=1000, timeout=self.tick)
        self.tick_timer.start()
        self.scan_timer = QTimer(self, interval=200, timeout=self.scan_tick)
        self.refresh_devices()

    # ------------------------------------------------------------- UI

    def _build_ui(self):
        split = QSplitter()
        self.setCentralWidget(split)

        left = QWidget()
        lv = QVBoxLayout(left)
        tuner = QGroupBox("Tuner")
        form = QFormLayout(tuner)
        self.device = QComboBox()
        refresh = QPushButton("Refresh", clicked=self.refresh_devices)
        row = QHBoxLayout()
        row.addWidget(self.device, 1)
        row.addWidget(refresh)
        form.addRow("Device", row)
        self.freq = self._mhz(float(self.settings.value("freq", 426.0)))
        form.addRow("Frequency", self.freq)
        self.bw = QComboBox()
        self.bw.addItems([f"{b} MHz" for b in (5, 6, 7, 8)])
        self.bw.setCurrentText(f"{self.settings.value('bw', 8)} MHz")
        form.addRow("Bandwidth", self.bw)
        buttons = QHBoxLayout()
        self.tune_btn = QPushButton("Tune", clicked=self.on_tune)
        self.tune_btn.setDefault(True)
        stop_btn = QPushButton("Stop", clicked=self.stop_source)
        open_btn = QPushButton("Open .ts…", clicked=self.on_open_file)
        for b in (self.tune_btn, stop_btn, open_btn):
            buttons.addWidget(b)
        form.addRow(buttons)
        lv.addWidget(tuner)

        scan = QGroupBox("Scan")
        sg = QGridLayout(scan)
        self.scan_from, self.scan_to = self._mhz(420.0), self._mhz(440.0)
        self.scan_step = self._mhz(50.0)
        self.scan_step.setRange(0.1, 100)
        self.scan_step.setValue(2.0)
        for col, (name, w) in enumerate((("From", self.scan_from), ("To", self.scan_to), ("Step", self.scan_step))):
            sg.addWidget(QLabel(name), 0, col)
            sg.addWidget(w, 1, col)
        self.scan_btn = QPushButton("Start scan", clicked=self.on_scan)
        sg.addWidget(self.scan_btn, 2, 0, 1, 3)
        self.found = QListWidget()
        self.found.setMaximumHeight(110)
        self.found.itemActivated.connect(self.on_found_activated)
        sg.addWidget(self.found, 3, 0, 1, 3)
        lv.addWidget(scan)

        channels = QGroupBox("Channels")
        cv = QVBoxLayout(channels)
        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["Name", "Streams", "Bitrate"])
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.table.verticalHeader().hide()
        self.table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.table.cellClicked.connect(self.on_channel_clicked)
        cv.addWidget(self.table)
        lv.addWidget(channels, 1)
        split.addWidget(left)

        center = QWidget()
        cl = QVBoxLayout(center)
        self.video = VideoView()
        cl.addWidget(self.video, 1)
        bar = QHBoxLayout()
        self.record_btn = QPushButton("Record", clicked=self.on_record)
        self.status_text = QLabel("Idle")
        bar.addWidget(self.record_btn)
        bar.addWidget(self.status_text, 1)
        cl.addLayout(bar)
        split.addWidget(center)

        right = QWidget()
        rv = QVBoxLayout(right)
        self.verdict = QLabel("IDLE")
        self.verdict.setAlignment(Qt.AlignmentFlag.AlignCenter)
        font = QFont()
        font.setPointSize(18)
        font.setBold(True)
        self.verdict.setFont(font)
        self.verdict.setFrameShape(QFrame.Shape.StyledPanel)
        self.verdict.setMinimumHeight(52)
        self.verdict_reason = QLabel("Tune a frequency or open a recording")
        self.verdict_reason.setWordWrap(True)
        self.verdict_reason.setAlignment(Qt.AlignmentFlag.AlignCenter)
        rv.addWidget(self.verdict)
        rv.addWidget(self.verdict_reason)

        grid = QFormLayout()
        self.metrics = {}
        for key, label in (("lock", "Demod lock"), ("strength", "Signal strength"), ("quality", "Signal quality"),
                           ("snr", "SNR"), ("ber", "Bit error rate"), ("aborts", "RS uncorrectable"),
                           ("chips", "Diversity"), ("service", "Service bitrate"), ("null", "Null packets"),
                           ("tei", "Corrupt packets"), ("cc", "Lost packets"), ("sync", "Sync losses"),
                           ("video", "Video"), ("fps", "Frame rate"), ("verr", "Decode errors"),
                           ("drop", "Engine dropped")):
            value = QLabel("-")
            value.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
            self.metrics[key] = value
            grid.addRow(label, value)
        rv.addLayout(grid)

        self.hist_bitrate = History("Mux bitrate", mbps, 1e6)
        self.hist_quality = History("Signal quality", lambda v: f"{v:.0f} / 100", 100)
        self.hist_errors = History("Packet errors", lambda v: f"{v:.0f} /s", 5)
        for h in (self.hist_bitrate, self.hist_quality, self.hist_errors):
            rv.addWidget(h)
        rv.addStretch(1)
        split.addWidget(right)
        split.setSizes([340, 760, 300])
        split.setStretchFactor(1, 1)

    @staticmethod
    def _mhz(value):
        box = QDoubleSpinBox()
        box.setRange(50.0, 2000.0)
        box.setDecimals(3)
        box.setSuffix(" MHz")
        box.setValue(value)
        return box

    def set_verdict(self, verdict, reason):
        self.verdict.setText(verdict)
        self.verdict.setStyleSheet(f"color: white; background: {STATUS[verdict]}; border-radius: 4px;")
        self.verdict_reason.setText(reason)

    # ------------------------------------------------------------- data path

    def on_ts(self, data):
        self.analyzer.feed(data)
        if self.decoding:
            self.decoder.write(data)
        with self.record_lock:
            if self.record_file:
                self.record_file.write(data)
                self.record_bytes += len(data)

    def on_raw_event(self, event):
        self.engine_event.emit(event)

    def on_engine_event(self, e):
        kind = e.get("event")
        if kind == "ready" and isinstance(self.source, LiveSource):
            self.source.send(f"open {max(self.device.currentIndex(), 0)}")
        elif kind == "opened":
            info = e.get("info", {})
            self.status_text.setText(f"Opened {e['name']}: {info.get('architecture', '?')}, {info.get('chips', '?')} chip(s)")
            if self.pending_tune:
                self.source.send("tune {} {}".format(*self.pending_tune))
                self.pending_tune = None
        elif kind == "tuned":
            self.status_text.setText(f"Tuned {e['khz'] / 1000:.3f} MHz, {e['bw']} MHz bandwidth")
        elif kind == "signal":
            self.signal = e
        elif kind == "error":
            self.status_text.setText(f"Error: {e.get('message')} {e.get('hr', '')}")
        elif kind == "eof":
            self.decoder.finish()
            self.status_text.setText("End of recording")
        elif kind == "exited" and self.source is not None and isinstance(self.source, LiveSource):
            self.status_text.setText("Engine exited")

    def _reset_views(self, message):
        self.analyzer.reset()
        self.signal = {}
        self.video_info = {}
        self.selected_program = None
        self.channel_keys = []
        self.recent_errors.clear()
        self.table.setRowCount(0)
        self.video.clear(message)
        for h in (self.hist_bitrate, self.hist_quality, self.hist_errors):
            h.clear()

    def refresh_devices(self):
        self.device.clear()
        if not ENGINE.exists():
            self.status_text.setText(f"Engine not built: {ENGINE}")
            return
        try:
            tuners = list_tuners()
        except Exception as exc:  # engine crash must not stop the GUI from opening
            self.status_text.setText(f"Could not list tuners: {exc}")
            return
        self.tuners = tuners
        self.device.addItems([t["name"] + ("" if t.get("ready", True) else "  (needs WinUSB driver)") for t in tuners])
        ready = sum(1 for t in tuners if t.get("ready", True))
        if not tuners:
            self.status_text.setText("No dongle found. Plug in the UT-120 and press Refresh")
        elif not ready:
            self.status_text.setText("UT-120 found, but it is not on the WinUSB driver yet. See README.md")
        else:
            self.status_text.setText(f"{ready} dongle(s) ready")

    def selected_ready(self):
        i = self.device.currentIndex()
        tuners = getattr(self, "tuners", [])
        if 0 <= i < len(tuners) and not tuners[i].get("ready", True):
            QMessageBox.information(self, "UT-120 Viewer",
                                    "The UT-120 is still assigned to ITE's driver, which Windows will not load.\n"
                                    "Switch it to Microsoft's WinUSB driver as described in README.md, then press Refresh.")
            return False
        return True

    def stop_source(self):
        self.stop_scan()
        if self.source:
            self.source.stop()
            self.source = None
        self.decoder.stop()
        self.set_record(False)
        self._reset_views("No video")
        self.set_verdict("IDLE", "Tune a frequency or open a recording")
        self.status_text.setText("Stopped")

    def _tune(self, khz, bw, decode=True):
        self.decoding = decode
        self.analyzer.reset()
        self.decoder.stop()
        self._reset_views("Waiting for video…")
        if decode:
            self.decoder.start()
        if isinstance(self.source, LiveSource):
            self.source.send(f"tune {khz} {bw}")
            return
        if self.source:
            self.source.stop()
        self.pending_tune = (khz, bw)
        self.source = LiveSource(self.on_ts, self.on_raw_event)

    def current_bw(self):
        return int(self.bw.currentText().split()[0])

    def on_tune(self):
        self.stop_scan()
        if self.device.count() == 0:
            self.refresh_devices()
        if self.device.count() == 0:
            QMessageBox.information(self, "UT-120 Viewer", "No tuner found.\nPlug in the UT-120, wait a few seconds, then press Refresh.")
            return
        if not self.selected_ready():
            return
        self.settings.setValue("freq", self.freq.value())
        self.settings.setValue("bw", self.current_bw())
        self._tune(round(self.freq.value() * 1000), self.current_bw())

    def open_file(self, path):
        self.stop_source()
        self._reset_views("Waiting for video…")
        self.decoding = True
        self.decoder.start()
        self.source = FileSource(path, self.on_ts, self.on_raw_event)
        self.status_text.setText(f"Replaying {Path(path).name}")

    def on_open_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Open recording", "", "Transport stream (*.ts *.mts *.m2ts);;All files (*)")
        if path:
            self.open_file(path)

    def on_channel_clicked(self, row, _):
        if row >= len(self.channel_keys):
            return
        number = self.channel_keys[row]
        if number == self.selected_program:
            return
        self.selected_program = number
        program = self.analyzer.programs.get(number)
        self.video.clear("Switching channel…")
        self.video_info = {}
        self.decoding = True
        self.decoder.start(program.video_pid if program else None)

    # ------------------------------------------------------------- scan

    def on_scan(self):
        if self.scan_timer.isActive():
            self.stop_scan()
            return
        if self.device.count() == 0:
            self.refresh_devices()
        if self.device.count() == 0:
            QMessageBox.information(self, "UT-120 Viewer", "No tuner found.")
            return
        if not self.selected_ready():
            return
        low, high, step = (round(b.value() * 1000) for b in (self.scan_from, self.scan_to, self.scan_step))
        self.scan_queue = list(range(low, high + 1, step))
        self.found.clear()
        self.scan_btn.setText("Stop scan")
        self.scan_next()
        self.scan_timer.start()

    def stop_scan(self):
        self.scan_timer.stop()
        self.scan_queue = []
        self.scan_btn.setText("Start scan")

    def scan_next(self):
        if not self.scan_queue:
            self.stop_scan()
            self.status_text.setText(f"Scan finished: {self.found.count()} found")
            return
        self.scan_khz = self.scan_queue.pop(0)
        self.scan_started = time.monotonic()
        self.scan_flushed = False
        self._tune(self.scan_khz, self.current_bw(), decode=False)
        self.status_text.setText(f"Scanning {self.scan_khz / 1000:.3f} MHz…")

    def scan_tick(self):
        elapsed = time.monotonic() - self.scan_started
        if elapsed < 0.6:
            return
        if not self.scan_flushed:  # discard data still in flight from the previous frequency
            self.scan_flushed = True
            self.signal = {}
            self.analyzer.reset()
            return
        programs = list(self.analyzer.programs.values())
        receiving = self.signal.get("locked") or self.analyzer.packets > self.analyzer.nulls
        named = programs and all(p.name for p in programs)
        if named or (programs and elapsed >= SCAN_PSI_S):
            names = ", ".join(p.name or f"Program {p.number}" for p in programs)
            item = QListWidgetItem(f"{self.scan_khz / 1000:.3f} MHz: {names}")
            item.setData(Qt.ItemDataRole.UserRole, self.scan_khz)
            self.found.addItem(item)
            self.scan_next()
        elif elapsed >= (SCAN_PSI_S if receiving else SCAN_NO_LOCK_S):
            self.scan_next()

    def on_found_activated(self, item):
        self.freq.setValue(item.data(Qt.ItemDataRole.UserRole) / 1000)
        self.on_tune()

    # ------------------------------------------------------------- recording

    def on_record(self):
        if self.record_file:
            self.set_record(False)
            return
        name = f"UT120_{self.freq.value():.3f}MHz_{time.strftime('%Y%m%d_%H%M%S')}.ts"
        path, _ = QFileDialog.getSaveFileName(self, "Record to", str(Path.home() / "Videos" / name), "Transport stream (*.ts)")
        if path:
            self.set_record(True, path)

    def set_record(self, on, path=None):
        with self.record_lock:
            if self.record_file:
                self.record_file.close()
                self.record_file = None
            if on:
                self.record_file = open(path, "wb")
                self.record_bytes = 0
        self.record_btn.setText("Stop recording" if on else "Record")

    # ------------------------------------------------------------- periodic

    def on_video_info(self, info):
        self.video_info = info

    def tick(self):
        if not self.source:
            return
        snap = self.analyzer.snapshot()
        self.update_channels(snap)
        live = isinstance(self.source, LiveSource)
        sig = self.signal
        errors = snap["tei_rate"] + snap["cc_rate"]
        self.recent_errors.append(errors > 0)
        locked = bool(sig.get("locked")) or not live
        quality = sig.get("quality", 0)

        if not self.scan_timer.isActive():
            self.set_verdict(*assess(locked, snap["tei_rate"], snap["cc_rate"], quality, snap["bitrate"], any(self.recent_errors)))

        m = self.metrics
        if live:
            chips = sig.get("chips", [])
            c0 = chips[0] if chips else None
            m["lock"].setText("Locked" if sig.get("locked") else ("Signal present" if sig.get("present") else "No lock"))
            m["strength"].setText(f"{c0['dbm']} dBm  ({sig.get('strength')} / 100)" if c0 else "-")
            m["quality"].setText(f"{quality} / 100" if sig else "-")
            m["snr"].setText(f"{c0['snr']} dB" if c0 else "-")
            m["ber"].setText(f"{c0['ber']:.1e}" if c0 else "-")
            m["aborts"].setText(str(sum(c["aborts"] for c in chips)) if chips else "-")
            m["chips"].setText("  |  ".join(f"#{i + 1} {'lock' if c['locked'] else 'no lock'} {c['dbm']} dBm"
                                             for i, c in enumerate(chips)) if len(chips) > 1 else "single chip")
            m["drop"].setText(f"{sig.get('dropped', 0)} bytes")
        else:
            for key in ("lock", "strength", "quality", "snr", "ber", "aborts", "chips", "drop"):
                m[key].setText("n/a (recording)")
        service = snap["program_bps"].get(self.selected_program) if self.selected_program else None
        if service is None and snap["program_bps"]:
            service = max(snap["program_bps"].values())
        m["service"].setText(mbps(service) if service is not None else "-")
        m["null"].setText(f"{snap['null_pct']:.1f} %")
        m["tei"].setText(f"{snap['tei_rate']:.0f} /s  (total {snap['total_tei']})")
        m["cc"].setText(f"{snap['cc_rate']:.0f} /s  (total {snap['total_cc']})")
        m["sync"].setText(str(snap["total_sync"]))
        v = self.video_info
        m["video"].setText(f"{v['codec']} {v['width']}×{v['height']}" if v else "-")
        m["fps"].setText(f"{v['fps']:.1f}" if v else "-")
        m["verr"].setText(str(v["total_errors"]) if v else "-")

        self.hist_bitrate.push(snap["bitrate"])
        self.hist_quality.push(quality)
        self.hist_errors.push(errors)
        if self.record_file:
            self.status_text.setText(f"Recording: {self.record_bytes / 1e6:.1f} MB")

    def update_channels(self, snap):
        programs = sorted(snap["programs"], key=lambda p: p.number)
        keys = [p.number for p in programs]
        if keys != self.channel_keys:
            self.channel_keys = keys
            self.table.setRowCount(len(keys))
        for row, p in enumerate(programs):
            streams = " + ".join(s.codec for s in p.streams if s.kind in ("video", "audio"))
            for col, text in enumerate((p.name or f"Program {p.number}", streams,
                                        mbps(snap["program_bps"].get(p.number, 0)))):
                item = self.table.item(row, col)
                if item is None:
                    self.table.setItem(row, col, QTableWidgetItem(text))
                elif item.text() != text:
                    item.setText(text)

    def closeEvent(self, event):
        self.stop_source()
        event.accept()


def dark_palette():
    p = QPalette()
    for role, color in ((QPalette.ColorRole.Window, "#1F2228"), (QPalette.ColorRole.WindowText, "#E6E8EB"),
                        (QPalette.ColorRole.Base, "#16181D"), (QPalette.ColorRole.AlternateBase, "#23262D"),
                        (QPalette.ColorRole.Text, "#E6E8EB"), (QPalette.ColorRole.Button, "#2B2F37"),
                        (QPalette.ColorRole.ButtonText, "#E6E8EB"), (QPalette.ColorRole.Highlight, ACCENT),
                        (QPalette.ColorRole.HighlightedText, "#101215"), (QPalette.ColorRole.ToolTipBase, "#2B2F37"),
                        (QPalette.ColorRole.ToolTipText, "#E6E8EB"), (QPalette.ColorRole.PlaceholderText, "#9AA0AA")):
        p.setColor(role, QColor(color))
    return p


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    app.setPalette(dark_palette())
    window = MainWindow()
    window.show()
    if len(sys.argv) > 1:
        window.open_file(sys.argv[1])
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
