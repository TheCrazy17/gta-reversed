#include "StdInc.h"

#include "TaskComplexFollowLeaderInFormation.h"
#include "General.h"
#include "Ped.h"
#include "PedGroup.h"
#include "PedGroupMembership.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexSeekEntityXYOffset.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleCarDrive.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexFollowLeaderInFormation::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowLeaderInFormation, 0x870c3c, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6949A0);
    RH_ScopedInstall(Destructor, 0x694A40);

    RH_ScopedVMTInstall(Clone, 0x695740);
    RH_ScopedVMTInstall(GetTaskType, 0x694A30);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x696820);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6968E0);
    RH_ScopedVMTInstall(ControlSubTask, 0x696940, { .Reversed = false });
}

// 0x6949A0
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(CPedGroup* pedGroup, CPed* ped, const CVector& posn, float a5) : CTaskComplex() {
    m_Group = pedGroup;
    m_Leader = ped;
    m_Pos = posn;
    m_Int = 4;
    m_Dist = a5;

    if (m_Leader) {
        CEntity::RegisterReference(m_Leader);
    }
}

// For 0x695740
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(const CTaskComplexFollowLeaderInFormation& o) :
    CTaskComplexFollowLeaderInFormation{
        o.m_Group,
        o.m_Leader,
        o.m_Pos,
        o.m_Dist
    }
{
}

// 0x694A40
CTaskComplexFollowLeaderInFormation::~CTaskComplexFollowLeaderInFormation() {
    CEntity::SafeCleanUpRef(m_Leader);
}

// 0x6962A0
CTask* CTaskComplexFollowLeaderInFormation::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_PAUSE:
        return new CTaskSimplePause{ 2000 };
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger{ m_Leader->m_pVehicle, 0, false };
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, eTargetDoor::TARGET_DOOR_FRONT_LEFT, 0, true, false };
    case TASK_SIMPLE_CAR_DRIVE:
        return new CTaskSimpleCarDrive{ ped->m_pVehicle, nullptr, false };
    case TASK_SIMPLE_ACHIEVE_HEADING: {
        auto& membership = m_Group->GetMembership();
        auto member = membership.GetMember(CGeneral::GetRandomNumberInRange(0, static_cast<int32>(membership.CountMembers())));
        if (member == ped) {
            member = membership.GetLeader();
        }

        // Only 1 out of 50 times the ped turns to face a group member, the rest of the times it just keeps its heading
        if (!member || CGeneral::GetRandomNumberInRange(0, 50) != 20) {
            return new CTaskSimpleAchieveHeading{ ped->m_fCurrentRotation };
        }

        auto dirToMember = member->GetPosition() - ped->GetPosition();
        dirToMember.Normalise();
        return new CTaskSimpleAchieveHeading{ CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(dirToMember.x, dirToMember.y, 0.0f, 0.0f)) };
    }
    case TASK_COMPLEX_SEEK_ENTITY: {
        const auto seek = new CTaskComplexSeekEntityXYOffset{
            m_Leader,
            50'000,
            1'000,
            1.0f,
            2.0f,
            2.0f,
            false,
            true,
            CEntitySeekPosCalculatorXYOffset{ m_Pos }
        };
        seek->SetMoveState(PEDMOVE_SPRINT);
        return seek;
    }
    default:
        return nullptr;
    }
}

// 0x696820
CTask* CTaskComplexFollowLeaderInFormation::CreateNextSubTask(CPed* ped) {
    // Read before the switch, the original does the same (and crashes on a missing leader just like here)
    const auto leaderMoveState = m_Leader->m_nMoveState;

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PAUSE:
        return CreateSubTask(TASK_SIMPLE_PAUSE, ped);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return CreateFirstSubTask(ped);
    case TASK_SIMPLE_STAND_STILL:
    case TASK_COMPLEX_LEAVE_CAR:
    case TASK_SIMPLE_CAR_DRIVE:
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    case TASK_COMPLEX_SEEK_ENTITY:
        // Rest if the leader isn't moving, otherwise chase it
        return CreateSubTask(
            leaderMoveState == PEDMOVE_NONE || leaderMoveState == PEDMOVE_STILL ? TASK_SIMPLE_STAND_STILL : TASK_COMPLEX_SEEK_ENTITY,
            ped
        );
    default:
        return nullptr;
    }
}

// 0x6968E0
CTask* CTaskComplexFollowLeaderInFormation::CreateFirstSubTask(CPed* ped) {
    if (!ped->bInVehicle || !ped->m_pVehicle) {
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    }

    if (!m_Leader->bInVehicle || m_Leader->m_pVehicle != ped->m_pVehicle) {
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    }

    // Both are in the same vehicle - drive it instead of leaving it
    return CreateSubTask(TASK_SIMPLE_CAR_DRIVE, ped);
}

// 0x696940
CTask* CTaskComplexFollowLeaderInFormation::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x696940, CTaskComplexFollowLeaderInFormation*, CPed*>(this, ped);
}
