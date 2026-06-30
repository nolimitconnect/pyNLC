from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class NetworkTestWidget(QWidget):
    """Compatibility network test result panel shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(QLabel("Network test pending", self))
