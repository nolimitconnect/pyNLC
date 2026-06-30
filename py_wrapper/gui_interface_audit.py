from __future__ import annotations

import re
from pathlib import Path
from typing import Iterable

from .migration_stubs import IFromGuiContractStub, IToGuiContractStub


_VIRTUAL_DECL_PATTERN = re.compile(r"\bvirtual\b(.*?);", re.DOTALL)
_IDENT_AT_END_PATTERN = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*$")


def _extract_virtual_method_names(header_text: str) -> list[str]:
    names: list[str] = []
    for match in _VIRTUAL_DECL_PATTERN.finditer(header_text):
        decl = match.group(1)
        head = decl.split("(", 1)[0]
        head = head.replace("\n", " ").replace("\r", " ").strip()
        ident_match = _IDENT_AT_END_PATTERN.search(head)
        if ident_match is None:
            continue
        names.append(ident_match.group(1))

    # Normalize to unique names while preserving order.
    seen: set[str] = set()
    ordered: list[str] = []
    for name in names:
        if name in seen:
            continue
        seen.add(name)
        ordered.append(name)
    return ordered


def _diff(expected: Iterable[str], actual: Iterable[str]) -> tuple[list[str], list[str]]:
    expected_set = set(expected)
    actual_set = set(actual)
    missing = sorted(expected_set - actual_set)
    extra = sorted(actual_set - expected_set)
    return missing, extra


def audit_gui_interface_contract(repo_root: Path) -> dict[str, object]:
    """Audit Python contract coverage vs libs/GuiInterface headers.

    Args:
        repo_root: Repository root containing libs/GuiInterface.

    Returns:
        Dict with method counts and missing/extra method names for IFromGui/IToGui.
    """

    ifrom_header = repo_root / "libs" / "GuiInterface" / "IFromGui.h"
    itogui_header = repo_root / "libs" / "GuiInterface" / "IToGui.h"

    ifrom_text = ifrom_header.read_text(encoding="utf-8")
    itogui_text = itogui_header.read_text(encoding="utf-8")

    ifrom_methods = _extract_virtual_method_names(ifrom_text)
    itogui_methods = _extract_virtual_method_names(itogui_text)

    ifrom_contract_methods = sorted(IFromGuiContractStub._METHODS.keys())
    itogui_contract_methods = sorted(IToGuiContractStub._METHODS.keys())

    ifrom_missing, ifrom_extra = _diff(ifrom_methods, ifrom_contract_methods)
    itogui_missing, itogui_extra = _diff(itogui_methods, itogui_contract_methods)

    return {
        "ifrom": {
            "header_unique_methods": len(ifrom_methods),
            "contract_methods": len(ifrom_contract_methods),
            "missing_in_contract": ifrom_missing,
            "extra_in_contract": ifrom_extra,
        },
        "itogui": {
            "header_unique_methods": len(itogui_methods),
            "contract_methods": len(itogui_contract_methods),
            "missing_in_contract": itogui_missing,
            "extra_in_contract": itogui_extra,
        },
    }


def format_audit_report(report: dict[str, object]) -> str:
    ifrom = report["ifrom"]
    itogui = report["itogui"]

    def _line(name: str, section: dict[str, object]) -> list[str]:
        lines = [
            f"[{name}] header_unique_methods={section['header_unique_methods']} contract_methods={section['contract_methods']}",
            f"[{name}] missing_in_contract={section['missing_in_contract']}",
            f"[{name}] extra_in_contract={section['extra_in_contract']}",
        ]
        return lines

    lines = []
    lines.extend(_line("ifrom", ifrom))
    lines.extend(_line("itogui", itogui))
    return "\n".join(lines)
