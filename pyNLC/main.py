import sys
from PySide6.QtCore import QObject, Signal, Slot
from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget
import nlc_engine

# 1. Create a bridge dispatcher using PySide6 Signals for thread safety
class EngineSignals(QObject):
    status_changed = Signal(int, str)
    message_received = Signal(str, str)

# 2. Inherit from the C++ interface exposed by pybind11
class PythonToGuiReceiver(nlc_engine.IToGui):
    def __init__(self):
        super().__init__()
        self.signals = EngineSignals()

    # Override the C++ virtual methods
    def connection_status(self, status_code: int, message: str):
        # Triggered by C++ thread -> Safely routes to Main GUI thread via signal
        self.signals.status_changed.emit(status_code, message)

    def receive_message(self, sender: str, text: str):
        self.signals.message_received.emit(sender, text)

# 3. Main GUI Window
class MainWindow(QMainWindow):
    def __init__(self, receiver):
        super().__init__()
        self.setWindowTitle("pyNLC - Secure Event Loop")
        
        self.label = QLabel("Waiting for Engine Status...")
        layout = QVBoxLayout()
        layout.addWidget(self.label)
        
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        # Connect the C++ callback signals to Python UI updates (Slots)
        self.receiver = receiver
        self.receiver.signals.status_changed.connect(self.update_status)

    @Slot(int, str)
    def update_status(self, status_code: int, message: str):
        # This executes safely on the main thread
        self.label.setText(self.label.text() + f"\n[{status_code}] {message}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Initialize our event listener
    receiver_instance = PythonToGuiReceiver()
    
    # Pass the receiver callback instance to your core C++ engine initializer
    # (e.g., nlc_core.register_callback_interface(receiver_instance))

    window = MainWindow(receiver_instance)
    window.show()
    sys.exit(app.exec())
