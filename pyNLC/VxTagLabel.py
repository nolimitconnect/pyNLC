from __future__ import annotations

from PySide6.QtWidgets import QLabel


class VxTagLabel(QLabel):
    """Styled tag label shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setStyleSheet("padding: 2px 6px; border: 1px solid #b0b0b0; border-radius: 8px;")
