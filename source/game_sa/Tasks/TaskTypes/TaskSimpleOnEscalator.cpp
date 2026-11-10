#include "StdInc.h"

#include "TaskSimpleOnEscalator.h"
#include "ModelIndices.h"

void CTaskSimpleOnEscalator::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleOnEscalator, 0x85B8DC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x4B8670);
    RH_ScopedVMTInstall(Clone, 0x4B86C0);
    RH_ScopedVMTInstall(GetTaskType, 0x4B8720);
    RH_ScopedVMTInstall(ProcessPed, 0x62F520);
}

// 0x4B8670
CTaskSimpleOnEscalator::CTaskSimpleOnEscalator() :
    CTaskSimpleStandStill{0, true, false, 8.0f} // Stands still forever (looped)
{
}

// 0x62F520
bool CTaskSimpleOnEscalator::ProcessPed(CPed* ped) {
    CTaskSimpleStandStill::ProcessPed(ped); // The result is ignored by the exe; only the idle pose handling is wanted

    const auto contact = ped->m_pContactEntity;
    if (!contact) {
        return true;
    }

    const auto modelId = contact->m_nModelIndex;
    if (modelId != ModelIndices::MI_ESCALATORSTEP && modelId != ModelIndices::MI_ESCALATORSTEP8) {
        return true; // The ped stepped off the escalator
    }

    // Aim along the step's horizontal velocity; the contact entity of a ped on an escalator is a moving CObject
    const auto& stepSpeed = contact->AsPhysical()->m_vecMoveSpeed;
    ped->m_fAimingRotation = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(stepSpeed.x, stepSpeed.y, 0.0f, 0.0f));
    return false;
}
