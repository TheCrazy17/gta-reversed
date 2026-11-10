#include "StdInc.h"

#include "TaskComplexWanderProstitute.h"
#include "TaskComplexProstituteSolicit.h"
#include "EventAcquaintancePedRespect.h"

void CTaskComplexWanderProstitute::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderProstitute, 0x870148, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x672690);
    RH_ScopedVMTInstall(Clone, 0x673C80);
    RH_ScopedVMTInstall(GetWanderType, 0x6726C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x674920);
    RH_ScopedVMTInstall(ScanForStuff, 0x672700);
}

// 0x672690
CTaskComplexWanderProstitute::CTaskComplexWanderProstitute(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWanderStandard(MoveState, Dir, bWanderSensibly),
    m_nStartTimeInMs{ 0 }
{
}

// 0x672700
void CTaskComplexWanderProstitute::ScanForStuff(CPed* ped) {
    CTaskComplexWanderStandard::ScanForStuff(ped);

    const auto now = CTimer::GetTimeInMS();
    if (now <= m_nStartTimeInMs) {
        return;
    }
    m_nStartTimeInMs = now + 2000;

    // Does not look for customers while a cop is around
    auto* const nearbyPeds = ped->GetIntelligence()->GetPedEntities();
    for (size_t i = 0; i < MAX_NUM_ENTITIES; i++) {
        const auto* const other = static_cast<CPed*>(nearbyPeds[i]);
        if (other && other->m_nPedType == PED_TYPE_COP) {
            return;
        }
    }

    for (size_t i = 0; i < MAX_NUM_ENTITIES; i++) {
        auto* const other = static_cast<CPed*>(nearbyPeds[i]);
        if (!other || !other->GetPlayerData() || !other->m_pVehicle) {
            continue;
        }
        if (other->GetPlayerData()->m_pLastProstituteShagged == ped) {
            continue;
        }

        // Customers in expensive cars are more likely to be picked
        if (other->m_pVehicle->m_pHandlingData->m_nMonetaryValue <= (uint32)(CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 50'000.0f)) {
            continue;
        }

        // Only cars that are almost stopped
        if ((other->m_pVehicle->m_vecMoveSpeed * 50.0f).SquaredMagnitude() >= 4.0f) {
            continue;
        }

        if (!CTaskComplexProstituteSolicit::IsTaskValid(ped, other)) {
            continue;
        }

        ped->GetEventGroup().Add(CEventAcquaintancePedRespect{ other, TASK_COMPLEX_PROSTITUTE_SOLICIT });
    }
}
