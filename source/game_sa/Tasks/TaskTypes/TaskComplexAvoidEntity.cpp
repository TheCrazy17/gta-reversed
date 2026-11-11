#include "StdInc.h"

#include <Ragdoll/IKChainManager.h>
#include <PedGeometryAnalyser.h>
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleGoTo.h"
#include "TaskComplexAvoidEntity.h"
#include "Collision/ColSphere.h"

void CTaskComplexAvoidEntity::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexAvoidEntity, 0x86FF00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x66AA20);
    RH_ScopedInstall(Destructor, 0x66AAE0);

    RH_ScopedInstall(SetUpIK, 0x66AB40);
    RH_ScopedInstall(QuitIK, 0x66AD10);
    RH_ScopedInstall(ComputeSphere, 0x66F6C0);
    RH_ScopedInstall(ComputeRouteRoundSphere, 0x66FD30);
    RH_ScopedVMTInstall(Clone, 0x66D0C0);
    RH_ScopedVMTInstall(GetTaskType, 0x66AAD0);
    RH_ScopedVMTInstall(MakeAbortable, 0x66AD40);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x66AD70);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x672530);
    RH_ScopedVMTInstall(ControlSubTask, 0x66ADE0);
}

// 0x66AA20
CTaskComplexAvoidEntity::CTaskComplexAvoidEntity(eMoveState moveState, CEntity* entityToAvoid, const CVector& targetPoint) :
    CTaskComplex(),
    m_EntityToAvoid{ entityToAvoid },
    m_MoveState{ moveState },
    m_TargetPt{ targetPoint },
    m_DetourTargetPt{ targetPoint }
{
}

// 0x66AD40
bool CTaskComplexAvoidEntity::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    QuitIK(ped);
    return true;
}

// 0x66AD70
CTask* CTaskComplexAvoidEntity::CreateNextSubTask(CPed* ped) {
    QuitIK(ped);
    return nullptr;
}

// 0x672530
CTask* CTaskComplexAvoidEntity::CreateFirstSubTask(CPed* ped) {
    m_StartPt = ped->GetPosition();
    if (!ComputeRouteRoundSphere(ped)) {
        QuitIK(ped);
        return nullptr;
    }
    return new CTaskSimpleGoToPoint{ m_MoveState, m_DetourTargetPt, 0.5f };
}

// 0x66ADE0
CTask* CTaskComplexAvoidEntity::ControlSubTask(CPed* ped) {
    if (!m_EntityToAvoid) {
        QuitIK(ped);
        return nullptr;
    }

    SetUpIK(ped);

    if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) {
        QuitIK(ped);
        return nullptr;
    }

    // The task is over once the entity is more than 15 units away
    if ((ped->GetPosition() - m_EntityToAvoid->GetPosition()).SquaredMagnitude() > sq(7.5f) * 4.0f) {
        QuitIK(ped);
        return nullptr;
    }

    return m_pSubTask;
}

// 0x66AD10
void CTaskComplexAvoidEntity::QuitIK(CPed* ped) {
    if (m_IsDoingIK && g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped);
    }
}

// 0x66AB40
void CTaskComplexAvoidEntity::SetUpIK(CPed* ped) {
    // The player only looks round by itself when the controls are locked
    if (ped == FindPlayerPed() && !CPad::GetPad()->DisablePlayerControls) {
        return;
    }
    if (!ped->GetIsOnScreen()) {
        return;
    }
    if (m_IsDoingIK) {
        return;
    }
    // Something else already makes the ped look somewhere
    if (g_ikChainMan.GetLookAtEntity(ped)) {
        return;
    }
    if (ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_IK)) {
        return;
    }
    if (m_Parent) {
        switch (m_Parent->GetTaskType()) {
        case TASK_COMPLEX_AVOID_OTHER_PED_WHILE_WANDERING:
        case TASK_COMPLEX_AVOID_ENTITY:
            return;
        }
    }
    const auto pedToTarget       = m_TargetPt - ped->GetPosition();
    const auto pedToTargetDistSq = pedToTarget.SquaredMagnitude();
    if (pedToTargetDistSq <= sq(3.f)) {
        return;
    }
    const auto pedToTargetDir = pedToTarget / std::sqrt(pedToTargetDistSq); // Normalized
    if (ped->GetForward().Dot(pedToTargetDir) >= CTaskSimpleGoTo::ms_fLookAtThresholdDotProduct) {
        return;
    }
    // Look a bit past the target; x and y come from the target, but z from the ped
    CVector lookAt = m_TargetPt + pedToTargetDir * 2.f;
    lookAt.z = ped->GetPosition().z + pedToTargetDir.z * 2.f + 0.61f;
    g_ikChainMan.LookAt("TaskAvoidEntity", ped, nullptr, 5000, BONE_UNKNOWN, &lookAt, false, 0.25f, 500, 3, false);
    m_IsDoingIK = true;
}

// 0x66F6C0
void CTaskComplexAvoidEntity::ComputeSphere(CColSphere& outSphere, EntitiesToAvoidArray& entities) {
    // Step 1: Find the centre (z is left at 0)
    CVector sum{};
    size_t  numEntities{};
    for (CEntity* const entity : entities) {
        if (!entity) {
            continue;
        }
        numEntities++;
        sum += entity->GetPosition();
    }
    const float invNum = 1.f / (float)numEntities;
    const CVector center{ sum.x * invNum, sum.y * invNum, 0.f };

    // Step 2: The radius covers the 2D distance to the centre and the bounding radius of each entity, squared and added up
    float maxRadiusSq{ 0.f };
    for (CEntity* const entity : entities) {
        if (!entity) {
            continue;
        }
        const auto  offset      = entity->GetPosition() - center;
        const float boundRadius = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->GetBoundRadius();
        const float radiusSq    = sq(offset.x) + sq(offset.y) + sq(boundRadius);
        if (radiusSq > maxRadiusSq) {
            maxRadiusSq = radiusSq;
        }
    }

    outSphere.Set(std::sqrt(maxRadiusSq) + 0.7f, center, SURFACE_DEFAULT, 0, tColLighting{ 0xFF });
}

// 0x66FD30
bool CTaskComplexAvoidEntity::ComputeRouteRoundSphere(CPed* ped) {
    EntitiesToAvoidArray entities{ m_EntityToAvoid };
    CColSphere           sphere;
    ComputeSphere(sphere, entities);
    sphere.m_vecCenter.z = ped->GetPosition().z;
    return CPedGeometryAnalyser::ComputeRouteRoundSphere(
        *ped,
        sphere,
        m_StartPt,
        m_TargetPt,
        m_NewTargetPt,
        m_DetourTargetPt
    );
}
