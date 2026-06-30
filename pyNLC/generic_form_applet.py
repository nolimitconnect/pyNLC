from __future__ import annotations

import importlib
import re
from typing import Any

from PySide6.QtWidgets import QLabel, QLineEdit, QPushButton, QVBoxLayout, QWidget


class GenericFormApplet(QWidget):
    """Concrete form-backed applet with local-only stubbed behavior.

    This adapter creates a usable widget for generated Qt form modules without
    invoking engine/GuiInterface calls.
    """

    def __init__(self, form_module_name: str, settings: Any = None, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._settings = settings
        self._form_module_name = form_module_name
        self._ui = self._load_ui(form_module_name)
        self._ui.setupUi(self)
        self._status = QLabel("Ready (local stub mode)", self)

        layout = self.layout()
        if isinstance(layout, QVBoxLayout):
            layout.addWidget(self._status)
        else:
            fallback = QVBoxLayout(self)
            fallback.setContentsMargins(4, 4, 4, 4)
            fallback.addWidget(self._status)

        self._wire_local_handlers()

    @staticmethod
    def _load_ui(form_module_name: str) -> Any:
        module = importlib.import_module(form_module_name)
        for attr_name in dir(module):
            if attr_name.startswith("Ui_"):
                ui_cls = getattr(module, attr_name)
                return ui_cls()
        raise ImportError(f"No Ui_* class found in {form_module_name}")

    def _wire_local_handlers(self) -> None:
        for btn in self.findChildren(QPushButton):
            btn.clicked.connect(lambda _checked=False, name=btn.objectName() or btn.text(): self._on_button(name))

        for line_edit in self.findChildren(QLineEdit):
            line_edit.editingFinished.connect(
                lambda obj=line_edit: self._on_line_edit_commit(obj.objectName() or "line_edit", obj.text())
            )

    def _on_button(self, name: str) -> None:
        self._status.setText(f"Action: {name} (stubbed locally)")
        self._remember(f"ui.stub.last_button.{self._sanitize(name)}", True)

    def _on_line_edit_commit(self, key: str, value: str) -> None:
        self._status.setText(f"Updated: {key}")
        self._remember(f"ui.stub.field.{self._sanitize(key)}", value)

    def _remember(self, key: str, value: Any) -> None:
        if self._settings is None:
            return
        store = getattr(self._settings, "_settings_store", None)
        if isinstance(store, dict):
            store[key] = value

    @staticmethod
    def _sanitize(value: str) -> str:
        return re.sub(r"[^a-zA-Z0-9_.-]+", "_", value).strip("_")


def try_create_form_backed_applet(enum_name: str, settings: Any = None, parent: QWidget | None = None) -> QWidget | None:
    """Try to create a concrete applet from generated resources/Forms modules."""

    if enum_name in {"eAppletUnknown", "eAppletHomePage"}:
        return None

    bases: list[str] = []
    for prefix in ("eApplet", "eActivity", "ePluginApplet"):
        if enum_name.startswith(prefix):
            bases.append(enum_name[len(prefix) :])
            break
    if not bases:
        return None

    base = bases[0]
    candidates = [
        f"resources.Forms.Applet{base}_ui",
        f"resources.Forms.Applet{base.replace('Storyboard', 'StoryBoard')}_ui",
        f"resources.Forms.Applet{base.replace('StoryBoard', 'Storyboard')}_ui",
    ]

    if enum_name == "eActivityBrowseFiles":
        candidates.append("resources.Forms.AppletBrowseFiles_ui")

    tried: set[str] = set()
    for module_name in candidates:
        if module_name in tried:
            continue
        tried.add(module_name)
        try:
            return GenericFormApplet(module_name, settings=settings, parent=parent)
        except Exception:
            continue

    return None
