from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletHostJoinRequestList_ui import Ui_AppletHostJoinRequestListUi
from resources.Forms.AppletHostNetworkStatus_ui import Ui_AppletHostNetworkStatusUi


class HostJoinRequestListApplet(QWidget):
    """Concrete local host join-request applet."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostJoinRequestListUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_RequestStateComboBox.addItems(["Pending", "Invited", "Accepted", "Blocked"])
        self._seed_requests("Pending")

        self.ui.m_RequestStateComboBox.currentTextChanged.connect(self._seed_requests)
        self.ui.m_InviteCreateButton.clicked.connect(lambda: self._append_request("invite-created"))
        self.ui.m_InviteAcceptButton.clicked.connect(lambda: self._append_request("invite-accepted"))
        self.ui.m_AcceptAllButton.clicked.connect(self._accept_all)

    def _seed_requests(self, state: str) -> None:
        self.ui.m_HostJoinRequestList.clear()
        for idx in range(1, 4):
            self.ui.m_HostJoinRequestList.addItem(f"user-{idx:02d} ({state.lower()})")

    def _append_request(self, suffix: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_HostJoinRequestList.addItem(f"local-{suffix} @ {stamp}")

    def _accept_all(self) -> None:
        for i in range(self.ui.m_HostJoinRequestList.count()):
            item = self.ui.m_HostJoinRequestList.item(i)
            item.setText(f"{item.text()} [accepted]")


class HostNetworkStatusApplet(QWidget):
    """Concrete local host network status applet."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostNetworkStatusUi()
        self.ui.setupUi(self)

        self.ui.m_HostPermissionWidget.set_label("Host Permission")
        self.ui.m_ConnectTestPermissionWidget.set_label("Connection Test Permission")

        self.ui.m_UrlText.setText("https://localhost:8443")
        self.ui.m_HostingStatusText.setText("Ready (local)")
        self.ui.m_GroupListCountLabel.setText("3")

        for name in ["alice", "bob", "charlie"]:
            self.ui.m_FriendListWidget.addItem(name)

        self.ui.m_OpenPortCheckBox.setChecked(True)
        self.ui.m_HostPermissionCheckBox.setChecked(True)
        self.ui.m_ConnectionTestPermissionCheckBox.setChecked(True)

        self.ui.m_HostingRequirementsButton.clicked.connect(self._refresh_status)

    def _refresh_status(self) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_HostingStatusText.setText(f"Checked @ {stamp}")
