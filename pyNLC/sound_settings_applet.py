from __future__ import annotations

from datetime import datetime

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QWidget

from resources.Forms.AppletSoundSettings_ui import Ui_AppletSoundSettingsUi


class SoundSettingsApplet(QWidget):
    """Python translation of AppletSoundSettings with placeholder audio controls."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletSoundSettingsUi()
        self.ui.setupUi(self)
        self.settings = settings

        self._tick = 0
        self._meter_timer = QTimer(self)
        self._meter_timer.setInterval(250)
        self._meter_timer.timeout.connect(self._update_levels)

        self._init_controls()
        self._wire_controls()
        self._restore_settings()

    def _store(self) -> dict:
        if self.settings is None:
            return {}
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            return store
        return {}

    def _init_controls(self) -> None:
        self.ui.m_InDeviceComboBox.clear()
        self.ui.m_InDeviceComboBox.addItems(["Default Mic", "System Mic", "USB Mic"])
        self.ui.m_OutDeviceComboBox.clear()
        self.ui.m_OutDeviceComboBox.addItems(["Default Output", "Speakers", "Headphones"])

        self.ui.m_TestFileComboBox.clear()
        self.ui.m_TestFileComboBox.addItems(["tone-400hz.wav", "speech-sample.wav", "music-loop.wav"])

        self.ui.m_TestDelayResultLineEdit.setText("0")
        self.ui.m_EchoDelayLineEdit.setText("120")
        self.ui.m_AudioInPeakProgressBar.setValue(0)

        self.ui.m_ShowInWaveFormCheckBox.setChecked(True)
        self.ui.m_ShowOutWaveFormCheckBox.setChecked(True)
        self.ui.m_ShowSoundInCheckBox.setChecked(True)
        self.ui.m_ShowSoundOutCheckBox.setChecked(True)
        self.ui.m_ShowLogCheckBox.setChecked(True)

        self.ui.m_StatusMsgLabel.setText("Sound settings ready")
        self.ui.m_LogWidget.addLogLine("Sound settings initialized")

    def _wire_controls(self) -> None:
        self.ui.m_ShowInWaveFormCheckBox.toggled.connect(self.ui.m_AudioInWaveFormFrame.setVisible)
        self.ui.m_ShowOutWaveFormCheckBox.toggled.connect(self.ui.m_AudioOutWaveFormFrame.setVisible)
        self.ui.m_ShowSoundInCheckBox.toggled.connect(self.ui.m_InSettingsGroupBox.setVisible)
        self.ui.m_ShowSoundOutCheckBox.toggled.connect(self.ui.m_OutSettingsGroupBox.setVisible)
        self.ui.m_ShowLogCheckBox.toggled.connect(self.ui.m_LogWidget.setVisible)

        self.ui.m_NoAecLoopbackCheckBox.toggled.connect(lambda value: self._set_status(f"No AEC loopback: {'on' if value else 'off'}"))
        self.ui.m_WithAecLoopbackCheckBox.toggled.connect(lambda value: self._set_status(f"With AEC loopback: {'on' if value else 'off'}"))

        self.ui.m_ApplyDefaultInDeviceButton.clicked.connect(self._apply_input_device)
        self.ui.m_ApplyDefaultOutDeviceButton.clicked.connect(self._apply_output_device)
        self.ui.m_TestSoundDelayButton.clicked.connect(self._test_delay)
        self.ui.m_EchoDelaySaveButton.clicked.connect(self._save_echo_delay)
        self.ui.m_PlayTestFileButton.clicked.connect(self._play_test_file)
        self.ui.m_GenerateToneCheckBox.toggled.connect(self._toggle_tone)

        self.ui.m_ShowInWaveFormCheckBox.toggled.connect(self._toggle_meter_timer)
        self.ui.m_ShowOutWaveFormCheckBox.toggled.connect(self._toggle_meter_timer)
        self._toggle_meter_timer()

    def _restore_settings(self) -> None:
        store = self._store()
        in_idx = int(store.get("ui.sound.in_device_idx", 0))
        out_idx = int(store.get("ui.sound.out_device_idx", 0))
        self.ui.m_InDeviceComboBox.setCurrentIndex(max(0, min(in_idx, self.ui.m_InDeviceComboBox.count() - 1)))
        self.ui.m_OutDeviceComboBox.setCurrentIndex(max(0, min(out_idx, self.ui.m_OutDeviceComboBox.count() - 1)))
        self.ui.m_EchoDelayLineEdit.setText(str(store.get("ui.sound.echo_delay_ms", "120")))

    def _set_status(self, text: str) -> None:
        self.ui.m_StatusMsgLabel.setText(text)
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_LogWidget.addLogLine(f"[{stamp}] {text}")

    def _apply_input_device(self) -> None:
        idx = self.ui.m_InDeviceComboBox.currentIndex()
        self._store()["ui.sound.in_device_idx"] = idx
        self._set_status(f"Input device applied: {self.ui.m_InDeviceComboBox.currentText()}")

    def _apply_output_device(self) -> None:
        idx = self.ui.m_OutDeviceComboBox.currentIndex()
        self._store()["ui.sound.out_device_idx"] = idx
        self._set_status(f"Output device applied: {self.ui.m_OutDeviceComboBox.currentText()}")

    def _test_delay(self) -> None:
        test_value = 80 + (self._tick % 40)
        self.ui.m_TestDelayResultLineEdit.setText(str(test_value))
        self._set_status(f"Measured test delay: {test_value} ms")

    def _save_echo_delay(self) -> None:
        value = self.ui.m_EchoDelayLineEdit.text().strip() or "120"
        self._store()["ui.sound.echo_delay_ms"] = value
        self._set_status(f"Echo delay saved: {value} ms")

    def _play_test_file(self) -> None:
        self._set_status(f"Play test file: {self.ui.m_TestFileComboBox.currentText()} (backend pending)")

    def _toggle_tone(self, enabled: bool) -> None:
        self._set_status("400Hz tone enabled" if enabled else "400Hz tone disabled")

    def _toggle_meter_timer(self, _unused: bool | None = None) -> None:
        if self.ui.m_ShowInWaveFormCheckBox.isChecked() or self.ui.m_ShowOutWaveFormCheckBox.isChecked():
            if not self._meter_timer.isActive():
                self._meter_timer.start()
        else:
            self._meter_timer.stop()

    def _update_levels(self) -> None:
        self._tick = (self._tick + 7) % 100
        in_value = self._tick / 100.0
        out_value = ((self._tick + 35) % 100) / 100.0

        self.ui.m_AudioInWaveFormFrame.setLevel(in_value)
        self.ui.m_AudioOutWaveFormFrame.setLevel(out_value)
        self.ui.m_AudioInPeakProgressBar.setValue(int(in_value * 100))
