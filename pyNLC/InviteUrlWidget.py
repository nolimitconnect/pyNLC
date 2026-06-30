from __future__ import annotations

from uuid import uuid4

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InviteUrlWidget_ui import Ui_InviteUrlWidgetUi


class InviteUrlWidget(QWidget):
    """Compatibility invite URL selector/editor widget."""

    inviteTextChanged = Signal(str)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InviteUrlWidgetUi()
        self.ui.setupUi(self)

        self.ui.m_PersonalButton.setText("Gen")
        self.ui.m_GroupButton.setText("Gen")
        self.ui.m_ChatRoomButton.setText("Gen")
        self.ui.m_RandomConnectButton.setText("Gen")
        self.ui.m_NetworkSettingsInfoButton.setText("?")
        self.ui.m_NetworkButton.setText("Gen")

        self.ui.m_PersonalButton.clicked.connect(lambda: self._gen_for("personal", self.ui.m_PersonalUrlEdit))
        self.ui.m_GroupButton.clicked.connect(lambda: self._gen_for("group", self.ui.m_GroupUrlEdit))
        self.ui.m_ChatRoomButton.clicked.connect(lambda: self._gen_for("chat", self.ui.m_ChatRoomUrlEdit))
        self.ui.m_RandomConnectButton.clicked.connect(lambda: self._gen_for("random", self.ui.m_RandomConnectUrlEdit))
        self.ui.m_NetworkButton.clicked.connect(lambda: self._gen_for("network", self.ui.m_InviteTextEdit))

        for box in [
            self.ui.m_PersonalCheckBox,
            self.ui.m_GroupCheckBox,
            self.ui.m_ChatRoomCheckBox,
            self.ui.m_RandomConnectCheckBox,
            self.ui.m_NetworkSettingsCheckBox,
        ]:
            box.toggled.connect(self._rebuild_invite_text)

        for edit in [
            self.ui.m_PersonalUrlEdit,
            self.ui.m_GroupUrlEdit,
            self.ui.m_ChatRoomUrlEdit,
            self.ui.m_RandomConnectUrlEdit,
        ]:
            edit.textChanged.connect(self._rebuild_invite_text)

    def _gen_for(self, prefix: str, target) -> None:
        token = uuid4().hex[:12]
        value = f"nlc://invite/{prefix}/{token}"
        set_text = getattr(target, "setText", None)
        if callable(set_text):
            set_text(value)
        else:
            target.setPlainText(value)
        self._rebuild_invite_text()

    def _rebuild_invite_text(self, _unused: object | None = None) -> None:
        parts: list[str] = []
        if self.ui.m_PersonalCheckBox.isChecked() and self.ui.m_PersonalUrlEdit.text().strip():
            parts.append(self.ui.m_PersonalUrlEdit.text().strip())
        if self.ui.m_GroupCheckBox.isChecked() and self.ui.m_GroupUrlEdit.text().strip():
            parts.append(self.ui.m_GroupUrlEdit.text().strip())
        if self.ui.m_ChatRoomCheckBox.isChecked() and self.ui.m_ChatRoomUrlEdit.text().strip():
            parts.append(self.ui.m_ChatRoomUrlEdit.text().strip())
        if self.ui.m_RandomConnectCheckBox.isChecked() and self.ui.m_RandomConnectUrlEdit.text().strip():
            parts.append(self.ui.m_RandomConnectUrlEdit.text().strip())
        if self.ui.m_NetworkSettingsCheckBox.isChecked():
            parts.append("network-settings=true")

        text = "\n".join(parts)
        self.ui.m_InviteTextEdit.setPlainText(text)
        self.inviteTextChanged.emit(text)

    def invite_text(self) -> str:
        return self.ui.m_InviteTextEdit.toPlainText().strip()

    def set_invite_text(self, text: str) -> None:
        self.ui.m_InviteTextEdit.setPlainText(text or "")
        self.inviteTextChanged.emit(self.invite_text())
