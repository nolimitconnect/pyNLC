from __future__ import annotations

from PySide6.QtWidgets import QTextEdit


class RichTextWidget(QTextEdit):
    """Compatibility rich text editor/view shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setAcceptRichText(True)
