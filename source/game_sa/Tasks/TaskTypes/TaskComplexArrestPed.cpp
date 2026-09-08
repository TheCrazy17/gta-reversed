#include "StdInc.h"

#include "extensions/utility.hpp"

#include "TaskComplexArrestPed.h"
#include "TaskComplexFallAndGetUp.h"
#include "TaskSimpleWaitUntilPedIsOutCar.h"
#include "TaskSimpleArrestPed.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexDestroyCar.h"
// #include "TaskComplexSeekEntity.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexOpenDriverDoor.h"
#include "TaskComplexOpenPassengerDoor.h"
#include "SeekEntity/TaskComplexSeekEntityStandard.h"

#include "eTargetDoor.h"

void CTaskComplexArrestPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexArrestPed, 0x8709A8, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x68B990);
    RH_ScopedInstall(Destructor, 0x68BA00);
    RH_ScopedVMTInstall(MakeAbortable, 0x68BA60);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x690220);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6907A0);
    RH_ScopedVMTInstall(ControlSubTask, 0x68D350);
    RH_ScopedInstall(CreateSubTask, 0x68CF80);
}

// 0x68B990
CTaskComplexArrestPed::CTaskComplexArrestPed(CPed* ped) : CTaskComplex() {
    m_PedToArrest = ped;
    m_Vehicle = nullptr;
    CEntity::SafeRegisterRef(m_PedToArrest);
}

// 0x68BA00
CTaskComplexArrestPed::~CTaskComplexArrestPed() {
    CEntity::SafeCleanUpRef(m_PedToArrest);
}

// 0x68BA60


