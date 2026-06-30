from __future__ import annotations

from typing import TYPE_CHECKING

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletFriendList_ui import Ui_AppletFriendListUi

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


class FriendListApplet(QWidget):
    """Python translation of nolimitgui AppletFriendList."""

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletFriendListUi()
        self.ui.setupUi(self)

        self.settings = settings
        self._current_view = 2  # Start with Friends view.

        self.ui.m_UserListWidget.disconnectUserUpdates()

        self.ui.m_FriendsButton.setText("Friends")
        self.ui.m_IgnoredButton.setText("Blocked")
        self.ui.m_OfflineButton.setText("Offline")
        self.ui.m_FriendsInfoButton.setText("?")
        self.ui.m_IgnoredInfoButton.setText("?")
        self.ui.m_OfflineInfoButton.setText("?")

        self.ui.m_FriendsButton.clicked.connect(lambda: self._set_view(2, "Showing friends"))
        self.ui.m_IgnoredButton.clicked.connect(lambda: self._set_view(4, "Showing blocked users"))
        self.ui.m_OfflineButton.clicked.connect(lambda: self._set_view(0, "Showing all users including offline"))

        self.ui.m_FriendsInfoButton.clicked.connect(lambda: self._show_info("Friends", "People you have added as friends."))
        self.ui.m_IgnoredInfoButton.clicked.connect(lambda: self._show_info("Blocked", "Users currently blocked or ignored."))
        self.ui.m_OfflineInfoButton.clicked.connect(lambda: self._show_info("Offline", "Users who are not currently online."))

        self._set_view(self._current_view, "Friend list ready")

    def _set_view(self, view_type: int, status_text: str) -> None:
        self._current_view = int(view_type)
        self.ui.m_UserListWidget.setUserViewType(self._current_view)
        self.ui.m_StatusLabel.setText(status_text)

    def _show_info(self, title: str, text: str) -> None:
        QMessageBox.information(self, title, text)
