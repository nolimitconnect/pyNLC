from __future__ import annotations

from collections.abc import Mapping


PERMISSION_ARRAY_SIZE = 24
MAX_PLUGIN_SLOTS = PERMISSION_ARRAY_SIZE * 2


def _validate_plugin_type(plugin_type: int) -> int:
    plugin_index = int(plugin_type)
    if plugin_index < 0 or plugin_index >= MAX_PLUGIN_SLOTS:
        raise ValueError(f"plugin_type out of range: {plugin_index} (expected 0..{MAX_PLUGIN_SLOTS - 1})")
    return plugin_index


def _validate_permission_blob(permission_blob: bytes | bytearray | memoryview) -> bytes:
    blob = bytes(permission_blob)
    if len(blob) != PERMISSION_ARRAY_SIZE:
        raise ValueError(f"permission blob must be {PERMISSION_ARRAY_SIZE} bytes, got {len(blob)}")
    return blob


def decode_plugin_permissions(permission_blob: bytes | bytearray | memoryview) -> dict[int, int]:
    """Decode 24-byte plugin permissions blob into {plugin_type: friend_state} mapping.

    Each plugin uses 4 bits, so one byte stores two plugin permissions.
    """
    blob = _validate_permission_blob(permission_blob)
    decoded: dict[int, int] = {}
    for byte_index, packed in enumerate(blob):
        low_plugin = byte_index * 2
        high_plugin = low_plugin + 1
        decoded[low_plugin] = packed & 0x0F
        decoded[high_plugin] = (packed >> 4) & 0x0F
    return decoded


def encode_plugin_permissions(permission_map: Mapping[int, int], default_permission: int = 0) -> bytes:
    """Encode {plugin_type: friend_state} mapping into a 24-byte permissions blob."""
    default_nibble = int(default_permission) & 0x0F
    packed = bytearray(PERMISSION_ARRAY_SIZE)
    fill = (default_nibble << 4) | default_nibble
    packed[:] = bytes([fill]) * PERMISSION_ARRAY_SIZE

    for plugin_type, friend_state in permission_map.items():
        plugin_index = _validate_plugin_type(int(plugin_type))
        byte_index = plugin_index // 2
        nibble_value = int(friend_state) & 0x0F
        current = packed[byte_index]
        if (plugin_index % 2) == 0:
            packed[byte_index] = (current & 0xF0) | nibble_value
        else:
            packed[byte_index] = (current & 0x0F) | (nibble_value << 4)

    return bytes(packed)


def get_plugin_permission_from_blob(permission_blob: bytes | bytearray | memoryview, plugin_type: int) -> int:
    blob = _validate_permission_blob(permission_blob)
    plugin_index = _validate_plugin_type(plugin_type)
    packed = blob[plugin_index // 2]
    if (plugin_index % 2) == 0:
        return packed & 0x0F
    return (packed >> 4) & 0x0F


def set_plugin_permission_in_blob(
    permission_blob: bytes | bytearray | memoryview,
    plugin_type: int,
    friend_state: int,
) -> bytes:
    blob = bytearray(_validate_permission_blob(permission_blob))
    plugin_index = _validate_plugin_type(plugin_type)
    nibble_value = int(friend_state) & 0x0F
    byte_index = plugin_index // 2
    current = blob[byte_index]
    if (plugin_index % 2) == 0:
        blob[byte_index] = (current & 0xF0) | nibble_value
    else:
        blob[byte_index] = (current & 0x0F) | (nibble_value << 4)
    return bytes(blob)


def read_permissions_from_ident(vx_net_ident) -> dict[int, int]:
    """Return decoded plugin permissions for a VxNetIdent-like object."""
    blob = vx_net_ident.get_plugin_permissions_bytes()
    return decode_plugin_permissions(blob)


def write_permissions_to_ident(vx_net_ident, permission_map: Mapping[int, int], default_permission: int = 0) -> None:
    """Write mapping into a VxNetIdent-like object through bytes API."""
    blob = encode_plugin_permissions(permission_map, default_permission=default_permission)
    vx_net_ident.set_plugin_permissions_bytes(blob)


__all__ = [
    "PERMISSION_ARRAY_SIZE",
    "MAX_PLUGIN_SLOTS",
    "decode_plugin_permissions",
    "encode_plugin_permissions",
    "get_plugin_permission_from_blob",
    "set_plugin_permission_in_blob",
    "read_permissions_from_ident",
    "write_permissions_to_ident",
]