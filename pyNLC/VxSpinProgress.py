from __future__ import annotations

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QProgressBar


class VxSpinProgress(QProgressBar):
    """Simple animated busy indicator shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setRange(0, 0)
        self._timer = QTimer(self)

    def start(self) -> None:
        self.setRange(0, 0)

    def stop(self) -> None:
        self.setRange(0, 100)
        self.setValue(0)
