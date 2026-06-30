from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletHostJoinConnect_ui import Ui_AppletHostJoinConnectUi


class HostJoinConnectApplet(QWidget):
    """Concrete local host-join chooser with local-only actions."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHostJoinConnectUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_StatusLabel_2.setText("Choose how to connect (local stub mode)")
        self.ui.m_InfoPlainTextEdit.setReadOnly(True)
        self._write_info("Ready")

        self.ui.m_ViewCurrentButton.clicked.connect(lambda: self._do_action("view_current"))
        self.ui.m_RejoinButton.clicked.connect(lambda: self._do_action("rejoin_last"))
        self.ui.m_SearchButton.clicked.connect(lambda: self._do_action("search_new"))

    def _write_info(self, text: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_InfoPlainTextEdit.appendPlainText(f"[{stamp}] {text}")

    def _do_action(self, action: str) -> None:
        labels = {
            "view_current": "Viewing current host (stub)",
            "rejoin_last": "Rejoining last host (stub)",
            "search_new": "Opening host search (stub)",
        }
        message = labels.get(action, action)
        self.ui.m_StatusLabel_2.setText(message)
        self._write_info(message)

        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.host_join_connect.last_action"] = action
