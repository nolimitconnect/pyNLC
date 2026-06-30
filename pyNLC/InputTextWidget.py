from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputTextWidget_ui import Ui_InputTextWidgetClass


class InputTextWidget(QWidget):
    """Compatibility text input widget."""

    sendTextRequested = Signal(str)
    canceled = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputTextWidgetClass()
        self.ui.setupUi(self)

        self.ui.m_CancelTextButton.setText("X")
        self.ui.m_SendTextButton.setText("Send")

        self.ui.m_CancelTextButton.clicked.connect(self._cancel)
        self.ui.m_SendTextButton.clicked.connect(self._send)

    def text(self) -> str:
        return self.ui.m_ChatTextEdit.toPlainText()

    def setText(self, value: str) -> None:
        self.ui.m_ChatTextEdit.setPlainText(value)

    def clear(self) -> None:
        self.ui.m_ChatTextEdit.clear()

    def _send(self) -> None:
        message = self.text().strip()
        if message:
            self.sendTextRequested.emit(message)
            self.clear()

    def _cancel(self) -> None:
        self.clear()
        self.canceled.emit()
