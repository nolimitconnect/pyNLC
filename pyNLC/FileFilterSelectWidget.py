from __future__ import annotations

from PySide6.QtWidgets import QComboBox


class FileFilterSelectWidget(QComboBox):
    """Compatibility file filter selector shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.addItems(["All", "Images", "Video", "Audio", "Docs"])
