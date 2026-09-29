#!/usr/bin/env python3
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
TRANS_DIR = os.path.join(ROOT_DIR, "assets", "Interface", "Translations")
os.makedirs(TRANS_DIR, exist_ok=True)

EN_TRANSLATIONS = {
    "$SkyCore_ModName": "SkyCore (SEC)",
    "$SkyCore_Page_Display": "Display & FPS",
    "$SkyCore_Header_FPS": "FPS Counter Settings",
    "$SkyCore_ShowFPS": "Show FPS Counter",
    "$SkyCore_ShowFPS_Help": "Toggle the small FPS display in the top-left corner on or off.",
    "$SkyCore_ToggleKey": "Toggle FPS Hotkey",
    "$SkyCore_ToggleKey_Help": "Press any key to assign the hotkey for showing/hiding the FPS counter. Default is Insert.",
    "$SkyCore_Header_Display": "Display Engine & Refresh Rate",
    "$SkyCore_DynamicHavok": "Dynamic Havok Physics",
    "$SkyCore_DynamicHavok_Help": "Dynamically scales physics time step to eliminate physics glitches over 60 FPS.",
    "$SkyCore_Borderless": "Borderless Fullscreen",
    "$SkyCore_Borderless_Help": "Eliminates DXGI occlusion black screens and enables seamless Alt-Tab.",
    "$SkyCore_DisableVSync": "Disable V-Sync",
    "$SkyCore_DisableVSync_Help": "Disables V-Sync to achieve maximum framerate without input latency.",
    "$SkyCore_TargetFPS": "Target Framerate Limit",
    "$SkyCore_TargetFPS_Help": "Sets the engine framerate cap. Set to 0 for unlimited / native display refresh rate.",
    "$SkyCore_Page_Engine": "Engine & Stability",
    "$SkyCore_Header_EngineFixes": "Engine Core Fixes",
    "$SkyCore_MaxStdIO": "MaxStdIO (8192 File Handles)",
    "$SkyCore_MaxStdIO_Help": "Increases stdio file handle limit from 512 to 8192 to prevent missing mesh/texture CTDs with large load orders.",
    "$SkyCore_SafeExit": "Safe Exit",
    "$SkyCore_SafeExit_Help": "Prevents the game from hanging or freezing on desktop exit.",
    "$SkyCore_CleanSKSECoSaves": "Clean Orphaned Co-Saves",
    "$SkyCore_CleanSKSECoSaves_Help": "Automatically cleans up orphaned .skse co-save files without matching .ess saves.",
    "$SkyCore_CellInit": "Cell Init Crash Fix",
    "$SkyCore_CellInit_Help": "Resolves uninitialized location form IDs to pointers when loading cells (prevents rare cell transition CTDs).",
    "$SkyCore_EffectShader": "Effect Shader Z-Buffer Fix",
    "$SkyCore_EffectShader_Help": "Disables Z-write on particle shaders to prevent depth buffer fighting and visual artifacts.",
    "$SkyCore_BSTempEffect": "BSTempEffect NiRTTI Fix",
    "$SkyCore_BSTempEffect_Help": "Corrects base NiRTTI linkage on temporary effects to prevent invalid cast CTDs.",
    "$SkyCore_AltF4Quit": "Alt+F4 Quick Quit",
    "$SkyCore_AltF4Quit_Help": "Allows cleanly closing the Skyrim process immediately by pressing Alt + F4 at any time.",
    "$SkyCore_LipSyncLimit": "FaceGen Lip-Sync Morph Expansion (64 Morphs)",
    "$SkyCore_LipSyncLimit_Help": "Expands simultaneous dialogue facial morph / lip-sync cap from 10 to 64 actors in engine memory.",
    "$SkyCore_DynamicMerchantGold": "Dynamic Merchant Gold",
    "$SkyCore_DynamicMerchantGold_Help": "Dynamically scales merchant gold with player level and Speechcraft, preventing the 32k barter overflow bug.",
    "$SkyCore_Header_Diagnostics": "Diagnostics",
    "$SkyCore_CrashLogging": "Active VEH Crash Diagnostics",
    "$SkyCore_CrashLogging_Help": "Logs stack backtraces and x64 CPU register dumps to Documents/My Games/Skyrim Special Edition/SKSE/SkyCore.log."
}

