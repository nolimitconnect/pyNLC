from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QPushButton, QWidget


class FileActionBarWidget(QWidget):
    """Compatibility file action bar shim."""

    openClicked = Signal()
    saveClicked = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        open_btn = QPushButton("Open", self)
        save_btn = QPushButton("Save", self)
        open_btn.clicked.connect(self.openClicked)
        save_btn.clicked.connect(self.saveClicked)
        layout.addWidget(open_btn)
        layout.addWidget(save_btn)
