from __future__ import annotations

from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import QWidget


class RenderGlWidget(QWidget):
    """Compatibility rendering surface placeholder for media playback."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._overlay_text = "No media loaded"

    def setOverlayText(self, text: str) -> None:
        self._overlay_text = text
        self.update()

    def paintEvent(self, event) -> None:  # type: ignore[override]
        super().paintEvent(event)
        painter = QPainter(self)
        painter.fillRect(self.rect(), QColor(20, 20, 20))
        painter.setPen(QColor(220, 220, 220))
        painter.drawText(self.rect(), 0x0004 | 0x0080, self._overlay_text)
