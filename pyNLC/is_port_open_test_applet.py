from __future__ import annotations

from datetime import datetime

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QWidget

from resources.Forms.AppletIsPortOpenTest_ui import Ui_AppletIsPortOpenTestUi


class IsPortOpenTestApplet(QWidget):
    """Concrete local-only network test applet.

    Simulates test status/log updates without calling engine interfaces.
    """

    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.ui = Ui_AppletIsPortOpenTestUi()
        self.ui.setupUi(self)
        self.settings = settings
        self._step = 0

        self.ui.m_StatusLabel.setText("Idle - local stub test")
        self.ui.m_NodeUrlLabel.setText("node://local-simulated")
        self.ui.m_LogEdit.setReadOnly(True)
        self._append_log("Ready")

        self._timer = QTimer(self)
        self._timer.setInterval(900)
        self._timer.timeout.connect(self._tick)
        self._timer.start()

    def _append_log(self, text: str) -> None:
        stamp = datetime.now().strftime("%H:%M:%S")
        self.ui.m_LogEdit.append(f"[{stamp}] {text}")

    def _tick(self) -> None:
        states = [
            "Resolving endpoint",
            "Probing local port",
            "Waiting for probe response",
            "Port appears open",
            "Test complete",
        ]
        msg = states[self._step % len(states)]
        self.ui.m_StatusLabel.setText(msg)
        self._append_log(msg)
        self._step += 1

        if self._step % len(states) == 0:
            self._timer.stop()
