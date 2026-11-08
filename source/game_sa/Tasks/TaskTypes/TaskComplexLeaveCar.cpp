#include "StdInc.h"

#include "TaskComplexLeaveCar.h"
#include "TaskComplexLeaveBoat.h"
#include "TaskComplexGetUpAndStandStill.h"
#include "TaskComplexCarSlowBeDraggedOut.h"
#include "TaskSimpleCarWaitForDoorNotToBeInUse.h"
#include "TaskSimpleCarWaitToSlowDown.h"
#include "TaskSimpleCarCloseDoorFromOutside.h"
#include "TaskSimpleCarGetOut.h"
#include "TaskSimpleCarJumpOut.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleDie.h"
#include "TaskSimplePause.h"
#include "CarEnterExit.h"
#include "Events/EventDamage.h"

void CTaskComplexLeaveCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexLeaveCar, 0x86E828, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(CreateSubTask, 0x641530);
    RH_ScopedVMTInstall(MakeAbortable, 0x641100);
}

// 0x62F1A0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime) : CTaskComplexLeaveCar(targetVehicle, nTargetDoor, nDelayTime, false, true) {
    m_bDie = true;
}

// 0x63B8C0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime, bool bSensibleLeaveCar, bool bForceGetOut) : CTaskComplex() {
    m_nTargetDoor                  = nTargetDoor;
    m_nDelayTime                   = nDelayTime;
    m_bSensibleLeaveCar            = bSensibleLeaveCar;
    m_pTargetVehicle               = targetVehicle;
    m_bForceGetOut                 = bForceGetOut;
    m_bDie                         = false;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_nDoorFlagsSet                = 0;
    m_nNumGettingInSet             = 0;
    m_nDieAnimID                   = ANIM_ID_KO_SHOT_FRONT_0;
    m_fDieAnimBlendDelta           = 4.0f;
    m_fDieAnimSpeed                = 1.0f;
    m_bIsInAir                     = false;

    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// 0x63B970
CTaskComplexLeaveCar::~CTaskComplexLeaveCar() {
    if (m_pTargetVehicle) {
        m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
        m_pTargetVehicle->CleanUpOldReference(reinterpret_cast<CEntity**>(&m_pTargetVehicle));
    }

    delete m_pTaskUtilityLineUpPedWithCar;
}

CTaskComplexLeaveCar::CTaskComplexLeaveCar(const CTaskComplexLeaveCar& o) :
    CTaskComplexLeaveCar{ o.m_pTargetVehicle, o.m_nTargetDoor, o.m_nDelayTime, o.m_bSensibleLeaveCar, o.m_bForceGetOut }
{
}

// 0x641100
bool CTaskComplexLeaveCar::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pTargetVehicle) {
        return true;
    }

    // Gives back the door flags and the seat that this task reserved in the vehicle
    const auto ReleaseVehicle = [this] {
        m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
        m_nDoorFlagsSet = 0;
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
        m_nNumGettingInSet = 0;
    };

    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);

        // A ped that is still in the vehicle is put out of it right away
        if (ped->bInVehicle) {
            if (!m_nTargetDoor) {
                m_nTargetDoor = CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, ped);
            }

            CTaskSimpleCarCloseDoorFromOutside closeDoor{ m_pTargetVehicle, (uint32)m_nTargetDoor, nullptr };
            closeDoor.MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);

            CTaskSimpleCarSetPedOut setPedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, m_bSensibleLeaveCar };
            setPedOut.ProcessPed(ped);
        }

        ReleaseVehicle();
        return true;
    }

    if (priority != ABORT_PRIORITY_URGENT) {
        return false;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();

    // Dying always aborts; waiting to slow down or getting up only abort for a damage event that killed the ped
    const auto* const damage = event && event->GetEventType() == EVENT_DAMAGE ? static_cast<const CEventDamage*>(event) : nullptr;
    if (subTaskType == TASK_SIMPLE_DIE
        || ((subTaskType == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN || subTaskType == TASK_COMPLEX_GET_UP_AND_STAND_STILL) && damage && damage->HasKilledPed())
    ) {
        const bool aborted = m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event);
        if (aborted) {
            ReleaseVehicle();
        }
        return aborted;
    }

    // Falling out of a vehicle while jumping out: the jump is aborted, but this task does not report it as done
    if (subTaskType == TASK_SIMPLE_CAR_JUMP_OUT) {
        if (!event) {
            return false;
        }

        const auto eventType = event->GetEventType();
        if (eventType != EVENT_IN_AIR && eventType != EVENT_IN_WATER) {
            return false;
        }

        if (eventType == EVENT_IN_AIR) {
            m_bIsInAir = true;
        }

        if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
            return false;
        }

        float blendDelta;
        if (ped->GetActiveWeapon().GetType() == WEAPON_PARACHUTE && eventType == EVENT_IN_AIR && m_pTargetVehicle->GetNumContactWheels() == 0) {
            // A plane that is not moving fast, or any other vehicle, needs a player with nothing under the feet
            if (m_pTargetVehicle->m_nVehicleSubType != VEHICLE_TYPE_PLANE || !(m_pTargetVehicle->m_vecMoveSpeed.Magnitude() > 0.2f)) {
                if (!ped->IsPlayer()) {
                    return false;
                }

                CColPoint colPoint;
                CEntity*  groundEntity;
                if (CWorld::ProcessVerticalLine(ped->GetPosition(), -10.0f, colPoint, groundEntity, true, false, false, false, true, false, nullptr)) {
                    return false;
                }
            }
            blendDelta = 8.0f;
        } else if (eventType == EVENT_IN_WATER && ped->m_vecMoveSpeed.z < -0.3f) {
            blendDelta = 16.0f;
        } else {
            return false;
        }

        CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FALL_FRONT, blendDelta);
        return false;
    }

    // Any other event only matters while the ped drives the vehicle for a set time
    if (event && subTaskType == TASK_SIMPLE_CAR_DRIVE_TIMED) {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event);
    }
    return false;
}

