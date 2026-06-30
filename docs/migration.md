# Migration Roadmap

This page tracks our progress moving code from `nolimitgui/` (C++) to `pyNLC/` (PySide6).

## Core Application Status

- [ ] `main.cpp` $\rightarrow$ `main.py` (App entry point and event loop)
- [ ] Core Settings and Configurations handling

## Interface Windows & Dialogs

- [ ] Main Window layout (`mainwindow.cpp`)
- [ ] Connection / Setup Dialogs
- [ ] Custom Chat or Data Windows

## Completed Applet Conversions

- [x] `AppletTheme.cpp/.h` -> `pyNLC/theme_applet.py` (using `resources/Forms/AppletTheme_ui.py`)
- [x] `AppletUserPreferences.cpp/.h` -> `pyNLC/user_preferences_applet.py` (using `resources/Forms/AppletUserPreferences_ui.py`)
- [x] `AppletNetworkSettings.cpp/.h` -> `pyNLC/network_settings_applet.py` (using `resources/Forms/AppletNetworkSettings_ui.py`)
- [x] `AppletPermissionList.cpp/.h` -> `pyNLC/permission_list_applet.py` (using `resources/Forms/AppletPermissionList_ui.py`)
- [x] `PermissionListItem.cpp/.h` -> `pyNLC/permission_list_applet.py::PermissionListItemWidget` (using `resources/Forms/PermissionListItemWidget_ui.py`)
- [x] `AppletUserConnections.cpp/.h` -> `pyNLC/user_connections_applet.py` (using `resources/Forms/AppletUserConnections_ui.py`)
- [x] `AppletSocketList.cpp/.h` -> `pyNLC/socket_list_applet.py` (using `resources/Forms/AppletSocketList_ui.py`)
- [x] `AppletHackerList.cpp/.h` -> `pyNLC/hacker_list_applet.py` (using `resources/Forms/AppletHackerList_ui.py`)
- [x] Legacy custom form widgets shimmed for PySide6 forms:
  - `AcceptCancelWidget` -> `pyNLC/AcceptCancelWidget.py`
  - `VxPushButton` -> `pyNLC/VxPushButton.py`
  - `VxComboBox` -> `pyNLC/VxComboBox.py`
  - `VxLabel` -> `pyNLC/VxLabel.py`
  - `VxPlainTextEdit` -> `pyNLC/VxPlainTextEdit.py`
  - `ClipboardCopyWidget` -> `pyNLC/ClipboardCopyWidget.py`
  - `GuiUserListWidget` -> `pyNLC/GuiUserListWidget.py`

## Deferred Until Final Media Step

- `AppletCam*` and camera-related widgets
- `AppletPlayer*` and media-player widgets
- `AppletSoundSettings` and audio-focused widgets

## Data & Networking Layer

- [ ] Network socket threads mapped to `QThread` / `QObject` signals
- [ ] Data stream parser conversion

---

## Translation Checklist for AI Agent

When converting a file, ensure:

1. All `QString` variables become native Python `str`.
2. Connectors use the `widget.signal.connect(slot)` layout.
3. Memory cleanup (`delete`) is removed completely.
