#include "StdInc.h"

#include "AttractorScanner.h"

void CAttractorScanner::InjectHooks() {
    RH_ScopedClass(CAttractorScanner);
    RH_ScopedCategory("Core");

    RH_ScopedInstall(Clear, 0x5FFF90);
}

// 0x5FFF90
void CAttractorScanner::Clear() {
    for (auto i = 0u; i < std::size(field_18); i++) {
        field_18[i] = 0;
        field_40[i] = 0;

        // The exe first stores 15 * 15 and then replaces it with 25 for every slot but 4 and 7 (0x5FFFBE)
        field_68[i] = (i == 4 || i == 7) ? 15.0f * 15.0f : 25.0f;
    }
}
