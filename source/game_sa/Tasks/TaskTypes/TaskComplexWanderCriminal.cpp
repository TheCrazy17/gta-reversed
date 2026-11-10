#include "StdInc.h"

#include "TaskComplexWanderCriminal.h"
#include "EventVehicleToSteal.h"
#include "CarEnterExit.h"

void CTaskComplexWanderCriminal::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderCriminal, 0x85A23C, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x48E610);
    RH_ScopedInstall(LookForCarsToSteal, 0x66B4F0);
    RH_ScopedVMTInstall(Clone, 0x48E650);
    RH_ScopedVMTInstall(GetWanderType, 0x48E6F0);
    RH_ScopedVMTInstall(ScanForStuff, 0x670350);
}

// 0x48E610
CTaskComplexWanderCriminal::CTaskComplexWanderCriminal(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) : CTaskComplexWander(MoveState, Dir, bWanderSensibly) { }

// 0x670350
void CTaskComplexWanderCriminal::ScanForStuff(CPed* ped) {
    if (!m_TaskTimer.IsStarted()) {
        // First call; the first look for cars waits up to 30 seconds
        m_TaskTimer.Start(50);
        m_nMinNextScanTime = CTimer::GetTimeInMS() + (uint32)CGeneral::GetRandomNumberInRange(0.0f, 30'000.0f);
    }

    if (m_TaskTimer.IsOutOfTime()) {
        m_TaskTimer.Start(50);
        if (CTimer::GetTimeInMS() >= m_nMinNextScanTime) {
            LookForCarsToSteal(ped);
        }
    }
}

// 0x66B4F0
void CTaskComplexWanderCriminal::LookForCarsToSteal(CPed* ped) {
    CVehicle* closest{};
    float     closestDistSq = std::numeric_limits<float>::max();
    for (CEntity* const entity : ped->GetIntelligence()->GetVehicleScanner().m_apEntities) {
        auto* const veh = static_cast<CVehicle*>(entity);
        if (!veh || !CCarEnterExit::IsVehicleStealable(veh, ped)) {
            continue;
        }
        const float distSq = (veh->GetPosition() - ped->GetPosition()).SquaredMagnitude();
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closest       = veh;
        }
    }

    if (!closest) {
        return;
    }

    m_nMinNextScanTime = CTimer::GetTimeInMS() + 30'000;
    if (CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.2f) {
        ped->GetEventGroup().Add(CEventVehicleToSteal{ closest });
    }
}
