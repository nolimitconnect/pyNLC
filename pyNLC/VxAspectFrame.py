from __future__ import annotations

from PySide6.QtWidgets import QFrame, QWidget


class VxAspectFrame(QFrame):
    """Compatibility frame that keeps the first child at a fixed aspect ratio."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._aspect_w = 4
        self._aspect_h = 3

    def setAspectRatio(self, width: int, height: int) -> None:
        if width <= 0 or height <= 0:
            return
        self._aspect_w = int(width)
        self._aspect_h = int(height)
        self._layout_primary_child()

    def resizeEvent(self, event) -> None:  # type: ignore[override]
        super().resizeEvent(event)
        self._layout_primary_child()

    def _layout_primary_child(self) -> None:
        widgets = [child for child in self.findChildren(QWidget) if child.parent() is self]
        primary = widgets[0] if widgets else None

        if primary is None:
            return

        frame_w = max(1, self.width() - 2)
        frame_h = max(1, self.height() - 2)
        target_w = frame_w
        target_h = int(target_w * self._aspect_h / self._aspect_w)
        if target_h > frame_h:
            target_h = frame_h
            target_w = int(target_h * self._aspect_w / self._aspect_h)

        x = (self.width() - target_w) // 2
        y = (self.height() - target_h) // 2
        primary.setGeometry(x, y, target_w, target_h)
