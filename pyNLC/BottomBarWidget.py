from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QPushButton, QWidget


class BottomBarWidget(QWidget):
    """Bottom command-bar shim."""

    leftClicked = Signal()
    centerClicked = Signal()
    rightClicked = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)

        self._left = QPushButton("Back", self)
        self._center = QPushButton("Action", self)
        self._right = QPushButton("Next", self)

        self._left.clicked.connect(self.leftClicked)
        self._center.clicked.connect(self.centerClicked)
        self._right.clicked.connect(self.rightClicked)

        layout.addWidget(self._left)
        layout.addWidget(self._center)
        layout.addWidget(self._right)
