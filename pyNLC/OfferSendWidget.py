from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QLabel, QLineEdit, QPushButton, QWidget


class OfferSendWidget(QWidget):
    """Local send-controls shim used by converted offer forms."""

    sendClicked = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)

        self._label = QLabel("Message:", self)
        self._message = QLineEdit(self)
        self._message.setPlaceholderText("Add an offer note")
        self._send = QPushButton("Send", self)
        self._send.clicked.connect(self._emit_send)

        layout.addWidget(self._label)
        layout.addWidget(self._message)
        layout.addWidget(self._send)

    def _emit_send(self) -> None:
        self.sendClicked.emit(self._message.text().strip())

    def setMessage(self, message: str) -> None:
        self._message.setText(message)

    def message(self) -> str:
        return self._message.text()
