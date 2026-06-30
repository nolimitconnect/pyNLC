from __future__ import annotations

from PySide6.QtWidgets import QListWidget, QListWidgetItem


class GuiUserListWidget(QListWidget):
    """Compatibility list widget for user-connection views."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._view_type = 0
        self._populate_sample_items()

    def disconnectUserUpdates(self) -> None:
        return None

    def setUserViewType(self, view_type: int) -> None:
        self._view_type = int(view_type)
        self._populate_sample_items()

    def _populate_sample_items(self) -> None:
        self.clear()
        labels = {
            0: ["All Users", "Alice", "Bob", "Charlie"],
            1: ["Online Users", "Alice", "Charlie"],
            2: ["Friends", "Alice", "Bob"],
            3: ["Joined Hosts", "Host: Group Alpha", "Host: Chat Room"],
            4: ["Pending", "Invite from Dana"],
        }

        for name in labels.get(self._view_type, labels[0]):
            self.addItem(QListWidgetItem(name))
