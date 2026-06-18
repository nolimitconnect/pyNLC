"""Account management and login applet for first-run experience."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMessageBox,
)


@dataclass
class AccountInfo:
    """Simple account metadata."""

    name: str
    user_id: str = ""
    last_login: str = ""


class AccountSelector(QWidget):
    """Account selection/creation widget for login flow."""

    account_selected = Signal(str)  # Emits account name
    login_completed = Signal()

    def __init__(self, app_data_dir: Path, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.app_data_dir = app_data_dir
        self.accounts: list[AccountInfo] = []
        self._load_accounts()

        layout = QVBoxLayout(self)

        title = QLabel("Select Account")
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        layout.addWidget(title)

        desc = QLabel("Choose an existing account or create a new one to continue.")
        desc.setWordWrap(True)
        layout.addWidget(desc)

        # Account list
        layout.addWidget(QLabel("Available Accounts:", self))
        self.account_list = QListWidget(self)
        self.account_list.itemClicked.connect(self._on_account_selected)
        self._refresh_account_list()
        layout.addWidget(self.account_list)

        # Buttons
        button_layout = QHBoxLayout()

        login_btn = QPushButton("Login to Selected Account", self)
        login_btn.clicked.connect(self._on_login_clicked)
        button_layout.addWidget(login_btn)

        new_acct_btn = QPushButton("Create New Account", self)
        new_acct_btn.clicked.connect(self._on_create_account)
        button_layout.addWidget(new_acct_btn)

        layout.addLayout(button_layout)
        layout.addStretch(1)

        self.setStyleSheet(
            "AccountSelector { background-color: #2b2b2b; color: #ffffff; }\n"
            "QLineEdit { background-color: #3d3d3d; color: #ffffff; border: 1px solid #555555; padding: 2px; }\n"
            "QPushButton { background-color: #3d3d3d; color: #ffffff; padding: 4px; border-radius: 2px; }\n"
            "QPushButton:pressed { background-color: #4d4d4d; }\n"
            "QListWidget { background-color: #1e1e1e; color: #ffffff; border: 1px solid #555555; }"
        )

    def _load_accounts(self) -> None:
        """Load accounts from app data directory."""
        accounts_dir = self.app_data_dir / "accounts"
        if accounts_dir.exists():
            for account_dir in accounts_dir.iterdir():
                if account_dir.is_dir():
                    info_file = account_dir / "info.txt"
                    if info_file.exists():
                        account_name = account_dir.name
                        self.accounts.append(
                            AccountInfo(
                                name=account_name,
                                user_id=account_dir.name,
                                last_login="(not yet)",
                            )
                        )

    def _refresh_account_list(self) -> None:
        """Refresh the account list widget."""
        self.account_list.clear()
        for account in self.accounts:
            item = QListWidgetItem(account.name)
            item.setData(Qt.UserRole, account.user_id)
            self.account_list.addItem(item)

        if not self.accounts:
            self.account_list.addItem("(No accounts yet)")

    def _on_account_selected(self, item: QListWidgetItem) -> None:
        """Handle account selection."""
        account_name = item.text()
        if account_name != "(No accounts yet)":
            self.account_selected.emit(account_name)

    def _on_login_clicked(self) -> None:
        """Handle login button click."""
        selected_items = self.account_list.selectedItems()
        if selected_items and selected_items[0].text() != "(No accounts yet)":
            account_name = selected_items[0].text()
            self.account_selected.emit(account_name)
            self.login_completed.emit()
        else:
            QMessageBox.warning(self, "No Account Selected", "Please select an account to log in.")

    def _on_create_account(self) -> None:
        """Handle create new account."""
        # Simple stub: just ask for account name
        from PySide6.QtWidgets import QInputDialog

        name, ok = QInputDialog.getText(self, "Create Account", "Enter account name:", QLineEdit.Normal, "")
        if ok and name.strip():
            # Create account directory
            accounts_dir = self.app_data_dir / "accounts"
            accounts_dir.mkdir(parents=True, exist_ok=True)

            account_dir = accounts_dir / name
            account_dir.mkdir(exist_ok=True)

            # Write stub info file
            info_file = account_dir / "info.txt"
            info_file.write_text(f"Account: {name}\nCreated: new\n")

            # Add to list
            self.accounts.append(AccountInfo(name=name, user_id=name, last_login="just now"))
            self._refresh_account_list()

            QMessageBox.information(self, "Account Created", f"Account '{name}' created successfully.")

    def get_selected_account(self) -> str | None:
        """Get currently selected account name."""
        selected_items = self.account_list.selectedItems()
        if selected_items and selected_items[0].text() != "(No accounts yet)":
            return selected_items[0].text()
        return None
