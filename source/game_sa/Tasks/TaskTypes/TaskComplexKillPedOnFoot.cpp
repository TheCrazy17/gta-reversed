#include "StdInc.h"

#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexKillPedOnFootMelee.h"
#include "TaskComplexKillPedOnFootArmed.h"
#include "TaskComplexDestroyCar.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleLeaveGroup.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "CarEnterExit.h"
#include "CullZones.h"
#include "CopPed.h"
#include "EventAreaCodes.h"
#include "Wanted.h"

void CTaskComplexKillPedOnFoot::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillPedOnFoot, 0x86D894, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x620E30);
    RH_ScopedInstall(Destructor, 0x620EF0);
    RH_ScopedVMTInstall(Clone, 0x623370);
    RH_ScopedVMTInstall(MakeAbortable, 0x625E40);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x62B150);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x62B490);
    RH_ScopedVMTInstall(ControlSubTask, 0x626260);
    RH_ScopedInstall(CreateSubTask, 0x625E70);
}

namespace {
// The exe function at 0x65E9A0, the attack counterpart of CTaskComplexGangLeader::DoGangAbuseSpeech
void DoGangAttackSpeech(CPed* ped, CPed* target) {
    if (!ped || !target || !ped->IsGangster()) {
        return;
    }
    if (!target->IsGangster() && target != FindPlayerPed(0)) {
        return;
    }
    switch (target->m_nPedType) {
    case PED_TYPE_GANG1: ped->Say(CTX_GLOBAL_ATTACK_GANG_BALLAS); break;
    case PED_TYPE_GANG3: ped->Say(CTX_GLOBAL_ATTACK_GANG_LSV);    break;
    case PED_TYPE_GANG8: ped->Say(CTX_GLOBAL_ATTACK_GANG_VLA);    break;
    default:                                                       break;
    }
}

// Gang members and cops pull a pistol when the player stands still next to them, the same check runs when the task starts and after the ped left its car
void ArmPedAgainstStoppedPlayer(CPed* ped, CPed* target) {
    if (!target->IsPlayer()) {
        return;
    }
    if (!CCullZones::NoPolice()) {
        const auto* const standingOn = target->m_standingOnEntity;
        if (!standingOn || standingOn == ped->m_standingOnEntity) {
            return;
        }
        if (!((target->GetPosition() - ped->GetPosition()).Magnitude() < 5.0f)) {
            return;
        }
    }
    if (!target->GetPlayerData()->m_bStoppedMoving) { // Bit 0 of the player flags
        return;
    }
    if (!ped->IsGangster() && ped->m_nPedType != PED_TYPE_COP) {
        return;
    }
    if (ped->IsCreatedByMission()) {
        return;
    }
    ped->GiveWeapon(WEAPON_PISTOL, 1000, true);
    ped->SetCurrentWeapon(WEAPON_PISTOL);
    ped->SetMoveState(PEDMOVE_STILL);
}

// Whether the target can be dragged out of its vehicle (as opposed to destroying the car)
bool IsRoomToDragTargetOut(CPed* target, CVehicle* vehicle) {
    return CCarEnterExit::IsRoomForPedToLeaveCar(vehicle, CCarEnterExit::ComputeTargetDoorToExit(vehicle, target), nullptr);
}
} // namespace

// 0x620E30
CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(
    CPed* target,
    int32 time,
    int32 pedFlags,
    int32 delay,
    int32 chance,
    uint8 nCompetence,
    bool bWaitForPlayerToBeSafe,
    bool bWaitingForPlayerToBeSafe
) :
    m_bWaitForPlayerToBeSafe{ bWaitForPlayerToBeSafe },
    m_bWaitingForPlayerToBeSafe{ bWaitingForPlayerToBeSafe },
    m_target{ target },
    m_pedFlags{ pedFlags },
    m_actionDelay{ delay },
    m_actionChance{ chance },
    m_nCompetence{ nCompetence },
    m_time{ time },
    m_startTime{ CTimer::GetTimeInMS() }
{
    CEntity::SafeRegisterRef(m_target);
}

