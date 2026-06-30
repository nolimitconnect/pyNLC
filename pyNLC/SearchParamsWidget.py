from __future__ import annotations

from PySide6.QtWidgets import QFormLayout, QLineEdit, QWidget


class SearchParamsWidget(QWidget):
    """Compatibility search parameter editor shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QFormLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._query = QLineEdit(self)
        self._tag = QLineEdit(self)
        layout.addRow("Query", self._query)
        layout.addRow("Tag", self._tag)
