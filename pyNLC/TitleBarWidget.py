from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QWidget


class TitleBarWidget(QWidget):
    """Simple title bar shim with back/menu actions."""

    backClicked = Signal()
    menuClicked = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)

        self._back = QPushButton("<", self)
        self._title = QLabel("Title", self)
        self._menu = QPushButton("...", self)

        self._back.clicked.connect(self.backClicked)
        self._menu.clicked.connect(self.menuClicked)

        layout.addWidget(self._back)
        layout.addWidget(self._title, 1)
        layout.addWidget(self._menu)

    def setTitle(self, text: str) -> None:
        self._title.setText(text)
