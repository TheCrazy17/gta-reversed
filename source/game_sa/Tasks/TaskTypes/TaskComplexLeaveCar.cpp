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
#include "Automobile.h"
#include "PedGroups.h"
#include "PedGroup.h"
#include "Events/EventDamage.h"
#include "Events/EventGroupEvent.h"
#include "Events/EventLeaderExitedCarAsDriver.h"

void CTaskComplexLeaveCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexLeaveCar, 0x86E828, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x63B8C0);
    RH_ScopedInstall(Destructor, 0x63B970);
    RH_ScopedInstall(CreateSubTask, 0x641530);
    RH_ScopedInstall(ReserveDoorAndSeat, 0x63BA00);
    RH_ScopedInstall(ComputeTargetDoorIfUnset, 0x63BAB0);
    RH_ScopedInstall(CreateLineUpUtility, 0x63BAE0);
    RH_ScopedInstall(GetDoorUse, 0x646CF0);
    RH_ScopedVMTInstall(GetTaskType, 0x63B960);
    RH_ScopedVMTInstall(Clone, 0x63D9E0);
    RH_ScopedVMTInstall(MakeAbortable, 0x641100);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6419F0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x641FC0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6421B0);
}

// 0x63BA00
void CTaskComplexLeaveCar::ReserveDoorAndSeat(CPed* ped) {
    m_nDoorFlagsSet = (uint8)CCarEnterExit::ComputeDoorFlag(m_pTargetVehicle, m_nTargetDoor, true);
    m_pTargetVehicle->SetGettingOutFlags(m_nDoorFlagsSet);
    m_nNumGettingInSet = 1;
    m_pTargetVehicle->m_nNumGettingIn++;

    // A computer controlled driver that leaves on its own stops driving
    CPed* const driver = m_pTargetVehicle->m_pDriver;
    if (driver && !driver->IsPlayer() && ped == driver && m_bSensibleLeaveCar) {
        m_pTargetVehicle->m_autoPilot.SetCruiseSpeed(0);
        m_pTargetVehicle->m_autoPilot.ClearCarMission();
    }

    // The vehicle the player leaves from the driver's seat is held still while the player gets out
    if (ped->IsPlayer() && ped == m_pTargetVehicle->m_pDriver) {
        m_pTargetVehicle->SetStatus(STATUS_FORCED_STOP);
    }
}

// 0x63BAB0
void CTaskComplexLeaveCar::ComputeTargetDoorIfUnset(CPed* ped) {
    if (!m_nTargetDoor) {
        m_nTargetDoor = CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, ped);
    }
}

// 0x63BAE0
void CTaskComplexLeaveCar::CreateLineUpUtility(CPed*) {
    m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar{ CVector{}, 0, 0, m_nTargetDoor };
}

// NOTSA
void CTaskComplexLeaveCar::ReleaseDoorAndSeat() {
    m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
    m_nDoorFlagsSet = 0;
    m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
    m_nNumGettingInSet = 0;
}