DE_TRANSLATIONS = {
    "$SkyCore_ModName": "SkyCore (SEC)",
    "$SkyCore_Page_Display": "Grafik & FPS",
    "$SkyCore_Header_FPS": "FPS-Anzeige Einstellungen",
    "$SkyCore_ShowFPS": "FPS-Anzeige aktivieren",
    "$SkyCore_ShowFPS_Help": "Schaltet die kleine FPS-Anzeige oben links im Bild ein oder aus.",
    "$SkyCore_ToggleKey": "FPS-Taste belegen",
    "$SkyCore_ToggleKey_Help": "Taste drücken, um den Hotkey zum Ein-/Ausblenden festzulegen. Standard: Einfügen (Insert).",
    "$SkyCore_Header_Display": "Grafik-Engine & Bildwiederholrate",
    "$SkyCore_DynamicHavok": "Dynamische Havok-Physik",
    "$SkyCore_DynamicHavok_Help": "Passt die Physik dynamisch an hohe Bildraten an, um Physik-Bugs bei über 60 FPS zu verhindern.",
    "$SkyCore_Borderless": "Rahmenloses Vollbild",
    "$SkyCore_Borderless_Help": "Verhindert DXGI-Blackscreens und ermöglicht nahtloses Wechseln mit Alt+Tab.",
    "$SkyCore_DisableVSync": "V-Sync deaktivieren",
    "$SkyCore_DisableVSync_Help": "Deaktiviert V-Sync für maximale Bildrate ohne Eingabeverzögerung (Input Lag).",
    "$SkyCore_TargetFPS": "FPS-Limitierung",
    "$SkyCore_TargetFPS_Help": "Setzt das Framerate-Limit. 0 bedeutet unbegrenzt bzw. native Bildwiederholrate des Monitors.",
    "$SkyCore_Page_Engine": "Engine & Stabilität",
    "$SkyCore_Header_EngineFixes": "Kern-Engine-Fixes",
    "$SkyCore_MaxStdIO": "MaxStdIO (8192 Datei-Handles)",
    "$SkyCore_MaxStdIO_Help": "Erhöht das Dateilimit von 512 auf 8192, um Abstürze durch fehlende Meshes/Texturen bei vielen Mods zu verhindern.",
    "$SkyCore_SafeExit": "Sicheres Beenden (Safe Exit)",
    "$SkyCore_SafeExit_Help": "Verhindert, dass Skyrim beim Beenden zum Desktop einfriert oder hängen bleibt.",
    "$SkyCore_CleanSKSECoSaves": "Verwaiste Co-Saves bereinigen",
    "$SkyCore_CleanSKSECoSaves_Help": "Entfernt automatisch verwaiste .skse Co-Save-Dateien ohne zugehörigen Spielstand (.ess).",
    "$SkyCore_CellInit": "Zellen-Ladeabsturz-Fix (Cell Init)",
    "$SkyCore_CellInit_Help": "Löst uninitialisierte Location-IDs beim Laden von Zellen auf (verhindert seltene Übergangs-CTDs).",
    "$SkyCore_EffectShader": "Effekt-Shader Tiefenpuffer-Fix",
    "$SkyCore_EffectShader_Help": "Deaktiviert Z-Write auf Partikel-Shadern zur Beseitigung von Z-Fighting und Grafikflackern.",
    "$SkyCore_BSTempEffect": "BSTempEffect NiRTTI Fix",
    "$SkyCore_BSTempEffect_Help": "Korrigiert die NiRTTI-Verknüpfung bei temporären Effekten gegen Typ-Konvertierungs-Abstürze.",
    "$SkyCore_AltF4Quit": "Sofortiges Beenden mit Alt+F4",
    "$SkyCore_AltF4Quit_Help": "Ermöglicht das sofortige, saubere Schließen von Skyrim mit Alt+F4 zu jedem Zeitpunkt.",
    "$SkyCore_LipSyncLimit": "Lippensynchronisations-Erweiterung (64 Morphs)",
    "$SkyCore_LipSyncLimit_Help": "Erhöht das Limit für gleichzeitige Gesichts-Morphs und Dialog-Lippensynchronisation von 10 auf 64 Akteure.",
    "$SkyCore_DynamicMerchantGold": "Dynamisches Händler-Gold",
    "$SkyCore_DynamicMerchantGold_Help": "Skaliert das Händler-Gold dynamisch mit Level & Redekunst und verhindert den 32k-Überlauf-Bug.",
    "$SkyCore_Header_Diagnostics": "Diagnose",
    "$SkyCore_CrashLogging": "Aktive VEH-Absturzdiagnose",
    "$SkyCore_CrashLogging_Help": "Protokolliert Stack-Backtraces und CPU-Register in Documents/My Games/Skyrim Special Edition/SKSE/SkyCore.log."
}

def write_translation(filename, mapping):
    filepath = os.path.join(TRANS_DIR, filename)
    with open(filepath, "w", encoding="utf-16-le") as f:
        # Write UTF-16 LE BOM
        f.write("\ufeff")
        for key, val in mapping.items():
            f.write(f"{key}\t{val}\r\n")
    print(f"[+] Written {len(mapping)} keys to {filepath} ({os.path.getsize(filepath)} bytes)")

if __name__ == "__main__":
    write_translation("SkyCore_ENGLISH.txt", EN_TRANSLATIONS)
    write_translation("SkyCore_GERMAN.txt", DE_TRANSLATIONS)
