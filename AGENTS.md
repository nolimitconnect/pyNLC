# AGENTS

## Scope

- This repository is a migration workspace: the long-term goal is a Python 3.10+ and PySide6 application in `pyNLC/`, derived from the C++ upstream described in [README.md](README.md) and [docs/index.md](docs/index.md).
- Treat `nolimitgui/` as the legacy C++ reference implementation unless the task explicitly asks you to modify the old Qt code. The migration roadmap lives in [docs/migration.md](docs/migration.md).
- Prefer linking to existing project docs instead of duplicating them. Put new markdown documentation under `docs/` unless you are updating an existing top-level document.

## Where To Work

- Use `pyNLC/` for new Python GUI and application work.
- Use `py_wrapper/` for pybind11 bindings that expose C++ engine functionality to Python.
- Use `libs/` for shared native engine code.
- Be careful with `.vscode/` scripts and tasks: they are part of the supported developer workflow and often encode platform-specific setup.

## Migration Focus

- Treat `nolimitgui/` as read-only reference material while porting Qt-specific behavior into `pyNLC/`.
- Keep Python/C++ interface code in `py_wrapper/`; do not mix bridge code into GUI modules.
- Preserve user-visible behavior, signal flow, and network semantics while translating.
- Consider the legacy C++ app structure disposable only after the Python app reaches functional parity.

## Key References

- [README.md](README.md): project origin and high-level goal.
- [docs/index.md](docs/index.md): migration stage and project direction.
- [docs/migration.md](docs/migration.md): file-by-file translation guidance.
- [docs/reference.md](docs/reference.md): technical reference material.
- [CONTRIBUTING.md](CONTRIBUTING.md): contribution workflow and PR expectations.

## Build And Validation

- Root CMake currently configures the native libraries plus the local `pybind11` checkout and `py_wrapper/`. Confirm actual configured build targets before assuming the legacy Qt app target exists.
- Expect the build to change as the migration progresses; verify whether the current configure step still produces the intended Python extension targets before editing build logic around the old C++ executable.
- On Windows, VS Code CMake presets are gated by `NLC_ENABLE_VSCODE_PRESETS=1` and expect local Qt/Ninja/Android paths from [CMakePresets.json](CMakePresets.json).
- On Linux, the active presets are `linux-x64-debug` and `linux-x64-release` in [CMakePresets.json](CMakePresets.json).
- If a task or script refers to `nolimitconnect`, verify that the target is still produced by the current configure step before changing code around that workflow.
- For Qt translation work, use the existing VS Code tasks in [.vscode/tasks.json](.vscode/tasks.json); they reflect the intended `lupdate`/`lrelease` flow.

## Project-Specific Pitfalls

- Root [CMakeLists.txt](CMakeLists.txt) deliberately clears stale `CMAKE_PROJECT_INCLUDE_BEFORE` paths left behind by Qt Creator. Do not remove that guard unless you replace the workflow causing it.
- `CMakePresets.json` contains machine-local SDK and Qt paths. If configure fails, check environment and local tool paths before editing source.
- The repo contains both current migration docs and legacy upstream docs under subdirectories. Prefer the top-level `docs/` set when deciding project intent.
- Keep changes focused. Do not refactor broad legacy C++ surfaces during Python migration tasks unless the user asks for that specifically.

## Done Criteria

- The migration is complete when `pyNLC/` provides the functional equivalent of the original NoLimitConnect app.
- At that point, the old `nolimitgui/` tree can be removed.
