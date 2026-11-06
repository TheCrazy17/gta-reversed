#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>
#include <cassert>

#include "PlayerInfo.h"
#include "World.h"
#include "CarGenerator.h"
#include "TheCarGenerators.h"
#include "CommandParser/Parser.hpp"
using namespace notsa::script;

/*!
* Various vehicle commands
*/

namespace {
void ClearHeliOrientation(CHeli& heli) {
    heli.ClearHeliOrientation();
}

void RemoveRCBuggy() {
    FindPlayerInfo().BlowUpRCBuggy(false);
}

void SetCarProofs(CVehicle& veh, bool bullet, bool fire, bool explosion, bool collision, bool melee) {
    auto& flags           = veh.physicalFlags;
    flags.bBulletProof    = bullet;
    flags.bFireProof      = fire;
    flags.bExplosionProof = explosion;
    flags.bCollisionProof = collision;
    flags.bMeleeProof     = melee;
}

void SwitchCarGenerator(int32 generatorId, int32 count) {
    const auto generator = CTheCarGenerators::Get(generatorId);
    if (count) {
        generator->SwitchOn();
        if (count <= 100) {
            generator->m_nGenerateCount = count;
        }
    } else {
        generator->SwitchOff();
    }
}

void SetHasBeenOwnedForCarGenerator(int32 generatorId, bool alreadyOwned) {
    CTheCarGenerators::Get(generatorId)->bPlayerHasAlreadyOwnedCar = alreadyOwned;
}

float GetCarSpeed(CVehicle& veh) {
    return veh.m_vecMoveSpeed.Magnitude() * 50.f;
}

void SetCarDrivingStyle(CVehicle& veh, eCarDrivingStyle style) {
    veh.m_autoPilot.m_nCarDrivingStyle = style;
}

bool IsFirstCarColor(CVehicle& veh, int32 color) {
    return veh.m_nPrimaryColor == color;
}

bool IsSecondCarColor(CVehicle& veh, int32 color) {
    return veh.m_nSecondaryColor == color;
}

MultiRet<uint8, uint8> GetExtraCarColors(CVehicle& veh) {
    return { veh.m_nTertiaryColor, veh.m_nQuaternaryColor };
}

void PopCarBootUsingPhysics(CAutomobile& automobile) {
    automobile.PopBootUsingPhysics();
}

void SkipToNextAllowedStation(CTrain& train) {
    CTrain::SkipToNextAllowedStation(&train);
}

void SetRailTrackResistanceMult(float value) {
    CVehicle::ms_fRailTrackResistance = CVehicle::ms_fRailTrackResistanceDefault * (value > 0.0f ? value : 1.0f);
}

void DisableHeliAudio(CVehicle& vehicle, bool enable) {
    if (enable) {
        vehicle.m_vehicleAudio.EnableHelicoptor();
    } else {
        vehicle.m_vehicleAudio.DisableHelicoptor();
    }
}

bool IsCarInAirProper(CRunningScript& S, CVehicle& vehicle) {
    for (auto* const e : vehicle.GetCollidingEntities()) {
        if (e && (e->GetIsTypeBuilding() || e->GetIsTypeVehicle())) {
            return false;
        }
    }
    return true;
}

bool IsCarStuck(CVehicle& vehicle) {
    return CTheScripts::StuckCars.HasCarBeenStuckForAWhile(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheck(CVehicle& vehicle, float stuckRadius, uint32 time) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, false, false, false, false, 0);
}

void RemoveStuckCarCheck(CVehicle& vehicle) {
    CTheScripts::StuckCars.RemoveCarFromCheck(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheckWithWarp(CVehicle& vehicle, float stuckRadius, uint32 time, bool stuck, bool flipped, bool inWater, int8 numberOfNodesToCheck) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, true, stuck, flipped, inWater, numberOfNodesToCheck);
}

void PlaneAttackPlayerUsingDogFight(CPlane& plane, CPlayerPed& player, float altitude) {
    if (plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        plane.m_autoPilot.SetCarMission(eCarMission::MISSION_PLANE_DOG_FIGHT_PLAYER);
    }
    plane.m_minAltitude = altitude;
}

/// SET_CAR_ALWAYS_CREATE_SKIDS(07EE)
void SetCarAlwaysCreateSkids(CVehicle& vehicle, bool enable) {
    vehicle.vehicleFlags.bAlwaysSkidMarks = enable;
}

/// SET_CAR_AS_MISSION_CAR(0763)
void SetCarAsMissionCar(CRunningScript& S, CVehicle& vehicle) {
    if (S.m_UsesMissionCleanup && (vehicle.IsCreatedBy(eVehicleCreatedBy::RANDOM_VEHICLE) || vehicle.IsCreatedBy(eVehicleCreatedBy::PARKED_VEHICLE))) {
        vehicle.SetVehicleCreatedBy(eVehicleCreatedBy::MISSION_VEHICLE);
        CTheScripts::MissionCleanUp.AddEntityToList(vehicle);
    }
}

/// IS_CAR_IN_WATER(02BF)
bool IsCarInWater(CVehicle& vehicle) {
    if (vehicle.physicalFlags.bSubmergedInWater) {
        return true;
    }

    // The Vortex (hovercraft, hardcoded by model) is also in water while its first wheel is pressed against shallow water
    if (vehicle.m_nModelIndex == MODEL_VORTEX) {
        auto& automobile = *vehicle.AsAutomobile();
        return automobile.m_fWheelsSuspensionCompression[0] < 1.0f
            && g_surfaceInfos.IsShallowWater(automobile.m_wheelColPoint[0].m_nSurfaceTypeB);
    }
    return false;
}

/// IS_EMERGENCY_SERVICES_VEHICLE(0975)
bool IsEmergencyServicesVehicle(CVehicle& vehicle) {
    // Besides the police vehicles, the models hardcoded as emergency services are the ambulance and both fire trucks
    return vehicle.IsLawEnforcementVehicle()
        || vehicle.m_nModelIndex == MODEL_AMBULAN
        || vehicle.m_nModelIndex == MODEL_FIRETRUK
        || vehicle.m_nModelIndex == MODEL_FIRELA;
}

/// IS_COP_VEHICLE_IN_AREA_3D_NO_SAVE(09C3)
bool IsCopVehicleInArea3DNoSave(CVector p1, CVector p2) {
    // The corners can be given in any order
    if (p1.x > p2.x) { std::swap(p1.x, p2.x); }
    if (p1.y > p2.y) { std::swap(p1.y, p2.y); }
    if (p1.z > p2.z) { std::swap(p1.z, p2.z); }

    bool found = false;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        // The Predator (police boat, hardcoded by model) is not counted as a cop vehicle here
        if (!vehicle.IsLawEnforcementVehicle() || vehicle.m_nModelIndex == MODEL_PREDATOR) {
            continue;
        }
        const auto& pos = vehicle.GetPosition();
        if (p1.x <= pos.x && pos.x <= p2.x
            && p1.y <= pos.y && pos.y <= p2.y
            && p1.z <= pos.z && pos.z <= p2.z
        ) {
            found = true;
        }
    }
    return found;
}

