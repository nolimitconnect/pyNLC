from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputAllWidget_ui import Ui_InputAllWidget


class InputAllWidget(QWidget):
    """Compatibility widget for choosing an input mode."""

    inputModeRequested = Signal(str)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputAllWidget()
        self.ui.setupUi(self)

        self.ui.m_TextAllButton.clicked.connect(lambda: self.inputModeRequested.emit("text"))
        self.ui.m_SelectFaceAllButton.clicked.connect(lambda: self.inputModeRequested.emit("face"))
        self.ui.m_CameraAllButton.clicked.connect(lambda: self.inputModeRequested.emit("photo"))
        self.ui.m_GalleryAllButton.clicked.connect(lambda: self.inputModeRequested.emit("photo"))
        self.ui.m_VideoAllButton.clicked.connect(lambda: self.inputModeRequested.emit("video"))
        self.ui.m_MicAllButton.clicked.connect(lambda: self.inputModeRequested.emit("voice"))

        self.ui.m_SelectFaceAllButton.setText(":)")
        self.ui.m_CameraAllButton.setText("Cam")
        self.ui.m_GalleryAllButton.setText("Gal")
        self.ui.m_VideoAllButton.setText("Vid")
        self.ui.m_MicAllButton.setText("Mic")

