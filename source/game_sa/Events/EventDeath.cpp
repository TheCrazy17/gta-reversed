#include "StdInc.h"

#include "EventDeath.h"

void CEventDeath::InjectHooks() {
    RH_ScopedVirtualClass(CEventDeath, 0x85ADC0, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor0, 0x4ADDF0);
    RH_ScopedInstall(Constructor1, 0x4ADE50);
    RH_ScopedVMTInstall(Clone, 0x4B6E30);
}


// 0x4ADE50, `time` should be int32
CEventDeath::CEventDeath(bool bDrowning, uint32 deathTimeInMs) : CEvent() {
    m_bDrowning = bDrowning;
    m_deathTimeInMs = deathTimeInMs;
}

// 0x4ADDF0
CEventDeath::CEventDeath(bool bDrowning) : CEvent() {
    m_bDrowning = bDrowning;
    m_deathTimeInMs = CTimer::GetTimeInMS();
}
