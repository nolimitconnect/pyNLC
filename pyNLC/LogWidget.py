from __future__ import annotations

from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import QWidget

from resources.Forms.LogWidget_ui import Ui_LogWidgetUi


class LogWidget(QWidget):
    """Compatibility log widget wrapper."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_LogWidgetUi()
        self.ui.setupUi(self)

        self.ui.m_ClearLogButton.clicked.connect(self.clear)
        self.ui.m_CopyToClipboardButton.clicked.connect(self.copyToClipboard)

    def addLogLine(self, text: str) -> None:
        self.ui.m_LogPlainTextEdit.appendPlainText(str(text))

    def clear(self) -> None:
        self.ui.m_LogPlainTextEdit.clear()

    def copyToClipboard(self) -> None:
        clipboard = QGuiApplication.clipboard()
        if clipboard is not None:
            clipboard.setText(self.ui.m_LogPlainTextEdit.toPlainText())
