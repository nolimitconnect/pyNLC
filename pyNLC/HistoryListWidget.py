from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QListWidget, QListWidgetItem


class HistoryListWidget(QListWidget):
    """Compatibility history widget for session/chat timelines."""

    def addHistoryLine(self, text: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.addItem(QListWidgetItem(f"[{stamp}] {text}"))
