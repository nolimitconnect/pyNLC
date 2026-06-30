from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


class PersonalRecorderApplet(QWidget):
    """Concrete non-media personal recorder shell pending media pipeline migration."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings
        self._is_recording = False

        root = QVBoxLayout(self)
        self._status = QLabel("Recorder idle", self)
        root.addWidget(self._status)

        self._toggle = QPushButton("Start Recording", self)
        self._toggle.clicked.connect(self._toggle_recording)
        root.addWidget(self._toggle)

        self._history = QListWidget(self)
        root.addWidget(self._history)

    def _toggle_recording(self) -> None:
        self._is_recording = not self._is_recording
        stamp = datetime.now().strftime("%H:%M:%S")
        if self._is_recording:
            self._status.setText("Recorder running (media backend pending)")
            self._toggle.setText("Stop Recording")
            self._history.addItem(QListWidgetItem(f"{stamp} started recording"))
        else:
            self._status.setText("Recorder idle")
            self._toggle.setText("Start Recording")
            self._history.addItem(QListWidgetItem(f"{stamp} stopped recording"))
