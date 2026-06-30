from __future__ import annotations

from PySide6.QtWidgets import QListWidget


class HostJoinRequestListWidget(QListWidget):
    """Local join-request list shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setAlternatingRowColors(True)
