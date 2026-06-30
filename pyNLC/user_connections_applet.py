from __future__ import annotations

from typing import TYPE_CHECKING

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletUserConnections_ui import Ui_AppletUserConnectionsUi

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


class UserConnectionsApplet(QWidget):
    """Python translation of nolimitgui AppletUserConnections."""

    VIEW_TYPES = [
        "All",
        "Online",
        "Friends",
        "Joined",
        "Pending",
    ]

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletUserConnectionsUi()
        self.ui.setupUi(self)

        self.settings = settings

        self.ui.m_UserListWidget.disconnectUserUpdates()
        self.ui.m_UserViewTypeComboBox.clear()
        self.ui.m_UserViewTypeComboBox.addItems(self.VIEW_TYPES)

        start_idx = int(self._settings_get("getLastUserConnectionsUserViewType", 0))
        if 0 <= start_idx < self.ui.m_UserViewTypeComboBox.count():
            self.ui.m_UserViewTypeComboBox.setCurrentIndex(start_idx)

        self.ui.m_UserViewTypeComboBox.currentIndexChanged.connect(self.slot_user_view_type_selection_change)
        self.refresh_list()

    def _settings_get(self, getter_name: str, default):
        if self.settings is None:
            return default
        getter = getattr(self.settings, getter_name)
        return getter()

    def _settings_set(self, setter_name: str, value) -> None:
        if self.settings is None:
            return
        setter = getattr(self.settings, setter_name)
        setter(value)

    def get_selected_user_view_type(self) -> int:
        return self.ui.m_UserViewTypeComboBox.currentIndex()

    def slot_user_view_type_selection_change(self, combo_idx: int) -> None:
        self._settings_set("setLastUserConnectionsUserViewType", int(combo_idx))
        self.ui.m_UserListWidget.setUserViewType(self.get_selected_user_view_type())

    def refresh_list(self) -> None:
        self.ui.m_UserListWidget.setUserViewType(self.get_selected_user_view_type())