// 0x620EF0
CTaskComplexKillPedOnFoot::~CTaskComplexKillPedOnFoot() {
    CEntity::SafeCleanUpRef(m_target);
}

CTaskComplexKillPedOnFoot* CTaskComplexKillPedOnFoot::Constructor(CPed* target, int32 time, int32 pedFlags, int32 delay, int32 chance, int8 a7) {
    this->CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(target, time, pedFlags, delay, chance, a7);
    return this;
}

// 0x625E40
bool CTaskComplexKillPedOnFoot::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    // The task sets this flag while it runs (bit 22 of the third flag dword, 0x474); leaving the task clears it
    ped->bDontAcceptIKLookAts = false;
    return m_pSubTask ? m_pSubTask->MakeAbortable(ped, priority, event) : true;
}

// 0x62B150
CTask* CTaskComplexKillPedOnFoot::CreateNextSubTask(CPed* ped) {
    if (!m_target) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_NONE:
    case TASK_SIMPLE_STAND_STILL:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_SIMPLE_PAUSE: {
        if (ped->m_nPedType == PED_TYPE_COP && static_cast<CCopPed*>(ped)->m_bDontPursuit) {
            return CreateSubTask(FindPlayerWanted(-1)->m_ChanceOnRoadBlock > 0 ? TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED : TASK_SIMPLE_PAUSE, ped);
        }
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe && !FindPlayerWanted(-1)->m_bEverybodyBackOff) {
            m_bWaitingForPlayerToBeSafe = false;
            return CreateFirstSubTask(ped);
        }
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        if (static_cast<CTaskComplexDragPedFromCar*>(m_pSubTask)->GetQuitAfterDraggingPedOut()) {
            return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
        }
        return CreateSubTask(ped->GetActiveWeapon().IsTypeMelee() ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: {
        if (ped->bInVehicle) {
            return CreateSubTask(TASK_SIMPLE_CAR_DRIVE_TIMED, ped);
        }
        ArmPedAgainstStoppedPlayer(ped, m_target);
        return CreateSubTask(ped->GetActiveWeapon().IsTypeMelee() ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED, ped);
    }
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE:
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED:
    case TASK_COMPLEX_DESTROY_CAR:
        return CreateSubTask(TASK_FINISHED, ped);
    default: // Includes TASK_COMPLEX_SIGNAL_AT_PED
        return nullptr;
    }
}

// 0x62B490
CTask* CTaskComplexKillPedOnFoot::CreateFirstSubTask(CPed* ped) {
    m_bNewTarget             = false;
    m_bRoomToDragPedOutOfCar = true;

    if (!m_target) {
        ped->bDontAcceptIKLookAts = false;
        return nullptr;
    }

    if (m_target->m_fHealth <= 0.0f) {
        m_bTargetKilled = true;
    }

    eTaskType taskId{};
    if (ped->m_pVehicle && ped->bInVehicle) {
        taskId = TASK_COMPLEX_LEAVE_CAR;
    } else {
        ArmPedAgainstStoppedPlayer(ped, m_target);

        taskId = ped->GetActiveWeapon().IsTypeMelee() ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;

        // A target that sits in a vehicle is dragged out of it when that is possible, otherwise the vehicle is destroyed
        auto* const vehicle = m_target->m_pVehicle;
        if (m_target->bInVehicle && vehicle && (vehicle->IsDriver(m_target) || vehicle->IsPassenger(m_target))) {
            m_bRoomToDragPedOutOfCar = true;

            const auto subType = vehicle->m_nVehicleSubType;
            if (subType == VEHICLE_TYPE_HELI
                || subType == VEHICLE_TYPE_PLANE
                || ped->bStayInSamePlace
                || vehicle->m_vecMoveSpeed.Magnitude2D() > 0.1f
                || !vehicle->CanPedOpenLocks(ped)
                || (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && !ped->GetActiveWeapon().IsTypeMelee())
                || vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT
            ) {
                taskId = TASK_COMPLEX_DESTROY_CAR;
            } else {
                m_bRoomToDragPedOutOfCar = IsRoomToDragTargetOut(m_target, vehicle);
                if (!m_bRoomToDragPedOutOfCar || m_target->bDontDragMeOutCar) {
                    taskId = TASK_COMPLEX_DESTROY_CAR;
                } else {
                    taskId = TASK_COMPLEX_DRAG_PED_FROM_CAR;
                }
            }
        }
    }

    const auto task = CreateSubTask(taskId, ped);
    ped->DropEntityThatThisPedIsHolding(true);
    return task;
}

