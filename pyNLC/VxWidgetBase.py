from __future__ import annotations

from PySide6.QtWidgets import QWidget


class VxWidgetBase(QWidget):
    """Base compatibility widget for converted UI classes."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
