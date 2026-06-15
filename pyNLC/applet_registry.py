"""Applet registry mapping legacy C++ applets to Python stub applets."""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from PySide6.QtWidgets import QWidget


class EApplet(IntEnum):
    """Mirrors C++ enum EApplet from nolimitgui/src/AppDefs.h."""

    eAppletUnknown = 0
    eAppletActivityDialog = 1
    eAppletEditAvatarImage = 2
    eAppletHomePage = 3
    eAppletGetStarted = 4

    # Home page applets
    eAppletFriendList = 5
    eAppletGroupJoin = 6
    eAppletChatRoomJoin = 7
    eAppletRandomConnectJoin = 8
    eAppletInviteCreate = 9
    eAppletInviteAccept = 10
    eAppletUserIdentity = 11
    eAppletPersonalRecorder = 12
    eAppletLibrary = 13
    eAppletPlayerNlc = 14

    eAppletDownloads = 15
    eAppletUploads = 16
    eAppletShareServicesPage = 17
    eAppletNetHostingPage = 18
    eAppletSettingsPage = 19
    eAppletAboutNoLimitConnect = 20

    eAppletLanguageSelect = 21

    eMaxBasicApplets = 22  # marker

    # Settings applets
    eAppletTheme = 23
    eAppletUserPreferences = 24
    eAppletNetworkSettings = 25
    eAppletSoundSettings = 26
    eAppletCamSettings = 27
    eAppletPermissionList = 28
    eAppletTestAndDebug = 29
    eAppletUserConnections = 30
    eAppletSocketList = 31
    eAppletHackerList = 32

    eMaxSettingsApplets = 33  # marker


@dataclass
class AppletMetadata:
    """Metadata for an applet."""

    applet_id: int
    name: str
    description: str
    category: str  # "home", "settings", "util"
    is_stub: bool = True  # True if stub implementation only


