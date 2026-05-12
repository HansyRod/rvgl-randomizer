#include "Randomizer.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "Carbox.h"
#include "GameUtils.h"
#include "ConfigManager.h"
#include "Image.h"
#include "Logger.h"
#include <windows.h>
#include <string>
#include <cstdint>
#include <vector>

// ============================================================================
// Randomizer.cpp
//
// Mod logic for randomizing the car grid.
// This file is responsible only for what the hooks do — not how they are
// installed. See HookManager.cpp for installation.
// ============================================================================

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;
FnInitCarPhysicsBlock       Orig_InitCarPhysicsBlock       = nullptr;
FnToken_Matches             Orig_Token_Matches             = nullptr;
FnReadTokenFloat            Orig_ReadTokenFloat            = nullptr;
FnReadTokenInt              Orig_ReadTokenInt              = nullptr;
FnReadTokenBool             Orig_ReadTokenBool             = nullptr;

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
    Orig_Profile_LoadAndReset(profileName);
}

void Hook_LoadSettingsFromIni(char* profileName) {

    RandomizerContext& ctx = GetRandomizerContext();
    bool useCupDC = ctx.config.useCupDC;

    Orig_LoadSettingsFromIni(profileName);

    // Set CupDC flag according to the config info.
    bool* cupDCFlag = reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
    *cupDCFlag = useCupDC;
}

int spinnerType = 0;
float spinnerAngVel = 0.0f;
bool flippable = false;

bool checkSpinner = false;
bool checkFlippable = false;

void Hook_InitCarPhysicsBlock(CarPhysicsData *physData, int carIndex) {
    
    RandomizerContext& ctx = GetRandomizerContext();

    // Reset temp values to their defaults;
    spinnerType = 0;
    spinnerAngVel = 0.0f;
    flippable = false;

    // Get current car folder
    CarInfo* carPool = ctx.carState.carPool.data();
    CarInfo* car = &carPool[carIndex];
    char* carName = car->internalName;

    // Store the original values before the game overwrites them
    if (strcmp(carName, "panga") != 0 && carIndex == 27) {
        checkSpinner = true;
    }
    else {
        checkSpinner = false;
    }

    if (strcmp(carName, "rotor") != 0 && carIndex == 22) {
        checkFlippable = true;
    }
    else {
        checkFlippable = false;
    }
    
    Orig_InitCarPhysicsBlock(physData, carIndex);

    // Set spinner props for panga, or for the car in panga's slot
    if (strcmp(carName, "panga") == 0 && carIndex != 27) {
        physData->spinnerType = 6;
        physData->spinnerAngVel = 1.5;
    }
    else if (checkSpinner) {
        physData->spinnerType = spinnerType;
        physData->spinnerAngVel = spinnerAngVel;
    }

    // Set flippable prop for rotor, or for the car in rotor's slot
    if (strcmp(carName, "rotor") == 0 && carIndex != 22) {
        physData->flippable = true;
    }
    else if (checkFlippable) {
        physData->flippable = flippable;
    }
}

bool storeNextFloat = false;
bool storeNextInt = false;
bool storeNextBool = false;

bool Hook_Token_Matches(char* token, char* str) {

    bool result = Orig_Token_Matches(token, str);

    if (result) {
        if (checkSpinner) {
            if (strcmp(str, "TYPE") == 0) {
                storeNextInt = true;
            }
            else if (strcmp(str, "ANGVEL") == 0) {
                storeNextFloat = true;
            }
        }
        if (checkFlippable) {
            if (strcmp(str, "FLIPPABLE") == 0) {
                storeNextBool = true;
            }
        }
    }

    return result;
}

bool Hook_ReadTokenFloat(float* outValue, FILE* file) {
    bool result = Orig_ReadTokenFloat(outValue, file);
    if (result && storeNextFloat) {
        spinnerAngVel = *outValue;
        storeNextFloat = false;
    }
    return result;
}

bool Hook_ReadTokenInt(int* outValue, FILE* file) {
    bool result = Orig_ReadTokenInt(outValue, file);
    if (result && storeNextInt) {
        spinnerType = *outValue;
        storeNextInt = false;
    }
    return result;
}

bool Hook_ReadTokenBool(bool* outValue, FILE* file) {
    bool result = Orig_ReadTokenBool(outValue, file);
    if (result && storeNextBool) {
        flippable = *outValue;
        storeNextBool = false;
    }
    return result;
}

} // namespace Randomizer
