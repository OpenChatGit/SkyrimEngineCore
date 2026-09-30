# Skyrim Engine Core (SEC)

> Unified, modern C++23 native SKSE plugin consolidating engine fixes, display enhancements, stability patches, and performance tweaks for Skyrim Special Edition & Anniversary Edition (1.6.1170+ / 1.7.104+).

---

## ✦ Features

- **File Handle Expansion (8,192 MaxStdIO):** Eliminates missing-mesh and texture loading crashes in heavy modlists by multiplying available file descriptors.
- **SafeExit & Alt+F4 Quick Termination:** Prevents freeze-on-exit and allows closing Skyrim cleanly and immediately with Alt+F4.
- **Automated Co-Save Purge:** Automatically detects and purges orphaned `.skse` co-saves without matching `.ess` save files.
- **Lip-Sync Expansion:** Expands simultaneous dialogue facial morph / lip-sync cap to 64 actors (engine INI limit).
- **High-Refresh Dynamic Havok Physics:** High-framerate physics timescale scaling (>60 FPS / 144Hz / 240Hz) preventing physics glitching and camera stutter.
- **Engine Integrity Patches:** Fixes temporary effect NiRTTI inheritance memory crashes and particle shader depth writing.
- **Native Diagnostics & Self-Test:** Automatic startup self-test report in `SkyCore.log` and lightweight VEH crash handler with register and module dump.
- **Native FPS Counter & In-Game OSD:** Dedicated click-through layered Win32 overlay with DXGI swapchain timing, hotkey toggle (Insert), and SkyUI MCM configuration.

---

## ✦ Features Overview

| Feature / Fix | Skyrim Engine Core (SEC) | Description |
| :--- | :---: | :--- |
| **MaxStdIO File Descriptors (8192)** |  **Native C++** | Eliminates file handle exhaustion crashes |
| **SafeExit (Instant Desktop Exit)** |  **Integrated** | Subclassed Win32 message handling for clean exit |
| **Dynamic Havok High-FPS Physics** |  **Integrated** | Physics delta unlocked up to 240 FPS |
| **Native Borderless Fullscreen** |  **Integrated** | Win32 borderless monitor bounds positioning |
| **V-Sync Override & Frame Timing** |  **Integrated** | DXGI SwapChain Present hook for unlocked tearing |
| **Native In-Game FPS Counter** |  **Integrated** | Transparent click-through HUD overlay (Insert toggle) |
| **Dynamic Merchant Gold Scaling** |  **Integrated** | Organically scales merchant gold with player speech |
| **Clean SKSE Co-Saves** |  **Integrated** | Automatically purges orphaned .skse save files |
| **po3_Tweaks Papyrus Hook** |  **Native C++** | Emulates `po3_Tweaks.IsTweakInstalled` for dependent mods |
| **MCM Helper In-Game Config** |  **Included** | Full SkyUI MCM with English and German translations |
| **VEH Lightweight Crash Logger** |  **Integrated** | Vectored exception handling register & module dump |
| **Skyrim AE 1.7.104+ Compatibility** |  **100% Native** | Tested and verified on the latest Steam runtime |

---

## ✦ Installation Guide

### Vortex Users
1. Download `SkyrimEngineCore-v0.4.0-AE.zip` and drop it into Vortex (or click *Install with Mod Manager*).
2. The FOMOD installer will install the **Core Engine Suite**.
3. Enable and Deploy the mod.
4. **Vortex Dependency FAQ:** If Vortex displays a notification that another mod recommends *SSE Engine Fixes* or *powerofthree's Tweaks*, simply click **"Dismiss / Don't remind me"** (or create a rule *"Replaced by Skyrim Engine Core"*). Skyrim Engine Core satisfies these dependencies directly at the engine level.

### Mod Organizer 2 (MO2) Users
1. Install the archive via the *Install Mod* icon or drag-and-drop.
2. Confirm the installation in the dialog.
3. Check the box to activate the mod in your left pane load order.

---

## ✦ Configuration & In-Game MCM

Skyrim Engine Core comes pre-configured with safe, optimal defaults:
- Configuration file: `Data/SKSE/Plugins/SkyCore.toml`
- In-Game MCM: In the pause menu under **Mod Configuration > SkyCore**, you can toggle features, adjust FPS limits, and assign hotkeys in real-time.
- Supports **English** and **German** interface translations out of the box.

---

## ✦ License & Legal Compliance

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.

All research, open-source logic, and memory reverse-engineering utilized in this project operate strictly within the legal scope of the **GNU General Public License v3.0** and the **MIT License**.

Acknowledgements & Notices:
- **SSE Engine Fixes** (MIT License) - Copyright (c) 2018-2021 aers, Nukem, Ryan (fudgyduff). Portions adapted and modified.
- **powerofthree's Tweaks** (GPL-3.0 License) - Copyright (c) powerofthree. Portions adapted and modified.
- **SKSE Team & CommonLibSSE-NG** - Copyright (c) ianpatt, behippo, scripthoge, CharmedBaryon.
- Full license terms and verbatim notices for all dependencies are provided in the `LICENSE` file.
