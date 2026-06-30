"""Applet registry mapping legacy C++ applets to Python stub applets."""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import IntEnum
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from PySide6.QtWidgets import QWidget


_EAPPLET_NAMES = [
    "eAppletUnknown",
    "eAppletActivityDialog",
    "eAppletEditAvatarImage",
    "eAppletHomePage",
    "eAppletGetStarted",
    "eAppletFriendList",
    "eAppletGroupJoin",
    "eAppletChatRoomJoin",
    "eAppletRandomConnectJoin",
    "eAppletInviteCreate",
    "eAppletInviteAccept",
    "eAppletUserIdentity",
    "eAppletPersonalRecorder",
    "eAppletLibrary",
    "eAppletPlayerNlc",
    "eAppletDownloads",
    "eAppletUploads",
    "eAppletShareServicesPage",
    "eAppletNetHostingPage",
    "eAppletSettingsPage",
    "eAppletAboutNoLimitConnect",
    "eAppletLanguageSelect",
    "eMaxBasicApplets",
    "eAppletTheme",
    "eAppletUserPreferences",
    "eAppletNetworkSettings",
    "eAppletSoundSettings",
    "eAppletCamSettings",
    "eAppletPermissionList",
    "eAppletTestAndDebug",
    "eAppletUserConnections",
    "eAppletSocketList",
    "eAppletHackerList",
    "eMaxSettingsApplets",
    "eAppletGroupJoinSearch",
    "eAppletChatRoomJoinSearch",
    "eAppletRandomConnectJoinSearch",
    "eAppletSearchPersons",
    "eAppletSearchMood",
    "eAppletScanAboutMe",
    "eAppletScanStoryboard",
    "eAppletScanSharedFiles",
    "eAppletScanWebCam",
    "eMaxSearchApplets",
    "eAppletGroupHostAdmin",
    "eAppletSettingsHostGroup",
    "eAppletHostGroupStatus",
    "eAppletChatRoomHostAdmin",
    "eAppletSettingsHostChatRoom",
    "eAppletHostChatRoomStatus",
    "eAppletRandomConnectHostAdmin",
    "eAppletSettingsHostRandomConnect",
    "eAppletHostRandomConnectStatus",
    "eAppletServiceHostNetwork",
    "eAppletSettingsHostNetwork",
    "eAppletHostNetworkStatus",
    "eAppletChatRoomListLocalView",
    "eAppletGroupListLocalView",
    "eAppletRandomConnectListLocalView",
    "eAppletServiceConnectionTest",
    "eMaxHostApplets",
    "eAppletSearchPage",
    "eAppletServiceAboutMe",
    "eAppletEditAboutMe",
    "eAppletAboutMeServerViewMine",
    "eAppletServiceStoryboard",
    "eAppletEditStoryboard",
    "eAppletStoryboardServerViewMine",
    "eAppletServiceShareWebCam",
    "eAppletCamServerViewMine",
    "eAppletServiceShareFiles",
    "eAppletFileShareServerViewMine",
    "eMaxSharedServicesApplets",
    "eAppletApplicationInfo",
    "eAppletBrowseFiles",
    "eAppletPlayerCamClip",
    "eAppletPlayerPhoto",
    "eAppletPlayerVideo",
    "eAppletAvatarImageClient",
    "eAppletConnectionTestClient",
    "eAppletHostNetworkClient",
    "eAppletGroupClient",
    "eAppletChatRoomClient",
    "eAppletRandomConnectClient",
    "eAppletClientRelay",
    "eAppletClientShareFiles",
    "eAppletCamClient",
    "eAppletChooseThumbnail",
    "eAppletHostJoinConnect",
    "eAppletHostJoinChoose",
    "eAppletHostJoinSearch",
    "eAppletHostLeave",
    "eAppletFriendRequest",
    "eAppletFriendRequestList",
    "eAppletGalleryEmoticon",
    "eAppletGalleryImage",
    "eAppletGalleryThumb",
    "eAppletGroupListClient",
    "eAppletSnapshot",
    "eAppletServiceSettings",
    "eAppletSettingsAboutMe",
    "eAppletSettingsAvatarImage",
    "eAppletSettingsWebCamServer",
    "eAppletSettingsConnectTest",
    "eAppletSettingsShareFiles",
    "eAppletSettingsFileXfer",
    "eAppletSettingsFriendRequest",
    "eAppletSettingsMessenger",
    "eAppletSettingsPushToTalk",
    "eAppletSettingsRandomConnect",
    "eAppletSettingsStoryboard",
    "eAppletSettingsTruthOrDare",
    "eAppletSettingsVideoPhone",
    "eAppletSettingsVoicePhone",
    "ePluginAppletCamProvider",
    "ePluginAppletNlcStation",
    "ePluginAppletNlcNetworkHost",
    "eAppletTestHostClient",
    "eAppletTestHostService",
    "eAppletTestUpnp",
    "eAppletLog",
    "eAppletLogSettings",
    "eAppletCreateAccount",
    "eAppletSnapShot",
    "eActivityBrowseFiles",
    "eAppletMultiMessenger",
    "eAppletPeerChangeFriendship",
    "eAppletPeerReplyOfferFile",
    "eAppletPeerTruthOrDare",
    "eAppletPeerVideoPhone",
    "eAppletPeerVoicePhone",
    "eAppletPeerSessionFileOffer",
    "eAppletHostJoinRequestList",
    "eAppletOfferList",
    "eAppletPersonOfferList",
    "eAppletPopupMenu",
    "eAppletHelpNetSignalBars",
    "eAppletAboutFile",
    "eAppletAboutMeClient",
    "eAppletAboutUser",
    "eAppletFileShareClientView",
    "eAppletStoryboardClient",
    "eAppletFileOfferSelect",
    "eAppletOfferInfo",
    "eAppletOfferResponse",
    "eAppletOfferResponseAccept",
    "eAppletOfferRandSession",
    "eAppletOfferSend",
    "eAppletOfferView",
    "eAppletIgnoredHosts",
    "eAppletIsPortOpenTest",
    "eActivityGenerateHash",
    "eAppletInformation",
    "eAppletHomeFrame",
    "eAppletMessengerFrame",
    "eMaxApplets",
]

