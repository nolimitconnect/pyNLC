from __future__ import annotations

import importlib
from datetime import datetime

from PySide6.QtWidgets import QCheckBox, QFormLayout, QGroupBox, QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class ShareServicesApplet(QWidget):
    """Concrete non-media applet for Share Services page."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self._enabled_services: dict[str, bool] = {}

        layout = QVBoxLayout(self)

        title = QLabel("Share Services", self)
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        layout.addWidget(title)

        self._state_label = QLabel("Configure shared services", self)
        layout.addWidget(self._state_label)

        service_group = QGroupBox("Service Toggles", self)
        service_form = QFormLayout(service_group)
        self._service_checks: dict[str, QCheckBox] = {}
        for key, label in [
            ("about", "About Me"),
            ("files", "File Share"),
            ("messenger", "Messenger"),
            ("group", "Group Host"),
            ("chat", "Chat Room Host"),
            ("story", "Storyboard"),
        ]:
            check = QCheckBox("Enabled", service_group)
            check.toggled.connect(lambda value, k=key: self._on_service_toggled(k, value))
            service_form.addRow(label, check)
            self._service_checks[key] = check
        layout.addWidget(service_group)

        self._apply_button = QPushButton("Apply Service Settings", self)
        self._apply_button.clicked.connect(self._apply)
        layout.addWidget(self._apply_button)

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
        enabled = store.get("ui.share_services.enabled", {})
        if not isinstance(enabled, dict):
            enabled = {}
        for key, check in self._service_checks.items():
            value = bool(enabled.get(key, False))
            self._enabled_services[key] = value
            check.setChecked(value)

    def _on_service_toggled(self, key: str, enabled: bool) -> None:
        self._enabled_services[key] = bool(enabled)
        self._state_label.setText(f"Service '{key}' {'enabled' if enabled else 'disabled'}")

    def _apply(self) -> None:
        self._store()["ui.share_services.enabled"] = dict(self._enabled_services)
        self._state_label.setText("Share service settings saved")

    def add_plugin_status(self, plugin_type: int, status_type: int, status_value: int, timestamp_ms: int | None = None) -> None:
        describe = getattr(nlc_engine, "describe_plugin_status", None) if nlc_engine is not None else None
        status_text = describe(status_type, status_value) if callable(describe) else f"type={status_type} value={status_value}"
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        self._events.addItem(QListWidgetItem(f"plugin={plugin_type} {status_text} {stamp}".strip()))
