from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class IdentOfferWidget(QWidget):
    """Local identity/offer header widget shim."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        self._title = QLabel("Offer Peer", self)
        self._subtitle = QLabel("local-user", self)
        self._subtitle.setStyleSheet("color: #888888;")
        layout.addWidget(self._title)
        layout.addWidget(self._subtitle)

    def set_offer_peer(self, title: str, subtitle: str = "") -> None:
        self._title.setText(title or "Offer Peer")
        self._subtitle.setText(subtitle or "")
