#include "DisplayModule.h"
#include "SkyCore/Config.h"
#include "RE/B/BSInputDeviceManager.h"
#include "RE/I/InputEvent.h"
#include "RE/B/ButtonEvent.h"
#include "RE/U/UI.h"
#include "RE/C/Console.h"
#include <fstream>
#include <sstream>
#include <atomic>

namespace SkyCore::Modules::Display
{
    struct MCMDisplaySettings
    {
        bool showFPS{ true };
        uint32_t toggleKey{ 0xD2 }; // DX scan code 0xD2 = Insert = 210
        bool disableVSync{ false };
        int  targetFPS{ 0 };
    };

    static std::atomic<uint32_t> s_activeToggleKey{ 0xD2 };

    static std::string Trim(const std::string& s)
    {
        const auto f = s.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) return {};
        const auto l = s.find_last_not_of(" \t\r\n");
        return s.substr(f, l - f + 1);
    }

    // Always use absolute path based on Skyrim executable location to avoid CWD issues
    static std::filesystem::path GetSkyrimRoot()
    {
        char buffer[MAX_PATH];
        if (GetModuleFileNameA(NULL, buffer, MAX_PATH) > 0) {
            std::filesystem::path exePath{ buffer };
            return exePath.parent_path();
        }
        return std::filesystem::current_path();
    }

    static std::filesystem::path GetMCMIniPath()
    {
        return GetSkyrimRoot() / "Data" / "MCM" / "Settings" / "SkyCore.ini";
    }

    static std::filesystem::path GetDisplayTweaksIniPath()
    {
        return GetSkyrimRoot() / "Data" / "SKSE" / "Plugins" / "SSEDisplayTweaks.ini";
    }

    static MCMDisplaySettings ReadMCMSettings()
    {
        MCMDisplaySettings out;
        const auto& cfg = Config::Get();
        out.disableVSync = cfg.disableVSync;
        out.targetFPS = cfg.targetFPS;

        const auto mcmIni = GetMCMIniPath();
        std::ifstream file(mcmIni);
        if (!file.is_open())
            return out;

        std::string line;
        std::string currentSection;
        while (std::getline(file, line)) {
            const auto c = line.find('#');
            if (c != std::string::npos) line = line.substr(0, c);
            const auto cs = line.find(';');
            if (cs != std::string::npos) line = line.substr(0, cs);
            line = Trim(line);
            if (line.empty()) continue;

            if (line.starts_with('[') && line.ends_with(']')) {
                currentSection = line;
                continue;
            }

            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto key = Trim(line.substr(0, eq));
            auto val = Trim(line.substr(eq + 1));

            if (currentSection == "[Display]") {
                if (key == "bShowFPS") {
                    out.showFPS = (val == "1" || val == "true");
                } else if (key == "iToggleKey") {
                    try {
                        int parsed = std::stoi(val);
                        if (parsed >= 0) {
                            out.toggleKey = static_cast<uint32_t>(parsed);
                        }
                    } catch (...) {}
                } else if (key == "bDisableVSync") {
                    out.disableVSync = (val == "1" || val == "true");
                } else if (key == "iTargetFPS") {
                    try { out.targetFPS = std::stoi(val); } catch (...) {}
                }
            } else if (currentSection == "[Engine]") {
                if (key == "bAltF4QuitFix" || key == "altf4quitfix" || key == "df4quitfix" || key == "bdf4quitfix") {
                    Config::Get().altF4QuitFix = (val == "1" || val == "true");
                }
            }
        }
        return out;
    }

    static uint32_t ReadActiveKeyFromIni()
    {
        const auto mcmIni = GetMCMIniPath();
        std::ifstream file(mcmIni);
        if (!file.is_open())
            return s_activeToggleKey.load();

        std::string line;
        bool inDisplay = false;
        while (std::getline(file, line)) {
            const auto c = line.find('#');
            if (c != std::string::npos) line = line.substr(0, c);
            const auto cs = line.find(';');
            if (cs != std::string::npos) line = line.substr(0, cs);
            line = Trim(line);
            if (line.empty()) continue;

            if (line.starts_with('[')) {
                inDisplay = (line == "[Display]");
                continue;
            }
            if (!inDisplay) continue;

            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto key = Trim(line.substr(0, eq));
            auto val = Trim(line.substr(eq + 1));

            if (key == "iToggleKey") {
                try {
                    int parsed = std::stoi(val);
                    if (parsed >= 0) {
                        return static_cast<uint32_t>(parsed);
                    }
                } catch (...) {}
            }
        }
        return s_activeToggleKey.load();
    }

    // -----------------------------------------------------------------------
    // Direct In-Memory OSD Controller (SSEDisplayTweaks runtime state)
    // RVA 0x5AA48 = volatile bool m_stats.draw
    // RVA 0x5AA4C = volatile uint32_t m_stats.warmup
    // -----------------------------------------------------------------------
    static void ToggleFPS()
    {
        const uintptr_t sdtBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("SSEDisplayTweaks.dll"));
        if (!sdtBase) {
            logger::warn("DisplayModule: SSEDisplayTweaks.dll not loaded in process, cannot toggle OSD.");
            return;
        }

        auto pDraw = reinterpret_cast<volatile bool*>(sdtBase + 0x5AA48);
        auto pWarmup = reinterpret_cast<volatile uint32_t*>(sdtBase + 0x5AA4C);

        const bool newState = !(*pDraw);
        if (newState) {
            *pWarmup = 2;
        }
        *pDraw = newState;

        logger::info("DisplayModule: FPS Counter toggled => {} (Active Hotkey Code={})",
            newState ? "VISIBLE" : "HIDDEN", s_activeToggleKey.load());
    }

    static void SetOSDVisible(bool a_visible)
    {
        const uintptr_t sdtBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("SSEDisplayTweaks.dll"));
        if (!sdtBase) return;

        auto pDraw = reinterpret_cast<volatile bool*>(sdtBase + 0x5AA48);
        auto pWarmup = reinterpret_cast<volatile uint32_t*>(sdtBase + 0x5AA4C);

        if (a_visible) {
            *pWarmup = 2;
        }
        *pDraw = a_visible;
        logger::info("DisplayModule: OSD visibility set to => {}", a_visible ? "VISIBLE" : "HIDDEN");
    }

    static void RefreshMCMSettings()
    {
        const auto mcm = ReadMCMSettings();
        const auto oldKey = s_activeToggleKey.exchange(mcm.toggleKey);

        if (oldKey != mcm.toggleKey) {
            logger::info("DisplayModule: Active Hotkey updated from MCM => {} (was {})", mcm.toggleKey, oldKey);
        }

        const uintptr_t sdtBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("SSEDisplayTweaks.dll"));
        if (sdtBase) {
            auto pDraw = reinterpret_cast<volatile bool*>(sdtBase + 0x5AA48);
            auto pWarmup = reinterpret_cast<volatile uint32_t*>(sdtBase + 0x5AA4C);
            if (*pDraw != mcm.showFPS) {
                if (mcm.showFPS) {
                    *pWarmup = 2;
                }
                *pDraw = mcm.showFPS;
                logger::info("DisplayModule: Synchronized OSD state from MCM => {}", mcm.showFPS ? "VISIBLE" : "HIDDEN");
            }
        }
    }

    // -----------------------------------------------------------------------
    // Patch SSEDisplayTweaks.ini
    // -----------------------------------------------------------------------
    static void PatchDisplayTweaks(const MCMDisplaySettings& mcm)
    {
        const auto iniPath = GetDisplayTweaksIniPath();
        if (!std::filesystem::exists(iniPath)) {
            logger::warn("DisplayModule: SSEDisplayTweaks.ini not found, skipping patch.");
            return;
        }

        const auto& cfg = Config::Get();

        // Read all lines
        std::vector<std::string> lines;
        {
            std::ifstream in(iniPath);
            std::string l;
            while (std::getline(in, l)) lines.push_back(l);
        }

        // Build hex string for ToggleKey (e.g. 0xD2)
        char hexBuf[16];
        std::snprintf(hexBuf, sizeof(hexBuf), "0x%02X", static_cast<unsigned>(mcm.toggleKey));

        std::string currentSection;
        for (auto& l : lines) {
            const std::string trimmed = Trim(l);
            if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
                currentSection = trimmed;
                continue;
            }

            if (currentSection == "[Render]") {
                if (trimmed.starts_with("EnableVSync")) {
                    l = std::string("EnableVSync=") + (mcm.disableVSync ? "false" : "true");
                } else if (trimmed.starts_with("EnableTearing")) {
                    l = "EnableTearing=true";
                } else if (trimmed.starts_with("FramerateLimit=")) {
                    l = std::string("FramerateLimit=") + (mcm.targetFPS > 0 ? std::to_string(mcm.targetFPS) : "0");
                } else if (trimmed.starts_with("Fullscreen=")) {
                    l = std::string("Fullscreen=") + (cfg.borderlessFullscreen ? "false" : "true");
                } else if (trimmed.starts_with("Borderless=")) {
                    l = std::string("Borderless=") + (cfg.borderlessFullscreen ? "true" : "false");
                }
            } else if (currentSection == "[HAVOK]") {
                if (trimmed.starts_with("DynamicMaxTimeScaling")) {
                    l = std::string("DynamicMaxTimeScaling=") + (cfg.dynamicHavok ? "true" : "false");
                }
            } else if (currentSection == "[OSD]") {
                if (trimmed.starts_with("InitiallyOn")) {
                    l = std::string("InitiallyOn=") + (mcm.showFPS ? "true" : "false");
                } else if (trimmed.starts_with("ComboKey")) {
                    l = "ComboKey=0";
                } else if (trimmed.starts_with("ToggleKey")) {
                    l = std::string("ToggleKey=") + hexBuf;
                }
            }
        }

        // Write back
        {
            std::ofstream out(iniPath, std::ios::trunc);
            for (const auto& l : lines) {
                out << l << '\n';
            }
        }

        logger::info("DisplayModule: Display config synchronized => VSync={}, TargetFPS={}, Borderless={}, InitiallyOn={}, ToggleKey={}",
            !mcm.disableVSync, mcm.targetFPS, cfg.borderlessFullscreen, mcm.showFPS, hexBuf);
    }

    // -----------------------------------------------------------------------
    // Windows Message Subclass for Alt+F4 & WM_CLOSE Instant Shutdown
    // -----------------------------------------------------------------------
    static WNDPROC s_originalWndProc = nullptr;

    static LRESULT CALLBACK SkyCoreWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (Config::Get().altF4QuitFix) {
            if (uMsg == WM_CLOSE) {
                logger::info("AltF4QuitFix: Intercepted WM_CLOSE. Cleanly terminating process immediately.");
                spdlog::default_logger()->flush();
                TerminateProcess(GetCurrentProcess(), 0);
                return 0;
            }
            if (uMsg == WM_SYSKEYDOWN && wParam == VK_F4) {
                logger::info("AltF4QuitFix: Intercepted WM_SYSKEYDOWN (VK_F4). Cleanly terminating process immediately.");
                spdlog::default_logger()->flush();
                TerminateProcess(GetCurrentProcess(), 0);
                return 0;
            }
            if (uMsg == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE) {
                logger::info("AltF4QuitFix: Intercepted SC_CLOSE. Cleanly terminating process immediately.");
                spdlog::default_logger()->flush();
                TerminateProcess(GetCurrentProcess(), 0);
                return 0;
            }
        }
        return CallWindowProcA(s_originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    static void InstallWndProcHook()
    {
        if (!Config::Get().altF4QuitFix) {
            return;
        }

        HWND targetHwnd = nullptr;

        struct EnumData {
            DWORD pid;
            HWND hwnd;
        } data = { GetCurrentProcessId(), nullptr };

        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
            auto* d = reinterpret_cast<EnumData*>(lParam);
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);
            if (pid == d->pid) {
                char title[256];
                GetWindowTextA(hWnd, title, sizeof(title));
                char className[256];
                GetClassNameA(hWnd, className, sizeof(className));
                if (strstr(title, "Skyrim") != nullptr || strstr(className, "Skyrim") != nullptr || strstr(title, "Special Edition") != nullptr) {
                    d->hwnd = hWnd;
                    return FALSE;
                }
                if (!d->hwnd && IsWindowVisible(hWnd)) {
                    d->hwnd = hWnd;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&data));

        targetHwnd = data.hwnd;

        if (targetHwnd && IsWindow(targetHwnd)) {
            s_originalWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrA(targetHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SkyCoreWndProc))
            );
            if (s_originalWndProc) {
                logger::info("DisplayModule: Successfully subclassed Skyrim window HWND {:p} for Alt+F4 quit fix", (void*)targetHwnd);
            } else {
                logger::warn("DisplayModule: SetWindowLongPtrA failed on HWND {:p} (error={})", (void*)targetHwnd, GetLastError());
            }
        } else {
            logger::warn("DisplayModule: Could not locate Skyrim main window HWND for Alt+F4 quit fix");
        }
    }

    // -----------------------------------------------------------------------
    // Native Input Event Sink for Instant Single-Key FPS Toggle & Alt+F4 Quick Quit
    // -----------------------------------------------------------------------
    class HotkeyHandler : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        static HotkeyHandler* GetSingleton()
        {
            static HotkeyHandler instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_event,
            [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override
        {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            for (auto it = *a_event; it; it = it->next) {
                auto button = it->AsButtonEvent();
                if (!button || !button->IsDown()) {
                    continue;
                }

                const auto device = button->GetDevice();

                // -----------------------------------------------------------
                // Alt + F4 Quick Quit Fix (DirectInput / RawInput event)
                // -----------------------------------------------------------
                if (Config::Get().altF4QuitFix && device == RE::INPUT_DEVICE::kKeyboard) {
                    // 0x3E is DIK_F4
                    if (button->GetIDCode() == 0x3E) {
                        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) {
                            logger::info("DisplayModule: Alt+F4 hotkey pressed! Cleanly terminating process immediately.");
                            spdlog::default_logger()->flush();
                            TerminateProcess(GetCurrentProcess(), 0);
                            return RE::BSEventNotifyControl::kContinue;
                        }
                    }
                }

                // If console is open, do not trigger OSD hotkey
                if (const auto ui = RE::UI::GetSingleton()) {
                    if (ui->IsMenuOpen(RE::Console::MENU_NAME)) {
                        continue;
                    }
                }

                uint32_t keyCode = 0;
                if (device == RE::INPUT_DEVICE::kKeyboard) {
                    keyCode = button->GetIDCode();
                } else if (device == RE::INPUT_DEVICE::kMouse) {
                    keyCode = button->GetIDCode() + 256;
                } else {
                    continue;
                }

                uint32_t active = s_activeToggleKey.load();
                if (keyCode == active) {
                    ToggleFPS();
                    break;
                }

                // If keyCode did not match the cached active key, check if user remapped the key in MCM!
                uint32_t iniKey = ReadActiveKeyFromIni();
                if (iniKey != active) {
                    s_activeToggleKey.store(iniKey);
                    logger::info("DisplayModule: Active Hotkey updated from INI => {} (was {})", iniKey, active);
                    if (keyCode == iniKey) {
                        ToggleFPS();
                        break;
                    }
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // -----------------------------------------------------------------------
    // Module Lifecycle
    // -----------------------------------------------------------------------
    void Install()
    {
        auto& cfg = Config::Get();

        // Read MCM-persisted display settings
        const auto mcm = ReadMCMSettings();
        cfg.disableVSync = mcm.disableVSync;
        cfg.targetFPS = mcm.targetFPS;
        s_activeToggleKey.store(mcm.toggleKey);

        logger::info("DisplayModule: Active Settings => bDisableVSync={}, iTargetFPS={}, bDynamicHavok={}, bBorderlessFullscreen={}, iToggleKey={}",
            cfg.disableVSync, cfg.targetFPS, cfg.dynamicHavok, cfg.borderlessFullscreen, mcm.toggleKey);

        // Propagate settings to DisplayTweaks INI
        PatchDisplayTweaks(mcm);

        // Disable SSEDisplayTweaks internal KeyPressHandler to prevent double-toggle fighting
        const uintptr_t sdtBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("SSEDisplayTweaks.dll"));
        if (sdtBase) {
            // RVA 0x1E690: DOSD::KeyPressHandler::OnKeyPressed => patch with 0xC3 (ret)
            auto patchAddr = reinterpret_cast<uint8_t*>(sdtBase + 0x1E690);
            DWORD oldProtect;
            if (VirtualProtect(patchAddr, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                *patchAddr = 0xC3; // ret
                VirtualProtect(patchAddr, 1, oldProtect, &oldProtect);
                logger::info("DisplayModule: Disabled SSEDisplayTweaks internal OnKeyPressed at RVA 0x1E690 (Double-toggle prevented)");
            }
        }
    }

    void OnDataLoaded()
    {
        const auto& cfg = Config::Get();

        // 1. Force bLockFramerate = false (disables Bethesda's hardcoded 60 FPS limiter in engine)
        if (const auto ini = RE::INISettingCollection::GetSingleton()) {
            if (const auto setting = ini->GetSetting("bLockFramerate:Display")) {
                setting->data.b = false;
                logger::info("DisplayModule: bLockFramerate forced to false (base game 60 FPS limit bypassed)");
            }
            if (const auto setting = ini->GetSetting("iFPSClamp:HAVOK")) {
                setting->data.i = 0;
                logger::info("DisplayModule: iFPSClamp forced to 0 (physics delta unconstrained)");
            }
        }

        // 2. Force iVSyncPresentInterval based on SkyCore configuration (overrides SkyrimPrefs.ini in engine memory)
        if (const auto iniPref = RE::INIPrefSettingCollection::GetSingleton()) {
            if (const auto setting = iniPref->GetSetting("iVSyncPresentInterval:Display")) {
                setting->data.i = cfg.disableVSync ? 0 : 1;
                logger::info("DisplayModule: iVSyncPresentInterval in engine memory set to {} (base game VSync overridden)", setting->data.i);
            }
        }
    }

    void OnInputLoaded()
    {
        // Register native input hook for hotkey handling
        if (const auto inputManager = RE::BSInputDeviceManager::GetSingleton()) {
            inputManager->AddEventSink(HotkeyHandler::GetSingleton());
            logger::info("DisplayModule: HotkeyHandler registered with BSInputDeviceManager (Active Target Key={})", s_activeToggleKey.load());
        } else {
            logger::error("DisplayModule: BSInputDeviceManager is null! Cannot register HotkeyHandler.");
        }

        // Install window message subclass hook for Alt+F4 / WM_CLOSE
        InstallWndProcHook();

        // Ensure initial OSD state matches MCM setting
        const auto mcm = ReadMCMSettings();
        SetOSDVisible(mcm.showFPS);
    }
}
