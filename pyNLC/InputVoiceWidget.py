from __future__ import annotations

from PySide6.QtCore import QTimer, Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputVoiceWidget_ui import Ui_InputVoiceWidget


class InputVoiceWidget(QWidget):
    """Compatibility voice input widget."""

    backRequested = Signal()
    canceled = Signal()
    recordingToggled = Signal(bool)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputVoiceWidget()
        self.ui.setupUi(self)

        self._recording = False
        self._seconds = 0
        self._timer = QTimer(self)
        self._timer.setInterval(1000)
        self._timer.timeout.connect(self._tick)

        self.ui.m_BackButton.setText("Back")
        self.ui.m_CancelRecordButton.setText("X")
        self.ui.m_RecVoiceButton.setText("Mic")
        self.ui.m_RecLenLabel.setText("00:00")

        self.ui.m_BackButton.clicked.connect(self.backRequested.emit)
        self.ui.m_CancelRecordButton.clicked.connect(self._cancel)
        self.ui.m_RecVoiceButton.clicked.connect(self._toggle_record)

    def _toggle_record(self) -> None:
        self._recording = not self._recording
        if self._recording:
            self._seconds = 0
            self.ui.m_RecVoiceButton.setText("Stop")
            self._timer.start()
        else:
            self.ui.m_RecVoiceButton.setText("Mic")
            self._timer.stop()
        self.recordingToggled.emit(self._recording)

    def _tick(self) -> None:
        self._seconds += 1
        mm = self._seconds // 60
        ss = self._seconds % 60
        self.ui.m_RecLenLabel.setText(f"{mm:02d}:{ss:02d}")

    def _cancel(self) -> None:
        self._recording = False
        self._seconds = 0
        self._timer.stop()
        self.ui.m_RecVoiceButton.setText("Mic")
        self.ui.m_RecLenLabel.setText("00:00")
        self.canceled.emit()
