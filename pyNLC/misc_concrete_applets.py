from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QWidget

from resources.Forms.AppletAboutFile_ui import Ui_AppletAboutFileUi
from resources.Forms.AppletAboutMeClient_ui import Ui_AppletAboutMeClientUi
from resources.Forms.AppletAboutUser_ui import Ui_AppletAboutUserUi
from resources.Forms.AppletApplicationInfo_ui import Ui_AppletApplicationInfoUi
from resources.Forms.AppletBrowseFiles_ui import Ui_AppletBrowseFilesUi
from resources.Forms.AppletChooseThumbnail_ui import Ui_AppletChooseThumbnailUi
from resources.Forms.AppletFileOfferSelect_ui import Ui_AppletFileOfferSelectUi
from resources.Forms.AppletFileShareClientView_ui import Ui_AppletFileShareClientViewUi
from resources.Forms.AppletFriendRequestList_ui import Ui_AppletFriendRequestListUi
from resources.Forms.AppletFriendRequest_ui import Ui_AppletFriendRequestUi
from resources.Forms.AppletHelpNetSignalBars_ui import Ui_AppletHelpNetSignalBarsUi
from resources.Forms.AppletIgnoredHosts_ui import Ui_AppletIgnoredHostsUi
from resources.Forms.AppletLogSettings_ui import Ui_AppletLogSettingsWidget
from resources.Forms.AppletLog_ui import Ui_AppletLogUi
from resources.Forms.AppletPlayerCamClip_ui import Ui_AppletCamClipPlayerUi
from resources.Forms.AppletPlayerPhoto_ui import Ui_AppletPlayerPhotoUi
from resources.Forms.AppletPlayerVideo_ui import Ui_AppletPlayerVideoUi
from resources.Forms.AppletPopupMenu_ui import Ui_AppletPopupMenuUi
from resources.Forms.AppletSnapshot_ui import Ui_AppletSnapshotUi
from resources.Forms.AppletStoryBoardClient_ui import Ui_AppletStoryboardClientUi
from resources.Forms.AppletTestUpnp_ui import Ui_AppletTestUpnpUi


def _write_setting(settings, key: str, value: str) -> None:
    if settings is None:
        return
    store = getattr(settings, "_settings_store", None)
    if isinstance(store, dict):
        store[key] = value


class _SimpleApplet(QWidget):
    def __init__(self, ui_cls, title: str, key: str, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._key = key
        self.ui = ui_cls()
        self.ui.setupUi(self)
        self.setWindowTitle(title)
        _write_setting(self._settings, self._key, f"opened:{datetime.now().strftime('%H:%M:%S')}")


class ApplicationInfoApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletApplicationInfoUi, "Application Info", "ui.app.application_info", settings, parent)


class BrowseFilesApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletBrowseFilesUi, "Browse Files", "ui.app.browse_files", settings, parent)


class PlayerCamClipApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletCamClipPlayerUi, "Player Cam Clip", "ui.player.cam_clip", settings, parent)


class PlayerPhotoApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletPlayerPhotoUi, "Player Photo", "ui.player.photo", settings, parent)


class PlayerVideoApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletPlayerVideoUi, "Player Video", "ui.player.video", settings, parent)


class ChooseThumbnailApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletChooseThumbnailUi, "Choose Thumbnail", "ui.media.choose_thumbnail", settings, parent)


class FriendRequestApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletFriendRequestUi, "Friend Request", "ui.friend.request", settings, parent)


class FriendRequestListApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletFriendRequestListUi, "Friend Request List", "ui.friend.request_list", settings, parent)


class SnapshotApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletSnapshotUi, "Snapshot", "ui.media.snapshot", settings, parent)


class LogApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletLogUi, "Log", "ui.log.main", settings, parent)


class LogSettingsApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletLogSettingsWidget, "Log Settings", "ui.log.settings", settings, parent)


class TestUpnpApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletTestUpnpUi, "UPnP Test", "ui.net.upnp_test", settings, parent)


class AboutFileApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletAboutFileUi, "About File", "ui.about.file", settings, parent)


class AboutMeClientApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletAboutMeClientUi, "About Me Client", "ui.about.me_client", settings, parent)


class AboutUserApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletAboutUserUi, "About User", "ui.about.user", settings, parent)


class FileShareClientViewApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletFileShareClientViewUi, "File Share Client View", "ui.file_share.client_view", settings, parent)


class StoryboardClientApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletStoryboardClientUi, "Storyboard Client", "ui.storyboard.client", settings, parent)


class FileOfferSelectApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletFileOfferSelectUi, "File Offer Select", "ui.offer.file_select", settings, parent)


class IgnoredHostsApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletIgnoredHostsUi, "Ignored Hosts", "ui.host.ignored", settings, parent)


class PopupMenuApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletPopupMenuUi, "Popup Menu", "ui.util.popup_menu", settings, parent)


class HelpNetSignalBarsApplet(_SimpleApplet):
    def __init__(self, settings=None, parent: QWidget | None = None) -> None:
        super().__init__(Ui_AppletHelpNetSignalBarsUi, "Help Net Signal Bars", "ui.help.net_signal_bars", settings, parent)
