from __future__ import annotations

from PySide6.QtWidgets import QPushButton


class VxPushButton(QPushButton):
    """Compatibility shim for legacy VxPushButton."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._notify_type = 0

    def setNotifyType(self, notify_type: int) -> None:
        self._notify_type = int(notify_type)

    def notifyType(self) -> int:
        return self._notify_type
