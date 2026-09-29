#include "SKSE/SKSE.h"
#include "RE/N/NativeFunction.h"
using namespace std::literals;

namespace
{
    using VM = RE::BSScript::Internal::VirtualMachine;
    using StackID = RE::VMStackID;

    bool IsTweakInstalled(VM*, StackID, RE::StaticFunctionTag*, RE::BSFixedString)
    {
        // SkyCore handles engine fixes natively; report all requested tweaks as active
        return true;
    }

    bool BindPapyrus(VM* a_vm)
    {
        if (a_vm) {
            a_vm->RegisterFunction("IsTweakInstalled", "po3_Tweaks", IsTweakInstalled, true);
        }
        return true;
    }
}

SKSEPluginInfo(
    .Version = REL::Version{ 1, 8, 1, 0 },
    .Name = "powerofthree's Tweaks"sv,
    .Author = "powerofthree (SkyCore Compatibility Shim)"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    auto papyrus = SKSE::GetPapyrusInterface();
    if (papyrus) {
        papyrus->Register(BindPapyrus);
    }
    return true;
}
