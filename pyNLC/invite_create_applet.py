from __future__ import annotations

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletInviteCreate_ui import Ui_AppletInviteCreateUi


class InviteCreateApplet(QWidget):
    """Python translation of nolimitgui AppletInviteCreate."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletInviteCreateUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_InviteInfoButton.setText("?")
        self.ui.m_InviteInfoButton.clicked.connect(self._show_invite_help)

        self.ui.m_InviteUrlWidget.inviteTextChanged.connect(self._sync_copy_text)
        self.ui.m_InviteMessageTextEdit.textChanged.connect(self._sync_copy_text)
        self.ui.m_ClipboardCopyWidget.clicked.connect(self._copy_feedback)

        self._sync_copy_text()

    def _show_invite_help(self) -> None:
        QMessageBox.information(
            self,
            "Create Invite",
            "Select invite types, generate links, then copy the invite text to share.",
        )

    def _sync_copy_text(self) -> None:
        invite_urls = self.ui.m_InviteUrlWidget.invite_text()
        message = self.ui.m_InviteMessageTextEdit.toPlainText().strip()
        payload = invite_urls
        if message:
            payload = f"{payload}\n\nMessage:\n{message}" if payload else f"Message:\n{message}"
        self.ui.m_ClipboardCopyWidget.setCopyText(payload)

    def _copy_feedback(self) -> None:
        if self.ui.m_ClipboardCopyWidget.copyText():
            self.ui.m_InviteInfoLabel.setText("Invite copied to clipboard")
        else:
            self.ui.m_InviteInfoLabel.setText("Nothing to copy yet")
