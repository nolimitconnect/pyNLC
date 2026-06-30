from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QPushButton, QVBoxLayout, QWidget


class ResidualApplet(QWidget):
    """Concrete local fallback for residual unmapped legacy applets."""

    def __init__(
        self,
        title: str,
        settings_key: str,
        settings=None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self._settings = settings
        self._settings_key = settings_key

        layout = QVBoxLayout(self)
        self._title = QLabel(title, self)
        self._status = QLabel("Ready (local stub)", self)
        self._action = QPushButton("Simulate Action", self)
        self._action.clicked.connect(self._touch)

        layout.addWidget(self._title)
        layout.addWidget(self._status)
        layout.addWidget(self._action)

        self._touch()

    def _touch(self) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self._status.setText(f"Updated @ {stamp}")
        if self._settings is not None:
            store = getattr(self._settings, "_settings_store", None)
            if isinstance(store, dict):
                store[self._settings_key] = stamp
