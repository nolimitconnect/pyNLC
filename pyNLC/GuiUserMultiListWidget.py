from __future__ import annotations

from PySide6.QtWidgets import QListWidget, QListWidgetItem


class GuiUserMultiListWidget(QListWidget):
    """Compatibility list widget for multi-user session views."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._users: list[str] = []
        self._seed()

    def _seed(self) -> None:
        if not self._users:
            self._users = ["You", "Alice", "Bob"]
            self._refresh()

    def _refresh(self) -> None:
        self.clear()
        for user in self._users:
            self.addItem(QListWidgetItem(user))

    def setUsers(self, users: list[str]) -> None:
        self._users = [str(u) for u in users]
        self._refresh()
