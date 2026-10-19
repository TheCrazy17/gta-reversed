#include "StdInc.h"
#include "TaskComplexEnterCarAsPassengerWait.h"
#include "TaskComplexEnterCarAsPassengerTimed.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleWaitUntilPedIsInCar.h"
#include "CarEnterExit.h"
#include "PedGeometryAnalyser.h"

void CTaskComplexEnterCarAsPassengerWait::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexEnterCarAsPassengerWait, 0x86E778, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x63B320);
    RH_ScopedInstall(Destructor, 0x63B3C0);

    RH_ScopedInstall(CreateSubTask, 0x6408D0);
    RH_ScopedVMTInstall(Clone, 0x63D850);
    RH_ScopedVMTInstall(GetTaskType, 0x63B3B0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x643E10);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x643F00);
    RH_ScopedVMTInstall(ControlSubTask, 0x640730);
}

// 0x63B320
CTaskComplexEnterCarAsPassengerWait::CTaskComplexEnterCarAsPassengerWait(CVehicle* target, CPed* waitFor, bool forceFrontSeat, eMoveState ms) :
    m_Car{target},
    m_WaitForPed{waitFor},
    m_bForceFrontSeat{forceFrontSeat},
    m_MoveState{ms}
{
    CEntity::SafeRegisterRef(m_Car);
    CEntity::SafeRegisterRef(m_WaitForPed);
}
    
// NOTSA (for 0x63D850)
CTaskComplexEnterCarAsPassengerWait::CTaskComplexEnterCarAsPassengerWait(const CTaskComplexEnterCarAsPassengerWait& o) :
    CTaskComplexEnterCarAsPassengerWait{
        o.m_Car,
        o.m_WaitForPed,
        o.m_bForceFrontSeat,
        o.m_MoveState
    }
{
}

// 0x63B3C0
CTaskComplexEnterCarAsPassengerWait::~CTaskComplexEnterCarAsPassengerWait() {
    CEntity::SafeCleanUpRef(m_Car);
    CEntity::SafeCleanUpRef(m_WaitForPed);
}

// 0x6408D0
CTask* CTaskComplexEnterCarAsPassengerWait::CreateSubTask(int32 taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR:
        return new CTaskSimpleWaitUntilPedIsInCar{ m_WaitForPed };
    case TASK_COMPLEX_SEQUENCE: {
        const auto seq = new CTaskComplexSequence{};
        seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_Car, 0.5f, 0.2f });
        seq->AddTask(new CTaskSimpleStandStill{ 1000, false, false, 8.0f });
        return seq;
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED: {
        const auto door = m_bForceFrontSeat ? CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(m_Car, 0) : 0;
        const auto task = new CTaskComplexEnterCarAsPassengerTimed{ m_Car, (uint32)door, (uint32)-1, true };
        task->SetMoveState(m_MoveState);
        return task;
    }
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ m_Car, 0 };
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        CVector surfacePoint;
        if (CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Car, surfacePoint)) {
            return new CTaskComplexGoToPointAndStandStill{ m_MoveState, surfacePoint, 1.5f, 2.0f, false, false };
        }
        const auto radius = CModelInfo::GetModelInfo(m_Car->m_nModelIndex)->GetColModel()->GetBoundRadius();
        return new CTaskComplexGoToPointAndStandStill{ m_MoveState, m_Car->GetPosition(), radius + 1.5f, 2.0f, false, false };
    }
    default:
        return nullptr;
    }
}

// 0x643E10
CTask* CTaskComplexEnterCarAsPassengerWait::CreateNextSubTask(CPed* ped) {
    if (!m_Car) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_SEQUENCE:
    case TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR:
        return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED, ped);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED:
        // Retry a limited number of times if the ped failed to get in
        if (ped->bInVehicle || ++m_EnterCarFails > 15) {
            return CreateSubTask(TASK_FINISHED, ped);
        }
        return CreateSubTask(TASK_COMPLEX_SEQUENCE, ped);
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return CreateSubTask(m_WaitForPed ? TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR : TASK_FINISHED, ped);
    default:
        return nullptr;
    }
}

// 0x643F00
CTask* CTaskComplexEnterCarAsPassengerWait::CreateFirstSubTask(CPed* ped) {
    if (ped->bInVehicle && ped->m_pVehicle && ped->m_pVehicle == m_Car && m_Car->IsPassenger(ped)) {
        return CreateSubTask(TASK_SIMPLE_CAR_DRIVE_TIMED, ped);
    }
    if (!m_WaitForPed) {
        return CreateSubTask(m_Car ? TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED : TASK_FINISHED, ped);
    }
    return CreateSubTask(m_Car ? TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL : TASK_FINISHED, ped);
}

// 0x640730
CTask* CTaskComplexEnterCarAsPassengerWait::ControlSubTask(CPed* ped) {
    // Say something to the player once the ped is getting in
    if (!m_bPlayedSample) {
        if (m_WaitForPed && m_WaitForPed->IsPlayer()) {
            if (ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_GET_IN)) {
                m_bPlayedSample = true;
                ped->Say(CTX_GLOBAL_CAR_GET_IN);
            }
        } else if (!m_WaitForPed) {
            if (const auto group = CPedGroups::GetPedsGroup(ped)) {
                const auto leader = group->GetMembership().GetLeader();
                if (leader && leader->IsPlayer() && ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_GET_IN)) {
                    m_bPlayedSample = true;
                    ped->Say(CTX_GLOBAL_CAR_WAIT_FOR_ME);
                }
            }
        }
    }

    if (m_Car && m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        const auto goToTask = static_cast<CTaskComplexGoToPointAndStandStill*>(m_pSubTask);
        CVector    surfacePoint;
        if (CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Car, surfacePoint)) {
            if ((surfacePoint - goToTask->m_vecTargetPoint).SquaredMagnitude() > 0.025f) {
                goToTask->GoToPoint(surfacePoint, 1.5f, 2.0f, false);
            }
        } else {
            // The original compares against an unset point here, the target is only updated when it changed
            const auto radius = CModelInfo::GetModelInfo(m_Car->m_nModelIndex)->GetColModel()->GetBoundRadius();
            goToTask->GoToPoint(m_Car->GetPosition(), radius + 1.5f, 2.0f, false);
        }
    }
    return m_pSubTask;
}
