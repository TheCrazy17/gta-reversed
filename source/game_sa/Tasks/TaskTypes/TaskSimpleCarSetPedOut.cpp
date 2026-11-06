#include "StdInc.h"

#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimplePlayerOnFoot.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexWander.h"
#include "CarEnterExit.h"
#include "Garages.h"
#include "Bike.h"

void CTaskSimpleCarSetPedOut::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarSetPedOut, 0x86EEB8, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(Clone, 0x649F50);
    RH_ScopedVMTInstall(ProcessPed, 0x647D10);
}

CTaskSimpleCarSetPedOut::CTaskSimpleCarSetPedOut(CVehicle* targetVehicle, eTargetDoor nTargetDoor, bool bSwitchOffEngine, bool warpingOutOfCar) :
    m_nTargetDoor{ nTargetDoor },
    m_pTargetVehicle{ targetVehicle },
    m_bSwitchOffEngine{ bSwitchOffEngine },
    m_bWarpingOutOfCar{ warpingOutOfCar }
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

CTaskSimpleCarSetPedOut::~CTaskSimpleCarSetPedOut() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x6479B0
void CTaskSimpleCarSetPedOut::PositionPedOutOfCollision(CPed* ped, CVehicle* veh, int32 door) {
    return plugin::Call<0x6479B0>(ped, veh, door);
}

// 0x649F50
CTask* CTaskSimpleCarSetPedOut::Clone() const {
    auto* const task = new CTaskSimpleCarSetPedOut{ m_pTargetVehicle, m_nTargetDoor, m_bSwitchOffEngine };
    task->m_bWarpingOutOfCar      = m_bWarpingOutOfCar;
    task->m_bFallingOutOfCar      = m_bFallingOutOfCar;
    task->m_bKnockedOffBike       = m_bKnockedOffBike;
    task->m_nDoorFlagsToClear     = m_nDoorFlagsToClear;
    task->m_nNumGettingInToClear  = m_nNumGettingInToClear;
    return task;
}

// 0x647D10
bool CTaskSimpleCarSetPedOut::ProcessPed(CPed* ped) {
    ped->bInVehicle = false;
    ped->SetUsesCollision(true);
    ped->UpdateStatLeavingVehicle();

    if (!m_bKnockedOffBike) {
        PositionPedOutOfCollision(ped, nullptr, m_nTargetDoor);
    }
    CCarEnterExit::RemoveCarSitAnim(ped);
    ped->RestartNonPartialAnims();

    // Whoever gets out keeps no speed, except when the vehicle is a boat or they are falling / knocked off
    if (!m_bKnockedOffBike && !m_bFallingOutOfCar && m_pTargetVehicle->m_nVehicleSubType != VEHICLE_TYPE_BOAT) {
        ped->m_vecMoveSpeed = CVector{};
    }

    if (auto* const veh = ped->m_pVehicle) {
        if (m_nDoorFlagsToClear) {
            m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsToClear);
        }
        if (m_nNumGettingInToClear) {
            m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInToClear;
        }

        if (veh->m_pDriver == ped) {
            veh->RemoveDriver(!m_bSwitchOffEngine);
            veh->SetStatus(STATUS_ABANDONED);
            if (veh->m_nDoorLock == CARLOCK_COP_CAR) {
                veh->m_nDoorLock = CARLOCK_UNLOCKED;
            }
            if (ped->m_nPedType == PED_TYPE_COP && veh->IsLawEnforcementVehicle()) {
                veh->ChangeLawEnforcerState(false);
            }
        } else {
            veh->RemovePassenger(ped);
        }

        // A vehicle left inside a garage gets its lights back to the default
        CVector pos = ped->GetPosition();
        if (CGarages::IsPointWithinAnyGarage(pos)) {
            veh->m_nOverrideLights = NO_CAR_LIGHT_OVERRIDE;
            veh->vehicleFlags.bLightsOn = false;
        }
    }

    // A bike that is left at rest is put on its side stand
    if (!m_bFallingOutOfCar && !m_bKnockedOffBike) {
        if (auto* const veh = ped->m_pVehicle; veh && veh->m_nVehicleType == VEHICLE_TYPE_BIKE
            && std::fabs(veh->m_vecMoveSpeed.x) < 0.1 && std::fabs(veh->m_vecMoveSpeed.y) < 0.1
        ) {
            veh->AsBike()->bikeFlags.bOnSideStand = true;
        }
    }

    auto& taskMgr = ped->GetTaskManager();
    if (ped->IsPlayer()) {
        taskMgr.SetTask(new CTaskSimplePlayerOnFoot{}, TASK_PRIMARY_DEFAULT, false);
    } else if (ped->IsCreatedBy(PED_MISSION)) {
        taskMgr.SetTask(new CTaskSimpleStandStill{ 999999, true, false, 8.0f }, TASK_PRIMARY_DEFAULT, false);
    } else {
        taskMgr.SetTask(CTaskComplexWander::GetWanderTaskByPedType(ped), TASK_PRIMARY_DEFAULT, false);
    }

    ped->ReplaceWeaponWhenExitingVehicle();
    ped->bHasJustLeftCar = true;
    ped->m_nPedState     = PEDSTATE_IDLE;

    if (m_pTargetVehicle && m_pTargetVehicle->physicalFlags.bTouchingWater) {
        ped->physicalFlags.bTouchingWater = true;
    }
    return true;
}
