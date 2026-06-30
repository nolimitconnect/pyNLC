from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QPushButton, QWidget


class OfferBarWidget(QWidget):
    """Local offer action bar shim."""

    acceptClicked = Signal()
    rejectClicked = Signal()
    cancelClicked = Signal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._accept = QPushButton("Accept", self)
        self._reject = QPushButton("Reject", self)
        self._cancel = QPushButton("Cancel", self)
        self._accept.clicked.connect(self.acceptClicked)
        self._reject.clicked.connect(self.rejectClicked)
        self._cancel.clicked.connect(self.cancelClicked)
        layout.addWidget(self._accept)
        layout.addWidget(self._reject)
        layout.addWidget(self._cancel)
