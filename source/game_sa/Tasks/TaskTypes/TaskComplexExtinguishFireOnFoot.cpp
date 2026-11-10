#include "StdInc.h"

#include "TaskComplexExtinguishFireOnFoot.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleGunControl.h"
#include "FireManager.h"
#include "Fire.h"

void CTaskComplexExtinguishFireOnFoot::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexExtinguishFireOnFoot, 0x86F5FC, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x659870);
    RH_ScopedInstall(FindNearbyFire, 0x659990);

    RH_ScopedVMTInstall(Clone, 0x659D70);
    RH_ScopedVMTInstall(GetTaskType, 0x6598A0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6598C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x65A720);
    RH_ScopedVMTInstall(ControlSubTask, 0x659980);
}

// 0x659870
CTaskComplexExtinguishFireOnFoot::CTaskComplexExtinguishFireOnFoot(const CVector& firePos) :
    m_FirePos{firePos}
{
}

// 0x659990
CFire* CTaskComplexExtinguishFireOnFoot::FindNearbyFire(CPed* ped) {
    const auto& pedPos = ped->GetPosition();

    // Skips script fires; fires that are already being extinguished are still candidates
    const auto fire = gFireManager.FindNearestFire(pedPos, false, true);
    if (!fire) {
        return nullptr;
    }

    // `FindNearestFire` ranks by the 2D distance, this is the 3D range check (negated like the exe, so NaN rejects)
    if (!((pedPos - fire->GetPosition()).SquaredMagnitude() < sq(10.0f))) {
        return nullptr;
    }

    return fire;
}

// 0x6598C0
CTask* CTaskComplexExtinguishFireOnFoot::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: // Arrived at the fire
        return new CTaskSimpleGunControl{nullptr, m_FirePos, CVector{}, eGunCommand::RELOAD, 1, -1};
    case TASK_SIMPLE_GUN_CTRL: // Look for the next fire
        return CreateFirstSubTask(ped);
    default:
        return nullptr;
    }
}

// 0x65A720
CTask* CTaskComplexExtinguishFireOnFoot::CreateFirstSubTask(CPed* ped) {
    const auto fire = FindNearbyFire(ped);
    if (!fire) {
        return nullptr;
    }

    m_FirePos = fire->GetPosition();

    return new CTaskComplexGoToPointAndStandStill{PEDMOVE_RUN, fire->GetPosition(), 2.0f, 2.0f, false, false};
}
