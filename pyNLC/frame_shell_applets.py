from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget


class _BaseFrameApplet(QWidget):
    def __init__(self, title: str, key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._key = key

        self.setWindowTitle(title)
        layout = QVBoxLayout(self)
        self._title = QLabel(title, self)
        self._status = QLabel("Initialized", self)
        layout.addWidget(self._title)
        layout.addWidget(self._status)

        self._touch()

    def _touch(self) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self._status.setText(f"Frame ready @ {stamp}")
        if self._settings is not None:
            store = getattr(self._settings, "_settings_store", None)
            if isinstance(store, dict):
                store[self._key] = stamp


class HomeFrameApplet(_BaseFrameApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Home Frame", "ui.frame.home", settings, parent)


class MessengerFrameApplet(_BaseFrameApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__("Messenger Frame", "ui.frame.messenger", settings, parent)
