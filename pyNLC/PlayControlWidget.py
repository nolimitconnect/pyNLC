from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QSlider, QWidget


class PlayControlWidget(QWidget):
    """Compatibility playback control strip."""

    playRequested = Signal()
    pauseRequested = Signal()
    stopRequested = Signal()
    positionChanged = Signal(int)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.setSpacing(4)

        self._play = QPushButton("Play", self)
        self._pause = QPushButton("Pause", self)
        self._stop = QPushButton("Stop", self)
        self._position = QSlider(self)
        self._position.setOrientation(1)  # Horizontal
        self._position.setRange(0, 100)
        self._label = QLabel("00:00", self)

        self._play.clicked.connect(self.playRequested.emit)
        self._pause.clicked.connect(self.pauseRequested.emit)
        self._stop.clicked.connect(self.stopRequested.emit)
        self._position.valueChanged.connect(self.positionChanged.emit)

        layout.addWidget(self._play)
        layout.addWidget(self._pause)
        layout.addWidget(self._stop)
        layout.addWidget(self._position, 1)
        layout.addWidget(self._label)

    def setPosition(self, value: int) -> None:
        self._position.setValue(int(value))

    def setTimeLabel(self, value: str) -> None:
        self._label.setText(value)
