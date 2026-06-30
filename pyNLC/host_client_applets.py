from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletHostClient_ui import Ui_AppletHostClientUi
from resources.Forms.AppletHostJoinChoose_ui import Ui_AppletHostJoinChooseUi


class HostNetworkClientApplet(QWidget):
    """Concrete local host-client applet."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostClientUi()
        self.ui.setupUi(self)
        self.settings = settings
        self.ui.m_UserListWidget.setUsers(["You", "Alpha", "Beta", "Gamma"])


class HostJoinSearchApplet(QWidget):
    """Concrete local host search applet using join chooser layout."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self.ui = Ui_AppletHostJoinChooseUi()
        self.ui.setupUi(self)

        self.ui.m_ViewCurrentFrame.hide()
        self.ui.m_LeaveFrame.hide()
        self.ui.m_RejoinFrame.hide()
        self.ui.m_SearchLabel.setText("Search for hosts (local)")

        self.ui.m_SearchButton.clicked.connect(self._run_search)

    def _run_search(self) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_SearchLabel.setText(f"Search completed @ {stamp}")
        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.host_join_search.last_run"] = stamp
