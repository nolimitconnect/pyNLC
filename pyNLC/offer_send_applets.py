from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletOfferRandSession_ui import Ui_AppletOfferRandSessionUi
from resources.Forms.AppletOfferSend_ui import Ui_AppletOfferSendUi
from resources.Forms.AppletOfferView_ui import Ui_AppletOfferViewUi


class OfferSendApplet(QWidget):
    """Concrete local offer-send applet."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self.ui = Ui_AppletOfferSendUi()
        self.ui.setupUi(self)
        self.ui.m_StatusMsgLabel.setText("Ready to send (local)")
        self.ui.m_OfferSendWidget.sendClicked.connect(self._on_send)

    def _on_send(self, message: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_StatusMsgLabel.setText(f"Sent @ {stamp}")
        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.offer_send.last_message"] = message


class OfferViewApplet(QWidget):
    """Concrete local offer-view applet with response controls."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletOfferViewUi()
        self.ui.setupUi(self)
        self.ui.m_StatusMsgLabel.setText("Viewing offer (local)")
        self.ui.m_OfferExpireTime.setText("On response")
        self.ui.m_FileName.setText("view-example.bin")
        self.ui.m_Path.setText("/local/stub/path/view-example.bin")

        self.ui.m_OfferSendWidget.sendClicked.connect(lambda msg: self.ui.m_StatusMsgLabel.setText("Reply queued" if msg else "Reply empty"))
        self.ui.m_OfferBarWidget.acceptClicked.connect(lambda: self.ui.m_StatusMsgLabel.setText("Accepted"))
        self.ui.m_OfferBarWidget.rejectClicked.connect(lambda: self.ui.m_StatusMsgLabel.setText("Rejected"))
        self.ui.m_OfferBarWidget.cancelClicked.connect(lambda: self.ui.m_StatusMsgLabel.setText("Cancelled"))


class OfferRandSessionApplet(QWidget):
    """Concrete local random-session offer applet."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self.ui = Ui_AppletOfferRandSessionUi()
        self.ui.setupUi(self)

        self.ui.m_ToVoicePhoneButton.setChecked(True)
        self.ui.m_SelectedPluginLabel.setText("Voice Phone")
        self.ui.m_StatusMsgLabel.setText("Choose a session to offer")

        self.ui.m_ToVoicePhoneButton.toggled.connect(lambda checked: checked and self._set_plugin("Voice Phone"))
        self.ui.m_ToVideoChatButton.toggled.connect(lambda checked: checked and self._set_plugin("Video Chat"))
        self.ui.m_ToTruthOrDareButton.toggled.connect(lambda checked: checked and self._set_plugin("Truth Or Dare"))
        self.ui.m_OfferSendWidget.sendClicked.connect(self._on_send)

    def _set_plugin(self, plugin: str) -> None:
        self.ui.m_SelectedPluginLabel.setText(plugin)

    def _on_send(self, message: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        plugin = self.ui.m_SelectedPluginLabel.text() or "Session"
        self.ui.m_StatusMsgLabel.setText(f"{plugin} offer sent @ {stamp}")
        if self.settings is not None:
            store = getattr(self.settings, "_settings_store", None)
            if isinstance(store, dict):
                store["ui.offer_rand_session.last_message"] = message
                store["ui.offer_rand_session.last_plugin"] = plugin