/// PLANE_STARTS_IN_AIR(0745)
void PlaneStartsInAir(CPlane& plane) {
    plane.IsAlreadyFlying();

    // Only the Hydra (hardcoded by model) gets the angle of its moving component reset
    if (plane.m_nModelIndex == MODEL_HYDRA) {
        plane.m_wMiscComponentAngle = 0;
    }
}

}

void notsa::script::commands::vehicle::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Vehicle");

    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_HELI_ORIENTATION, ClearHeliOrientation);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_RC_BUGGY, RemoveRCBuggy);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_PROOFS, SetCarProofs);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_CAR_GENERATOR, SwitchCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_HAS_BEEN_OWNED_FOR_CAR_GENERATOR, SetHasBeenOwnedForCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_SPEED, GetCarSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_DRIVING_STYLE, SetCarDrivingStyle);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_FIRST_CAR_COLOUR, IsFirstCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_SECOND_CAR_COLOUR, IsSecondCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_EXTRA_CAR_COLOURS, GetExtraCarColors);
    REGISTER_COMMAND_HANDLER(COMMAND_POP_CAR_BOOT_USING_PHYSICS, PopCarBootUsingPhysics);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_TO_NEXT_ALLOWED_STATION, SkipToNextAllowedStation);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RAILTRACK_RESISTANCE_MULT, SetRailTrackResistanceMult);
    REGISTER_COMMAND_HANDLER(COMMAND_DISABLE_HELI_AUDIO, DisableHeliAudio);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_AIR_PROPER, IsCarInAirProper);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STUCK, IsCarStuck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK, AddStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_STUCK_CAR_CHECK, RemoveStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK_WITH_WARP, AddStuckCarCheckWithWarp);
    REGISTER_COMMAND_HANDLER(COMMAND_PLANE_ATTACK_PLAYER_USING_DOG_FIGHT, PlaneAttackPlayerUsingDogFight);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ALWAYS_CREATE_SKIDS, SetCarAlwaysCreateSkids);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_WATER, IsCarInWater);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_EMERGENCY_SERVICES_VEHICLE, IsEmergencyServicesVehicle);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_COP_VEHICLE_IN_AREA_3D_NO_SAVE, IsCopVehicleInArea3DNoSave);
    REGISTER_COMMAND_HANDLER(COMMAND_PLANE_STARTS_IN_AIR, PlaneStartsInAir);


    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_TAXI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_TAXI_TIMER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_BOAT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_CARS_COLLECTED_BY_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CAR_BEEN_TAKEN_TO_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_RANDOM_CAR_OF_TYPE_IN_ZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_RESPRAY_HAPPENED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_RAM_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BLOCK_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_FUNNY_SUSPENSION);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BIG_WHEELS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_CAR_RADIO);
}
