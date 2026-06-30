from __future__ import annotations

from PySide6.QtWidgets import QCheckBox, QHBoxLayout, QLabel, QWidget


class PermissionWidget(QWidget):
    """Local permission status row shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._label = QLabel("Permission", self)
        self._check = QCheckBox(self)
        self._check.setChecked(True)
        layout.addWidget(self._label)
        layout.addWidget(self._check)

    def set_label(self, text: str) -> None:
        self._label.setText(text)

    def is_enabled(self) -> bool:
        return self._check.isChecked()
