#include "ProfileHooks.h"
#include "RandomizerState.h"
#include "RVGLAddresses.h"
#include "RVGLStructs.h"
#include "MenuMod.h"
#include "CarHooks.h"

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;

bool skipNextProfileLoad = false;

int Hook_GetProfileIndex(char* profileName) {
    int index = Orig_GetProfileIndex(profileName);
    if (index == -1) {
        Orig_Profile_CreateOrLoad(profileName);
        index = Orig_GetProfileIndex(profileName);
        
        // Profile_CreateOrLoad already calls Profile_LoadAndReset,
        // so we can skip it the next time it's called
        skipNextProfileLoad = true;
    }
    return index;
}

bool Hook_Profile_CreateOrLoad(char* displayName) {
    return Orig_Profile_CreateOrLoad(displayName);
}

void Hook_Profile_LoadAndReset(char* profileName) {
    // Profile already loaded in GetProfileIndex
    // This happens when we are loading a profile created
    // for the randomizer for the first time
    if (skipNextProfileLoad) {
        skipNextProfileLoad = false;
        return;
    }

    // Reset unlock check when loading a new profile
    // Otherwise we will get the popup message when switching profiles
    int carCount = GetRuntimeCarCount();
    CarRuntimeState& carState = GetRandomizerContext().carState;
    carState.checkCarUnlocksPopup = false;
    carState.carSelectableState.assign(carCount, false);

    Orig_Profile_LoadAndReset(profileName);

}

void Hook_LoadSettingsFromIni(char* profileName) {

    RandomizerContext& ctx = GetRandomizerContext();
    bool useCupDC = ctx.config.useCupDC;

    Orig_LoadSettingsFromIni(profileName);

    // Set CupDC flag according to the config info.
    bool* cupDCFlag = reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
    *cupDCFlag = useCupDC;

    // Sync our cars per race count with the stored cars per race
    int nCars = *reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    ctx.carState.carsPerRace = nCars;

    // Change the NCars settings row so it uses our functions
    PatchCarCountMenuDescriptor();

}

} // namespace Randomizer
