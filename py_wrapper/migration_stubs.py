from __future__ import annotations

import json
import logging
import shutil
import socket
import sqlite3
from datetime import datetime
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class AppSettingsStub:
    # Simple enum-like constants for theme and language
    THEME_DARK = 0
    THEME_LIGHT = 1
    LANG_ENGLISH = 0
    _LEGACY_ACCOUNTS_MIGRATION_KEY = "migration.accounts_dir_to_sqlite.v1"

    def __init__(self) -> None:
        self._initialized = False
        self._settings_path: Path | None = None
        self._settings_conn: sqlite3.Connection | None = None
        self._accounts_conn: sqlite3.Connection | None = None
        self._favorites_conn: sqlite3.Connection | None = None
        self._settings_store: dict[str, Any] = {
            "lastSelectedTheme": self.THEME_DARK,
            "selectedLanguage": self.LANG_ENGLISH,
            "lastAppletLaunched": 0,
        }

    def appSettingStartup(self, dbSettingsFile: str) -> int:
        self._settings_path = self._normalize_db_path(dbSettingsFile)
        self._initialized = True
        self._open_settings_db()
        self._load_settings()
        return 0

    def appSettingShutdown(self) -> None:
        self._save_settings()
        self._close_settings_db()
        self.shutdownFavoritesDb()
        self.shutdownAccountDb()

    def getIsAppSettingInitialized(self) -> bool:
        return self._initialized

    def getAppShortName(self) -> str:
        return "nolimitconnect"

    def startupAccountDb(self, dbPathAndName: str) -> int:
        db_path = self._normalize_db_path(dbPathAndName)
        db_path.parent.mkdir(parents=True, exist_ok=True)
        self._accounts_conn = sqlite3.connect(db_path)
        self._accounts_conn.execute(
            """
            CREATE TABLE IF NOT EXISTS TblUsers(
                username TEXT PRIMARY KEY,
                online_id TEXT NOT NULL DEFAULT '',
                net_ident BLOB,
                modified_utc TEXT NOT NULL DEFAULT (datetime('now'))
            )
            """
        )
        self._accounts_conn.execute(
            """
            CREATE TABLE IF NOT EXISTS TblMySettings(
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """
        )
        self._accounts_conn.commit()
        return 0

    def shutdownAccountDb(self) -> None:
        if self._accounts_conn is not None:
            self._accounts_conn.close()
            self._accounts_conn = None

    def startupFavoritesDb(self, dbPathAndName: str) -> int:
        db_path = self._normalize_db_path(dbPathAndName)
        db_path.parent.mkdir(parents=True, exist_ok=True)
        self._favorites_conn = sqlite3.connect(db_path)
        self._favorites_conn.execute(
            """
            CREATE TABLE IF NOT EXISTS favorites(
                online_id TEXT PRIMARY KEY,
                favorite INTEGER NOT NULL CHECK (favorite IN (0,1))
            )
            """
        )
        self._favorites_conn.commit()
        return 0

    def shutdownFavoritesDb(self) -> None:
        if self._favorites_conn is not None:
            self._favorites_conn.close()
            self._favorites_conn = None

    def updateLastLogin(self, login_name: str) -> bool:
        if not login_name or self._accounts_conn is None:
            return False
        self._accounts_conn.execute(
            "INSERT INTO TblMySettings(key, value) VALUES('LastLogin', ?) "
            "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
            (login_name,),
        )
        self._accounts_conn.commit()
        return True

    def getLastLogin(self) -> str:
        if self._accounts_conn is None:
            return ""
        row = self._accounts_conn.execute(
            "SELECT value FROM TblMySettings WHERE key='LastLogin'"
        ).fetchone()
        return str(row[0]) if row else ""

    def insertAccount(self, account_name: str, online_id: str = "", net_ident_blob: bytes | None = None) -> bool:
        if not account_name or self._accounts_conn is None:
            return False
        self._accounts_conn.execute(
            "INSERT OR REPLACE INTO TblUsers(username, online_id, net_ident, modified_utc) VALUES(?, ?, ?, datetime('now'))",
            (account_name, online_id, net_ident_blob),
        )
        self._accounts_conn.commit()
        return True

    def updateAccount(self, account_name: str, online_id: str = "", net_ident_blob: bytes | None = None) -> bool:
        return self.insertAccount(account_name, online_id, net_ident_blob)

    def removeAccountByName(self, account_name: str) -> bool:
        if not account_name or self._accounts_conn is None:
            return False
        self._accounts_conn.execute("DELETE FROM TblUsers WHERE username=?", (account_name,))
        self._accounts_conn.commit()
        return True

    def getAllAccounts(self) -> list[dict[str, str]]:
        if self._accounts_conn is None:
            return []
        rows = self._accounts_conn.execute(
            "SELECT username, online_id, modified_utc FROM TblUsers ORDER BY username COLLATE NOCASE"
        ).fetchall()
        return [
            {
                "name": str(row[0]),
                "online_id": str(row[1] or ""),
                "last_login": str(row[2] or ""),
            }
            for row in rows
        ]

    def importLegacyAccountsFromDir(self, accounts_dir: str | Path) -> int:
        """Import legacy pyNLC/accounts folder entries into sqlite AccountDb.

        Returns number of imported accounts.
        """
        if self._accounts_conn is None:
            return 0

        source_dir = Path(accounts_dir)
        if not source_dir.exists() or not source_dir.is_dir():
            return 0

        imported = 0
        for account_dir in source_dir.iterdir():
            if not account_dir.is_dir():
                continue
            account_name = account_dir.name.strip()
            if not account_name:
                continue

            row = self._accounts_conn.execute(
                "SELECT 1 FROM TblUsers WHERE username=?",
                (account_name,),
            ).fetchone()
            if row:
                continue

            self._accounts_conn.execute(
                "INSERT INTO TblUsers(username, online_id, net_ident, modified_utc) VALUES(?, ?, NULL, datetime('now'))",
                (account_name, account_name),
            )
            imported += 1

        if imported:
            self._accounts_conn.commit()
        return imported

    def migrateLegacyAccountsFromDirOnce(self, accounts_dir: str | Path) -> int:
        """Run legacy account-folder import at most once per settings DB."""
        if bool(self._settings_store.get(self._LEGACY_ACCOUNTS_MIGRATION_KEY, False)):
            return 0

        imported = self.importLegacyAccountsFromDir(accounts_dir)
        self._settings_store[self._LEGACY_ACCOUNTS_MIGRATION_KEY] = True
        self._save_settings()
        return imported

    def setIsFavorite(self, online_id: str, favorite: bool) -> None:
        if not online_id or self._favorites_conn is None:
            return
        if favorite:
            self._favorites_conn.execute(
                "INSERT OR REPLACE INTO favorites(online_id, favorite) VALUES(?, 1)",
                (online_id,),
            )
        else:
            self._favorites_conn.execute("DELETE FROM favorites WHERE online_id=?", (online_id,))
        self._favorites_conn.commit()

    def getIsFavorite(self, online_id: str) -> bool:
        if not online_id or self._favorites_conn is None:
            return False
        row = self._favorites_conn.execute(
            "SELECT favorite FROM favorites WHERE online_id=?",
            (online_id,),
        ).fetchone()
        return bool(row and int(row[0]) == 1)

    def toggleIsFavorite(self, online_id: str) -> None:
        self.setIsFavorite(online_id, not self.getIsFavorite(online_id))

    @staticmethod
    def _normalize_db_path(path_value: str) -> Path:
        path = Path(path_value)
        if not path.suffix:
            return path.with_suffix(".db3")
        return path

    def _open_settings_db(self) -> None:
        if self._settings_path is None:
            return
        self._settings_path.parent.mkdir(parents=True, exist_ok=True)
        self._settings_conn = sqlite3.connect(self._settings_path)
        self._settings_conn.execute(
            """
            CREATE TABLE IF NOT EXISTS settings(
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """
        )
        self._settings_conn.commit()

    def _close_settings_db(self) -> None:
        if self._settings_conn is not None:
            self._settings_conn.close()
            self._settings_conn = None

    def _load_settings(self) -> None:
        if self._settings_conn is None:
            return
        try:
            rows = self._settings_conn.execute("SELECT key, value FROM settings").fetchall()
            for key, value in rows:
                self._settings_store[str(key)] = json.loads(value)
        except (json.JSONDecodeError, sqlite3.Error):
            pass

    def _save_settings(self) -> None:
        if self._settings_conn is None:
            return
        try:
            items = [(k, json.dumps(v)) for k, v in self._settings_store.items()]
            self._settings_conn.executemany(
                "INSERT INTO settings(key, value) VALUES(?, ?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                items,
            )
            self._settings_conn.commit()
        except sqlite3.Error:
            pass

    def getLastSelectedTheme(self) -> int:
        return self._settings_store.get("lastSelectedTheme", self.THEME_DARK)

    def setLastSelectedTheme(self, theme: int) -> None:
        self._settings_store["lastSelectedTheme"] = theme

    def getSelectedLanguage(self) -> int:
        return self._settings_store.get("selectedLanguage", self.LANG_ENGLISH)

    def setSelectedLanguage(self, lang: int) -> None:
        self._settings_store["selectedLanguage"] = lang

    def getLastAppletLaunched(self, launch_frame: int = 0) -> int:
        return self._settings_store.get("lastAppletLaunched", 0)

    def setLastAppletLaunched(self, launch_frame: int, applet: int) -> None:
        self._settings_store["lastAppletLaunched"] = applet

    def __getattr__(self, name: str) -> Any:
        if name.startswith("set"):
            key = self._method_name_to_key(name)
            return self._make_setter(key)
        if name.startswith("get"):
            key = self._method_name_to_key(name)
            return self._make_getter(name, key)
        raise AttributeError(name)

    @staticmethod
    def _method_name_to_key(name: str) -> str:
        raw = name[3:] if len(name) > 3 else name
        if not raw:
            return name
        return raw[0].lower() + raw[1:]

    def _make_setter(self, key: str) -> Any:
        def setter(*args: Any, **kwargs: Any) -> None:
            del kwargs
            if not args:
                return None
            self._settings_store[key] = args[0]
            self._save_settings()
            return None

        return setter

    def _make_getter(self, name: str, key: str) -> Any:
        default_value = AppSettingsStub._default_value_for(name)

        def getter(*args: Any, **kwargs: Any) -> Any:
            del args, kwargs
            return self._settings_store.get(key, default_value)

        return getter

    @staticmethod
    def _default_value_for(name: str) -> Any:
        lower_name = name.lower()
        if any(token in lower_name for token in ("dir", "file", "path", "url", "text", "name", "id")):
            return ""
        if any(
            token in lower_name
            for token in (
                "is",
                "want",
                "use",
                "mute",
                "disable",
                "allow",
                "show",
                "enable",
                "with",
                "no",
                "verbose",
                "confirm",
            )
        ):
            return False
        return 0


class MediaFeatureStub:
    _FALSE_METHODS = {
        "fromGuiRecordAudio",
        "fromGuiVideoRecord",
        "fromGuiIsNoLimitVideoFile",
        "fromGuiIsNoLimitAudioFile",
        "toGuiMediaAction",
        "toGuiGetIsAppModuleRunning",
        "toGuiRunModule",
        "toGuiStopModule",
    }
    _ZERO_METHODS = {
        "toGuiModuleAudioFrame",
        "toGuiPlayerNlcAudio",
    }
    _FLOAT_METHODS = {
        "toGuiGetAudioDelaySeconds",
        "toGuiGetAudioCacheFreeSpaceBytes",
        "toGuiGetAudioCacheMaxSeconds",
    }
    _MEDIA_TOKENS = ("audio", "video", "media", "camera", "capture", "wave", "aec", "echo")

    def supports(self, name: str) -> bool:
        if name in self._FALSE_METHODS or name in self._ZERO_METHODS or name in self._FLOAT_METHODS:
            return True
        lower_name = name.lower()
        return any(token in lower_name for token in self._MEDIA_TOKENS)

    def __getattr__(self, name: str) -> Any:
        if name in self._FALSE_METHODS:
            return self._return_false
        if name in self._ZERO_METHODS:
            return self._return_zero
        if name in self._FLOAT_METHODS:
            return self._return_float_zero
        if self.supports(name):
            return self._noop
        raise AttributeError(name)

    @staticmethod
    def _noop(*args: Any, **kwargs: Any) -> None:
        del args, kwargs
        return None

    @staticmethod
    def _return_false(*args: Any, **kwargs: Any) -> bool:
        del args, kwargs
        return False

    @staticmethod
    def _return_zero(*args: Any, **kwargs: Any) -> int:
        del args, kwargs
        return 0

    @staticmethod
    def _return_float_zero(*args: Any, **kwargs: Any) -> float:
        del args, kwargs
        return 0.0


