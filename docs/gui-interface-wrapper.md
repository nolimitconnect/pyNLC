# GuiInterface Wrapper Contract

This document defines the Python-side wrapper contract for the native interface in `libs/GuiInterface`.

## Scope

- Source of truth for GUI-to-engine calls: `libs/GuiInterface/IFromGui.h`
- Source of truth for engine-to-GUI callbacks: `libs/GuiInterface/IToGui.h`
- Python migration contract implementation: `py_wrapper/migration_stubs.py`

## Implemented Python Contract

The following contract classes mirror the method surface from `libs/GuiInterface`:

- `IFromGuiContractStub`
- `IToGuiContractStub`

Each method entry includes:

- native equivalent (header method name)
- implementation notes
- safe default return value for migration mode

All methods are installed on the contract classes dynamically from per-method specs.

Coverage check from headers:

- IFromGui header virtual declarations: 119
- IFromGui unique method names mirrored in Python: 115
- IToGui header virtual declarations: 71
- IToGui unique method names mirrored in Python: 71

Note: the IFromGui delta is due to overloaded methods sharing the same name in C++.
Overload-specific implementation notes are stored in the single method entry for that name.

## Runtime Integration

`pyNLC/main.py` integrates the contract through `GuiToEngineBridge`.

When a GUI call is not implemented directly in `GuiToEngineBridge`, lookup order is:

1. `IFromGuiContractStub` method support
2. `MediaFeatureStub` support
3. raise `AttributeError`

This enables broad interface coverage during migration without silently dropping unknown calls.

For engine-to-GUI callback staging, `py_wrapper` now provides `IToGuiEventSink`.
It supports handler registration by IToGui method name and records all calls,
so callback consumers can be developed before full native callback adapter wiring.

The native extension now also exposes `nlc_engine.IToGuiAdapter` with callback registration:

- `set_default_handler(callable)`
- `register_handler(method_name, callable)`
- `clear_handlers(method_name)` / `clear_all_handlers()`

Current high-frequency callback dispatch implemented in native adapter:

- `toGuiStatusMessage`
- `toGuiPluginMsg`
- `toGuiPluginCommError`
- `toGuiPluginStatus`
- `toGuiFileXferState`
- `toGuiHostSearchStatus`
- `toGuiHostSearchResult`
- `toGuiHostSearchComplete`
- `toGuiGroupieSearchStatus`
- `toGuiGroupieSearchComplete`
- `toGuiNetAvailableStatus`
- `toGuiNetworkState`

Each registered callable is invoked as:

- `handler(method_name, *event_args)`

## Native pybind11 Coverage (Current)

The extension module `nlc_engine` now binds these concrete native types and enums:

- `VxGUID`
- `VxNetIdent`
- `NetHostSetting`
- `NetSettings`
- `FileInfo`
- `HostedInfo`
- `SearchParams`
- `ECacheType`
- `EFirewallTestType`
- `EFriendState`
- `EPluginServerState`
- `EHostType`
- `ESearchType`
- `EInternetStatus`
- `ENetAvailStatus`
- `EPluginAccess`
- `EAppState`
- `EHackerLevel`
- `EHackerReason`

`VxNetIdent` coverage was expanded to include connectivity, host-join/admin flags,
friendship transitions, plugin permission/access checks, and descriptive helpers
so callback payloads can be interpreted directly in Python.

Additional coverage now includes Python equality operators, plugin-permission setters,
and debug/diagnostic helpers (`debug_dump_ident`, `dump_permissions`).

`VxNetIdent` plugin permission parity now also includes bulk permission-array access:

- `set_plugin_permissions_to_default_values()`
- `get_plugin_permissions_bytes()` (returns 24-byte `bytes`)
- `set_plugin_permissions_bytes(bytes_24)`
- `add_to_blob_bytes()`
- `extract_from_blob_bytes(blob_bytes)`

Python helper utilities are exposed from `py_wrapper` for working with that blob:

