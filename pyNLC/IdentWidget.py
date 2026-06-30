from __future__ import annotations

from PySide6.QtWidgets import QWidget

from resources.Forms.IdentWidget_ui import Ui_IdentWidget


class IdentWidget(QWidget):
    """Compatibility ident card widget for invite and host views."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_IdentWidget()
        self.ui.setupUi(self)

        self.ui.m_AvatarButton.setText("A")
        self.ui.m_PushToTalkButton.setText("PTT")
        self.ui.m_FriendshipButton.setText("Friend")
        self.ui.m_OfferViewButton.setText("View")
        self.ui.m_OfferInfoButton.setText("Info")
        self.ui.m_OfferAcceptButton.setText("Accept")
        self.ui.m_OfferRejectButton.setText("Reject")
        self.ui.m_FriendMenuButton.setText("Menu")

        self.set_display_name("Invite User")
        self.set_description("Invite details not resolved yet")

    def set_display_name(self, name: str) -> None:
        self.ui.m_FriendNameLabel.setText(name)

    def set_description(self, text: str) -> None:
        self.ui.m_DescTextLabel.setText(text)
