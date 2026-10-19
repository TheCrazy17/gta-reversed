#include "StdInc.h"

#include "TaskComplexCarSlowBeDraggedOutAndStandUp.h"
#include "TaskComplexCarSlowBeDraggedOut.h"
#include "TaskComplexGetUpAndStandStill.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleCarSetPedOut.h"
#include "Enums/eTargetDoor.h"
#include "Events/EventDeath.h"

void CTaskComplexCarSlowBeDraggedOutAndStandUp::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexCarSlowBeDraggedOutAndStandUp, 0x86EF80, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x648620);
    RH_ScopedInstall(Destructor, 0x648690);
    RH_ScopedInstall(CreateSubTask, 0x648710);
    RH_ScopedInstall(Clone, 0x64A190);
    RH_ScopedInstall(GetTaskType, 0x648680);
    RH_ScopedInstall(MakeAbortable, 0x6486F0);
    RH_ScopedInstall(CreateNextSubTask, 0x6488F0);
    RH_ScopedInstall(CreateFirstSubTask, 0x648A10);
    RH_ScopedInstall(ControlSubTask, 0x648B80);
}

// 0x648620
CTaskComplexCarSlowBeDraggedOutAndStandUp::CTaskComplexCarSlowBeDraggedOutAndStandUp(CVehicle* vehicle, int32 a3) : CTaskComplex() {
    m_Vehicle = vehicle;
    dword10 = a3;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x648690
CTaskComplexCarSlowBeDraggedOutAndStandUp::~CTaskComplexCarSlowBeDraggedOutAndStandUp() {
    CEntity::SafeCleanUpRef(m_Vehicle);
}

// Coaches and tanks can't drag the ped out, and neither can the bus unless it is the front right door.
// The ped just leaves those vehicles instead.
bool CTaskComplexCarSlowBeDraggedOutAndStandUp::LeavesInsteadOfBeingDragged(const CVehicle* vehicle) const {
    switch (vehicle->m_pHandlingData->GetAnimGroupId()) {
    case ANIM_GROUP_COACHCARANIMS:
    case ANIM_GROUP_TANKCARANIMS:
        return true;
    case ANIM_GROUP_BUSCARANIMS:
        return dword10 == TARGET_DOOR_FRONT_RIGHT;
    default:
        return false;
    }
}

// 0x648710
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_CAR_SET_PED_OUT:
        return new CTaskSimpleCarSetPedOut{ ped->m_pVehicle, static_cast<eTargetDoor>(dword10), true };
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        return new CTaskComplexGetUpAndStandStill{};
    case TASK_COMPLEX_LEAVE_CAR: {
        // The door depends on the vehicle, not on the task
        switch (m_Vehicle->m_pHandlingData->GetAnimGroupId()) {
        case ANIM_GROUP_COACHCARANIMS:
            return new CTaskComplexLeaveCar{ ped->m_pVehicle, TARGET_DOOR_FRONT_RIGHT, 0, false, true };
        case ANIM_GROUP_TANKCARANIMS:
            return new CTaskComplexLeaveCar{ ped->m_pVehicle, TARGET_DOOR_DRIVER, 0, false, true };
        case ANIM_GROUP_BUSCARANIMS:
            if (dword10 == TARGET_DOOR_FRONT_RIGHT) {
                return new CTaskComplexLeaveCar{ ped->m_pVehicle, TARGET_DOOR_FRONT_RIGHT, 0, false, true };
            }
            return nullptr;
        default:
            return nullptr;
        }
    }
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT:
        return new CTaskComplexCarSlowBeDraggedOut{ m_Vehicle, static_cast<eTargetDoor>(dword10), false };
    default:
        return nullptr;
    }
}

// 0x6486F0
bool CTaskComplexCarSlowBeDraggedOutAndStandUp::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return priority == ABORT_PRIORITY_IMMEDIATE && m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x6488F0
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateNextSubTask(CPed* ped) {
    // The original also asks whether the ped is the player after the stand up task, and drops the answer
    if (m_pSubTask->GetTaskType() != TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT) {
        return nullptr;
    }
    if (!ped->IsAlive() || ped->m_fHealth <= 0.0f) {
        ped->GetEventGroup().Add(CEventDeath{ false });
        return nullptr;
    }
    return new CTaskComplexGetUpAndStandStill{};
}

// 0x648A10
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateFirstSubTask(CPed* ped) {
    auto* const vehicle = ped->m_pVehicle;

    if (ped == FindPlayerPed(0)) {
        if (vehicle) {
            vehicle->m_vehicleAudio.PlayerAboutToExitVehicleAsDriver();
        }
    } else if (vehicle && vehicle->m_pDriver == ped) {
        ped->SetRadioStation();
    }

    if (!vehicle) {
        return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
    }
    if (LeavesInsteadOfBeingDragged(vehicle)) {
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    }

    // The ped shouts or hits the vehicle while being pulled out
    switch (vehicle->m_pHandlingData->GetAnimGroupId()) {
    case ANIM_GROUP_STDCARAMIMS:
    case ANIM_GROUP_VANCARANIMS:
    case ANIM_GROUP_RUSTPLANEANIMS:
    case ANIM_GROUP_KARTCARANIMS:
    case ANIM_GROUP_CONVCARANIMS:
    case ANIM_GROUP_STDTALLCARAMIMS:
    case ANIM_GROUP_HOVERCARANIMS:
    case ANIM_GROUP_STDCARUPRIGHT:
        if (dword10 == TARGET_DOOR_DRIVER) {
            ped->GetAE().AddAudioEvent(AE_PED_JACKED_CAR_PUNCH);
        } else if (dword10 == TARGET_DOOR_FRONT_RIGHT) {
            ped->GetAE().AddAudioEvent(AE_PED_JACKED_CAR_HEAD_BANG);
        }
        break;
    case ANIM_GROUP_LOWCARAMIMS:
        ped->GetAE().AddAudioEvent(AE_PED_JACKED_CAR_KICK);
        break;
    case ANIM_GROUP_DOZERCARANIMS:
        ped->GetAE().AddAudioEvent(AE_PED_JACKED_DOZER);
        break;
    default:
        break;
    }

    return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
}

// 0x648B80
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::ControlSubTask(CPed* ped) {
    return m_pSubTask;
}
