from __future__ import annotations

from PySide6.QtWidgets import QLabel, QHBoxLayout, QWidget


class CamTitleBarPreview(QWidget):
    """Compatibility camera title-bar preview shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(QLabel("Camera Preview", self))
