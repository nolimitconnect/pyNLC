from __future__ import annotations

import importlib
import os
import shutil
import sys
from collections import deque
from dataclasses import dataclass
from time import time
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from PySide6.QtCore import QObject, QCoreApplication, QStandardPaths, Signal, Slot, QSettings
from PySide6.QtWidgets import QApplication, QMessageBox


from py_wrapper import AppSettingsStub, IFromGuiContractStub, MediaFeatureStub
from pyNLC.home_window import HomeWindow

try:
    nlc_engine = importlib.import_module("nlc_engine")
except ImportError:  # The bridge is still under active migration.
    nlc_engine = None


APP_NAME = "pyNLC"
APP_TITLE = "pyNLC"
APP_VERSION = "1.0.0"
APP_ORGANIZATION = ""
APP_DOMAIN = "nolimitconnect.org"
TRANSLATION_FILES = [
    "nolimitconnect_ar_SA.qm",
    "nolimitconnect_de_DE.qm",
    "nolimitconnect_es_ES.qm",
    "nolimitconnect_fr_FR.qm",
    "nolimitconnect_hi_IN.qm",
    "nolimitconnect_id_ID.qm",
    "nolimitconnect_ja_JP.qm",
    "nolimitconnect_ko_KR.qm",
    "nolimitconnect_pt_PT.qm",
    "nolimitconnect_ru_RU.qm",
    "nolimitconnect_th_TH.qm",
    "nolimitconnect_zh_CN.qm",
]


@dataclass(frozen=True)
class AppPaths:
    app_data_dir: Path
    root_app_data_dir: Path
    xfer_dir: Path
    assets_dir: Path
    translations_dir: Path


class GuiSignals(QObject):
    startup_requested = Signal(str, str)
    user_specific_dir_requested = Signal(str)
    user_xfer_dir_requested = Signal(str)
    shutdown_requested = Signal()
    status_message = Signal(str)
    hack_reported = Signal(int, int, str, str)
    plugin_message = Signal(int, object, int, str)
    plugin_comm_error = Signal(int, object, int, int)
    plugin_status = Signal(int, int, int)
    file_xfer_state = Signal(int, object, int, int, int, int)
    host_search_status = Signal(int, object, int, int, str)
    groupie_search_status = Signal(int, object, int, int, str)
    host_search_result = Signal(int, object, object)
    host_search_complete = Signal(int, object)
    groupie_search_complete = Signal(int, object)
    net_available_status = Signal(int)
    network_state = Signal(int, str)


if nlc_engine is not None:
    BaseGuiToEngine = nlc_engine.IFromGui
else:
    class BaseGuiToEngine:
        pass


