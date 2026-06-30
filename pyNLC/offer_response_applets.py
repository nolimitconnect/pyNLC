from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletOfferInfo_ui import Ui_AppletOfferInfoUi
from resources.Forms.AppletOfferResponse_ui import Ui_AppletOfferResponseUi
from resources.Forms.AppletOfferResponseAccept_ui import Ui_AppletOfferResponseAcceptUi


class OfferInfoApplet(QWidget):
    """Concrete local offer-info viewer."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletOfferInfoUi()
        self.ui.setupUi(self)
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.ui.m_StatusMsgLabel.setText("Offer pending (local)")
        self.ui.m_OfferTimeLabel.setText(now)
        self.ui.m_MsgTextEdit.setPlainText("Local stub: offer message preview.")
        self.ui.m_FileName.setText("example.bin")
        self.ui.m_FileSize.setText("12.4 MB")


class _BaseOfferResponseApplet(QWidget):
    _STATUS_TEXT = "Offer response"

    def _wire_offer_bar(self) -> None:
        self.ui.m_OfferBarWidget.acceptClicked.connect(lambda: self._set_status("Accepted"))
        self.ui.m_OfferBarWidget.rejectClicked.connect(lambda: self._set_status("Rejected"))
        self.ui.m_OfferBarWidget.cancelClicked.connect(lambda: self._set_status("Cancelled"))

    def _set_status(self, text: str) -> None:
        self.ui.m_StatusMsgLabel.setText(f"{self._STATUS_TEXT}: {text}")


class OfferResponseApplet(_BaseOfferResponseApplet):
    _STATUS_TEXT = "Offer response"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletOfferResponseUi()
        self.ui.setupUi(self)
        self.ui.m_OfferExpireTime.setText("When response received")
        self.ui.m_FileName.setText("example.bin")
        self.ui.m_Path.setText("/local/stub/path/example.bin")
        self._set_status("Pending")
        self._wire_offer_bar()


class OfferResponseAcceptApplet(_BaseOfferResponseApplet):
    _STATUS_TEXT = "Accept response"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletOfferResponseAcceptUi()
        self.ui.setupUi(self)
        self.ui.m_OfferExpireTime.setText("When response received")
        self.ui.m_FileName.setText("accepted-example.bin")
        self.ui.m_Path.setText("/local/stub/path/accepted-example.bin")
        self._set_status("Waiting")
        self._wire_offer_bar()
