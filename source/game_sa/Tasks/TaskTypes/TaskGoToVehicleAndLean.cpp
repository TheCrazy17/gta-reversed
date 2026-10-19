#include "StdInc.h"

#include "TaskGoToVehicleAndLean.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexGangLeader.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleStandStill.h"
#include "TaskLeanOnVehicle.h"

void CTaskGoToVehicleAndLean::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGoToVehicleAndLean, 0x86FAC8, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x660E60);
    RH_ScopedInstall(Destructor, 0x660EE0);
    RH_ScopedInstall(Clone, 0x6621B0);
    RH_ScopedInstall(CalcTargetPos, 0x664770);
    RH_ScopedInstall(MakeAbortable, 0x664500);
    RH_ScopedInstall(CreateNextSubTask, 0x664590);
    RH_ScopedInstall(CreateFirstSubTask, 0x664D40);
    RH_ScopedInstall(ControlSubTask, 0x664E60);
}

// 0x660E60
CTaskGoToVehicleAndLean::CTaskGoToVehicleAndLean(CVehicle* vehicle, int32 leanAnimDurationInMs) : CTaskComplex() {
    m_Vehicle = vehicle;
    m_LeanAnimDurationInMs = leanAnimDurationInMs;
    m_LeanOnVehicle = false;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x660EE0
CTaskGoToVehicleAndLean::~CTaskGoToVehicleAndLean() {
    if (m_LeanOnVehicle) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_LeanOnVehicle = false;
    }
    CEntity::SafeCleanUpRef(m_Vehicle);
}

// 0x664500
bool CTaskGoToVehicleAndLean::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);
    } else if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }
    DoTidyUp(ped);
    return true;
}

// 0x664770
CVector CTaskGoToVehicleAndLean::CalcTargetPos(CPed* ped) {
    const auto& box = CModelInfo::GetModelInfo(m_Vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox();

    // Stand on the side the ped is currently on
    const auto toPed = ped->GetPosition() - m_Vehicle->GetPosition();
    m_IsOnRightSide  = DotProduct(m_Vehicle->GetRight(), toPed) > 0.0f;

    const CVector local{ m_IsOnRightSide ? box.m_vecMax.x + 0.5f : box.m_vecMin.x - 0.5f, 0.0f, 0.0f };
    return m_Vehicle->GetMatrix().TransformPoint(local);
}

// 0x664590
CTask* CTaskGoToVehicleAndLean::CreateNextSubTask(CPed* ped) {
    if (!m_Vehicle) {
        if (m_pSubTask) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
        }
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        // Arrived, so face along the vehicle
        auto dir = m_Vehicle->GetRight();
        if (!m_IsOnRightSide) {
            dir = -dir;
        }
        const auto heading = CGeneral::GetRadianAngleBetweenPoints(dir.x, dir.y, 0.0f, 0.0f);
        return new CTaskSimpleAchieveHeading{ heading, 0.5f, 0.2f };
    }

    const auto subTaskType = m_pSubTask->GetTaskType();
    if (subTaskType != TASK_SIMPLE_ACHIEVE_HEADING && subTaskType != TASK_SIMPLE_STAND_STILL) {
        DoTidyUp(ped);
        return nullptr;
    }

    if (m_LeanOnVehicle) {
        return new CTaskLeanOnVehicle{ m_Vehicle, m_LeanAnimDurationInMs, 0 };
    }
    return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
}

// 0x664D40
CTask* CTaskGoToVehicleAndLean::CreateFirstSubTask(CPed* ped) {
    if (m_Vehicle) {
        if (!m_Vehicle->vehicleFlags.bHasGangLeaningOn) {
            m_Vehicle->vehicleFlags.bHasGangLeaningOn = true;
            m_TargetPos = CalcTargetPos(ped);
            if ((m_TargetPos - ped->GetPosition()).SquaredMagnitude() >= 1.0f) {
                return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, m_TargetPos, 0.05f, 2.0f, false, true };
            }
            return nullptr;
        }
        // Someone else is already leaning on it
        m_Vehicle->vehicleFlags.bHasGangLeaningOn = false;
    }
    if (m_pSubTask) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
    }
    return nullptr;
}

// 0x664E60
CTask* CTaskGoToVehicleAndLean::ControlSubTask(CPed* ped) {
    if (!m_Vehicle) {
        if (m_pSubTask) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
        }
        return nullptr;
    }

    // Give up if the ped has been stuck for a while
    if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) {
        m_Vehicle->vehicleFlags.bHasGangLeaningOn = false;
        if (m_pSubTask) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
        }
        return nullptr;
    }

    // And when the vehicle starts moving
    const auto speed = m_Vehicle->GetSpeed(ped->GetPosition() - m_Vehicle->GetPosition());
    if ((m_Vehicle->m_pDriver || speed.Magnitude() > 0.01f) && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        DoTidyUp(ped);
        return nullptr;
    }

    // Or the spot to stand on has moved
    if ((CalcTargetPos(ped) - m_TargetPos).SquaredMagnitude() > 0.01f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        DoTidyUp(ped);
        return nullptr;
    }

    // Same gang anim bookkeeping as the other gang tasks
    if (!m_LeanOnVehicle) {
        if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
            if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
                CAnimManager::AddAnimBlockRef(blk);
                m_LeanOnVehicle = true;
            } else {
                CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
            }
        }
    } else if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_LeanOnVehicle = false;
    }
    return m_pSubTask;
}

// 0x660F60
void CTaskGoToVehicleAndLean::DoTidyUp(CPed* ped) {
    if (m_Vehicle)
        m_Vehicle->vehicleFlags.bHasGangLeaningOn = false;

    if (m_pSubTask)
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
}
