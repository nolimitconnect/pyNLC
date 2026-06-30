from __future__ import annotations

import importlib
from datetime import datetime

from PySide6.QtWidgets import QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class _BaseJoinSearchApplet(QWidget):
    _HOST_TYPE = 0
    _TITLE = "Join Search"

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self._sessions: dict[str, dict[str, str]] = {}

        root = QVBoxLayout(self)
        self._status = QLabel("Idle", self)
        root.addWidget(self._status)

        toolbar = QHBoxLayout()
        self._filter = QLineEdit(self)
        self._filter.setPlaceholderText("Filter sessions/results...")
        self._filter.textChanged.connect(self._apply_filter)
        toolbar.addWidget(self._filter)

        self._clear = QPushButton("Clear", self)
        self._clear.clicked.connect(self._clear_rows)
        toolbar.addWidget(self._clear)
        root.addLayout(toolbar)

        self._list = QListWidget(self)
        root.addWidget(self._list)

    def _matches_host_type(self, host_type: int) -> bool:
        return int(host_type) == self._HOST_TYPE

    @staticmethod
    def _id_to_text(value: object) -> str:
        if value is None:
            return ""
        if hasattr(value, "to_online_id"):
            try:
                return str(value.to_online_id())
            except Exception:
                pass
        if hasattr(value, "to_hex"):
            try:
                return str(value.to_hex())
            except Exception:
                pass
        return str(value)

    def _set_session(self, session_id: object, **fields: str) -> None:
        session_key = self._id_to_text(session_id)
        current = self._sessions.get(session_key, {})
        current.update(fields)
        self._sessions[session_key] = current
        self._refresh_rows()

    def _refresh_rows(self) -> None:
        self._list.clear()
        query = self._filter.text().strip().lower()
        for session_id, values in self._sessions.items():
            status = values.get("status", "")
            message = values.get("message", "")
            result = values.get("result", "")
            stamp = values.get("stamp", "")
            text = f"{session_id} | {status} | {result} | {message} {stamp}".strip()
            if query and query not in text.lower():
                continue
            self._list.addItem(QListWidgetItem(text))

    def _apply_filter(self, _text: str) -> None:
        self._refresh_rows()

    def _clear_rows(self) -> None:
        self._sessions.clear()
        self._list.clear()
        self._status.setText("Cleared")

    def add_host_search_status(
        self,
        host_type: int,
        session_id: object,
        search_status: int,
        comm_error: int,
        message: str,
        timestamp_ms: int | None = None,
    ) -> None:
        if not self._matches_host_type(host_type):
            return
        describe_search = getattr(nlc_engine, "describe_host_search_status", None) if nlc_engine is not None else None
        describe_err = getattr(nlc_engine, "describe_comm_error", None) if nlc_engine is not None else None
        status_text = describe_search(search_status) if callable(describe_search) else str(search_status)
        err_text = describe_err(comm_error) if callable(describe_err) else str(comm_error)
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        self._set_session(
            session_id,
            status=status_text.strip(),
            message=f"err={err_text.strip()} {message}".strip(),
            stamp=stamp,
        )
        self._status.setText(f"{self._TITLE}: {status_text.strip()}")

    def add_host_search_result(
        self,
        host_type: int,
        session_id: object,
        hosted_info: object,
        timestamp_ms: int | None = None,
    ) -> None:
        if not self._matches_host_type(host_type):
            return
        title = ""
        if hosted_info is not None and hasattr(hosted_info, "get_host_title"):
            try:
                title = str(hosted_info.get_host_title())
            except Exception:
                title = ""
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        self._set_session(session_id, result=title or "result", stamp=stamp)
        self._status.setText(f"{self._TITLE}: result received")

    def add_host_search_complete(
        self,
        host_type: int,
        session_id: object,
        timestamp_ms: int | None = None,
    ) -> None:
        if not self._matches_host_type(host_type):
            return
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        self._set_session(session_id, status="complete", stamp=stamp)
        self._status.setText(f"{self._TITLE}: complete")


class GroupJoinApplet(_BaseJoinSearchApplet):
    _HOST_TYPE = 4
    _TITLE = "Group Join"


class ChatRoomJoinApplet(_BaseJoinSearchApplet):
    _HOST_TYPE = 5
    _TITLE = "Chat Room Join"


class RandomConnectJoinApplet(_BaseJoinSearchApplet):
    _HOST_TYPE = 6
    _TITLE = "Random Connect Join"
