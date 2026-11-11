#include "StdInc.h"

#include "EventHealthReallyLow.h"

void CEventHealthReallyLow::InjectHooks() {
    RH_ScopedVirtualClass(CEventHealthReallyLow, 0x86CBE8, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x5FF500);
    RH_ScopedVMTInstall(GetEventType, 0x5FF530);
    RH_ScopedVMTInstall(GetEventPriority, 0x5FF8E0);
    RH_ScopedVMTInstall(AffectsPed, 0x4B0E80);
    RH_ScopedVMTInstall(CloneEditable, 0x5FF550);
    RH_ScopedVMTInstall(GetLifeTime, 0x5FF540);
}