EApplet = IntEnum("EApplet", {name: idx for idx, name in enumerate(_EAPPLET_NAMES)})


_MARKER_APPLETS = {
    EApplet.eMaxBasicApplets,
    EApplet.eMaxSettingsApplets,
    EApplet.eMaxSearchApplets,
    EApplet.eMaxHostApplets,
    EApplet.eMaxSharedServicesApplets,
    EApplet.eMaxApplets,
}

_NON_HOME_APPLETS = {
    EApplet.eAppletUnknown,
    EApplet.eAppletActivityDialog,
    EApplet.eAppletEditAvatarImage,
    EApplet.eAppletHomePage,
}


def _humanize_applet_name(enum_name: str) -> str:
    base = re.sub(r"^(eApplet|eActivity|ePluginApplet)", "", enum_name)
    base = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", base)
    return base.strip() or enum_name


def _category_for_applet(applet_id: EApplet) -> str:
    if applet_id in _NON_HOME_APPLETS:
        return "util"
    if applet_id < EApplet.eMaxBasicApplets:
        return "home"
    if applet_id < EApplet.eMaxSettingsApplets:
        return "settings"
    if applet_id < EApplet.eMaxSearchApplets:
        return "search"
    if applet_id < EApplet.eMaxHostApplets:
        return "host"
    if applet_id < EApplet.eMaxSharedServicesApplets:
        return "service"
    return "util"


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

    for _applet in EApplet:
        if _applet in _MARKER_APPLETS:
            continue
        _APPLETS.setdefault(
            _applet,
            AppletMetadata(
                applet_id=_applet,
                name=_humanize_applet_name(_applet.name),
                description=f"Stubbed applet for {_applet.name}",
                category=_category_for_applet(_applet),
                is_stub=True,
            ),
        )

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
        from generic_form_applet import try_create_form_backed_applet

        # Special cases for applets with real implementations
        if applet_id == EApplet.eAppletTheme:
            try:
                from theme_applet import ThemeApplet

                return ThemeApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletUserPreferences:
            try:
                from user_preferences_applet import UserPreferencesApplet

                return UserPreferencesApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletNetworkSettings:
            try:
                from network_settings_applet import NetworkSettingsApplet

                return NetworkSettingsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletCamSettings:
            try:
                from cam_settings_applet import CamSettingsApplet

                return CamSettingsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSoundSettings:
            try:
                from sound_settings_applet import SoundSettingsApplet

                return SoundSettingsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPermissionList:
            try:
                from permission_list_applet import PermissionListApplet

                return PermissionListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletUserConnections:
            try:
                from user_connections_applet import UserConnectionsApplet

                return UserConnectionsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSocketList:
            try:
                from socket_list_applet import SocketListApplet

                return SocketListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHackerList:
            try:
                from hacker_list_applet import HackerListApplet

                return HackerListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletTestAndDebug:
            try:
                from test_and_debug_applet import TestAndDebugApplet

                return TestAndDebugApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsPage:
            # Return settings applet for Settings
            try:
                from settings_applet import SettingsApplet

                return SettingsApplet(settings, parent)
            except ImportError:
                pass  # Fall through to stub

        if applet_id == EApplet.eAppletGetStarted:
            try:
                from get_started_applet import GetStartedApplet

                return GetStartedApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletFriendList:
            try:
                from friend_list_applet import FriendListApplet

                return FriendListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupJoin:
            try:
                from join_search_applets import GroupJoinApplet

                return GroupJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChatRoomJoin:
            try:
                from join_search_applets import ChatRoomJoinApplet

                return ChatRoomJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletRandomConnectJoin:
            try:
                from join_search_applets import RandomConnectJoinApplet

                return RandomConnectJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupListLocalView:
            try:
                from search_and_list_applets import GroupListLocalViewApplet

                return GroupListLocalViewApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChatRoomListLocalView:
            try:
                from search_and_list_applets import ChatRoomListLocalViewApplet

                return ChatRoomListLocalViewApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletRandomConnectListLocalView:
            try:
                from search_and_list_applets import RandomConnectListLocalViewApplet

                return RandomConnectListLocalViewApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupJoinSearch:
            try:
                from join_search_applets import GroupJoinApplet

                return GroupJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChatRoomJoinSearch:
            try:
                from join_search_applets import ChatRoomJoinApplet

                return ChatRoomJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletRandomConnectJoinSearch:
            try:
                from join_search_applets import RandomConnectJoinApplet

                return RandomConnectJoinApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletDownloads:
            try:
                from downloads_applet import DownloadsApplet

                return DownloadsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletUploads:
            try:
                from uploads_applet import UploadsApplet

                return UploadsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletInviteCreate:
            try:
                from invite_create_applet import InviteCreateApplet

                return InviteCreateApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletInviteAccept:
            try:
                from invite_accept_applet import InviteAcceptApplet

                return InviteAcceptApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletLanguageSelect:
            try:
                from language_select_applet import LanguageSelectApplet

                return LanguageSelectApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletAboutNoLimitConnect:
            try:
                from about_app_applet import AboutAppApplet

                return AboutAppApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletAboutFile:
            try:
                from misc_concrete_applets import AboutFileApplet

                return AboutFileApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletAboutMeClient:
            try:
                from misc_concrete_applets import AboutMeClientApplet

                return AboutMeClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletAboutUser:
            try:
                from misc_concrete_applets import AboutUserApplet

                return AboutUserApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHelpNetSignalBars:
            try:
                from misc_concrete_applets import HelpNetSignalBarsApplet

                return HelpNetSignalBarsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletUserIdentity:
            try:
                from user_identity_applet import UserIdentityApplet

                return UserIdentityApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletShareServicesPage:
            try:
                from share_services_applet import ShareServicesApplet

                return ShareServicesApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletNetHostingPage:
            try:
                from net_hosting_applet import NetHostingApplet

                return NetHostingApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceSettings:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Service Settings", "ui.service.settings", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsAboutMe:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: About Me", "ui.settings.about_me", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsAvatarImage:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Avatar Image", "ui.settings.avatar_image", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsWebCamServer:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: WebCam Server", "ui.settings.webcam_server", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsConnectTest:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Connect Test", "ui.settings.connect_test", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsShareFiles:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Share Files", "ui.settings.share_files", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsFileXfer:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: File Transfer", "ui.settings.file_xfer", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsFriendRequest:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Friend Request", "ui.settings.friend_request", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsMessenger:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Messenger", "ui.settings.messenger", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsPushToTalk:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Push To Talk", "ui.settings.push_to_talk", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsRandomConnect:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Random Connect", "ui.settings.random_connect", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsStoryboard:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Storyboard", "ui.settings.storyboard", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsTruthOrDare:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Truth Or Dare", "ui.settings.truth_or_dare", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsVideoPhone:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Video Phone", "ui.settings.video_phone", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsVoicePhone:
            try:
                from service_family_applets import ServiceBaseSettingsApplet

                return ServiceBaseSettingsApplet("Settings: Voice Phone", "ui.settings.voice_phone", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceAboutMe:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: About Me", "ui.service.about_me", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceStoryboard:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: Storyboard", "ui.service.storyboard", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceShareWebCam:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: Share WebCam", "ui.service.share_webcam", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceShareFiles:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: Share Files", "ui.service.share_files", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceConnectionTest:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: Connection Test", "ui.service.connection_test", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletServiceHostNetwork:
            try:
                from service_family_applets import ServiceBaseApplet

                return ServiceBaseApplet("Service: Host Network", "ui.service.host_network", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupHostAdmin:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Group Host Admin", "ui.host.group_admin", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChatRoomHostAdmin:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Chat Room Host Admin", "ui.host.chatroom_admin", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletRandomConnectHostAdmin:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Random Connect Host Admin", "ui.host.random_admin", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsHostGroup:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Host Settings: Group", "ui.host.settings_group", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsHostChatRoom:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Host Settings: Chat Room", "ui.host.settings_chatroom", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsHostRandomConnect:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Host Settings: Random Connect", "ui.host.settings_random", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSettingsHostNetwork:
            try:
                from service_family_applets import HostSettingsBaseApplet

                return HostSettingsBaseApplet("Host Settings: Network", "ui.host.settings_network", settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletLibrary:
            try:
                from library_applet import LibraryApplet

                return LibraryApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletApplicationInfo:
            try:
                from misc_concrete_applets import ApplicationInfoApplet

                return ApplicationInfoApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletBrowseFiles:
            try:
                from misc_concrete_applets import BrowseFilesApplet

                return BrowseFilesApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPersonalRecorder:
            try:
                from personal_recorder_applet import PersonalRecorderApplet

                return PersonalRecorderApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPlayerNlc:
            try:
                from player_nlc_applet import PlayerNlcApplet

                return PlayerNlcApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPlayerCamClip:
            try:
                from misc_concrete_applets import PlayerCamClipApplet

                return PlayerCamClipApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPlayerPhoto:
            try:
                from misc_concrete_applets import PlayerPhotoApplet

                return PlayerPhotoApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPlayerVideo:
            try:
                from misc_concrete_applets import PlayerVideoApplet

                return PlayerVideoApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletCamClient:
            try:
                from cam_client_applet import CamClientApplet

                return CamClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletMultiMessenger:
            try:
                from multi_messenger_applet import MultiMessengerApplet

                return MultiMessengerApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletIsPortOpenTest:
            try:
                from is_port_open_test_applet import IsPortOpenTestApplet

                return IsPortOpenTestApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostJoinConnect:
            try:
                from host_join_connect_applet import HostJoinConnectApplet

                return HostJoinConnectApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChooseThumbnail:
            try:
                from misc_concrete_applets import ChooseThumbnailApplet

                return ChooseThumbnailApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostNetworkClient:
            try:
                from host_client_applets import HostNetworkClientApplet

                return HostNetworkClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupClient:
            try:
                from client_applets import GroupClientApplet

                return GroupClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletChatRoomClient:
            try:
                from client_applets import ChatRoomClientApplet

                return ChatRoomClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletRandomConnectClient:
            try:
                from client_applets import RandomConnectClientApplet

                return RandomConnectClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletTestHostClient:
            try:
                from client_applets import TestHostClientApplet

                return TestHostClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGroupListClient:
            try:
                from client_applets import GroupListClientApplet

                return GroupListClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostJoinChoose:
            try:
                from host_leave_choose_applets import HostJoinChooseApplet

                return HostJoinChooseApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostJoinSearch:
            try:
                from host_client_applets import HostJoinSearchApplet

                return HostJoinSearchApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostLeave:
            try:
                from host_leave_choose_applets import HostLeaveApplet

                return HostLeaveApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletFriendRequest:
            try:
                from misc_concrete_applets import FriendRequestApplet

                return FriendRequestApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletFriendRequestList:
            try:
                from misc_concrete_applets import FriendRequestListApplet

                return FriendRequestListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGalleryEmoticon:
            try:
                from peer_gallery_applets import GalleryEmoticonApplet

                return GalleryEmoticonApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGalleryImage:
            try:
                from peer_gallery_applets import GalleryImageApplet

                return GalleryImageApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletGalleryThumb:
            try:
                from peer_gallery_applets import GalleryThumbApplet

                return GalleryThumbApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostJoinRequestList:
            try:
                from host_status_applets import HostJoinRequestListApplet

                return HostJoinRequestListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletHostNetworkStatus:
            try:
                from host_status_applets import HostNetworkStatusApplet

                return HostNetworkStatusApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletSnapshot:
            try:
                from misc_concrete_applets import SnapshotApplet

                return SnapshotApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletLog:
            try:
                from misc_concrete_applets import LogApplet

                return LogApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletLogSettings:
            try:
                from misc_concrete_applets import LogSettingsApplet

                return LogSettingsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletTestUpnp:
            try:
                from misc_concrete_applets import TestUpnpApplet

                return TestUpnpApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPopupMenu:
            try:
                from misc_concrete_applets import PopupMenuApplet

                return PopupMenuApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletFileShareClientView:
            try:
                from misc_concrete_applets import FileShareClientViewApplet

                return FileShareClientViewApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletStoryboardClient:
            try:
                from misc_concrete_applets import StoryboardClientApplet

                return StoryboardClientApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletFileOfferSelect:
            try:
                from misc_concrete_applets import FileOfferSelectApplet

                return FileOfferSelectApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerChangeFriendship:
            try:
                from peer_gallery_applets import PeerChangeFriendshipApplet

                return PeerChangeFriendshipApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerReplyOfferFile:
            try:
                from peer_gallery_applets import PeerReplyOfferFileApplet

                return PeerReplyOfferFileApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerTruthOrDare:
            try:
                from peer_gallery_applets import PeerTodGameApplet

                return PeerTodGameApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerVideoPhone:
            try:
                from peer_gallery_applets import PeerVideoPhoneApplet

                return PeerVideoPhoneApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerVoicePhone:
            try:
                from peer_gallery_applets import PeerVoicePhoneApplet

                return PeerVoicePhoneApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPeerSessionFileOffer:
            try:
                from peer_gallery_applets import PeerSessionFileOfferApplet

                return PeerSessionFileOfferApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletIgnoredHosts:
            try:
                from misc_concrete_applets import IgnoredHostsApplet

                return IgnoredHostsApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferList:
            try:
                from offer_list_applet import OfferListApplet

                return OfferListApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferInfo:
            try:
                from offer_response_applets import OfferInfoApplet

                return OfferInfoApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferResponse:
            try:
                from offer_response_applets import OfferResponseApplet

                return OfferResponseApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferResponseAccept:
            try:
                from offer_response_applets import OfferResponseAcceptApplet

                return OfferResponseAcceptApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferRandSession:
            try:
                from offer_send_applets import OfferRandSessionApplet

                return OfferRandSessionApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferSend:
            try:
                from offer_send_applets import OfferSendApplet

                return OfferSendApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletOfferView:
            try:
                from offer_send_applets import OfferViewApplet

                return OfferViewApplet(settings, parent)
            except ImportError:
                pass

        if applet_id == EApplet.eAppletPersonOfferList:
            try:
                from offer_list_applet import PersonOfferListApplet

                return PersonOfferListApplet(settings, parent)
            except ImportError:
                pass

        residual_map = {
            EApplet.eAppletUnknown: ("Unknown", "ui.residual.unknown"),
            EApplet.eAppletActivityDialog: ("Activity Dialog", "ui.residual.activity_dialog"),
            EApplet.eAppletEditAvatarImage: ("Edit Avatar Image", "ui.residual.edit_avatar_image"),
            EApplet.eAppletHomePage: ("Home Page", "ui.residual.home_page"),
            EApplet.eAppletSearchPersons: ("Search Persons", "ui.residual.search_persons"),
            EApplet.eAppletSearchMood: ("Search Mood", "ui.residual.search_mood"),
            EApplet.eAppletScanAboutMe: ("Scan About Me", "ui.residual.scan_about_me"),
            EApplet.eAppletScanStoryboard: ("Scan Storyboard", "ui.residual.scan_storyboard"),
            EApplet.eAppletScanSharedFiles: ("Scan Shared Files", "ui.residual.scan_shared_files"),
            EApplet.eAppletScanWebCam: ("Scan WebCam", "ui.residual.scan_webcam"),
            EApplet.eAppletHostGroupStatus: ("Host Group Status", "ui.residual.host_group_status"),
            EApplet.eAppletHostChatRoomStatus: ("Host ChatRoom Status", "ui.residual.host_chatroom_status"),
            EApplet.eAppletHostRandomConnectStatus: (
                "Host Random Connect Status",
                "ui.residual.host_random_connect_status",
            ),
            EApplet.eAppletSearchPage: ("Search Page", "ui.residual.search_page"),
            EApplet.eAppletEditAboutMe: ("Edit About Me", "ui.residual.edit_about_me"),
            EApplet.eAppletAboutMeServerViewMine: (
                "About Me Server View Mine",
                "ui.residual.about_me_server_view_mine",
            ),
            EApplet.eAppletEditStoryboard: ("Edit Storyboard", "ui.residual.edit_storyboard"),
            EApplet.eAppletStoryboardServerViewMine: (
                "Storyboard Server View Mine",
                "ui.residual.storyboard_server_view_mine",
            ),
            EApplet.eAppletCamServerViewMine: ("Cam Server View Mine", "ui.residual.cam_server_view_mine"),
            EApplet.eAppletFileShareServerViewMine: (
                "File Share Server View Mine",
                "ui.residual.file_share_server_view_mine",
            ),
            EApplet.eAppletAvatarImageClient: ("Avatar Image Client", "ui.residual.avatar_image_client"),
            EApplet.eAppletConnectionTestClient: (
                "Connection Test Client",
                "ui.residual.connection_test_client",
            ),
            EApplet.eAppletClientRelay: ("Client Relay", "ui.residual.client_relay"),
            EApplet.eAppletClientShareFiles: ("Client Share Files", "ui.residual.client_share_files"),
            EApplet.eAppletTestHostService: ("Test Host Service", "ui.residual.test_host_service"),
            EApplet.eAppletCreateAccount: ("Create Account", "ui.residual.create_account"),
            EApplet.eAppletInformation: ("Information", "ui.residual.information"),
            EApplet.eAppletHomeFrame: ("Home Frame", "ui.residual.home_frame"),
            EApplet.eAppletMessengerFrame: ("Messenger Frame", "ui.residual.messenger_frame"),
        }
        if applet_id in residual_map:
            try:
                from residual_applets import ResidualApplet

                title, key = residual_map[applet_id]
                return ResidualApplet(title, key, settings, parent)
            except ImportError:
                pass

        if isinstance(applet_id, EApplet):
            concrete_form = try_create_form_backed_applet(applet_id.name, settings=settings, parent=parent)
            if concrete_form is not None:
                return concrete_form

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
