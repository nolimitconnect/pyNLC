from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class AssetVideoWidget(QWidget):
    """Video asset preview shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._preview = QLabel("Video", self)
        self._preview.setMinimumHeight(80)
        self._preview.setStyleSheet("background: #dcfce7; border: 1px solid #86efac;")
        layout.addWidget(self._preview)
