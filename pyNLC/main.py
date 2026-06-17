from __future__ import annotations

import importlib
import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from PySide6.QtCore import QObject, QCoreApplication, QStandardPaths, Signal, Slot
from PySide6.QtWidgets import QApplication, QMessageBox, QSettings

from py_wrapper import AppSettingsStub, MediaFeatureStub
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


if nlc_engine is not None:
    BaseGuiToEngine = nlc_engine.IFromGui
else:
    class BaseGuiToEngine:
        pass


class GuiToEngineBridge(BaseGuiToEngine):
    def __init__(self) -> None:
        super().__init__()
        self.signals = GuiSignals()
        self.media_stub = MediaFeatureStub()
        self.asset_dir: Path | None = None
        self.root_data_dir: Path | None = None
        self.user_specific_dir: Path | None = None
        self.user_xfer_dir: Path | None = None

    def fromGuiAppStartup(self, assetsDir: str, rootDataDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.asset_dir = Path(assetsDir)
        self.root_data_dir = Path(rootDataDir)
        self.signals.startup_requested.emit(str(self.asset_dir), str(self.root_data_dir))

    def fromGuiSetUserSpecificDir(self, userSpecificDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.user_specific_dir = Path(userSpecificDir)
        self.signals.user_specific_dir_requested.emit(str(self.user_specific_dir))

    def fromGuiSetUserXferDir(self, userDownloadDir: str, fromThread: bool = False) -> None:
        del fromThread
        self.user_xfer_dir = Path(userDownloadDir)
        self.signals.user_xfer_dir_requested.emit(str(self.user_xfer_dir))

    def fromGuiAppShutdown(self) -> None:
        self.signals.shutdown_requested.emit()

    def fromGuiDeleteUser(self, onlineId) -> bool:
        del onlineId
        return False

    def fromGuiGetDiskFreeSpace(self, dir: str) -> int:
        path = Path(dir) if dir else Path.cwd()
        try:
            return shutil.disk_usage(path).free
        except OSError:
            fallback = path.anchor or Path.cwd()
            return shutil.disk_usage(fallback).free

    def __getattr__(self, name: str):
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
    QCoreApplication.setApplicationDisplayName(APP_TITLE)
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

    bridge = GuiToEngineBridge()
    window = HomeWindow(APP_TITLE, QSettings(APP_DOMAIN, APP_NAME), bridge, app_paths)
    window.apply_theme(settings.getLastSelectedTheme())
    bridge.fromGuiAppStartup(str(app_paths.assets_dir), str(app_paths.root_app_data_dir))
    bridge.fromGuiSetUserSpecificDir(str(app_paths.root_app_data_dir))
    bridge.fromGuiSetUserXferDir(str(app_paths.xfer_dir))
    app.aboutToQuit.connect(bridge.fromGuiAppShutdown)
    app.aboutToQuit.connect(settings.appSettingShutdown)

    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
