#include "StdInc.h"
#include "EventLeaderEntryExit.h"


void CEventLeaderEntryExit::InjectHooks()
{
    RH_ScopedVirtualClass(CEventLeaderEntryExit, 0x859540, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x43E1C0);
    RH_ScopedVMTInstall(GetEventType, 0x43E200);
    RH_ScopedVMTInstall(GetLifeTime, 0x43E210);
}

// 0x43E1C0
CEventLeaderEntryExit* CEventLeaderEntryExit::Constructor(CPed* ped)
{
    this->CEventLeaderEntryExit::CEventLeaderEntryExit(ped);
    return this;
}

