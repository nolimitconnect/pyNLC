from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtWidgets import QWidget

from resources.Forms.VidWidget_ui import Ui_VidWidgetClass


class VidWidget(QWidget):
    """Compatibility video widget used across player/cam/input forms."""

    cameraEnabledChanged = Signal(bool)
    previewRequested = Signal(bool)
    sourceSelectionRequested = Signal()
    rotateCameraRequested = Signal()
    rotateImageRequested = Signal()
    snapshotRequested = Signal()
    normalRecordToggled = Signal(bool)
    motionRecordToggled = Signal(bool)
    motionAlarmToggled = Signal(bool)
    filesRequested = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_VidWidgetClass()
        self.ui.setupUi(self)

        self._camera_enabled = True
        self._preview_enabled = False
        self._normal_recording = False
        self._motion_recording = False
        self._motion_alarm = False

        self._wire_controls()
        self._refresh_control_text()

    def _wire_controls(self) -> None:
        self.ui.m_CamSourceButton.clicked.connect(self.sourceSelectionRequested.emit)
        self.ui.m_CamPreviewButton.clicked.connect(self._toggle_preview)
        self.ui.m_CamRotateButton.clicked.connect(self.rotateCameraRequested.emit)
        self.ui.m_ImageRotateButton.clicked.connect(self.rotateImageRequested.emit)
        self.ui.m_CamEnableButton.clicked.connect(self._toggle_camera)

        self.ui.m_PictureSnapshotButton.clicked.connect(self.snapshotRequested.emit)
        self.ui.m_NormalRecordButton.clicked.connect(self._toggle_normal_record)
        self.ui.m_MotionRecordButton.clicked.connect(self._toggle_motion_record)
        self.ui.m_MotionAlarmButton.clicked.connect(self._toggle_motion_alarm)
        self.ui.m_VidFilesButton.clicked.connect(self.filesRequested.emit)

        self.ui.m_MotionSensitivitySlider.valueChanged.connect(self.ui.m_MotionBar.setValue)

    def _refresh_control_text(self) -> None:
        self.ui.m_CamSourceButton.setText("Src")
        self.ui.m_CamPreviewButton.setText("Prev" if not self._preview_enabled else "Stop")
        self.ui.m_CamRotateButton.setText("Rot")
        self.ui.m_ImageRotateButton.setText("Img")
        self.ui.m_CamEnableButton.setText("Cam" if self._camera_enabled else "Off")

        self.ui.m_PictureSnapshotButton.setText("Snap")
        self.ui.m_NormalRecordButton.setText("Rec" if not self._normal_recording else "Stop")
        self.ui.m_MotionRecordButton.setText("MRec" if not self._motion_recording else "MStop")
        self.ui.m_MotionAlarmButton.setText("Alarm" if not self._motion_alarm else "Armed")
        self.ui.m_VidFilesButton.setText("Files")

    def _toggle_preview(self) -> None:
        self._preview_enabled = not self._preview_enabled
        self._refresh_control_text()
        self.previewRequested.emit(self._preview_enabled)

    def _toggle_camera(self) -> None:
        self._camera_enabled = not self._camera_enabled
        self._refresh_control_text()
        self.cameraEnabledChanged.emit(self._camera_enabled)

    def _toggle_normal_record(self) -> None:
        self._normal_recording = not self._normal_recording
        self._refresh_control_text()
        self.normalRecordToggled.emit(self._normal_recording)

    def _toggle_motion_record(self) -> None:
        self._motion_recording = not self._motion_recording
        self._refresh_control_text()
        self.motionRecordToggled.emit(self._motion_recording)

    def _toggle_motion_alarm(self) -> None:
        self._motion_alarm = not self._motion_alarm
        self._refresh_control_text()
        self.motionAlarmToggled.emit(self._motion_alarm)

    def setUserMessage(self, text: str) -> None:
        self.ui.m_UserMsgLabel.setText(text)

    def setCameraEnabled(self, enabled: bool) -> None:
        self._camera_enabled = bool(enabled)
        self._refresh_control_text()

    def setPreviewEnabled(self, enabled: bool) -> None:
        self._preview_enabled = bool(enabled)
        self._refresh_control_text()

    def setVideoPlaceholderText(self, text: str) -> None:
        self.ui.m_VideoScreen.setText(text)

    def setVideoPixmap(self, pixmap: QPixmap) -> None:
        self.ui.m_VideoScreen.setPixmap(pixmap)

    def setVideoImage(self, image: QImage) -> None:
        self.ui.m_VideoScreen.setPixmap(QPixmap.fromImage(image))

    def clearVideo(self) -> None:
        self.ui.m_VideoScreen.clear()
