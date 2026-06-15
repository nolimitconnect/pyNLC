from __future__ import annotations

from dataclasses import dataclass

from PySide6.QtCore import Qt, Signal, Slot
from PySide6.QtWidgets import QFrame, QLabel, QMainWindow, QPushButton, QSplitter, QVBoxLayout, QWidget

from applet_registry import AppletRegistry, EApplet
from account_selector import AccountSelector


@dataclass(frozen=True)
class ShellPaneState:
    launch_visible: bool = True
    messenger_visible: bool = True


class HomeWindow(QMainWindow):
    main_window_resized = Signal()
    main_window_moved = Signal()

    def __init__(self, app_title: str, settings, bridge, app_paths) -> None:
        super().__init__()
        self.settings = settings
        self.bridge = bridge
        self.app_paths = app_paths
        self._pane_state = ShellPaneState()
        self._current_applet_widget: QWidget | None = None
        self._logged_in_account: str | None = None

        self.setObjectName("HomeWindow")
        self.setWindowTitle(app_title)

        # Check if an account is currently logged in
        self._check_logged_in_account()

        # If not logged in, show account selector
        if not self._logged_in_account:
            self._setup_account_selector_ui()
        else:
            self._setup_launcher_ui()

        self.restore_home_window_geometry()
        if self._logged_in_account:
            self._apply_pane_state()
            self._connect_bridge_signals()

    def _check_logged_in_account(self) -> None:
        """Check if an account is currently logged in from settings."""
        # This would be stored in AppSettings in the real implementation
        # For now, we'll check if there's a "currentAccount" in the settings store
        pass  # Stub: always show account selector on first run

    def _setup_account_selector_ui(self) -> None:
        """Setup account selector as main UI."""
        self.account_selector = AccountSelector(self.app_paths.root_app_data_dir, self)
        self.account_selector.account_selected.connect(self._on_account_selected)
        self.account_selector.login_completed.connect(self._on_login_completed)
        self.setCentralWidget(self.account_selector)

    def _setup_launcher_ui(self) -> None:
        """Setup normal launcher UI with applets."""
        self.launch_frame = self._create_frame()
        self.messenger_frame = self._create_frame()
        self.launch_page = self._create_launch_page()
        self.messenger_page = self._create_messenger_page()
        self.launch_frame.layout().addWidget(self.launch_page)
        self.messenger_frame.layout().addWidget(self.messenger_page)

        self.splitter = QSplitter(Qt.Horizontal, self)
        self.splitter.addWidget(self.launch_frame)
        self.splitter.addWidget(self.messenger_frame)
        self.splitter.setChildrenCollapsible(False)

        container = QWidget(self)
        layout = QVBoxLayout(container)
        layout.setContentsMargins(3, 3, 3, 3)
        layout.addWidget(self.splitter)
        self.setCentralWidget(container)

    def _on_account_selected(self, account_name: str) -> None:
        """Handle account selection."""
        self._logged_in_account = account_name

    def _on_login_completed(self) -> None:
        """Handle login completion - switch to launcher UI."""
        if self._logged_in_account:
            self.account_selector.deleteLater()
            self._setup_launcher_ui()
            self._apply_pane_state()
            self._connect_bridge_signals()

    def _create_frame(self) -> QFrame:
        frame = QFrame(self)
        frame.setFrameShape(QFrame.Box)
        frame.setLineWidth(1)
        frame_layout = QVBoxLayout(frame)
        frame_layout.setContentsMargins(8, 8, 8, 8)
        return frame

    def _connect_bridge_signals(self) -> None:
        self.bridge.signals.startup_requested.connect(self.on_startup_requested)
        self.bridge.signals.user_specific_dir_requested.connect(self.on_user_specific_dir_requested)
        self.bridge.signals.user_xfer_dir_requested.connect(self.on_user_xfer_dir_requested)
        self.bridge.signals.shutdown_requested.connect(self.on_shutdown_requested)
        self.bridge.signals.status_message.connect(self.on_status_message)

    def apply_theme(self, theme_id: int) -> None:
        """Apply theme by ID. 0=Dark (default), 1=Light."""
        if theme_id == 1:  # Light theme
            self.setStyleSheet(
                """
                QMainWindow, QDialog, QWidget { background-color: #f5f5f5; color: #000000; }
                QPushButton { background-color: #e0e0e0; color: #000000; padding: 4px; border-radius: 2px; }
                QPushButton:pressed { background-color: #d0d0d0; }
                QLabel { color: #000000; }
                QFrame { background-color: #ffffff; border: 1px solid #d0d0d0; }
                """
            )
        else:  # Dark theme (default)
            self.setStyleSheet(
                """
                QMainWindow, QDialog, QWidget { background-color: #2b2b2b; color: #ffffff; }
                QPushButton { background-color: #3d3d3d; color: #ffffff; padding: 4px; border-radius: 2px; }
                QPushButton:pressed { background-color: #4d4d4d; }
                QLabel { color: #ffffff; }
                QFrame { background-color: #1e1e1e; border: 1px solid #444444; }
                """
            )

    def _create_launch_page(self) -> QWidget:
        page = QWidget(self.launch_frame)
        layout = QVBoxLayout(page)

        title = QLabel("Home Page", page)
        title.setStyleSheet("font-weight: 600;")
        layout.addWidget(title)

        self.status_label = QLabel(
            "First-pass migration shell. Audio, video, and complex engine features are stubbed.",
            page,
        )
        self.status_label.setWordWrap(True)
        layout.addWidget(self.status_label)

        toggle_messenger = QPushButton("Toggle Messenger Pane", page)
        toggle_messenger.clicked.connect(self.toggle_messenger_pane)
        layout.addWidget(toggle_messenger)

        show_launch_only = QPushButton("Launch Pane Only", page)
        show_launch_only.clicked.connect(self.show_launch_only)
        layout.addWidget(show_launch_only)

        show_both = QPushButton("Show Both Panes", page)
        show_both.clicked.connect(self.show_both_panes)
        layout.addWidget(show_both)

        # Applets from registry
        applets_title = QLabel("Available Applets", page)
        applets_title.setStyleSheet("font-weight: 600; margin-top: 12px;")
        layout.addWidget(applets_title)

        applet_buttons_layout = QVBoxLayout()
        self._applet_buttons = {}
        for applet_id, meta in AppletRegistry.get_home_applets():
            btn = QPushButton(meta.name, page)
            btn.clicked.connect(lambda checked=False, aid=applet_id: self._on_applet_launch(aid))
            applet_buttons_layout.addWidget(btn)
            self._applet_buttons[applet_id] = btn

        layout.addLayout(applet_buttons_layout)

        paths = QLabel(
            "\n".join(
                [
                    f"App data: {self.app_paths.app_data_dir}",
                    f"Root app data: {self.app_paths.root_app_data_dir}",
                    f"Xfer dir: {self.app_paths.xfer_dir}",
                    f"Assets dir: {self.app_paths.assets_dir}",
                    f"Translations dir: {self.app_paths.translations_dir}",
                ]
            ),
            page,
        )
        paths.setTextInteractionFlags(Qt.TextSelectableByMouse)
        layout.addWidget(paths)
        layout.addStretch(1)
        return page

    def _on_applet_launch(self, applet_id: int) -> None:
        meta = AppletRegistry.get_applet_metadata(applet_id)
        if meta:
            # Create new applet widget, pass settings if available
            applet_widget = AppletRegistry.create_applet_widget(applet_id, settings=self.settings)

            # Remove current applet widget from messenger frame
            layout = self.messenger_frame.layout()
            if self._current_applet_widget is not None:
                layout.removeWidget(self._current_applet_widget)
                self._current_applet_widget.deleteLater()

            # Add new applet widget to messenger frame
            layout.addWidget(applet_widget)
            self._current_applet_widget = applet_widget

            # Connect applet signals if available
            if hasattr(applet_widget, "theme_changed"):
                applet_widget.theme_changed.connect(self._on_applet_theme_changed)

            # Update status
            self.messenger_status_label.setText(f"Applet '{meta.name}' launched")

    def _on_applet_theme_changed(self, theme_id: int) -> None:
        """Handle theme change from settings applet."""
        self.apply_theme(theme_id)


    def _create_messenger_page(self) -> QWidget:
        page = QWidget(self.messenger_frame)
        layout = QVBoxLayout(page)

        title = QLabel("Messenger", page)
        title.setStyleSheet("font-weight: 600;")
        layout.addWidget(title)

        self.messenger_status_label = QLabel(
            "Messenger shell placeholder. Conversation and media applets will be migrated in later slices.",
            page,
        )
        self.messenger_status_label.setWordWrap(True)
        layout.addWidget(self.messenger_status_label)
        layout.addStretch(1)
        return page

    def _apply_pane_state(self) -> None:
        self.launch_frame.setVisible(self._pane_state.launch_visible)
        self.messenger_frame.setVisible(self._pane_state.messenger_visible)

        if self._pane_state.launch_visible and self._pane_state.messenger_visible:
            self.splitter.setSizes([1, 1])
        elif self._pane_state.launch_visible:
            self.splitter.setSizes([1, 0])
        else:
            self.splitter.setSizes([0, 1])

        self.main_window_resized.emit()

    def toggle_messenger_pane(self) -> None:
        self._pane_state = ShellPaneState(
            launch_visible=True,
            messenger_visible=not self._pane_state.messenger_visible,
        )
        self._apply_pane_state()

    def show_launch_only(self) -> None:
        self._pane_state = ShellPaneState(launch_visible=True, messenger_visible=False)
        self._apply_pane_state()

    def show_both_panes(self) -> None:
        self._pane_state = ShellPaneState(launch_visible=True, messenger_visible=True)
        self._apply_pane_state()

    def restore_home_window_geometry(self) -> None:
        restore_geom = self.settings.value("mainWindowGeometry", b"")
        if restore_geom:
            self.restoreGeometry(restore_geom)
            return

        available_geometry = self.screen().availableGeometry()
        self.resize(max(640, available_geometry.width() // 3), max(480, available_geometry.height() // 2))
        self.move(
            (available_geometry.width() - self.width()) // 2,
            (available_geometry.height() - self.height()) // 2,
        )

    def save_home_window_geometry(self) -> None:
        if not self.isMaximized() and not self.isMinimized():
            self.settings.setValue("mainWindowGeometry", self.saveGeometry())

    @Slot(str, str)
    def on_startup_requested(self, assets_dir: str, root_data_dir: str) -> None:
        self.status_label.setText(f"Engine startup requested\nassets: {assets_dir}\nroot: {root_data_dir}")

    @Slot(str)
    def on_user_specific_dir_requested(self, user_specific_dir: str) -> None:
        self.messenger_status_label.setText(f"User data directory configured: {user_specific_dir}")

    @Slot(str)
    def on_user_xfer_dir_requested(self, user_xfer_dir: str) -> None:
        self.messenger_status_label.setText(f"Transfer directory configured: {user_xfer_dir}")

    @Slot()
    def on_shutdown_requested(self) -> None:
        self.messenger_status_label.setText("Shutdown requested")

    @Slot(str)
    def on_status_message(self, message: str) -> None:
        self.messenger_status_label.setText(message)

    def resizeEvent(self, event) -> None:  # type: ignore[override]
        self.main_window_resized.emit()
        super().resizeEvent(event)

    def moveEvent(self, event) -> None:  # type: ignore[override]
        self.main_window_moved.emit()
        super().moveEvent(event)

    def closeEvent(self, event) -> None:  # type: ignore[override]
        self.save_home_window_geometry()
        super().closeEvent(event)
