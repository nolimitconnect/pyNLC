from __future__ import annotations

import importlib
import random
from datetime import datetime
from typing import TYPE_CHECKING

from PySide6.QtWidgets import QLineEdit, QMessageBox, QWidget

from resources.Forms.AppletNetworkSettings_ui import Ui_AppletNetworkSettingsUi

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class NetworkSettingsApplet(QWidget):
    """Python translation of nolimitgui AppletNetworkSettings (non-engine subset)."""

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletNetworkSettingsUi()
        self.ui.setupUi(self)

        self.settings = settings
        self._network_key_visible = False

        self.ui.m_NetworkKeyEdit.setEchoMode(QLineEdit.Password)

        self._connect_signals()
        self._update_dlg_from_settings(initial_settings=True)

    def _connect_signals(self) -> None:
        self.ui.RandomPortButton.clicked.connect(self.slot_random_port_button_click)
        self.ui.m_TestIsPortOpenButton.clicked.connect(self.slot_test_is_my_port_open_button_click)
        self.ui.m_TestUpnpButton.clicked.connect(self.slot_test_upnp_button_click)

        self.ui.AutoDetectProxyRadioButton.clicked.connect(self.slot_auto_detect_proxy_click)
        self.ui.AssumeNoProxyRadioButton.clicked.connect(self.slot_no_proxy_click)

        self.ui.m_UseUpnpCheckBox.clicked.connect(self.slot_use_upnp_checkbox_click)
        self.ui.m_UseIpv6NetworkCheckBox.clicked.connect(self.slot_use_ipv6_checkbox_click)

        self.ui.m_SaveSettingsButton.clicked.connect(self.on_save_button_click)
        self.ui.m_SaveSettingsLabel.clicked.connect(self.slot_save_label_click)

        self.ui.m_DeleteSettingsButton.clicked.connect(self.on_delete_button_click)
        self.ui.m_DeleteSettingsLabel.clicked.connect(self.slot_delete_label_click)

        self.ui.m_ClipboardCopyWidget.clicked.connect(self.slot_copy_my_url_to_clipboard)

        self.ui.m_NetworkHostInfoButton.clicked.connect(self.slot_show_network_host_information)
        self.ui.m_NetworkKeyInfoButton.clicked.connect(self.slot_show_network_key_information)
        self.ui.m_ConnectTestUrlInfoButton.clicked.connect(self.slot_show_connect_test_url_information)
        self.ui.m_ConnectIsOpenInfoButton.clicked.connect(self.slot_show_connect_test_settings_information)
        self.ui.m_NetworkKeyEyeButton.clicked.connect(self.slot_network_key_eye_button_click)

    def _settings_get(self, getter_name: str, default):
        if self.settings is None:
            return default
        getter = getattr(self.settings, getter_name)
        return getter()

    def _settings_set(self, setter_name: str, value) -> None:
        if self.settings is None:
            return
        setter = getattr(self.settings, setter_name)
        setter(value)

    def _update_dlg_from_settings(self, initial_settings: bool) -> None:
        del initial_settings
        settings_name = str(self._settings_get("getLastNetHostSettingName", "default")) or "default"

        self.ui.m_NetworkSettingsNameComboBox.clear()
        self.ui.m_NetworkSettingsNameComboBox.addItem(settings_name)
        self.ui.m_NetworkSettingsNameComboBox.setCurrentIndex(0)

        self.ui.PortEdit.setText(str(self._settings_get("getTcpPort", 45124)))
        self.ui.m_NetworkHostUrlEdit.setText(str(self._settings_get("getNetworkHostUrl", "https://nolimitconnect.org")))
        self.ui.m_NetworkKeyEdit.setText(str(self._settings_get("getNetworkKey", "")))
        self.ui.m_ConnectTestUrlEdit.setText(str(self._settings_get("getConnectTestUrl", "https://nolimitconnect.org/test")))
        self.ui.m_ExternIpEdit.setText(str(self._settings_get("getUserSpecifiedExternIpAddr", "")))

        firewall_type = int(self._settings_get("getFirewallTestType", 0))
        self.ui.AutoDetectProxyRadioButton.setChecked(firewall_type == 0)
        self.ui.AssumeNoProxyRadioButton.setChecked(firewall_type == 1)

        self.ui.m_UseUpnpCheckBox.setChecked(bool(self._settings_get("getUseUpnpPortForward", False)))
        self.ui.m_UseIpv6NetworkCheckBox.setChecked(bool(self._settings_get("getUseIpv6", False)))

        node_url = self._build_node_url()
        self.ui.m_NodeUrlLabel.setText(node_url)
        self.ui.m_ClipboardCopyWidget.setCopyText(node_url)

    def _update_settings_from_dlg(self) -> None:
        settings_name = self.ui.m_NetworkSettingsNameComboBox.currentText().strip() or "default"

        self._settings_set("setLastNetHostSettingName", settings_name)
        self._settings_set("setTcpPort", self._parse_port(self.ui.PortEdit.text(), fallback=45124))
        self._settings_set("setNetworkHostUrl", self.ui.m_NetworkHostUrlEdit.text().strip())
        self._settings_set("setNetworkKey", self.ui.m_NetworkKeyEdit.text())
        self._settings_set("setConnectTestUrl", self.ui.m_ConnectTestUrlEdit.text().strip())
        self._settings_set("setUserSpecifiedExternIpAddr", self.ui.m_ExternIpEdit.text().strip())
        self._settings_set("setFirewallTestType", self.get_firewall_test_type())
        self._settings_set("setUseUpnpPortForward", self.ui.m_UseUpnpCheckBox.isChecked())
        self._settings_set("setUseIpv6", self.ui.m_UseIpv6NetworkCheckBox.isChecked())

        node_url = self._build_node_url()
        self.ui.m_NodeUrlLabel.setText(node_url)
        self.ui.m_ClipboardCopyWidget.setCopyText(node_url)

    @staticmethod
    def _parse_port(text: str, fallback: int) -> int:
        try:
            value = int(text.strip())
        except (TypeError, ValueError):
            return fallback
        if value <= 0:
            return fallback
        return value

    def _build_node_url(self) -> str:
        host = self.ui.m_ExternIpEdit.text().strip() or "127.0.0.1"
        port = self._parse_port(self.ui.PortEdit.text(), fallback=45124)
        return f"nlc://{host}:{port}"

    def set_firewall_test_type(self, firewall_type: int) -> None:
        self.ui.AutoDetectProxyRadioButton.setChecked(firewall_type == 0)
        self.ui.AssumeNoProxyRadioButton.setChecked(firewall_type == 1)

    def get_firewall_test_type(self) -> int:
        return 1 if self.ui.AssumeNoProxyRadioButton.isChecked() else 0

    def slot_random_port_button_click(self) -> None:
        self.ui.PortEdit.setText(str(random.randint(10000, 65535)))
        self._update_settings_from_dlg()

    def slot_test_is_my_port_open_button_click(self) -> None:
        port = self._parse_port(self.ui.PortEdit.text(), fallback=0)
        if port < 10000:
            QMessageBox.information(self, "TCP Listen Port Error", "TCP Listen Port cannot be less than 10000.")
            return

        QMessageBox.information(self, "Port Test", f"Port test would run for port {port} (engine migration pending).")

    def slot_test_upnp_button_click(self) -> None:
        QMessageBox.information(self, "UPNP Test", "UPNP test is not yet connected to engine in pyNLC.")

    def slot_auto_detect_proxy_click(self) -> None:
        self.set_firewall_test_type(0)

    def slot_no_proxy_click(self) -> None:
        self.set_firewall_test_type(1)

    def slot_use_upnp_checkbox_click(self) -> None:
        self._update_settings_from_dlg()

    def slot_use_ipv6_checkbox_click(self) -> None:
        self._update_settings_from_dlg()

    def on_save_button_click(self) -> None:
        self._update_settings_from_dlg()
        self.ui.m_SaveSettingsLabel.setText("Saved Network Setting")

    def slot_save_label_click(self) -> None:
        self.on_save_button_click()

    def on_delete_button_click(self) -> None:
        self.ui.m_NetworkSettingsNameComboBox.clear()
        self.ui.m_NetworkSettingsNameComboBox.addItem("default")
        self.ui.m_NetworkSettingsNameComboBox.setCurrentIndex(0)
        self._update_settings_from_dlg()
        self.ui.m_DeleteSettingsLabel.setText("Deleted Current Setting")

    def slot_delete_label_click(self) -> None:
        self.on_delete_button_click()

    def slot_show_network_host_information(self) -> None:
        QMessageBox.information(self, "Network Host URL", "Defines the service endpoint used for host discovery.")

    def slot_show_network_key_information(self) -> None:
        QMessageBox.information(self, "Network Key", "Network key is used to scope your network grouping.")

    def slot_show_connect_test_url_information(self) -> None:
        QMessageBox.information(self, "Connect Test URL", "Service URL used to verify external reachability.")

    def slot_show_connect_test_settings_information(self) -> None:
        QMessageBox.information(
            self,
            "Connection Test Settings",
            "Auto detect uses external tests. Assume no proxy skips network probing.",
        )

    def slot_copy_my_url_to_clipboard(self) -> None:
        self.ui.m_ClipboardCopyWidget.setCopyText(self.ui.m_NodeUrlLabel.text())

    def slot_network_key_eye_button_click(self) -> None:
        self._network_key_visible = not self._network_key_visible
        mode = QLineEdit.Normal if self._network_key_visible else QLineEdit.Password
        self.ui.m_NetworkKeyEdit.setEchoMode(mode)

    def on_net_available_status(self, status: int, timestamp_ms: int | None = None) -> None:
        describe = getattr(nlc_engine, "describe_net_avail_status", None) if nlc_engine is not None else None
        status_text = describe(status) if callable(describe) else str(status)
        suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S") if timestamp_ms is not None else ""
        self.ui.m_NetAvailStateLabel.setText(f"Net Avail: {status_text}{suffix}")

    def on_network_state(self, state: int, state_message: str, timestamp_ms: int | None = None) -> None:
        describe = getattr(nlc_engine, "describe_network_state", None) if nlc_engine is not None else None
        state_text = describe(state) if callable(describe) else str(state)
        suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S") if timestamp_ms is not None else ""
        if state_message:
            self.ui.m_InternetStateLabel.setText(f"Net State {state_text}: {state_message}{suffix}")
        else:
            self.ui.m_InternetStateLabel.setText(f"Net State: {state_text}{suffix}")
