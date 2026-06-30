from __future__ import annotations

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletInviteAccept_ui import Ui_AppletInviteAcceptUi


class InviteAcceptApplet(QWidget):
    """Python translation of nolimitgui AppletInviteAccept."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletInviteAcceptUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_InviteInfoButton.setText("?")
        self.ui.m_AcceptInviteButton.setText("Accept")
        self.ui.m_RejectInviteButton.setText("Reject")

        self.ui.m_InviteInfoButton.clicked.connect(self._show_invite_help)
        self.ui.m_ClipboardPasteWidget.textPasted.connect(self._apply_pasted_invite)
        self.ui.m_AcceptInviteButton.clicked.connect(self._accept_invite)
        self.ui.m_RejectInviteButton.clicked.connect(self._reject_invite)

    def _show_invite_help(self) -> None:
        QMessageBox.information(
            self,
            "Accept Invite",
            "Paste an invite, review details, then accept or reject.",
        )

    def _apply_pasted_invite(self, text: str) -> None:
        text = (text or "").strip()
        self.ui.m_InviteTextEdit.setPlainText(text)
        self.ui.m_InviteUrlWidget.set_invite_text(text)

        if text:
            self.ui.m_IdentWidget.set_display_name("Invite Sender")
            self.ui.m_IdentWidget.set_description("Invite payload received")
        else:
            self.ui.m_IdentWidget.set_display_name("Invite User")
            self.ui.m_IdentWidget.set_description("No invite text available")

    def _accept_invite(self) -> None:
        if not self.ui.m_InviteTextEdit.toPlainText().strip():
            QMessageBox.information(self, "Accept Invite", "Paste an invite first.")
            return
        QMessageBox.information(self, "Accept Invite", "Invite accepted flow is pending engine migration.")

    def _reject_invite(self) -> None:
        if not self.ui.m_InviteTextEdit.toPlainText().strip():
            QMessageBox.information(self, "Reject Invite", "Paste an invite first.")
            return
        QMessageBox.information(self, "Reject Invite", "Invite rejected.")
