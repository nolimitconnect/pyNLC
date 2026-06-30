from __future__ import annotations

from typing import TYPE_CHECKING

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletUserPreferences_ui import Ui_AppletUserPreferencesUi

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


class UserPreferencesApplet(QWidget):
    """Python translation of nolimitgui AppletUserPreferences."""

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletUserPreferencesUi()
        self.ui.setupUi(self)

        self.settings = settings
        self.ui.m_SavePushButton.clicked.connect(self.slot_save_settings)
        self.ui.m_CancelPushButton.clicked.connect(self.slot_cancel)
        self.ui.m_MaxMessageHistoryInfoButton.clicked.connect(self.slot_max_msg_history_info_button_clicked)
        self.ui.m_MaxMessageHistorySpinBox.valueChanged.connect(self.slot_max_msg_history_value_changed)

        self.update_dlg_from_settings()

    def _settings_bool(self, getter_name: str, default: bool = False) -> bool:
        if self.settings is None:
            return default
        getter = getattr(self.settings, getter_name)
        return bool(getter())

    def _settings_int(self, getter_name: str, default: int) -> int:
        if self.settings is None:
            return default
        getter = getattr(self.settings, getter_name)
        return int(getter())

    def update_dlg_from_settings(self) -> None:
        self.ui.m_UseSystemPlayerCheckBox.setChecked(self._settings_bool("getUseSystemMediaPlayer"))
        self.ui.m_UnattendedHostCheckBox.setChecked(self._settings_bool("getIsAutomatedHost"))
        self.ui.m_AllowJoinMultipleHostsCheckBox.setChecked(self._settings_bool("getAllowJoinMultipleHosts"))
        self.ui.m_MilitaryTimeCheckBox.setChecked(self._settings_bool("getUseMilitaryTime"))
        self.ui.m_DisableSoundEffectsCheckBox.setChecked(self._settings_bool("getDisableAllSoundEffects"))
        self.ui.m_SndDisableTrashCheckBox.setChecked(self._settings_bool("getDisableSndTrash"))
        self.ui.m_SndDisableButtonPressCheckBox.setChecked(self._settings_bool("getDisableSndKeyClick"))
        self.ui.m_SndDisableNotifyCheckBox.setChecked(self._settings_bool("getDisableSndNotify"))
        self.ui.m_SndDisableMessageRxCheckBox.setChecked(self._settings_bool("getDisableSndMsgRx"))

        max_history = self._settings_int("getMaxMessageHistory", 500)
        self.ui.m_MaxMessageHistorySpinBox.setValue(max_history)

    def update_settings_from_dlg(self) -> None:
        if self.settings is None:
            return

        self.settings.setIsAutomatedHost(self.ui.m_UnattendedHostCheckBox.isChecked())
        self.settings.setAllowJoinMultipleHosts(self.ui.m_AllowJoinMultipleHostsCheckBox.isChecked())
        self.settings.setUseSystemMediaPlayer(self.ui.m_UseSystemPlayerCheckBox.isChecked())
        self.settings.setUseMilitaryTime(self.ui.m_MilitaryTimeCheckBox.isChecked())
        self.settings.setDisableAllSoundEffects(self.ui.m_DisableSoundEffectsCheckBox.isChecked())
        self.settings.setDisableSndTrash(self.ui.m_SndDisableTrashCheckBox.isChecked())
        self.settings.setDisableSndKeyClick(self.ui.m_SndDisableButtonPressCheckBox.isChecked())
        self.settings.setDisableSndNotify(self.ui.m_SndDisableNotifyCheckBox.isChecked())
        self.settings.setDisableSndMsgRx(self.ui.m_SndDisableMessageRxCheckBox.isChecked())
        self.settings.setMaxMessageHistory(self.ui.m_MaxMessageHistorySpinBox.value())

    def slot_save_settings(self) -> None:
        self.update_settings_from_dlg()
        self.ui.m_SavePushButton.setText("Saved")

    def slot_cancel(self) -> None:
        self.update_dlg_from_settings()
        self.ui.m_SavePushButton.setText("Save User Settings")

    def slot_max_msg_history_info_button_clicked(self) -> None:
        QMessageBox.information(
            self,
            "Max Message History",
            "Controls how many messages are retained in local history.\n"
            "Higher values use more memory but allow longer chat history.",
        )

    def slot_max_msg_history_value_changed(self, max_history: int) -> None:
        if self.settings is not None:
            self.settings.setMaxMessageHistory(max_history)
        self.ui.m_MaxMessageHistoryLabel.setText(f"Max Message History Retained ({max_history})")