class GuiToEngineBridge(BaseGuiToEngine):
    _EVENT_BUFFER_SIZE = 128

    def __init__(self) -> None:
        super().__init__()
        self.signals = GuiSignals()
        self.interface_stub = IFromGuiContractStub()
        self.media_stub = MediaFeatureStub()
        self.asset_dir: Path | None = None
        self.root_data_dir: Path | None = None
        self.user_specific_dir: Path | None = None
        self.user_xfer_dir: Path | None = None
        self.to_gui_adapter = None
        self._hack_events: deque[tuple[int, int, str, str, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._plugin_events: deque[tuple[int, object, int, str, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._plugin_comm_error_events: deque[tuple[int, object, int, int, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._plugin_status_events: deque[tuple[int, int, int, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._xfer_events: deque[tuple[int, object, int, int, int, int, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._host_search_status_events: deque[tuple[int, object, int, int, str, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._groupie_search_status_events: deque[tuple[int, object, int, int, str, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._host_search_result_events: deque[tuple[int, object, object, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._host_search_complete_events: deque[tuple[int, object, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._groupie_search_complete_events: deque[tuple[int, object, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._net_avail_events: deque[tuple[int, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)
        self._network_state_events: deque[tuple[int, str, int]] = deque(maxlen=self._EVENT_BUFFER_SIZE)

    @staticmethod
    def _now_ms() -> int:
        return int(time() * 1000)

    def replay_events_to_applet(self, applet_widget: object) -> None:
        add_hack_report = getattr(applet_widget, "add_hack_report", None)
        if callable(add_hack_report):
            for event in self._hack_events:
                add_hack_report(*event)

        add_plugin_message = getattr(applet_widget, "add_plugin_message", None)
        if callable(add_plugin_message):
            for event in self._plugin_events:
                add_plugin_message(*event)

        add_plugin_comm_error = getattr(applet_widget, "add_plugin_comm_error", None)
        if callable(add_plugin_comm_error):
            for event in self._plugin_comm_error_events:
                add_plugin_comm_error(*event)

        add_plugin_status = getattr(applet_widget, "add_plugin_status", None)
        if callable(add_plugin_status):
            for event in self._plugin_status_events:
                add_plugin_status(*event)

        add_file_xfer_state = getattr(applet_widget, "add_file_xfer_state", None)
        if callable(add_file_xfer_state):
            for event in self._xfer_events:
                add_file_xfer_state(*event)

        add_host_search_status = getattr(applet_widget, "add_host_search_status", None)
        if callable(add_host_search_status):
            for event in self._host_search_status_events:
                add_host_search_status(*event)

        add_groupie_search_status = getattr(applet_widget, "add_groupie_search_status", None)
        if callable(add_groupie_search_status):
            for event in self._groupie_search_status_events:
                add_groupie_search_status(*event)

        add_host_search_result = getattr(applet_widget, "add_host_search_result", None)
        if callable(add_host_search_result):
            for event in self._host_search_result_events:
                add_host_search_result(*event)

        add_host_search_complete = getattr(applet_widget, "add_host_search_complete", None)
        if callable(add_host_search_complete):
            for event in self._host_search_complete_events:
                add_host_search_complete(*event)

        add_groupie_search_complete = getattr(applet_widget, "add_groupie_search_complete", None)
        if callable(add_groupie_search_complete):
            for event in self._groupie_search_complete_events:
                add_groupie_search_complete(*event)

        on_net_available_status = getattr(applet_widget, "on_net_available_status", None)
        if callable(on_net_available_status):
            for event in self._net_avail_events:
                on_net_available_status(*event)

        on_network_state = getattr(applet_widget, "on_network_state", None)
        if callable(on_network_state):
            for event in self._network_state_events:
                on_network_state(*event)

    def _on_native_to_gui_event(self, method_name: str, *args) -> None:
        if method_name == "toGuiStatusMessage":
            message = str(args[0]) if args else ""
            self.signals.status_message.emit(message)
            return

        if method_name == "toGuiPluginMsg":
            plugin_type = int(args[0]) if len(args) > 0 else 0
            online_id = args[1] if len(args) > 1 else None
            msg_type = int(args[2]) if len(args) > 2 else 0
            param_msg = str(args[3]) if len(args) > 3 else ""
            self._plugin_events.append((plugin_type, online_id, msg_type, param_msg, self._now_ms()))
            self.signals.plugin_message.emit(plugin_type, online_id, msg_type, param_msg)
            return

        if method_name == "toGuiPluginCommError":
            plugin_type = int(args[0]) if len(args) > 0 else 0
            online_id = args[1] if len(args) > 1 else None
            msg_type = int(args[2]) if len(args) > 2 else 0
            comm_error = int(args[3]) if len(args) > 3 else 0
            self._plugin_comm_error_events.append((plugin_type, online_id, msg_type, comm_error, self._now_ms()))
            self.signals.plugin_comm_error.emit(plugin_type, online_id, msg_type, comm_error)
            return

        if method_name == "toGuiPluginStatus":
            plugin_type = int(args[0]) if len(args) > 0 else 0
            status_type = int(args[1]) if len(args) > 1 else 0
            status_value = int(args[2]) if len(args) > 2 else 0
            self._plugin_status_events.append((plugin_type, status_type, status_value, self._now_ms()))
            self.signals.plugin_status.emit(plugin_type, status_type, status_value)
            return

        if method_name == "toGuiFileXferState":
            plugin_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            xfer_direction = int(args[2]) if len(args) > 2 else 0
            xfer_state = int(args[3]) if len(args) > 3 else 0
            xfer_error = int(args[4]) if len(args) > 4 else 0
            param1 = int(args[5]) if len(args) > 5 else 0
            self._xfer_events.append((plugin_type, session_id, xfer_direction, xfer_state, xfer_error, param1, self._now_ms()))
            self.signals.file_xfer_state.emit(
                plugin_type,
                session_id,
                xfer_direction,
                xfer_state,
                xfer_error,
                param1,
            )
            return

        if method_name == "toGuiHostSearchStatus":
            host_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            search_status = int(args[2]) if len(args) > 2 else 0
            comm_error = int(args[3]) if len(args) > 3 else 0
            message = str(args[4]) if len(args) > 4 else ""
            self._host_search_status_events.append(
                (host_type, session_id, search_status, comm_error, message, self._now_ms())
            )
            self.signals.host_search_status.emit(host_type, session_id, search_status, comm_error, message)
            return

        if method_name == "toGuiGroupieSearchStatus":
            host_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            search_status = int(args[2]) if len(args) > 2 else 0
            comm_error = int(args[3]) if len(args) > 3 else 0
            message = str(args[4]) if len(args) > 4 else ""
            self._groupie_search_status_events.append(
                (host_type, session_id, search_status, comm_error, message, self._now_ms())
            )
            self.signals.groupie_search_status.emit(host_type, session_id, search_status, comm_error, message)
            return

        if method_name == "toGuiHostSearchResult":
            host_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            hosted_info = args[2] if len(args) > 2 else None
            self._host_search_result_events.append((host_type, session_id, hosted_info, self._now_ms()))
            self.signals.host_search_result.emit(host_type, session_id, hosted_info)
            return

        if method_name == "toGuiHostSearchComplete":
            host_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            self._host_search_complete_events.append((host_type, session_id, self._now_ms()))
            self.signals.host_search_complete.emit(host_type, session_id)
            return

        if method_name == "toGuiGroupieSearchComplete":
            host_type = int(args[0]) if len(args) > 0 else 0
            session_id = args[1] if len(args) > 1 else None
            self._groupie_search_complete_events.append((host_type, session_id, self._now_ms()))
            self.signals.groupie_search_complete.emit(host_type, session_id)
            return

        if method_name == "toGuiNetAvailableStatus":
            status = int(args[0]) if args else 0
            self._net_avail_events.append((status, self._now_ms()))
            self.signals.net_available_status.emit(status)
            return

        if method_name == "toGuiNetworkState":
            state = int(args[0]) if len(args) > 0 else 0
            state_msg = str(args[1]) if len(args) > 1 else ""
            self._network_state_events.append((state, state_msg, self._now_ms()))
            self.signals.network_state.emit(state, state_msg)

    def install_native_callbacks(self) -> None:
        if nlc_engine is None:
            return

        set_hack_handler = getattr(nlc_engine, "set_hack_report_handler", None)
        if set_hack_handler is None:
            return

        def _on_hack_report(hacker_level, hacker_reason, ip_addr: str, description: str) -> None:
            payload = (int(hacker_level), int(hacker_reason), ip_addr, description, self._now_ms())
            self._hack_events.append(payload)
            self.signals.hack_reported.emit(*payload[:4])

        set_hack_handler(_on_hack_report)

        adapter_cls = getattr(nlc_engine, "IToGuiAdapter", None)
        if adapter_cls is not None:
            self.to_gui_adapter = adapter_cls()
            self.to_gui_adapter.set_default_handler(self._on_native_to_gui_event)

    def uninstall_native_callbacks(self) -> None:
        if nlc_engine is None:
            return

        clear_hack_handler = getattr(nlc_engine, "clear_hack_report_handler", None)
        if clear_hack_handler is not None:
            clear_hack_handler()

        self.to_gui_adapter = None

    def fromGuiAppStartup(self, assetsDir: str, rootDataDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.asset_dir = Path(assetsDir)
        self.root_data_dir = Path(rootDataDir)
        self.interface_stub.fromGuiAppStartup(assetsDir, rootDataDir)
        self.signals.startup_requested.emit(str(self.asset_dir), str(self.root_data_dir))

    def fromGuiSetUserSpecificDir(self, userSpecificDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.user_specific_dir = Path(userSpecificDir)
        self.interface_stub.fromGuiSetUserSpecificDir(userSpecificDir)
        self.signals.user_specific_dir_requested.emit(str(self.user_specific_dir))

    def fromGuiSetUserXferDir(self, userDownloadDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.user_xfer_dir = Path(userDownloadDir)
        self.interface_stub.fromGuiSetUserXferDir(userDownloadDir)
        self.signals.user_xfer_dir_requested.emit(str(self.user_xfer_dir))

    def fromGuiAppShutdown(self) -> None:
        self.interface_stub.fromGuiAppShutdown()
        self.signals.shutdown_requested.emit()

    def fromGuiDeleteUser(self, onlineId) -> bool:
        return bool(self.interface_stub.fromGuiDeleteUser(onlineId))

    def fromGuiGetDiskFreeSpace(self, dir: str) -> int:
        return int(self.interface_stub.fromGuiGetDiskFreeSpace(dir))

    def __getattr__(self, name: str):
        if self.interface_stub.supports(name):
            return getattr(self.interface_stub, name)
        if self.media_stub.supports(name):
            return getattr(self.media_stub, name)
        raise AttributeError(name)


def get_preferred_linux_home_path() -> Path:
    snap_name = os.environ.get("SNAP_NAME", "")
    if snap_name == "code":
        snap_real_home = os.environ.get("SNAP_REAL_HOME", "")
        if snap_real_home:
            return Path(snap_real_home)

        real_home = os.environ.get("REAL_HOME", "")
        if real_home:
            return Path(real_home)

    return Path.home()


def resolve_app_paths() -> AppPaths:
    app_data_location = QStandardPaths.writableLocation(QStandardPaths.AppDataLocation)
    if not app_data_location:
        app_data_location = str(Path.home() / f".{APP_NAME}")

    root_storage_dir = Path(app_data_location)
    if sys.platform.startswith("linux"):
        preferred_home = get_preferred_linux_home_path()
        if preferred_home:
            root_storage_dir = preferred_home / ".local" / "share" / APP_NAME

    root_storage_dir.mkdir(parents=True, exist_ok=True)

    root_app_data_dir = root_storage_dir / "app"
    root_app_data_dir.mkdir(parents=True, exist_ok=True)

    xfer_dir = root_storage_dir / "xfer" / APP_NAME
    xfer_dir.mkdir(parents=True, exist_ok=True)

    assets_dir = root_app_data_dir / "assets"
    assets_dir.mkdir(parents=True, exist_ok=True)

    translations_dir = root_storage_dir / "translations"
    translations_dir.mkdir(parents=True, exist_ok=True)

    return AppPaths(
        app_data_dir=root_storage_dir,
        root_app_data_dir=root_app_data_dir,
        xfer_dir=xfer_dir,
        assets_dir=assets_dir,
        translations_dir=translations_dir,
    )


def copy_bundled_translations_if_required(translations_dir: Path) -> None:
    source_dir = Path(__file__).resolve().parent.parent / "translations"
    if not source_dir.exists():
        return

    for translation_file in TRANSLATION_FILES:
        source_file = source_dir / translation_file
        if not source_file.exists():
            continue

        destination_file = translations_dir / translation_file
        if not destination_file.exists():
            shutil.copy2(source_file, destination_file)


def ensure_adult_confirmation() -> bool:
    settings = QCoreApplication.instance()
    if settings is None:
        return True

    qsettings = QSettings(APP_DOMAIN, APP_NAME)
    if qsettings.contains("isAdult"):
        return bool(qsettings.value("isAdult", False, type=bool))

    warn_adult_title = "You must be an adult to use No Limit Connect application"
    warn_adult_body = (
        "Although No Limit Connect does not host any offensive media, users of No Limit Connect "
        "may host offensive material or act in an offensive manner.\n"
        "No Limit Connect does not monitor or log any user actions or content.\n\n"
        "Are you an adult and at least 18 years old?"
    )
    reply = QMessageBox.question(
        None,
        warn_adult_title,
        warn_adult_body,
        QMessageBox.Yes | QMessageBox.No,
    )

    is_adult = reply == QMessageBox.Yes
    if not is_adult:
        QMessageBox.information(
            None,
            "Access Denied",
            "You must be 18 or older to use this application.",
        )
        return False

    qsettings.setValue("isAdult", is_adult)
    return True


def configure_application_metadata() -> None:
    QCoreApplication.setOrganizationName(APP_ORGANIZATION)
    QCoreApplication.setApplicationName(APP_NAME)
    QCoreApplication.setApplicationVersion(APP_VERSION)
    QApplication.setApplicationDisplayName(APP_TITLE)
    QCoreApplication.setOrganizationDomain(APP_DOMAIN)


def main() -> int:
    app = QApplication(sys.argv)
    configure_application_metadata()

    if not ensure_adult_confirmation():
        return 0

    app_paths = resolve_app_paths()
    copy_bundled_translations_if_required(app_paths.translations_dir)

    settings = AppSettingsStub()
    settings.appSettingStartup(str(app_paths.root_app_data_dir / "AppSettingsDb"))
    short_name = settings.getAppShortName()
    settings.startupAccountDb(str(app_paths.root_app_data_dir / f"{short_name}_accounts.db3"))
    settings.migrateLegacyAccountsFromDirOnce(app_paths.root_app_data_dir / "accounts")
    settings.startupFavoritesDb(str(app_paths.root_app_data_dir / f"{short_name}_favorites.db3"))

    bridge = GuiToEngineBridge()
    bridge.install_native_callbacks()
    window = HomeWindow(APP_TITLE, settings, bridge, app_paths)
    window.apply_theme(settings.getLastSelectedTheme())
    bridge.fromGuiAppStartup(str(app_paths.assets_dir), str(app_paths.root_app_data_dir))
    bridge.fromGuiSetUserSpecificDir(str(app_paths.root_app_data_dir))
    bridge.fromGuiSetUserXferDir(str(app_paths.xfer_dir))
    app.aboutToQuit.connect(bridge.fromGuiAppShutdown)
    app.aboutToQuit.connect(bridge.uninstall_native_callbacks)
    app.aboutToQuit.connect(settings.appSettingShutdown)

    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
