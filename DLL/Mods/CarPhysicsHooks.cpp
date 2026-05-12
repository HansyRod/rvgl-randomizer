#include "CarPhysicsHooks.h"
#include "RandomizerState.h"
#include <string>

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnInitCarPhysicsBlock       Orig_InitCarPhysicsBlock       = nullptr;
FnToken_Matches             Orig_Token_Matches             = nullptr;
FnReadTokenFloat            Orig_ReadTokenFloat            = nullptr;
FnReadTokenInt              Orig_ReadTokenInt              = nullptr;
FnReadTokenBool             Orig_ReadTokenBool             = nullptr;

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
