#include "CrashModule.h"
#include "SkyCore/Config.h"
#include <DbgHelp.h>
#include <fstream>
#include <iomanip>
#include <shlobj.h>
#include <atomic>

#pragma comment(lib, "DbgHelp.lib")

namespace SkyCore::Modules::CrashHandler
{
    static LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = nullptr;
    static std::atomic<bool> g_handlingCrash{ false };

    static void WriteCrashReport(EXCEPTION_POINTERS* pExceptionInfo)
    {
        bool expected = false;
        if (!g_handlingCrash.compare_exchange_strong(expected, true)) {
            return;
        }

        wchar_t myDocs[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, myDocs))) {
            std::filesystem::path logPath = myDocs;
            logPath /= L"My Games\\Skyrim Special Edition\\SKSE\\SkyCore_Crash.log";

            std::ofstream out(logPath, std::ios::trunc);
            if (out.is_open()) {
                const auto ctx = pExceptionInfo->ContextRecord;
                const auto rec = pExceptionInfo->ExceptionRecord;

                out << "========================================================\n"
                    << " Skyrim Engine Core (SEC) Crash Diagnostics Report\n"
                    << " Game: The Elder Scrolls V: Skyrim Special Edition (1.7.104.0)\n"
                    << "========================================================\n\n";

                out << "Exception Code: 0x" << std::hex << std::uppercase << rec->ExceptionCode;
                switch (rec->ExceptionCode) {
                case EXCEPTION_ACCESS_VIOLATION: out << " (EXCEPTION_ACCESS_VIOLATION)"; break;
                case EXCEPTION_ILLEGAL_INSTRUCTION: out << " (EXCEPTION_ILLEGAL_INSTRUCTION)"; break;
                case EXCEPTION_INT_DIVIDE_BY_ZERO: out << " (EXCEPTION_INT_DIVIDE_BY_ZERO)"; break;
                case EXCEPTION_STACK_OVERFLOW: out << " (EXCEPTION_STACK_OVERFLOW)"; break;
                default: break;
                }
                out << "\n";

                out << "Fault Address:  0x" << std::hex << (std::uintptr_t)rec->ExceptionAddress << "\n\n";

                // Faulting Module
                HMODULE hModule = NULL;
                if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    (LPCWSTR)rec->ExceptionAddress, &hModule)) {
                    wchar_t modPath[MAX_PATH];
                    if (GetModuleFileNameW(hModule, modPath, MAX_PATH)) {
                        out << "Faulting Module: " << std::filesystem::path(modPath).filename().string() << "\n"
                            << "Module Path:     " << std::filesystem::path(modPath).string() << "\n"
                            << "Module Base:     0x" << std::hex << (std::uintptr_t)hModule << "\n"
                            << "Relative Offset: 0x" << std::hex << ((std::uintptr_t)rec->ExceptionAddress - (std::uintptr_t)hModule) << "\n\n";
                    }
                } else {
                    out << "Faulting Module: [Unknown / JIT code]\n\n";
                }

                out << "Registers (x64):\n"
                    << "  RAX: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->Rax << "  RBX: 0x" << std::setw(16) << ctx->Rbx << "\n"
                    << "  RCX: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->Rcx << "  RDX: 0x" << std::setw(16) << ctx->Rdx << "\n"
                    << "  RSI: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->Rsi << "  RDI: 0x" << std::setw(16) << ctx->Rdi << "\n"
                    << "  RBP: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->Rbp << "  RSP: 0x" << std::setw(16) << ctx->Rsp << "\n"
                    << "  RIP: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->Rip << "\n"
                    << "  R8:  0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->R8  << "  R9:  0x" << std::setw(16) << ctx->R9  << "\n"
                    << "  R10: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->R10 << "  R11: 0x" << std::setw(16) << ctx->R11 << "\n"
                    << "  R12: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->R12 << "  R13: 0x" << std::setw(16) << ctx->R13 << "\n"
                    << "  R14: 0x" << std::setw(16) << std::setfill('0') << std::hex << ctx->R14 << "  R15: 0x" << std::setw(16) << ctx->R15 << "\n\n";

                // Call Stack Walk
                out << "Call Stack:\n";
                void* stack[32];
                WORD frames = CaptureStackBackTrace(0, 32, stack, NULL);
                for (WORD i = 0; i < frames; ++i) {
                    HMODULE hMod = NULL;
                    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        (LPCWSTR)stack[i], &hMod)) {
                        wchar_t path[MAX_PATH];
                        if (GetModuleFileNameW(hMod, path, MAX_PATH)) {
                            out << "  [" << std::dec << std::setw(2) << i << "] "
                                << std::filesystem::path(path).filename().string()
                                << " + 0x" << std::hex << ((std::uintptr_t)stack[i] - (std::uintptr_t)hMod)
                                << " (0x" << (std::uintptr_t)stack[i] << ")\n";
                            continue;
                        }
                    }
                    out << "  [" << std::dec << std::setw(2) << i << "] 0x" << std::hex << (std::uintptr_t)stack[i] << "\n";
                }

                out << "\n========================================================\n"
                    << " End of SkyCore Crash Diagnostics\n"
                    << "========================================================\n";
                out.flush();
            }
        }

        // Flush logger if available
        try {
            spdlog::default_logger()->flush();
        } catch (...) {}
    }

    static LONG WINAPI SkyCoreVectoredExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
    {
        if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const auto code = pExceptionInfo->ExceptionRecord->ExceptionCode;
        // Only trigger on actual fatal crashes, ignoring C++ exceptions (0xE06D7363) or thread name notifications
        if (code == EXCEPTION_ACCESS_VIOLATION ||
            code == EXCEPTION_ILLEGAL_INSTRUCTION ||
            code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
            code == EXCEPTION_STACK_OVERFLOW ||
            code == EXCEPTION_PRIV_INSTRUCTION ||
            code == EXCEPTION_IN_PAGE_ERROR) {
            WriteCrashReport(pExceptionInfo);
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    static PVOID g_vehHandle = nullptr;

    void Install()
    {
        if (Config::Get().enableCrashLogging) {
            if (!g_vehHandle) {
                g_vehHandle = AddVectoredExceptionHandler(1, SkyCoreVectoredExceptionHandler);
            }
            logger::info("CrashHandler: Active VEH crash diagnostics and register logger armed");
        }
    }
}
