from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletGalleryEmoticon_ui import Ui_AppletGalleryEmoticonUi
from resources.Forms.AppletGalleryImage_ui import Ui_AppletGalleryImageUi
from resources.Forms.AppletGalleryThumb_ui import Ui_AppletGalleryThumbUi
from resources.Forms.AppletPeerChangeFriendship_ui import Ui_AppletPeerChangeFriendshipUi
from resources.Forms.AppletPeerReplyOfferFile_ui import Ui_AppletPeerReplyFileOfferUi
from resources.Forms.AppletPeerSessionFileOffer_ui import Ui_AppletPeerSessionFileOfferUi
from resources.Forms.AppletPeerTodGame_ui import Ui_AppletPeerTodGameUi
from resources.Forms.AppletPeerVideoPhone_ui import Ui_AppletPeerVideoPhoneUi
from resources.Forms.AppletPeerVoicePhone_ui import Ui_AppletPeerVoicePhoneUi


class _BasePeerApplet(QWidget):
    def _set_status(self, label, text: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        label.setText(f"{text} @ {stamp}")


class PeerChangeFriendshipApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerChangeFriendshipUi()
        self.ui.setupUi(self)
        self.ui.m_AgeTextLabel.setText("21")
        self.ui.m_GenderTextLabel.setText("Unknown")
        self.ui.m_ContentTextLabel.setText("General")
        self.ui.m_LanguageTextLabel.setText("en")
        self.ui.m_MakeFriendButton.clicked.connect(lambda: self._set_status(self.ui.m_StatusLabel, "Friend request sent"))
        self.ui.m_IgnoreButton.clicked.connect(lambda: self._set_status(self.ui.m_StatusLabel, "Peer ignored"))


class PeerReplyOfferFileApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerReplyFileOfferUi()
        self.ui.setupUi(self)
        self.ui.m_PermissionLabel.setText("Offer a file (local)")
        self.ui.m_ViewUploadsButton.clicked.connect(lambda: self._set_status(self.ui.m_PermissionLabel, "Viewing uploads"))


class PeerTodGameApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerTodGameUi()
        self.ui.setupUi(self)
        self.ui.m_StateText.setText("Connected")
        self.ui.m_HangUpButton.clicked.connect(lambda: self._set_status(self.ui.m_StateText, "Ended"))


class PeerVideoPhoneApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerVideoPhoneUi()
        self.ui.setupUi(self)
        self.ui.m_StateText.setText("Connected")
        self.ui.m_HangUpButton.clicked.connect(lambda: self._set_status(self.ui.m_StateText, "Ended"))


class PeerVoicePhoneApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerVoicePhoneUi()
        self.ui.setupUi(self)
        self.ui.m_StateText.setText("Connected")
        self.ui.m_StatusLabel.setText("Voice session active")
        self.ui.m_HangUpButton.clicked.connect(lambda: self._set_status(self.ui.m_StateText, "Ended"))


class PeerSessionFileOfferApplet(_BasePeerApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPeerSessionFileOfferUi()
        self.ui.setupUi(self)
        self.ui.FileNameEdit.setText("sample.bin")
        self.ui.AcceptButton.clicked.connect(lambda: self._set_status(self.ui.m_StatusLabel, "Transfer continued"))
        self.ui.CancelButton.clicked.connect(lambda: self._set_status(self.ui.m_StatusLabel, "Transfer cancelled"))


class GalleryEmoticonApplet(QWidget):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGalleryEmoticonUi()
        self.ui.setupUi(self)
        for icon_name in [":)", ":D", ";)", "<3"]:
            self.ui.m_ImageListWidget.addItem(icon_name)


class GalleryImageApplet(QWidget):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGalleryImageUi()
        self.ui.setupUi(self)
        for i in range(1, 7):
            self.ui.m_ImageListWidget.addItem(f"image_{i:02d}.jpg")


class GalleryThumbApplet(QWidget):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletGalleryThumbUi()
        self.ui.setupUi(self)
        self.ui.m_ThumbDirLabel.setText("/local/thumbs")
        for i in range(1, 7):
            self.ui.m_ImageListWidget.addItem(f"thumb_{i:02d}.png")