bool CTaskComplexArrestPed::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x690220
CTask* CTaskComplexArrestPed::CreateNextSubTask(CPed* ped) {
    if (!m_PedToArrest) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    // If the target has already been cuffed, only a seek-then-arrest chain matters - override
    // whatever the sub-task's own logic would otherwise decide.
    if (m_PedToArrest->bIsBeingArrested && m_pSubTask->GetTaskType() != TASK_SIMPLE_ARREST_PED) {
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_SEEK_ENTITY) {
            return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
        }
        if (!static_cast<CTaskComplexSeekEntityStandard*>(m_pSubTask)->IsAchievedSeekEntity()) {
            return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
        }
        return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
    }

    // Shared by the SEEK_ENTITY/DRAG_PED_FROM_CAR cases below: if the target is falling and close
    // enough to `ped`, tell the fall task to get up soon and switch straight to arresting them.
    const auto TryFinishViaFallAndGetUp = [&]() -> CTask* {
        const auto fallTask = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP));
        if (!fallTask || !fallTask->IsFalling()) {
            return nullptr;
        }
        const auto dir = ped->GetPosition() - m_PedToArrest->GetPosition();
        if (std::abs(dir.z) > 2.f || dir.SquaredMagnitude() > sq(3.0f)) {
            return nullptr;
        }
        fallTask->SetDownTime(100'000);
        return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
    };

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_DRAG_PED_FROM_CAR:
        if (!static_cast<CTaskComplexEnterCar*>(m_pSubTask)->GetQuitAfterDraggingPedOut()) {
            if (const auto result = TryFinishViaFallAndGetUp()) {
                return result;
            }
        }
        break;

    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR:
    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR: {
        auto* const enterCarTask = static_cast<CTaskComplexEnterCar*>(m_pSubTask);

        if (enterCarTask->GetQuitAfterOpeningDoor() && m_PedToArrest->m_pVehicle && !m_PedToArrest->m_pVehicle->CanPedOpenLocks(ped)) {
            m_Vehicle = m_PedToArrest->m_pVehicle;
        }

        if (!m_PedToArrest->IsAlive()) {
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }

        // NOTSA: `m_PedToArrest`'s flags dword at +0x46C (bit 0x100) hasn't been identified/named
        // elsewhere in this codebase yet - verified directly via raw disassembly at 0x690341/0x6903d5.
        if ((*reinterpret_cast<uint32*>(reinterpret_cast<char*>(m_PedToArrest) + 0x46C) & 0x100) && !enterCarTask->GetQuitAfterOpeningDoor()) {
            if (m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
                return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
            }
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }
        break;
    }

    case TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR:
        break;

    case TASK_COMPLEX_SEEK_ENTITY:
        if (static_cast<CTaskComplexSeekEntityStandard*>(m_pSubTask)->IsAchievedSeekEntity()) {
            if (const auto result = TryFinishViaFallAndGetUp()) {
                return result;
            }
        }
        break;

    case TASK_COMPLEX_DESTROY_CAR:
        return CreateSubTask(TASK_FINISHED, ped);

    case TASK_SIMPLE_ARREST_PED:
        return CreateSubTask(TASK_FINISHED, ped);

    case TASK_COMPLEX_KILL_PED_ON_FOOT: {
        // NOTSA: `m_PedToArrest`+0x540 (float) hasn't been identified/named elsewhere in this
        // codebase yet - verified directly via raw disassembly at 0x6904b0.
        if (!(*reinterpret_cast<float*>(reinterpret_cast<char*>(m_PedToArrest) + 0x540) > 0.f)) {
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }

        const auto fallTask = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP));
        if (fallTask && fallTask->IsFalling()) {
            const auto dir = ped->GetPosition() - m_PedToArrest->GetPosition();
            if (std::abs(dir.z) > 2.f || dir.SquaredMagnitude() > sq(3.0f)) {
                return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
            }
            fallTask->SetDownTime(100'000);
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }

        // NOTSA: `m_PedToArrest`+0x598 (int32, compared against `ePedState::PEDSTATE_SEEK_POSITION`
        // = 6) and +0x480 (a pointer chain, +0x18 byte checked on the final object) haven't been
        // identified/named elsewhere in this codebase yet - verified directly via raw disassembly
        // at 0x690597/0x6905c7.
        if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(m_PedToArrest) + 0x598) != PEDSTATE_SEEK_POSITION && ped->IsPlayer()) {
            const auto p = *reinterpret_cast<void***>(reinterpret_cast<char*>(m_PedToArrest) + 0x480);
            const auto obj = p ? *p : nullptr;
            if (obj && *(reinterpret_cast<char*>(obj) + 0x18) != 0) {
                return CreateSubTask(TASK_FINISHED, ped);
            }
        }
        break;
    }

    default:
        return nullptr;
    }

    return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
}

// NOTSA
void MakeSurePedHasWeaponInHand(CPed* ped) {
    // Make sure ped has an actual weapon in their hand
    if (!ped->GetActiveWeapon().IsTypeMelee())
        return;

    if (ped->DoWeHaveWeaponAvailable(WEAPON_SHOTGUN)) { // Use shotgun (if available)
        ped->SetCurrentWeapon(WEAPON_SHOTGUN);
        return;
    }

    // Otherwise a pistol
    if (!ped->DoWeHaveWeaponAvailable(WEAPON_PISTOL)) { // Make sure they have one
        ped->GiveWeapon(WEAPON_PISTOL, 10, false);
    }
    ped->SetCurrentWeapon(WEAPON_PISTOL);
}

// 0x6907A0


CTask* CTaskComplexArrestPed::CreateFirstSubTask(CPed* ped) {
    if (!m_PedToArrest) {
        return nullptr;
    }

    m_bSubTaskNeedsToBeCreated = false;

    if (!m_PedToArrest->bInVehicle) {
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }

    if (m_PedToArrest->m_pVehicle->IsBike() || m_PedToArrest->m_pVehicle->IsSubQuad()) { // Just drag ped from a bike/quad
        return CreateSubTask(TASK_COMPLEX_DRAG_PED_FROM_CAR, ped);
    }

    if (m_PedToArrest->m_pVehicle->IsSubBoat()) { // If they're in a boat, just destroy it
        MakeSurePedHasWeaponInHand(ped);
        return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
    } else {
        if (m_PedToArrest->m_pVehicle->IsUpsideDown() || m_PedToArrest->m_pVehicle->IsOnItsSide()) {
            return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
        }
        return CreateSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR, ped);
    }
}

