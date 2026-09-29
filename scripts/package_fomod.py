#!/usr/bin/env python3
"""
SkyCore FOMOD Packaging Script
Creates a Vortex-compatible 1-click installer archive and stages the mod files.
"""

import os
import shutil
import zipfile

SKYCORE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST_DIR = os.path.join(SKYCORE_ROOT, "dist")
STAGE_DIR = os.path.join(DIST_DIR, "stage")
FOMOD_DIR = os.path.join(STAGE_DIR, "fomod")
PLUGINS_DIR = os.path.join(STAGE_DIR, "SKSE", "Plugins")
BUILD_DLL = os.path.join(SKYCORE_ROOT, "build", "SkyCore.dll")

def create_fomod_package():
    if os.path.exists(STAGE_DIR):
        shutil.rmtree(STAGE_DIR)
    os.makedirs(FOMOD_DIR, exist_ok=True)
    os.makedirs(PLUGINS_DIR, exist_ok=True)

    info_xml = """<fomod>
  <Name>Skyrim Engine Core (SEC) - Unified Engine, Display &amp; Tweaks Suite</Name>
  <Author>Nicol</Author>
  <Version>0.3.6</Version>
  <Description>Unified master engine suite for Skyrim AE 1.7.104+. Directly replaces SSE Engine Fixes (Nexus #17230), SSE Display Tweaks (Nexus #34705), powerofthree's Tweaks (Nexus #51073), and Actor Limit Fix (Nexus #32349). Provides native Papyrus compatibility hooks so third-party mods operate seamlessly without legacy DLL dependencies.</Description>
  <Website>https://github.com/OpenChatGit/SkyrimEngineCore</Website>
</fomod>"""

    module_config_xml = """<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Skyrim Engine Core (SEC) - Unified Engine, Display &amp; Tweaks Suite</moduleName>
  <installSteps order="Explicit">
    <installStep name="Unified Engine Architecture">
      <optionalFileGroups order="Explicit">
        <group name="Integrated Engine &amp; Tweaks Components" type="SelectAny">
          <plugins order="Explicit">
            <plugin name="Skyrim Engine Core (Unified Master Suite)">
              <description>Installs the complete unified engine suite. Natively replaces:
- SSE Engine Fixes (Nexus #17230): MaxStdIO 8192 file handles, SafeExit, Memory patch
- SSE Display Tweaks (Nexus #34705): High-refresh Havok physics, borderless fullscreen
- powerofthree's Tweaks (Nexus #51073): Native Papyrus IsTweakInstalled hook
- Actor Limit Fix (Nexus #32349): 256 NPC mover limit &amp; 64 lip-sync face morphs

VORTEX NOTE: If any other mod asks for SSE Engine Fixes or po3_Tweaks, you can safely select 'Dismiss / Ignore' in Vortex.</description>
              <image path=""/>
              <typeHandling>
                <defaultType name="Required"/>
              </typeHandling>
              <files>
                <file source="SkyCore.esp" destination="SkyCore.esp" priority="0"/>
                <folder source="SKSE" destination="SKSE" priority="0"/>
                <folder source="scripts" destination="scripts" priority="0"/>
                <folder source="MCM" destination="MCM" priority="0"/>
              </files>
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
# SkyCore - Unified Engine & Display Suite for Skyrim AE 1.7.104+
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
    with open(os.path.join(PLUGINS_DIR, "SkyCore.toml"), "w", encoding="utf-8") as f:
        f.write(default_toml)

    # Copy built native DLL
    build_dir = os.path.join(SKYCORE_ROOT, "build")
    game_plugins_dir = os.path.join(SKYCORE_ROOT, "..", "Data", "SKSE", "Plugins")
    for dll_name in ["SkyCore.dll"]:
        src_dll = os.path.join(build_dir, dll_name)
        if not os.path.exists(src_dll):
            src_dll = os.path.join(game_plugins_dir, dll_name)
        if os.path.exists(src_dll):
            dest_dll = os.path.join(PLUGINS_DIR, dll_name)
            shutil.copy2(src_dll, dest_dll)
            print(f"[+] Successfully copied: {dll_name} ({os.path.getsize(dest_dll)} bytes)")
        else:
            print(f"[!] Warning: Built DLL not found at {src_dll}")

    # Copy SkyCore.esp
    esp_src = os.path.join(SKYCORE_ROOT, "assets", "SkyCore.esp")
    if not os.path.exists(esp_src):
        esp_src = os.path.join(SKYCORE_ROOT, "..", "Data", "SkyCore.esp")
    if os.path.exists(esp_src):
        shutil.copy2(esp_src, os.path.join(STAGE_DIR, "SkyCore.esp"))
        print("[+] Successfully bundled SkyCore.esp")

    # Copy Papyrus scripts
    stage_scripts = os.path.join(STAGE_DIR, "scripts")
    os.makedirs(stage_scripts, exist_ok=True)
    for pex_name in ["SkyCore_MCM.pex"]:
        pex_src = os.path.join(SKYCORE_ROOT, "assets", "scripts", pex_name)
        if not os.path.exists(pex_src):
            pex_src = os.path.join(SKYCORE_ROOT, "..", "Data", "scripts", pex_name)
        if os.path.exists(pex_src):
            shutil.copy2(pex_src, os.path.join(stage_scripts, pex_name))
            print(f"[+] Successfully bundled scripts/{pex_name}")

    # Copy MCM Helper config and settings
    mcm_config_dir = os.path.join(STAGE_DIR, "MCM", "Config", "SkyCore")
    mcm_settings_dir = os.path.join(STAGE_DIR, "MCM", "Settings")
    os.makedirs(mcm_config_dir, exist_ok=True)
    os.makedirs(mcm_settings_dir, exist_ok=True)
    
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

    # Create zip archive for Vortex / Mod Organizer 2
    zip_path = os.path.join(DIST_DIR, "SkyrimEngineCore-v0.3.6-AE.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(STAGE_DIR):
            for file in files:
                abs_file = os.path.join(root, file)
                rel_path = os.path.relpath(abs_file, STAGE_DIR)
                zf.write(abs_file, rel_path)

    print(f"[+] Vortex-ready FOMOD archive created successfully:")
    print(f"    -> {zip_path} ({os.path.getsize(zip_path)} bytes)")

if __name__ == "__main__":
    create_fomod_package()
