from __future__ import annotations

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletMultiMessenger_ui import Ui_AppletMultiMessengerUi


class MultiMessengerApplet(QWidget):
    """Python translation of AppletMultiMessenger with session placeholder behavior."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletMultiMessengerUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_AcceptOfferButton.setText("Accept")
        self.ui.m_RejectOfferButton.setText("Reject")
        self.ui.m_HangupSessionButton.setText("Hangup")
        self.ui.m_AcceptLabel.setText("Accept session")
        self.ui.m_RejectLabel.setText("Reject session")

        self.ui.m_VidWidget.setUserMessage("Session video surface")
        self.ui.m_VidWidget.setVideoPlaceholderText("MultiMessenger stream")

        self.ui.m_AcceptOfferButton.clicked.connect(lambda: self.ui.m_SessionWidget.ui.m_StatusLabel.setText("Offer accepted"))
        self.ui.m_RejectOfferButton.clicked.connect(lambda: self.ui.m_SessionWidget.ui.m_StatusLabel.setText("Offer rejected"))
        self.ui.m_HangupSessionButton.clicked.connect(lambda: self.ui.m_SessionWidget.ui.m_StatusLabel.setText("Session ended"))