@dataclass(frozen=True)
class GuiInterfaceMethodSpec:
    method_name: str
    return_default: Any
    equivalent: str
    notes: str


class GuiInterfaceContractBase:
    """Base contract used to mirror libs/GuiInterface methods in Python.

    These stubs are intentionally non-throwing by default so migration can proceed
    even when native bindings are incomplete.
    """

    _METHODS: dict[str, GuiInterfaceMethodSpec] = {}

    def __init__(self) -> None:
        self.call_log: list[tuple[str, tuple[Any, ...], dict[str, Any]]] = []
        self.logger = logging.getLogger(self.__class__.__name__)

    @classmethod
    def supports(cls, method_name: str) -> bool:
        return method_name in cls._METHODS

    @classmethod
    def implementation_notes_for(cls, method_name: str) -> str:
        spec = cls._METHODS.get(method_name)
        if spec is None:
            return "No contract entry."
        return f"Equivalent: {spec.equivalent}. Notes: {spec.notes}"

    def _handle_stub_call(self, method_name: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> Any:
        self.call_log.append((method_name, args, kwargs))
        spec = self._METHODS.get(method_name)
        if spec is None:
            raise AttributeError(method_name)

        self.logger.debug("Stubbed GuiInterface call %s: %s", method_name, spec.notes)
        return spec.return_default


class IFromGuiContractStub(GuiInterfaceContractBase):
    """Contract mirror of libs/GuiInterface/IFromGui.h."""

    _METHODS = {
        "fromGuiAppStartup": GuiInterfaceMethodSpec("fromGuiAppStartup", None, "IFromGui::fromGuiAppStartup", "Wire to engine startup path initialization."),
        "fromGuiSetUserSpecificDir": GuiInterfaceMethodSpec("fromGuiSetUserSpecificDir", None, "IFromGui::fromGuiSetUserSpecificDir", "Persist user-specific data root path."),
        "fromGuiSetUserXferDir": GuiInterfaceMethodSpec("fromGuiSetUserXferDir", None, "IFromGui::fromGuiSetUserXferDir", "Persist transfer/download path."),
        "fromGuiUserLoggedOn": GuiInterfaceMethodSpec("fromGuiUserLoggedOn", None, "IFromGui::fromGuiUserLoggedOn", "Bound in py_wrapper via from_gui_user_logged_on."),
        "fromGuiAppShutdown": GuiInterfaceMethodSpec("fromGuiAppShutdown", None, "IFromGui::fromGuiAppShutdown", "Flush pending engine state and shutdown."),
        "fromGuiDeleteUser": GuiInterfaceMethodSpec("fromGuiDeleteUser", False, "IFromGui::fromGuiDeleteUser", "Needs identity deletion in account DB."),
        "fromGuiGetDiskFreeSpace": GuiInterfaceMethodSpec("fromGuiGetDiskFreeSpace", 0, "IFromGui::fromGuiGetDiskFreeSpace", "Native equivalent exists; fallback returns 0."),
        "fromGuiClearCache": GuiInterfaceMethodSpec("fromGuiClearCache", 0, "IFromGui::fromGuiClearCache", "Needs cache manager bridge and ECacheType enum binding."),
        "fromGuiOrientationEvent": GuiInterfaceMethodSpec("fromGuiOrientationEvent", False, "IFromGui::fromGuiOrientationEvent", "Route mobile sensor orientation to native render/event system."),
        "fromGuiMouseEvent": GuiInterfaceMethodSpec("fromGuiMouseEvent", False, "IFromGui::fromGuiMouseEvent", "Requires EMouseButtonType and EMouseEventType enum bindings."),
        "fromGuiMouseWheel": GuiInterfaceMethodSpec("fromGuiMouseWheel", False, "IFromGui::fromGuiMouseWheel", "Route wheel input event to app modules."),
        "fromGuiKeyEvent": GuiInterfaceMethodSpec("fromGuiKeyEvent", False, "IFromGui::fromGuiKeyEvent", "Requires EKeyEventType and EKeyCode bindings."),
        "fromGuiNativeGlInit": GuiInterfaceMethodSpec("fromGuiNativeGlInit", None, "IFromGui::fromGuiNativeGlInit", "Optional for native GL modules; currently can stay stubbed."),
        "fromGuiNativeGlResize": GuiInterfaceMethodSpec("fromGuiNativeGlResize", None, "IFromGui::fromGuiNativeGlResize", "Optional for native GL modules; currently can stay stubbed."),
        "fromGuiNativeGlRender": GuiInterfaceMethodSpec("fromGuiNativeGlRender", 0, "IFromGui::fromGuiNativeGlRender", "Optional for native GL modules; return frame status code."),
        "fromGuiNativeGlPauseRender": GuiInterfaceMethodSpec("fromGuiNativeGlPauseRender", None, "IFromGui::fromGuiNativeGlPauseRender", "Optional for native GL modules."),
        "fromGuiNativeGlResumeRender": GuiInterfaceMethodSpec("fromGuiNativeGlResumeRender", None, "IFromGui::fromGuiNativeGlResumeRender", "Optional for native GL modules."),
        "fromGuiNativeGlDestroy": GuiInterfaceMethodSpec("fromGuiNativeGlDestroy", None, "IFromGui::fromGuiNativeGlDestroy", "Optional for native GL modules."),
        "fromGuiSndRecord": GuiInterfaceMethodSpec("fromGuiSndRecord", False, "IFromGui::fromGuiSndRecord", "Media/audio pipeline not fully bound yet."),
        "fromGuiVideoRecord": GuiInterfaceMethodSpec("fromGuiVideoRecord", False, "IFromGui::fromGuiVideoRecord", "Media/video pipeline not fully bound yet."),
        "fromGuiPlayLocalMedia": GuiInterfaceMethodSpec("fromGuiPlayLocalMedia", False, "IFromGui::fromGuiPlayLocalMedia", "Requires overload handling and media module bridge."),
        "fromGuiWantMediaInput": GuiInterfaceMethodSpec("fromGuiWantMediaInput", None, "IFromGui::fromGuiWantMediaInput", "Requires media callback/session bindings."),
        "fromGuiOnlineNameChanged": GuiInterfaceMethodSpec("fromGuiOnlineNameChanged", None, "IFromGui::fromGuiOnlineNameChanged", "Bound in py_wrapper via from_gui_online_name_changed."),
        "fromGuiMoodMessageChanged": GuiInterfaceMethodSpec("fromGuiMoodMessageChanged", None, "IFromGui::fromGuiMoodMessageChanged", "Bound in py_wrapper via from_gui_mood_message_changed."),
        "fromGuiIdentPersonalInfoChanged": GuiInterfaceMethodSpec("fromGuiIdentPersonalInfoChanged", None, "IFromGui::fromGuiIdentPersonalInfoChanged", "Requires personal-info field mapping to net ident."),
        "fromGuiSetUserHasProfilePicture": GuiInterfaceMethodSpec("fromGuiSetUserHasProfilePicture", None, "IFromGui::fromGuiSetUserHasProfilePicture", "Bound in py_wrapper via from_gui_set_user_has_profile_picture."),
        "fromGuiUpdateMyIdent": GuiInterfaceMethodSpec("fromGuiUpdateMyIdent", None, "IFromGui::fromGuiUpdateMyIdent", "Bound in py_wrapper via from_gui_update_my_ident."),
        "fromGuiQueryMyIdent": GuiInterfaceMethodSpec("fromGuiQueryMyIdent", None, "IFromGui::fromGuiQueryMyIdent", "Bound in py_wrapper via from_gui_query_my_ident/query_my_ident."),
        "fromGuiSetIdentHasTextOffers": GuiInterfaceMethodSpec("fromGuiSetIdentHasTextOffers", None, "IFromGui::fromGuiSetIdentHasTextOffers", "Bound in py_wrapper via from_gui_set_ident_has_text_offers."),
        "fromGuiChangeMyFriendshipToHim": GuiInterfaceMethodSpec("fromGuiChangeMyFriendshipToHim", False, "IFromGui::fromGuiChangeMyFriendshipToHim", "Bound in py_wrapper via from_gui_change_my_friendship_to_him."),
        "fromGuiApplyNetHostSettings": GuiInterfaceMethodSpec("fromGuiApplyNetHostSettings", None, "IFromGui::fromGuiApplyNetHostSettings", "Needs NetHostSetting binding and engine forwarding."),
        "fromGuiSetNetSettings": GuiInterfaceMethodSpec("fromGuiSetNetSettings", None, "IFromGui::fromGuiSetNetSettings", "Needs NetSettings struct binding."),
        "fromGuiGetNetSettings": GuiInterfaceMethodSpec("fromGuiGetNetSettings", None, "IFromGui::fromGuiGetNetSettings", "Needs NetSettings struct binding."),
        "fromGuiSetRelaySettings": GuiInterfaceMethodSpec("fromGuiSetRelaySettings", None, "IFromGui::fromGuiSetRelaySettings", "Bound in py_wrapper via from_gui_set_relay_settings."),
        "fromGuiRunIsPortOpenTest": GuiInterfaceMethodSpec("fromGuiRunIsPortOpenTest", None, "IFromGui::fromGuiRunIsPortOpenTest", "Bound in py_wrapper via from_gui_run_is_port_open_test; callback wiring remains native-dependent."),
        "fromGuiRunUrlAction": GuiInterfaceMethodSpec("fromGuiRunUrlAction", None, "IFromGui::fromGuiRunUrlAction", "Requires ENetCmdType binding and async callback path."),
        "fromGuiAnnounceHost": GuiInterfaceMethodSpec("fromGuiAnnounceHost", None, "IFromGui::fromGuiAnnounceHost", "Requires HostedId/session/url binding."),
        "fromGuiJoinHost": GuiInterfaceMethodSpec("fromGuiJoinHost", None, "IFromGui::fromGuiJoinHost", "Requires HostedId/session/url binding."),
        "fromGuiLeaveHost": GuiInterfaceMethodSpec("fromGuiLeaveHost", None, "IFromGui::fromGuiLeaveHost", "Requires HostedId binding."),
        "fromGuiUnJoinHost": GuiInterfaceMethodSpec("fromGuiUnJoinHost", None, "IFromGui::fromGuiUnJoinHost", "Requires HostedId binding."),
        "fromGuiSearchHost": GuiInterfaceMethodSpec("fromGuiSearchHost", None, "IFromGui::fromGuiSearchHost", "Requires SearchParams and EHostType bindings."),
        "fromGuiBlockUser": GuiInterfaceMethodSpec("fromGuiBlockUser", None, "IFromGui::fromGuiBlockUser", "User moderation path."),
        "fromGuiSendAnnouncedList": GuiInterfaceMethodSpec("fromGuiSendAnnouncedList", None, "IFromGui::fromGuiSendAnnouncedList", "Push current announced host list to GUI."),
        "fromGuiDisconnectFromUser": GuiInterfaceMethodSpec("fromGuiDisconnectFromUser", None, "IFromGui::fromGuiDisconnectFromUser", "Disconnect peer session/socket."),
        "fromGuiSetFileShareSettings": GuiInterfaceMethodSpec("fromGuiSetFileShareSettings", None, "IFromGui::fromGuiSetFileShareSettings", "Needs FileShareSettings binding."),
        "fromGuiGetFileShareSettings": GuiInterfaceMethodSpec("fromGuiGetFileShareSettings", None, "IFromGui::fromGuiGetFileShareSettings", "Needs FileShareSettings binding."),
        "fromGuiUpdateWebPageProfile": GuiInterfaceMethodSpec("fromGuiUpdateWebPageProfile", None, "IFromGui::fromGuiUpdateWebPageProfile", "Update profile HTML resources and metadata."),
        "fromGuiSetPluginPermission": GuiInterfaceMethodSpec("fromGuiSetPluginPermission", None, "IFromGui::fromGuiSetPluginPermission", "Bound in py_wrapper via from_gui_set_plugin_permission."),
        "fromGuiGetPluginPermission": GuiInterfaceMethodSpec("fromGuiGetPluginPermission", 0, "IFromGui::fromGuiGetPluginPermission", "Bound in py_wrapper via from_gui_get_plugin_permission."),
        "fromGuiGetPluginServerState": GuiInterfaceMethodSpec("fromGuiGetPluginServerState", 0, "IFromGui::fromGuiGetPluginServerState", "Bound in py_wrapper via from_gui_get_plugin_server_state."),
        "fromGuiStartPluginSession": GuiInterfaceMethodSpec("fromGuiStartPluginSession", False, "IFromGui::fromGuiStartPluginSession", "Bound in py_wrapper via from_gui_start_plugin_session."),
        "fromGuiStopPluginSession": GuiInterfaceMethodSpec("fromGuiStopPluginSession", None, "IFromGui::fromGuiStopPluginSession", "Bound in py_wrapper via from_gui_stop_plugin_session."),
        "fromGuiIsPluginInSession": GuiInterfaceMethodSpec("fromGuiIsPluginInSession", False, "IFromGui::fromGuiIsPluginInSession", "Bound in py_wrapper via from_gui_is_plugin_in_session/from_gui_is_plugin_in_session_simple."),
        "fromGuiMakePluginOffer": GuiInterfaceMethodSpec("fromGuiMakePluginOffer", False, "IFromGui::fromGuiMakePluginOffer", "Needs OfferBaseInfo binding."),
        "fromGuiToPluginOfferReply": GuiInterfaceMethodSpec("fromGuiToPluginOfferReply", False, "IFromGui::fromGuiToPluginOfferReply", "Needs OfferBaseInfo binding."),
        "fromGuiFileXferControl": GuiInterfaceMethodSpec("fromGuiFileXferControl", 0, "IFromGui::fromGuiFileXferControl", "Needs EXferAction/FileInfo bindings."),
        "fromGuiInstMsg": GuiInterfaceMethodSpec("fromGuiInstMsg", False, "IFromGui::fromGuiInstMsg", "Instant-message dispatch path."),
        "fromGuiPushToTalk": GuiInterfaceMethodSpec("fromGuiPushToTalk", False, "IFromGui::fromGuiPushToTalk", "Bound in py_wrapper via from_gui_push_to_talk."),
        "fromGuiAdminViewHost": GuiInterfaceMethodSpec("fromGuiAdminViewHost", None, "IFromGui::fromGuiAdminViewHost", "Marks admin viewport active/inactive."),
        "fromGuiSendContactList": GuiInterfaceMethodSpec("fromGuiSendContactList", None, "IFromGui::fromGuiSendContactList", "Push contact subset to GUI."),
        "fromGuiRefreshContactList": GuiInterfaceMethodSpec("fromGuiRefreshContactList", None, "IFromGui::fromGuiRefreshContactList", "Force refresh from anchor/network."),
        "fromGuiStartScan": GuiInterfaceMethodSpec("fromGuiStartScan", None, "IFromGui::fromGuiStartScan", "Network scan control path."),
        "fromGuiNextScan": GuiInterfaceMethodSpec("fromGuiNextScan", None, "IFromGui::fromGuiNextScan", "Advance scan result pagination."),
        "fromGuiStopScan": GuiInterfaceMethodSpec("fromGuiStopScan", None, "IFromGui::fromGuiStopScan", "Stop network scanning."),
        "fromGuiGetMyIpAddress": GuiInterfaceMethodSpec("fromGuiGetMyIpAddress", "", "IFromGui::fromGuiGetMyIpAddress", "InetAddress object is not yet bound; returns empty string placeholder."),
        "fromGuiGetMyIPv4Address": GuiInterfaceMethodSpec("fromGuiGetMyIPv4Address", "", "IFromGui::fromGuiGetMyIPv4Address", "InetAddress object is not yet bound; returns empty string placeholder."),
        "fromGuiGetMyIPv6Address": GuiInterfaceMethodSpec("fromGuiGetMyIPv6Address", "", "IFromGui::fromGuiGetMyIPv6Address", "InetAddress object is not yet bound; returns empty string placeholder."),
        "fromGuiCancelDownload": GuiInterfaceMethodSpec("fromGuiCancelDownload", None, "IFromGui::fromGuiCancelDownload", "Cancel transfer by file instance id."),
        "fromGuiCancelUpload": GuiInterfaceMethodSpec("fromGuiCancelUpload", None, "IFromGui::fromGuiCancelUpload", "Cancel transfer by file instance id."),
        "fromGuiTodGameActionSend": GuiInterfaceMethodSpec("fromGuiTodGameActionSend", False, "IFromGui::fromGuiTodGameActionSend", "Game action dispatch path."),
        "fromGuiBrowseFiles": GuiInterfaceMethodSpec("fromGuiBrowseFiles", False, "IFromGui::fromGuiBrowseFiles", "Needs file browser and filter mask support."),
        "fromGuiGetFileDownloadState": GuiInterfaceMethodSpec("fromGuiGetFileDownloadState", -1, "IFromGui::fromGuiGetFileDownloadState", "Return -1 when unknown as documented in header."),
        "fromGuiSetFileIsShared": GuiInterfaceMethodSpec("fromGuiSetFileIsShared", False, "IFromGui::fromGuiSetFileIsShared", "Needs FileInfo binding."),
        "fromGuiGetIsFileShared": GuiInterfaceMethodSpec("fromGuiGetIsFileShared", False, "IFromGui::fromGuiGetIsFileShared", "Needs FileInfo binding."),
        "fromGuiSetFileIsInLibrary": GuiInterfaceMethodSpec("fromGuiSetFileIsInLibrary", False, "IFromGui::fromGuiSetFileIsInLibrary", "Needs FileInfo binding."),
        "fromGuiGetFileIsInLibrary": GuiInterfaceMethodSpec("fromGuiGetFileIsInLibrary", False, "IFromGui::fromGuiGetFileIsInLibrary", "Needs FileInfo binding."),
        "fromGuiGetFileLibraryList": GuiInterfaceMethodSpec("fromGuiGetFileLibraryList", None, "IFromGui::fromGuiGetFileLibraryList", "Push file library listing through IToGui callbacks."),
        "fromGuiScanFolderForMedia": GuiInterfaceMethodSpec("fromGuiScanFolderForMedia", None, "IFromGui::fromGuiScanFolderForMedia", "Media scan path with async callbacks."),
        "fromGuiScanItemReceived": GuiInterfaceMethodSpec("fromGuiScanItemReceived", None, "IFromGui::fromGuiScanItemReceived", "Backpressure acknowledgment path."),
        "fromGuiScanFolderCancel": GuiInterfaceMethodSpec("fromGuiScanFolderCancel", None, "IFromGui::fromGuiScanFolderCancel", "Cancel scan operation by app instance id."),
        "fromGuiIsNoLimitVideoFile": GuiInterfaceMethodSpec("fromGuiIsNoLimitVideoFile", False, "IFromGui::fromGuiIsNoLimitVideoFile", "Media format detection not yet bound."),
        "fromGuiIsNoLimitAudioFile": GuiInterfaceMethodSpec("fromGuiIsNoLimitAudioFile", False, "IFromGui::fromGuiIsNoLimitAudioFile", "Media format detection not yet bound."),
        "fromGuiDeleteFile": GuiInterfaceMethodSpec("fromGuiDeleteFile", -1, "IFromGui::fromGuiDeleteFile", "Return code contract not yet defined in Python bridge."),
        "fromGuiQuerySessionHistory": GuiInterfaceMethodSpec("fromGuiQuerySessionHistory", None, "IFromGui::fromGuiQuerySessionHistory", "Needs GroupieId binding and history callback path."),
        "fromGuiAssetAction": GuiInterfaceMethodSpec("fromGuiAssetAction", False, "IFromGui::fromGuiAssetAction", "Asset control path has overloaded signatures not yet disambiguated."),
        "fromGuiQueueAssetAction": GuiInterfaceMethodSpec("fromGuiQueueAssetAction", False, "IFromGui::fromGuiQueueAssetAction", "Asset queueing path pending AssetBaseInfo binding."),
        "fromGuiSendAsset": GuiInterfaceMethodSpec("fromGuiSendAsset", False, "IFromGui::fromGuiSendAsset", "Asset send path pending AssetBaseInfo binding."),
        "fromGuiMultiSessionAction": GuiInterfaceMethodSpec("fromGuiMultiSessionAction", False, "IFromGui::fromGuiMultiSessionAction", "Session action path pending EMSessionAction binding."),
        "fromGuiGetRandomTcpPort": GuiInterfaceMethodSpec("fromGuiGetRandomTcpPort", 0, "IFromGui::fromGuiGetRandomTcpPort", "Use engine random port helper when bound."),
        "fromGuiGetNodeUrl": GuiInterfaceMethodSpec("fromGuiGetNodeUrl", None, "IFromGui::fromGuiGetNodeUrl", "Bound in py_wrapper via from_gui_get_node_url."),
        "fromGuiGetInternetStatus": GuiInterfaceMethodSpec("fromGuiGetInternetStatus", 0, "IFromGui::fromGuiGetInternetStatus", "Returns enum integer placeholder."),
        "fromGuiGetNetAvailStatus": GuiInterfaceMethodSpec("fromGuiGetNetAvailStatus", 0, "IFromGui::fromGuiGetNetAvailStatus", "Returns enum integer placeholder."),
        "fromGuiTestCmd": GuiInterfaceMethodSpec("fromGuiTestCmd", False, "IFromGui::fromGuiTestCmd", "Test/debug command path."),
        "fromGuiGetJoinedListCount": GuiInterfaceMethodSpec("fromGuiGetJoinedListCount", 0, "IFromGui::fromGuiGetJoinedListCount", "Bound in py_wrapper via from_gui_get_joined_list_count."),
        "fromGuiGetAnnouncedHostCount": GuiInterfaceMethodSpec("fromGuiGetAnnouncedHostCount", 0, "IFromGui::fromGuiGetAnnouncedHostCount", "Bound in py_wrapper via from_gui_get_announced_host_count."),
        "fromGuiListAction": GuiInterfaceMethodSpec("fromGuiListAction", None, "IFromGui::fromGuiListAction", "List action dispatch pending enum binding."),
        "fromGuiQueryDefaultUrl": GuiInterfaceMethodSpec("fromGuiQueryDefaultUrl", "", "IFromGui::fromGuiQueryDefaultUrl", "Bound in py_wrapper via from_gui_query_default_url."),
        "fromGuiSetDefaultUrl": GuiInterfaceMethodSpec("fromGuiSetDefaultUrl", False, "IFromGui::fromGuiSetDefaultUrl", "Bound in py_wrapper via from_gui_set_default_url."),
        "fromGuiQueryIdentity": GuiInterfaceMethodSpec("fromGuiQueryIdentity", False, "IFromGui::fromGuiQueryIdentity", "Bound in py_wrapper via from_gui_query_identity_by_url/from_gui_query_identity_by_online_id."),
        "fromGuiQueryHosts": GuiInterfaceMethodSpec("fromGuiQueryHosts", False, "IFromGui::fromGuiQueryHosts", "Requires HostedInfo vector marshalling."),
        "fromGuiQueryMyHostedInfo": GuiInterfaceMethodSpec("fromGuiQueryMyHostedInfo", False, "IFromGui::fromGuiQueryMyHostedInfo", "Requires HostedInfo vector marshalling."),
        "fromGuiQueryHostListFromNetworkHost": GuiInterfaceMethodSpec("fromGuiQueryHostListFromNetworkHost", False, "IFromGui::fromGuiQueryHostListFromNetworkHost", "Requires VxPtopUrl and HostedInfo bindings."),
        "fromGuiQueryGroupiesFromHosted": GuiInterfaceMethodSpec("fromGuiQueryGroupiesFromHosted", False, "IFromGui::fromGuiQueryGroupiesFromHosted", "Requires VxPtopUrl and GroupieInfo bindings."),
        "fromGuiDownloadWebPage": GuiInterfaceMethodSpec("fromGuiDownloadWebPage", False, "IFromGui::fromGuiDownloadWebPage", "Web-page fetch path pending EWebPageType binding."),
        "fromGuiCancelWebPage": GuiInterfaceMethodSpec("fromGuiCancelWebPage", False, "IFromGui::fromGuiCancelWebPage", "Web-page cancel path pending EWebPageType binding."),
        "fromGuiDownloadFileList": GuiInterfaceMethodSpec("fromGuiDownloadFileList", False, "IFromGui::fromGuiDownloadFileList", "File-list fetch path pending transfer/session bindings."),
        "fromGuiDownloadFileListCancel": GuiInterfaceMethodSpec("fromGuiDownloadFileListCancel", False, "IFromGui::fromGuiDownloadFileListCancel", "File-list cancel path pending transfer/session bindings."),
        "fromGuiQueryJoinState": GuiInterfaceMethodSpec("fromGuiQueryJoinState", 0, "IFromGui::fromGuiQueryJoinState", "Bound in py_wrapper via from_gui_query_join_state."),
        "fromGuiUpdatePluginPermission": GuiInterfaceMethodSpec("fromGuiUpdatePluginPermission", None, "IFromGui::fromGuiUpdatePluginPermission", "Bound in py_wrapper via from_gui_update_plugin_permission."),
        "fromGuiQueryFileHash": GuiInterfaceMethodSpec("fromGuiQueryFileHash", False, "IFromGui::fromGuiQueryFileHash", "Needs FileInfo hash output marshalling."),
        "fromGuiDeleteDatabase": GuiInterfaceMethodSpec("fromGuiDeleteDatabase", False, "IFromGui::fromGuiDeleteDatabase", "Database management path pending EDatabaseType binding."),
        "fromGuiSetIsAutomatedHost": GuiInterfaceMethodSpec("fromGuiSetIsAutomatedHost", None, "IFromGui::fromGuiSetIsAutomatedHost", "Set automated host state in engine/user identity."),
        "fromGuiSendRandConnectSelected": GuiInterfaceMethodSpec("fromGuiSendRandConnectSelected", False, "IFromGui::fromGuiSendRandConnectSelected", "Random-connect selection signaling path."),
        "fromGuiQueryFriendRequest": GuiInterfaceMethodSpec("fromGuiQueryFriendRequest", False, "IFromGui::fromGuiQueryFriendRequest", "Requires FriendRequestInfo vector bindings."),
        "fromGuiSendFriendRequest": GuiInterfaceMethodSpec("fromGuiSendFriendRequest", False, "IFromGui::fromGuiSendFriendRequest", "Friend-request submission path."),
    }

    def __init__(self) -> None:
        super().__init__()
        self._startup_assets_dir = ""
        self._startup_root_dir = ""
        self._user_specific_dir = ""
        self._user_xfer_dir = ""
        self._default_url = ""
        self._shared_files: set[str] = set()
        self._library_files: set[str] = set()
        self._download_states: dict[str, int] = {}
        self._users: set[str] = set()
        self._announced_hosts: dict[str, dict[str, Any]] = {}
        self._joined_hosts: set[str] = set()
        self._blocked_users: set[str] = set()
        self._plugin_permissions: dict[str, int] = {}
        self._plugin_sessions: set[str] = set()
        self._relay_settings: Any = None
        self._scan_active = False
        self._last_search: dict[str, Any] = {}
        self._last_port_open_test: dict[str, Any] = {}
        self._default_urls: dict[str, str] = {}
        self._automated_host = False
        self._push_to_talk = False
        self._admin_view_host = False
        self._net_settings: dict[str, Any] = {}
        self._net_host_settings: dict[str, Any] = {}
        self._file_share_settings: dict[str, Any] = {}
        self._identity: dict[str, Any] = {}
        self._contact_list: list[Any] = []
        self._friendships: dict[str, int] = {}
        self._text_offer_flags: dict[str, bool] = {}
        self._plugin_offers: list[dict[str, Any]] = []
        self._plugin_offer_replies: list[dict[str, Any]] = []
        self._instant_messages: list[dict[str, Any]] = []
        self._session_history: dict[str, list[Any]] = {}
        self._asset_actions: list[dict[str, Any]] = []
        self._queued_asset_actions: list[dict[str, Any]] = []
        self._sent_assets: list[dict[str, Any]] = []
        self._multi_session_actions: list[dict[str, Any]] = []
        self._web_page_downloads: dict[str, dict[str, Any]] = {}
        self._file_list_downloads: dict[str, dict[str, Any]] = {}
        self._last_list_action: dict[str, Any] = {}
        self._last_url_action: dict[str, Any] = {}
        self._folder_scan_results: dict[str, list[str]] = {}
        self._folder_scan_acks: dict[str, int] = {}
        self._media_recording_audio = False
        self._media_recording_video = False
        self._want_media_input = False
        self._gl_initialized = False
        self._gl_size = (0, 0)
        self._gl_paused = False
        self._gl_frame_count = 0
        self._last_input_event: dict[str, Any] = {}
        self._port_open_state = 0
        self._friend_requests: list[dict[str, Any]] = []
        self._web_profile: dict[str, Any] = {}

    def _record_call(self, method_name: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> None:
        self.call_log.append((method_name, args, kwargs))

    @staticmethod
    def _key_from_obj(obj: Any) -> str:
        if obj is None:
            return ""
        if isinstance(obj, Path):
            return str(obj)
        if isinstance(obj, (str, int)):
            return str(obj)
        for attr in ("file_name", "fileName", "name", "id"):
            if hasattr(obj, attr):
                try:
                    attr_value = getattr(obj, attr)
                    return str(attr_value() if callable(attr_value) else attr_value)
                except Exception:
                    continue
        return str(obj)

    @staticmethod
    def _normalize_pathish(value: str) -> str:
        if not value:
            return ""
        return str(Path(value))

    @staticmethod
    def _arg(args: tuple[Any, ...], index: int, default: Any = None) -> Any:
        if index < 0:
            return default
        return args[index] if len(args) > index else default

    @staticmethod
    def _safe_int(value: Any, default: int = 0) -> int:
        try:
            return int(value)
        except (TypeError, ValueError):
            return default

    def fromGuiAppStartup(self, assets_dir: str, root_data_dir: str) -> None:
        self._record_call("fromGuiAppStartup", (assets_dir, root_data_dir), {})
        self._startup_assets_dir = self._normalize_pathish(str(assets_dir))
        self._startup_root_dir = self._normalize_pathish(str(root_data_dir))

    def fromGuiSetUserSpecificDir(self, user_specific_dir: str) -> None:
        self._record_call("fromGuiSetUserSpecificDir", (user_specific_dir,), {})
        self._user_specific_dir = self._normalize_pathish(str(user_specific_dir))

    def fromGuiSetUserXferDir(self, user_xfer_dir: str) -> None:
        self._record_call("fromGuiSetUserXferDir", (user_xfer_dir,), {})
        self._user_xfer_dir = self._normalize_pathish(str(user_xfer_dir))

    def fromGuiAppShutdown(self) -> None:
        self._record_call("fromGuiAppShutdown", (), {})

    def fromGuiDeleteUser(self, *args: Any) -> bool:
        self._record_call("fromGuiDeleteUser", args, {})
        user = str(self._arg(args, 0, "")).strip()
        if not user:
            return False
        if user in self._users:
            self._users.remove(user)
        return True

    def fromGuiGetDiskFreeSpace(self, *args: Any) -> int:
        self._record_call("fromGuiGetDiskFreeSpace", args, {})
        path = str(self._arg(args, 0, "")) or None
        probe = str(path or self._user_xfer_dir or self._user_specific_dir or self._startup_root_dir or ".")
        try:
            return int(shutil.disk_usage(probe).free)
        except (FileNotFoundError, OSError, ValueError):
            return 0

    def fromGuiGetMyIpAddress(self) -> str:
        self._record_call("fromGuiGetMyIpAddress", (), {})
        try:
            return socket.gethostbyname(socket.gethostname())
        except OSError:
            return ""

    def fromGuiGetMyIPv4Address(self) -> str:
        self._record_call("fromGuiGetMyIPv4Address", (), {})
        try:
            infos = socket.getaddrinfo(socket.gethostname(), None, family=socket.AF_INET)
            if infos:
                return str(infos[0][4][0])
        except OSError:
            pass
        return ""

    def fromGuiGetMyIPv6Address(self) -> str:
        self._record_call("fromGuiGetMyIPv6Address", (), {})
        try:
            infos = socket.getaddrinfo(socket.gethostname(), None, family=socket.AF_INET6)
            if infos:
                return str(infos[0][4][0])
        except OSError:
            pass
        return ""

    def fromGuiCancelDownload(self, *args: Any) -> None:
        self._record_call("fromGuiCancelDownload", args, {})
        key = self._key_from_obj(self._arg(args, 0, None))
        if key:
            self._download_states[key] = 0

    def fromGuiCancelUpload(self, *args: Any) -> None:
        self._record_call("fromGuiCancelUpload", args, {})
        key = self._key_from_obj(self._arg(args, 0, None))
        if key:
            self._download_states[key] = 0

    def fromGuiGetFileDownloadState(self, *args: Any) -> int:
        self._record_call("fromGuiGetFileDownloadState", args, {})
        key = self._key_from_obj(self._arg(args, 0, None))
        if not key:
            return -1
        return int(self._download_states.get(key, -1))

    def fromGuiSetFileIsShared(self, *args: Any) -> bool:
        self._record_call("fromGuiSetFileIsShared", args, {})
        file_info = self._arg(args, 0, None)
        is_shared = bool(self._arg(args, 1, True))
        key = self._key_from_obj(file_info)
        if not key:
            return False
        if bool(is_shared):
            self._shared_files.add(key)
        else:
            self._shared_files.discard(key)
        return True

    def fromGuiGetIsFileShared(self, *args: Any) -> bool:
        self._record_call("fromGuiGetIsFileShared", args, {})
        key = self._key_from_obj(self._arg(args, 0, None))
        return key in self._shared_files if key else False

    def fromGuiSetFileIsInLibrary(self, *args: Any) -> bool:
        self._record_call("fromGuiSetFileIsInLibrary", args, {})
        file_info = self._arg(args, 0, None)
        is_in_library = bool(self._arg(args, 1, True))
        key = self._key_from_obj(file_info)
        if not key:
            return False
        if bool(is_in_library):
            self._library_files.add(key)
        else:
            self._library_files.discard(key)
        return True

    def fromGuiGetFileIsInLibrary(self, *args: Any) -> bool:
        self._record_call("fromGuiGetFileIsInLibrary", args, {})
        key = self._key_from_obj(self._arg(args, 0, None))
        return key in self._library_files if key else False

    def fromGuiGetRandomTcpPort(self) -> int:
        self._record_call("fromGuiGetRandomTcpPort", (), {})
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.bind(("", 0))
            port = int(sock.getsockname()[1])
            sock.close()
            return port
        except OSError:
            return 0

    def fromGuiQueryDefaultUrl(self) -> str:
        self._record_call("fromGuiQueryDefaultUrl", (), {})
        return self._default_url

    def fromGuiSetDefaultUrl(self, *args: Any) -> bool:
        self._record_call("fromGuiSetDefaultUrl", args, {})
        url_value = str(self._arg(args, 0, "")).strip()
        if not url_value:
            return False
        if len(args) > 1:
            key = self._key_from_obj(self._arg(args, 1, None))
            self._default_urls[key] = url_value
        self._default_url = url_value
        return True

    def fromGuiGetNodeUrl(self, *args: Any) -> None:
        self._record_call("fromGuiGetNodeUrl", args, {})

    def fromGuiSetRelaySettings(self, *args: Any) -> None:
        self._record_call("fromGuiSetRelaySettings", args, {})
        self._relay_settings = self._arg(args, 0, None)

    def fromGuiGetRelaySettings(self, *args: Any) -> Any:
        self._record_call("fromGuiGetRelaySettings", args, {})
        target_dict = self._arg(args, 0, None)
        if isinstance(target_dict, dict):
            target_dict["relay_settings"] = self._relay_settings
        return self._relay_settings

    def fromGuiAnnounceHost(self, *args: Any) -> None:
        self._record_call("fromGuiAnnounceHost", args, {})
        host_key = self._key_from_obj(self._arg(args, 0, None))
        if not host_key:
            return
        self._announced_hosts[host_key] = {
            "session": self._arg(args, 1, None),
            "url": self._arg(args, 2, None),
        }

    def fromGuiJoinHost(self, *args: Any) -> None:
        self._record_call("fromGuiJoinHost", args, {})
        host_key = self._key_from_obj(self._arg(args, 0, None))
        if host_key:
            self._joined_hosts.add(host_key)

    def fromGuiLeaveHost(self, *args: Any) -> None:
        self._record_call("fromGuiLeaveHost", args, {})
        host_key = self._key_from_obj(self._arg(args, 0, None))
        if host_key:
            self._joined_hosts.discard(host_key)

    def fromGuiUnJoinHost(self, *args: Any) -> None:
        self._record_call("fromGuiUnJoinHost", args, {})
        host_key = self._key_from_obj(self._arg(args, 0, None))
        if host_key:
            self._joined_hosts.discard(host_key)

    def fromGuiSearchHost(self, *args: Any) -> None:
        self._record_call("fromGuiSearchHost", args, {})
        self._last_search = {
            "host_type": self._arg(args, 0, None),
            "params": self._arg(args, 1, None),
            "time": datetime.utcnow().isoformat(timespec="seconds"),
        }

    def fromGuiBlockUser(self, *args: Any) -> None:
        self._record_call("fromGuiBlockUser", args, {})
        user_key = self._key_from_obj(self._arg(args, 0, None))
        if user_key:
            self._blocked_users.add(user_key)

    def fromGuiSendAnnouncedList(self, *args: Any) -> None:
        self._record_call("fromGuiSendAnnouncedList", args, {})

    def fromGuiDisconnectFromUser(self, *args: Any) -> None:
        self._record_call("fromGuiDisconnectFromUser", args, {})
        user_key = self._key_from_obj(self._arg(args, 0, None))
        if user_key:
            self._joined_hosts.discard(user_key)

    def fromGuiSetPluginPermission(self, *args: Any) -> None:
        self._record_call("fromGuiSetPluginPermission", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        permission = self._safe_int(self._arg(args, 1, 0), 0)
        if plugin_key:
            self._plugin_permissions[plugin_key] = permission

    def fromGuiGetPluginPermission(self, *args: Any) -> int:
        self._record_call("fromGuiGetPluginPermission", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        if not plugin_key:
            return 0
        return int(self._plugin_permissions.get(plugin_key, 0))

    def fromGuiGetPluginServerState(self, *args: Any) -> int:
        self._record_call("fromGuiGetPluginServerState", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        if not plugin_key:
            return 0
        return 1 if any(entry.startswith(f"{plugin_key}:") for entry in self._plugin_sessions) else 0

    def fromGuiStartPluginSession(self, *args: Any) -> bool:
        self._record_call("fromGuiStartPluginSession", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        session_key = self._key_from_obj(self._arg(args, 1, "default"))
        if not plugin_key:
            return False
        self._plugin_sessions.add(f"{plugin_key}:{session_key}")
        return True

    def fromGuiStopPluginSession(self, *args: Any) -> None:
        self._record_call("fromGuiStopPluginSession", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        session_key = self._key_from_obj(self._arg(args, 1, "default"))
        if plugin_key:
            self._plugin_sessions.discard(f"{plugin_key}:{session_key}")

    def fromGuiIsPluginInSession(self, *args: Any) -> bool:
        self._record_call("fromGuiIsPluginInSession", args, {})
        plugin_key = self._key_from_obj(self._arg(args, 0, None))
        session_key = self._key_from_obj(self._arg(args, 1, "default"))
        if not plugin_key:
            return False
        return f"{plugin_key}:{session_key}" in self._plugin_sessions

    def fromGuiPushToTalk(self, *args: Any) -> bool:
        self._record_call("fromGuiPushToTalk", args, {})
        self._push_to_talk = bool(self._arg(args, 0, False))
        return True

    def fromGuiRunIsPortOpenTest(self, *args: Any) -> None:
        self._record_call("fromGuiRunIsPortOpenTest", args, {})
        self._last_port_open_test = {
            "args": args,
            "time": datetime.utcnow().isoformat(timespec="seconds"),
        }

    def fromGuiGetInternetStatus(self, *args: Any) -> int:
        self._record_call("fromGuiGetInternetStatus", args, {})
        try:
            socket.getaddrinfo("example.com", 80)
            return 1
        except OSError:
            return 0

    def fromGuiGetNetAvailStatus(self, *args: Any) -> int:
        self._record_call("fromGuiGetNetAvailStatus", args, {})
        try:
            socket.getaddrinfo(socket.gethostname(), None)
            return 1
        except OSError:
            return 0

    def fromGuiGetJoinedListCount(self, *args: Any) -> int:
        self._record_call("fromGuiGetJoinedListCount", args, {})
        return len(self._joined_hosts)

    def fromGuiGetAnnouncedHostCount(self, *args: Any) -> int:
        self._record_call("fromGuiGetAnnouncedHostCount", args, {})
        return len(self._announced_hosts)

    def fromGuiQueryHosts(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryHosts", args, {})
        target_list = self._arg(args, 0, None)
        if isinstance(target_list, list):
            target_list.extend(list(self._announced_hosts.values()))
        return True

    def fromGuiQueryMyHostedInfo(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryMyHostedInfo", args, {})
        target_list = self._arg(args, 0, None)
        if isinstance(target_list, list):
            target_list.extend(list(self._announced_hosts.values()))
        return True

    def fromGuiQueryHostListFromNetworkHost(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryHostListFromNetworkHost", args, {})
        target_list = self._arg(args, 1, None)
        if isinstance(target_list, list):
            target_list.extend(list(self._announced_hosts.values()))
        return True

    def fromGuiQueryGroupiesFromHosted(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryGroupiesFromHosted", args, {})
        target_list = self._arg(args, 1, None)
        if isinstance(target_list, list):
            target_list.extend([{"id": groupie_id} for groupie_id in sorted(self._joined_hosts)])
        return True

    def fromGuiStartScan(self, *args: Any) -> None:
        self._record_call("fromGuiStartScan", args, {})
        self._scan_active = True

    def fromGuiNextScan(self, *args: Any) -> None:
        self._record_call("fromGuiNextScan", args, {})

    def fromGuiStopScan(self, *args: Any) -> None:
        self._record_call("fromGuiStopScan", args, {})
        self._scan_active = False

    def fromGuiQueryIdentity(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryIdentity", args, {})
        return bool(args)

    def fromGuiSetIsAutomatedHost(self, *args: Any) -> None:
        self._record_call("fromGuiSetIsAutomatedHost", args, {})
        self._automated_host = bool(self._arg(args, 0, False))

    def fromGuiSendRandConnectSelected(self, *args: Any) -> bool:
        self._record_call("fromGuiSendRandConnectSelected", args, {})
        return bool(args)

    def fromGuiUserLoggedOn(self, *args: Any) -> None:
        self._record_call("fromGuiUserLoggedOn", args, {})
        user_key = self._key_from_obj(self._arg(args, 0, None))
        if user_key:
            self._users.add(user_key)

    def fromGuiClearCache(self, *args: Any) -> int:
        self._record_call("fromGuiClearCache", args, {})
        self._asset_actions.clear()
        self._queued_asset_actions.clear()
        self._sent_assets.clear()
        self._session_history.clear()
        self._folder_scan_results.clear()
        return 0

    def fromGuiOrientationEvent(self, *args: Any) -> bool:
        self._record_call("fromGuiOrientationEvent", args, {})
        self._last_input_event = {"type": "orientation", "args": args}
        return True

    def fromGuiMouseEvent(self, *args: Any) -> bool:
        self._record_call("fromGuiMouseEvent", args, {})
        self._last_input_event = {"type": "mouse", "args": args}
        return True

    def fromGuiMouseWheel(self, *args: Any) -> bool:
        self._record_call("fromGuiMouseWheel", args, {})
        self._last_input_event = {"type": "wheel", "args": args}
        return True

    def fromGuiKeyEvent(self, *args: Any) -> bool:
        self._record_call("fromGuiKeyEvent", args, {})
        self._last_input_event = {"type": "key", "args": args}
        return True

    def fromGuiNativeGlInit(self, *args: Any) -> None:
        self._record_call("fromGuiNativeGlInit", args, {})
        self._gl_initialized = True
        self._gl_paused = False

    def fromGuiNativeGlResize(self, *args: Any) -> None:
        self._record_call("fromGuiNativeGlResize", args, {})
        if len(args) >= 2:
            self._gl_size = (
                self._safe_int(self._arg(args, 0, 0), 0),
                self._safe_int(self._arg(args, 1, 0), 0),
            )

    def fromGuiNativeGlRender(self, *args: Any) -> int:
        self._record_call("fromGuiNativeGlRender", args, {})
        if not self._gl_initialized or self._gl_paused:
            return 0
        self._gl_frame_count += 1
        return 1

    def fromGuiNativeGlPauseRender(self, *args: Any) -> None:
        self._record_call("fromGuiNativeGlPauseRender", args, {})
        self._gl_paused = True

    def fromGuiNativeGlResumeRender(self, *args: Any) -> None:
        self._record_call("fromGuiNativeGlResumeRender", args, {})
        self._gl_paused = False

    def fromGuiNativeGlDestroy(self, *args: Any) -> None:
        self._record_call("fromGuiNativeGlDestroy", args, {})
        self._gl_initialized = False
        self._gl_size = (0, 0)

    def fromGuiSndRecord(self, *args: Any) -> bool:
        self._record_call("fromGuiSndRecord", args, {})
        self._media_recording_audio = bool(self._arg(args, 0, not self._media_recording_audio))
        return True

    def fromGuiVideoRecord(self, *args: Any) -> bool:
        self._record_call("fromGuiVideoRecord", args, {})
        self._media_recording_video = bool(self._arg(args, 0, not self._media_recording_video))
        return True

    def fromGuiPlayLocalMedia(self, *args: Any) -> bool:
        self._record_call("fromGuiPlayLocalMedia", args, {})
        path = self._key_from_obj(self._arg(args, 0, None))
        return bool(path and Path(path).exists())

    def fromGuiWantMediaInput(self, *args: Any) -> None:
        self._record_call("fromGuiWantMediaInput", args, {})
        self._want_media_input = bool(self._arg(args, 0, False))

    def fromGuiOnlineNameChanged(self, *args: Any) -> None:
        self._record_call("fromGuiOnlineNameChanged", args, {})
        online_name = self._key_from_obj(self._arg(args, 0, None))
        if online_name:
            self._identity["online_name"] = online_name

    def fromGuiMoodMessageChanged(self, *args: Any) -> None:
        self._record_call("fromGuiMoodMessageChanged", args, {})
        mood_message = self._key_from_obj(self._arg(args, 0, None))
        if mood_message:
            self._identity["mood_message"] = mood_message

    def fromGuiIdentPersonalInfoChanged(self, *args: Any) -> None:
        self._record_call("fromGuiIdentPersonalInfoChanged", args, {})
        personal_info = self._arg(args, 0, None)
        if personal_info is not None:
            self._identity["personal_info"] = personal_info

    def fromGuiSetUserHasProfilePicture(self, *args: Any) -> None:
        self._record_call("fromGuiSetUserHasProfilePicture", args, {})
        self._identity["has_profile_picture"] = bool(self._arg(args, 0, False))

    def fromGuiUpdateMyIdent(self, *args: Any) -> None:
        self._record_call("fromGuiUpdateMyIdent", args, {})
        ident_value = self._arg(args, 0, None)
        if ident_value is not None:
            self._identity["ident"] = ident_value

    def fromGuiQueryMyIdent(self, *args: Any) -> None:
        self._record_call("fromGuiQueryMyIdent", args, {})
        target_dict = self._arg(args, 0, None)
        if isinstance(target_dict, dict):
            target_dict.update(self._identity)

    def fromGuiSetIdentHasTextOffers(self, *args: Any) -> None:
        self._record_call("fromGuiSetIdentHasTextOffers", args, {})
        ident_key = self._key_from_obj(self._arg(args, 0, "self"))
        has_text_offers = bool(self._arg(args, 1, True))
        self._text_offer_flags[ident_key] = has_text_offers

    def fromGuiChangeMyFriendshipToHim(self, *args: Any) -> bool:
        self._record_call("fromGuiChangeMyFriendshipToHim", args, {})
        if not args:
            return False
        user_key = self._key_from_obj(self._arg(args, 0, None))
        level = self._safe_int(self._arg(args, 1, 0), 0)
        self._friendships[user_key] = level
        return True

    def fromGuiApplyNetHostSettings(self, *args: Any) -> None:
        self._record_call("fromGuiApplyNetHostSettings", args, {})
        setting_value = self._arg(args, 0, None)
        if setting_value is not None:
            self._net_host_settings["current"] = setting_value

    def fromGuiSetNetSettings(self, *args: Any) -> None:
        self._record_call("fromGuiSetNetSettings", args, {})
        setting_value = self._arg(args, 0, None)
        if setting_value is not None:
            self._net_settings["current"] = setting_value

    def fromGuiGetNetSettings(self, *args: Any) -> Any:
        self._record_call("fromGuiGetNetSettings", args, {})
        target_dict = self._arg(args, 0, None)
        if isinstance(target_dict, dict):
            target_dict.update(self._net_settings)
        return self._net_settings.get("current")

    def fromGuiRunUrlAction(self, *args: Any) -> None:
        self._record_call("fromGuiRunUrlAction", args, {})
        self._last_url_action = {
            "args": args,
            "time": datetime.utcnow().isoformat(timespec="seconds"),
        }

    def fromGuiSetFileShareSettings(self, *args: Any) -> None:
        self._record_call("fromGuiSetFileShareSettings", args, {})
        setting_value = self._arg(args, 0, None)
        if setting_value is not None:
            self._file_share_settings["current"] = setting_value

    def fromGuiGetFileShareSettings(self, *args: Any) -> Any:
        self._record_call("fromGuiGetFileShareSettings", args, {})
        target_dict = self._arg(args, 0, None)
        if isinstance(target_dict, dict):
            target_dict.update(self._file_share_settings)
        return self._file_share_settings.get("current")

    def fromGuiUpdateWebPageProfile(self, *args: Any) -> None:
        self._record_call("fromGuiUpdateWebPageProfile", args, {})
        self._web_profile = {
            "args": args,
            "time": datetime.utcnow().isoformat(timespec="seconds"),
        }

    def fromGuiUpdatePluginPermission(self, *args: Any) -> None:
        self._record_call("fromGuiUpdatePluginPermission", args, {})
        self.fromGuiSetPluginPermission(*args)

    def fromGuiMakePluginOffer(self, *args: Any) -> bool:
        self._record_call("fromGuiMakePluginOffer", args, {})
        self._plugin_offers.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiToPluginOfferReply(self, *args: Any) -> bool:
        self._record_call("fromGuiToPluginOfferReply", args, {})
        self._plugin_offer_replies.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiFileXferControl(self, *args: Any) -> int:
        self._record_call("fromGuiFileXferControl", args, {})
        key = self._key_from_obj(self._arg(args, 1, self._arg(args, 0, None)))
        if key:
            self._download_states[key] = self._safe_int(self._arg(args, 0, 0), 0)
        return 0

    def fromGuiInstMsg(self, *args: Any) -> bool:
        self._record_call("fromGuiInstMsg", args, {})
        self._instant_messages.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiAdminViewHost(self, *args: Any) -> None:
        self._record_call("fromGuiAdminViewHost", args, {})
        self._admin_view_host = bool(self._arg(args, 0, False))

    def fromGuiSendContactList(self, *args: Any) -> None:
        self._record_call("fromGuiSendContactList", args, {})
        contact_list = self._arg(args, 0, None)
        if isinstance(contact_list, list):
            self._contact_list = list(contact_list)

    def fromGuiRefreshContactList(self, *args: Any) -> None:
        self._record_call("fromGuiRefreshContactList", args, {})

    def fromGuiTodGameActionSend(self, *args: Any) -> bool:
        self._record_call("fromGuiTodGameActionSend", args, {})
        self._multi_session_actions.append({"type": "tod", "args": args})
        return True

    def fromGuiBrowseFiles(self, *args: Any) -> bool:
        self._record_call("fromGuiBrowseFiles", args, {})
        start_dir = self._key_from_obj(self._arg(args, 0, None))
        if not start_dir:
            start_dir = self._user_specific_dir or self._startup_root_dir
        path = Path(start_dir)
        return path.exists() and path.is_dir()

    def fromGuiGetFileLibraryList(self, *args: Any) -> None:
        self._record_call("fromGuiGetFileLibraryList", args, {})
        target_list = self._arg(args, 0, None)
        if isinstance(target_list, list):
            target_list.extend(sorted(self._library_files))

    def fromGuiScanFolderForMedia(self, *args: Any) -> None:
        self._record_call("fromGuiScanFolderForMedia", args, {})
        folder = self._key_from_obj(self._arg(args, 0, None))
        session_key = self._key_from_obj(self._arg(args, 1, folder))
        result: list[str] = []
        if folder:
            path = Path(folder)
            if path.exists() and path.is_dir():
                for file_path in path.iterdir():
                    if file_path.is_file() and (self.fromGuiIsNoLimitVideoFile(str(file_path)) or self.fromGuiIsNoLimitAudioFile(str(file_path))):
                        result.append(str(file_path))
        self._folder_scan_results[session_key] = result
        self._folder_scan_acks[session_key] = 0

    def fromGuiScanItemReceived(self, *args: Any) -> None:
        self._record_call("fromGuiScanItemReceived", args, {})
        session_key = self._key_from_obj(self._arg(args, 0, None))
        if session_key:
            self._folder_scan_acks[session_key] = self._folder_scan_acks.get(session_key, 0) + 1

    def fromGuiScanFolderCancel(self, *args: Any) -> None:
        self._record_call("fromGuiScanFolderCancel", args, {})
        session_key = self._key_from_obj(self._arg(args, 0, None))
        if session_key:
            self._folder_scan_results.pop(session_key, None)
            self._folder_scan_acks.pop(session_key, None)

    def fromGuiIsNoLimitVideoFile(self, *args: Any) -> bool:
        self._record_call("fromGuiIsNoLimitVideoFile", args, {})
        path = self._key_from_obj(self._arg(args, 0, None))
        return Path(path).suffix.lower() in {".mp4", ".mkv", ".avi", ".mov", ".webm"}

    def fromGuiIsNoLimitAudioFile(self, *args: Any) -> bool:
        self._record_call("fromGuiIsNoLimitAudioFile", args, {})
        path = self._key_from_obj(self._arg(args, 0, None))
        return Path(path).suffix.lower() in {".mp3", ".wav", ".flac", ".ogg", ".m4a"}

    def fromGuiDeleteFile(self, *args: Any) -> int:
        self._record_call("fromGuiDeleteFile", args, {})
        path_value = self._key_from_obj(self._arg(args, 0, None))
        if not path_value:
            return -1
        path = Path(path_value)
        try:
            if path.exists() and path.is_file():
                path.unlink()
                return 0
        except OSError:
            return -1
        return -1

    def fromGuiQuerySessionHistory(self, *args: Any) -> None:
        self._record_call("fromGuiQuerySessionHistory", args, {})
        session_key = self._key_from_obj(self._arg(args, 0, None))
        history = self._session_history.get(session_key, [])
        target_list = self._arg(args, 1, None)
        if isinstance(target_list, list):
            target_list.extend(history)

    def fromGuiAssetAction(self, *args: Any) -> bool:
        self._record_call("fromGuiAssetAction", args, {})
        self._asset_actions.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiQueueAssetAction(self, *args: Any) -> bool:
        self._record_call("fromGuiQueueAssetAction", args, {})
        self._queued_asset_actions.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiSendAsset(self, *args: Any) -> bool:
        self._record_call("fromGuiSendAsset", args, {})
        self._sent_assets.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiMultiSessionAction(self, *args: Any) -> bool:
        self._record_call("fromGuiMultiSessionAction", args, {})
        self._multi_session_actions.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True

    def fromGuiTestCmd(self, *args: Any) -> bool:
        self._record_call("fromGuiTestCmd", args, {})
        return True

    def fromGuiListAction(self, *args: Any) -> None:
        self._record_call("fromGuiListAction", args, {})
        self._last_list_action = {"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")}

    def fromGuiDownloadWebPage(self, *args: Any) -> bool:
        self._record_call("fromGuiDownloadWebPage", args, {})
        key = self._key_from_obj(self._arg(args, 0, "default"))
        self._web_page_downloads[key] = {"args": args, "active": True, "time": datetime.utcnow().isoformat(timespec="seconds")}
        return True

    def fromGuiCancelWebPage(self, *args: Any) -> bool:
        self._record_call("fromGuiCancelWebPage", args, {})
        key = self._key_from_obj(self._arg(args, 0, "default"))
        if key in self._web_page_downloads:
            self._web_page_downloads[key]["active"] = False
        return True

    def fromGuiDownloadFileList(self, *args: Any) -> bool:
        self._record_call("fromGuiDownloadFileList", args, {})
        key = self._key_from_obj(self._arg(args, 0, "default"))
        self._file_list_downloads[key] = {"args": args, "active": True, "time": datetime.utcnow().isoformat(timespec="seconds")}
        return True

    def fromGuiDownloadFileListCancel(self, *args: Any) -> bool:
        self._record_call("fromGuiDownloadFileListCancel", args, {})
        key = self._key_from_obj(self._arg(args, 0, "default"))
        if key in self._file_list_downloads:
            self._file_list_downloads[key]["active"] = False
        return True

    def fromGuiQueryJoinState(self, *args: Any) -> int:
        self._record_call("fromGuiQueryJoinState", args, {})
        host_key = self._key_from_obj(self._arg(args, 0, None))
        return 1 if host_key and host_key in self._joined_hosts else 0

    def fromGuiQueryFileHash(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryFileHash", args, {})
        path_value = self._key_from_obj(self._arg(args, 0, None))
        if not path_value:
            return False
        path = Path(path_value)
        if not path.exists() or not path.is_file():
            return False
        import hashlib

        hasher = hashlib.sha256()
        try:
            with path.open("rb") as handle:
                for chunk in iter(lambda: handle.read(65536), b""):
                    hasher.update(chunk)
        except OSError:
            return False

        out = self._arg(args, 1, None)
        if isinstance(out, dict):
            out["sha256"] = hasher.hexdigest()
        return True

    def fromGuiDeleteDatabase(self, *args: Any) -> bool:
        self._record_call("fromGuiDeleteDatabase", args, {})
        # Reset in-memory model stores used by this migration stub.
        self._announced_hosts.clear()
        self._joined_hosts.clear()
        self._blocked_users.clear()
        self._plugin_permissions.clear()
        self._plugin_sessions.clear()
        self._download_states.clear()
        self._shared_files.clear()
        self._library_files.clear()
        self._session_history.clear()
        self._friend_requests.clear()
        return True

    def fromGuiQueryFriendRequest(self, *args: Any) -> bool:
        self._record_call("fromGuiQueryFriendRequest", args, {})
        target_list = self._arg(args, 0, None)
        if isinstance(target_list, list):
            target_list.extend(self._friend_requests)
        return True

    def fromGuiSendFriendRequest(self, *args: Any) -> bool:
        self._record_call("fromGuiSendFriendRequest", args, {})
        self._friend_requests.append({"args": args, "time": datetime.utcnow().isoformat(timespec="seconds")})
        return True


class IToGuiContractStub(GuiInterfaceContractBase):
    """Contract mirror of libs/GuiInterface/IToGui.h."""

    _METHODS = {
        "toGuiMediaAction": GuiInterfaceMethodSpec("toGuiMediaAction", False, "IToGui::toGuiMediaAction", "Media action callback path."),
        "toGuiMediaError": GuiInterfaceMethodSpec("toGuiMediaError", None, "IToGui::toGuiMediaError", "Media error callback path."),
        "toGuiSetIsAppModuleRunning": GuiInterfaceMethodSpec("toGuiSetIsAppModuleRunning", None, "IToGui::toGuiSetIsAppModuleRunning", "Track module running state."),
        "toGuiGetIsAppModuleRunning": GuiInterfaceMethodSpec("toGuiGetIsAppModuleRunning", False, "IToGui::toGuiGetIsAppModuleRunning", "Return tracked module running state."),
        "toGuiRunModule": GuiInterfaceMethodSpec("toGuiRunModule", False, "IToGui::toGuiRunModule", "Request GUI-side module startup."),
        "toGuiStopModule": GuiInterfaceMethodSpec("toGuiStopModule", False, "IToGui::toGuiStopModule", "Request GUI-side module stop."),
        "toGuiPlayNlcMedia": GuiInterfaceMethodSpec("toGuiPlayNlcMedia", None, "IToGui::toGuiPlayNlcMedia", "Play media asset in GUI layer."),
        "toGuiLog": GuiInterfaceMethodSpec("toGuiLog", None, "IToGui::toGuiLog", "Forward log entry to GUI logger."),
        "toGuiAppErr": GuiInterfaceMethodSpec("toGuiAppErr", None, "IToGui::toGuiAppErr", "Forward non-popup app error to GUI."),
        "toGuiAppPopupErr": GuiInterfaceMethodSpec("toGuiAppPopupErr", None, "IToGui::toGuiAppPopupErr", "Forward popup-worthy app error to GUI."),
        "toGuiStatusMessage": GuiInterfaceMethodSpec("toGuiStatusMessage", None, "IToGui::toGuiStatusMessage", "Forward status bar message."),
        "toGuiPluginMsg": GuiInterfaceMethodSpec("toGuiPluginMsg", None, "IToGui::toGuiPluginMsg", "Forward plugin message callback."),
        "toGuiPluginCommError": GuiInterfaceMethodSpec("toGuiPluginCommError", None, "IToGui::toGuiPluginCommError", "Forward plugin communication error callback."),
        "toGuiModuleState": GuiInterfaceMethodSpec("toGuiModuleState", None, "IToGui::toGuiModuleState", "Module lifecycle state callback."),
        "toGuiWantVideoCapture": GuiInterfaceMethodSpec("toGuiWantVideoCapture", None, "IToGui::toGuiWantVideoCapture", "Start/stop capture request callback."),
        "toGuiPlayJpgVideo": GuiInterfaceMethodSpec("toGuiPlayJpgVideo", None, "IToGui::toGuiPlayJpgVideo", "Video frame callback path."),
        "toGuiHostAnnounceStatus": GuiInterfaceMethodSpec("toGuiHostAnnounceStatus", None, "IToGui::toGuiHostAnnounceStatus", "Host announce status callback."),
        "toGuiHostJoinStatus": GuiInterfaceMethodSpec("toGuiHostJoinStatus", None, "IToGui::toGuiHostJoinStatus", "Host join status callback."),
        "toGuiHostSearchStatus": GuiInterfaceMethodSpec("toGuiHostSearchStatus", None, "IToGui::toGuiHostSearchStatus", "Host search status callback."),
        "toGuiHostSearchResult": GuiInterfaceMethodSpec("toGuiHostSearchResult", None, "IToGui::toGuiHostSearchResult", "Host search result callback."),
        "toGuiHostSearchComplete": GuiInterfaceMethodSpec("toGuiHostSearchComplete", None, "IToGui::toGuiHostSearchComplete", "Host search complete callback."),
        "toGuiGroupieSearchStatus": GuiInterfaceMethodSpec("toGuiGroupieSearchStatus", None, "IToGui::toGuiGroupieSearchStatus", "Groupie search status callback."),
        "toGuiGroupieSearchResult": GuiInterfaceMethodSpec("toGuiGroupieSearchResult", None, "IToGui::toGuiGroupieSearchResult", "Groupie search result callback."),
        "toGuiGroupieSearchComplete": GuiInterfaceMethodSpec("toGuiGroupieSearchComplete", None, "IToGui::toGuiGroupieSearchComplete", "Groupie search complete callback."),
        "toGuiIsPortOpenStatus": GuiInterfaceMethodSpec("toGuiIsPortOpenStatus", None, "IToGui::toGuiIsPortOpenStatus", "Port-open test status callback."),
        "toGuiNetAvailableStatus": GuiInterfaceMethodSpec("toGuiNetAvailableStatus", None, "IToGui::toGuiNetAvailableStatus", "Network availability callback."),
        "toGuiNetworkState": GuiInterfaceMethodSpec("toGuiNetworkState", None, "IToGui::toGuiNetworkState", "Network state callback."),
        "toGuiRandomConnectStatus": GuiInterfaceMethodSpec("toGuiRandomConnectStatus", None, "IToGui::toGuiRandomConnectStatus", "Random-connect status callback."),
        "toGuiRunTestStatus": GuiInterfaceMethodSpec("toGuiRunTestStatus", None, "IToGui::toGuiRunTestStatus", "General test status callback."),
        "toGuiIndentListUpdate": GuiInterfaceMethodSpec("toGuiIndentListUpdate", None, "IToGui::toGuiIndentListUpdate", "User view list update callback."),
        "toGuiIndentListRemove": GuiInterfaceMethodSpec("toGuiIndentListRemove", None, "IToGui::toGuiIndentListRemove", "User view list remove callback."),
        "toGuiContactAdded": GuiInterfaceMethodSpec("toGuiContactAdded", None, "IToGui::toGuiContactAdded", "Contact added callback."),
        "toGuiContactRemoved": GuiInterfaceMethodSpec("toGuiContactRemoved", None, "IToGui::toGuiContactRemoved", "Contact removed callback."),
        "toGuiContactOnline": GuiInterfaceMethodSpec("toGuiContactOnline", None, "IToGui::toGuiContactOnline", "Contact online callback."),
        "toGuiContactAnythingChange": GuiInterfaceMethodSpec("toGuiContactAnythingChange", None, "IToGui::toGuiContactAnythingChange", "Generic contact change callback."),
        "toGuiContactLastSessionTimeChange": GuiInterfaceMethodSpec("toGuiContactLastSessionTimeChange", None, "IToGui::toGuiContactLastSessionTimeChange", "Contact last-session-time callback."),
        "toGuiUpdateMyIdent": GuiInterfaceMethodSpec("toGuiUpdateMyIdent", None, "IToGui::toGuiUpdateMyIdent", "My-identity update callback."),
        "toGuiSaveMyIdent": GuiInterfaceMethodSpec("toGuiSaveMyIdent", None, "IToGui::toGuiSaveMyIdent", "My-identity persist callback."),
        "toGuiRxedPluginOffer": GuiInterfaceMethodSpec("toGuiRxedPluginOffer", None, "IToGui::toGuiRxedPluginOffer", "Plugin offer received callback."),
        "toGuiRxedOfferReply": GuiInterfaceMethodSpec("toGuiRxedOfferReply", None, "IToGui::toGuiRxedOfferReply", "Plugin offer reply callback."),
        "toGuiPluginSessionStarted": GuiInterfaceMethodSpec("toGuiPluginSessionStarted", None, "IToGui::toGuiPluginSessionStarted", "Plugin session started callback."),
        "toGuiPluginSessionEnded": GuiInterfaceMethodSpec("toGuiPluginSessionEnded", None, "IToGui::toGuiPluginSessionEnded", "Plugin session ended callback."),
        "toGuiPluginStatus": GuiInterfaceMethodSpec("toGuiPluginStatus", None, "IToGui::toGuiPluginStatus", "Plugin status/value callback."),
        "toGuiInstMsg": GuiInterfaceMethodSpec("toGuiInstMsg", None, "IToGui::toGuiInstMsg", "Instant message received callback."),
        "toGuiFileListReply": GuiInterfaceMethodSpec("toGuiFileListReply", None, "IToGui::toGuiFileListReply", "Shared file list callback."),
        "toGuiFileList": GuiInterfaceMethodSpec("toGuiFileList", None, "IToGui::toGuiFileList", "File list callback."),
        "toGuiFileListCompleted": GuiInterfaceMethodSpec("toGuiFileListCompleted", None, "IToGui::toGuiFileListCompleted", "File list completion callback."),
        "toGuiFolderScan": GuiInterfaceMethodSpec("toGuiFolderScan", None, "IToGui::toGuiFolderScan", "Folder scan entry callback."),
        "toGuiFolderScanCompleted": GuiInterfaceMethodSpec("toGuiFolderScanCompleted", None, "IToGui::toGuiFolderScanCompleted", "Folder scan complete callback."),
        "toGuiFileUploadStart": GuiInterfaceMethodSpec("toGuiFileUploadStart", None, "IToGui::toGuiFileUploadStart", "Upload start callback."),
        "toGuiFileUploadComplete": GuiInterfaceMethodSpec("toGuiFileUploadComplete", None, "IToGui::toGuiFileUploadComplete", "Upload complete callback."),
        "toGuiFileDownloadStart": GuiInterfaceMethodSpec("toGuiFileDownloadStart", None, "IToGui::toGuiFileDownloadStart", "Download start callback."),
        "toGuiFileDownloadComplete": GuiInterfaceMethodSpec("toGuiFileDownloadComplete", None, "IToGui::toGuiFileDownloadComplete", "Download complete callback."),
        "toGuiFileXferState": GuiInterfaceMethodSpec("toGuiFileXferState", None, "IToGui::toGuiFileXferState", "Transfer progress/state callback."),
        "toGuiFileDeleted": GuiInterfaceMethodSpec("toGuiFileDeleted", None, "IToGui::toGuiFileDeleted", "File deletion callback."),
        "toGuiAssetAdded": GuiInterfaceMethodSpec("toGuiAssetAdded", None, "IToGui::toGuiAssetAdded", "Asset added callback."),
        "toGuiAssetUpdated": GuiInterfaceMethodSpec("toGuiAssetUpdated", None, "IToGui::toGuiAssetUpdated", "Asset updated callback."),
        "toGuiAssetRemoved": GuiInterfaceMethodSpec("toGuiAssetRemoved", None, "IToGui::toGuiAssetRemoved", "Asset removed callback."),
        "toGuiAssetXferState": GuiInterfaceMethodSpec("toGuiAssetXferState", None, "IToGui::toGuiAssetXferState", "Asset transfer state callback."),
        "toGuiAssetSessionHistory": GuiInterfaceMethodSpec("toGuiAssetSessionHistory", None, "IToGui::toGuiAssetSessionHistory", "Asset history callback."),
        "toGuiAssetAction": GuiInterfaceMethodSpec("toGuiAssetAction", None, "IToGui::toGuiAssetAction", "Asset action callback."),
        "toGuiMultiSessionAction": GuiInterfaceMethodSpec("toGuiMultiSessionAction", None, "IToGui::toGuiMultiSessionAction", "Multi-session action callback."),
        "toGuiBlobAdded": GuiInterfaceMethodSpec("toGuiBlobAdded", None, "IToGui::toGuiBlobAdded", "Blob added callback."),
        "toGuiBlobAction": GuiInterfaceMethodSpec("toGuiBlobAction", None, "IToGui::toGuiBlobAction", "Blob action callback."),
        "toGuiBlobSessionHistory": GuiInterfaceMethodSpec("toGuiBlobSessionHistory", None, "IToGui::toGuiBlobSessionHistory", "Blob history callback."),
        "toGuiTodGameAction": GuiInterfaceMethodSpec("toGuiTodGameAction", None, "IToGui::toGuiTodGameAction", "Game action callback."),
        "toGuiSearchResultFileSearch": GuiInterfaceMethodSpec("toGuiSearchResultFileSearch", None, "IToGui::toGuiSearchResultFileSearch", "Search result callback."),
        "toGuiNetworkIsTested": GuiInterfaceMethodSpec("toGuiNetworkIsTested", None, "IToGui::toGuiNetworkIsTested", "Network tested callback."),
        "toGuiAdminAvail": GuiInterfaceMethodSpec("toGuiAdminAvail", None, "IToGui::toGuiAdminAvail", "Admin availability callback."),
        "toGuiUpdateWantMicrophoneCount": GuiInterfaceMethodSpec("toGuiUpdateWantMicrophoneCount", None, "IToGui::toGuiUpdateWantMicrophoneCount", "Microphone-demand counter callback."),
        "toGuiUpdateWantSpeakerCount": GuiInterfaceMethodSpec("toGuiUpdateWantSpeakerCount", None, "IToGui::toGuiUpdateWantSpeakerCount", "Speaker-demand counter callback."),
    }

    def __init__(self) -> None:
        super().__init__()
        self.module_running: dict[str, bool] = {}
        self.last_status_message = ""
        self.last_app_error = ""
        self.last_popup_error = ""
        self.logs: list[str] = []
        self.event_history: list[dict[str, Any]] = []
        self.last_network_state: tuple[int, str] = (0, "")
        self.last_net_available_status = 0
        self.last_plugin_status: tuple[int, int, int] = (0, 0, 0)
        self.last_plugin_message: tuple[int, Any, int, str] = (0, None, 0, "")
        self.last_plugin_comm_error: tuple[int, Any, int, int] = (0, None, 0, 0)
        self.last_host_announce_status: tuple[Any, ...] = ()
        self.last_host_join_status: tuple[Any, ...] = ()
        self.last_host_search_status: tuple[Any, ...] = ()
        self.last_host_search_result: tuple[Any, ...] = ()
        self.last_host_search_complete: tuple[Any, ...] = ()
        self.last_groupie_search_status: tuple[Any, ...] = ()
        self.last_groupie_search_result: tuple[Any, ...] = ()
        self.last_groupie_search_complete: tuple[Any, ...] = ()
        self.last_port_open_status: tuple[Any, ...] = ()
        self.last_run_test_status: tuple[Any, ...] = ()
        self.last_random_connect_status: tuple[Any, ...] = ()
        self.last_network_is_tested: tuple[Any, ...] = ()
        self.last_file_xfer_state: tuple[Any, ...] = ()
        self.last_folder_scan: tuple[Any, ...] = ()
        self.last_folder_scan_completed: tuple[Any, ...] = ()
        self.last_media_error: tuple[Any, ...] = ()
        self.last_module_state: tuple[Any, ...] = ()
        self.last_want_video_capture: tuple[Any, ...] = ()
        self.last_play_jpg_video: tuple[Any, ...] = ()
        self.last_play_nlc_media: tuple[Any, ...] = ()
        self.last_indent_list_update: tuple[Any, ...] = ()
        self.last_indent_list_remove: tuple[Any, ...] = ()
        self.last_contact_added: tuple[Any, ...] = ()
        self.last_contact_removed: tuple[Any, ...] = ()
        self.last_contact_online: tuple[Any, ...] = ()
        self.last_contact_anything_change: tuple[Any, ...] = ()
        self.last_contact_last_session_time_change: tuple[Any, ...] = ()
        self.last_update_my_ident: tuple[Any, ...] = ()
        self.last_save_my_ident: tuple[Any, ...] = ()
        self.last_rxed_plugin_offer: tuple[Any, ...] = ()
        self.last_rxed_offer_reply: tuple[Any, ...] = ()
        self.last_plugin_session_started: tuple[Any, ...] = ()
        self.last_plugin_session_ended: tuple[Any, ...] = ()
        self.last_file_list_reply: tuple[Any, ...] = ()
        self.last_file_list: tuple[Any, ...] = ()
        self.last_file_list_completed: tuple[Any, ...] = ()
        self.last_file_upload_start: tuple[Any, ...] = ()
        self.last_file_upload_complete: tuple[Any, ...] = ()
        self.last_file_download_start: tuple[Any, ...] = ()
        self.last_file_download_complete: tuple[Any, ...] = ()
        self.last_file_deleted: tuple[Any, ...] = ()
        self.last_asset_added: tuple[Any, ...] = ()
        self.last_asset_updated: tuple[Any, ...] = ()
        self.last_asset_removed: tuple[Any, ...] = ()
        self.last_asset_xfer_state: tuple[Any, ...] = ()
        self.last_asset_session_history: tuple[Any, ...] = ()
        self.last_asset_action: tuple[Any, ...] = ()
        self.last_multi_session_action: tuple[Any, ...] = ()
        self.last_blob_added: tuple[Any, ...] = ()
        self.last_blob_action: tuple[Any, ...] = ()
        self.last_blob_session_history: tuple[Any, ...] = ()
        self.last_tod_game_action: tuple[Any, ...] = ()
        self.last_inst_msg: tuple[Any, ...] = ()
        self.last_search_result_file_search: tuple[Any, ...] = ()
        self.last_microphone_want_count = 0
        self.last_speaker_want_count = 0
        self.last_admin_available = False

    @staticmethod
    def _module_key(args: tuple[Any, ...]) -> str:
        if not args:
            return "default"
        return str(args[0])

    @staticmethod
    def _safe_int(value: Any, default: int = 0) -> int:
        try:
            return int(value)
        except (TypeError, ValueError):
            return default

    @staticmethod
    def _arg(args: tuple[Any, ...], index: int, default: Any = None) -> Any:
        if index < 0:
            return default
        return args[index] if len(args) > index else default

    def _record_to_gui(self, method_name: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> None:
        self.call_log.append((method_name, args, kwargs))
        self.event_history.append(
            {
                "method": method_name,
                "args": args,
                "kwargs": kwargs,
                "time": datetime.utcnow().isoformat(timespec="seconds"),
            }
        )
        if len(self.event_history) > 512:
            self.event_history = self.event_history[-512:]

    def _handle_stub_call(self, method_name: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> Any:
        self._record_to_gui(method_name, args, kwargs)
        spec = self._METHODS.get(method_name)
        if spec is None:
            raise AttributeError(method_name)

        if method_name == "toGuiSetIsAppModuleRunning":
            module = self._module_key(args)
            running = bool(self._arg(args, 1, self._arg(args, 0, False)))
            self.module_running[module] = running
            return None

        if method_name == "toGuiGetIsAppModuleRunning":
            module = self._module_key(args)
            return bool(self.module_running.get(module, False))

        if method_name == "toGuiRunModule":
            module = self._module_key(args)
            self.module_running[module] = True
            return True

        if method_name == "toGuiStopModule":
            module = self._module_key(args)
            self.module_running[module] = False
            return True

        if method_name == "toGuiStatusMessage":
            self.last_status_message = str(args[0]) if args else ""
            self.logger.info("IToGui status: %s", self.last_status_message)
            return None

        if method_name == "toGuiLog":
            message = " ".join(str(v) for v in args)
            self.logs.append(message)
            if len(self.logs) > 1024:
                self.logs = self.logs[-1024:]
            self.logger.info("IToGui log: %s", message)
            return None

        if method_name == "toGuiAppErr":
            self.last_app_error = " ".join(str(v) for v in args)
            self.logger.error("IToGui app error: %s", self.last_app_error)
            return None

        if method_name == "toGuiAppPopupErr":
            self.last_popup_error = " ".join(str(v) for v in args)
            self.logger.error("IToGui popup error: %s", self.last_popup_error)
            return None

        if method_name == "toGuiNetworkState":
            state = self._safe_int(self._arg(args, 0, 0), 0)
            message = str(self._arg(args, 1, ""))
            self.last_network_state = (state, message)
            return None

        if method_name == "toGuiNetAvailableStatus":
            self.last_net_available_status = self._safe_int(self._arg(args, 0, 0), 0)
            return None

        if method_name == "toGuiPluginStatus":
            plugin = self._safe_int(self._arg(args, 0, 0), 0)
            status = self._safe_int(self._arg(args, 1, 0), 0)
            status_value = self._safe_int(self._arg(args, 2, 0), 0)
            self.last_plugin_status = (plugin, status, status_value)
            return None

        if method_name == "toGuiPluginMsg":
            plugin = self._safe_int(self._arg(args, 0, 0), 0)
            online_id = self._arg(args, 1, None)
            msg_type = self._safe_int(self._arg(args, 2, 0), 0)

            # Some call paths include a compact 3-arg form where the trailing value is the message.
            if len(args) > 3:
                message = str(self._arg(args, 3, ""))
            elif len(args) > 2 and not isinstance(self._arg(args, 2, None), (int, float, bool)):
                message = str(self._arg(args, 2, ""))
                msg_type = 0
            else:
                message = ""
            self.last_plugin_message = (plugin, online_id, msg_type, message)
            return None

        if method_name == "toGuiPluginCommError":
            plugin = self._safe_int(self._arg(args, 0, 0), 0)
            online_id = self._arg(args, 1, None)
            msg_type = self._safe_int(self._arg(args, 2, 0), 0)
            comm_error = self._safe_int(self._arg(args, 3, 0), 0)

            # Compact 3-arg form: plugin, online_id, comm_error
            if len(args) == 3 and isinstance(self._arg(args, 2, None), (int, float, bool)):
                msg_type = 0
                comm_error = self._safe_int(self._arg(args, 2, 0), 0)
            self.last_plugin_comm_error = (plugin, online_id, msg_type, comm_error)
            return None

        if method_name == "toGuiHostAnnounceStatus":
            self.last_host_announce_status = args
            return None

        if method_name == "toGuiHostJoinStatus":
            self.last_host_join_status = args
            return None

        if method_name == "toGuiHostSearchStatus":
            self.last_host_search_status = args
            return None

        if method_name == "toGuiHostSearchResult":
            self.last_host_search_result = args
            return None

        if method_name == "toGuiHostSearchComplete":
            self.last_host_search_complete = args
            return None

        if method_name == "toGuiGroupieSearchStatus":
            self.last_groupie_search_status = args
            return None

        if method_name == "toGuiGroupieSearchResult":
            self.last_groupie_search_result = args
            return None

        if method_name == "toGuiGroupieSearchComplete":
            self.last_groupie_search_complete = args
            return None

        if method_name == "toGuiIsPortOpenStatus":
            self.last_port_open_status = args
            return None

        if method_name == "toGuiRunTestStatus":
            self.last_run_test_status = args
            return None

        if method_name == "toGuiRandomConnectStatus":
            self.last_random_connect_status = args
            return None

        if method_name == "toGuiNetworkIsTested":
            self.last_network_is_tested = args
            return None

        if method_name == "toGuiFileXferState":
            self.last_file_xfer_state = args
            return None

        if method_name == "toGuiFolderScan":
            self.last_folder_scan = args
            return None

        if method_name == "toGuiFolderScanCompleted":
            self.last_folder_scan_completed = args
            return None

        if method_name == "toGuiUpdateWantMicrophoneCount":
            self.last_microphone_want_count = self._safe_int(self._arg(args, 0, 0), 0)
            return None

        if method_name == "toGuiUpdateWantSpeakerCount":
            self.last_speaker_want_count = self._safe_int(self._arg(args, 0, 0), 0)
            return None

        if method_name == "toGuiAdminAvail":
            self.last_admin_available = bool(args[0]) if args else False
            return None

        if method_name == "toGuiMediaAction":
            return bool(args)

        if method_name == "toGuiMediaError":
            self.last_media_error = args
            self.logger.warning("IToGui media error: %s", args)
            return None

        if method_name == "toGuiModuleState":
            self.last_module_state = args
            return None

        if method_name == "toGuiWantVideoCapture":
            self.last_want_video_capture = args
            return None

        if method_name == "toGuiPlayJpgVideo":
            self.last_play_jpg_video = args
            return None

        if method_name == "toGuiPlayNlcMedia":
            self.last_play_nlc_media = args
            return None

        if method_name == "toGuiIndentListUpdate":
            self.last_indent_list_update = args
            return None

        if method_name == "toGuiIndentListRemove":
            self.last_indent_list_remove = args
            return None

        if method_name == "toGuiContactAdded":
            self.last_contact_added = args
            return None

        if method_name == "toGuiContactRemoved":
            self.last_contact_removed = args
            return None

        if method_name == "toGuiContactOnline":
            self.last_contact_online = args
            return None

        if method_name == "toGuiContactAnythingChange":
            self.last_contact_anything_change = args
            return None

        if method_name == "toGuiContactLastSessionTimeChange":
            self.last_contact_last_session_time_change = args
            return None

        if method_name == "toGuiUpdateMyIdent":
            self.last_update_my_ident = args
            return None

        if method_name == "toGuiSaveMyIdent":
            self.last_save_my_ident = args
            return None

        if method_name == "toGuiRxedPluginOffer":
            self.last_rxed_plugin_offer = args
            return None

        if method_name == "toGuiRxedOfferReply":
            self.last_rxed_offer_reply = args
            return None

        if method_name == "toGuiPluginSessionStarted":
            self.last_plugin_session_started = args
            return None

        if method_name == "toGuiPluginSessionEnded":
            self.last_plugin_session_ended = args
            return None

        if method_name == "toGuiFileListReply":
            self.last_file_list_reply = args
            return None

        if method_name == "toGuiFileList":
            self.last_file_list = args
            return None

        if method_name == "toGuiFileListCompleted":
            self.last_file_list_completed = args
            return None

        if method_name == "toGuiFileUploadStart":
            self.last_file_upload_start = args
            return None

        if method_name == "toGuiFileUploadComplete":
            self.last_file_upload_complete = args
            return None

        if method_name == "toGuiFileDownloadStart":
            self.last_file_download_start = args
            return None

        if method_name == "toGuiFileDownloadComplete":
            self.last_file_download_complete = args
            return None

        if method_name == "toGuiFileDeleted":
            self.last_file_deleted = args
            return None

        if method_name == "toGuiAssetAdded":
            self.last_asset_added = args
            return None

        if method_name == "toGuiAssetUpdated":
            self.last_asset_updated = args
            return None

        if method_name == "toGuiAssetRemoved":
            self.last_asset_removed = args
            return None

        if method_name == "toGuiAssetXferState":
            self.last_asset_xfer_state = args
            return None

        if method_name == "toGuiAssetSessionHistory":
            self.last_asset_session_history = args
            return None

        if method_name == "toGuiAssetAction":
            self.last_asset_action = args
            return None

        if method_name == "toGuiMultiSessionAction":
            self.last_multi_session_action = args
            return None

        if method_name == "toGuiBlobAdded":
            self.last_blob_added = args
            return None

        if method_name == "toGuiBlobAction":
            self.last_blob_action = args
            return None

        if method_name == "toGuiBlobSessionHistory":
            self.last_blob_session_history = args
            return None

        if method_name == "toGuiTodGameAction":
            self.last_tod_game_action = args
            return None

        if method_name == "toGuiInstMsg":
            self.last_inst_msg = args
            return None

        if method_name == "toGuiSearchResultFileSearch":
            self.last_search_result_file_search = args
            return None

        return spec.return_default


def _install_contract_methods(contract_cls: type[GuiInterfaceContractBase]) -> None:
    for method_name in contract_cls._METHODS:
        if hasattr(contract_cls, method_name):
            continue

        def _method(self: GuiInterfaceContractBase, *args: Any, _m: str = method_name, **kwargs: Any) -> Any:
            return self._handle_stub_call(_m, args, kwargs)

        setattr(contract_cls, method_name, _method)


_install_contract_methods(IFromGuiContractStub)
_install_contract_methods(IToGuiContractStub)


class IToGuiEventSink(IToGuiContractStub):
    """Python callback sink for engine->GUI style notifications.

    This is intentionally generic: callback keys are method names from IToGui.
    It provides immediate integration value before full native callback wiring.
    """

    def __init__(self) -> None:
        super().__init__()
        self._handlers: dict[str, list[Any]] = {}

    def register_handler(self, method_name: str, handler: Any) -> None:
        if not self.supports(method_name):
            raise ValueError(f"Unsupported IToGui method: {method_name}")
        self._handlers.setdefault(method_name, []).append(handler)

    def clear_handlers(self, method_name: str | None = None) -> None:
        if method_name is None:
            self._handlers.clear()
            return
        self._handlers.pop(method_name, None)

    def _handle_stub_call(self, method_name: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> Any:
        ret = super()._handle_stub_call(method_name, args, kwargs)
        for handler in self._handlers.get(method_name, []):
            try:
                handler(*args, **kwargs)
            except Exception:
                self.logger.exception("IToGui handler failed for %s", method_name)
        return ret