- `decode_plugin_permissions(bytes_24)`
- `encode_plugin_permissions(permission_map, default_permission=0)`
- `get_plugin_permission_from_blob(bytes_24, plugin_type)`
- `set_plugin_permission_in_blob(bytes_24, plugin_type, friend_state)`
- `read_permissions_from_ident(vx_net_ident)`
- `write_permissions_to_ident(vx_net_ident, permission_map, default_permission=0)`

VxNetIdent roundtrip smoke helpers are also exposed from `py_wrapper`:

- `snapshot_vx_net_ident(vx_net_ident)`
- `clone_vx_net_ident_via_blob(vx_net_ident)`
- `verify_vx_net_ident_roundtrip(vx_net_ident)`
- `verify_vx_net_ident_roundtrip_with_mode(vx_net_ident, strict=False, compare_keys=None)`
- `run_vx_net_ident_roundtrip_smoke_test(strict=False)`
- `BASIC_COMPARE_KEYS`
- `STRICT_COMPARE_KEYS`
- `format_vx_net_ident_mismatch_report(mismatches)`

Roundtrip verification results now include a human-readable summary string:

- `mismatch_summary`

Blob parity note:

- `PktBlobEntry` itself is still not directly exposed in Python.
- `VxNetIdent` serialization is available via byte-based wrappers that internally use `PktBlobEntry`.

`VxCommon` hack reporting can now be consumed in Python via:

- `nlc_engine.set_hack_report_handler(callable_or_none)`
- `nlc_engine.clear_hack_report_handler()`

The registered callable signature is:

- `handler(hacker_level, hacker_reason, ip_addr, description)`

Replay-aware applet signature now supports an optional event timestamp:

- `add_hack_report(hacker_level, hacker_reason, ip_addr, description, timestamp_ms=None)`

Current pyNLC wiring:

- `pyNLC/main.py` installs the native callback through `GuiToEngineBridge.install_native_callbacks()`
- `GuiSignals.hack_reported` emits normalized values (`int, int, str, str`)
- `pyNLC/home_window.py` connects `hack_reported` to applets implementing `add_hack_report`
- `pyNLC/hacker_list_applet.py` consumes live events and aggregates repeat offenses by `(ip, level, offense)`

Additional pyNLC wiring for `IToGuiAdapter` default-handler dispatch:

- `toGuiStatusMessage` -> `GuiSignals.status_message`
- `toGuiPluginMsg` -> `GuiSignals.plugin_message`
- `toGuiPluginCommError` -> `GuiSignals.plugin_comm_error`
- `toGuiPluginStatus` -> `GuiSignals.plugin_status`
- `toGuiFileXferState` -> `GuiSignals.file_xfer_state`
- `toGuiHostSearchStatus` -> `GuiSignals.host_search_status`
- `toGuiHostSearchResult` -> `GuiSignals.host_search_result`
- `toGuiHostSearchComplete` -> `GuiSignals.host_search_complete`
- `toGuiGroupieSearchStatus` -> `GuiSignals.groupie_search_status`
- `toGuiGroupieSearchComplete` -> `GuiSignals.groupie_search_complete`
- `toGuiNetAvailableStatus` -> `GuiSignals.net_available_status`
- `toGuiNetworkState` -> `GuiSignals.network_state`

`pyNLC/home_window.py` subscribes to these signals and updates messenger status text.

Applet consumers currently wired:

