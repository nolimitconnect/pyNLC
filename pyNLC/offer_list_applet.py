from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletOfferList_ui import Ui_AppletOfferListUi


class OfferListApplet(QWidget):
    """Concrete local offer list applet with active/history toggle."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletOfferListUi()
        self.ui.setupUi(self)
        self.settings = settings

        self._mode = "active"
        self.ui.m_StatusLabel.setText("Active offers")

        self.ui.m_ActiveOffersButton.clicked.connect(lambda: self._switch_mode("active"))
        self.ui.m_OfferHistoryButton.clicked.connect(lambda: self._switch_mode("history"))

        self._populate_rows()

    def _switch_mode(self, mode: str) -> None:
        self._mode = mode
        title = "Active offers" if mode == "active" else "Offer history"
        self.ui.m_StatusLabel.setText(title)
        self._populate_rows()

    def _populate_rows(self) -> None:
        self.ui.m_OfferListWidget.clear_offers()
        now = datetime.now().strftime("%H:%M")
        if self._mode == "active":
            rows = [
                f"[{now}] Incoming file offer from demo-user",
                f"[{now}] Voice session offer from local-peer",
            ]
        else:
            rows = [
                f"[{now}] Completed offer: sample_video.mp4",
                f"[{now}] Rejected offer: random_connect",
            ]

        for row in rows:
            self.ui.m_OfferListWidget.add_offer_row(row)


class PersonOfferListApplet(OfferListApplet):
    """Person-offer variant that reuses OfferList behavior."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(settings=settings, parent=parent)
        self.ui.m_StatusLabel.setText("Person offers")
