# Migration Roadmap

This page tracks our progress moving code from `nolimitgui/` (C++) to `pyNLC/` (PySide6).

## Core Application Status
- [ ] `main.cpp` $\rightarrow$ `main.py` (App entry point and event loop)
- [ ] Core Settings and Configurations handling

## Interface Windows & Dialogs
- [ ] Main Window layout (`mainwindow.cpp`)
- [ ] Connection / Setup Dialogs
- [ ] Custom Chat or Data Windows

## Data & Networking Layer
- [ ] Network socket threads mapped to `QThread` / `QObject` signals
- [ ] Data stream parser conversion

---

## Translation Checklist for AI Agent
When converting a file, ensure:
1. All `QString` variables become native Python `str`.
2. Connectors use the `widget.signal.connect(slot)` layout.
3. Memory cleanup (`delete`) is removed completely.
