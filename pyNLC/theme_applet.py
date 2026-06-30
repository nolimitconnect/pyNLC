from __future__ import annotations

from typing import TYPE_CHECKING

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.AppletTheme_ui import Ui_ThemeWidgetClass

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


class ThemeApplet(QWidget):
    """Python translation of nolimitgui AppletTheme."""

    theme_preview_requested = Signal(int)
    theme_accepted = Signal(int)
    theme_canceled = Signal(int)

    THEME_NAMES = {
        0: "Dark",
        1: "Light",
    }

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_ThemeWidgetClass()
        self.ui.setupUi(self)

        self.settings = settings
        self._saved_theme = self._read_saved_theme()
        self._current_theme = self._saved_theme
        self._notify_state = 0

        self._fill_theme_combo()
        self._fill_example_combo()
        self.ui.m_PlainTextEdit.setPlainText("Some Text To Edit")
        self.ui.horizontalSlider.setSingleStep(1)
        self.ui.horizontalSlider.setRange(0, 100000)
        self.ui.m_ExampleProgressBar.setRange(0, 100000)
        self.ui.m_VertProgressBar.setRange(0, 100000)

        self.ui.m_ThemeComboBox.currentIndexChanged.connect(self._on_theme_selection_changed)
        self.ui.m_ExampleCheckBox.stateChanged.connect(self._on_checkbox_clicked)
        self.ui.horizontalSlider.valueChanged.connect(self._on_slider_value_changed)
        self.ui.m_ThemeExampleButton.clicked.connect(self._on_example_button_clicked)

        self.ui.m_AcceptCancelFrame.signalAccepted.connect(self._on_accept)
        self.ui.m_AcceptCancelFrame.signalCanceled.connect(self._on_cancel)

        self.ui.m_StatusLabel.setText("Theme preview ready")

    def _read_saved_theme(self) -> int:
        if self.settings is None:
            return 0

        theme_id = int(self.settings.getLastSelectedTheme())
        if theme_id not in self.THEME_NAMES:
            return 0
        return theme_id

    def _fill_theme_combo(self) -> None:
        for theme_id, theme_name in self.THEME_NAMES.items():
            self.ui.m_ThemeComboBox.addItem(theme_name, theme_id)

        idx = self.ui.m_ThemeComboBox.findData(self._saved_theme)
        self.ui.m_ThemeComboBox.setCurrentIndex(0 if idx < 0 else idx)

    def _fill_example_combo(self) -> None:
        self.ui.m_ExampleComboBox.clear()
        self.ui.m_ExampleComboBox.addItems([
            "Selection 1",
            "Selection 2",
            "Selection 3",
            "Selection 4",
            "Selection 5",
        ])
        self.ui.m_ExampleComboBox.setCurrentIndex(0)

    def _on_theme_selection_changed(self, index: int) -> None:
        theme_id = int(self.ui.m_ThemeComboBox.itemData(index))
        self._current_theme = theme_id
        self.theme_preview_requested.emit(theme_id)
        self.ui.m_StatusLabel.setText(f"Previewing theme: {self.ui.m_ThemeComboBox.currentText()}")

    def _on_checkbox_clicked(self, checked_state: int) -> None:
        self.ui.m_StatusLabel.setText(f"Example checkbox state: {checked_state}")

    def _on_slider_value_changed(self, slider_value: int) -> None:
        self.ui.m_ExampleProgressBar.setValue(slider_value)
        self.ui.m_VertProgressBar.setValue(slider_value)

    def _on_example_button_clicked(self) -> None:
        self._notify_state = (self._notify_state + 1) % 6
        self.ui.m_ThemeExampleButton.setNotifyType(self._notify_state)
        self.ui.m_StatusLabel.setText(f"Example notify state: {self._notify_state}")

    def _on_accept(self) -> None:
        if self.settings is not None:
            self.settings.setLastSelectedTheme(self._current_theme)
        self.theme_accepted.emit(self._current_theme)
        self.ui.m_StatusLabel.setText("Theme applied")

    def _on_cancel(self) -> None:
        self._current_theme = self._saved_theme
        idx = self.ui.m_ThemeComboBox.findData(self._saved_theme)
        if idx >= 0:
            self.ui.m_ThemeComboBox.setCurrentIndex(idx)

        self.theme_canceled.emit(self._saved_theme)
        self.ui.m_StatusLabel.setText("Theme changes canceled")
