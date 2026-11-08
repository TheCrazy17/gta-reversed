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

    RH_ScopedInstall(PositionPedOutOfCollision, 0x6479B0);
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
    if (!veh) {
        veh = ped->m_pVehicle;
        if (!veh) {
            return;
        }
    }

    // The ped faces the way the vehicle does
    const float heading = veh->GetHeading();
    ped->m_fAimingRotation = ped->m_fCurrentRotation = heading;
    ped->SetHeading(heading);

    if (veh->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        if (veh->m_pHandlingData->m_bSitInBoat) {
            ped->GetPosition() += veh->GetUp() * 0.5f;
        }

        if (ped->TestCollision(false)) {
            // Nudge the ped and test again; if it is still stuck, let the ped search for a free spot
            if (veh->vehicleFlags.bIsDrowning) {
                ped->GetPosition().z -= 0.3f;
                if (!ped->TestCollision(false)) {
                    return;
                }
                ped->GetPosition().z += 0.3f;
                ped->PositionPedOutOfCollision(door, veh, true);
                return;
            }

            ped->GetPosition() -= ped->GetForward() * 0.3f;
            if (ped->TestCollision(false)) {
                ped->GetPosition() += ped->GetForward() * 0.3f;
                ped->PositionPedOutOfCollision(door, veh, true);
                return;
            }
        }

        // The ped keeps most of the boat's speed and stands on it
        ped->m_vecMoveSpeed = veh->m_vecMoveSpeed * 0.9f;
        ped->m_vecMoveSpeed.z -= 0.1f;
        ped->bIsStanding = true;
        if (!ped->m_standingOnEntity) {
            ped->m_standingOnEntity = veh;
            veh->RegisterReference(&ped->m_standingOnEntity);
        }
        return;
    }

    // Anything solid at the ped's feet or head, or a wall between the vehicle and the ped, means the exit point is blocked
    CWorld::pIgnoreEntity = veh;

    bool blocked = false;
    for (const float offsetZ : { -0.2f, 0.2f }) {
        const CVector center = ped->GetPosition() + CVector{ 0.0f, 0.0f, offsetZ };
        const auto*   hit = CWorld::TestSphereAgainstWorld(center, 0.4f, veh, true, true, false, false, false, false);
        if (hit && hit != veh->m_pAttachedTo) {
            blocked = true;
        }
    }

    if (!CWorld::GetIsLineOfSightClear(veh->GetPosition(), ped->GetPosition(), true, false, false, true, false, false, false) || blocked) {
        ped->PositionPedOutOfCollision(door, veh, true);
    }

    CWorld::pIgnoreEntity = nullptr;
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
