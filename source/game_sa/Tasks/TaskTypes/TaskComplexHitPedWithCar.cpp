#include "StdInc.h"

#include "TaskComplexHitPedWithCar.h"

void CTaskComplexHitPedWithCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexHitPedWithCar, 0x86f294, 11);
    RH_ScopedCategory(); // TODO: Change this to the appropriate category!

    RH_ScopedInstall(Constructor, 0x6539A0);
    RH_ScopedInstall(Destructor, 0x653A30);

    RH_ScopedGlobalInstall(ComputeEvasiveStepMoveDir, 0x653B40);

    RH_ScopedInstall(HitHurtsPed, 0x653AE0);
    RH_ScopedInstall(CreateSubTask, 0x6560E0, { .reversed = false });

    RH_ScopedVMTInstall(Clone, 0x6559B0);
    RH_ScopedVMTInstall(GetTaskType, 0x653A20);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x657AF0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x656300);
    RH_ScopedVMTInstall(ControlSubTask, 0x653A90);

}

// 0x6539A0
CTaskComplexHitPedWithCar::CTaskComplexHitPedWithCar(CVehicle* veh, float impulseMagnitude) :
    m_Veh{veh},
    m_ImpulseMag{impulseMagnitude}
{
    CEntity::SafeRegisterRef(m_Veh);
}

CTaskComplexHitPedWithCar::CTaskComplexHitPedWithCar(const CTaskComplexHitPedWithCar& o) :
    CTaskComplexHitPedWithCar{o.m_Veh, o.m_ImpulseMag}
{
}

// 0x653A30
CTaskComplexHitPedWithCar::~CTaskComplexHitPedWithCar() {
    CEntity::SafeCleanUpRef(m_Veh);
}

// 0x653B40
CVector CTaskComplexHitPedWithCar::ComputeEvasiveStepMoveDir(const CPed* ped, CVehicle* veh) {
    return CPedGeometryAnalyser::ComputeEntityDir(*veh, (eDirection)CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *veh)); // Same shit, difference packaging
}

// 0x653AE0
bool CTaskComplexHitPedWithCar::HitHurtsPed(CPed* ped) {
    const auto threshold = ped->IsPlayer() ? 10.0f : 6.0f;
    if (threshold < m_ImpulseMag) {
        return true;
    }

    // NOTSA: `ped+0xe8` looks like a per-ped float field (compared against -0.8) - exact field
    // name not identified this session, kept as a raw offset check.
    if (*(float*)((char*)ped + 0xe8) >= -0.8f) {
        return false;
    }

    return m_ImpulseMag > 3.0f;
}

// 0x6560E0
CTask* CTaskComplexHitPedWithCar::CreateSubTask(eTaskType tt) {
    return plugin::CallMethodAndReturn<CTask*, 0x6560E0, CTaskComplexHitPedWithCar*, eTaskType>(this, tt);
}

// 0x657AF0
CTask* CTaskComplexHitPedWithCar::CreateNextSubTask(CPed* ped) {
    const auto subTaskType = m_pSubTask->GetTaskType();
    if (subTaskType < TASK_SIMPLE_EVASIVE_DIVE) {
        if (subTaskType != TASK_COMPLEX_EVASIVE_STEP && subTaskType != TASK_NONE &&
            subTaskType != TASK_COMPLEX_FALL_AND_GET_UP && subTaskType != TASK_SIMPLE_HIT_BEHIND
        ) {
            return nullptr;
        }
    } else if (subTaskType != TASK_SIMPLE_KILL_PED_WITH_CAR) {
        if (subTaskType != TASK_SIMPLE_HURT_PED_WITH_CAR) {
            return nullptr;
        }
        // NOTSA: `m_pSubTask+0x10` is CTaskSimpleHurtPedWithCar's private `m_bWillKillPed` -
        // no public accessor exists, kept as a raw offset check.
        if (*(bool*)((char*)m_pSubTask + 0x10)) {
            return nullptr;
        }
        return CreateSubTask(TASK_COMPLEX_FALL_AND_GET_UP);
    }
    return CreateSubTask(TASK_FINISHED);
}

// 0x656300
CTask* CTaskComplexHitPedWithCar::CreateFirstSubTask(CPed* ped) {
    m_PedHitSide = CPedGeometryAnalyser::ComputePedHitSide(*ped, *m_Veh);

    const auto threshold = ped->IsPlayer() ? 20.0f : 12.0f;
    if (m_ImpulseMag <= threshold) {
        if (!HitHurtsPed(ped)) {
            m_MoveDir = ComputeEvasiveStepMoveDir(ped, m_Veh);
            return CreateSubTask(TASK_COMPLEX_EVASIVE_STEP);
        }
        return CreateSubTask(TASK_SIMPLE_HURT_PED_WITH_CAR);
    }
    return CreateSubTask(TASK_SIMPLE_KILL_PED_WITH_CAR);
}

// 0x653A90
CTask* CTaskComplexHitPedWithCar::ControlSubTask(CPed* ped) {
    return m_pSubTask;
}
