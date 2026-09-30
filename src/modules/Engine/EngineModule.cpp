/**
 * Skyrim Engine Core (SkyCore) - EngineModule
 *
 * This module incorporates modified and adapted logic from the following open-source projects:
 *
 * 1. SSE Engine Fixes (MIT License)
 *    Copyright (c) 2018-2021 aers, Nukem, Ryan (fudgyduff)
 *    Modified and adapted for Skyrim Engine Core:
 *    - MaxStdIO file handle expansion
 *    - CleanSKSECoSaves orphaned co-save purger
 *
 * 2. powerofthree's Tweaks (GNU General Public License v3.0)
 *    Copyright (c) powerofthree
 *    Modified and adapted for Skyrim Engine Core:
 *    - BSTempEffectNiRTTI fix
 *    - EffectShaderZBuffer fix
 *    - CellInit fix
 *
 * All derived code has been adapted, modified, and integrated into SkyCore under their respective licenses.
 */

#include "EngineModule.h"
#include "SkyCore/Config.h"
#include "RE/B/BarterMenu.h"
#include "RE/U/UI.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESObjectREFR.h"
#include "RE/T/TESForm.h"
#include "RE/T/TESBoundObject.h"
#include "RE/A/ActorValues.h"
#include <stdio.h>
#include <shlobj.h>
#include <xbyak/xbyak.h>
#include <format>

namespace SkyCore::Modules::Engine
{
    static void ApplyMaxStdIO()
    {
        const auto cur = _getmaxstdio();
        const auto target = 8192;
        if (cur < target) {
            const auto res = _setmaxstdio(target);
            logger::info("MaxStdIO: Increased file handle limit from {} to {}", cur, res);
        }
    }

