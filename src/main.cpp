#include "SkyCore/PCH.h"
#include "SkyCore/Version.h"
#include "SkyCore/Config.h"
#include "modules/Engine/EngineModule.h"
#include "modules/Display/DisplayModule.h"
#include "modules/CrashHandler/CrashModule.h"
#include <shlobj.h>

// Standard SKSE plugin declaration for AE 1.7.104+
SKSEPluginInfo(
    .Version = REL::Version{ SkyCore::Version::MAJOR, SkyCore::Version::MINOR, SkyCore::Version::PATCH, 0 },
    .Name = "Skyrim Engine Core"sv,
    .Author = "Nicol"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

namespace
{
    void InitializeLogging()
    {
        std::filesystem::path logPath;
        auto skseDir = SKSE::log::log_directory();
        if (skseDir) {
            logPath = *skseDir / "SkyCore.log";
        } else {
            wchar_t myDocs[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, myDocs))) {
                logPath = myDocs;
                logPath /= L"My Games\\Skyrim Special Edition\\SKSE\\SkyCore.log";
            }
        }

        if (!logPath.empty()) {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
            auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

            spdlog::sinks_init_list sink_list = { file_sink, msvc_sink };
            auto log = std::make_shared<spdlog::logger>("global log", sink_list.begin(), sink_list.end());

            // Real-time flush on EVERY message so logs are NEVER lost
            log->set_level(spdlog::level::info);
            log->flush_on(spdlog::level::info);

            spdlog::set_default_logger(std::move(log));
            spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
        }
    }

    // Native Papyrus compatibility: third-party mods inquiring about po3_Tweaks
    bool Papyrus_IsTweakInstalled(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, RE::BSFixedString a_tweak)
    {
        logger::info("Papyrus: po3_Tweaks.IsTweakInstalled('{}') -> Satisfied natively by Skyrim Engine Core", a_tweak.c_str());
        return true;
    }

    bool BindPapyrusCompatibility(RE::BSScript::Internal::VirtualMachine* a_vm)
    {
        if (!a_vm) {
            return false;
        }
        a_vm->RegisterFunction("IsTweakInstalled", "po3_Tweaks", Papyrus_IsTweakInstalled, true);
        logger::info("Papyrus: Registered 'po3_Tweaks.IsTweakInstalled' compatibility stub.");
        return true;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    InitializeLogging();

    try {
        logger::info("======================================================================");
        logger::info("{} v{}.{}.{} by {} initialized", SkyCore::Version::NAME, SkyCore::Version::MAJOR, SkyCore::Version::MINOR, SkyCore::Version::PATCH, SkyCore::Version::AUTHOR);
        logger::info("Target Game: Skyrim Special Edition (1.7.104+)");
        logger::info("SKSE Release Version: {}.{}.{}", a_skse->SKSEVersion() >> 24, (a_skse->SKSEVersion() >> 16) & 0xFF, (a_skse->SKSEVersion() >> 4) & 0xFFF);
        logger::info("======================================================================");

        SKSE::Init(a_skse);
        logger::info("SKSE interface initialized successfully.");

        // Allocate 2 KB code trampoline for native engine hooks (safe size within SKSE branch pool)
        SKSE::AllocTrampoline(2048);
        logger::info("Trampoline allocated successfully (2048 bytes).");

        // Load configuration
        const auto configPath = std::filesystem::path("Data/SKSE/Plugins/SkyCore.toml");
        logger::info("Loading configuration from: {} (exists: {})", configPath.string(), std::filesystem::exists(configPath));
        SkyCore::Config::Get().Load(configPath);
        logger::info("Configuration parsed: MaxStdIO={}, SafeExit={}, DynamicHavok={}, CellInit={}, AltF4QuitFix={}",
            SkyCore::Config::Get().maxStdIO,
            SkyCore::Config::Get().safeExit,
            SkyCore::Config::Get().dynamicHavok,
            SkyCore::Config::Get().fixCellInit,
            SkyCore::Config::Get().altF4QuitFix);

        // Install modules
        logger::info("Installing CrashHandler module...");
        SkyCore::Modules::CrashHandler::Install();

        logger::info("Installing Engine module...");
        SkyCore::Modules::Engine::Install();

        logger::info("Installing Display module...");
        SkyCore::Modules::Display::Install();

        // Register SKSE message listener for engine data and input events
        auto messaging = SKSE::GetMessagingInterface();
        if (messaging) {
            messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
                if (!a_msg) return;
                if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
                    logger::info("SKSE kDataLoaded received: Applying engine memory overrides...");
                    SkyCore::Modules::Display::OnDataLoaded();
                    SkyCore::Modules::Engine::OnDataLoaded();
                    SkyCore::Modules::Engine::RunStartupDiagnostics();
                } else if (a_msg->type == SKSE::MessagingInterface::kInputLoaded) {
                    logger::info("SKSE kInputLoaded received: Registering native hotkey listener...");
                    SkyCore::Modules::Display::OnInputLoaded();
                }
            });
            logger::info("SKSE messaging listener registered for kDataLoaded & kInputLoaded.");
        }

        // Register Papyrus interface for native script compatibility stubs
        auto papyrus = SKSE::GetPapyrusInterface();
        if (papyrus) {
            papyrus->Register(BindPapyrusCompatibility);
            logger::info("SKSE Papyrus compatibility listener registered.");
        }

        logger::info("SkyCore: All unified modules loaded and initialized successfully.");
        return true;
    } catch (const std::exception& e) {
        logger::critical("Fatal exception during SkyCore initialization: {}", e.what());
        return false;
    } catch (...) {
        logger::critical("Unknown fatal exception during SkyCore initialization");
        return false;
    }
}
