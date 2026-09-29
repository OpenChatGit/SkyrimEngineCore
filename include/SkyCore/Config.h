#pragma once

#include "PCH.h"

namespace SkyCore
{
    struct Config
    {
        // [Display]
        bool borderlessFullscreen{ true };
        bool disableVSync{ false };
        int  targetFPS{ 0 }; // 0 = unlimited / native display rate
        bool dynamicHavok{ true };

        // [Engine]
        bool safeExit{ true };
        bool maxStdIO{ true };
        bool cleanSKSECoSaves{ true };
        bool fixCellInit{ true };
        bool fixDoublePerkApply{ true };
        bool fixEffectShaderZBuffer{ true };
        bool fixBSTempEffectNiRTTI{ true };
        bool fixMusicOverlap{ true };
        bool fixStuckMouseButtons{ true };
        bool fixPerkFragmentIsRunning{ true };
        bool loadEditorIDs{ true };
        bool fixCalendarSkipping{ false };
        bool fixMemoryAccess{ false };
        bool fixBSLightingAmbientSpecular{ false };
        bool fixDistantRefLoadCrash{ false };
        bool fixGlobalTime{ true };
        bool altF4QuitFix{ true };
        bool fixLipSyncLimit{ true };
        uint32_t faceGenMorphLimit{ 64 };

        // [Diagnostics]
        bool enableCrashLogging{ true };
        bool verboseLogging{ false };

        static Config& Get()
        {
            static Config instance;
            return instance;
        }

        void Load(const std::filesystem::path& a_path);
    };
}
