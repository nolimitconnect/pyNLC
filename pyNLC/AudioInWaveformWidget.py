from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import QFrame


class AudioInWaveformWidget(QFrame):
    """Compatibility placeholder for input waveform rendering."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._level = 0.0

    def setLevel(self, value: float) -> None:
        self._level = max(0.0, min(1.0, float(value)))
        self.update()

    def paintEvent(self, event) -> None:  # type: ignore[override]
        super().paintEvent(event)
        painter = QPainter(self)
        painter.fillRect(self.rect(), QColor(18, 18, 18))

        bars = 32
        width = max(1, self.width() // bars)
        active = int(self._level * bars)
        for idx in range(bars):
            h = int((idx + 1) / bars * self.height())
            x = idx * width
            y = self.height() - h
            color = QColor(80, 180, 120) if idx < active else QColor(45, 70, 50)
            painter.fillRect(x, y, max(1, width - 1), h, color)

        painter.setPen(QColor(180, 180, 180))
        painter.drawText(self.rect(), Qt.AlignCenter, "Audio In")
