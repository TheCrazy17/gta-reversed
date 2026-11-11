#include "StdInc.h"
#include "EventLeaderQuitEnteringCarAsDriver.h"


void CEventLeaderQuitEnteringCarAsDriver::InjectHooks()
{
    RH_ScopedVirtualClass(CEventLeaderQuitEnteringCarAsDriver, 0x86E6B0, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x63A110);
    RH_ScopedVMTInstall(AffectsPedGroup, 0x4B1010);
    RH_ScopedVMTInstall(GetEventType, 0x63A130);
    RH_ScopedVMTInstall(GetEventPriority, 0x63A1E0);
    RH_ScopedVMTInstall(GetLifeTime, 0x63A140);
}

// 0x63A110
CEventLeaderQuitEnteringCarAsDriver* CEventLeaderQuitEnteringCarAsDriver::Constructor()
{
    this->CEventLeaderQuitEnteringCarAsDriver::CEventLeaderQuitEnteringCarAsDriver();
    return this;
}

// 0x4B1010
bool CEventLeaderQuitEnteringCarAsDriver::AffectsPedGroup(CPedGroup* pedGroup) {
    const auto oe = pedGroup->GetIntelligence().GetCurrentEvent();
    return oe && oe->GetEvent().GetEventType() == EVENT_LEADER_ENTERED_CAR_AS_DRIVER;
}