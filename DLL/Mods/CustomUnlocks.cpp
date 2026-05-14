#include "CustomUnlocks.h"

namespace Randomizer {

bool IsDefaultObtain(int32_t obtain) {
    return obtain >= -1 && obtain <= 5;
}

bool IsCustomObtain(int32_t obtain) {
    return obtain > 5;
}

bool EvaluateCustomUnlock(
    UnlockTargetKind targetKind,
    int targetIndex,
    int32_t obtain,
    const CustomUnlockCondition* condition
) {
    (void)targetKind;
    (void)targetIndex;
    (void)obtain;
    (void)condition;

    // Custom unlock methods are intentionally locked until their
    // progress/query implementations are added in later steps.
    return false;
}

} // namespace Randomizer
