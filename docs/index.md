# pyNLC

pyNLC is a Python-based fork of the C++ upstream repository:
[NoLimitConnect](https://github.com/nolimitconnect/NoLimitConnect)

## Summary

This project is forked from **v1.1.3** of the original C++ repository. 

Our main goal is to completely replace the C++ Qt Widgets and core UI libraries with **Python 3.10+ and PySide6**. 

### Project Timeline
1. **Isolate:** Keep the old C++ GUI folder (`nolimitgui/`) untouched for reference.
2. **Translate:** Convert components piece-by-piece into the new `pyNLC/` directory.
3. **Verify:** Ensure the Python GUI acts exactly like the original app.
4. **Next Step:** Package the code into a dedicated NLC plugin for Kodi.

---

## Project Context

* **Target Kodi Addon:** [kodi-addon-nolimitconnect](https://github.com/nolimitconnect/kodi-addon-nolimitconnect)
* **Official Website:** Learn how the system works at [nolimitconnect.org](https://nolimitconnect.org)

To see what we are currently translating and what is left to do, check out our [Migration Roadmap](migration.md).
