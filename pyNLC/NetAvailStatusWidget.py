from __future__ import annotations

from PySide6.QtWidgets import QLabel


class NetAvailStatusWidget(QLabel):
    """Compatibility network-availability status label shim."""

    def __init__(self, parent=None) -> None:
        super().__init__("Network: Unknown", parent)
