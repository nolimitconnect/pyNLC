from __future__ import annotations

from datetime import datetime
import re

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QWidget

from resources.Forms.AppletChatRoomJoinSearch_ui import Ui_AppletChatRoomJoinSearchUi
from resources.Forms.AppletChatRoomListLocalView_ui import Ui_AppletChatRoomListLocalViewUi
from resources.Forms.AppletGroupJoinSearch_ui import Ui_AppletGroupJoinSearchUi
from resources.Forms.AppletGroupListLocalView_ui import Ui_AppletGroupListLocalViewUi
from resources.Forms.AppletRandomConnectJoinSearch_ui import Ui_AppletRandomConnectJoinSearchUi
from resources.Forms.AppletRandomConnectListLocalView_ui import Ui_AppletRandomConnectListLocalViewUi


class _BaseSearchApplet(QWidget):
    _TITLE = "Search"

    def _account_scope(self) -> str:
        settings = getattr(self, "_settings", None)
        if settings is not None and hasattr(settings, "getLastLogin"):
            try:
                account = str(settings.getLastLogin()).strip()
                if account:
                    return account
            except Exception:
                pass
        return "anonymous"

    def _state_key(self, suffix: str) -> str:
        account_key = re.sub(r"[^a-zA-Z0-9_.-]", "_", self._account_scope())
        scope = self._TITLE.lower().replace(" ", "_")
        return f"ui.search_list.{scope}.{account_key}.{suffix}"

    def _save_state(self, key: str, value) -> None:
        settings = getattr(self, "_settings", None)
        if settings is None:
            return
        store = getattr(settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value
            save_fn = getattr(settings, "_save_settings", None)
            if callable(save_fn):
                try:
                    save_fn()
                except Exception:
                    pass

    def _is_favorite(self, online_id: str) -> bool:
        settings = getattr(self, "_settings", None)
        if settings is not None and hasattr(settings, "getIsFavorite"):
            try:
                return bool(settings.getIsFavorite(online_id))
            except Exception:
                return False
        return False

    def _make_online_id(self, scope: str, text: str) -> str:
        normalized = re.sub(r"[^a-zA-Z0-9]+", "_", f"{scope}:{text}".lower()).strip("_")
        return normalized or "result"

    def _toggle_favorite_selected(self, ui, scope: str) -> None:
        selected = ui.m_GuiHostedListWidget.currentItem()
        if selected is None:
            ui.m_StatusLabel.setText(f"{scope} select an entry first")
            return

        online_id = str(selected.data(Qt.UserRole) or "")
        settings = getattr(self, "_settings", None)
        if not online_id or settings is None or not hasattr(settings, "toggleIsFavorite"):
            ui.m_StatusLabel.setText(f"{scope} favorites not available")
            return

        try:
            settings.toggleIsFavorite(online_id)
        except Exception:
            ui.m_StatusLabel.setText(f"{scope} failed to update favorite")
            return

        self._seed(ui, scope)
        state = "favorited" if self._is_favorite(online_id) else "unfavorited"
        ui.m_StatusLabel.setText(f"{scope} {online_id} {state}")

    def _init_search(self, ui, scope: str) -> None:
        ui.m_StatusLabel.setText(f"{scope} search ready")
        ui.m_InfoLabel.setText(f"Local stub {scope.lower()} search results")
        ui.m_GuiHostedListWidget.itemDoubleClicked.connect(lambda _item: self._toggle_favorite_selected(ui, scope))
        self._seed(ui, scope)

    def _seed(self, ui, scope: str) -> None:
        ui.m_GuiHostedListWidget.clear()
        recent: list[str] = []
        settings = getattr(self, "_settings", None)
        if settings is not None:
            store = getattr(settings, "_settings_store", None)
            if isinstance(store, dict):
                raw_recent = store.get(self._state_key("recent"), [])
                if isinstance(raw_recent, list):
                    recent = [str(v) for v in raw_recent]

        rows = [f"{scope} #{idx}" for idx in range(1, 4)]
        for value in reversed(recent[:5]):
            if value and value not in rows:
                rows.insert(0, value)

        for row in rows:
            online_id = self._make_online_id(scope, row)
            prefix = "*" if self._is_favorite(online_id) else "-"
            item_text = f"{prefix} {row}"
            item = ui.m_GuiHostedListWidget.addItem(item_text)
            del item
            last_item = ui.m_GuiHostedListWidget.item(ui.m_GuiHostedListWidget.count() - 1)
            if last_item is not None:
                last_item.setData(Qt.UserRole, online_id)

        self._save_state(self._state_key("recent"), rows[:10])
        self._save_state(self._state_key("last_refresh"), datetime.now().strftime("%H:%M:%S"))


class GroupJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletGroupJoinSearchUi()
        self.ui.setupUi(self)
        self._TITLE = "Group Join Search"
        self._init_search(self.ui, "Group")


class ChatRoomJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletChatRoomJoinSearchUi()
        self.ui.setupUi(self)
        self._TITLE = "Chat Room Join Search"
        self._init_search(self.ui, "Chat Room")


class RandomConnectJoinSearchApplet(_BaseSearchApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletRandomConnectJoinSearchUi()
        self.ui.setupUi(self)
        self._TITLE = "Random Connect Join Search"
        self._init_search(self.ui, "Random Connect")


class _BaseListApplet(QWidget):
    _TITLE = "List"

    def _account_scope(self) -> str:
        settings = getattr(self, "_settings", None)
        if settings is not None and hasattr(settings, "getLastLogin"):
            try:
                account = str(settings.getLastLogin()).strip()
                if account:
                    return account
            except Exception:
                pass
        return "anonymous"

    def _state_key(self, suffix: str) -> str:
        account_key = re.sub(r"[^a-zA-Z0-9_.-]", "_", self._account_scope())
        scope = self._TITLE.lower().replace(" ", "_")
        return f"ui.local_list.{scope}.{account_key}.{suffix}"

    def _save_state(self, key: str, value) -> None:
        settings = getattr(self, "_settings", None)
        if settings is None:
            return
        store = getattr(settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value
            save_fn = getattr(settings, "_save_settings", None)
            if callable(save_fn):
                try:
                    save_fn()
                except Exception:
                    pass

    def _is_favorite(self, online_id: str) -> bool:
        settings = getattr(self, "_settings", None)
        if settings is not None and hasattr(settings, "getIsFavorite"):
            try:
                return bool(settings.getIsFavorite(online_id))
            except Exception:
                return False
        return False

    @staticmethod
    def _make_online_id(scope: str, text: str) -> str:
        normalized = re.sub(r"[^a-zA-Z0-9]+", "_", f"{scope}:{text}".lower()).strip("_")
        return normalized or "host"

    def _toggle_favorite_selected(self, ui, scope: str) -> None:
        selected = ui.m_HostedListWidget.currentItem()
        if selected is None:
            ui.m_StatusLabel.setText(f"{scope} select a host first")
            return

        online_id = str(selected.data(Qt.UserRole) or "")
        settings = getattr(self, "_settings", None)
        if not online_id or settings is None or not hasattr(settings, "toggleIsFavorite"):
            ui.m_StatusLabel.setText(f"{scope} favorites not available")
            return

        try:
            settings.toggleIsFavorite(online_id)
        except Exception:
            ui.m_StatusLabel.setText(f"{scope} failed to update favorite")
            return

        self._refresh(ui, scope)
        state = "favorited" if self._is_favorite(online_id) else "unfavorited"
        ui.m_StatusLabel.setText(f"{scope} {online_id} {state}")

    def _init_list(self, ui, scope: str) -> None:
        ui.m_ListDescriptionLabel.setText(f"{scope} local list")
        ui.m_StatusLabel.setText(f"{scope} list loaded")
        ui.m_InfoLabel.setText(f"Local stub {scope.lower()} host list")
        ui.m_RefreshButton.clicked.connect(lambda: self._refresh(ui, scope))
        ui.m_HostedListWidget.itemDoubleClicked.connect(lambda _item: self._toggle_favorite_selected(ui, scope))
        self._refresh(ui, scope)

    def _refresh(self, ui, scope: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        ui.m_StatusLabel.setText(f"{scope} refreshed @ {stamp}")
        ui.m_HostedListWidget.clear()

        recent: list[str] = []
        settings = getattr(self, "_settings", None)
        if settings is not None:
            store = getattr(settings, "_settings_store", None)
            if isinstance(store, dict):
                raw_recent = store.get(self._state_key("recent"), [])
                if isinstance(raw_recent, list):
                    recent = [str(v) for v in raw_recent]

        rows = [f"{scope} Host {idx}" for idx in range(1, 5)]
        for value in reversed(recent[:6]):
            if value and value not in rows:
                rows.insert(0, value)

        for row in rows[:12]:
            online_id = self._make_online_id(scope, row)
            prefix = "*" if self._is_favorite(online_id) else "-"
            ui.m_HostedListWidget.addItem(f"{prefix} {row}")
            item = ui.m_HostedListWidget.item(ui.m_HostedListWidget.count() - 1)
            if item is not None:
                item.setData(Qt.UserRole, online_id)

        self._save_state(self._state_key("recent"), rows[:10])
        self._save_state(self._state_key("last_refresh"), stamp)


class GroupListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletGroupListLocalViewUi()
        self.ui.setupUi(self)
        self._TITLE = "Group Local View"
        self._init_list(self.ui, "Group")


class ChatRoomListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletChatRoomListLocalViewUi()
        self.ui.setupUi(self)
        self._TITLE = "Chat Room Local View"
        self._init_list(self.ui, "Chat Room")


class RandomConnectListLocalViewApplet(_BaseListApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self.ui = Ui_AppletRandomConnectListLocalViewUi()
        self.ui.setupUi(self)
        self._TITLE = "Random Connect Local View"
        self._init_list(self.ui, "Random Connect")
