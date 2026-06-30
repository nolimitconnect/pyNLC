from __future__ import annotations

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletGroupListClient_ui import Ui_AppletGroupListClientUi
from resources.Forms.AppletHostClient_ui import Ui_AppletHostClientUi


class _BaseClientApplet(QWidget):
    _SCOPE = "Client"

    def _init_client(self, ui) -> None:
        ui.m_UserListWidget.setUsers(["You", f"{self._SCOPE} A", f"{self._SCOPE} B"])


class GroupClientApplet(_BaseClientApplet):
    _SCOPE = "Group"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostClientUi()
        self.ui.setupUi(self)
        self._init_client(self.ui)


class ChatRoomClientApplet(_BaseClientApplet):
    _SCOPE = "ChatRoom"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostClientUi()
        self.ui.setupUi(self)
        self._init_client(self.ui)


class RandomConnectClientApplet(_BaseClientApplet):
    _SCOPE = "RandomConnect"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostClientUi()
        self.ui.setupUi(self)
        self._init_client(self.ui)


class TestHostClientApplet(_BaseClientApplet):
    _SCOPE = "TestHost"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostClientUi()
        self.ui.setupUi(self)
        self._init_client(self.ui)


class GroupListClientApplet(QWidget):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGroupListClientUi()
        self.ui.setupUi(self)
        self.ui.m_StatusLabel.setText("Group list client ready")
        self.ui.m_InfoLabel.setText("Local group list search")
        self.ui.m_GuiHostedListWidget.clear()
        for idx in range(1, 4):
            self.ui.m_GuiHostedListWidget.addItem(f"Group Client Host {idx}")
