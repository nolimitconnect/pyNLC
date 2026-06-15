from __future__ import annotations

import json
from pathlib import Path
from typing import Any


class AppSettingsStub:
    # Simple enum-like constants for theme and language
    THEME_DARK = 0
    THEME_LIGHT = 1
    LANG_ENGLISH = 0

    def __init__(self) -> None:
        self._initialized = False
        self._settings_path: Path | None = None
        self._settings_store: dict[str, Any] = {
            "lastSelectedTheme": self.THEME_DARK,
            "selectedLanguage": self.LANG_ENGLISH,
            "lastAppletLaunched": 0,
        }

    def appSettingStartup(self, dbSettingsFile: str) -> int:
        self._settings_path = Path(dbSettingsFile).with_suffix(".json")
        self._initialized = True
        self._load_settings()
        return 0

    def appSettingShutdown(self) -> None:
        self._save_settings()

    def getIsAppSettingInitialized(self) -> bool:
        return self._initialized

    def getAppShortName(self) -> str:
        return "nolimitconnect"

    def _load_settings(self) -> None:
        if self._settings_path and self._settings_path.exists():
            try:
                with open(self._settings_path, "r") as f:
                    loaded = json.load(f)
                    self._settings_store.update(loaded)
            except (json.JSONDecodeError, IOError):
                pass

    def _save_settings(self) -> None:
        if self._settings_path:
            self._settings_path.parent.mkdir(parents=True, exist_ok=True)
            try:
                with open(self._settings_path, "w") as f:
                    json.dump(self._settings_store, f, indent=2)
            except IOError:
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
            return self._make_setter()
        if name.startswith("get"):
            return self._make_getter(name)
        raise AttributeError(name)

    @staticmethod
    def _make_setter() -> Any:
        def setter(*args: Any, **kwargs: Any) -> None:
            del args, kwargs
            return None

        return setter

    @staticmethod
    def _make_getter(name: str) -> Any:
        default_value = AppSettingsStub._default_value_for(name)

        def getter(*args: Any, **kwargs: Any) -> Any:
            del args, kwargs
            return default_value

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
