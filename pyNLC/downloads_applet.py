from __future__ import annotations

import importlib
from datetime import datetime

from PySide6.QtWidgets import QListWidgetItem, QWidget

from resources.Forms.AppletDownloads_ui import Ui_AppletDownloadsUi


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class DownloadsApplet(QWidget):
    """Python translation of nolimitgui AppletDownloads (non-media subset)."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletDownloadsUi()
        self.ui.setupUi(self)
        self.settings = settings
        self._rows: dict[str, int] = {}

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
        # Downloads correspond to Rx direction.
        if int(xfer_direction) != 1:
            return

        describe_state = getattr(nlc_engine, "describe_xfer_state", None) if nlc_engine is not None else None
        describe_error = getattr(nlc_engine, "describe_xfer_error", None) if nlc_engine is not None else None
        state_text = describe_state(xfer_state) if callable(describe_state) else str(xfer_state)
        err_text = describe_error(xfer_error) if callable(describe_error) else str(xfer_error)
        stamp = datetime.fromtimestamp(timestamp_ms / 1000).strftime("@%H:%M:%S") if timestamp_ms else ""
        key = self._id_to_text(session_id)
        text = f"{key} plugin={plugin_type} state={state_text.strip()} err={err_text.strip()} bytes={param1} {stamp}".strip()

        row = self._rows.get(key)
        if row is None:
            self._rows[key] = self.ui.m_FileItemList.count()
            self.ui.m_FileItemList.addItem(QListWidgetItem(text))
            return

        item = self.ui.m_FileItemList.item(row)
        if item is not None:
            item.setText(text)