// 0x646CF0
void CTaskComplexLeaveCar::GetDoorUse(const CVehicle* veh, int32 door, bool* isGettingIn, bool* isGettingOut) {
    // The bit each door owns in the vehicle's getting in / getting out flags
    uint8 flag;
    switch (door) {
    case TARGET_DOOR_FRONT_RIGHT: flag = 4; break;
    case TARGET_DOOR_REAR_RIGHT:  flag = 8; break;
    case TARGET_DOOR_DRIVER:      flag = 1; break;
    case TARGET_DOOR_REAR_LEFT:   flag = 2; break;
    default:                      return;
    }

    if (veh->m_nGettingInFlags & flag) {
        *isGettingIn = true;
    }
    if (veh->m_nGettingOutFlags & flag) {
        *isGettingOut = true;
    }
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

        ReleaseDoorAndSeat();
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
            ReleaseDoorAndSeat();
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
    // A null return ends this task; the exe asks CreateSubTask for TASK_FINISHED, which also gives null
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PAUSE:
    case TASK_SIMPLE_DIE:
        return nullptr;

    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        // A ped that is still marked as inside the vehicle has to be put out of it first
        return ped->bInVehicle ? CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped) : nullptr;

    case TASK_COMPLEX_LEAVE_BOAT:
        return m_bDie ? CreateSubTask(TASK_SIMPLE_DIE, ped) : nullptr;

    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE:
        m_nDieAnimID = ANIM_ID_KO_SHOT_FRONT_0;
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);

    case TASK_SIMPLE_CAR_DRIVE_TIMED: {
        if (!ped->bInVehicle) {
            return nullptr;
        }

        // The ped says it is in a vehicle but sits in none of its seats
        if (!ped->m_pVehicle->IsPassenger(ped) && !ped->m_pVehicle->IsDriver(ped)) {
            ped->bInVehicle = false;
            return nullptr;
        }

        ComputeTargetDoorIfUnset(ped);

        bool isGettingIn  = false;
        bool isGettingOut = false;
        GetDoorUse(m_pTargetVehicle, m_nTargetDoor, &isGettingIn, &isGettingOut);

        // Unless the ped has to get out, it waits for someone leaving through the same door and gives up if someone enters
        if (!m_bForceGetOut) {
            if (isGettingOut) {
                return CreateSubTask(TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE, ped);
            }
            if (isGettingIn) {
                return nullptr;
            }
        }
        return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
    }

    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE:
        return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);

    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: {
        // Too fast to step out, so the ped jumps
        if (!m_pTargetVehicle->CanPedStepOutCar(false)) {
            ReserveDoorAndSeat(ped);
            ped->SetPedState(PEDSTATE_NONE);
            return CreateSubTask(TASK_SIMPLE_CAR_JUMP_OUT, ped);
        }

        // No room to get out through this door and none left to try: the ped is thrown out
        const auto ForcePedOut = [&] {
            ReserveDoorAndSeat(ped);
            ped->SetPedState(PEDSTATE_NONE);
            CreateLineUpUtility(ped);
            return CreateSubTask(TASK_SIMPLE_CAR_FORCE_PED_OUT, ped);
        };

        auto* const veh = m_pTargetVehicle;
        if (!CCarEnterExit::IsRoomForPedToLeaveCar(veh, m_nTargetDoor, nullptr)) {
            // The door on the other side of the vehicle that shares its row (or seat on a bike) is the only alternative,
            // and only when nobody sits at it or is about to
            const bool tandem  = veh->IsBike() || veh->m_pHandlingData->m_bTandemSeats;
            int32      altDoor = 0;
            switch (m_nTargetDoor) {
            case TARGET_DOOR_FRONT_RIGHT:
                if (veh->m_pDriver || (veh->m_nGettingInFlags & 1)) {
                    return ForcePedOut();
                }
                altDoor = TARGET_DOOR_DRIVER;
                break;
            case TARGET_DOOR_REAR_RIGHT:
                if (veh->m_apPassengers[1] || (veh->m_nGettingInFlags & 2)) {
                    return ForcePedOut();
                }
                altDoor = TARGET_DOOR_REAR_LEFT;
                break;
            case TARGET_DOOR_DRIVER:
                if (!tandem && (veh->m_apPassengers[0] || (veh->m_nGettingInFlags & 4))) {
                    return ForcePedOut();
                }
                altDoor = TARGET_DOOR_FRONT_RIGHT;
                break;
            case TARGET_DOOR_REAR_LEFT:
                if (!tandem && (veh->m_apPassengers[2] || (veh->m_nGettingInFlags & 8))) {
                    return ForcePedOut();
                }
                altDoor = TARGET_DOOR_REAR_RIGHT;
                break;
            }

            if (!CCarEnterExit::IsRoomForPedToLeaveCar(veh, altDoor, nullptr)) {
                return ForcePedOut();
            }

            // Models flagged for the door check need the door's frame to exist
            if (veh->m_pHandlingData->m_bForceDoorCheck && veh->IsAutomobile() && !veh->AsAutomobile()->m_aCarNodes[altDoor]) {
                return ForcePedOut();
            }
            m_nTargetDoor = altDoor;
        }

        ReserveDoorAndSeat(ped);
        ped->SetPedState(PEDSTATE_NONE);
        if (m_bDie) {
            return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
        }
        CreateLineUpUtility(ped);
        return CreateSubTask(TASK_SIMPLE_CAR_GET_OUT, ped);
    }

    case TASK_SIMPLE_CAR_GET_OUT: {
        m_pTaskUtilityLineUpPedWithCar->m_nDoorOpenPosType = 2;

        // A ped forced out of an unlocked vehicle does not stop to close the door
        if (!m_bForceGetOut || m_pTargetVehicle->m_nDoorLock != CARLOCK_UNLOCKED) {
            if (CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)) {
                return CreateSubTask(TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE, ped);
            }
        }

        // No close door task follows, so the door the ped used is left open
        if (m_pTargetVehicle->IsAutomobile() && CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)) {
            m_pTargetVehicle->AsAutomobile()->m_damageManager.SetDoorOpen_Component((tComponent)m_nTargetDoor);
        }
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }

    case TASK_SIMPLE_CAR_JUMP_OUT: {
        if (m_bDie) {
            m_nDieAnimID         = ANIM_ID_KO_SHOT_FRONT_0;
            m_fDieAnimBlendDelta = 1000.0f;
            m_fDieAnimSpeed      = 0.5f;
            return CreateSubTask(TASK_SIMPLE_DIE, ped);
        }

        // Nothing to jump out of, or no jump for a bike or a quad
        const auto* const veh = m_pTargetVehicle;
        if (!veh || veh->IsBike() || veh->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            return nullptr;
        }

        // The ped is already in the air or in water, so it is put on the ground at once
        if (m_bIsInAir || ped->bIsDrowning) {
            return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
        }

        // Landed: the door flags and seat are no longer needed while the ped gets up
        ReleaseDoorAndSeat();
        return CreateSubTask(TASK_COMPLEX_GET_UP_AND_STAND_STILL, ped);
    }

    case TASK_SIMPLE_CAR_FORCE_PED_OUT:
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);

    case TASK_SIMPLE_CAR_SET_PED_OUT:
        return m_bDie ? CreateSubTask(TASK_SIMPLE_DIE, ped) : nullptr;

    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT:
        m_nDieAnimID         = ANIM_ID_FLOOR_HIT;
        m_fDieAnimBlendDelta = 1000.0f;
        m_fDieAnimSpeed      = 0.5f;

        // The police lock of a vehicle ends when its driver is dragged out
        if (m_pTargetVehicle && m_pTargetVehicle->m_nDoorLock == CARLOCK_COP_CAR) {
            m_pTargetVehicle->m_nDoorLock = CARLOCK_UNLOCKED;
        }
        return CreateSubTask(TASK_SIMPLE_DIE, ped);

    default:
        return nullptr;
    }
}

