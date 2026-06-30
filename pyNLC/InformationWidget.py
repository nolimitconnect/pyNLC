from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class InformationWidget(QWidget):
    """Compatibility information panel shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(QLabel("Information", self))
