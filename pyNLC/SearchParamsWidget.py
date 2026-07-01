from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QFormLayout, QLineEdit, QWidget


class SearchParamsWidget(QWidget):
    """Compatibility search parameter editor shim."""

    params_changed = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QFormLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._query = QLineEdit(self)
        self._tag = QLineEdit(self)
        self._query.textChanged.connect(self.params_changed.emit)
        self._tag.textChanged.connect(self.params_changed.emit)
        layout.addRow("Query", self._query)
        layout.addRow("Tag", self._tag)

    def query_text(self) -> str:
        return self._query.text().strip()

    def tag_text(self) -> str:
        return self._tag.text().strip()

    def set_query_text(self, value: str) -> None:
        self._query.setText(value)

    def set_tag_text(self, value: str) -> None:
        self._tag.setText(value)
