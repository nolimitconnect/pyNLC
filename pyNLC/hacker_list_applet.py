from __future__ import annotations

from datetime import datetime

from PySide6.QtGui import QGuiApplication, QStandardItem, QStandardItemModel
from PySide6.QtWidgets import QWidget

from resources.Forms.AppletHackerList_ui import Ui_AppletHackerListUi


class HackerListApplet(QWidget):
    """Python translation of nolimitgui AppletHackerList (stub data source)."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletHackerListUi()
        self.ui.setupUi(self)

        self.settings = settings
        self.ui.m_CopyToClipboardButton.setVisible(False)
        self.ui.m_CopyToClipboardButton.clicked.connect(self.slot_copy_to_clipboard_clicked)
        self._offense_rows: dict[tuple[str, str, str], int] = {}

        self.model = QStandardItemModel(0, 5, self)
        self.ui.m_TreeView.setRootIsDecorated(False)
        self.ui.m_TreeView.setAlternatingRowColors(True)
        self.ui.m_TreeView.setModel(self.model)
        self.ui.m_TreeView.setSortingEnabled(True)

        headers = ["Count", "IP Address", "Level", "Offense", "Signature"]
        for idx, header in enumerate(headers):
            self.model.setHeaderData(idx, 1, header)

        self.refresh_hacker_list()

    def slot_copy_to_clipboard_clicked(self) -> None:
        clipboard = QGuiApplication.clipboard()
        if clipboard is None:
            return

        rows = []
        for row in range(self.model.rowCount()):
            rows.append("\t".join(self.model.data(self.model.index(row, col)) or "" for col in range(5)))
        clipboard.setText("\n".join(rows))

    def refresh_hacker_list(self) -> None:
        self._offense_rows.clear()
        sample_rows = [
            ("3", "203.0.113.18", "2", "Bad handshake", "8A17FF10"),
            ("1", "198.51.100.27", "1", "Rate limit", "09BC7D21"),
        ]
        for row in sample_rows:
            self.add_hacker(*row)

    def add_hack_report(
        self,
        hacker_level: int,
        hacker_reason: int,
        ip_addr: str,
        description: str,
        timestamp_ms: int | None = None,
    ) -> None:
        level_text = str(int(hacker_level))
        reason_text = str(int(hacker_reason))
        if timestamp_ms is not None:
            ts = datetime.fromtimestamp(timestamp_ms / 1000).strftime("%H:%M:%S")
            signature = f"{reason_text}:{description[:18]}@{ts}"
        else:
            signature = f"{reason_text}:{description[:24]}"
        self.add_hacker("1", ip_addr, level_text, description, signature)

    def add_hacker(self, count: str, ip_addr: str, level: str, offense: str, signature: str) -> None:
        key = (ip_addr, level, offense)
        prior_count = self._offense_rows.get(key)
        if prior_count is not None:
            row = prior_count
            existing = int(self.model.item(row, 0).text() or "0")
            self.model.item(row, 0).setText(str(existing + int(count)))
            return

        row_items = [
            QStandardItem(count),
            QStandardItem(ip_addr),
            QStandardItem(level),
            QStandardItem(offense),
            QStandardItem(signature),
        ]
        self._offense_rows[key] = self.model.rowCount()
        self.model.appendRow(row_items)
