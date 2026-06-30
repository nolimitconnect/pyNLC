from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import QLabel


class VxAvatarImage(QLabel):
    """Avatar placeholder shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setMinimumSize(40, 40)
        self.setAlignment(Qt.AlignCenter)
        self.setText("A")

    def paintEvent(self, event) -> None:  # type: ignore[override]
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setBrush(QColor("#7aa6c2"))
        painter.setPen(Qt.NoPen)
        painter.drawEllipse(self.rect())
        painter.setPen(QColor("white"))
        painter.drawText(self.rect(), Qt.AlignCenter, self.text() or "A")
