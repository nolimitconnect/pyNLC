from __future__ import annotations

from PySide6.QtWidgets import QPushButton


class VxMenuButtonTop(QPushButton):
    """Top menu button shim."""

    def __init__(self, parent=None) -> None:
        super().__init__("Menu", parent)