// 0x626260
CTask* CTaskComplexKillPedOnFoot::ControlSubTask(CPed* ped) {
    ped->bDontAcceptIKLookAts = true;

    // TASK_NONE means the current sub task is kept
    eTaskType newTaskId = TASK_NONE;

    // The target of the sub task type switch below, decided from what the sub task is doing
    const auto DecideBySubTask = [&]() -> eTaskType {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
            static_cast<CTaskComplexKillPedOnFootMelee*>(m_pSubTask)->m_bShotFiredByPlayerFlag = m_bShotFiredByPlayerFlag;

            if (!ped->GetActiveWeapon().IsTypeMelee()) {
                return TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
            }

            auto* const vehicle = m_target->m_pVehicle;
            if (!m_target->bInVehicle || !vehicle) {
                return TASK_NONE;
            }
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI) {
                return TASK_COMPLEX_DESTROY_CAR;
            }
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT || ped->bStayInSamePlace || !vehicle->CanPedOpenLocks(ped)) {
                return TASK_NONE;
            }
            m_bRoomToDragPedOutOfCar = IsRoomToDragTargetOut(m_target, vehicle);
            return m_bRoomToDragPedOutOfCar ? TASK_COMPLEX_DRAG_PED_FROM_CAR : TASK_COMPLEX_DESTROY_CAR;
        }
        case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
            static_cast<CTaskComplexKillPedOnFootArmed*>(m_pSubTask)->m_bShotFiredByPlayer = m_bShotFiredByPlayerFlag;

            eTaskType result = TASK_NONE;
            if (ped->GetActiveWeapon().IsTypeMelee() && !ped->IsPlayer()) {
                result = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
            } else if (ped->GetActiveWeapon().m_TotalAmmo == 0 && !ped->IsPlayer()) {
                // Out of ammo, so look for a weapon that still has some and fall back to fists
                int32 slot = 0;
                while (slot < (int32)NUM_WEAPON_SLOTS && (int32)ped->GetWeaponInSlot((size_t)slot).m_TotalAmmo <= 0) {
                    slot++;
                }
                if (slot < (int32)NUM_WEAPON_SLOTS) {
                    ped->SetCurrentWeapon(slot);
                } else {
                    ped->SetCurrentWeapon(WEAPON_UNARMED);
                    result = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
                }
            }

            auto* const vehicle = m_target->m_pVehicle;
            if (!m_target->bInVehicle || !vehicle) {
                return result;
            }
            if (!vehicle->IsDriver(m_target) && !vehicle->IsPassenger(m_target)) {
                return result;
            }
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI) {
                return TASK_COMPLEX_DESTROY_CAR;
            }
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE
                || vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT
                || ped->bStayInSamePlace
                || vehicle->m_vecMoveSpeed.Magnitude2D() > 0.1f
                || !vehicle->CanPedOpenLocks(ped)
            ) {
                return result;
            }
            m_bRoomToDragPedOutOfCar = IsRoomToDragTargetOut(m_target, vehicle);
            return m_bRoomToDragPedOutOfCar ? TASK_COMPLEX_DRAG_PED_FROM_CAR : TASK_COMPLEX_DESTROY_CAR;
        }
        case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
            auto* vehicle = m_target->m_pVehicle;
            if (!m_target->bInVehicle || !vehicle) {
                return TASK_NONE;
            }
            if (!vehicle->IsDriver(m_target) && !vehicle->IsPassenger(m_target)) {
                return TASK_NONE;
            }
            if (vehicle->m_vecMoveSpeed.Magnitude2D() > 0.1f) {
                return TASK_COMPLEX_DESTROY_CAR;
            }
            if (!m_timer.IsOutOfTime()) {
                return TASK_NONE;
            }
            vehicle = m_target->m_pVehicle;
            if (!vehicle) {
                return TASK_NONE;
            }
            if (!vehicle->IsPassenger(m_target) && !vehicle->IsDriver(m_target)) {
                return TASK_NONE;
            }

            // Checked again every 2 seconds while the target is dragged out
            m_bRoomToDragPedOutOfCar = IsRoomToDragTargetOut(m_target, vehicle);
            m_timer.Start(2000);
            return m_bRoomToDragPedOutOfCar ? TASK_NONE : TASK_COMPLEX_DESTROY_CAR;
        }
        case TASK_COMPLEX_DESTROY_CAR: {
            if (!m_target->bInVehicle) {
                return ped->GetActiveWeapon().IsTypeMelee() ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
            }

            auto* const vehicle = m_target->m_pVehicle;
            if (vehicle && !CCarEnterExit::IsVehicleHealthy(vehicle)) {
                return TASK_COMPLEX_SIGNAL_AT_PED;
            }
            if (m_target->bDontDragMeOutCar) {
                return TASK_NONE;
            }
            if (!vehicle) { // The exe reads the vehicle here without checking it
                return TASK_NONE;
            }
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE
                || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI
                || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE
                || vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT
                || ped->bStayInSamePlace
                || vehicle->m_vecMoveSpeed.Magnitude2D() > 0.1f
                || !vehicle->CanPedOpenLocks(ped)
            ) {
                return TASK_NONE;
            }
            if (m_bRoomToDragPedOutOfCar) {
                return TASK_COMPLEX_DRAG_PED_FROM_CAR;
            }
            if (!m_timer.IsOutOfTime()) {
                return TASK_NONE;
            }

            m_bRoomToDragPedOutOfCar = IsRoomToDragTargetOut(m_target, vehicle);
            m_timer.Start(2000);
            return m_bRoomToDragPedOutOfCar ? TASK_COMPLEX_DRAG_PED_FROM_CAR : TASK_NONE;
        }
        default:
            return TASK_NONE;
        }
    };

    // Whether the shared tail below clears the shot flag; the early outs that skip the sub task type switch do not
    bool clearShotFlag = true;

    if (!m_target) {
        newTaskId     = TASK_FINISHED;
        clearShotFlag = false;
    } else {
        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_PAUSE) {
            if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe && !FindPlayerWanted(-1)->m_bEverybodyBackOff) {
                m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
            }
            return m_pSubTask;
        }

        if (m_time > 0 && CTimer::GetTimeInMS() > m_startTime + m_time) {
            if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
            }
            clearShotFlag = false;
        } else if (m_bNewTarget) {
            if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                return CreateFirstSubTask(ped);
            }
            clearShotFlag = false;
        } else {
            // Set when the sub task type switch must not run because the target is handled here
            bool handled = false;

            if (!m_bTargetKilled) {
                if (m_target->m_fHealth <= 0.0f) {
                    if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                        m_bTargetKilled = true;
                        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SIGNAL_AT_PED) {
                            return m_pSubTask;
                        }
                        if (ped->m_nPedType == PED_TYPE_COP && !m_target->IsPlayer()) {
                            ped->Say(CTX_GLOBAL_ARREST_CRIM);
                        }
                        if (ped->bSignalAfterKill) {
                            return CreateSubTask(TASK_COMPLEX_SIGNAL_AT_PED, ped);
                        }
                        return new CTaskSimpleLeaveGroup{};
                    }
                } else if (m_target->IsPlayer()) {
                    // The player is left alone while everybody backs off, or as a cop that does not pursue when there is no roadblock chance
                    const auto* const wanted = m_target->GetPlayerData()->m_pWanted;
                    if (wanted->m_bEverybodyBackOff
                        || (ped->m_nPedType == PED_TYPE_COP && static_cast<CCopPed*>(ped)->m_bDontPursuit && FindPlayerWanted(-1)->m_ChanceOnRoadBlock == 0)
                    ) {
                        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_PAUSE) {
                            newTaskId = TASK_SIMPLE_PAUSE;
                            if (m_bWaitForPlayerToBeSafe && m_target->GetPlayerData()->m_pWanted->m_bEverybodyBackOff) {
                                m_bWaitingForPlayerToBeSafe = true;
                            }
                            handled = true;
                        }
                    }
                }
            }

            if (!handled) {
                newTaskId = DecideBySubTask();
            }
        }
    }

    if (clearShotFlag) {
        m_bShotFiredByPlayerFlag = false;
    }

    // Peds that are in different interiors can not reach each other
    if (m_target) {
        const auto* const targetContact = m_target->m_pContactEntity;
        const auto* const pedContact    = ped->m_pContactEntity;
        if (targetContact && pedContact && targetContact->GetAreaCode() != pedContact->GetAreaCode()) {
            CEventAreaCodes event{ m_target };
            ped->GetEventGroup().Add(&event, false);
        }
    }

    if (newTaskId != TASK_NONE && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateSubTask(newTaskId, ped);
    }

    if (m_target) {
        DoGangAttackSpeech(ped, m_target);
    }
    return m_pSubTask;
}

