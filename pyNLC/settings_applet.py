"""Settings applet for theme and language configuration."""

from __future__ import annotations

from typing import TYPE_CHECKING

from PySide6.QtCore import Qt, pyqtSignal
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QLabel,
    QComboBox,
    QPushButton,
    QMessageBox,
)

if TYPE_CHECKING:
    from py_wrapper import AppSettingsStub


class SettingsApplet(QWidget):
    """Settings applet for theme and language configuration."""

    theme_changed = pyqtSignal(int)  # Emits theme ID
    language_changed = pyqtSignal(int)  # Emits language ID

    def __init__(self, settings: AppSettingsStub | None = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.settings = settings

        layout = QVBoxLayout(self)

        title = QLabel("Settings")
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        layout.addWidget(title)

        # Theme selection
        layout.addWidget(QLabel("Theme:", self))
        self.theme_combo = QComboBox(self)
        self.theme_combo.addItem("Dark", 0)
        self.theme_combo.addItem("Light", 1)
        if settings:
            theme_id = settings.getLastSelectedTheme()
            self.theme_combo.setCurrentIndex(theme_id if theme_id < self.theme_combo.count() else 0)
        self.theme_combo.currentIndexChanged.connect(self._on_theme_changed)
        layout.addWidget(self.theme_combo)

        # Language selection
        layout.addWidget(QLabel("Language:", self))
        self.lang_combo = QComboBox(self)
        self.lang_combo.addItem("English", 0)
        self.lang_combo.addItem("Spanish", 1)
        self.lang_combo.addItem("French", 2)
        self.lang_combo.addItem("German", 3)
        if settings:
            lang_id = settings.getSelectedLanguage()
            self.lang_combo.setCurrentIndex(lang_id if lang_id < self.lang_combo.count() else 0)
        self.lang_combo.currentIndexChanged.connect(self._on_language_changed)
        layout.addWidget(self.lang_combo)

        # Save button
        save_btn = QPushButton("Save Settings", self)
        save_btn.clicked.connect(self._on_save)
        layout.addWidget(save_btn)

        # Reset button
        reset_btn = QPushButton("Reset to Defaults", self)
        reset_btn.clicked.connect(self._on_reset)
        layout.addWidget(reset_btn)

        # Status label
        self.status_label = QLabel("No changes yet", self)
        self.status_label.setWordWrap(True)
        self.status_label.setStyleSheet("color: #888888; margin-top: 12px;")
        layout.addWidget(self.status_label)

        layout.addStretch(1)

        self.setStyleSheet(
            "SettingsApplet { background-color: #2b2b2b; color: #ffffff; }\n"
            "QLabel { color: #ffffff; }\n"
            "QComboBox { background-color: #3d3d3d; color: #ffffff; border: 1px solid #555555; padding: 2px; }\n"
            "QComboBox QAbstractItemView { background-color: #3d3d3d; color: #ffffff; selection-background-color: #555555; }\n"
            "QPushButton { background-color: #3d3d3d; color: #ffffff; padding: 4px; border-radius: 2px; }\n"
            "QPushButton:pressed { background-color: #4d4d4d; }"
        )

    def _on_theme_changed(self, index: int) -> None:
        """Handle theme selection change."""
        theme_id = self.theme_combo.itemData(index)
        self.theme_changed.emit(theme_id)
        self.status_label.setText(f"Theme changed to '{self.theme_combo.itemText(index)}'")

    def _on_language_changed(self, index: int) -> None:
        """Handle language selection change."""
        lang_id = self.lang_combo.itemData(index)
        self.language_changed.emit(lang_id)
        self.status_label.setText(f"Language changed to '{self.lang_combo.itemText(index)}'")

    def _on_save(self) -> None:
        """Save settings to persistence layer."""
        if self.settings:
            theme_id = self.theme_combo.currentData()
            lang_id = self.lang_combo.currentData()
            self.settings.setLastSelectedTheme(theme_id)
            self.settings.setSelectedLanguage(lang_id)
            self.status_label.setText("Settings saved successfully!")
        else:
            QMessageBox.information(self, "Settings", "Settings saved (stub - no persistence backend)")

    def _on_reset(self) -> None:
        """Reset to default settings."""
        self.theme_combo.setCurrentIndex(0)  # Dark theme
        self.lang_combo.setCurrentIndex(0)  # English
        if self.settings:
            self.settings.setLastSelectedTheme(0)
            self.settings.setSelectedLanguage(0)
        self.status_label.setText("Reset to default settings")
