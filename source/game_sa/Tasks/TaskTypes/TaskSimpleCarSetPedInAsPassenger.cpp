#include "StdInc.h"

#include "TaskSimpleCarSetPedInAsPassenger.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarDrive.h"
#include "CarEnterExit.h"
#include "Crime.h"
#include "Entity/Ped/PlayerPed.h"

void CTaskSimpleCarSetPedInAsPassenger::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarSetPedInAsPassenger, 0x86EE04, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(ProcessPed, 0x64B5D0);
    RH_ScopedVMTInstall(GetTaskType, 0x647060);
}

// OG constructor was at 0x646FE0
CTaskSimpleCarSetPedInAsPassenger::CTaskSimpleCarSetPedInAsPassenger(CVehicle* targetVehicle, eTargetDoor nTargetDoor, bool warpingInToCar, CTaskUtilityLineUpPedWithCar* utility) :
    m_nTargetDoor{ nTargetDoor },
    m_pTargetVehicle{ targetVehicle },
    m_pUtility{ utility },
    m_bWarpingInToCar{warpingInToCar}
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// For 0x649D90
CTaskSimpleCarSetPedInAsPassenger::CTaskSimpleCarSetPedInAsPassenger(const CTaskSimpleCarSetPedInAsPassenger& o) :
    CTaskSimpleCarSetPedInAsPassenger{
        o.m_pTargetVehicle,
        o.m_nTargetDoor,
        o.m_bWarpingInToCar,
        o.m_pUtility
    }
{
    m_nNumGettingInToClear = o.m_nNumGettingInToClear;
}

// 0x647080
CTaskSimpleCarSetPedInAsPassenger::~CTaskSimpleCarSetPedInAsPassenger() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x64B5D0
bool CTaskSimpleCarSetPedInAsPassenger::ProcessPed(CPed* ped) {
    auto* const veh = m_pTargetVehicle;

    CEntity::SafeCleanUpRef(ped->m_pVehicle);
    CEntity::SetEntityReference(ped->m_pVehicle, veh);

    ped->m_fAimingRotation = ped->m_fCurrentRotation;
    ped->bInVehicle = true;

    if (ped->IsPlayer()) {
        ped->GetPlayerData()->m_bPlayersGangActive = true;
        ped->AsPlayer()->ClearAdrenaline();
    }

    if (ped->IsPlayer() && !veh->vehicleFlags.bHasBeenOwnedByPlayer) {
        veh->vehicleFlags.bHasBeenOwnedByPlayer = true;
        CCrime::ReportCrime(CRIME_CAR_STEAL, veh, FindPlayerPed());
    }

    // A bus has no numbered passenger seats
    int32 seat = -1;
    if (!veh->vehicleFlags.bIsBus) {
        seat = CCarEnterExit::ComputePassengerIndexFromCarDoor(veh, m_nTargetDoor);

        // Whoever sits in that seat is thrown out, unless they are already on their way out
        if (seat != -1) {
            if (auto* const occupant = veh->m_apPassengers[seat]; occupant && occupant != ped) {
                auto& taskMgr = occupant->GetIntelligence()->GetTaskManager();
                if (!taskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
                    && !taskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
                    && !taskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_DIE)
                    && !taskMgr.FindActiveTaskByType(TASK_SIMPLE_BIKE_JACKED)
                ) {
                    const auto door = (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(veh, occupant);
                    CTaskSimpleCarSetPedOut task{ veh, door, true, true };
                    task.ProcessPed(occupant);
                }
            }
        }
    }

    if (m_bWarpingInToCar) {
        ped->GetPosition() = veh->GetPosition();
    }

    if (seat == -1) {
        veh->AddPassenger(ped);
    } else {
        veh->AddPassenger(ped, (uint8)seat);
    }

    ped->UpdateStatEnteringVehicle();
    ped->SetMoveState(PEDMOVE_NONE);
    ped->SetMoveAnim();
    ped->m_bUsesCollision = false;

    if (veh->m_nAlarmState == (uint16)-1) {
        veh->m_nAlarmState = 15000;
    }

    ped->SetPedState(PEDSTATE_DRIVING);

    if (m_nDoorFlagsToClear) {
        veh->ClearGettingInFlags(m_nDoorFlagsToClear);
    }
    if (m_nNumGettingInToClear) {
        veh->m_nNumGettingIn -= m_nNumGettingInToClear;
    }

    ped->RemoveWeaponWhenEnteringVehicle(0);

    // Unlike the driver, a passenger of a bus is never rendered inside it, whatever the model
    ped->bRenderPedInCar = !veh->vehicleFlags.bIsBus;

    CCarEnterExit::RemoveGetInAnims(ped);
    CCarEnterExit::AddInCarAnim(veh, ped, false);

    if (!m_bWarpingInToCar) {
        m_pUtility->ProcessPed(ped, veh, nullptr);
    }

    ped->GetTaskManager().SetTask(new CTaskSimpleCarDrive{ veh, m_pUtility, false }, TASK_PRIMARY_DEFAULT, false);

    return true;
}
