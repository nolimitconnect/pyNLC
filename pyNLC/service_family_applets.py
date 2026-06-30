from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletServiceBaseSettings_ui import Ui_AppletServiceBaseSettingsUi
from resources.Forms.AppletServiceBase_ui import Ui_AppletServiceBaseUi
from resources.Forms.AppletSettingsHostBase_ui import Ui_AppletSettingsHostBaseUi


def _write_setting(settings, key: str, value: str) -> None:
    if settings is None:
        return
    store = getattr(settings, "_settings_store", None)
    if isinstance(store, dict):
        store[key] = value


class ServiceBaseSettingsApplet(QWidget):
    """Reusable local service-settings applet."""

    def __init__(self, title: str, settings_key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._settings_key = settings_key
        self.ui = Ui_AppletServiceBaseSettingsUi()
        self.ui.setupUi(self)

        self.ui.m_ServiceTitleLabel.setText(title)
        self.ui.m_UrlEdit.setText("localhost")
        self.ui.m_NameEdit.setText(title)
        self.ui.m_DescriptionEdit.setPlainText("Local migration stub configuration")
        self.ui.m_ApplyButton.clicked.connect(self._apply)

    def _apply(self) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        summary = f"{self.ui.m_NameEdit.text().strip()} @ {stamp}"
        _write_setting(self._settings, self._settings_key, summary)
        self.ui.m_LogWidget.addLogLine(f"Applied {summary}")


class ServiceBaseApplet(QWidget):
    """Reusable local service-control applet."""

    def __init__(self, title: str, settings_key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._settings_key = settings_key
        self.ui = Ui_AppletServiceBaseUi()
        self.ui.setupUi(self)

        self.ui.m_ServiceTitleLabel.setText(title)
        self.ui.m_ContentRatingComboBox.addItems(["All", "Teen", "Mature"])
        self.ui.m_LanguageComboBox.addItems(["en", "es", "fr"])
        self.ui.m_UrlEdit.setText("localhost")
        self.ui.m_NameEdit.setText(title)
        self.ui.m_KeyWordsEdit.setText("migration,stub")

        self.ui.m_StartButton.clicked.connect(lambda: self._set_status("running"))
        self.ui.m_StopButton.clicked.connect(lambda: self._set_status("stopped"))
        self.ui.m_ApplyButton.clicked.connect(lambda: self._set_status("applied"))

    def _set_status(self, status: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        _write_setting(self._settings, self._settings_key, f"{status}:{stamp}")
        self.ui.m_DescriptionEdit.setPlainText(f"Service {status} at {stamp}")


class HostSettingsBaseApplet(QWidget):
    """Reusable local host-settings applet."""

    def __init__(self, title: str, settings_key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._settings_key = settings_key
        self.ui = Ui_AppletSettingsHostBaseUi()
        self.ui.setupUi(self)
        self.setWindowTitle(title)

        self.ui.m_FriendListWidget.clear()
        self.ui.m_FriendListWidget.addItem("alice")
        self.ui.m_FriendListWidget.addItem("bob")
        self.ui.m_FriendListWidget.addItem("charlie")

        self.ui.m_HostingRequirementsButton.clicked.connect(lambda: self._touch("requirements"))
        self.ui.m_ViewMyHostButton.clicked.connect(lambda: self._touch("view_my_host"))

    def _touch(self, action: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        _write_setting(self._settings, self._settings_key, f"{action}:{stamp}")
