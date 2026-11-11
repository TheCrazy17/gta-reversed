#include "StdInc.h"

#include "TaskSimpleCarSetPedInAsDriver.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarDrive.h"
#include "CarEnterExit.h"
#include "PedGroups.h"
#include "Crime.h"
#include "ModelIndices.h"
#include "Events/EventGroupEvent.h"
#include "Events/EventLeaderEnteredCarAsDriver.h"
#include "Events/EventCopCarBeingStolen.h"
#include "Events/EventGlobalGroup.h"
#include "Entity/Ped/PlayerPed.h"

void CTaskSimpleCarSetPedInAsDriver::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarSetPedInAsDriver, 0x86EE28, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(ProcessPed, 0x64B950);
    RH_ScopedVMTInstall(GetTaskType, 0x647150);
}

// 0x6470E0
CTaskSimpleCarSetPedInAsDriver::CTaskSimpleCarSetPedInAsDriver(CVehicle* targetVehicle, CTaskUtilityLineUpPedWithCar* utility) : CTaskSimple() {
    m_bIsFinished = 0;
    m_pAnim = 0;
    m_pTargetVehicle = targetVehicle;
    m_pUtility = utility;
    m_bWarpingInToCar = 0;
    m_nDoorFlagsToClear = 0;
    m_nNumGettingInToClear = 0;
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

CTaskSimpleCarSetPedInAsDriver::CTaskSimpleCarSetPedInAsDriver(CVehicle* targetVehicle, bool warpingInToCar, CTaskUtilityLineUpPedWithCar* utility) : // NOTSA
    CTaskSimpleCarSetPedInAsDriver{ targetVehicle, utility }
{
    m_bWarpingInToCar = warpingInToCar;
}

CTaskSimpleCarSetPedInAsDriver::~CTaskSimpleCarSetPedInAsDriver() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x649E00
CTask* CTaskSimpleCarSetPedInAsDriver::Clone() const {
    auto task = new CTaskSimpleCarSetPedInAsDriver(m_pTargetVehicle, m_pUtility);
    task->m_bWarpingInToCar = m_bWarpingInToCar;
    task->m_nDoorFlagsToClear = m_nDoorFlagsToClear;
    task->m_nNumGettingInToClear = m_nNumGettingInToClear;
    return task;
}

// 0x64B950
bool CTaskSimpleCarSetPedInAsDriver::ProcessPed(CPed* ped) {
    auto* const veh = m_pTargetVehicle;

    CEntity::SafeCleanUpRef(ped->m_pVehicle);
    CEntity::SetEntityReference(ped->m_pVehicle, veh);

    // Whoever drives the vehicle is thrown out, unless they are already on their way out
    if (auto* const oldDriver = veh->m_pDriver; oldDriver && oldDriver != ped) {
        auto& taskMgr = oldDriver->GetIntelligence()->GetTaskManager();
        if (!taskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
            && !taskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
            && !taskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_DIE)
            && !taskMgr.FindActiveTaskByType(TASK_SIMPLE_BIKE_JACKED)
        ) {
            const auto door = (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(veh, oldDriver);
            CTaskSimpleCarSetPedOut task{ veh, door, true, true };
            task.ProcessPed(oldDriver);
        }
    }

    ped->m_fAimingRotation = ped->m_fCurrentRotation;

    if (m_bWarpingInToCar) {
        ped->GetPosition() = veh->GetPosition();
    }

    // The leader of a group tells their followers that they are now driving
    auto* const group = CPedGroups::GetPedsGroup(ped);
    if (group && group->GetMembership().IsLeader(ped) && !group->GetIntelligence().GetCurrentEvent()) {
        CEventGroupEvent groupEvent{ ped, new CEventLeaderEnteredCarAsDriver{ veh } };
        group->GetIntelligence().AddEvent(&groupEvent);
    }

    veh->SetDriver(ped);
    ped->bInVehicle = true;

    if (ped->IsPlayer()) {
        ped->GetPlayerData()->m_bPlayersGangActive = true;
        ped->AsPlayer()->ClearAdrenaline();
    }

    if (ped->IsPlayer() && !veh->vehicleFlags.bHasBeenOwnedByPlayer) {
        veh->vehicleFlags.bHasBeenOwnedByPlayer = true;
        CCrime::ReportCrime(CRIME_CAR_STEAL, veh, FindPlayerPed());
    }

    ped->UpdateStatEnteringVehicle();
    ped->SetMoveState(PEDMOVE_NONE);
    ped->SetMoveAnim();
    ped->m_bUsesCollision = false;

    if (veh->m_nAlarmState == (uint16)-1) {
        veh->m_nAlarmState = 15000;
    }
    veh->vehicleFlags.bEngineOn = !veh->vehicleFlags.bEngineBroken;

    ped->SetPedState(PEDSTATE_DRIVING);

    if (ped->IsPlayer()) {
        veh->SetStatus(STATUS_PLAYER);
    } else if (veh->GetStatus() != STATUS_SIMPLE || ped->IsCreatedBy(PED_MISSION)) {
        veh->SetStatus(veh->m_nVehicleSubType == VEHICLE_TYPE_TRAIN ? STATUS_TRAIN_NOT_MOVING : STATUS_PHYSICS);
    }

    if (m_nDoorFlagsToClear) {
        veh->ClearGettingInFlags(m_nDoorFlagsToClear);
    }
    if (m_nNumGettingInToClear) {
        veh->m_nNumGettingIn -= m_nNumGettingInToClear;
    }

    CCarEnterExit::RemoveGetInAnims(ped);
    CCarEnterExit::AddInCarAnim(veh, ped, true);
    ped->RemoveWeaponWhenEnteringVehicle(0);

    // The ped is only rendered inside a vehicle flagged as a bus when it is the model hardcoded as MODEL_BUS
    ped->bRenderPedInCar = !veh->vehicleFlags.bIsBus || veh->m_nModelIndex == MODEL_BUS;

    ped->GetTaskManager().SetTask(
        new CTaskSimpleCarDrive{ veh, m_bWarpingInToCar ? nullptr : m_pUtility, false },
        TASK_PRIMARY_DEFAULT,
        false
    );

    if (ped->IsPlayer() && veh->IsLawEnforcementVehicle()) {
        CEventCopCarBeingStolen event{ ped, veh };
        GetEventGlobalGroup()->Add(&event, false);
    }

    return true;
}
