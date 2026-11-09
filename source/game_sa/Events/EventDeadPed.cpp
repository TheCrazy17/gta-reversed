#include "StdInc.h"

#include "EventDeadPed.h"

#include "Ragdoll/IKChainManager.h"

void CEventDeadPed::InjectHooks() {
    RH_ScopedVirtualClass(CEventDeadPed, 0x85AE00, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4ADEA0);
    RH_ScopedInstall(Destructor, 0x4ADF70);
    RH_ScopedVMTInstall(AffectsPed, 0x4B4830);
    RH_ScopedVMTInstall(CloneEditable, 0x4B6E70);
}


// 0x4ADEA0
CEventDeadPed::CEventDeadPed(CPed* ped, bool bUnknown, uint32 deathTimeInMs) : CEventEditableResponse() {
    m_ped = ped;
    field_18 = bUnknown;
    m_deathTimeInMs = deathTimeInMs;
    CEntity::SafeRegisterRef(m_ped);
}

// 0x4ADF70
CEventDeadPed::~CEventDeadPed() {
    CEntity::SafeCleanUpRef(m_ped);
}

// 0x4B4830
bool CEventDeadPed::AffectsPed(CPed* ped) {
    if (!m_ped || ped == m_ped || !ped->IsAlive())
        return false;

    if (g_ikChainMan.IsLooking(ped) && g_ikChainMan.GetLookAtEntity(ped) == m_ped)
        return false;

    if (m_ped->physicalFlags.bSubmergedInWater)
        return false;

    const CVector distance = m_ped->GetPosition() - ped->GetPosition();
    const float squaredMagnitude = distance.SquaredMagnitude();

    // A ped killed by stealth is only noticed from the front and from close up; any other dead ped is noticed within 20 units.
    // The exe skips the stealth test (0x4B48D7) for a ped that was not killed that way, where this used to return false
    if (m_ped->bKilledByStealth && (DotProduct(distance, ped->GetForward()) < 0.0f || squaredMagnitude > sq(8.0f))) {
        return false;
    }
    return sq(20.0f) > squaredMagnitude;
}

// 0x4B6E70
CEventEditableResponse* CEventDeadPed::CloneEditable() const noexcept {
    return new CEventDeadPed(m_ped, field_18, m_deathTimeInMs);

}
