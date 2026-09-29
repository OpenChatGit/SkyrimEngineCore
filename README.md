# Skyrim Engine Core (SEC)

> Unified, modern C++23 native SKSE plugin consolidating engine fixes, display enhancements, stability patches, and performance tweaks for Skyrim Special Edition & Anniversary Edition (1.6.1170+ / 1.7.104+).

---

## ✦ Features

- **File Handle Expansion (8,192 MaxStdIO):** Eliminates missing-mesh and texture loading crashes in heavy modlists by multiplying available file descriptors.
- **SafeExit & Quick Termination:** Prevents freeze-on-exit and allows closing Skyrim immediately with Alt+F4.
- **Automated Co-Save Purge:** Automatically detects and purges orphaned `.skse` co-saves without matching `.ess` save files.
- **Actor Movement & Lip-Sync Scaler:** Expands active moving actor cap to 256 and simultaneous facial morph / lip-sync cap to 64 actors.
- **High-Refresh Dynamic Havok Physics:** High-framerate physics timescale scaling (>60 FPS / 144Hz / 240Hz) preventing physics glitching and camera stutter.
- **Engine Integrity Patches:** Fixes temporary effect NiRTTI inheritance memory crashes and particle shader depth writing.
- **Native Diagnostics & Self-Test:** Automatic startup self-test report in `SkyCore.log` and lightweight VEH crash handler with register and module dump.
- **In-Game Hardware Monitor & OSD:** Native DirectX 11 / ImGui overlay with hotkey toggling and SkyUI MCM configuration.

---

## ✦ Requirements

- [SKSE64](https://skse.silverlock.org/) (matching your game version)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

---

## ✦ Building from Source

```powershell
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## ✦ License & Legal Compliance

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.

All research, open-source logic, and memory reverse-engineering utilized in this project operate strictly within the legal scope of the **GNU General Public License v3.0** and the **MIT License**. The rights granted by the original copyright holders under these licenses are legally binding and cannot be diminished or superseded by third parties.

Acknowledgements to the open-source reverse-engineering community:
- aers, Nukem, Ryan (fudgyduff)
- SlavicPotato
- powerofthree
- KernalsEgg
- D7ry
- ianpatt, behippo, scripthoge, CharmedBaryon