// 0x6419F0
CTask* CTaskComplexLeaveCar::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x6419F0, CTask*, CPed*>(this, ped);
}

// 0x641FC0
CTask* CTaskComplexLeaveCar::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x641FC0, CTask*, CPed*>(this, ped);
}

// 0x6421B0
CTask* CTaskComplexLeaveCar::ControlSubTask(CPed* ped) {
    if (!m_pTargetVehicle) {
        return nullptr;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();
    if (!ped->bInVehicle
        && subTaskType != TASK_SIMPLE_CAR_SET_PED_OUT
        && subTaskType != TASK_SIMPLE_CAR_JUMP_OUT
        && subTaskType != TASK_COMPLEX_GET_UP_AND_STAND_STILL
        && subTaskType != TASK_SIMPLE_DIE
    ) {
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }

    if (!m_bSensibleLeaveCar && subTaskType == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
    }

    return m_pSubTask;
}

// 0x641530
CTask* CTaskComplexLeaveCar::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE:
        return new CTaskSimpleCarWaitForDoorNotToBeInUse{ m_pTargetVehicle, (uint32)m_nTargetDoor, 0 };
    case TASK_COMPLEX_LEAVE_BOAT:
        return new CTaskComplexLeaveBoat{ m_pTargetVehicle, 0 };
    case TASK_SIMPLE_PAUSE:
        return new CTaskSimplePause{ -1 };
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        return new CTaskComplexGetUpAndStandStill{};
    case TASK_SIMPLE_DIE:
        return new CTaskSimpleDie{ ANIM_GROUP_DEFAULT, (AnimationId)m_nDieAnimID, m_fDieAnimBlendDelta, m_fDieAnimSpeed };
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ m_pTargetVehicle, m_nDelayTime };
    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE:
        return new CTaskSimpleCarCloseDoorFromOutside{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: {
        using SlowDownType = CTaskSimpleCarWaitToSlowDown::SlowDownType;
        if (m_bDie || !m_bSensibleLeaveCar) {
            return new CTaskSimpleCarWaitToSlowDown{ m_pTargetVehicle, SlowDownType::DONT_WAIT };
        }
        return new CTaskSimpleCarWaitToSlowDown{ m_pTargetVehicle, ped->IsPlayer() ? SlowDownType::SLOW_ENOUGH_TO_STEP_OR_JUMP : SlowDownType::SLOW_ENOUGH_TO_STEP };
    }
    case TASK_SIMPLE_CAR_SET_PED_OUT:
        return new CTaskSimpleCarSetPedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, m_bSensibleLeaveCar };
    case TASK_SIMPLE_CAR_GET_OUT:
        return new CTaskSimpleCarGetOut{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_JUMP_OUT:
        return new CTaskSimpleCarJumpOut{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_FORCE_PED_OUT: {
        // No class for this one yet, so build it with the original constructor
        const auto task = (CTask*)CTask::operator new(0x10);
        plugin::CallMethod<0x647710, CTask*, CVehicle*, int32>(task, m_pTargetVehicle, m_nTargetDoor);
        return task;
    }
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT:
        return new CTaskComplexCarSlowBeDraggedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, true };
    default:
        return nullptr;
    }
}
