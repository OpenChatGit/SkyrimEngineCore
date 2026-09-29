#include "SkyCore/Config.h"
#include <fstream>
#include <sstream>

namespace SkyCore
{
    static std::string Trim(const std::string& str)
    {
        const auto first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const auto last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    void Config::Load(const std::filesystem::path& a_path)
    {
        if (!std::filesystem::exists(a_path)) {
            // Write default config
            std::ofstream out(a_path);
            if (out.is_open()) {
                out << "# SkyCore - Unified Engine & Display Suite for Skyrim AE 1.7+\n\n"
                    << "[Display]\n"
                    << "bBorderlessFullscreen = true\n"
                    << "bDynamicHavok = true\n"
                    << "iTargetFPS = 0\n"
                    << "bDisableVSync = false\n\n"
                    << "[Engine]\n"
                    << "bSafeExit = true\n"
                    << "bMaxStdIO = true\n"
                    << "bCleanSKSECoSaves = true\n"
                    << "bFixCellInit = true\n"
                    << "bFixDoublePerkApply = true\n"
                    << "bFixCalendarSkipping = true\n"
                    << "bFixMemoryAccess = true\n"
                    << "bFixBSLightingAmbientSpecular = true\n"
                    << "bFixEffectShaderZBuffer = true\n"
                    << "bFixDistantRefLoadCrash = true\n"
                    << "bFixGlobalTime = true\n"
                    << "bAltF4QuitFix = true\n"
                    << "bFixActorLimit = true\n"
                    << "iActorMoverLimit = 256\n"
                    << "iActorMorphLimit = 64\n\n"
                    << "[Gameplay]\n"
                    << "bDynamicMerchantGold = true\n"
                    << "iMerchantGoldBase = 750\n"
                    << "iMerchantGoldPerLevel = 25\n"
                    << "iMerchantGoldPerSpeech = 10\n"
                    << "iMerchantGoldCap = 25000\n\n"
                    << "[Diagnostics]\n"
                    << "bEnableCrashLogging = true\n"
                    << "bVerboseLogging = false\n";
            }
            return;
        }

        std::ifstream file(a_path);
        std::string line;
        while (std::getline(file, line)) {
            // Strip inline comments (# and ;)
            const auto commentHash = line.find('#');
            if (commentHash != std::string::npos) {
                line = line.substr(0, commentHash);
            }
            const auto commentSemi = line.find(';');
            if (commentSemi != std::string::npos) {
                line = line.substr(0, commentSemi);
            }

            line = Trim(line);
            if (line.empty()) continue;
            if (line.starts_with('[') && line.ends_with(']')) continue;

            const auto eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            auto key = Trim(line.substr(0, eqPos));
            auto val = Trim(line.substr(eqPos + 1));

            auto isTrue = (val == "true" || val == "1");

            if (key == "bBorderlessFullscreen") borderlessFullscreen = isTrue;
            else if (key == "bDynamicHavok") dynamicHavok = isTrue;
            else if (key == "bDisableVSync") disableVSync = isTrue;
            else if (key == "iTargetFPS") targetFPS = std::stoi(val);
            else if (key == "bSafeExit") safeExit = isTrue;
            else if (key == "bMaxStdIO") maxStdIO = isTrue;
            else if (key == "bCleanSKSECoSaves") cleanSKSECoSaves = isTrue;
            else if (key == "bFixCellInit") fixCellInit = isTrue;
            else if (key == "bFixDoublePerkApply") fixDoublePerkApply = isTrue;
            else if (key == "bFixEffectShaderZBuffer") fixEffectShaderZBuffer = isTrue;
            else if (key == "bFixBSTempEffectNiRTTI") fixBSTempEffectNiRTTI = isTrue;
            else if (key == "bFixMusicOverlap") fixMusicOverlap = isTrue;
            else if (key == "bFixStuckMouseButtons") fixStuckMouseButtons = isTrue;
            else if (key == "bFixPerkFragmentIsRunning") fixPerkFragmentIsRunning = isTrue;
            else if (key == "bLoadEditorIDs") loadEditorIDs = isTrue;
            else if (key == "bFixCalendarSkipping") fixCalendarSkipping = isTrue;
            else if (key == "bFixMemoryAccess") fixMemoryAccess = isTrue;
            else if (key == "bFixBSLightingAmbientSpecular") fixBSLightingAmbientSpecular = isTrue;
            else if (key == "bFixDistantRefLoadCrash") fixDistantRefLoadCrash = isTrue;
            else if (key == "bAltF4QuitFix" || key == "altf4quitfix" || key == "df4quitfix" || key == "bdf4quitfix") altF4QuitFix = isTrue;
            else if (key == "bFixActorLimit") fixActorLimit = isTrue;
            else if (key == "iActorMoverLimit") { try { actorMoverLimit = static_cast<uint32_t>(std::stoul(val)); } catch (...) {} }
            else if (key == "iActorMorphLimit") { try { actorMorphLimit = static_cast<uint32_t>(std::stoul(val)); } catch (...) {} }
            else if (key == "bEnableCrashLogging") enableCrashLogging = isTrue;
            else if (key == "bVerboseLogging") verboseLogging = isTrue;
        }
    }
}
