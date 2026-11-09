#include "StdInc.h"

#include "EventAcquaintancePedRespect.h"

void CEventAcquaintancePedRespect::InjectHooks() {
    RH_ScopedVirtualClass(CEventAcquaintancePedRespect, 0x86CA38, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x5FF010);
    RH_ScopedInstall(Destructor, 0x5FF030);
    RH_ScopedVMTInstall(GetEventType, 0x5FF040);
    RH_ScopedVMTInstall(GetEventPriority, 0x5FF8B0);
    RH_ScopedVMTInstall(CloneEditable, 0x5FF050);
}

