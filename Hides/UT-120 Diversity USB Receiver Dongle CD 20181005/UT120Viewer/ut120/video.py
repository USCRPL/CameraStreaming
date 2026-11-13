"""PyAV video decoding of a live TS byte stream."""
from __future__ import annotations

import threading
import time
from collections import deque

import av
from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QImage

_OPTIONS = {"probesize": "1500000", "analyzeduration": "1000000", "fflags": "nobuffer+discardcorrupt"}
_MAX_BACKLOG = 16 * 1024 * 1024


class TsPipe:
    """Blocking file-like object. Drops the oldest data rather than falling behind live."""

    def __init__(self):
        self._chunks = deque()
        self._size = 0
        self._closed = False
        self._cv = threading.Condition()
        self.dropped = 0

    def write(self, data):
        with self._cv:
            if self._closed:
                return
            self._chunks.append(data)
            self._size += len(data)
            while self._size > _MAX_BACKLOG:
                old = self._chunks.popleft()
                self._size -= len(old)
                self.dropped += len(old)
            self._cv.notify()

    def read(self, n):
        with self._cv:
            while not self._chunks and not self._closed:
                self._cv.wait()
            if not self._chunks:
                return b""
            first = self._chunks.popleft()
            if len(first) > n:
                self._chunks.appendleft(first[n:])
                first = first[:n]
            self._size -= len(first)
            return first

    def close(self, discard=False):
        with self._cv:
            self._closed = True
            if discard:
                self._chunks.clear()
                self._size = 0
            self._cv.notify_all()


class VideoDecoder(QObject):
    frame = Signal(QImage)
    info = Signal(dict)

    def __init__(self):
        super().__init__()
        self._pipe = None
        self._thread = None
        self._stop = threading.Event()

    def start(self, video_pid=None):
        self.stop()
        self._stop = threading.Event()
        self._pipe = TsPipe()
        self._thread = threading.Thread(target=self._run, args=(self._pipe, self._stop, video_pid), daemon=True)
        self._thread.start()

    def write(self, data):
        pipe = self._pipe
        if pipe:
            pipe.write(data)

    def finish(self):
        """Source ended: decode what is queued, then stop."""
        if self._pipe:
            self._pipe.close()

    def stop(self):
        self._stop.set()
        if self._pipe:
            self._pipe.close(discard=True)
        if self._thread:
            self._thread.join(3)
        self._pipe = self._thread = None

    def _run(self, pipe, stop, video_pid):
        while not stop.is_set():
            try:
                container = av.open(pipe, format="mpegts", options=_OPTIONS)
            except (av.FFmpegError, OSError, StopIteration):
                if pipe._closed:
                    return
                time.sleep(0.2)
                continue
            try:
                streams = list(container.streams.video)
                stream = next((s for s in streams if s.id == video_pid), streams[0] if streams else None)
                if stream is None:
                    continue
                if self._decode(container, stream, stop):
                    return
            finally:
                container.close()

    def _decode(self, container, stream, stop):
        """Returns True at end of data."""
        frames = errors = shown = 0
        total_errors = 0
        started = False
        window = last_show = time.monotonic()
        width = height = 0
        for packet in container.demux(stream):
            if stop.is_set():
                return True
            if packet.dts is None and packet.size == 0:
                return True
            try:
                decoded = packet.decode()
            except av.FFmpegError:
                errors += started
                decoded = []
            for frame in decoded:
                # Joining mid-GOP always yields broken frames until the first keyframe.
                # Those say nothing about the link, so neither show nor count them.
                started = started or frame.key_frame
                if not started:
                    continue
                frames += 1
                width, height = frame.width, frame.height
                if frame.is_corrupt:
                    errors += 1
                now = time.monotonic()
                if now - last_show >= 1 / 30:
                    last_show = now
                    rgb = frame.to_ndarray(format="rgb24")
                    h, w = rgb.shape[:2]
                    self.frame.emit(QImage(rgb.data, w, h, rgb.strides[0], QImage.Format.Format_RGB888).copy())
                    shown += 1
            now = time.monotonic()
            if now - window >= 1.0:
                total_errors += errors
                self.info.emit({"fps": frames / (now - window), "width": width, "height": height,
                                "codec": stream.codec_context.name, "errors": errors,
                                "total_errors": total_errors})
                frames = errors = 0
                window = now
        return True
