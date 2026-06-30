from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget

from resources.Forms.AppletHostJoinChoose_ui import Ui_AppletHostJoinChooseUi
from resources.Forms.AppletHostLeave_ui import Ui_AppletHostLeaveUi


class HostJoinChooseApplet(QWidget):
    """Concrete local chooser for host join/leave/search actions."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostJoinChooseUi()
        self.ui.setupUi(self)
        self.settings = settings

        self._status = QLabel("Choose host action", self)
        if isinstance(self.layout(), QVBoxLayout):
            self.layout().insertWidget(0, self._status)

        self.ui.m_ViewCurrentButton.clicked.connect(lambda: self._set_action("view_current"))
        self.ui.m_LeaveButton.clicked.connect(lambda: self._set_action("leave_current"))
        self.ui.m_RejoinButton.clicked.connect(lambda: self._set_action("rejoin_last"))
        self.ui.m_SearchButton.clicked.connect(lambda: self._set_action("search_new"))

    def _set_action(self, action: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self._status.setText(f"[{stamp}] Action: {action}")
        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.host_join_choose.last_action"] = action


class HostLeaveApplet(QWidget):
    """Concrete local host leave/boot/block dialog logic."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostLeaveUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_AdminLabel.setText("Admin (local)")

        self.ui.m_LeaveButton.clicked.connect(lambda: self._set_status("Leave requested"))
        self.ui.m_BootButton.clicked.connect(lambda: self._set_status("Boot + leave requested"))
        self.ui.m_BlockButton.clicked.connect(lambda: self._set_status("Block host requested"))
        self.ui.m_CancelButton.clicked.connect(lambda: self._set_status("Cancelled"))

    def _set_status(self, text: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_LeaveLabel.setText(f"{text} @ {stamp}")
        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.host_leave.last_action"] = text