- `pyNLC/socket_list_applet.py`: `add_plugin_message(plugin_type, online_id, msg_type, message, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_plugin_comm_error(plugin_type, online_id, msg_type, comm_error, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_plugin_status(plugin_type, status_type, status_value, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_file_xfer_state(plugin_type, session_id, xfer_direction, xfer_state, xfer_error, param1, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_host_search_status(host_type, session_id, search_status, comm_error, message, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_host_search_result(host_type, session_id, hosted_info, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_host_search_complete(host_type, session_id, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_groupie_search_status(host_type, session_id, search_status, comm_error, message, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: `add_groupie_search_complete(host_type, session_id, timestamp_ms=None)`
- `pyNLC/socket_list_applet.py`: button action runs `run_vx_net_ident_roundtrip_smoke_test(strict=True)` and shows status/mismatch keys in the table
- `pyNLC/socket_list_applet.py`: `Copy Smoke Summary` action copies latest `mismatch_summary` to clipboard
- `pyNLC/socket_list_applet.py`: `Show Smoke Details` action displays full smoke result payload for diagnostics
- `pyNLC/socket_list_applet.py`: activating the `vxnetident`/`smoke` diagnostics row opens the same full details dialog
- `pyNLC/socket_list_applet.py`: smoke row temp text and tooltips explicitly show "double-click for details"
- `pyNLC/socket_list_applet.py`: smoke row includes strict run duration in milliseconds (for regression visibility)
- `pyNLC/socket_list_applet.py`: smoke diagnostics track session duration stats (`runs`, `last_ms`, `min_ms`, `max_ms`, `avg_ms`) and show them in details
- `pyNLC/network_settings_applet.py`: `on_net_available_status(status, timestamp_ms=None)`
- `pyNLC/network_settings_applet.py`: `on_network_state(state, state_message, timestamp_ms=None)`

Bridge replay behavior:

- `GuiToEngineBridge` keeps bounded recent-event buffers (size 128) for hack/plugin/xfer/network callbacks.
- each buffered entry also stores `timestamp_ms` so replay consumers can show event recency.
- replay now includes search lifecycle events (`host/groupie status`, `host result`, `host/groupie complete`).
- `HomeWindow` calls `bridge.replay_events_to_applet(applet_widget)` after connecting applet handlers.
- Applets launched after events have already occurred now receive recent state immediately.

Native describe helpers exposed in `nlc_engine`:

- `describe_hacker_level(int)`
- `describe_hacker_reason(int)`
- `describe_net_avail_status(int)`
- `describe_network_state(int)`
- `describe_xfer_direction(int)`
- `describe_xfer_state(int)`
- `describe_xfer_error(int)`
- `describe_xfer_action(int)`
- `describe_comm_error(int)`
- `describe_host_search_status(int)`
- `describe_host_type(int)`
- `describe_plugin_type(int)`

These are now used by applets to render readable callback states.

The `IFromGui` interface currently exposes real bindings for:

- startup/shutdown and user directory setup
- user deletion and disk free space query
- cache clearing
- net settings set/get
- net host settings apply
- random tcp port and network status queries
- type-backed payload paths used by host/search/file workflows (`VxNetIdent`, `SearchParams`, `HostedInfo`, `FileInfo`)

All remaining `IFromGui` and `IToGui` methods continue to route through Python contract stubs with implementation notes.

## Migration Rule

When implementing a missing behavior:

1. Check for equivalent in `libs/GuiInterface` first.
2. If equivalent exists, bind/forward to native behavior.
3. If no equivalent exists, keep a Python stub with clear implementation notes in `py_wrapper/migration_stubs.py`.

## Next Steps

- Replace high-use `IFromGuiContractStub` methods with real pybind11-backed implementations.
- Add pybind11 bindings for frequently used value types: `VxNetIdent`, `FileInfo`, `HostedInfo`, and related enums.
- Introduce a callback bridge implementing `IToGui` to route native events into Qt signals.

Current focus for the callback bridge:

- map high-frequency events first (`toGuiStatusMessage`, `toGuiPluginMsg`, transfer state callbacks)
- forward into `IToGuiEventSink` handlers and then Qt signals in `pyNLC`

Remaining native callback bridge work:

- wire `IToGuiAdapter` into the app-instance callback path (`GetAppInstance().getIToGui()`)
- expand dispatch coverage across the remaining `IToGui` callbacks

Current blocker for live `IToGui` installation from `py_wrapper`:

- `GetAppInstance()` / `AppCommon` ownership lives under `nolimitgui/src` (Qt app layer),
  while `py_wrapper` links engine/static libs without that Qt app singleton surface.
