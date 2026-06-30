from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import QPushButton


class ClipboardCopyWidget(QPushButton):
    """Compatibility widget that copies configured text to clipboard."""

    clicked = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._copy_text = ""
        self.setText("Copy")
        super().clicked.connect(self._on_clicked)

    def setCopyText(self, value: str) -> None:
        self._copy_text = value

    def copyText(self) -> str:
        return self._copy_text

    def _on_clicked(self) -> None:
        clipboard = QGuiApplication.clipboard()
        if clipboard is not None and self._copy_text:
            clipboard.setText(self._copy_text)
        self.clicked.emit()
