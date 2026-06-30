from __future__ import annotations

import importlib
import json
from datetime import datetime
from time import perf_counter

from PySide6.QtGui import QGuiApplication, QStandardItem, QStandardItemModel
from PySide6.QtWidgets import QMessageBox, QPushButton, QWidget

from resources.Forms.AppletSocketList_ui import Ui_AppletSocketListUi
from py_wrapper import run_vx_net_ident_roundtrip_smoke_test


try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:
    nlc_engine = None


class SocketListApplet(QWidget):
    """Python translation of nolimitgui AppletSocketList (stub data source)."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletSocketListUi()
        self.ui.setupUi(self)

        self.settings = settings
        self.ui.m_CopyToClipboardButton.setVisible(True)
        self.ui.m_CopyToClipboardButton.setText("Run VxNetIdent Smoke Test")
        self.ui.m_CopyToClipboardButton.clicked.connect(self.slot_run_vx_net_ident_smoke_test)
        self._copy_smoke_summary_button = QPushButton("Copy Smoke Summary", self)
        self._copy_smoke_summary_button.clicked.connect(self.slot_copy_smoke_summary_clicked)
        self.ui.horizontalLayout.addWidget(self._copy_smoke_summary_button)
        self._show_smoke_details_button = QPushButton("Show Smoke Details", self)
        self._show_smoke_details_button.clicked.connect(self.slot_show_smoke_details_clicked)
        self.ui.horizontalLayout.addWidget(self._show_smoke_details_button)
        self._latest_smoke_result: dict[str, object] | None = None
        self._smoke_run_count = 0
        self._smoke_total_duration_ms = 0
        self._smoke_min_duration_ms: int | None = None
        self._smoke_max_duration_ms: int | None = None
        self._event_rows: dict[tuple[str, str], int] = {}

        self.model = QStandardItemModel(0, 6, self)
        self.ui.m_TreeView.setRootIsDecorated(False)
        self.ui.m_TreeView.setAlternatingRowColors(False)
        self.ui.m_TreeView.setModel(self.model)
        self.ui.m_TreeView.setSortingEnabled(True)
        self.ui.m_TreeView.doubleClicked.connect(self.slot_tree_item_activated)
        self.ui.m_TreeView.activated.connect(self.slot_tree_item_activated)

        headers = ["Socket", "IP Address", "Txed", "Rxed", "Peer User", "Temp?"]
        for idx, header in enumerate(headers):
            self.model.setHeaderData(idx, 1, header)

        self.refresh_skt_list()

    def slot_copy_to_clipboard_clicked(self) -> None:
        clipboard = QGuiApplication.clipboard()
        if clipboard is None:
            return

        rows = []
        for row in range(self.model.rowCount()):
            rows.append("\t".join(self.model.data(self.model.index(row, col)) or "" for col in range(6)))
        clipboard.setText("\n".join(rows))

    def slot_run_vx_net_ident_smoke_test(self) -> None:
        start = perf_counter()
        result = run_vx_net_ident_roundtrip_smoke_test(strict=True)
        duration_ms = int((perf_counter() - start) * 1000)
        result["duration_ms"] = duration_ms

        self._smoke_run_count += 1
        self._smoke_total_duration_ms += duration_ms
        self._smoke_min_duration_ms = duration_ms if self._smoke_min_duration_ms is None else min(self._smoke_min_duration_ms, duration_ms)
        self._smoke_max_duration_ms = duration_ms if self._smoke_max_duration_ms is None else max(self._smoke_max_duration_ms, duration_ms)
        avg_duration_ms = int(self._smoke_total_duration_ms / self._smoke_run_count)

        result["duration_stats"] = {
            "runs": self._smoke_run_count,
            "last_ms": duration_ms,
            "min_ms": self._smoke_min_duration_ms,
            "max_ms": self._smoke_max_duration_ms,
            "avg_ms": avg_duration_ms,
        }

        self._latest_smoke_result = result
        key = ("vxnetident-smoke", "strict")
        row = self._event_rows.get(key)
        ts_text = datetime.now().strftime("%H:%M:%S")

        if "error" in result:
            status = "error"
            details = str(result.get("error", "unknown error"))[:48]
            mismatch_keys = "-"
        else:
            ok = bool(result.get("ok", False))
            status = "pass" if ok else "fail"
            mismatches = result.get("mismatches", {})
            mismatch_keys = ",".join(sorted(mismatches.keys()))[:48] if mismatches else "-"
            details = str(result.get("mismatch_summary", ""))
            if "\n" in details:
                details = details.splitlines()[0]
            details = details[:48] if details else "No mismatches"

        if row is None:
            row = self._append_row(
                key,
                "vxnetident",
                f"strict {duration_ms}ms avg {avg_duration_ms}ms",
                status,
                mismatch_keys,
                "smoke",
                f"{details} @{ts_text} (double-click for details)",
            )
            self._set_smoke_row_tooltip(row)
            return

        self.model.item(row, 1).setText(f"strict {duration_ms}ms avg {avg_duration_ms}ms")
        self.model.item(row, 2).setText(status)
        self.model.item(row, 3).setText(mismatch_keys)
        self.model.item(row, 5).setText(f"{details} @{ts_text} (double-click for details)")
        self._set_smoke_row_tooltip(row)

    def slot_copy_smoke_summary_clicked(self) -> None:
        clipboard = QGuiApplication.clipboard()
        if clipboard is None:
            return

        if self._latest_smoke_result is None:
            clipboard.setText("No VxNetIdent smoke test has been run yet.")
            return

        summary = str(self._latest_smoke_result.get("mismatch_summary", ""))
        if not summary:
            summary = "No VxNetIdent mismatch summary available."
        clipboard.setText(summary)

    def slot_show_smoke_details_clicked(self) -> None:
        if self._latest_smoke_result is None:
            QMessageBox.information(self, "VxNetIdent Smoke Test", "No smoke test has been run yet.")
            return

        summary = str(self._latest_smoke_result.get("mismatch_summary", "No summary available."))
        duration_stats = self._latest_smoke_result.get("duration_stats", {})
        stats_text = (
            f"Duration runs={duration_stats.get('runs', 0)} "
            f"last={duration_stats.get('last_ms', 0)}ms "
            f"min={duration_stats.get('min_ms', 0)}ms "
            f"max={duration_stats.get('max_ms', 0)}ms "
            f"avg={duration_stats.get('avg_ms', 0)}ms"
        )
        details = json.dumps(self._latest_smoke_result, indent=2, default=str)
        message = f"{summary}\n{stats_text}\n\n{details}"
        QMessageBox.information(self, "VxNetIdent Smoke Test", message)

    def slot_tree_item_activated(self, index) -> None:
        if not index.isValid():
            return
        row = index.row()
        socket_id_item = self.model.item(row, 0)
        peer_user_item = self.model.item(row, 4)
        socket_id = socket_id_item.text() if socket_id_item is not None else ""
        peer_user = peer_user_item.text() if peer_user_item is not None else ""
        if socket_id == "vxnetident" and peer_user == "smoke":
            self.slot_show_smoke_details_clicked()

    def _set_smoke_row_tooltip(self, row: int) -> None:
        tooltip = "Double-click this row to open full VxNetIdent smoke-test details."
        for col in range(self.model.columnCount()):
            item = self.model.item(row, col)
            if item is not None:
                item.setToolTip(tooltip)

    def refresh_skt_list(self) -> None:
        sample_rows = [
            ("101", "192.168.1.44", "1.2 MB", "940 KB", "Alice", "no"),
            ("103", "10.0.0.18", "532 KB", "123 KB", "Bob", "yes"),
        ]
        for row in sample_rows:
            self.add_socket_stat(*row)

    def add_socket_stat(self, socket_id: str, ip_addr: str, txed: str, rxed: str, peer_user: str, is_temp: str) -> None:
        row_items = [
            QStandardItem(socket_id),
            QStandardItem(ip_addr),
            QStandardItem(txed),
            QStandardItem(rxed),
            QStandardItem(peer_user),
            QStandardItem(is_temp),
        ]
        self.model.appendRow(row_items)

    def _append_row(
        self,
        key: tuple[str, str],
        socket_id: str,
        ip_addr: str,
        txed: str,
        rxed: str,
        peer_user: str,
        is_temp: str,
    ) -> int:
        row = self.model.rowCount()
        self._event_rows[key] = row
        self.add_socket_stat(socket_id, ip_addr, txed, rxed, peer_user, is_temp)
        return row

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

    def add_plugin_message(
        self,
        plugin_type: int,
        online_id: object,
        msg_type: int,
        message: str,
        timestamp_ms: int | None = None,
    ) -> None:
        peer_text = self._id_to_text(online_id)
        key = ("plugin", f"{plugin_type}:{peer_text}")
        row = self._event_rows.get(key)
        ts_text = ""
        if timestamp_ms is not None:
            ts_text = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")

        if row is None:
            row = self.model.rowCount()
            self._event_rows[key] = row
            self.add_socket_stat(
                str(plugin_type),
                "-",
                str(msg_type),
                "-",
                peer_text,
                f"{message[:40]}{ts_text}",
            )
            return

        self.model.item(row, 2).setText(str(msg_type))
        self.model.item(row, 5).setText(f"{message[:40]}{ts_text}")

    def add_plugin_comm_error(
        self,
        plugin_type: int,
        online_id: object,
        msg_type: int,
        comm_error: int,
        timestamp_ms: int | None = None,
    ) -> None:
        peer_text = self._id_to_text(online_id)
        key = ("commerr", f"{plugin_type}:{peer_text}")
        row = self._event_rows.get(key)
        describe_err = getattr(nlc_engine, "describe_comm_error", None) if nlc_engine is not None else None
        err_text = describe_err(comm_error) if callable(describe_err) else str(comm_error)
        suffix = ""
        if timestamp_ms is not None:
            suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")

        if row is None:
            self._append_row(
                key,
                "plugin-comm-error",
                str(plugin_type),
                str(msg_type),
                err_text,
                peer_text,
                f"comm={err_text}{suffix}",
            )
            return

        self.model.item(row, 2).setText(str(msg_type))
        self.model.item(row, 3).setText(err_text)
        self.model.item(row, 5).setText(f"comm={err_text}{suffix}")

    def add_plugin_status(
        self,
        plugin_type: int,
        status_type: int,
        status_value: int,
        timestamp_ms: int | None = None,
    ) -> None:
        key = ("plugin-status", str(plugin_type))
        row = self._event_rows.get(key)
        describe_plugin = getattr(nlc_engine, "describe_plugin_type", None) if nlc_engine is not None else None
        plugin_text = describe_plugin(plugin_type) if callable(describe_plugin) else str(plugin_type)
        suffix = ""
        if timestamp_ms is not None:
            suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")

        if row is None:
            self._append_row(
                key,
                plugin_text,
                "-",
                str(status_type),
                str(status_value),
                "plugin",
                f"status{suffix}",
            )
            return

        self.model.item(row, 2).setText(str(status_type))
        self.model.item(row, 3).setText(str(status_value))
        self.model.item(row, 5).setText(f"status{suffix}")

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
        session_text = self._id_to_text(session_id)
        key = ("xfer", f"{plugin_type}:{session_text}")
        row = self._event_rows.get(key)

        describe_dir = getattr(nlc_engine, "describe_xfer_direction", None) if nlc_engine is not None else None
        describe_state = getattr(nlc_engine, "describe_xfer_state", None) if nlc_engine is not None else None
        describe_error = getattr(nlc_engine, "describe_xfer_error", None) if nlc_engine is not None else None

        tx_text = str(param1) if xfer_direction == 2 else "-"
        rx_text = str(param1) if xfer_direction == 1 else "-"
        dir_text = describe_dir(xfer_direction) if callable(describe_dir) else str(xfer_direction)
        state_text = describe_state(xfer_state) if callable(describe_state) else str(xfer_state)
        err_text = describe_error(xfer_error) if callable(describe_error) else str(xfer_error)
        temp_text = f"{dir_text.strip()} {state_text.strip()} err={err_text.strip()}"
        if timestamp_ms is not None:
            temp_text += datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")

        if row is None:
            row = self.model.rowCount()
            self._event_rows[key] = row
            self.add_socket_stat(
                str(plugin_type),
                "-",
                tx_text,
                rx_text,
                session_text,
                temp_text,
            )
            return

        self.model.item(row, 2).setText(tx_text)
        self.model.item(row, 3).setText(rx_text)
        self.model.item(row, 5).setText(temp_text)

    def add_host_search_status(
        self,
        host_type: int,
        session_id: object,
        search_status: int,
        comm_error: int,
        message: str,
        timestamp_ms: int | None = None,
    ) -> None:
        self._add_search_status_row(
            "host-search",
            host_type,
            session_id,
            search_status,
            comm_error,
            message,
            timestamp_ms,
        )

    def add_groupie_search_status(
        self,
        host_type: int,
        session_id: object,
        search_status: int,
        comm_error: int,
        message: str,
        timestamp_ms: int | None = None,
    ) -> None:
        self._add_search_status_row(
            "groupie-search",
            host_type,
            session_id,
            search_status,
            comm_error,
            message,
            timestamp_ms,
        )

    def add_host_search_result(
        self,
        host_type: int,
        session_id: object,
        hosted_info: object,
        timestamp_ms: int | None = None,
    ) -> None:
        session_text = self._id_to_text(session_id)
        key = ("host-search", session_text)
        row = self._event_rows.get(key)
        describe_host_type = getattr(nlc_engine, "describe_host_type", None) if nlc_engine is not None else None
        host_text = describe_host_type(host_type) if callable(describe_host_type) else str(host_type)
        host_url = ""
        if hosted_info is not None and hasattr(hosted_info, "get_host_invite_url"):
            try:
                host_url = str(hosted_info.get_host_invite_url())
            except Exception:
                host_url = ""
        suffix = ""
        if timestamp_ms is not None:
            suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")
        temp_text = f"result {host_url[:28]}{suffix}".strip()

        if row is None:
            self._append_row(
                key,
                "host-search",
                host_text,
                "result",
                "-",
                session_text,
                temp_text,
            )
            return

        self.model.item(row, 2).setText("result")
        self.model.item(row, 5).setText(temp_text)

    def add_host_search_complete(
        self,
        host_type: int,
        session_id: object,
        timestamp_ms: int | None = None,
    ) -> None:
        self._add_search_complete_row("host-search", host_type, session_id, timestamp_ms)

    def add_groupie_search_complete(
        self,
        host_type: int,
        session_id: object,
        timestamp_ms: int | None = None,
    ) -> None:
        self._add_search_complete_row("groupie-search", host_type, session_id, timestamp_ms)

    def _add_search_status_row(
        self,
        prefix: str,
        host_type: int,
        session_id: object,
        search_status: int,
        comm_error: int,
        message: str,
        timestamp_ms: int | None,
    ) -> None:
        session_text = self._id_to_text(session_id)
        key = (prefix, session_text)
        row = self._event_rows.get(key)
        describe_host_type = getattr(nlc_engine, "describe_host_type", None) if nlc_engine is not None else None
        describe_search = getattr(nlc_engine, "describe_host_search_status", None) if nlc_engine is not None else None
        describe_err = getattr(nlc_engine, "describe_comm_error", None) if nlc_engine is not None else None

        host_text = describe_host_type(host_type) if callable(describe_host_type) else str(host_type)
        search_text = describe_search(search_status) if callable(describe_search) else str(search_status)
        err_text = describe_err(comm_error) if callable(describe_err) else str(comm_error)
        suffix = ""
        if timestamp_ms is not None:
            suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")

        temp_text = f"{search_text.strip()} err={err_text.strip()} {message[:24]}{suffix}".strip()

        if row is None:
            self._append_row(
                key,
                prefix,
                host_text,
                search_text,
                err_text,
                session_text,
                temp_text,
            )
            return

        self.model.item(row, 1).setText(host_text)
        self.model.item(row, 2).setText(search_text)
        self.model.item(row, 3).setText(err_text)
        self.model.item(row, 5).setText(temp_text)

    def _add_search_complete_row(
        self,
        prefix: str,
        host_type: int,
        session_id: object,
        timestamp_ms: int | None,
    ) -> None:
        session_text = self._id_to_text(session_id)
        key = (prefix, session_text)
        row = self._event_rows.get(key)
        describe_host_type = getattr(nlc_engine, "describe_host_type", None) if nlc_engine is not None else None
        host_text = describe_host_type(host_type) if callable(describe_host_type) else str(host_type)
        suffix = ""
        if timestamp_ms is not None:
            suffix = datetime.fromtimestamp(timestamp_ms / 1000).strftime(" @%H:%M:%S")
        temp_text = f"complete{suffix}"

        if row is None:
            self._append_row(
                key,
                prefix,
                host_text,
                "complete",
                "-",
                session_text,
                temp_text,
            )
            return

        self.model.item(row, 1).setText(host_text)
        self.model.item(row, 2).setText("complete")
        self.model.item(row, 3).setText("-")
        self.model.item(row, 5).setText(temp_text)
