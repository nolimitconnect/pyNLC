from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QWidget

from VxPushButton import VxPushButton


class AcceptCancelWidget(QWidget):
    """Accept/cancel widget used by migrated form classes."""

    signalAccepted = Signal()
    signalCanceled = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)

        self.m_AcceptButton = VxPushButton(self)
        self.m_AcceptButton.setText("Apply")
        self.m_AcceptButton.clicked.connect(self.signalAccepted.emit)
        layout.addWidget(self.m_AcceptButton)

        self.m_CancelButton = VxPushButton(self)
        self.m_CancelButton.setText("Cancel")
        self.m_CancelButton.clicked.connect(self.signalCanceled.emit)
        layout.addWidget(self.m_CancelButton)
