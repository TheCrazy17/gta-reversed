#include "StdInc.h"

#include "EventHealthLow.h"

void CEventHealthLow::InjectHooks() {
    RH_ScopedVirtualClass(CEventHealthLow, 0x86CBA0, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x5FF440);
    RH_ScopedVMTInstall(GetEventType, 0x5FF470);
    RH_ScopedVMTInstall(GetEventPriority, 0x5FF900);
    RH_ScopedVMTInstall(AffectsPed, 0x4B0E60);
    RH_ScopedVMTInstall(CloneEditable, 0x5FF490);
}

