from __future__ import annotations

from PySide6.QtWidgets import QListWidgetItem, QMessageBox, QWidget

from resources.Forms.AppletUserIdentity_ui import Ui_AppletUserIdentityUi


class UserIdentityApplet(QWidget):
    """Python translation of nolimitgui AppletUserIdentity (non-media subset)."""

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletUserIdentityUi()
        self.ui.setupUi(self)
        self.settings = settings

        self.ui.m_EditAvatarImageButton.setText("Edit")
        self.ui.m_AvatarImageButton.setText("View")
        self.ui.m_EditAboutMeButton.setText("Edit")
        self.ui.m_ViewAboutMeButton.setText("View")
        self.ui.m_EditStoryboardButton.setText("Edit")
        self.ui.m_ViewStoryboardButton.setText("View")

        self._init_lists()
        self._load_settings()
        self._connect_signals()

    def _store(self) -> dict:
        if self.settings is None:
            return {}
        store = getattr(self.settings, "_settings_store", None)
        if isinstance(store, dict):
            return store
        return {}

    def _init_lists(self) -> None:
        self.ui.m_AgeComboBox.clear()
        for age in range(13, 100):
            self.ui.m_AgeComboBox.addItem(str(age), age)

        self.ui.m_LanguageComboBox.clear()
        for label, code in [
            ("English", "en_US"),
            ("German", "de_DE"),
            ("Spanish", "es_ES"),
            ("French", "fr_FR"),
            ("Japanese", "ja_JP"),
            ("Korean", "ko_KR"),
            ("Chinese", "zh_CN"),
        ]:
            self.ui.m_LanguageComboBox.addItem(label, code)

        self.ui.m_GenderComboBox.clear()
        for value in ["Unspecified", "Female", "Male", "Non-Binary"]:
            self.ui.m_GenderComboBox.addItem(value)

        self.ui.m_ContentComboBox.clear()
        for value in ["General", "Teen", "Mature"]:
            self.ui.m_ContentComboBox.addItem(value)

    def _load_settings(self) -> None:
        store = self._store()
        accounts = store.get("ui.identity.accounts", ["default"])
        if not isinstance(accounts, list) or not accounts:
            accounts = ["default"]

        self.ui.m_AccountListWidget.clear()
        self.ui.m_AccountComboBox.clear()
        for name in [str(x) for x in accounts]:
            self.ui.m_AccountListWidget.addItem(QListWidgetItem(name))
            self.ui.m_AccountComboBox.addItem(name)

        current = str(store.get("ui.identity.account", accounts[0]))
        idx = self.ui.m_AccountComboBox.findText(current)
        self.ui.m_AccountComboBox.setCurrentIndex(idx if idx >= 0 else 0)

        self.ui.m_MoodMessageEdit.setText(str(store.get("ui.identity.mood", "Ready to connect")))
        self._set_combo_by_data(self.ui.m_AgeComboBox, int(store.get("ui.identity.age", 21)))
        self._set_combo_by_data(self.ui.m_LanguageComboBox, str(store.get("ui.identity.lang", "en_US")))
        self._set_combo_by_text(self.ui.m_GenderComboBox, str(store.get("ui.identity.gender", "Unspecified")))
        self._set_combo_by_text(self.ui.m_ContentComboBox, str(store.get("ui.identity.content", "General")))

    @staticmethod
    def _set_combo_by_data(combo, value) -> None:
        idx = combo.findData(value)
        combo.setCurrentIndex(idx if idx >= 0 else 0)

    @staticmethod
    def _set_combo_by_text(combo, value: str) -> None:
        idx = combo.findText(value)
        combo.setCurrentIndex(idx if idx >= 0 else 0)

    def _connect_signals(self) -> None:
        self.ui.m_ApplyMoodButton.clicked.connect(self._apply_mood)
        self.ui.m_ApplyAgeButton.clicked.connect(self._apply_age_language)
        self.ui.m_ApplyGenderButton.clicked.connect(self._apply_gender_content)
        self.ui.m_DeleteAccountButton.clicked.connect(self._delete_account)
        self.ui.m_CreateNewAccountButton.clicked.connect(self._create_account)
        self.ui.m_AccountComboBox.currentTextChanged.connect(self._account_selected)

        self.ui.m_EditAvatarImageButton.clicked.connect(lambda: self._pending("Avatar edit"))
        self.ui.m_AvatarImageButton.clicked.connect(lambda: self._pending("Avatar view"))
        self.ui.m_EditAboutMeButton.clicked.connect(lambda: self._pending("About Me edit"))
        self.ui.m_ViewAboutMeButton.clicked.connect(lambda: self._pending("About Me view"))
        self.ui.m_EditStoryboardButton.clicked.connect(lambda: self._pending("Storyboard edit"))
        self.ui.m_ViewStoryboardButton.clicked.connect(lambda: self._pending("Storyboard view"))

    def _apply_mood(self) -> None:
        store = self._store()
        store["ui.identity.mood"] = self.ui.m_MoodMessageEdit.text().strip()

    def _apply_age_language(self) -> None:
        store = self._store()
        store["ui.identity.age"] = int(self.ui.m_AgeComboBox.currentData())
        store["ui.identity.lang"] = str(self.ui.m_LanguageComboBox.currentData())

    def _apply_gender_content(self) -> None:
        store = self._store()
        store["ui.identity.gender"] = self.ui.m_GenderComboBox.currentText()
        store["ui.identity.content"] = self.ui.m_ContentComboBox.currentText()

    def _delete_account(self) -> None:
        row = self.ui.m_AccountListWidget.currentRow()
        if row < 0:
            return
        self.ui.m_AccountListWidget.takeItem(row)
        self.ui.m_AccountComboBox.removeItem(row)
        self._persist_accounts()

    def _create_account(self) -> None:
        next_name = f"user-{self.ui.m_AccountListWidget.count() + 1}"
        self.ui.m_AccountListWidget.addItem(QListWidgetItem(next_name))
        self.ui.m_AccountComboBox.addItem(next_name)
        self.ui.m_AccountComboBox.setCurrentText(next_name)
        self._persist_accounts()

    def _account_selected(self, account_name: str) -> None:
        self._store()["ui.identity.account"] = account_name

    def _persist_accounts(self) -> None:
        names = [self.ui.m_AccountListWidget.item(i).text() for i in range(self.ui.m_AccountListWidget.count())]
        if not names:
            names = ["default"]
            self.ui.m_AccountListWidget.addItem(QListWidgetItem("default"))
            self.ui.m_AccountComboBox.addItem("default")
        self._store()["ui.identity.accounts"] = names

    def _pending(self, area: str) -> None:
        QMessageBox.information(self, area, f"{area} is pending full media/profile migration.")