// 0x68D350

// 0x0
CTask* CTaskComplexArrestPed::ControlSubTask(CPed* ped) {
    // Automatically make ped say something on function return
    const notsa::ScopeGuard Have_A_Nice_Day_Sir{
        [this, ped] {
            if (m_PedToArrest && m_PedToArrest->IsPlayer()) {
                if (FindPlayerWanted()->m_NumCopsInPursuit == 1) {
                    ped->Say(CTX_GLOBAL_SOLO);
                }
            }
        }
    };

    // Tries to abort current sub-task and replace it with `taskType`.
    const auto TryReplaceSubTask = [this, ped](auto taskType) {
        // Inverted `if` and got rid of `taskType == TASK_NONE` (in which case `m_pSubTask` was returned always)
        if (m_pSubTask->MakeAbortable(ped)) {
            return CreateSubTask(taskType, ped);
        } else {
            return m_pSubTask;
        }
    };

    const auto DoDestroyCarTask = [&] {
        MakeSurePedHasWeaponInHand(ped);
        return TryReplaceSubTask(TASK_COMPLEX_DESTROY_CAR);
    };

    // 0x68D370, 0x68D37F
    if (!m_PedToArrest || m_PedToArrest->m_fHealth <= 0.f) {
        return TryReplaceSubTask(TASK_FINISHED);
    }

    // 0x68D39F
    if (m_bSubTaskNeedsToBeCreated) {
        if (m_pSubTask->MakeAbortable(ped)) {
            return m_pSubTask->AsComplex()->CreateFirstSubTask(ped);
        }
        return m_pSubTask;
    }

    // 0x68D3CB
    if (m_PedToArrest->bIsBeingArrested) {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_SIMPLE_ARREST_PED:
        case TASK_COMPLEX_SEEK_ENTITY:
            break;
        default:
            return TryReplaceSubTask(TASK_COMPLEX_SEEK_ENTITY);
        }
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: // 0x68D49D
    case TASK_COMPLEX_DESTROY_CAR: { // 0x68D5F6
        if (!m_PedToArrest->bInVehicle) { // If not in vehicle anymore, try to kill them
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }
        break;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT: { // 0x68D626
        // See if ped is falling, and is close enough
        if (const auto task = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP)); task && task->IsFalling()) {
            const auto dir = ped->GetPosition() - m_PedToArrest->GetPosition();
            if (std::abs(dir.z) <= 2.f && dir.SquaredMagnitude() <= sq(3.0f)) {
                task->SetDownTime(100'000);
                return TryReplaceSubTask(TASK_SIMPLE_ARREST_PED);
            }

            // Falling, but not close enough
            return TryReplaceSubTask(TASK_COMPLEX_SEEK_ENTITY);
        }

        if (!m_PedToArrest->bInVehicle || !m_PedToArrest->m_pVehicle) {
            break;
        }

        // Ped has gotten into a vehicle, we need a different task!

        if (   m_PedToArrest->m_pVehicle->IsBoat()
            || m_PedToArrest->m_pVehicle->IsSubPlane()
            || m_PedToArrest->m_pVehicle->IsSubHeli()
        ) {
            return DoDestroyCarTask();
        }

        if (!ped->GetActiveWeapon().IsTypeMelee()) {
            if (!FindPlayerWanted()->IsClosestCop(ped->AsCop(), 2)) {
                return DoDestroyCarTask();
            }
        }

        if (m_PedToArrest->m_pVehicle != m_Vehicle) {
            if (m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
                if (m_PedToArrest->m_pVehicle->IsBike() || m_PedToArrest->m_pVehicle->IsSubQuad()) {
                    return TryReplaceSubTask(TASK_COMPLEX_DRAG_PED_FROM_CAR);
                }

                if (!m_PedToArrest->m_pVehicle->IsUpsideDown() && !m_PedToArrest->m_pVehicle->IsOnItsSide()) {
                    return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR);
                }

                return DoDestroyCarTask();
            }
        }

        break;
    }
    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR: { // 0x68D424
        // Maybe wait until ped gets out of the car..
        if (const auto task = m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR); task && m_PedToArrest->bInVehicle && ped->IsEntityInRange(m_PedToArrest, 5.f)) {
            return TryReplaceSubTask(TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR);
        }

        if (!m_PedToArrest->bInVehicle) {
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }

        // Ped can't open driver door, but we can open front right door?
        if (!m_PedToArrest->m_pVehicle->IsRoomForPedToLeaveCar(TARGET_DOOR_DRIVER, nullptr)) {
            if (m_PedToArrest->m_pVehicle->IsRoomForPedToLeaveCar(TARGET_DOOR_FRONT_RIGHT, nullptr)) {
                return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR);
            }
        }

        // Neither door has room to open yet - keep waiting with the current sub-task
        break;
    }
    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR: { // 0x68D510
        // Pretty much the copy of the above, with minor changes (See change 1,2)

        if (const auto task = m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR); task && m_PedToArrest->bInVehicle && ped->IsEntityInRange(m_PedToArrest, 5.f)) {
            return TryReplaceSubTask(TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR);
        }

        if (!m_PedToArrest->bInVehicle) {
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }

        // Ped can't open passenger door, but driver door can be opened by us? - Change 1
        if (!m_PedToArrest->m_pVehicle->IsRoomForPedToLeaveCar(TARGET_DOOR_FRONT_RIGHT, nullptr)) {
            if (m_PedToArrest->m_pVehicle->IsRoomForPedToLeaveCar(TARGET_DOOR_DRIVER, nullptr)) {
                return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR);
            }

            // Change 2
            return DoDestroyCarTask();
        }

        // Front right still has room to open - keep waiting with the current sub-task
        break;
    }
    }

    // Just continue with current sub-task
    return m_pSubTask;
}

