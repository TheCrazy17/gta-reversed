#include "StdInc.h"

#include "Remote.h"
#include "Camera.h"
#include "CarCtrl.h"
#include "World.h"
#include "TheScripts.h"
#include "Entity/Vehicle/Automobile.h"
#include "Entity/Vehicle/Heli.h"
#include "Entity/Vehicle/Plane.h"

void CRemote::InjectHooks() {
    RH_ScopedClass(CRemote);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(TakeRemoteControlledCarFromPlayer, 0x45AE80);
    RH_ScopedInstall(GivePlayerRemoteControlledCar, 0x45AB10);
}

// 0x45AE80
void CRemote::TakeRemoteControlledCarFromPlayer(bool bCreateRemoteVehicleExplosion)
{
    auto& playerInfo = CWorld::Players[CWorld::PlayerInFocus];
    auto* const veh = playerInfo.m_pRemoteVehicle;

    // A car created by a mission stops being one: it becomes a random vehicle and leaves the mission cleanup list
    if (veh->m_nCreatedBy == MISSION_VEHICLE) {
        veh->SetVehicleCreatedBy(RANDOM_VEHICLE);
        CTheScripts::MissionCleanUp.RemoveEntityFromList(*veh);
    }
    veh->vehicleFlags.bIsLocked = false;

    playerInfo.m_nTimeOfRemoteVehicleExplosion       = CTimer::GetTimeInMS();
    playerInfo.m_bAfterRemoteVehicleExplosion        = true;
    playerInfo.m_bCreateRemoteVehicleExplosion       = bCreateRemoteVehicleExplosion;
    playerInfo.m_bFadeAfterRemoteVehicleExplosion    = true;
}

// 0x45AB10
void CRemote::GivePlayerRemoteControlledCar(CVector pos, float rotation, int16 modelId)
{
    // The kind of vehicle is decided by the model: RC Goblin is a heli, RC Baron a plane, the rest are automobiles
    CVehicle* veh;
    if (CModelInfo::IsHeliModel(modelId)) {
        veh = new CHeli(modelId, MISSION_VEHICLE);
    } else if (CModelInfo::IsPlaneModel(modelId)) {
        veh = new CPlane(modelId, MISSION_VEHICLE);
    } else {
        veh = new CAutomobile(modelId, MISSION_VEHICLE, true);
    }

    bool foundGround{};
    const auto groundZ = CWorld::FindGroundZFor3DCoord({ pos.x, pos.y, pos.z + 1.0f }, &foundGround, nullptr);
    pos.z = groundZ + veh->GetDistanceFromCentreOfMassToBaseOfModel();

    veh->GetMatrix().SetRotateZOnly(rotation);
    veh->GetPosition() = pos;

    veh->SetStatus(STATUS_REMOTE_CONTROLLED);
    veh->vehicleFlags.bIsLocked = true;
    CCarCtrl::JoinCarWithRoadSystem(veh);

    auto& autoPilot = veh->m_autoPilot;
    autoPilot.m_nCurrentLane   = 0;
    autoPilot.m_nNextLane      = 0;
    autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    autoPilot.m_nCarMission    = MISSION_NONE;
    autoPilot.m_nTempAction    = TEMPACT_NONE;
    autoPilot.m_speed          = 9.0f;
    autoPilot.m_nCruiseSpeed   = 9;

    veh->vehicleFlags.bEngineOn = !veh->vehicleFlags.bEngineBroken;
    CWorld::Add(veh);

    // Whatever the player was driving stops while the remote car is controlled
    if (auto* const playerVeh = FindPlayerVehicle()) {
        playerVeh->SetStatus(STATUS_FORCED_STOP);
    }

    auto& playerInfo = CWorld::Players[CWorld::PlayerInFocus];
    CEntity::SetEntityReference(playerInfo.m_pRemoteVehicle, veh);

    TheCamera.TakeControl(veh, MODE_CAM_ON_A_STRING, eSwitchType::INTERPOLATION, 1);
    TheCamera.SetZoomValueCamStringScript(1);
}
