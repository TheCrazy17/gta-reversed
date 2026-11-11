#include "StdInc.h"
#include "EventAcquaintancePedHate.h"


void CEventAcquaintancePedHate::InjectHooks()
{
    RH_ScopedVirtualClass(CEventAcquaintancePedHate, 0x858E68, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor2, 0x420E70);
    RH_ScopedVMTInstall(GetEventType, 0x420F40);
    RH_ScopedVMTInstall(GetEventPriority, 0x420FD0);
}

// 0x420E70
CEventAcquaintancePedHate* CEventAcquaintancePedHate::Constructor2(CPed* ped)
{
    this->CEventAcquaintancePedHate::CEventAcquaintancePedHate(ped);
    return this;
}

