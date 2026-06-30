from __future__ import annotations

from pathlib import Path

from PySide6.QtWidgets import QDialog, QFileDialog, QPushButton, QVBoxLayout, QWidget

from resources.Forms.AppletPlayerNlc_ui import Ui_AppletPlayerNlcUi


class PlayerNlcApplet(QWidget):
    """Python translation of AppletPlayerNlc using media placeholder shims."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletPlayerNlcUi()
        self.ui.setupUi(self)
        self.settings = settings
        self._last_path = ""
        self._messenger_dialog: QDialog | None = None

        self.ui.m_OpenVideoFileButton.setText("Vid")
        self.ui.m_OpenAudioFileButton.setText("Aud")
        self.ui.m_BrowseButton.setText("Browse")
        self.ui.m_ReplayButton.setText("Replay")

        self._open_messenger_button = QPushButton("Messenger", self)
        self.ui.horizontalLayout.addWidget(self._open_messenger_button)

        self.ui.m_OpenVideoFileButton.clicked.connect(lambda: self._open_file("Video (*.mp4 *.mkv *.avi *.mov);;All Files (*)"))
        self.ui.m_OpenAudioFileButton.clicked.connect(lambda: self._open_file("Audio (*.mp3 *.wav *.flac *.ogg);;All Files (*)"))
        self.ui.m_BrowseButton.clicked.connect(lambda: self._open_file("Media (*.mp3 *.wav *.flac *.ogg *.mp4 *.mkv *.avi *.mov);;All Files (*)"))
        self.ui.m_ReplayButton.clicked.connect(self._replay)
        self._open_messenger_button.clicked.connect(self._open_multi_messenger)

        self.ui.m_FilesComboBox.currentTextChanged.connect(self._select_combo)
        self.ui.m_PlayControlWidget.playRequested.connect(self._play)
        self.ui.m_PlayControlWidget.pauseRequested.connect(self._pause)
        self.ui.m_PlayControlWidget.stopRequested.connect(self._stop)

        self._set_status("No media loaded")

    def _set_status(self, text: str) -> None:
        self.ui.m_LastPlayedFileText.setText(text)
        self.ui.m_RenderWidget.setOverlayText(text)

    def _open_file(self, file_filter: str) -> None:
        start_dir = str(Path(self._last_path).parent) if self._last_path else ""
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Media", start_dir, file_filter)
        if not file_path:
            return

        self._last_path = file_path
        self._remember_path(file_path)
        self._add_recent(file_path)
        self._set_status(f"Loaded: {Path(file_path).name}")

    def _add_recent(self, file_path: str) -> None:
        idx = self.ui.m_FilesComboBox.findText(file_path)
        if idx >= 0:
            self.ui.m_FilesComboBox.removeItem(idx)
        self.ui.m_FilesComboBox.insertItem(0, file_path)
        self.ui.m_FilesComboBox.setCurrentIndex(0)

    def _select_combo(self, file_path: str) -> None:
        if file_path:
            self._last_path = file_path
            self._set_status(f"Selected: {Path(file_path).name}")

    def _play(self) -> None:
        if not self._last_path:
            self._set_status("No media selected")
            return
        self._set_status(f"Playing: {Path(self._last_path).name} (backend pending)")

    def _pause(self) -> None:
        if not self._last_path:
            self._set_status("No media selected")
            return
        self._set_status(f"Paused: {Path(self._last_path).name}")

    def _stop(self) -> None:
        if not self._last_path:
            self._set_status("No media selected")
            return
        self._set_status(f"Stopped: {Path(self._last_path).name}")

    def _replay(self) -> None:
        self._play()

    def _remember_path(self, file_path: str) -> None:
        if self.settings is None:
            return
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            store["ui.player_nlc.last_path"] = file_path

    def _open_multi_messenger(self) -> None:
        from multi_messenger_applet import MultiMessengerApplet

        if self._messenger_dialog is None:
            dialog = QDialog(self)
            dialog.setWindowTitle("Multi Messenger")
            layout = QVBoxLayout(dialog)
            layout.setContentsMargins(4, 4, 4, 4)
            layout.addWidget(MultiMessengerApplet(self.settings, dialog))
            self._messenger_dialog = dialog

        self._messenger_dialog.show()
        self._messenger_dialog.raise_()
        self._messenger_dialog.activateWindow()
