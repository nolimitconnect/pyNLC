from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

from PySide6.QtWidgets import QListWidgetItem, QMessageBox, QWidget

from resources.Forms.AppletPermissionList_ui import Ui_AppletPermissionListtUi
from resources.Forms.PermissionListItemWidget_ui import Ui_PermissionListItemUi

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


@dataclass(frozen=True)
class ServicePermissionDef:
    key: str
    name: str
    description: str


class PermissionListItemWidget(QWidget):
    PERMISSIONS = [
        "Ignore",
        "Anonymous",
        "Guest",
        "Friend",
        "Admin",
    ]

    def __init__(self, service_def: ServicePermissionDef, settings: AppSettingsStub | None = None, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_PermissionListItemUi()
        self.ui.setupUi(self)

        self.service_def = service_def
        self.settings = settings

        self.ui.m_ServiceNameLabel.setText(service_def.name)
        self.ui.m_ServiceDescLabel.setText(service_def.description)

        self.ui.m_PermissionComboBox.clear()
        self.ui.m_PermissionComboBox.addItems(self.PERMISSIONS)

        saved_idx = int(self._settings_get(self._permission_key(), 3))
        if saved_idx < 0 or saved_idx >= len(self.PERMISSIONS):
            saved_idx = 3
        self.ui.m_PermissionComboBox.setCurrentIndex(saved_idx)

        self.ui.m_PermissionComboBox.currentIndexChanged.connect(self._on_permission_changed)
        self.ui.m_PermissionButton.clicked.connect(self._show_permission_information)
        self.ui.m_PluginInfoButton.clicked.connect(self._show_plugin_information)
        self.ui.m_PluginRunButton.clicked.connect(self._run_plugin)
        self.ui.m_PluginSettingsButton.clicked.connect(self._setup_plugin)

    def _permission_key(self) -> str:
        return f"permission.{self.service_def.key}"

    def _settings_get(self, key: str, default):
        if self.settings is None:
            return default
        getter = getattr(self.settings, "_settings_store", None)
        if isinstance(getter, dict):
            return getter.get(key, default)
        return default

    def _settings_set(self, key: str, value) -> None:
        if self.settings is None:
            return
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value

    def _on_permission_changed(self, index: int) -> None:
        self._settings_set(self._permission_key(), int(index))

    def _show_permission_information(self) -> None:
        QMessageBox.information(self, "Permission", "Set who can access this service.")

    def _show_plugin_information(self) -> None:
        QMessageBox.information(self, self.service_def.name, self.service_def.description)

    def _run_plugin(self) -> None:
        QMessageBox.information(self, "Run Service", f"Launch for {self.service_def.name} is pending migration.")

    def _setup_plugin(self) -> None:
        QMessageBox.information(self, "Service Settings", f"Settings for {self.service_def.name} are pending migration.")


class PermissionListApplet(QWidget):
    """Python translation of nolimitgui AppletPermissionList."""

    SERVICE_DEFS = [
        ServicePermissionDef("about", "About Me Page", "Profile and about page service"),
        ServicePermissionDef("fileshare", "File Share", "File sharing and transfer service"),
        ServicePermissionDef("messenger", "Messenger", "Direct messaging service"),
        ServicePermissionDef("group", "Group Host", "Group hosting service"),
        ServicePermissionDef("chatroom", "Chat Room", "Chat room hosting service"),
        ServicePermissionDef("storyboard", "Storyboard", "Storyboard publishing service"),
    ]

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPermissionListtUi()
        self.ui.setupUi(self)

        self.settings = settings
        self._initialize_permission_list()

    def _initialize_permission_list(self) -> None:
        self.ui.m_PermissionItemList.clear()
        for service_def in self.SERVICE_DEFS:
            self._create_permission_item(service_def)

    def _create_permission_item(self, service_def: ServicePermissionDef) -> None:
        widget = PermissionListItemWidget(service_def, self.settings, self)
        item = QListWidgetItem(self.ui.m_PermissionItemList)
        item.setSizeHint(widget.sizeHint())
        self.ui.m_PermissionItemList.addItem(item)
        self.ui.m_PermissionItemList.setItemWidget(item, widget)
