from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletChatRoomJoinSearch_ui import Ui_AppletChatRoomJoinSearchUi
from resources.Forms.AppletChatRoomListLocalView_ui import Ui_AppletChatRoomListLocalViewUi
from resources.Forms.AppletGroupJoinSearch_ui import Ui_AppletGroupJoinSearchUi
from resources.Forms.AppletGroupListLocalView_ui import Ui_AppletGroupListLocalViewUi
from resources.Forms.AppletRandomConnectJoinSearch_ui import Ui_AppletRandomConnectJoinSearchUi
from resources.Forms.AppletRandomConnectListLocalView_ui import Ui_AppletRandomConnectListLocalViewUi


class _BaseSearchApplet(QWidget):
    _TITLE = "Search"

    def _init_search(self, ui, scope: str) -> None:
        ui.m_StatusLabel.setText(f"{scope} search ready")
        ui.m_InfoLabel.setText(f"Local stub {scope.lower()} search results")
        self._seed(ui, scope)

    def _seed(self, ui, scope: str) -> None:
        ui.m_GuiHostedListWidget.clear()
        for idx in range(1, 4):
            ui.m_GuiHostedListWidget.addItem(f"{scope} #{idx}")


class GroupJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGroupJoinSearchUi()
        self.ui.setupUi(self)
        self._init_search(self.ui, "Group")


class ChatRoomJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletChatRoomJoinSearchUi()
        self.ui.setupUi(self)
        self._init_search(self.ui, "Chat Room")


class RandomConnectJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletRandomConnectJoinSearchUi()
        self.ui.setupUi(self)
        self._init_search(self.ui, "Random Connect")


class _BaseListApplet(QWidget):
    _TITLE = "List"

    def _init_list(self, ui, scope: str) -> None:
        ui.m_ListDescriptionLabel.setText(f"{scope} local list")
        ui.m_StatusLabel.setText(f"{scope} list loaded")
        ui.m_InfoLabel.setText(f"Local stub {scope.lower()} host list")
        ui.m_RefreshButton.clicked.connect(lambda: self._refresh(ui, scope))
        self._refresh(ui, scope)

    def _refresh(self, ui, scope: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        ui.m_StatusLabel.setText(f"{scope} refreshed @ {stamp}")
        ui.m_HostedListWidget.clear()
        for idx in range(1, 5):
            ui.m_HostedListWidget.addItem(f"{scope} Host {idx}")


class GroupListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGroupListLocalViewUi()
        self.ui.setupUi(self)
        self._init_list(self.ui, "Group")


class ChatRoomListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletChatRoomListLocalViewUi()
        self.ui.setupUi(self)
        self._init_list(self.ui, "Chat Room")


class RandomConnectListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletRandomConnectListLocalViewUi()
        self.ui.setupUi(self)
        self._init_list(self.ui, "Random Connect")
