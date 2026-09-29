#!/usr/bin/env python3
"""
SkyCore FOMOD Packaging Script
Creates a Vortex/MO2-compatible modular installer archive with optional compatibility stubs.
"""

import os
import shutil
import zipfile

SKYCORE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST_DIR = os.path.join(SKYCORE_ROOT, "dist")
STAGE_DIR = os.path.join(DIST_DIR, "stage")
FOMOD_DIR = os.path.join(STAGE_DIR, "fomod")
CORE_DIR = os.path.join(STAGE_DIR, "00_Core")
COMPAT_DIR = os.path.join(STAGE_DIR, "01_Compatibility_po3_Tweaks")
BUILD_DLL = os.path.join(SKYCORE_ROOT, "build", "SkyCore.dll")

VERSION = "0.3.7"

def create_fomod_package():
    if os.path.exists(STAGE_DIR):
        shutil.rmtree(STAGE_DIR)
        
    os.makedirs(FOMOD_DIR, exist_ok=True)
    os.makedirs(os.path.join(CORE_DIR, "SKSE", "Plugins"), exist_ok=True)
    os.makedirs(os.path.join(CORE_DIR, "scripts"), exist_ok=True)
    os.makedirs(os.path.join(CORE_DIR, "Interface", "Translations"), exist_ok=True)
    os.makedirs(os.path.join(CORE_DIR, "MCM", "Config", "SkyCore"), exist_ok=True)
    os.makedirs(os.path.join(CORE_DIR, "MCM", "Settings"), exist_ok=True)
    os.makedirs(os.path.join(COMPAT_DIR, "scripts"), exist_ok=True)

    info_xml = f"""<fomod>
  <Name>Skyrim Engine Core (SEC) - Unified Master Engine Suite</Name>
  <Author>Nicol</Author>
  <Version>{VERSION}</Version>
  <Description>Unified master engine suite for Skyrim AE 1.7.104+. Directly replaces SSE Engine Fixes (Nexus #17230), SSE Display Tweaks (Nexus #34705), powerofthree's Tweaks (Nexus #51073), and Actor Limit Fix (Nexus #32349). Native Papyrus compatibility hooks and optional script stubs ensure seamless compatibility with dependent mods.</Description>
  <Website>https://github.com/OpenChatGit/SkyrimEngineCore</Website>
</fomod>"""

    module_config_xml = f"""<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Skyrim Engine Core (SEC)</moduleName>
  <requiredInstallFiles>
    <folder source="00_Core" destination="" priority="0"/>
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Third-Party Mod Compatibility">
      <optionalFileGroups order="Explicit">
        <group name="Legacy Mod Script Stubs" type="SelectAny">
          <plugins order="Explicit">
            <plugin name="po3_Tweaks Papyrus Script Stub (Recommended)">
              <description>Installs the compiled Papyrus interface script (po3_Tweaks.pex).
Select this if you use mods that query po3_Tweaks via Papyrus (e.g. True Directional Movement, Precision).
SkyCore.dll natively handles all tweak queries at the engine level.</description>
              <files>
                <folder source="01_Compatibility_po3_Tweaks" destination="" priority="1"/>
              </files>
              <typeDescriptor>
                <type name="Recommended"/>
              </typeDescriptor>
            </plugin>
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
</config>"""

    with open(os.path.join(FOMOD_DIR, "info.xml"), "w", encoding="utf-8") as f:
        f.write(info_xml)

    with open(os.path.join(FOMOD_DIR, "ModuleConfig.xml"), "w", encoding="utf-8") as f:
        f.write(module_config_xml)

    default_toml = """# =====================================================================
# Skyrim Engine Core (SEC) - Unified Engine Suite for Skyrim AE 1.7.104+
# Author: Nicol
# All fixes integrated natively in a single, high-performance module.
# =====================================================================

[Display]
bBorderlessFullscreen = true      # Eliminates DXGI occlusion black screens on dual-GPU / laptops
bDynamicHavok = true              # Smooth high-refresh physics scaling (prevents Havok glitching >60 FPS)
iTargetFPS = 0                    # Framerate limit (0 = display native / unlimited)
bDisableVSync = false             # Set true to unlock tearing-free high FPS

[Engine]
bSafeExit = true                  # Instant, clean game shutdown without freezing
bMaxStdIO = true                  # Increases file handle limit to 8192 (prevents missing mesh CTDs)
bCleanSKSECoSaves = true          # Removes orphaned .skse co-saves without matching .ess saves
bFixCellInit = false              # Requires 1.7.104 specific instruction offset
bFixDoublePerkApply = false       # Disabled to prevent actor vtable conflict on save load in 1.7.104
bFixCalendarSkipping = false      # Disabled: uses 1.5.97 specific function IDs
bFixBSLightingAmbientSpecular = false # Shader patch disabled for 1.7.104 compatibility
bFixEffectShaderZBuffer = false   # Shader patch disabled for 1.7.104 compatibility
bFixDistantRefLoadCrash = false   # Disabled: prevents fade-node offset conflict in 1.7.104
bFixMemoryAccess = false          # Disabled: uses 1.6.640 specific assembly offset
bFixGlobalTime = true             # Fixes slow-motion camera and animation synchronization
bAltF4QuitFix = true              # Allows cleanly closing Skyrim immediately with Alt+F4 (df4quitfix)
bFixActorLimit = true             # Increases NPC movement cap (256) and face morph limit (64)
iActorMoverLimit = 256            # Max actively moving actors in loaded cell (Vanilla = 128)
iActorMorphLimit = 64             # Max simultaneous facial expressions / lip-sync (Vanilla = 10)

[Gameplay]
bDynamicMerchantGold = true       # Händler-Gold skaliert organisch mit Spielerlevel & Redekunst
iMerchantGoldBase = 750           # Mindest-Goldbestand für Händler
iMerchantGoldPerLevel = 25        # Zusätzliches Gold pro Spielerlevel
iMerchantGoldPerSpeech = 10       # Zusätzliches Gold pro Redekunst-Punkt
iMerchantGoldCap = 25000          # Obergrenze (verhindert 32k Überlauf-Bug)

[Diagnostics]
bEnableCrashLogging = true        # Built-in lightweight crash handler with register and module dump
bVerboseLogging = false           # Detailed debug logs in Documents\\My Games\\Skyrim Special Edition\\SKSE\\SkyCore.log
"""
    with open(os.path.join(CORE_DIR, "SKSE", "Plugins", "SkyCore.toml"), "w", encoding="utf-8") as f:
        f.write(default_toml)

    # Copy built native DLL
    build_dir = os.path.join(SKYCORE_ROOT, "build")
    game_plugins_dir = os.path.join(SKYCORE_ROOT, "..", "Data", "SKSE", "Plugins")
    for dll_name in ["SkyCore.dll"]:
        src_dll = os.path.join(build_dir, dll_name)
        if not os.path.exists(src_dll):
            src_dll = os.path.join(game_plugins_dir, dll_name)
        if os.path.exists(src_dll):
            dest_dll = os.path.join(CORE_DIR, "SKSE", "Plugins", dll_name)
            shutil.copy2(src_dll, dest_dll)
            print(f"[+] Successfully copied: {dll_name} ({os.path.getsize(dest_dll)} bytes)")
        else:
            print(f"[!] Warning: Built DLL not found at {src_dll}")

    # Copy SkyCore.esp
    esp_src = os.path.join(SKYCORE_ROOT, "assets", "SkyCore.esp")
    if not os.path.exists(esp_src):
        esp_src = os.path.join(SKYCORE_ROOT, "..", "Data", "SkyCore.esp")
    if os.path.exists(esp_src):
        shutil.copy2(esp_src, os.path.join(CORE_DIR, "SkyCore.esp"))
        print("[+] Successfully bundled SkyCore.esp")

    # Copy Papyrus scripts
    for pex_name in ["SkyCore_MCM.pex"]:
        pex_src = os.path.join(SKYCORE_ROOT, "assets", "scripts", pex_name)
        if not os.path.exists(pex_src):
            pex_src = os.path.join(SKYCORE_ROOT, "..", "Data", "scripts", pex_name)
        if os.path.exists(pex_src):
            shutil.copy2(pex_src, os.path.join(CORE_DIR, "scripts", pex_name))
            print(f"[+] Successfully bundled scripts/{pex_name}")

    # Copy MCM Helper config and settings
    mcm_config_dir = os.path.join(CORE_DIR, "MCM", "Config", "SkyCore")
    mcm_settings_dir = os.path.join(CORE_DIR, "MCM", "Settings")
    for filename in ["config.json", "settings.ini"]:
        src = os.path.join(SKYCORE_ROOT, "assets", "MCM", "Config", "SkyCore", filename)
        if not os.path.exists(src):
            src = os.path.join(SKYCORE_ROOT, "..", "Data", "MCM", "Config", "SkyCore", filename)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(mcm_config_dir, filename))
            print(f"[+] Successfully bundled MCM/Config/SkyCore/{filename}")

    src_settings = os.path.join(SKYCORE_ROOT, "assets", "MCM", "Settings", "SkyCore.ini")
    if not os.path.exists(src_settings):
        src_settings = os.path.join(SKYCORE_ROOT, "..", "Data", "MCM", "Settings", "SkyCore.ini")
    if os.path.exists(src_settings):
        shutil.copy2(src_settings, os.path.join(mcm_settings_dir, "SkyCore.ini"))
        print("[+] Successfully bundled MCM/Settings/SkyCore.ini")

    # Copy Interface translations
    trans_src_dir = os.path.join(SKYCORE_ROOT, "assets", "Interface", "Translations")
    dest_trans_dir = os.path.join(CORE_DIR, "Interface", "Translations")
    if os.path.exists(trans_src_dir):
        for trans_file in os.listdir(trans_src_dir):
            shutil.copy2(os.path.join(trans_src_dir, trans_file), os.path.join(dest_trans_dir, trans_file))
            print(f"[+] Successfully bundled Translations/{trans_file}")

    # Copy optional po3_Tweaks compatibility script stub
    compat_script_src = os.path.join(SKYCORE_ROOT, "assets", "compat", "scripts", "po3_Tweaks.pex")
    if not os.path.exists(compat_script_src):
        compat_script_src = os.path.join(SKYCORE_ROOT, "3rd-party", "po3-Tweaks", "Skyrim", "Data", "scripts", "po3_Tweaks.pex")
    if os.path.exists(compat_script_src):
        shutil.copy2(compat_script_src, os.path.join(COMPAT_DIR, "scripts", "po3_Tweaks.pex"))
        print("[+] Successfully bundled optional po3_Tweaks.pex compatibility stub")

    # Create zip archive for Vortex / Mod Organizer 2
    zip_path = os.path.join(DIST_DIR, f"SkyrimEngineCore-v{VERSION}-AE.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(STAGE_DIR):
            for file in files:
                abs_file = os.path.join(root, file)
                rel_path = os.path.relpath(abs_file, STAGE_DIR)
                zf.write(abs_file, rel_path)

    print(f"[+] Vortex-ready modular FOMOD archive created successfully:")
    print(f"    -> {zip_path} ({os.path.getsize(zip_path)} bytes)")

if __name__ == "__main__":
    create_fomod_package()
