from __future__ import annotations

from PySide6.QtCore import QUrl
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import QWidget

from resources.Forms.WebsiteWidget_ui import Ui_WebsiteWidgetUi


class WebsiteWidget(QWidget):
    """Compatibility website launcher widget."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_WebsiteWidgetUi()
        self.ui.setupUi(self)

        self.ui.m_WebsiteButton.setText("Go")
        self.ui.m_UrlLabel.setText("https://nolimitconnect.org")
        self.ui.m_WebsiteButton.clicked.connect(self._open_url)

    def set_url(self, url: str) -> None:
        self.ui.m_UrlLabel.setText(url.strip())

    def url(self) -> str:
        return self.ui.m_UrlLabel.text().strip()

    def _open_url(self) -> None:
        url = self.url()
        if url:
            QDesktopServices.openUrl(QUrl(url))
