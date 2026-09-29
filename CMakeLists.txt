cmake_minimum_required(VERSION 3.20)

project(
    SkyCore
    VERSION 0.3.6
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SKYCORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# 3rd-party submodule configuration
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
set(RAPIDCSV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd-party/spdlog)
add_subdirectory(3rd-party/rapidcsv)
add_subdirectory(3rd-party/xbyak)
add_subdirectory(3rd-party/CommonLibSSE-NG)

# SkyCore source files
file(GLOB_RECURSE SKYCORE_SOURCES
    "${SKYCORE_ROOT}/src/*.cpp"
    "${SKYCORE_ROOT}/src/*.h"
    "${SKYCORE_ROOT}/include/SkyCore/*.h"
)
list(FILTER SKYCORE_SOURCES EXCLUDE REGEX ".*/shims/.*")

add_library(${PROJECT_NAME} SHARED ${SKYCORE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${SKYCORE_ROOT}/include"
    "${SKYCORE_ROOT}/src"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SKYCORE_EXPORTS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SKYRIM_AE
    SKSE_SUPPORT_XBYAK=1
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    CommonLibSSE::CommonLibSSE
    xbyak::xbyak
    spdlog::spdlog
    DbgHelp.lib
    dxgi.lib
    d3d11.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "SkyCore"
    SUFFIX ".dll"
)
