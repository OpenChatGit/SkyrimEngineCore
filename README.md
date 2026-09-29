# Skyrim Engine Core (SEC)

> Unified, modern C++23 native SKSE plugin consolidating engine fixes, display enhancements, stability patches, and performance tweaks for Skyrim Special Edition & Anniversary Edition (1.6.1170+ / 1.7.104+).

---

## ✦ Features

- **MaxStdIO Fix:** Increases maximum open file handles from vanilla 512 to **8,192** (prevents missing-mesh and texture loading crashes in heavy modlists).
- **SafeExit & Alt+F4 Quick Exit:** Prevents hang-on-exit crashes and allows closing Skyrim immediately with Alt+F4 (`df4quitfix`).
- **Clean Co-Saves:** Automatically purges orphaned `.skse` co-saves without matching `.ess` save files.
- **Actor Limit Fix:** Expands actor movement cap to 256 and facial morph/lip-sync cap to 64 simultaneous actors.
- **Dynamic Havok Physics:** High-refresh physics timescale scaling (>60 FPS / 144Hz / 240Hz) preventing Havok glitching.
- **Engine Patches:** Temporary effect NiRTTI inheritance fix and particle shader Z-buffer depth fix.
- **Built-in Diagnostics:** Startup self-test suite and lightweight exception handler with register and module crash logging.
- **In-Game FPS & OSD:** Native DirectX 11 / ImGui overlay with hotkey toggling and SkyUI MCM configuration.

---

## ✦ Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

---

## ✦ Building from Source

```powershell
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## ✦ License & Credits

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.

Special thanks and acknowledgement to the original creators whose research and open-source foundations made this unified project possible:
- **SSE Engine Fixes** – aers, Nukem, Ryan (fudgyduff)
- **SSE Display Tweaks** – SlavicPotato
- **powerofthree's Tweaks** – powerofthree
- **Actor Limit Fix** – KernalsEgg
- **df4quitfix** – D7ry
- **SKSE & CommonLibSSE-NG** – ianpatt, behippo, scripthoge, CharmedBaryon