class AppletRegistry:
    """Registry of available applets with metadata."""

    _APPLETS = {
        EApplet.eAppletGetStarted: AppletMetadata(
            applet_id=EApplet.eAppletGetStarted,
            name="Get Started",
            description="Getting started guide for new users",
            category="home",
        ),
        EApplet.eAppletFriendList: AppletMetadata(
            applet_id=EApplet.eAppletFriendList,
            name="Friends",
            description="View and manage your friends",
            category="home",
        ),
        EApplet.eAppletGroupJoin: AppletMetadata(
            applet_id=EApplet.eAppletGroupJoin,
            name="Group Join",
            description="Browse and join group channels",
            category="home",
        ),
        EApplet.eAppletChatRoomJoin: AppletMetadata(
            applet_id=EApplet.eAppletChatRoomJoin,
            name="Chat Rooms",
            description="Browse and join chat rooms",
            category="home",
        ),
        EApplet.eAppletRandomConnectJoin: AppletMetadata(
            applet_id=EApplet.eAppletRandomConnectJoin,
            name="Random Connect",
            description="Connect with random users",
            category="home",
        ),
        EApplet.eAppletInviteCreate: AppletMetadata(
            applet_id=EApplet.eAppletInviteCreate,
            name="Create Invite",
            description="Create an invitation",
            category="home",
        ),
        EApplet.eAppletInviteAccept: AppletMetadata(
            applet_id=EApplet.eAppletInviteAccept,
            name="Accept Invite",
            description="Accept an invitation",
            category="home",
        ),
        EApplet.eAppletUserIdentity: AppletMetadata(
            applet_id=EApplet.eAppletUserIdentity,
            name="User Identity",
            description="Manage your user identity and profile",
            category="home",
        ),
        EApplet.eAppletPersonalRecorder: AppletMetadata(
            applet_id=EApplet.eAppletPersonalRecorder,
            name="Personal Recorder",
            description="Record and playback personal media",
            category="home",
        ),
        EApplet.eAppletLibrary: AppletMetadata(
            applet_id=EApplet.eAppletLibrary,
            name="Library",
            description="Browse your media library",
            category="home",
        ),
        EApplet.eAppletPlayerNlc: AppletMetadata(
            applet_id=EApplet.eAppletPlayerNlc,
            name="Media Player",
            description="Play media files",
            category="home",
        ),
        EApplet.eAppletDownloads: AppletMetadata(
            applet_id=EApplet.eAppletDownloads,
            name="Downloads",
            description="Manage file downloads",
            category="home",
        ),
        EApplet.eAppletUploads: AppletMetadata(
            applet_id=EApplet.eAppletUploads,
            name="Uploads",
            description="Manage file uploads",
            category="home",
        ),
        EApplet.eAppletShareServicesPage: AppletMetadata(
            applet_id=EApplet.eAppletShareServicesPage,
            name="Share Services",
            description="Setup share with others services",
            category="home",
        ),
        EApplet.eAppletNetHostingPage: AppletMetadata(
            applet_id=EApplet.eAppletNetHostingPage,
            name="Network Hosting",
            description="Setup network hosts and services",
            category="home",
        ),
        EApplet.eAppletSettingsPage: AppletMetadata(
            applet_id=EApplet.eAppletSettingsPage,
            name="Settings",
            description="Application settings",
            category="home",
        ),
        EApplet.eAppletAboutNoLimitConnect: AppletMetadata(
            applet_id=EApplet.eAppletAboutNoLimitConnect,
            name="About NoLimitConnect",
            description="About this application",
            category="home",
        ),
        EApplet.eAppletLanguageSelect: AppletMetadata(
            applet_id=EApplet.eAppletLanguageSelect,
            name="Language",
            description="Select application language",
            category="home",
        ),
        EApplet.eAppletTheme: AppletMetadata(
            applet_id=EApplet.eAppletTheme,
            name="Theme",
            description="Customize application theme",
            category="settings",
        ),
        EApplet.eAppletUserPreferences: AppletMetadata(
            applet_id=EApplet.eAppletUserPreferences,
            name="Preferences",
            description="User preferences",
            category="settings",
        ),
        EApplet.eAppletNetworkSettings: AppletMetadata(
            applet_id=EApplet.eAppletNetworkSettings,
            name="Network",
            description="Network settings",
            category="settings",
        ),
        EApplet.eAppletSoundSettings: AppletMetadata(
            applet_id=EApplet.eAppletSoundSettings,
            name="Sound",
            description="Sound and audio settings",
            category="settings",
        ),
        EApplet.eAppletCamSettings: AppletMetadata(
            applet_id=EApplet.eAppletCamSettings,
            name="Camera",
            description="Camera settings",
            category="settings",
        ),
        EApplet.eAppletPermissionList: AppletMetadata(
            applet_id=EApplet.eAppletPermissionList,
            name="Permissions",
            description="Plugin permission levels",
            category="settings",
        ),
        EApplet.eAppletTestAndDebug: AppletMetadata(
            applet_id=EApplet.eAppletTestAndDebug,
            name="Test & Debug",
            description="Testing and debug tools",
            category="settings",
        ),
        EApplet.eAppletUserConnections: AppletMetadata(
            applet_id=EApplet.eAppletUserConnections,
            name="Connections",
            description="Active user connections",
            category="settings",
        ),
        EApplet.eAppletSocketList: AppletMetadata(
            applet_id=EApplet.eAppletSocketList,
            name="Sockets",
            description="Socket list",
            category="settings",
        ),
        EApplet.eAppletHackerList: AppletMetadata(
            applet_id=EApplet.eAppletHackerList,
            name="Hacker List",
            description="Detected hacker list",
            category="settings",
        ),
    }

    @classmethod
    def get_applet_metadata(cls, applet_id: int) -> AppletMetadata | None:
        """Get metadata for an applet by ID."""
        return cls._APPLETS.get(applet_id)

    @classmethod
    def get_home_applets(cls) -> list[tuple[int, AppletMetadata]]:
        """Get all home page applets in order."""
        return [
            (app_id, meta)
            for app_id, meta in cls._APPLETS.items()
            if meta.category == "home" and app_id < EApplet.eMaxBasicApplets
        ]

    @classmethod
    def get_settings_applets(cls) -> list[tuple[int, AppletMetadata]]:
        """Get all settings applets."""
        return [
            (app_id, meta)
            for app_id, meta in cls._APPLETS.items()
            if meta.category == "settings"
        ]

    @classmethod
    def create_applet_widget(cls, applet_id: int, parent: QWidget | None = None, settings: Any = None) -> QWidget:
        """Factory method to create an applet widget (stub implementation)."""
        from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel

        # Special cases for applets with real implementations
        if applet_id == EApplet.eAppletSettingsPage:
            # Return settings applet for Settings
            try:
                from settings_applet import SettingsApplet

                return SettingsApplet(settings, parent)
            except ImportError:
                pass  # Fall through to stub

        meta = cls.get_applet_metadata(applet_id)
        if not meta:
            meta = AppletMetadata(
                applet_id=applet_id,
                name=f"Unknown Applet {applet_id}",
                description="Unknown applet type",
                category="util",
            )

        widget = QWidget(parent)
        layout = QVBoxLayout(widget)

        title = QLabel(meta.name)
        title.setStyleSheet("font-weight: 600; font-size: 14px;")
        layout.addWidget(title)

        description = QLabel(meta.description)
        description.setWordWrap(True)
        layout.addWidget(description)

        content = QLabel(
            f"[Stub Implementation]\n\nApplet: {meta.name}\nID: {applet_id}\nCategory: {meta.category}"
        )
        content.setWordWrap(True)
        content.setStyleSheet("color: #888888; margin-top: 20px;")
        layout.addWidget(content)
        layout.addStretch(1)

        return widget
