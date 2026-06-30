from __future__ import annotations

from PySide6.QtWidgets import QDialog, QPushButton, QVBoxLayout, QWidget

from resources.Forms.AppletCamSettings_ui import Ui_AppletCamSettingsUi


class CamSettingsApplet(QWidget):
    """Python translation of AppletCamSettings with media placeholder wiring."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletCamSettingsUi()
        self.ui.setupUi(self)
        self.settings = settings
        self._cam_client_dialog: QDialog | None = None

        self.ui.m_InDeviceComboBox.clear()
        self.ui.m_InDeviceComboBox.addItems(["Default Camera", "Integrated Camera", "USB Camera"])

        self.ui.m_ApplyVideoInDeviceButton.clicked.connect(self._apply_device)
        self.ui.m_InDeviceComboBox.currentTextChanged.connect(self._preview_device)

        self._open_client_button = QPushButton("Open Cam Client", self)
        self.layout().insertWidget(2, self._open_client_button)
        self._open_client_button.clicked.connect(self._open_cam_client)

        self.ui.m_CamVidWidget.setUserMessage("Camera preview ready")
        self.ui.m_CamVidWidget.setVideoPlaceholderText("Cam preview")

        self.ui.m_CamVidWidget.cameraEnabledChanged.connect(self._on_camera_enabled_changed)
        self.ui.m_CamVidWidget.previewRequested.connect(self._on_preview_requested)
        self.ui.m_CamVidWidget.sourceSelectionRequested.connect(self._open_source_picker)

    def _apply_device(self) -> None:
        device = self.ui.m_InDeviceComboBox.currentText()
        self.ui.m_StatusMsgLabel.setText(f"Applied default camera: {device}")

    def _preview_device(self, device: str) -> None:
        self.ui.m_StatusMsgLabel.setText(f"Selected camera: {device}")
        self.ui.m_CamVidWidget.setVideoPlaceholderText(f"Preview: {device}")

    def _on_camera_enabled_changed(self, enabled: bool) -> None:
        self.ui.m_StatusMsgLabel.setText("Camera enabled" if enabled else "Camera disabled")

    def _on_preview_requested(self, enabled: bool) -> None:
        self.ui.m_StatusMsgLabel.setText("Preview started" if enabled else "Preview stopped")

    def _open_source_picker(self) -> None:
        self.ui.m_StatusMsgLabel.setText("Source picker requested")

    def _open_cam_client(self) -> None:
        from cam_client_applet import CamClientApplet

        if self._cam_client_dialog is None:
            dialog = QDialog(self)
            dialog.setWindowTitle("Cam Client")
            layout = QVBoxLayout(dialog)
            layout.setContentsMargins(4, 4, 4, 4)
            layout.addWidget(CamClientApplet(self.settings, dialog))
            self._cam_client_dialog = dialog

        self._cam_client_dialog.show()
        self._cam_client_dialog.raise_()
        self._cam_client_dialog.activateWindow()
