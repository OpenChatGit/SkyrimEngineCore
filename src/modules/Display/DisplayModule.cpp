#include "DisplayModule.h"
#include "SkyCore/Config.h"
#include "RE/B/BSInputDeviceManager.h"
#include "RE/I/InputEvent.h"
#include "RE/B/ButtonEvent.h"
#include "RE/U/UI.h"
#include "RE/C/Console.h"
#include "RE/R/Renderer.h"
#include <fstream>
#include <sstream>
#include <atomic>
#include <thread>
#include <chrono>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dxgi.lib")

namespace SkyCore::Modules::Display
{
    struct MCMDisplaySettings
    {
        bool showFPS{ true };
        uint32_t toggleKey{ 0xD2 }; // DX scan code 0xD2 = Insert = 210
        bool disableVSync{ true };
        int  targetFPS{ 0 };
    };

    static std::atomic<uint32_t> s_activeToggleKey{ 0xD2 };
    static std::atomic<bool>     s_showFPS{ true };
    static std::atomic<float>    s_currentFPS{ 0.0f };
    static std::atomic<HWND>     s_skyrimHwnd{ nullptr };
    static std::atomic<HWND>     s_overlayHwnd{ nullptr };
    static std::atomic<bool>     s_overlayRunning{ false };
    static std::atomic<bool>     s_presentHooked{ false };

