#include "StdInc.h"
#include "EventRevived.h"

void CEventRevived::InjectHooks() {
    RH_ScopedVirtualClass(CEventRevived, 0x85B030, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4AEC50);
    RH_ScopedVMTInstall(Clone, 0x4B71E0);
    RH_ScopedVMTInstall(AffectsPed, 0x4AECB0);
}



bool CEventRevived::AffectsPed(CPed* ped) {
    return !ped->IsCreatedByMission() && !ped->IsAlive();
}

