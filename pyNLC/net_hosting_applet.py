from __future__ import annotations

import importlib
from datetime import datetime

from PySide6.QtWidgets import QCheckBox, QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class NetHostingApplet(QWidget):
    """Concrete non-media applet for Network Hosting page."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings

        layout = QVBoxLayout(self)
        title = QLabel("Network Hosting", self)
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        layout.addWidget(title)

        self._url_label = QLabel("URL: nlc://127.0.0.1:45124", self)
        layout.addWidget(self._url_label)

        self._hosting_status = QLabel("Status: waiting for network events", self)
        layout.addWidget(self._hosting_status)

        self._open_port = QCheckBox("Has open port to receive connections", self)
        self._host_perm = QCheckBox("Host permission is enabled", self)
        self._test_perm = QCheckBox("Connection test is enabled", self)
        layout.addWidget(self._open_port)
        layout.addWidget(self._host_perm)
        layout.addWidget(self._test_perm)

        self._requirements = QPushButton("View Hosting Requirements", self)
        self._requirements.clicked.connect(self._show_requirements)
        layout.addWidget(self._requirements)

        self._events = QListWidget(self)
        layout.addWidget(self._events)

        self._load_from_settings()

    def _store(self) -> dict:
        if self.settings is None:
            return {}
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            return store
        return {}

    def _load_from_settings(self) -> None:
        store = self._store()
        host = str(store.get("ui.net_hosting.host", "127.0.0.1"))
        port = int(store.get("ui.net_hosting.port", 45124))
        self._url_label.setText(f"URL: nlc://{host}:{port}")
        self._open_port.setChecked(bool(store.get("ui.net_hosting.open_port", False)))
        self._host_perm.setChecked(bool(store.get("ui.net_hosting.host_permission", True)))
        self._test_perm.setChecked(bool(store.get("ui.net_hosting.connection_test_permission", True)))

    def _show_requirements(self) -> None:
        self._events.addItem(QListWidgetItem("Requirements: open port, host permission, connection-test permission"))

    def on_net_available_status(self, status: int, timestamp_ms: int | None = None) -> None:
        describe = getattr(nlc_engine, "describe_net_avail_status", None) if nlc_engine is not None else None
        text = describe(status) if callable(describe) else str(status)
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        self._hosting_status.setText(f"Status: net availability {text} {stamp}".strip())

    def on_network_state(self, state: int, state_message: str, timestamp_ms: int | None = None) -> None:
        describe = getattr(nlc_engine, "describe_network_state", None) if nlc_engine is not None else None
        text = describe(state) if callable(describe) else str(state)
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        message = state_message or ""
        self._events.addItem(QListWidgetItem(f"network={text} {message} {stamp}".strip()))
