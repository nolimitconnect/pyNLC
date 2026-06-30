from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


class TestAndDebugApplet(QWidget):
    """Concrete Test & Debug applet shell with live callback event log."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings

        root = QVBoxLayout(self)
        title = QLabel("Test & Debug", self)
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        root.addWidget(title)

        self._status = QLabel("Waiting for runtime events", self)
        root.addWidget(self._status)

        clear_btn = QPushButton("Clear Events", self)
        clear_btn.clicked.connect(self._clear)
        root.addWidget(clear_btn)

        self._events = QListWidget(self)
        root.addWidget(self._events)

    @staticmethod
    def _stamp(ts_ms: int | None) -> str:
        if not ts_ms:
            return datetime.now().strftime("@%H:%M:%S")
        return datetime.fromtimestamp(ts_ms / 1000).strftime("@%H:%M:%S")

    def _add(self, text: str) -> None:
        self._events.addItem(QListWidgetItem(text))
        self._status.setText(text)

    def _clear(self) -> None:
        self._events.clear()
        self._status.setText("Cleared")

    def add_plugin_message(self, plugin_type: int, online_id: object, msg_type: int, message: str, timestamp_ms: int | None = None) -> None:
        self._add(f"{self._stamp(timestamp_ms)} plugin_msg plugin={plugin_type} type={msg_type} id={online_id} {message}")

    def add_plugin_comm_error(self, plugin_type: int, online_id: object, msg_type: int, comm_error: int, timestamp_ms: int | None = None) -> None:
        self._add(f"{self._stamp(timestamp_ms)} plugin_err plugin={plugin_type} type={msg_type} err={comm_error} id={online_id}")

    def add_plugin_status(self, plugin_type: int, status_type: int, status_value: int, timestamp_ms: int | None = None) -> None:
        self._add(f"{self._stamp(timestamp_ms)} plugin_status plugin={plugin_type} status={status_type}:{status_value}")

    def add_file_xfer_state(
        self,
        plugin_type: int,
        session_id: object,
        xfer_direction: int,
        xfer_state: int,
        xfer_error: int,
        param1: int,
        timestamp_ms: int | None = None,
    ) -> None:
        self._add(
            f"{self._stamp(timestamp_ms)} xfer plugin={plugin_type} session={session_id} dir={xfer_direction} "
            f"state={xfer_state} err={xfer_error} p1={param1}"
        )

    def on_net_available_status(self, status: int, timestamp_ms: int | None = None) -> None:
        self._add(f"{self._stamp(timestamp_ms)} net_available {status}")

    def on_network_state(self, state: int, state_message: str, timestamp_ms: int | None = None) -> None:
        self._add(f"{self._stamp(timestamp_ms)} network_state {state} {state_message}")
