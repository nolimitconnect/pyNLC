# Skill: NoLimitConnect Qt6 to PySide6 Migration

## Role

You are migrating the NoLimitConnect desktop app from C++/Qt6 to Python 3.10+ / PySide6.

The immediate goal is to move all Qt-specific application code out of [nolimitgui/](nolimitgui/) and into [pyNLC/](pyNLC/), while keeping the C++ engine and Python bridge separation clean.

## Working Boundaries

- Write the new Python application and GUI code in [pyNLC/](pyNLC/).
- Write Python/C++ bridge code, callbacks, trampoline classes, and other interop code in [py_wrapper/](py_wrapper/).
- Treat [nolimitgui/](nolimitgui/) as a read-only reference implementation unless the user explicitly asks to change the legacy C++ app.
- Keep shared native engine work in [libs/](libs/) only when the task is about the underlying engine, not the GUI port.
- Do not delete [nolimitgui/](nolimitgui/) until the Python app is functionally equivalent to the original application and the migration is complete.

## Migration Priorities

1. Preserve behavior first, structure second. Match the original UI flow, state transitions, network interactions, and user-visible outcomes.
2. Translate Qt widgets, dialogs, and windows into PySide6 equivalents.
3. Keep engine-facing C++ callbacks isolated behind the bridge layer in [py_wrapper/](py_wrapper/).
4. Repair the build system as part of the migration when it still assumes the old C++ app layout.
5. Remove legacy code paths only after the Python replacement is complete and verified.

## Translation Rules

- Convert `QString` and `std::string` to Python `str`.
- Convert Qt and STL containers to native Python containers when practical.
- Rewrite classes as normal Python classes, typically `class MyWidget(QWidget):`.
- Prefer explicit, readable PySide6 signal/slot connections such as `button.clicked.connect(self.on_click)`.
- Use scoped Qt enums such as `Qt.AlignmentFlag.AlignLeft`.
- Remove manual memory management from translated Python code, but do not oversimplify ownership when C++ objects still cross the bridge.

## Bridge Rules

- `IToGui` and similar engine callback interfaces belong in the bridge layer, not in the GUI layer.
- C++ callbacks that can run on background threads must not touch PySide6 widgets directly.
- Bridge callbacks should marshal data into the GUI thread using PySide6 signals or queued handoff code.
- Acquire the GIL in C++ callback paths before invoking Python objects.
- Keep any trampoline, wrapper, or callback-adapter code in [py_wrapper/](py_wrapper/).

## Build Rules

- Expect the current build system to be partially broken because it was originally shaped around the C++ app.
- Fix CMake and related build logic as the migration progresses so the Python app and bridge build cleanly.
- Use the checked-in CMake targets and VS Code task flow as the source of truth, but verify that the target still exists before relying on it.
- `pybind11` is vendored in the repository; use the local checkout and `add_subdirectory()`, not `find_package(pybind11)`.

## Code Quality

- Keep names, workflows, and behavior aligned with the original app unless the port requires a deliberate redesign.
- Write clean, PEP 8-compliant Python.
- Add short docstrings or comments when they help map translated Python code back to the original C++ class or behavior.
- Prefer small, reviewable migration steps over large mechanical rewrites.
- Keep changes local to the migration slice you are actively translating.
<!-- End of migration skill. -->