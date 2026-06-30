from __future__ import annotations

from PySide6.QtWidgets import QProgressBar


class VxProgressBar(QProgressBar):
    """Compatibility progress bar with sensible defaults."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setRange(0, 100)
        self.setValue(0)