// 0x68CF80
CTask* CTaskComplexArrestPed::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_ARREST_PED:
        if (m_PedToArrest->m_pVehicle) {
            if (m_PedToArrest->m_pVehicle->IsDriver(m_PedToArrest)) {
                m_PedToArrest->m_pVehicle->vehicleFlags.bIsHandbrakeOn = true;
                m_PedToArrest->m_pVehicle->SetStatus(STATUS_FORCED_STOP);
            }
        }
        return new CTaskSimpleArrestPed(m_PedToArrest);

    case TASK_COMPLEX_KILL_PED_ON_FOOT:
        return new CTaskComplexKillPedOnFoot(m_PedToArrest, -1, 0, 0, 0, 1);

    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar(m_PedToArrest->m_pVehicle, 0, 0, 0);

    case TASK_COMPLEX_SEEK_ENTITY: {
        float radius = m_PedToArrest->bIsBeingArrested ? 4.0f : 3.0f;
        return new CTaskComplexSeekEntityStandard(m_PedToArrest, 50'000, 1000, radius, 2.0f, 2.0f, true, true);
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR:
        return new CTaskComplexDragPedFromCar(m_PedToArrest, 100'000);

    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR:
        return new CTaskComplexOpenDriverDoor(m_PedToArrest->m_pVehicle);

    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR:
        return new CTaskComplexOpenPassengerDoor(m_PedToArrest->m_pVehicle, 8); // todo: magic number

    case TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR: {
        return new CTaskSimpleWaitUntilPedIsOutCar{m_PedToArrest, m_PedToArrest->GetPosition() - ped->GetPosition()};
    }
    default:
        return nullptr;
    }
}
