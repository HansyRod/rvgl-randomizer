#include "RVGLFunctions.h"
#include "Addresses.h"

namespace Randomizer {

FnCreateCarEntity           RVGL_CreateCarEntity            = reinterpret_cast<FnCreateCarEntity>(AbsFromRva(RVA_CREATE_CAR_ENTITY));
FnComputeSpawnOrientation   RVGL_ComputeSpawnOrientation    = reinterpret_cast<FnComputeSpawnOrientation>(AbsFromRva(RVA_COMPUTE_SPAWN_ORIENT));
FnSetCarTransform           RVGL_SetCarTransform            = reinterpret_cast<FnSetCarTransform>(AbsFromRva(RVA_SET_CAR_TRANSFORM));
FnDrawNumericMenuValue      RVGL_DrawNumericMenuValue       = reinterpret_cast<FnDrawNumericMenuValue>(AbsFromRva(RVA_DRAW_NUMERIC_MENU_VALUE));
FnDecrementNumericMenuValue RVGL_DecrementNumericMenuValue  = reinterpret_cast<FnDecrementNumericMenuValue>(AbsFromRva(RVA_DECREMENT_NUMERIC_MENU_VALUE));
FnIncrementNumericMenuValue RVGL_IncrementNumericMenuValue  = reinterpret_cast<FnIncrementNumericMenuValue>(AbsFromRva(RVA_INCREMENT_NUMERIC_MENU_VALUE));
FnCreateObjectFromFob       RVGL_CreateObjectFromFob        = reinterpret_cast<FnCreateObjectFromFob>(AbsFromRva(RVA_CREATE_OBJECT_FROM_FOB));

} // namespace Randomizer
