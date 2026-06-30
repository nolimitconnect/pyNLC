from __future__ import annotations

import json
import logging
import sqlite3
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
