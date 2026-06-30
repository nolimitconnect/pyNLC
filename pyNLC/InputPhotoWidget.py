from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputPhotoWidget_ui import Ui_InputPhotoWidget


class InputPhotoWidget(QWidget):
    """Compatibility photo input widget."""

    backRequested = Signal()
    snapshotRequested = Signal()
    rotateRequested = Signal()
    selectCameraRequested = Signal()
    canceled = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputPhotoWidget()
        self.ui.setupUi(self)

        self.ui.m_BackButton.setText("Back")
        self.ui.m_SnapShotButton.setText("Snap")
        self.ui.m_RotateCamButton.setText("Rot")
        self.ui.m_SelectCamButton.setText("Src")
        self.ui.m_CancelPhotoButton.setText("X")

        self.ui.m_BackButton.clicked.connect(self.backRequested.emit)
        self.ui.m_SnapShotButton.clicked.connect(self.snapshotRequested.emit)
        self.ui.m_RotateCamButton.clicked.connect(self.rotateRequested.emit)
        self.ui.m_SelectCamButton.clicked.connect(self.selectCameraRequested.emit)
        self.ui.m_CancelPhotoButton.clicked.connect(self.canceled.emit)
