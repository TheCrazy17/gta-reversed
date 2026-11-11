#include "StdInc.h"

#include "EventPedToFlee.h"

void CEventPedToFlee::InjectHooks() {
    RH_ScopedVirtualClass(CEventPedToFlee, 0x85B178, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4AF240);
    RH_ScopedInstall(Destructor, 0x4AF2D0);
    RH_ScopedVMTInstall(Clone, 0x4B73D0);
    RH_ScopedVMTInstall(GetEventType, 0x4AF2A0);
    RH_ScopedVMTInstall(GetEventPriority, 0x4AF2C0);
    RH_ScopedVMTInstall(GetLifeTime, 0x4AF2B0);
}


// 0x4AF240
CEventPedToFlee::CEventPedToFlee(CPed* ped) {
    m_ped = ped;
    CEntity::SafeRegisterRef(m_ped);
}

// 0x4AF2D0
CEventPedToFlee::~CEventPedToFlee() {
    CEntity::SafeCleanUpRef(m_ped);
}
