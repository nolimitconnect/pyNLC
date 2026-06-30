from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QLabel


class VxLabel(QLabel):
    """Compatibility shim for legacy VxLabel."""

    clicked = Signal()

    def mousePressEvent(self, event) -> None:  # type: ignore[override]
        self.clicked.emit()
        super().mousePressEvent(event)
