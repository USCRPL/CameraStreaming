"""TS sources: the live BDA engine process and paced replay of a recording."""
from __future__ import annotations

import json
import subprocess
import threading
import time
from pathlib import Path

from .ts_analyzer import TS_SIZE, estimate_file_bitrate

ENGINE = Path(__file__).resolve().parent.parent / "engine" / "ut120_engine.exe"
_NO_WINDOW = subprocess.CREATE_NO_WINDOW


def list_tuners():
    out = subprocess.run([str(ENGINE), "list"], capture_output=True, text=True, encoding="utf-8",
                         timeout=15, creationflags=_NO_WINDOW)
    return json.loads(out.stdout or "[]")


class LiveSource:
    def __init__(self, on_ts, on_event):
        self._on_ts = on_ts
        self._on_event = on_event
        self._proc = subprocess.Popen([str(ENGINE), "run"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, creationflags=_NO_WINDOW, bufsize=0)
        threading.Thread(target=self._read_ts, daemon=True).start()
        threading.Thread(target=self._read_events, daemon=True).start()

    def send(self, line):
        try:
            self._proc.stdin.write((line + "\n").encode())
            self._proc.stdin.flush()
        except OSError:
            pass

    def _read_ts(self):
        while True:
            data = self._proc.stdout.read(188 * 512)
            if not data:
                return
            self._on_ts(data)

    def _read_events(self):
        for line in self._proc.stderr:
            try:
                self._on_event(json.loads(line))
            except ValueError:
                pass
        self._on_event({"event": "exited"})

    def stop(self):
        self.send("quit")
        try:
            self._proc.stdin.close()
            self._proc.wait(5)
        except (OSError, subprocess.TimeoutExpired):
            self._proc.kill()


class FileSource:
    """Replays a .ts recording at its own bit rate so all displays behave as if live."""

    def __init__(self, path, on_ts, on_event):
        self._path = path
        self._on_ts = on_ts
        self._on_event = on_event
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        rate = estimate_file_bitrate(self._path) / 8.0
        chunk = TS_SIZE * 256
        started = time.monotonic()
        sent = 0
        with open(self._path, "rb") as f:
            while not self._stop.is_set():
                data = f.read(chunk)
                if not data:
                    break
                self._on_ts(data)
                sent += len(data)
                delay = started + sent / rate - time.monotonic()
                if delay > 0:
                    self._stop.wait(delay)
        self._on_event({"event": "eof"})

    def stop(self):
        self._stop.set()
        self._thread.join(2)