CTask* CTaskComplexKillPedOnFoot::CreateSubTask(int32 taskId, CPed* ped) {
    switch (taskId) {
    case TASK_NONE:
        return new CTaskSimpleLeaveGroup{};
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
    case TASK_SIMPLE_PAUSE: {
        // The original runs a throwaway stand still task on the ped first
        CTaskSimpleStandStill{ 0, false, false, 8.0f }.ProcessPed(ped);
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe) {
            return new CTaskSimplePause{ 10000 };
        }
        return new CTaskSimplePause{ 2000 };
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        const auto task = new CTaskComplexDragPedFromCar{ m_target, 0 };
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, true };
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ ped->m_pVehicle, 2000 };
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
        const auto task = new CTaskComplexKillPedOnFootMelee{ m_target };
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
        const auto task = new CTaskComplexKillPedOnFootArmed{ m_target, (uint32)m_pedFlags, (uint32)m_actionDelay, (uint32)m_actionChance, (int8)m_nCompetence };
        task->m_aimImmediate = m_bAimImmediate;
        m_bAimImmediate = false;
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar{ m_target->m_pVehicle, 0, 0, 0 };
    case TASK_COMPLEX_SIGNAL_AT_PED: {
        // Gloat over the target unless it is the player or the player's group leader
        if (m_target == FindPlayerPed(0)) {
            ped->Say(CTX_GLOBAL_PLAYER_WASTED);
        } else {
            const auto group = CPedGroups::GetPedsGroup(m_target);
            if (!group || group->GetMembership().GetLeader() != FindPlayerPed(0)) {
                ped->Say(CTX_GLOBAL_ENEMY_GANG_WASTED);
            }
        }
        return new CTaskComplexSignalAtPed{ m_target, CGeneral::GetRandomNumberInRange(0, 1500), true };
    }
    case TASK_FINISHED:
        ped->bDontAcceptIKLookAts = false;
        return nullptr;
    default:
        return nullptr;
    }
}
