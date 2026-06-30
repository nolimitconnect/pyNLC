from __future__ import annotations

from PySide6.QtWidgets import QLabel, QLineEdit, QVBoxLayout, QWidget


class NetworkKeyWidget(QWidget):
    """Compatibility network-key editor shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(QLabel("Network Key", self))
        self._key = QLineEdit(self)
        layout.addWidget(self._key)
