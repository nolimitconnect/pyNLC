from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QPushButton, QVBoxLayout, QWidget


class _BaseCoreApplet(QWidget):
    def __init__(self, title: str, key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._title = title
        self._key = key
        self._settings = settings

        self.setWindowTitle(title)
        layout = QVBoxLayout(self)
        self._header = QLabel(title, self)
        self._status = QLabel("Ready", self)
        self._action = QPushButton("Touch", self)
        self._action.clicked.connect(self._touch)

        layout.addWidget(self._header)
        layout.addWidget(self._status)
        layout.addWidget(self._action)

        self._touch()

    def _touch(self) -> None:
        ts = datetime.now().strftime("%H:%M:%S")
        self._status.setText(f"{self._title} active @ {ts}")
        if self._settings is not None:
            store = getattr(self._settings, "_settings_store", None)
            if isinstance(store, dict):
                store[self._key] = ts


class UnknownApplet(_BaseCoreApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Unknown", "ui.core.unknown", settings, parent)


class ActivityDialogApplet(_BaseCoreApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Activity Dialog", "ui.core.activity_dialog", settings, parent)


class EditAvatarImageApplet(_BaseCoreApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Edit Avatar Image", "ui.core.edit_avatar_image", settings, parent)


class HomePageApplet(_BaseCoreApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Home Page", "ui.core.home_page", settings, parent)
