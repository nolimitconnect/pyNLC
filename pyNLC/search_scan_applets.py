from __future__ import annotations

from datetime import datetime
import re

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget

from SearchParamsWidget import SearchParamsWidget


def _now() -> str:
    return datetime.now().strftime("%H:%M:%S")


class _BaseSearchScanApplet(QWidget):
    def __init__(self, title: str, scope: str, settings_key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._scope = scope
        self._settings_key = settings_key
        self._settings = settings

        self.setWindowTitle(title)
        layout = QVBoxLayout(self)

        self._status = QLabel(f"{scope} ready", self)
        self._params = SearchParamsWidget(self)
        self._results = QListWidget(self)
        self._results.itemDoubleClicked.connect(self._toggle_selected_favorite)
        self._params.params_changed.connect(self.refresh_results)

        actions = QHBoxLayout()
        self._refresh = QPushButton("Refresh", self)
        self._refresh.clicked.connect(self.refresh_results)
        self._clear = QPushButton("Clear Params", self)
        self._clear.clicked.connect(self._clear_params)
        self._favorite = QPushButton("Toggle Favorite", self)
        self._favorite.clicked.connect(self._toggle_selected_favorite)
        actions.addWidget(self._refresh)
        actions.addWidget(self._clear)
        actions.addWidget(self._favorite)

        layout.addWidget(self._status)
        layout.addWidget(self._params)
        layout.addLayout(actions)
        layout.addWidget(self._results)

        self._load_params()
        self.refresh_results()

    def _account_scope(self) -> str:
        if self._settings is not None and hasattr(self._settings, "getLastLogin"):
            try:
                account = str(self._settings.getLastLogin()).strip()
                if account:
                    return account
            except Exception:
                pass
        return "anonymous"

    def _state_key(self, suffix: str) -> str:
        account_key = re.sub(r"[^a-zA-Z0-9_.-]", "_", self._account_scope())
        return f"{self._settings_key}.{account_key}.{suffix}"

    def _load_params(self) -> None:
        if self._settings is None:
            return
        store = getattr(self._settings, "_settings_store", None)
        if not isinstance(store, dict):
            return
        self._params.set_query_text(str(store.get(self._state_key("query"), "")))
        self._params.set_tag_text(str(store.get(self._state_key("tag"), "")))

    def _save_state(self, key: str, value) -> None:
        if self._settings is None:
            return
        store = getattr(self._settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value
            save_fn = getattr(self._settings, "_save_settings", None)
            if callable(save_fn):
                try:
                    save_fn()
                except Exception:
                    pass

    @staticmethod
    def _make_online_id(text: str) -> str:
        normalized = re.sub(r"[^a-zA-Z0-9]+", "_", text.lower()).strip("_")
        return normalized or "result"

    def _is_favorite(self, online_id: str) -> bool:
        if self._settings is not None and hasattr(self._settings, "getIsFavorite"):
            try:
                return bool(self._settings.getIsFavorite(online_id))
            except Exception:
                return False
        return False

    def _toggle_selected_favorite(self, *_args) -> None:
        selected = self._results.currentItem()
        if selected is None:
            self._status.setText("Select a result to toggle favorite")
            return

        online_id = str(selected.data(Qt.UserRole) or "")
        if not online_id or self._settings is None or not hasattr(self._settings, "toggleIsFavorite"):
            self._status.setText("Favorites DB not available")
            return

        try:
            self._settings.toggleIsFavorite(online_id)
        except Exception:
            self._status.setText("Failed to update favorite")
            return

        self.refresh_results()
        new_state = "favorited" if self._is_favorite(online_id) else "unfavorited"
        self._status.setText(f"{online_id} {new_state}")

    def _clear_params(self) -> None:
        self._params.set_query_text("")
        self._params.set_tag_text("")
        self.refresh_results()

    def refresh_results(self) -> None:
        self._results.clear()

        query = self._params.query_text()
        tag = self._params.tag_text()

        candidate_rows: list[str] = []
        for idx in range(1, 11):
            candidate_rows.append(f"{self._scope} result {idx}")

        recent_key = self._state_key("recent")
        recent_values = []
        if self._settings is not None:
            store = getattr(self._settings, "_settings_store", None)
            if isinstance(store, dict):
                raw = store.get(recent_key, [])
                if isinstance(raw, list):
                    recent_values = [str(v) for v in raw]
        for item in reversed(recent_values[:6]):
            if item and item not in candidate_rows:
                candidate_rows.insert(0, item)

        filtered_rows = []
        for row in candidate_rows:
            low = row.lower()
            if query and query.lower() not in low:
                continue
            if tag and tag.lower() not in low:
                continue
            filtered_rows.append(row)

        for row in filtered_rows[:20]:
            online_id = self._make_online_id(f"{self._scope}:{row}")
            prefix = "*" if self._is_favorite(online_id) else "-"
            item = QListWidgetItem(f"{prefix} {row}")
            item.setData(Qt.UserRole, online_id)
            self._results.addItem(item)

        stamp = _now()
        self._status.setText(f"{self._scope} refreshed @ {stamp} ({len(filtered_rows)} result(s))")

        self._save_state(self._state_key("query"), query)
        self._save_state(self._state_key("tag"), tag)
        self._save_state(recent_key, filtered_rows[:10])
        self._save_state(self._state_key("last_refresh"), stamp)

        self._save_state(self._settings_key, stamp)


class SearchPageApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Search Page", "Search", "ui.search.page", settings, parent)


class SearchPersonsApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Search Persons", "Persons", "ui.search.persons", settings, parent)


class SearchMoodApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Search Mood", "Mood", "ui.search.mood", settings, parent)


class ScanAboutMeApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Scan About Me", "About Me", "ui.scan.about_me", settings, parent)


class ScanStoryboardApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Scan Storyboard", "Storyboard", "ui.scan.storyboard", settings, parent)


class ScanSharedFilesApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Scan Shared Files", "Shared Files", "ui.scan.shared_files", settings, parent)


class ScanWebCamApplet(_BaseSearchScanApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Scan WebCam", "WebCam", "ui.scan.webcam", settings, parent)
