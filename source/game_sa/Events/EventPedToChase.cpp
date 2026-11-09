#include "StdInc.h"

#include "EventPedToChase.h"

void CEventPedToChase::InjectHooks() {
    RH_ScopedVirtualClass(CEventPedToChase, 0x85B138, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4AF130);
    RH_ScopedInstall(Destructor, 0x4AF1C0);
    RH_ScopedVMTInstall(Clone, 0x4B7360);
}


// 0x4AF130
CEventPedToChase::CEventPedToChase(CPed* ped) {
    m_ped = ped;
    CEntity::SafeRegisterRef(m_ped);
}

// 0x4AF1C0
CEventPedToChase::~CEventPedToChase() {
    CEntity::SafeCleanUpRef(m_ped);
}