// 0x641FC0
CTask* CTaskComplexLeaveCar::CreateFirstSubTask(CPed* ped) {
    // A player that is being arrested does not get out of the vehicle on its own
    if ((ped->m_nPedState == PEDSTATE_ARRESTED || ped->bIsBeingArrested) && ped->IsPlayer()) {
        return new CTaskSimplePause{ -1 };
    }

    // The exe prints "CTaskComplexLeaveCar - ped not in car" here, which has no visible effect
    if (!ped->bInVehicle) {
        return new CTaskSimplePause{ -1 };
    }

    if (m_pTargetVehicle->m_pDriver == ped) {
        // The rest of the group is told that its leader left the vehicle as the driver
        if (auto* const group = CPedGroups::GetPedsGroup(ped); group && group->GetMembership().IsLeader(ped)) {
            CEventGroupEvent groupEvent{ ped, new CEventLeaderExitedCarAsDriver{} };
            group->GetIntelligence().AddEvent(&groupEvent);
        }

        if (!ped->IsPlayer()) {
            ped->SetRadioStation();
        } else if (auto* const veh = ped->m_pVehicle) {
            veh->m_vehicleAudio.PlayerAboutToExitVehicleAsDriver();
        }
    }

    if (m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        return new CTaskComplexLeaveBoat{ m_pTargetVehicle, 0 };
    }
    return new CTaskSimpleCarDriveTimed{ m_pTargetVehicle, m_nDelayTime };
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
