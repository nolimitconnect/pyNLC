from __future__ import annotations

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletGetStarted_ui import Ui_AppletGetStartedUi


class GetStartedApplet(QWidget):
    """Python translation of nolimitgui AppletGetStarted."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGetStartedUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_BackButton.setText("Back")
        self.ui.m_ExpandButton.setText("Expand")
        self.ui.m_ShrinkButton.setText("Shrink")
        self.ui.m_JoinGroupButton.setText("Join")
        self.ui.m_JoinChatRoomButton.setText("Join")
        self.ui.m_JoinRandomConnectButton.setText("Join")
        self.ui.m_PersonalNotesButton.setText("Open")
        self.ui.m_LibraryButton.setText("Open")
        self.ui.m_IdentityButton.setText("Open")
        self.ui.m_AboutMeButton.setText("Open")
        self.ui.m_StoryboardButton.setText("Open")
        self.ui.m_WebCamButton.setText("Open")

        self.ui.m_WebsiteWidget.set_url("https://nolimitconnect.org")

        self.ui.m_BackButton.clicked.connect(lambda: self._hint("Use the Home navigation to go back."))
        self.ui.m_ExpandButton.clicked.connect(lambda: self._hint("Use shell expand controls (migration pending)."))
        self.ui.m_ShrinkButton.clicked.connect(lambda: self._hint("Use shell shrink controls (migration pending)."))

        self.ui.m_JoinGroupButton.clicked.connect(lambda: self._hint("Open Group Join applet from Home."))
        self.ui.m_JoinChatRoomButton.clicked.connect(lambda: self._hint("Open Chat Room Join applet from Home."))
        self.ui.m_JoinRandomConnectButton.clicked.connect(lambda: self._hint("Open Random Connect Join applet from Home."))

        self.ui.m_PersonalNotesButton.clicked.connect(lambda: self._hint("Personal recorder is scheduled for later migration."))
        self.ui.m_LibraryButton.clicked.connect(lambda: self._hint("Library is scheduled for later migration."))
        self.ui.m_IdentityButton.clicked.connect(lambda: self._hint("Open User Identity applet from Home."))
        self.ui.m_AboutMeButton.clicked.connect(lambda: self._hint("About Me editing is part of profile migration."))
        self.ui.m_StoryboardButton.clicked.connect(lambda: self._hint("Storyboard editing is part of profile migration."))
        self.ui.m_WebCamButton.clicked.connect(lambda: self._hint("WebCam applet is deferred with media/cam migration."))

    def _hint(self, message: str) -> None:
        QMessageBox.information(self, "Get Started", message)
