from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputVideoWidget_ui import Ui_InputVideoWidget


class InputVideoWidget(QWidget):
    """Compatibility video input widget."""

    backRequested = Signal()
    rotateRequested = Signal()
    selectSourceRequested = Signal()
    canceled = Signal()
    recordingToggled = Signal(bool)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputVideoWidget()
        self.ui.setupUi(self)
        self._recording = False

        self.ui.m_BackButton.setText("Back")
        self.ui.m_RotateCamButton.setText("Rot")
        self.ui.m_SelectVidSrcButton.setText("Src")
        self.ui.m_CancelRecordButton.setText("X")
        self.ui.m_StartStopRecButton.setText("Rec")

        self.ui.m_BackButton.clicked.connect(self.backRequested.emit)
        self.ui.m_RotateCamButton.clicked.connect(self.rotateRequested.emit)
        self.ui.m_SelectVidSrcButton.clicked.connect(self.selectSourceRequested.emit)
        self.ui.m_CancelRecordButton.clicked.connect(self.canceled.emit)
        self.ui.m_StartStopRecButton.clicked.connect(self._toggle_recording)

    def _toggle_recording(self) -> None:
        self._recording = not self._recording
        self.ui.m_StartStopRecButton.setText("Stop" if self._recording else "Rec")
        self.recordingToggled.emit(self._recording)