    static std::string Trim(const std::string& s)
    {
        const auto f = s.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) return {};
        const auto l = s.find_last_not_of(" \t\r\n");
        return s.substr(f, l - f + 1);
    }

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
    // Native DXGI SwapChain Hook for Unlocked Framerate & FPS Measurement
    // -----------------------------------------------------------------------
    using Present_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags);
    static Present_t s_originalPresent = nullptr;

    static HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags)
    {
        // 1. Force hardware VSync off when configured (guarantees teardown of 60Hz clamp)
        if (Config::Get().disableVSync) {
            a_syncInterval = 0;
            a_flags &= ~DXGI_PRESENT_DO_NOT_WAIT;
        }

        // 2. High-precision Frame Timing & Smoothed FPS Calculation
        static auto s_lastTime = std::chrono::steady_clock::now();
        static auto s_lastFPSTime = s_lastTime;
        static uint32_t s_frameCount = 0;

        auto now = std::chrono::steady_clock::now();
        s_frameCount++;

        std::chrono::duration<float> elapsed = now - s_lastFPSTime;
        if (elapsed.count() >= 0.20f) { // Update FPS calculation every 200ms
            float fps = static_cast<float>(s_frameCount) / elapsed.count();
            s_currentFPS.store(fps);
            s_frameCount = 0;
            s_lastFPSTime = now;
        }

        // 3. Optional Framerate Limiter
        const int targetFPS = Config::Get().targetFPS;
        if (targetFPS > 0) {
            const float targetFrameTime = 1.0f / static_cast<float>(targetFPS);
            std::chrono::duration<float> frameElapsed = now - s_lastTime;
            while (frameElapsed.count() < targetFrameTime) {
                float remain = targetFrameTime - frameElapsed.count();
                if (remain > 0.002f) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } else {
                    std::this_thread::yield();
                }
                now = std::chrono::steady_clock::now();
                frameElapsed = now - s_lastTime;
            }
        }
        s_lastTime = std::chrono::steady_clock::now();

        return s_originalPresent(a_this, a_syncInterval, a_flags);
    }

    static void InstallPresentHook(IDXGISwapChain* a_swapChain)
    {
        if (!a_swapChain) return;
        if (s_presentHooked.load()) return;

        void** vtable = *reinterpret_cast<void***>(a_swapChain);
        if (!vtable) return;

        if (vtable[8] == reinterpret_cast<void*>(HookedPresent)) {
            s_presentHooked.store(true);
            return;
        }

        DWORD oldProtect = 0;
        if (VirtualProtect(&vtable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            s_originalPresent = reinterpret_cast<Present_t>(vtable[8]);
            vtable[8] = reinterpret_cast<void*>(HookedPresent);
            VirtualProtect(&vtable[8], sizeof(void*), oldProtect, &oldProtect);
            s_presentHooked.store(true);
            logger::info("DisplayModule: Successfully installed native IDXGISwapChain::Present hook (Original: {:p}, Hook: {:p})",
                (void*)s_originalPresent, (void*)HookedPresent);
        } else {
            logger::error("DisplayModule: Failed to VirtualProtect swapchain vtable index 8 (error={})", GetLastError());
        }
    }

    // -----------------------------------------------------------------------
    // Native Win32 Layered Transparent Click-Through FPS Counter Overlay
    // -----------------------------------------------------------------------
    static LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
        case WM_ERASEBKGND:
            return 1; // Prevent GDI flicker

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            const int width = rcClient.right - rcClient.left;
            const int height = rcClient.bottom - rcClient.top;

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            // 1. Fill entire background with pure Black RGB(0,0,0) -> 100% Transparent ColorKey
            HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(memDC, &rcClient, blackBrush);
            DeleteObject(blackBrush);

            // 2. Draw sleek dark badge background (RGB(18, 22, 28) with subtle border RGB(55, 65, 81))
            HBRUSH badgeBrush = CreateSolidBrush(RGB(18, 22, 28));
            HPEN badgePen = CreatePen(PS_SOLID, 1, RGB(55, 65, 81));
            HGDIOBJ oldBrush = SelectObject(memDC, badgeBrush);
            HGDIOBJ oldPen = SelectObject(memDC, badgePen);
            RoundRect(memDC, 1, 1, width - 1, height - 1, 10, 10);
            SelectObject(memDC, oldBrush);
            SelectObject(memDC, oldPen);
            DeleteObject(badgeBrush);
            DeleteObject(badgePen);

            // 3. Format FPS string
            float fps = s_currentFPS.load();
            char buf[32];
            if (fps <= 0.0f) {
                snprintf(buf, sizeof(buf), "... FPS");
            } else {
                snprintf(buf, sizeof(buf), "%.1f FPS", fps);
            }

            // 4. Setup Font (Segoe UI Bold)
            SetBkMode(memDC, TRANSPARENT);
            HFONT font = CreateFontA(
                21, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            HFONT oldFont = (HFONT)SelectObject(memDC, font);

            // 5. Draw text shadow at (x+1, y+1)
            SetTextColor(memDC, RGB(12, 14, 18));
            RECT shadowRect = rcClient;
            OffsetRect(&shadowRect, 1, 1);
            DrawTextA(memDC, buf, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // 6. Draw dynamic colored text
            COLORREF textColor = RGB(74, 222, 128); // Vibrant gaming green (#4ade80)
            if (fps > 0.0f && fps < 35.0f) {
                textColor = RGB(248, 113, 113); // Coral red (#f87171)
            } else if (fps > 0.0f && fps < 55.0f) {
                textColor = RGB(250, 204, 21); // Amber yellow (#facc15)
            }
            SetTextColor(memDC, textColor);
            DrawTextA(memDC, buf, -1, &rcClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // 7. Blit to screen
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

            // Cleanup GDI objects
            SelectObject(memDC, oldFont);
            DeleteObject(font);
            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            return 0;

        default:
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
    }

    static std::thread s_overlayThread;

    static void OverlayThreadFunc(HWND a_skyrimHwnd)
    {
        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
        wc.lpfnWndProc = OverlayWndProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "SkyCore_FPS_Overlay";
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        RegisterClassExA(&wc);

        const int overlayW = 136;
        const int overlayH = 34;

        HWND hwnd = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            "SkyCore_FPS_Overlay",
            "SkyCore FPS",
            WS_POPUP,
            24, 24, overlayW, overlayH,
            NULL, NULL, GetModuleHandleA(NULL), NULL);

        if (!hwnd) {
            logger::error("DisplayModule: Failed to create FPS overlay window (error={})", GetLastError());
            return;
        }

        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        s_overlayHwnd.store(hwnd);
        s_overlayRunning.store(true);
        logger::info("DisplayModule: Native FPS overlay window initialized (HWND: {:p})", (void*)hwnd);

        if (s_showFPS.load()) {
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        }

        int prevX = -9999;
        int prevY = -9999;

        while (s_overlayRunning.load()) {
            MSG msg;
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    s_overlayRunning.store(false);
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }

            if (!s_overlayRunning.load()) break;

            HWND skyrim = s_skyrimHwnd.load();
            if (skyrim && IsWindow(skyrim)) {
                bool isIconic = IsIconic(skyrim);

                if (s_showFPS.load() && !isIconic) {
                    RECT rc;
                    if (GetWindowRect(skyrim, &rc)) {
                        int targetX = rc.left + 24;
                        int targetY = rc.top + 24;
                        if (targetX != prevX || targetY != prevY) {
                            prevX = targetX;
                            prevY = targetY;
                            SetWindowPos(hwnd, HWND_TOPMOST, targetX, targetY, overlayW, overlayH,
                                SWP_NOACTIVATE | SWP_SHOWWINDOW);
                        }
                    }
                    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
                    InvalidateRect(hwnd, NULL, FALSE);
                } else {
                    ShowWindow(hwnd, SW_HIDE);
                }
            } else {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (hwnd && IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
        UnregisterClassA("SkyCore_FPS_Overlay", GetModuleHandleA(NULL));
        s_overlayHwnd.store(nullptr);
    }

    static void StartOverlayThread(HWND a_skyrimHwnd)
    {
        static std::atomic<bool> s_threadStarted{ false };
        if (s_threadStarted.exchange(true)) return;

        s_overlayThread = std::thread(OverlayThreadFunc, a_skyrimHwnd);
        s_overlayThread.detach();
        logger::info("DisplayModule: Native FPS overlay background thread started");
    }

    // -----------------------------------------------------------------------
    // Native OSD Controller & Hotkey Commands
    // -----------------------------------------------------------------------
    static void ToggleFPS()
    {
        const bool newState = !s_showFPS.load();
        s_showFPS.store(newState);
        HWND ov = s_overlayHwnd.load();
        if (ov && IsWindow(ov)) {
            ShowWindow(ov, newState ? SW_SHOWNOACTIVATE : SW_HIDE);
            if (newState) {
                InvalidateRect(ov, NULL, TRUE);
            }
        }
        logger::info("DisplayModule: Native FPS Counter toggled => {}", newState ? "VISIBLE" : "HIDDEN");
    }

    static void SetOSDVisible(bool a_visible)
    {
        s_showFPS.store(a_visible);
        HWND ov = s_overlayHwnd.load();
        if (ov && IsWindow(ov)) {
            ShowWindow(ov, a_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
            if (a_visible) {
                InvalidateRect(ov, NULL, TRUE);
            }
        }
        logger::info("DisplayModule: Native FPS visibility set to => {}", a_visible ? "VISIBLE" : "HIDDEN");
    }

    static void RefreshMCMSettings()
    {
        const auto mcm = ReadMCMSettings();
        const auto oldKey = s_activeToggleKey.exchange(mcm.toggleKey);

        if (oldKey != mcm.toggleKey) {
            logger::info("DisplayModule: Active Hotkey updated from MCM => {} (was {})", mcm.toggleKey, oldKey);
        }

        SetOSDVisible(mcm.showFPS);
    }

    // -----------------------------------------------------------------------
    // Patch SSEDisplayTweaks.ini (Optional synchronization if present)
    // -----------------------------------------------------------------------
    static void PatchDisplayTweaks(const MCMDisplaySettings& mcm)
    {
        const auto iniPath = GetDisplayTweaksIniPath();
        if (!std::filesystem::exists(iniPath)) {
            return;
        }

        const auto& cfg = Config::Get();
        std::vector<std::string> lines;
        {
            std::ifstream in(iniPath);
            std::string l;
            while (std::getline(in, l)) lines.push_back(l);
        }

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

        {
            std::ofstream out(iniPath, std::ios::trunc);
            for (const auto& l : lines) {
                out << l << '\n';
            }
        }
    }

    // -----------------------------------------------------------------------
    // Windows Message Subclass for Alt+F4, WM_CLOSE & Borderless Maintenance
    // -----------------------------------------------------------------------
    static WNDPROC s_originalWndProc = nullptr;

    static void ApplyNativeBorderlessFullscreen(HWND a_hwnd)
    {
        if (!a_hwnd || !IsWindow(a_hwnd)) {
            return;
        }

        const auto& cfg = Config::Get();
        if (!cfg.borderlessFullscreen) {
            return;
        }

        HMONITOR hMon = MonitorFromWindow(a_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfoA(hMon, &mi)) {
            const int width = mi.rcMonitor.right - mi.rcMonitor.left;
            const int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

            LONG_PTR style = GetWindowLongPtrA(a_hwnd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            style |= WS_POPUP | WS_VISIBLE;
            SetWindowLongPtrA(a_hwnd, GWL_STYLE, style);

            LONG_PTR exStyle = GetWindowLongPtrA(a_hwnd, GWL_EXSTYLE);
            exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
            SetWindowLongPtrA(a_hwnd, GWL_EXSTYLE, exStyle);

            SetWindowPos(a_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, width, height,
                SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_SHOWWINDOW);

            logger::info("DisplayModule: Applied native borderless fullscreen ({}x{} at {},{}) to HWND {:p}",
                width, height, mi.rcMonitor.left, mi.rcMonitor.top, (void*)a_hwnd);
        }
    }

    static void TryHookDisplay();

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

        if (Config::Get().borderlessFullscreen) {
            if (uMsg == WM_STYLECHANGING && wParam == GWL_STYLE) {
                auto* styleStruct = reinterpret_cast<STYLESTRUCT*>(lParam);
                if (styleStruct) {
                    styleStruct->styleNew &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
                    styleStruct->styleNew |= WS_POPUP;
                }
            }
        }

        if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
            TryHookDisplay();
        }

        return CallWindowProcA(s_originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    static void TryHookDisplay()
    {
        auto* rw = RE::BSGraphics::Renderer::GetCurrentRenderWindow();
        if (!rw) return;

        if (rw->swapChain) {
            auto* sc = reinterpret_cast<IDXGISwapChain*>(rw->swapChain);
            InstallPresentHook(sc);

            BOOL isFullscreen = FALSE;
            if (SUCCEEDED(sc->GetFullscreenState(&isFullscreen, nullptr))) {
                if (isFullscreen && Config::Get().borderlessFullscreen) {
                    sc->SetFullscreenState(FALSE, nullptr);
                    logger::info("DisplayModule: Released DXGI swapchain exclusive fullscreen to windowed borderless.");
                }
            }
        }

        HWND targetHwnd = rw->hWnd ? reinterpret_cast<HWND>(rw->hWnd) : nullptr;
        if (!targetHwnd) {
            targetHwnd = s_skyrimHwnd.load();
        }
        if (targetHwnd && IsWindow(targetHwnd)) {
            s_skyrimHwnd.store(targetHwnd);
            ApplyNativeBorderlessFullscreen(targetHwnd);
            StartOverlayThread(targetHwnd);
        }
    }

    static void InstallWndProcHook()
    {
        HWND targetHwnd = nullptr;
        auto* rw = RE::BSGraphics::Renderer::GetCurrentRenderWindow();
        if (rw && rw->hWnd) {
            targetHwnd = reinterpret_cast<HWND>(rw->hWnd);
        }

        if (!targetHwnd) {
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
        }

        if (targetHwnd && IsWindow(targetHwnd)) {
            s_skyrimHwnd.store(targetHwnd);
            ApplyNativeBorderlessFullscreen(targetHwnd);
            StartOverlayThread(targetHwnd);

            if (Config::Get().altF4QuitFix) {
                s_originalWndProc = reinterpret_cast<WNDPROC>(
                    SetWindowLongPtrA(targetHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SkyCoreWndProc))
                );
                if (s_originalWndProc) {
                    logger::info("DisplayModule: Successfully subclassed Skyrim window HWND {:p} for Alt+F4 quit fix", (void*)targetHwnd);
                } else {
                    logger::warn("DisplayModule: SetWindowLongPtrA failed on HWND {:p} (error={})", (void*)targetHwnd, GetLastError());
                }
            }
        } else {
            logger::warn("DisplayModule: Could not locate Skyrim main window HWND");
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

                // Alt + F4 Quick Quit Fix
                if (Config::Get().altF4QuitFix && device == RE::INPUT_DEVICE::kKeyboard) {
                    if (button->GetIDCode() == 0x3E) { // DIK_F4
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
                if (keyCode == active || keyCode == 0xD2 || keyCode == 52) {
                    ToggleFPS();
                    break;
                }

                // Check dynamic keymap from INI
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
        s_showFPS.store(mcm.showFPS);
        s_activeToggleKey.store(mcm.toggleKey);

        logger::info("DisplayModule: Active Settings => bShowFPS={}, bDisableVSync={}, iTargetFPS={}, bDynamicHavok={}, bBorderlessFullscreen={}, iToggleKey={}",
            mcm.showFPS, cfg.disableVSync, cfg.targetFPS, cfg.dynamicHavok, cfg.borderlessFullscreen, mcm.toggleKey);

        // Propagate settings to DisplayTweaks INI if present
        PatchDisplayTweaks(mcm);

        // Disable SSEDisplayTweaks internal KeyPressHandler if loaded to prevent fighting
        const uintptr_t sdtBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("SSEDisplayTweaks.dll"));
        if (sdtBase) {
            auto patchAddr = reinterpret_cast<uint8_t*>(sdtBase + 0x1E690);
            DWORD oldProtect;
            if (VirtualProtect(patchAddr, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                *patchAddr = 0xC3; // ret
                VirtualProtect(patchAddr, 1, oldProtect, &oldProtect);
                logger::info("DisplayModule: Disabled SSEDisplayTweaks internal OnKeyPressed at RVA 0x1E690");
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
            if (const auto setting = ini->GetSetting("fMaxTime:HAVOK")) {
                setting->data.f = 0.00416666f; // Dynamic Havok unlocked up to 240 FPS
                logger::info("DisplayModule: fMaxTime:HAVOK set to 0.00416666 (Dynamic Havok unlocked up to 240 FPS)");
            }
        }

        // 2. Force iVSyncPresentInterval and Borderless in engine memory preferences
        if (const auto iniPref = RE::INIPrefSettingCollection::GetSingleton()) {
            if (const auto setting = iniPref->GetSetting("iVSyncPresentInterval:Display")) {
                setting->data.i = cfg.disableVSync ? 0 : 1;
                logger::info("DisplayModule: iVSyncPresentInterval in engine memory set to {} (VSync {})",
                    setting->data.i, cfg.disableVSync ? "DISABLED" : "ENABLED");
            }
            if (const auto setting = iniPref->GetSetting("bLockFramerate:Display")) {
                setting->data.b = false;
            }
            if (cfg.borderlessFullscreen) {
                if (const auto setting = iniPref->GetSetting("bFull Screen:Display")) {
                    setting->data.b = false;
                    logger::info("DisplayModule: bFull Screen forced to false (Borderless enabled)");
                }
                if (const auto setting = iniPref->GetSetting("bBorderless:Display")) {
                    setting->data.b = true;
                    logger::info("DisplayModule: bBorderless forced to true (Borderless enabled)");
                }
            }
        }

        // 3. Force BSGraphics::RendererData runtime fields
        if (auto* rData = RE::BSGraphics::Renderer::GetRendererData()) {
            if (cfg.disableVSync) {
                rData->presentInterval = 0;
            }
            if (cfg.borderlessFullscreen) {
                rData->fullScreen = false;
                rData->borderlessDisplay = true;
            }
            logger::info("DisplayModule: BSGraphics::RendererData updated (presentInterval={}, fullScreen={}, borderless={})",
                rData->presentInterval, rData->fullScreen, rData->borderlessDisplay);
        }

        // 4. Hook swapchain and initialize overlay
        TryHookDisplay();
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

        // Ensure swapchain hook and overlay thread are ready
        TryHookDisplay();

        // Ensure initial OSD state matches MCM setting
        const auto mcm = ReadMCMSettings();
        SetOSDVisible(mcm.showFPS);
    }
}