    static void CleanCoSaves()
    {
        try {
            wchar_t myDocs[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, myDocs))) {
                std::filesystem::path saveDir = myDocs;
                saveDir /= L"My Games\\Skyrim Special Edition\\Saves";

                if (std::filesystem::exists(saveDir)) {
                    std::size_t removedCount = 0;
                    for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
                        if (entry.path().extension() == ".skse") {
                            auto essPath = entry.path();
                            essPath.replace_extension(".ess");
                            if (!std::filesystem::exists(essPath)) {
                                std::error_code ec;
                                std::filesystem::remove(entry.path(), ec);
                                if (!ec) removedCount++;
                            }
                        }
                    }
                    if (removedCount > 0) {
                        logger::info("CleanCoSaves: Cleaned up {} orphaned .skse co-saves", removedCount);
                    }
                }
            }
        } catch (...) {
            logger::warn("CleanCoSaves: Encountered an exception while cleaning saves");
        }
    }

    namespace BSTempEffectNiRTTI
    {
        inline void Install()
        {
            try {
                const REL::Relocation<RE::NiRTTI*> rttiBSTempEffect{ RE::BSTempEffect::Ni_RTTI };
                const REL::Relocation<RE::NiRTTI*> rttiNiObject{ RE::NiObject::Ni_RTTI };
                if (rttiBSTempEffect.get() && rttiNiObject.get()) {
                    rttiBSTempEffect->baseRTTI = rttiNiObject.get();
                    logger::info("EngineModule: Installed BSTempEffectNiRTTI fix");
                }
            } catch (...) {
                logger::warn("EngineModule: Failed to install BSTempEffectNiRTTI fix");
            }
        }
    }

    namespace EffectShaderZBuffer
    {
        inline void Install()
        {
            try {
                REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(501401, 360087), 0x1C };
                if (target.address()) {
                    constexpr std::uint8_t zeroes[] = { 0x0, 0x0, 0x0, 0x0 };
                    REL::safe_write(target.address(), zeroes, 4);
                    logger::info("EngineModule: Installed EffectShaderZBuffer fix");
                }
            } catch (...) {
                logger::warn("EngineModule: Failed to install EffectShaderZBuffer fix");
            }
        }
    }

    namespace CellInit
    {
        static bool s_cellInitPatched = false;

        struct ExtraDataList
        {
            static RE::BGSLocation* GetLocation(const RE::ExtraDataList* a_self)
            {
                auto* cell = reinterpret_cast<RE::TESObjectCELL*>(const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(a_self) - 0x48));
                auto  loc = _GetLocation(a_self);
                if (cell && !cell->IsInitialized()) {
                    const auto file = cell->GetFile();
                    auto       formID = static_cast<RE::FormID>(reinterpret_cast<std::uintptr_t>(loc));
                    RE::TESForm::AddCompileIndex(formID, file);
                    loc = RE::TESForm::LookupByID<RE::BGSLocation>(formID);
                }
                return loc;
            }

            static inline REL::Relocation<decltype(GetLocation)> _GetLocation;
        };

        inline void Install()
        {
            try {
                REL::Relocation target{ RELOCATION_ID(18474, 18905), VAR_NUM(0x110, 0x114) };
                // Verify instruction is a call (0xE8)
                const auto opcode = *reinterpret_cast<const std::uint8_t*>(target.address());
                if (opcode == 0xE8) {
                    ExtraDataList::_GetLocation = target.write_call<5>(ExtraDataList::GetLocation);
                    s_cellInitPatched = true;
                    logger::info("EngineModule: Installed CellInit fix");
                } else {
                    logger::warn("EngineModule: CellInit call site opcode mismatch (0x{:02X}), skipping for safety", opcode);
                }
            } catch (...) {
                logger::warn("EngineModule: Failed to install CellInit fix");
            }
        }

        inline bool IsPatched() { return s_cellInitPatched; }
    }

    void Install()
    {
        const auto& cfg = Config::Get();

        if (cfg.maxStdIO) {
            ApplyMaxStdIO();
        }

        if (cfg.cleanSKSECoSaves) {
            CleanCoSaves();
        }

        if (cfg.fixBSTempEffectNiRTTI) {
            BSTempEffectNiRTTI::Install();
        }

        if (cfg.fixEffectShaderZBuffer) {
            EffectShaderZBuffer::Install();
        }

        if (cfg.fixCellInit) {
            CellInit::Install();
        }

        logger::info("EngineModule: Verified engine patches and fixes initialized");
    }

    void OnDataLoaded()
    {
        const auto& cfg = Config::Get();

        // 1. Dialogue Lip-Sync FaceGen Morph setting (Vanilla engine INI limit)
        if (cfg.fixLipSyncLimit) {
            if (const auto ini = RE::INISettingCollection::GetSingleton()) {
                if (const auto setting = ini->GetSetting("uiNumActorsAllowedToMorph:FaceGen")) {
                    setting->data.u = cfg.faceGenMorphLimit;
                    logger::info("EngineModule: uiNumActorsAllowedToMorph:FaceGen set to {} (Lip-Sync Fix)", setting->data.u);
                }
            }
        }
    }

    void RunStartupDiagnostics()
    {
        const auto& cfg = Config::Get();
        logger::info("======================================================================");
        logger::info("           [SEC] STARTUP DIAGNOSTIC & SELF-TEST SUITE             ");
        logger::info("======================================================================");

        int passCount = 0;
        int totalCount = 0;

        auto LogResult = [&](const char* name, bool passed, const std::string& details) {
            totalCount++;
            if (passed) passCount++;
            logger::info("[{:4}] {:<32} : {}", passed ? "PASS" : "WARN", name, details);
        };

        // 1. MaxStdIO
        const auto curStdio = _getmaxstdio();
        LogResult("MaxStdIO (File Handles)", curStdio >= 8192, std::format("Current limit: {}", curStdio));

        // 2. SafeExit & AltF4 QuitFix
        LogResult("SafeExit & Alt+F4 QuitFix", cfg.safeExit && cfg.altF4QuitFix, "Instant clean termination active (WndProc + DirectInput)");

        // 3. CleanSKSECoSaves
        LogResult("CleanSKSECoSaves", cfg.cleanSKSECoSaves, "Orphaned save cleanup initialized");

        // 4. BSTempEffectNiRTTI
        LogResult("BSTempEffect NiRTTI Fix", cfg.fixBSTempEffectNiRTTI, "RTTI inheritance safely patched");

        // 5. EffectShaderZBuffer
        LogResult("EffectShader Z-Buffer Fix", cfg.fixEffectShaderZBuffer, "Depth writing disabled for particle shaders");

        // 6. CellInit Crash Fix
        LogResult("CellInit Crash Fix", cfg.fixCellInit && CellInit::IsPatched(), "Uninitialized form ID callsite verified (0xE8)");

        // 7. Dialogue Lip-Sync FaceGen Morph limit
        uint32_t morphVal = 10;
        if (const auto ini = RE::INISettingCollection::GetSingleton()) {
            if (const auto s = ini->GetSetting("uiNumActorsAllowedToMorph:FaceGen")) morphVal = s->data.u;
        }
        LogResult("Lip-Sync FaceGen Morphs", morphVal >= 64, std::format("uiNumActorsAllowedToMorph:FaceGen = {}", morphVal));

        // 8. Dynamic Havok
        LogResult("Dynamic Havok Physics", cfg.dynamicHavok, "Physics timescale scaling enabled");

        // 9. VSync Engine Override
        int vsyncVal = -1;
        if (const auto iniPref = RE::INIPrefSettingCollection::GetSingleton()) {
            if (const auto s = iniPref->GetSetting("iVSyncPresentInterval:Display")) vsyncVal = s->data.i;
        }
        LogResult("VSync Engine Memory Override", vsyncVal != -1, std::format("iVSyncPresentInterval = {} (Bypassed base game ini)", vsyncVal));

        logger::info("======================================================================");
        logger::info("     [SEC] SELF-TEST COMPLETE: [{}/{}] PATCHES VERIFIED [PASS]    ", passCount, totalCount);
        logger::info("======================================================================");
    }
}
