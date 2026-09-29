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

## ✦ Feature Comparison Matrix

| Feature / Fix | Legacy SSE Engine Fixes | Legacy SSE Display Tweaks | Legacy po3_Tweaks | **Skyrim Engine Core (SEC)** |
| :--- | :---: | :---: | :---: | :---: |
| **MaxStdIO File Descriptors (8192)** |  (Part 1 + 2) | ❌ | ❌ | ** Native C++ (Single DLL)** |
| **SafeExit (Instant Desktop Exit)** |  | ❌ | ❌ | ** Integrated** |
| **Dynamic Havok High-FPS Physics** | ❌ |  | ❌ | ** Integrated** |
| **Borderless Fullscreen DXGI Fix** | ❌ |  | ❌ | ** Integrated** |
| **V-Sync Override & Frame Limiter** | ❌ |  | ❌ | ** Integrated** |
| **Actor Limit (256 Movers / 64 Morphs)** | ❌ | ❌ | ❌ | ** Integrated** |
| **Dynamic Merchant Gold Scaling** | ❌ | ❌ | ❌ | ** Integrated** |
| **Clean SKSE Co-Saves** |  | ❌ | ❌ | ** Integrated** |
| **po3_Tweaks Papyrus Hook (`IsTweakInstalled`)**| ❌ | ❌ |  | ** Native C++ Emulation** |
| **MCM Helper In-Game Config** | ❌ | ❌ | ❌ | ** (EN / DE localized)** |
| **VEH Lightweight Crash Logger** | ❌ | ❌ | ❌ | ** Integrated** |
| **Skyrim AE 1.7.104+ Compatibility** | ⚠️ Partial | ⚠️ Partial | ⚠️ Partial | ** 100% Native & Tested** |

---

## ✦ Installation Guide

### Vortex Users
1. Download `SkyrimEngineCore-v0.3.7-AE.zip` and drop it into Vortex (or click *Install with Mod Manager*).
2. The FOMOD installer will automatically select the **Core Engine Suite** and the recommended **po3_Tweaks Papyrus Script Stub**.
3. Enable and Deploy the mod.
4. **Vortex Dependency FAQ:** If Vortex displays a notification that another mod recommends *SSE Engine Fixes* or *powerofthree's Tweaks*, simply click **"Dismiss / Don't remind me"** (or create a rule *"Replaced by Skyrim Engine Core"*). Skyrim Engine Core satisfies these dependencies directly at the engine level.

### Mod Organizer 2 (MO2) Users
1. Install the archive via the *Install Mod* icon or drag-and-drop.
2. Confirm the FOMOD options in the installer dialog.
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

All research, open-source logic, and memory reverse-engineering utilized in this project operate strictly within the legal scope of the **GNU General Public License v3.0** and the **MIT License**. The rights granted by the original copyright holders under these licenses are legally binding and cannot be diminished or superseded by third parties.

Acknowledgements to the open-source reverse-engineering community:
- aers, Nukem, Ryan (fudgyduff)
- SlavicPotato
- powerofthree
- KernalsEgg
- D7ry
- ianpatt, behippo, scripthoge, CharmedBaryon
