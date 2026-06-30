from __future__ import annotations

from PySide6.QtWidgets import QLabel, QTextEdit, QVBoxLayout, QWidget


class AssetTextWidget(QWidget):
    """Text asset preview shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._title = QLabel("Text", self)
        self._text = QTextEdit(self)
        self._text.setReadOnly(True)
        self._text.setPlainText("Local text asset preview")
        layout.addWidget(self._title)
        layout.addWidget(self._text)
