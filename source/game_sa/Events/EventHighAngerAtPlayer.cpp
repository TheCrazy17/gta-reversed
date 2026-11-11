#include "StdInc.h"

#include "EventHighAngerAtPlayer.h"

void CEventHighAngerAtPlayer::InjectHooks() {
    RH_ScopedVirtualClass(CEventHighAngerAtPlayer, 0x86CC78, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x5FF680);
    RH_ScopedVMTInstall(GetEventType, 0x5FF6B0);
    RH_ScopedVMTInstall(GetEventPriority, 0x5FF8F0);
    RH_ScopedVMTInstall(AffectsPed, 0x4B0EC0);
    RH_ScopedVMTInstall(CloneEditable, 0x5FF6D0);
    RH_ScopedVMTInstall(GetLifeTime, 0x5FF6C0);
}

