#include "SKSE/SKSE.h"
using namespace std::literals;

SKSEPluginInfo(
    .Version = REL::Version{ 6, 2, 0, 0 },
    .Name = "EngineFixes"sv,
    .Author = "aers (SkyCore Compatibility Shim)"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    return true;
}
