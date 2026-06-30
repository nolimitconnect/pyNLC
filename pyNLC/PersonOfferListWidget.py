from __future__ import annotations

from PySide6.QtWidgets import QListWidget


class PersonOfferListWidget(QListWidget):
    """Compatibility person-offer list shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setAlternatingRowColors(True)
