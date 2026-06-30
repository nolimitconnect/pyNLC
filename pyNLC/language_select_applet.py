from __future__ import annotations

from PySide6.QtWidgets import QMessageBox, QWidget

from resources.Forms.AppletLanguageSelect_ui import Ui_LanguageSelectUi


class LanguageSelectApplet(QWidget):
    """Python translation of nolimitgui AppletLanguageSelect."""

    LANGUAGES = [
        ("System Default", "system"),
        ("English", "en_US"),
        ("Arabic", "ar_SA"),
        ("German", "de_DE"),
        ("Spanish", "es_ES"),
        ("French", "fr_FR"),
        ("Hindi", "hi_IN"),
        ("Indonesian", "id_ID"),
        ("Japanese", "ja_JP"),
        ("Korean", "ko_KR"),
        ("Portuguese", "pt_PT"),
        ("Russian", "ru_RU"),
        ("Thai", "th_TH"),
        ("Chinese (Simplified)", "zh_CN"),
    ]

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_LanguageSelectUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_LanguageComboBox.clear()
        for label, code in self.LANGUAGES:
            self.ui.m_LanguageComboBox.addItem(label, code)

        selected = self._settings_get("ui.language", "system")
        idx = self.ui.m_LanguageComboBox.findData(selected)
        self.ui.m_LanguageComboBox.setCurrentIndex(idx if idx >= 0 else 0)

        self.ui.m_ApplyButton.clicked.connect(self._apply)

    def _settings_get(self, key: str, default: str) -> str:
        if self.settings is None:
            return default
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            return str(store.get(key, default))
        return default

    def _settings_set(self, key: str, value: str) -> None:
        if self.settings is None:
            return
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value

    def _apply(self) -> None:
        code = str(self.ui.m_LanguageComboBox.currentData())
        label = self.ui.m_LanguageComboBox.currentText()
        self._settings_set("ui.language", code)
        QMessageBox.information(self, "Language", f"Language set to {label}. Restart may be required.")
