from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget

from resources.Forms.InputFaceWidget_ui import Ui_InputFaceWidgetClass


class InputFaceWidget(QWidget):
    """Compatibility widget for selecting emoji/face reactions."""

    faceSelected = Signal(str)
    canceled = Signal()

    _DEFAULT_FACES = [
        ":)", ":D", ":P", ";)", ":|", ":(", ":O", "<3",
        "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]",
        "[9]", "[10]", "[11]", "[12]", "[13]", "[14]", "[15]", "[16]",
        "[17]", "[18]", "[19]", "[20]", "[21]", "[22]", "[23]", "[24]",
    ]

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.ui = Ui_InputFaceWidgetClass()
        self.ui.setupUi(self)
        self._wire_faces()

        self.ui.m_CancelFaceButton.setText("X")
        self.ui.m_CancelFaceButton.clicked.connect(self.canceled.emit)

    def _wire_faces(self) -> None:
        for idx in range(1, 33):
            label = getattr(self.ui, f"m_FaceLabel1_{idx}", None)
            if label is None:
                continue
            face = self._DEFAULT_FACES[idx - 1]
            label.setText(face)
            label.clicked.connect(lambda _=None, text=face: self.faceSelected.emit(text))
