from __future__ import annotations

from PySide6.QtWidgets import QComboBox, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QVBoxLayout, QWidget


class LibraryApplet(QWidget):
    """Concrete non-media library applet shell for migrated home flow."""

    _SAMPLE_ITEMS = [
        ("audio", "song-demo.mp3"),
        ("video", "clip-demo.mp4"),
        ("image", "photo-demo.jpg"),
        ("doc", "notes-demo.txt"),
    ]

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings

        root = QVBoxLayout(self)

        self._status = QLabel("Library ready", self)
        root.addWidget(self._status)

        selectors = QHBoxLayout()
        self._media_type = QComboBox(self)
        self._media_type.addItem("All", "all")
        self._media_type.addItem("Audio", "audio")
        self._media_type.addItem("Video", "video")
        self._media_type.addItem("Image", "image")
        self._media_type.addItem("Document", "doc")
        self._media_type.currentIndexChanged.connect(self._refresh_items)
        selectors.addWidget(self._media_type)

        self._filter = QComboBox(self)
        self._filter.addItem("Everything", "all")
        self._filter.addItem("Shared", "shared")
        self._filter.addItem("Local", "local")
        self._filter.currentIndexChanged.connect(self._refresh_items)
        selectors.addWidget(self._filter)
        root.addLayout(selectors)

        self._list = QListWidget(self)
        root.addWidget(self._list)

        self._restore_settings()
        self._refresh_items()

    def _store(self) -> dict:
        if self.settings is None:
            return {}
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            return store
        return {}

    def _restore_settings(self) -> None:
        store = self._store()
        media = str(store.get("ui.library.media", "all"))
        flt = str(store.get("ui.library.filter", "all"))

        media_idx = self._media_type.findData(media)
        self._media_type.setCurrentIndex(media_idx if media_idx >= 0 else 0)
        filter_idx = self._filter.findData(flt)
        self._filter.setCurrentIndex(filter_idx if filter_idx >= 0 else 0)

    def _refresh_items(self, _idx: int | None = None) -> None:
        media = str(self._media_type.currentData())
        flt = str(self._filter.currentData())

        store = self._store()
        store["ui.library.media"] = media
        store["ui.library.filter"] = flt

        self._list.clear()
        for item_type, item_name in self._SAMPLE_ITEMS:
            if media != "all" and item_type != media:
                continue
            label = item_name if flt == "all" else f"{item_name} ({flt})"
            self._list.addItem(QListWidgetItem(label))

        self._status.setText(f"Library: {self._list.count()} item(s)")
