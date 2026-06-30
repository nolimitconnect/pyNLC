from __future__ import annotations

from PySide6.QtWidgets import QListWidget, QListWidgetItem


class GuiOfferListWidget(QListWidget):
    """Local offer list widget shim for generated AppletOfferList form."""

    def add_offer_row(self, text: str) -> None:
        self.addItem(QListWidgetItem(text))

    def clear_offers(self) -> None:
        self.clear()
