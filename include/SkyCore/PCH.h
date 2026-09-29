#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>
#include <span>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace logger = spdlog;
using namespace std::literals;

#ifdef SKYRIM_AE
#    define VAR_NUM(se, ae) ae
#else
#    define VAR_NUM(se, ae) se
#endif

