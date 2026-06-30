from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.ChatEntryWidget_ui import Ui_ChatEntryWidget


class ChatEntryWidget(QWidget):
    """Compatibility chat entry surface that switches between input modes."""

    textMessageSubmitted = Signal(str)
    faceSelected = Signal(str)
    modeChanged = Signal(str)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_ChatEntryWidget()
        self.ui.setupUi(self)

        self._mode_map = {
            "all": self.ui.m_InputAllWidget,
            "text": self.ui.m_InputTextWidget,
            "face": self.ui.m_InputFaceWidget,
            "photo": self.ui.m_InputPhotoWidget,
            "video": self.ui.m_InputVideoWidget,
            "voice": self.ui.m_InputVoiceWidget,
        }

        self.ui.m_InputAllWidget.inputModeRequested.connect(self.setInputMode)
        self.ui.m_InputTextWidget.sendTextRequested.connect(self.textMessageSubmitted.emit)
        self.ui.m_InputTextWidget.canceled.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputFaceWidget.faceSelected.connect(self.faceSelected.emit)
        self.ui.m_InputFaceWidget.canceled.connect(lambda: self.setInputMode("all"))

        self.ui.m_InputPhotoWidget.backRequested.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputPhotoWidget.canceled.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputVideoWidget.backRequested.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputVideoWidget.canceled.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputVoiceWidget.backRequested.connect(lambda: self.setInputMode("all"))
        self.ui.m_InputVoiceWidget.canceled.connect(lambda: self.setInputMode("all"))

        self.setInputMode("all")

    def setInputMode(self, mode: str) -> None:
        mode = mode if mode in self._mode_map else "all"
        for key, widget in self._mode_map.items():
            widget.setVisible(key == mode)
        self.modeChanged.emit(mode)
