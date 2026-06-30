from __future__ import annotations

import importlib
from typing import Any


BASIC_COMPARE_KEYS = (
    "online_name",
    "online_description",
    "ping_ms",
    "last_session_ms",
    "groupie_modified_ms",
    "truth_accept",
    "truth_reject",
    "dare_accept",
    "dare_reject",
    "permissions_blob",
)

STRICT_COMPARE_KEYS = BASIC_COMPARE_KEYS + (
    "online_id",
    "my_friendship",
    "his_friendship",
    "admin_flags",
    "joined_any",
    "is_valid",
    "is_myself",
    "is_online",
    "is_direct_connected",
    "is_relayed",
    "can_direct_connect",
    "is_online_name_valid",
    "requires_open_port",
    "describe_user",
)


def _safe_call(obj: Any, method_name: str, default: Any = None) -> Any:
    method = getattr(obj, method_name, None)
    if not callable(method):
        return default
    try:
        return method()
    except Exception:
        return default


def snapshot_vx_net_ident(vx_net_ident: Any) -> dict[str, Any]:
    """Return a lightweight snapshot of commonly used VxNetIdent fields."""
    return {
        "online_id": _safe_call(vx_net_ident, "get_my_online_id"),
        "online_name": _safe_call(vx_net_ident, "get_online_name", ""),
        "online_description": _safe_call(vx_net_ident, "get_online_description", ""),
        "my_friendship": _safe_call(vx_net_ident, "get_my_friendship_to_him", 0),
        "his_friendship": _safe_call(vx_net_ident, "get_his_friendship_to_me", 0),
        "admin_flags": _safe_call(vx_net_ident, "get_admin_avail_flags", 0),
        "joined_any": _safe_call(vx_net_ident, "is_joined_any", False),
        "is_valid": _safe_call(vx_net_ident, "is_valid_net_ident", False),
        "is_myself": _safe_call(vx_net_ident, "is_myself", False),
        "is_online": _safe_call(vx_net_ident, "is_online", False),
        "is_direct_connected": _safe_call(vx_net_ident, "is_direct_connected", False),
        "is_relayed": _safe_call(vx_net_ident, "is_relayed", False),
        "can_direct_connect": _safe_call(vx_net_ident, "can_direct_connect_to_user", False),
        "is_online_name_valid": _safe_call(vx_net_ident, "is_online_name_valid", False),
        "requires_open_port": _safe_call(vx_net_ident, "requires_an_open_port", False),
        "describe_user": _safe_call(vx_net_ident, "describe_user", ""),
        "ping_ms": _safe_call(vx_net_ident, "get_ping_time_ms", 0),
        "last_session_ms": _safe_call(vx_net_ident, "get_last_session_time_ms", 0),
        "groupie_modified_ms": _safe_call(vx_net_ident, "get_last_groupie_info_modified_time_ms", 0),
        "truth_accept": _safe_call(vx_net_ident, "get_truth_accept_count", 0),
        "truth_reject": _safe_call(vx_net_ident, "get_truth_reject_count", 0),
        "dare_accept": _safe_call(vx_net_ident, "get_dare_accept_count", 0),
        "dare_reject": _safe_call(vx_net_ident, "get_dare_reject_count", 0),
        "permissions_blob": _safe_call(vx_net_ident, "get_plugin_permissions_bytes", b""),
    }


def clone_vx_net_ident_via_blob(vx_net_ident: Any) -> Any:
    """Clone a VxNetIdent using add_to_blob_bytes/extract_from_blob_bytes."""
    nlc_engine = importlib.import_module("nlc_engine")
    clone = nlc_engine.VxNetIdent()
    blob = vx_net_ident.add_to_blob_bytes()
    clone.extract_from_blob_bytes(blob)
    return clone


def verify_vx_net_ident_roundtrip(vx_net_ident: Any) -> dict[str, Any]:
    """Run roundtrip serialization smoke check and report field mismatches.

    Optional args:
    - strict: compare STRICT_COMPARE_KEYS instead of BASIC_COMPARE_KEYS.
    - compare_keys: explicit iterable of snapshot keys to compare.
    """
    return verify_vx_net_ident_roundtrip_with_mode(vx_net_ident)


def verify_vx_net_ident_roundtrip_with_mode(
    vx_net_ident: Any,
    *,
    strict: bool = False,
    compare_keys: tuple[str, ...] | list[str] | None = None,
) -> dict[str, Any]:
    before = snapshot_vx_net_ident(vx_net_ident)
    clone = clone_vx_net_ident_via_blob(vx_net_ident)
    after = snapshot_vx_net_ident(clone)

    keys_to_compare = tuple(compare_keys) if compare_keys is not None else (STRICT_COMPARE_KEYS if strict else BASIC_COMPARE_KEYS)

    mismatches: dict[str, dict[str, Any]] = {}
    for key in keys_to_compare:
        before_value = before.get(key)
        after_value = after.get(key)
        if before_value != after_value:
            mismatches[key] = {"before": before_value, "after": after_value}

    mismatch_summary = format_vx_net_ident_mismatch_report(mismatches)

    return {
        "ok": len(mismatches) == 0,
        "strict": strict,
        "compare_keys": list(keys_to_compare),
        "mismatches": mismatches,
        "mismatch_summary": mismatch_summary,
        "before": before,
        "after": after,
    }


def format_vx_net_ident_mismatch_report(mismatches: dict[str, dict[str, Any]]) -> str:
    """Format mismatch dict from verify_vx_net_ident_roundtrip* as readable text."""
    if not mismatches:
        return "No VxNetIdent roundtrip mismatches."

    lines: list[str] = ["VxNetIdent roundtrip mismatches:"]
    for key in sorted(mismatches.keys()):
        before_value = mismatches[key].get("before")
        after_value = mismatches[key].get("after")
        lines.append(f"- {key}: before={before_value!r}, after={after_value!r}")
    return "\n".join(lines)


def run_vx_net_ident_roundtrip_smoke_test(*, strict: bool = False) -> dict[str, Any]:
    """Create a VxNetIdent, roundtrip it through blob bytes, and report mismatch details.

    This is intended as a quick migration-time sanity check.
    """
    try:
        nlc_engine = importlib.import_module("nlc_engine")
    except Exception as exc:
        return {
            "ok": False,
            "error": f"nlc_engine import failed: {exc}",
        }

    ident = nlc_engine.VxNetIdent()
    try:
        ident.set_online_name("pyNLC-smoke")
        ident.set_online_description("vxnetident roundtrip smoke")
        ident.set_ping_time_ms(123)
        ident.set_truth_accept_count(2)
        ident.set_truth_reject_count(1)
        ident.set_dare_accept_count(3)
        ident.set_dare_reject_count(0)
    except Exception as exc:
        return {
            "ok": False,
            "error": f"failed to initialize VxNetIdent sample values: {exc}",
        }

    return verify_vx_net_ident_roundtrip_with_mode(ident, strict=strict)


__all__ = [
    "BASIC_COMPARE_KEYS",
    "STRICT_COMPARE_KEYS",
    "format_vx_net_ident_mismatch_report",
    "snapshot_vx_net_ident",
    "clone_vx_net_ident_via_blob",
    "verify_vx_net_ident_roundtrip",
    "verify_vx_net_ident_roundtrip_with_mode",
    "run_vx_net_ident_roundtrip_smoke_test",
]