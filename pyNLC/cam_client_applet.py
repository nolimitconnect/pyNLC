from __future__ import annotations

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletCamClient_ui import Ui_AppletCamClientUi


class CamClientApplet(QWidget):
    """Python translation of AppletCamClient with placeholder preview state."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletCamClientUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_StatusMsgLabel.setText("Cam client ready")
        self.ui.m_CamVidWidget.setUserMessage("Waiting for remote cam stream")
        self.ui.m_CamVidWidget.setVideoPlaceholderText("Remote camera")

        self.ui.m_CamVidWidget.previewRequested.connect(self._on_preview_requested)
        self.ui.m_CamVidWidget.cameraEnabledChanged.connect(self._on_camera_toggled)

    def _on_preview_requested(self, enabled: bool) -> None:
        self.ui.m_StatusMsgLabel.setText("Remote preview requested" if enabled else "Remote preview stopped")

    def _on_camera_toggled(self, enabled: bool) -> None:
        self.ui.m_StatusMsgLabel.setText("Client camera enabled" if enabled else "Client camera disabled")
