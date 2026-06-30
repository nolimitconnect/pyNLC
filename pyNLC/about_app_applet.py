from __future__ import annotations

import platform

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletAboutApp_ui import Ui_AppletAboutAppUi


class AboutAppApplet(QWidget):
    """Python translation of nolimitgui AppletAboutApp."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletAboutAppUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.widget.set_url("https://nolimitconnect.org")
        self.ui.m_CopyDeviceInfoToClipboard.setText("Copy Device Info")
        self.ui.m_CopyConnectionInfoToClipboard.setText("Copy Connection Info")

        self._populate_details()

        self.ui.m_ShowAppInfo.clicked.connect(self._show_app_info)
        self.ui.m_CopyDeviceInfoToClipboard.clicked.connect(self._copy_device_info)
        self.ui.m_CopyConnectionInfoToClipboard.clicked.connect(self._copy_connection_info)

    def _populate_details(self) -> None:
        device_info = "\n".join(
            [
                f"System: {platform.system()}",
                f"Release: {platform.release()}",
                f"Version: {platform.version()}",
                f"Machine: {platform.machine()}",
                f"Processor: {platform.processor()}",
            ]
        )
        connection_info = "\n".join(
            [
                "Network State: Pending engine status binding",
                "External Reachability: Pending engine status binding",
                "Active Host Services: Pending migration",
            ]
        )

        self.ui.m_DeviceInfoTextEdit.setPlainText(device_info)
        self.ui.m_ConnectionInfoTextEdit.setPlainText(connection_info)
        self.ui.m_CopyDeviceInfoToClipboard.setCopyText(device_info)
        self.ui.m_CopyConnectionInfoToClipboard.setCopyText(connection_info)

    def _show_app_info(self) -> None:
        QMessageBox.information(
            self,
            "Application Information",
            "NoLimitConnect Python migration shell. Additional runtime details will be filled from engine bindings.",
        )

    def _copy_device_info(self) -> None:
        self.ui.m_DeviceInfoLabel.setText("Device Information (copied)")

    def _copy_connection_info(self) -> None:
        self.ui.m_ConnectionInfoLabel.setText("Connection Information (copied)")
