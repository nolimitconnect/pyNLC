from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import QPushButton


class ClipboardPasteWidget(QPushButton):
    """Compatibility widget that emits clipboard text when clicked."""

    textPasted = Signal(str)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setText("Paste Invite")
        self.clicked.connect(self._on_clicked)

    def _on_clicked(self) -> None:
        clipboard = QGuiApplication.clipboard()
        text = ""
        if clipboard is not None:
            text = clipboard.text() or ""
        self.textPasted.emit(text)
