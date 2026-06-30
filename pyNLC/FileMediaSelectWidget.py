from __future__ import annotations

from PySide6.QtWidgets import QComboBox


class FileMediaSelectWidget(QComboBox):
    """Compatibility media-type selector shim."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.addItems(["Any", "Photo", "Video", "Voice", "Text"])
