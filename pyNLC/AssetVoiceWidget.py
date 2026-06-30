from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class AssetVoiceWidget(QWidget):
    """Voice asset preview shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._preview = QLabel("Voice", self)
        self._preview.setMinimumHeight(60)
        self._preview.setStyleSheet("background: #fef3c7; border: 1px solid #fcd34d;")
        layout.addWidget(self._preview)
