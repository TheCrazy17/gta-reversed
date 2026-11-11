#include "StdInc.h"

#include "EventSpecial.h"

void CEventSpecial::InjectHooks() {
    RH_ScopedVirtualClass(CEventSpecial, 0x85B5D0, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4B1AE0);
    RH_ScopedVMTInstall(AffectsPed, 0x4B1B30);
    RH_ScopedVMTInstall(CloneEditable, 0x4B7920);
    RH_ScopedVMTInstall(GetEventType, 0x4B1B10);
    RH_ScopedVMTInstall(GetEventPriority, 0x4B1B40);
    RH_ScopedVMTInstall(GetLifeTime, 0x4B1B20);
}

