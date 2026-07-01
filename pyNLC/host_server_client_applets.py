from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


def _stamp() -> str:
    return datetime.now().strftime("%H:%M:%S")


class _BaseConcreteApplet(QWidget):
    def __init__(self, title: str, key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._title = title
        self._key = key
        self._settings = settings

        self.setWindowTitle(title)
        layout = QVBoxLayout(self)
        self._header = QLabel(title, self)
        self._status = QLabel("Ready", self)
        self._list = QListWidget(self)
        self._refresh = QPushButton("Refresh", self)
        self._refresh.clicked.connect(self._refresh_data)

        layout.addWidget(self._header)
        layout.addWidget(self._status)
        layout.addWidget(self._list)
        layout.addWidget(self._refresh)

        self._refresh_data()

    def _remember(self, key: str, value) -> None:
        if self._settings is None:
            return
        store = getattr(self._settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value
            save_fn = getattr(self._settings, "_save_settings", None)
            if callable(save_fn):
                try:
                    save_fn()
                except Exception:
                    pass

    def _refresh_data(self) -> None:
        ts = _stamp()
        self._status.setText(f"Updated @ {ts}")
        self._list.clear()
        for i in range(1, 5):
            self._list.addItem(f"{self._title} item {i}")
        self._remember(self._key, ts)


class HostGroupStatusApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Host Group Status", "ui.host.group_status", settings, parent)


class HostChatRoomStatusApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Host Chat Room Status", "ui.host.chatroom_status", settings, parent)


class HostRandomConnectStatusApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Host Random Connect Status", "ui.host.random_status", settings, parent)


class EditAboutMeApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Edit About Me", "ui.service.edit_about_me", settings, parent)


class AboutMeServerViewMineApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("About Me Server View", "ui.service.about_me_server_view", settings, parent)


class EditStoryboardApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Edit Storyboard", "ui.service.edit_storyboard", settings, parent)


class StoryboardServerViewMineApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Storyboard Server View", "ui.service.storyboard_server_view", settings, parent)


class CamServerViewMineApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Cam Server View", "ui.service.cam_server_view", settings, parent)


class FileShareServerViewMineApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("File Share Server View", "ui.service.file_share_server_view", settings, parent)


class AvatarImageClientApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Avatar Image Client", "ui.client.avatar_image", settings, parent)


class ConnectionTestClientApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Connection Test Client", "ui.client.connection_test", settings, parent)


class ClientRelayApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Client Relay", "ui.client.relay", settings, parent)


class ClientShareFilesApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Client Share Files", "ui.client.share_files", settings, parent)


class TestHostServiceApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Test Host Service", "ui.test.host_service", settings, parent)


class CreateAccountApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Create Account", "ui.account.create", settings, parent)

        row = QWidget(self)
        row_layout = QHBoxLayout(row)
        row_layout.setContentsMargins(0, 0, 0, 0)
        self._name_input = QLineEdit(row)
        self._name_input.setPlaceholderText("Account name")
        self._create_btn = QPushButton("Create", row)
        self._create_btn.clicked.connect(self._create_account)
        row_layout.addWidget(self._name_input)
        row_layout.addWidget(self._create_btn)
        self.layout().insertWidget(2, row)

        self._refresh_data()

    def _refresh_data(self) -> None:
        self._list.clear()
        accounts = []
        if self._settings is not None and hasattr(self._settings, "getAllAccounts"):
            try:
                accounts = self._settings.getAllAccounts()
            except Exception:
                accounts = []

        if not accounts:
            self._list.addItem("No accounts yet")
            self._status.setText("No accounts in local sqlite DB")
            return

        for item in accounts:
            name = str(item.get("name", "")).strip()
            if not name:
                continue
            online_id = str(item.get("online_id", ""))
            favorite = False
            if self._settings is not None and hasattr(self._settings, "getIsFavorite"):
                try:
                    favorite = bool(self._settings.getIsFavorite(online_id or name))
                except Exception:
                    favorite = False
            star = "*" if favorite else "-"
            self._list.addItem(f"{star} {name} ({online_id or 'no-online-id'})")

        self._status.setText(f"Loaded {len(accounts)} account(s)")
        self._remember(self._key, _stamp())

    def _create_account(self) -> None:
        account_name = self._name_input.text().strip()
        if not account_name:
            QMessageBox.warning(self, "Create Account", "Please enter an account name.")
            return

        if self._settings is None or not hasattr(self._settings, "insertAccount"):
            QMessageBox.warning(self, "Create Account", "Account database is not available.")
            return

        existing = []
        if hasattr(self._settings, "getAllAccounts"):
            try:
                existing = self._settings.getAllAccounts()
            except Exception:
                existing = []
        if any(str(item.get("name", "")).lower() == account_name.lower() for item in existing):
            QMessageBox.warning(self, "Create Account", f"Account '{account_name}' already exists.")
            return

        online_id = account_name.lower().replace(" ", "_")
        created = bool(self._settings.insertAccount(account_name, online_id))
        if created and hasattr(self._settings, "updateLastLogin"):
            self._settings.updateLastLogin(account_name)

        if not created:
            QMessageBox.warning(self, "Create Account", "Failed to create account.")
            return

        self._name_input.clear()
        self._refresh_data()
        QMessageBox.information(self, "Create Account", f"Account '{account_name}' created.")


class InformationApplet(_BaseConcreteApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Information", "ui.util.information", settings, parent)

    def _refresh_data(self) -> None:
        self._list.clear()

        last_login = ""
        last_applet = 0
        recent_ids = []
        account_count = 0

        if self._settings is not None and hasattr(self._settings, "getLastLogin"):
            try:
                last_login = str(self._settings.getLastLogin())
            except Exception:
                last_login = ""

        store = getattr(self._settings, "_settings_store", None)
        if isinstance(store, dict):
            try:
                last_applet = int(store.get("lastAppletLaunched", 0) or 0)
            except Exception:
                last_applet = 0
            raw_recent = store.get("ui.recentAppletIds", [])
            recent_ids = [int(v) for v in raw_recent if isinstance(v, (int, str)) and str(v).isdigit()][:5]

        if self._settings is not None and hasattr(self._settings, "getAllAccounts"):
            try:
                account_count = len(self._settings.getAllAccounts())
            except Exception:
                account_count = 0

        self._list.addItem(f"Current account: {last_login or 'none'}")
        self._list.addItem(f"Accounts in DB: {account_count}")
        self._list.addItem(f"Last applet launched: {last_applet}")
        self._list.addItem(
            "Recent applets: " + (", ".join(str(v) for v in recent_ids) if recent_ids else "none")
        )
        self._status.setText(f"Info refreshed @ {_stamp()}")
        self._remember(self._key, _stamp())
