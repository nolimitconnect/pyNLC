from __future__ import annotations

from PySide6.QtWidgets import QWidget

from resources.Forms.SessionWidget_ui import Ui_SessionWidgetUi


class SessionWidget(QWidget):
    """Compatibility session widget composed of invite, history, and chat entry."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_SessionWidgetUi()
        self.ui.setupUi(self)

        self.ui.m_CreateInviteButton.setText("Invite")
        self.ui.m_StatusLabel.setText("Session ready")

        self.ui.m_CreateInviteButton.clicked.connect(self._create_invite)
        self.ui.m_ChatEntry.textMessageSubmitted.connect(self._on_text_message)
        self.ui.m_ChatEntry.faceSelected.connect(self._on_face)

    def _create_invite(self) -> None:
        self.ui.m_HistoryList.addHistoryLine("Invite requested")
        self.ui.m_StatusLabel.setText("Invite requested")

    def _on_text_message(self, text: str) -> None:
        self.ui.m_HistoryList.addHistoryLine(f"You: {text}")
        self.ui.m_StatusLabel.setText("Text sent")

    def _on_face(self, face: str) -> None:
        self.ui.m_HistoryList.addHistoryLine(f"You reacted: {face}")
        self.ui.m_StatusLabel.setText("Reaction sent")
