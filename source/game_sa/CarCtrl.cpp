/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "CarCtrl.h"
#include "Tasks/TaskTypes/TaskComplexLeaveAnyCar.h"
#include "Entity/Vehicle/Plane.h"
#include "Entity/Vehicle/Train.h"
#include "Wanted.h"
#include "TheZones.h"
#include "Streaming.h"
#include "FireManager.h"
#include "AccidentManager.h"
#include "Tasks/TaskTypes/TaskComplexWander.h"
#include "BMX.h"
#include "CarAI.h"
#include "TrafficLights.h"
#include "ModelIndices.h"
#include "TheScripts.h"
#include "GangWars.h"
#include "Game.h"
#include "General.h"
#include "GameLogic.h"
#include "CutsceneMgr.h"
#include "TheCarGenerators.h"
#include "eAreaCodes.h"

#include <numbers>

#include <reversiblebugfixes/Bugs.hpp>

auto& apCarsToKeep = StaticRef<CVehicle*[2]>(0x969084);
auto& aCarsToKeepTime = StaticRef<std::array<uint32, 2>>(0x96907C);

// Calls `fn` for every repeat sector the rectangle touches, with a fresh scan code
template<typename Fn>
static void ForEachRepeatSectorInRect(float minX, float minY, float maxX, float maxY, Fn&& fn) {
    const auto Sector = [](float coord) { return (int32)std::floor(coord * 0.02f + 60.0f); };

    const auto startX = std::max(Sector(minX), 0);
    const auto startY = std::max(Sector(minY), 0);
    const auto endX   = std::min(Sector(maxX), 119);
    const auto endY   = std::min(Sector(maxY), 119);

    CWorld::AdvanceCurrentScanCode();
    for (auto y = startY; y <= endY; y++) {
        for (auto x = startX; x <= endX; x++) {
            fn(CWorld::GetRepeatSector(x, y));
        }
    }
}

void CCarCtrl::InjectHooks()
{
    RH_ScopedClass(CCarCtrl);
    RH_ScopedCategoryGlobal();

    using namespace ReversibleHooks;
    RH_ScopedInstall(Init, 0x4212E0);
    RH_ScopedInstall(ReInit, 0x4213B0);
    RH_ScopedInstall(InitSequence, 0x421740);
    RH_ScopedInstall(ChooseGangCarModel, 0x421A40);
    RH_ScopedInstall(ChoosePoliceCarModel, 0x421980);
    RH_ScopedInstall(CreateCarForScript, 0x431F80);
    RH_ScopedInstall(ChooseBoatModel, 0x421970);
    RH_ScopedInstall(ChooseCarModelToLoad, 0x421900);
    RH_ScopedInstall(GetNewVehicleDependingOnCarModel, 0x421440);
    RH_ScopedInstall(IsAnyoneParking, 0x42C250);
    RH_ScopedInstall(IsThisVehicleInteresting, 0x423EA0);
    RH_ScopedInstall(JoinCarWithRoadAccordingToMission, 0x432CB0);
    RH_ScopedInstall(PossiblyFireHSMissile, 0x429600);
    RH_ScopedInstall(PruneVehiclesOfInterest, 0x423F10);
    RH_ScopedInstall(RemoveCarsIfThePoolGetsFull, 0x4322B0);
    RH_ScopedInstall(RemoveDistantCars, 0x42CD10);
    RH_ScopedInstall(RemoveFromInterestingVehicleList, 0x423ED0);
    RH_ScopedInstall(ScriptGenerateOneEmergencyServicesCar, 0x42FBC0);
    RH_ScopedInstall(SlowCarDownForObject, 0x426220);
    RH_ScopedInstall(SlowCarOnRailsDownForTrafficAndLights, 0x434790);
    RH_ScopedInstall(FindMaxSteerAngle, 0x427FE0);
    RH_ScopedInstall(GenerateRandomCars, 0x4341C0);
    RH_ScopedInstall(ChooseModel, 0x424CE0);
    RH_ScopedInstall(ClearInterestingVehicleList, 0x423F00);
    RH_ScopedInstall(ClitargetOrientationToLink, 0x422760);
    RH_ScopedInstall(CreateConvoy, 0x42C740, { .Reversed = false });
    RH_ScopedInstall(CreatePoliceChase, 0x42C2B0, { .Reversed = false });
    RH_ScopedInstall(DealWithBend_Racing, 0x428040, { .Reversed = false });
    RH_ScopedInstall(DragCarToPoint, 0x42EC90, { .Reversed = false });
    RH_ScopedInstall(FindAngleToWeaveThroughTraffic, 0x4325C0);
    RH_ScopedInstall(FindIntersection2Lines, 0x4226F0);
    RH_ScopedInstall(FindLinksToGoWithTheseNodes, 0x42B470);
    RH_ScopedInstall(FindMaximumSpeedForThisCarInTraffic, 0x434400);
    RH_ScopedInstall(FindNodesThisCarIsNearestTo, 0x42BD20, { .Reversed = false });
    RH_ScopedInstall(FindPathDirection, 0x422090);
    RH_ScopedInstall(FindPercDependingOnDistToLink, 0x422620);
    RH_ScopedInstall(FindSequenceElement, 0x421770);
    RH_ScopedInstall(FindSpeedMultiplier, 0x4224E0);
    RH_ScopedInstall(FindSpeedMultiplierWithSpeedFromNodes, 0x424130);
    RH_ScopedInstall(FindGhostRoadHeight, 0x422370);
    RH_ScopedInstall(FireHeliRocketsAtTarget, 0x42B270);
    RH_ScopedInstall(FlyAIHeliInCertainDirection, 0x429A70, { .Reversed = false });
    RH_ScopedInstall(FlyAIHeliToTarget_FixedOrientation, 0x423940, { .Reversed = false });
    RH_ScopedInstall(FlyAIPlaneInCertainDirection, 0x423000, { .Reversed = false });
    RH_ScopedInstall(GenerateCarCreationCoors2, 0x424210, { .Reversed = false });
    RH_ScopedInstall(GenerateEmergencyServicesCar, 0x42F9C0);
    RH_ScopedInstall(GenerateOneEmergencyServicesCar, 0x42B7D0);
    RH_ScopedInstall(GenerateOneRandomCar, 0x430050, { .Reversed = false });
    RH_ScopedInstall(GetAIHeliToAttackPlayer, 0x42F3C0);
    RH_ScopedInstall(GetAIHeliToFlyInDirection, 0x42A730);
    RH_ScopedInstall(GetAIPlaneToAttackPlayer, 0x429780);
    RH_ScopedInstall(GetAIPlaneToDoDogFight, 0x429890);
    RH_ScopedInstall(GetAIPlaneToDoDogFightAgainstPlayer, 0x42F370);
    RH_ScopedInstall(IsThisAnAppropriateNode, 0x42DAB0);
    RH_ScopedInstall(JoinCarWithRoadSystem, 0x42F5A0);
    RH_ScopedInstall(JoinCarWithRoadSystemGotoCoors, 0x42F870);
    RH_ScopedInstall(PickNextNodeAccordingStrategy, 0x432B10);
    RH_ScopedInstall(PickNextNodeRandomly, 0x42DE80, { .Reversed = false });
    RH_ScopedInstall(PickNextNodeToChaseCar, 0x426EF0, { .Reversed = false });
    RH_ScopedInstall(PickNextNodeToFollowPath, 0x427740, { .Reversed = false });
    RH_ScopedInstall(PossiblyRemoveVehicle, 0x424F80, { .Reversed = false });
    RH_ScopedInstall(ReconsiderRoute, 0x42FC40);
    RH_ScopedInstall(RegisterVehicleOfInterest, 0x423DE0);
    RH_ScopedInstall(ScanForPedDanger, 0x42CE40);
    RH_ScopedInstall(SetCoordsOfScriptCar, 0x4342A0, { .Reversed = false });
    RH_ScopedInstall(SetUpDriverAndPassengersForVehicle, 0x4217C0);
    RH_ScopedInstall(SlowCarDownForCarsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x432420, { .Reversed = false });
    RH_ScopedInstall(SlowCarDownForObjectsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D4F0);
    RH_ScopedInstall(SlowCarDownForOtherCar, 0x42D0E0, { .Reversed = false });
    RH_ScopedInstall(SlowCarDownForPedsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x425440, { .Reversed = false });
    RH_ScopedInstall(SteerAIBoatWithPhysicsAttackingPlayer, 0x428DE0);
    RH_ScopedInstall(SteerAIBoatWithPhysicsCirclingPlayer, 0x429090);
    RH_ScopedInstall(SteerAIBoatWithPhysicsHeadingForTarget, 0x428BE0);
    RH_ScopedInstall(SteerAICarBlockingPlayerForwardAndBack, 0x422B20);
    RH_ScopedInstall(SteerAICarParkParallel, 0x433BA0);
    RH_ScopedInstall(SteerAICarParkPerpendicular, 0x433EA0);
    RH_ScopedInstall(SteerAICarTowardsPointInEscort, 0x4336D0, { .Reversed = false });
    RH_ScopedInstall(SteerAICarWithPhysics, 0x437C20, { .Reversed = false });
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath, 0x434900, { .Reversed = false });
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath_Racing, 0x435830, { .Reversed = false });
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPreRecordedPath, 0x432DD0, { .Reversed = false });
    RH_ScopedInstall(SteerAICarWithPhysicsHeadingForTarget, 0x433280);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget, 0x4335E0);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget_Stop, 0x428990);
    RH_ScopedInstall(SteerAICarWithPhysics_OnlyMission, 0x436A90, { .Reversed = false });
    RH_ScopedInstall(SteerAIHeliAsPoliceHeli, 0x42AAD0);
    RH_ScopedInstall(SteerAIHeliFlyingAwayFromPlayer, 0x42ACB0);
    RH_ScopedInstall(SteerAIHeliToCrashAndBurn, 0x4238E0);
    RH_ScopedInstall(SteerAIHeliToFollowEntity, 0x42A750);
    RH_ScopedInstall(SteerAIHeliToKeepEntityInView, 0x42AEB0);
    RH_ScopedInstall(SteerAIHeliToLand, 0x42AD30);
    RH_ScopedInstall(SteerAIHeliTowardsTargetCoors, 0x42A630);
    RH_ScopedInstall(SteerAIPlaneToCrashAndBurn, 0x423880);
    RH_ScopedInstall(SteerAIPlaneToFollowEntity, 0x4237F0);
    RH_ScopedInstall(SteerAIPlaneTowardsTargetCoors, 0x423790);
    RH_ScopedInstall(StopCarIfNodesAreInvalid, 0x422590);
    RH_ScopedInstall(SwitchBetweenPhysicsAndGhost, 0x4222A0);
    RH_ScopedInstall(SwitchVehicleToRealPhysics, 0x423FC0);
    RH_ScopedInstall(TestCollisionBetween2MovingRects, 0x425B30);
    RH_ScopedInstall(TestCollisionBetween2MovingRects_OnlyFrontBumper, 0x425F70);
    RH_ScopedInstall(TestWhetherToFirePlaneGuns, 0x429520);
    RH_ScopedInstall(ThisVehicleShouldTryNotToTurn, 0x421FE0);
    RH_ScopedInstall(TriggerDogFightMoves, 0x429300);
    RH_ScopedInstall(UpdateCarCount, 0x424000);
    RH_ScopedInstall(UpdateCarOnRails, 0x436540, { .Reversed = false });
    RH_ScopedInstall(WeaveForObject, 0x426BC0);
    RH_ScopedInstall(WeaveForPed, 0x426970);
    RH_ScopedInstall(WeaveForOtherCar, 0x426350);
    RH_ScopedInstall(WeaveThroughCarsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D680);
    RH_ScopedInstall(WeaveThroughObjectsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D950);
    RH_ScopedInstall(WeaveThroughPedsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D7E0);
}

// 0x4212E0
void CCarCtrl::Init() {
    ZoneScoped;

    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeAmbulanceCreated = 0;
    LastTimeFireTruckCreated = 0;
    bAllowEmergencyServicesToBeCreated = true;
    bCarsGeneratedAroundCamera = false;
    CountDownToCarsAtStart = 2;

    TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(600.0f, 1200.0f);

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x4213B0
void CCarCtrl::ReInit() {
    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeLawEnforcerCreated = 0;

    bAllowEmergencyServicesToBeCreated = true;
    CountDownToCarsAtStart = 2;

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x421970
int32 CCarCtrl::ChooseBoatModel() {
    return CPopulation::m_LoadedBoats.PickLeastUsedModel(1);
}

// 0x421900
int32 CCarCtrl::ChooseCarModelToLoad(int32 groupID) {
    const auto numCarsInGroup = CPopulation::m_nNumCarsInGroup[groupID];
    if (numCarsInGroup > 0) {
        for (auto i = 0; i < 16; i++) { // 16 tries
            const auto model = CPopulation::m_CarGroups[groupID][CGeneral::GetRandomNumberInRange(numCarsInGroup)];
            if (!CStreaming::IsModelLoaded(model)) {
                return model;
            }
        }
    }
    return -1;
}

eModelID CCarCtrl::ChooseGangCarModel(eGangID loadedCarGroupId) {
    return CPopulation::PickGangCar(loadedCarGroupId);
}

// 0x424CE0
int32 CCarCtrl::ChooseModel(int32* outZoneType) {
    const auto numTotalCars = (int32)(CPopCycle::m_NumOther_Cars + CPopCycle::m_NumCops_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars);
    if (numTotalCars <= 0) {
        return MODEL_INVALID;
    }

    if (CCheat::IsActive(CHEAT_REDUCED_TRAFFIC)) {
        if ((int32)((float)(CGeneral::GetRandomNumber() & 0xFFFF) * (1.f / 32768.f) * 100.f) != 0) {
            return MODEL_INVALID;
        }
    }

    const auto roll           = (float)CGeneral::GetRandomNumber() * 0.000030518509447574615f;
    const auto total          = (float)numTotalCars;
    const auto gangsOwnStreet = CCheat::IsActive(CHEAT_GANGS_CONTROLS_THE_STREETS);

    if (CPopCycle::m_NumDealers_Cars / total > roll && !gangsOwnStreet) {
        *outZoneType = 25;
        const auto model = CPopulation::m_CarGroups[POPCYCLE_CARGROUP_DEALERS][0];
        return CStreaming::IsModelLoaded(model) ? model : MODEL_INVALID;
    }

    if ((CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars) / total > roll || gangsOwnStreet) {
        if (CPopulation::m_bDontCreateRandomGangMembers) {
            return MODEL_INVALID;
        }

        const auto* const zoneInfo = CPopCycle::m_pCurrZoneInfo;
        const auto        sum      = (int32)zoneInfo->GetSumOfGangDensity();
        if (sum <= 0) {
            return MODEL_INVALID;
        }

        // Pick a gang, weighted by how much of the zone it owns
        auto pick = (int32)((float)(CGeneral::GetRandomNumber() & 0xFFFF) * (1.f / 32768.f) * (float)sum);
        auto gang = 0;
        if (zoneInfo->GangStrength[0] < pick) {
            auto strength = (int32)zoneInfo->GangStrength[0];
            do {
                pick -= strength;
                strength = zoneInfo->GangStrength[++gang];
            } while (strength < pick);
        }
        if (gangsOwnStreet) {
            gang = CGeneral::GetRandomNumberInRange(0, 9);
        }

        *outZoneType = gang + 14;
        if (CPopulation::PickGangCar((eGangID)gang) < 0) { // No model of this gang is loaded
            return MODEL_INVALID;
        }

        for (auto tries = 0; tries < 10; tries++) {
            int16 model;
            do {
                model = CPopulation::m_CarGroups[POPCYCLE_CARGROUP_BALLAS + gang][(int32)((float)(CGeneral::GetRandomNumber() & 0xFFFF) * (1.f / 32768.f) * 23.f)];
            } while (model == 2000); // Unused entry
            if (CStreaming::IsModelLoaded(model)) {
                return model;
            }
        }
        return MODEL_INVALID;
    }

    if ((CPopCycle::m_NumCops_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars) / total > roll) {
        if (CGangWars::GangWarFightingGoingOn() || CPopulation::m_bDontCreateRandomCops) {
            return MODEL_INVALID;
        }
        *outZoneType = 13;
        return ChoosePoliceCarModel(0);
    }

    *outZoneType = 0;
    if (CTheScripts::ForceRandomCarModel != -1) {
        return CTheScripts::ForceRandomCarModel;
    }
    return CPopulation::m_AppropriateLoadedCars.PickRandomCar(true, false);
}

int32 CCarCtrl::ChoosePoliceCarModel(uint32 ignoreLvpd1Model) {
    CWanted* playerWanted = FindPlayerWanted();
    if (playerWanted->AreSwatRequired()
        && CStreaming::IsModelLoaded(MODEL_ENFORCER)
        && CStreaming::IsModelLoaded(MODEL_SWAT)
    ) {
        if (CGeneral::GetRandomNumberInRange(0, 3) == 2)
            return MODEL_ENFORCER;
    }
    else
    {
        if (playerWanted->AreFbiRequired()
            && CStreaming::IsModelLoaded(MODEL_FBIRANCH)
            && CStreaming::IsModelLoaded(MODEL_FBI))
            return MODEL_FBIRANCH;

        if (playerWanted->AreArmyRequired()
            && CStreaming::IsModelLoaded(MODEL_RHINO)
            && CStreaming::IsModelLoaded(MODEL_BARRACKS)
            && CStreaming::IsModelLoaded(MODEL_ARMY))
            return (CGeneral::GetRandomNumber() < 0x3FFF) + MODEL_RHINO;
    }
    return CStreaming::GetDefaultCopCarModel(ignoreLvpd1Model);
}

// 0x423F00
void CCarCtrl::ClearInterestingVehicleList() {
    apCarsToKeep[0] = nullptr;
    apCarsToKeep[1] = nullptr;
}

// 0x422760
void CCarCtrl::ClitargetOrientationToLink(CVehicle* vehicle, CCarPathLinkAddress linkAddress, int8 direction, float* orientation, float targetX, float targetY) {
    if (!ThePaths.m_pPathNodes[linkAddress.m_wAreaId]) {
        return;
    }
    const auto& link = ThePaths.GetCarPathLink(linkAddress);

    // Direction of the link, and its perpendicular. Both lane edges are offset along the latter
    const auto dirX = (float)link.m_dir.x * (float)direction;
    const auto dirY = (float)link.m_dir.y * (float)direction;

    float edgeA, edgeB; // Distances (in lanes) from the center of the link to its two edges
    if (link.m_numOppositeDirLanes == 0) {
        edgeA = edgeB = (float)link.m_numSameDirLanes * 0.5f;
    } else if (link.m_numSameDirLanes == 0) {
        edgeA = edgeB = (float)link.m_numOppositeDirLanes * 0.5f;
    } else {
        edgeA = (float)(direction ? link.m_numSameDirLanes : link.m_numOppositeDirLanes);
        edgeB = (float)(direction ? link.m_numOppositeDirLanes : link.m_numSameDirLanes);
        edgeA += (float)std::bit_cast<uint8>(link.m_nPathNodeWidth) * 0.011574074f;
    }

    const auto& vehPos = vehicle->GetPosition();
    const auto  AngleTo = [&](float x, float y) {
        return CGeneral::GetATanOfXY(x - vehPos.x, y - vehPos.y);
    };
    const auto angleA = AngleTo(dirY * (edgeA - 0.3f) * 5.4f + targetX, targetY - (edgeA - 0.3f) * dirX * 5.4f);
    const auto angleB = AngleTo(targetX - (edgeB - 0.3f) * dirY * 5.4f, (edgeB - 0.3f) * dirX * 5.4f + targetY);

    const auto Normalize = [](float angle) {
        while (angle > 3.1415927f) {
            angle -= 6.2831855f;
        }
        while (angle < -3.1415927f) {
            angle += 6.2831855f;
        }
        return angle;
    };
    const auto diffA = Normalize(angleA - *orientation);
    const auto diffB = Normalize(angleB - *orientation);

    // Turn only as far as needed for the heading to fall between the two lane edges
    if (diffA >= 0.0f || diffB >= 0.0f) {
        if (diffA > 0.0f && diffB > 0.0f) {
            *orientation += std::min(diffA, diffB);
        }
    } else {
        *orientation += diffB < diffA ? diffA : diffB;
    }

    while (*orientation < 0.0f) {
        *orientation += 6.2831855f;
    }
    while (*orientation > 6.2831855f) {
        *orientation -= 6.2831855f;
    }
}

// 0x431F80
CVehicle* CCarCtrl::CreateCarForScript(int32 modelid, CVector posn, bool doMissionCleanup) {
    if (CModelInfo::IsBoatModel(modelid))
    {
        auto* boat = new CBoat(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
        if (posn.z <= MAP_Z_LOW_LIMIT)
            posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

        posn.z += boat->GetDistanceFromCentreOfMassToBaseOfModel();
        boat->SetPosn(posn);

        CTheScripts::ClearSpaceForMissionEntity(posn, boat);
        boat->vehicleFlags.bEngineOn = false;
        boat->vehicleFlags.bIsLocked = true;
        boat->SetStatus(STATUS_ABANDONED);
        JoinCarWithRoadSystem(boat);

        boat->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
        boat->m_autoPilot.m_nTempAction = TEMPACT_NONE;
        boat->m_autoPilot.m_speed = 20.0F;
        boat->m_autoPilot.SetCruiseSpeed(20);

        if (doMissionCleanup)
            boat->m_bIsStaticWaitingForCollision = true;

        boat->m_autoPilot.movementFlags.bIsStopped = true;
        CWorld::Add(boat);

        if (doMissionCleanup)
            CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(boat), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

        return boat;
    }

    auto* vehicle = GetNewVehicleDependingOnCarModel(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
    if (posn.z <= MAP_Z_LOW_LIMIT)
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

    posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    vehicle->SetPosn(posn);

    if (!doMissionCleanup)
    {
        if (vehicle->IsAutomobile())
            vehicle->AsAutomobile()->PlaceOnRoadProperly();
        else if (vehicle->IsBike())
            vehicle->AsBike()->PlaceOnRoadProperly();
    }

    if (vehicle->IsTrain())
        vehicle->AsTrain()->trainFlags.bNotOnARailRoad = true;

    CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);
    vehicle->vehicleFlags.bIsLocked = true;
    vehicle->SetStatus(STATUS_ABANDONED);
    JoinCarWithRoadSystem(vehicle);
    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;

    vehicle->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    vehicle->m_autoPilot.m_speed = 13.0F;
    vehicle->m_autoPilot.SetCruiseSpeed(13);
    vehicle->m_autoPilot.m_nCurrentLane = 0;
    vehicle->m_autoPilot.m_nNextLane = 0;

    if (doMissionCleanup)
        vehicle->m_bIsStaticWaitingForCollision = true;

    CWorld::Add(vehicle);
    if (doMissionCleanup)
        CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(vehicle), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

    if (vehicle->IsSubRoadVehicle())
        vehicle->m_autoPilot.movementFlags.bIsStopped = true;

    return vehicle;
}

// 0x42C740
bool CCarCtrl::CreateConvoy(CVehicle* vehicle, int32 arg2) {
    return plugin::CallAndReturn<bool, 0x42C740, CVehicle*, int32>(vehicle, arg2);
}

// 0x42C2B0
bool CCarCtrl::CreatePoliceChase(CVehicle* vehicle, int32 arg2, CNodeAddress NodeAddress) {
    return plugin::CallAndReturn<bool, 0x42C2B0, CVehicle*, int32, CNodeAddress>(vehicle, arg2, NodeAddress);
}

// 0x428040
bool CCarCtrl::DealWithBend_Racing(CVehicle* vehicle, CCarPathLinkAddress LinkAddress1, CCarPathLinkAddress LinkAddress2, CCarPathLinkAddress LinkAddress3, CCarPathLinkAddress LinkAddress4, char arg6, char arg7, char arg8, char arg9, float arg10, float* arg11, float* arg12, float* arg13, float* arg14, CVector* pos) {
    return plugin::CallAndReturn<bool, 0x428040, CVehicle*, CCarPathLinkAddress, CCarPathLinkAddress, CCarPathLinkAddress, CCarPathLinkAddress, int8, int8, int8, int8, float, float*, float*, float*, float*, CVector*>(vehicle, LinkAddress1, LinkAddress2, LinkAddress3, LinkAddress4, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, pos);
}

// 0x42EC90
void CCarCtrl::DragCarToPoint(CVehicle* vehicle, CVector* pos) {
    plugin::Call<0x42EC90, CVehicle*, CVector*>(vehicle, pos);
}

// 0x4325C0
float CCarCtrl::FindAngleToWeaveThroughTraffic(CVehicle* vehicle, CPhysical* physical, float arg3, float arg4, float arg5) {
    static_assert(offsetof(CVehicle, m_autoPilot) + offsetof(CAutoPilot, m_nCarDrivingStyle) == 0x3B9);
    // arg3 is the heading the vehicle has, arg4 the one it wants to go in and arg5 scales how far ahead to look
    const auto& moveSpeed = vehicle->GetMoveSpeed();
    const auto  reach     = std::min(std::sqrt(moveSpeed.y * moveSpeed.y + moveSpeed.x * moveSpeed.x) * 2.5f + 1.0f, 2.0f) * arg5 * 12.0f;

    const auto& pos  = vehicle->GetPosition();
    const auto  minX = pos.x - reach;
    const auto  maxX = reach + pos.x;
    const auto  minY = pos.y - reach;
    const auto  maxY = reach + pos.y;

    const auto Sector = [](float coord) { return (int32)std::floor(coord * 0.02f + 60.0f); };
    const auto startX = std::max(Sector(minX), 0);
    const auto startY = std::max(Sector(minY), 0);
    const auto endX   = std::min(Sector(maxX), 119);
    const auto endY   = std::min(Sector(maxY), 119);

    // The two headings that avoid what is around, they are narrowed down by what is in the sectors
    float avoidA = arg3;
    float avoidB = arg3;

    CWorld::AdvanceCurrentScanCode();

    // Go over the sectors until the result stops changing
    float lastA = -9999.9f;
    float lastB = -9999.9f;
    while (lastA != avoidA || lastB != avoidB) {
        lastA = avoidA;
        lastB = avoidB;
        for (auto y = startY; y <= endY; y++) {
            for (auto x = startX; x <= endX; x++) {
                auto& sector = CWorld::GetRepeatSector(x, y);
                WeaveThroughCarsSectorList(sector.Vehicles, vehicle, physical, minX, minY, maxX, maxY, &avoidA, &avoidB);
                if (vehicle->m_autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS) {
                    WeaveThroughPedsSectorList(sector.Peds, vehicle, physical, minX, minY, maxX, maxY, &avoidA, &avoidB);
                }
                WeaveThroughObjectsSectorList(sector.Objects, vehicle, minX, minY, maxX, maxY, &avoidA, &avoidB);
                // The original goes over the vehicles one more time here, with the same scan code that does nothing
            }
        }
    }

    // Everything is in the range of -PI to PI
    const auto Wrap = [](float angle) {
        while (angle < -PI) {
            angle += TWO_PI;
        }
        while (angle > PI) {
            angle -= TWO_PI;
        }
        return angle;
    };

    const auto middle = Wrap(Wrap(arg4 - arg3) * 0.5f + arg3);
    const auto diffA  = std::abs(Wrap(avoidA - middle));
    const auto diffB  = std::abs(Wrap(avoidB - middle));

    if (diffA > HALF_PI && diffB > HALF_PI) {
        return middle;
    }
    if (std::abs(diffA - diffB) >= 0.08f && diffA < diffB) {
        return avoidA;
    }
    return avoidB;
}

// 0x4226F0
void CCarCtrl::FindIntersection2Lines(float arg1, float arg2, float arg3, float arg4, float arg5, float arg6, float arg7, float arg8, float* arg9, float* arg10) {
    // The first line is (arg1, arg2) + t * (arg3, arg4), the second one (arg5, arg6) + u * (arg7, arg8)
    const auto denominator = arg3 * arg8 - arg4 * arg7;
    const auto t = denominator == 0.0f
        ? 0.0f
        : ((arg5 - arg1) * arg8 - (arg6 - arg2) * arg7) / denominator;
    *arg9  = arg3 * t + arg1;
    *arg10 = t * arg4 + arg2;
}

// 0x42B470
void CCarCtrl::FindLinksToGoWithTheseNodes(CVehicle* vehicle) {
    static_assert(offsetof(CAutoPilot, m_nCurrentPathNodeInfo) == 0x14 && offsetof(CAutoPilot, m_nNextPathNodeInfo) == 0x16);
    static_assert(offsetof(CAutoPilot, _smthCurr) == 0x25 && offsetof(CAutoPilot, _smthNext) == 0x26);
    static_assert(offsetof(CVehicle, m_nForcedRandomRouteSeed) == 0x45E);

    auto& autoPilot = vehicle->m_autoPilot;

    if (vehicle->m_nForcedRandomRouteSeed) {
        std::srand((unsigned)(int32)vehicle->m_nForcedRandomRouteSeed);
    }

    const auto current = autoPilot.m_currentAddress;
    const auto start   = autoPilot.m_startingRouteNode;
    const auto& node   = *ThePaths.GetPathNode(current);
    const auto  base   = node.m_wBaseLinkId;

    // Nodes are ordered by area first and then by their index in it
    const auto IsBefore = [](CNodeAddress a, CNodeAddress b) {
        return a.m_wAreaId < b.m_wAreaId || (a.m_wAreaId == b.m_wAreaId && a.m_wNodeId < b.m_wNodeId);
    };

    // The link that leads to the node the vehicle is heading to
    int32 toStartIdx = 0;
    do {
        if (ThePaths.m_pNodeLinks[current.m_wAreaId][base + toStartIdx] == start) {
            break;
        }
        toStartIdx++;
    } while (toStartIdx < 12);
    autoPilot.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[current.m_wAreaId][base + toStartIdx];
    autoPilot._smthNext           = IsBefore(current, start) ? -1 : 1;

    // The link to continue with, the one pointing the closest to where the vehicle is
    int32 chosen = 0;
    if (node.m_nNumLinks != 1) {
        chosen     = -1;
        auto best  = 999999.9f;
        for (auto i = 0u; i < node.m_nNumLinks; i++) {
            const auto link = ThePaths.m_pNodeLinks[current.m_wAreaId][base + i];
            if (link == start || !ThePaths.IsAreaLoaded(link.m_wAreaId)) {
                continue;
            }
            const auto fromPos = node.GetPosition();
            const auto toPos   = ThePaths.GetPathNode(link)->GetPosition();
            if (const auto dist = CCollision::DistToLine(fromPos, toPos, vehicle->GetPosition()); dist < best) {
                best   = dist;
                chosen = i;
            }
        }
        if (chosen < 0) {
            chosen = 0;
        }
    }

    autoPilot.m_nCurrentPathNodeInfo = ThePaths.m_pNaviLinks[current.m_wAreaId][base + chosen];
    autoPilot._smthCurr              = IsBefore(ThePaths.m_pNodeLinks[current.m_wAreaId][base + chosen], current) ? -1 : 1;
}

// 0x434400
float CCarCtrl::FindMaximumSpeedForThisCarInTraffic(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    const auto cruiseSpeed = (float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult;
    switch (autoPilot.m_nCarDrivingStyle) {
    case DRIVING_STYLE_AVOID_CARS:
    case DRIVING_STYLE_PLOUGH_THROUGH:
    case DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_OBEYLIGHTS:
        return cruiseSpeed;
    default:
        break;
    }

    // Look at what is around the vehicle, each of those can slow it down
    const auto& pos  = vehicle->GetPosition();
    const auto  minX = pos.x - 14.0f;
    const auto  minY = pos.y - 14.0f;
    const auto  maxX = pos.x + 14.0f;
    const auto  maxY = pos.y + 14.0f;

    auto speed = cruiseSpeed;
    ForEachRepeatSectorInRect(minX, minY, maxX, maxY, [&](CRepeatSector& sector) {
        const auto cruiseNow = (float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult;
        if (autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS) {
            SlowCarDownForCarsSectorList(sector.Vehicles, vehicle, minX, minY, maxX, maxY, &speed, cruiseNow);
        }
        SlowCarDownForPedsSectorList(sector.Peds, vehicle, minX, minY, maxX, maxY, &speed, (float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult);
        SlowCarDownForObjectsSectorList(sector.Objects, vehicle, minX, minY, maxX, maxY, &speed, (float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult);
    });

    vehicle->vehicleFlags.bWarnedPeds = true;

    if (autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_STOP_FOR_CARS && autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS) {
        return ((float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult + speed) * 0.5f;
    }
    return speed;
}

// 0x42BD20
void CCarCtrl::FindNodesThisCarIsNearestTo(CVehicle* vehicle, CNodeAddress& nodeAddress1, CNodeAddress& nodeAddress2) {
    plugin::Call<0x42BD20, CVehicle*, CNodeAddress&, CNodeAddress&>(vehicle, nodeAddress1, nodeAddress2);
}

// 0x422090
int8 CCarCtrl::FindPathDirection(CNodeAddress nodeAddress1, CNodeAddress nodeAddress2, CNodeAddress nodeAddress3, bool* arg4) {
    *arg4 = false;

    for (const auto& addr : { nodeAddress1, nodeAddress2, nodeAddress3 }) {
        if (!addr.IsAreaValid() || !ThePaths.IsAreaLoaded(addr.m_wAreaId)) {
            return 0;
        }
    }

    const CVector2D p1{ ThePaths.GetPathNode(nodeAddress1)->GetPosition() };
    const CVector2D p2{ ThePaths.GetPathNode(nodeAddress2)->GetPosition() };
    const CVector2D p3{ ThePaths.GetPathNode(nodeAddress3)->GetPosition() };

    const auto len12 = (p2 - p1).Magnitude();
    if (len12 == 0.0f) {
        return 0;
    }
    const auto dir12 = (p2 - p1) * (1.0f / len12);

    const auto len23 = (p3 - p2).Magnitude();
    if (len23 == 0.0f) {
        return 0;
    }
    const auto dir23 = (p3 - p2) * (1.0f / len23);

    const auto cross = dir12.x * dir23.y - dir23.x * dir12.y;
    const auto dot   = dir23.x * dir12.x + dir23.y * dir12.y;

    // 1: nearly straight, 2 or 4: bends to either side. The last one also tells if it's a (nearly) U turn
    if (dot > 0.4f) {
        return 1;
    }
    if (dot < -0.3f) {
        *arg4 = true;
    }
    return cross > 0.0f ? 4 : 2;
}

// 0x422620
float CCarCtrl::FindPercDependingOnDistToLink(CVehicle* vehicle, CCarPathLinkAddress linkAddress) {
    const auto  linkPos = CVector2D{ ThePaths.GetCarPathLink(linkAddress).m_posn };
    const float dist    = (linkPos - CVector2D{ vehicle->GetPosition() }).Magnitude();
    if (dist < 5.0f) {
        return 0.5f;
    }
    if (dist >= 15.0f) {
        return 1.0f;
    }
    return (dist - 5.0f) * 0.05f + 0.5f;
}

// 0x421770
int32 CCarCtrl::FindSequenceElement(int32 arg1) {
    if (bSequenceOtherWay) {
        return (arg1 + SequenceRandomOffset) % SequenceElements;
    }
    return (SequenceElements - arg1 + SequenceRandomOffset) % SequenceElements;
}

// 0x4224E0
float CCarCtrl::FindSpeedMultiplier(float arg1, float arg2, float arg3, float arg4) {
    // arg1 is an angle, the multiplier goes from 1 at `arg2` to `arg4` at `arg3`
    constexpr float PI_F = std::numbers::pi_v<float>;
    while (arg1 < -PI_F) {
        arg1 += 2.0f * PI_F;
    }
    while (arg1 > PI_F) {
        arg1 -= 2.0f * PI_F;
    }
    if (arg1 < 0.0f) {
        arg1 = -arg1;
    }
    arg1 -= arg2;
    if (arg1 < 0.0f) {
        arg1 = 0.0f;
    }
    if (arg3 - arg2 < arg1) {
        return arg4;
    }
    return 1.0f - (1.0f - arg4) * (arg1 / (arg3 - arg2));
}

// 0x424130
float CCarCtrl::FindSpeedMultiplierWithSpeedFromNodes(int8 arg1) {
    switch (arg1) {
    case -1: return 0.5f;
    case 0:  return 0.65f;
    case 2:  return 2.3f;
    default: return 1.0f;
    }
}

// 0x422370
float CCarCtrl::FindGhostRoadHeight(CVehicle* vehicle) {
    const auto& autoPilot = vehicle->m_autoPilot;
    const auto  from      = autoPilot.m_currentAddress;
    const auto  to        = autoPilot.m_startingRouteNode;
    if (!from.IsAreaValid() || !to.IsAreaValid()) {
        return 0.0f;
    }
    if (!ThePaths.IsAreaLoaded(from.m_wAreaId) || !ThePaths.IsAreaLoaded(to.m_wAreaId)) {
        return 0.0f;
    }

    // Blend between the heights of the 2 nodes, the closer one counts more
    const auto  fromPos  = ThePaths.GetPathNode(from)->GetPosition();
    const auto  toPos    = ThePaths.GetPathNode(to)->GetPosition();
    const auto& pos      = vehicle->GetPosition();
    const auto  distFrom = (CVector2D{ fromPos } - CVector2D{ pos }).Magnitude();
    const auto  distTo   = (CVector2D{ toPos } - CVector2D{ pos }).Magnitude();
    return (distTo * fromPos.z + distFrom * toPos.z) / (distTo + distFrom);
}

// 0x42B270
void CCarCtrl::FireHeliRocketsAtTarget(CAutomobile* entityLauncher, CEntity* entity) {
    static_assert(offsetof(CVehicle, m_nVehicleWeaponInUse) == 0x513);

    if (entityLauncher->m_nVehicleWeaponInUse != CAR_WEAPON_DOUBLE_ROCKET && entityLauncher->m_nVehicleWeaponInUse != CAR_WEAPON_NOT_USED) {
        return;
    }

    // One rocket every 250 ms, alternating sides
    const auto slot = CTimer::GetTimeInMS() / 250;
    if (slot == CTimer::GetPreviousTimeInMS() / 250) {
        return;
    }

    const auto& pos    = entityLauncher->GetPosition();
    const auto& matrix = entityLauncher->GetMatrix();
    if ((pos - entity->GetPosition()).Magnitude() >= 80.0f) {
        return;
    }

    // Only fire when the target is almost in front of the nose
    const auto aimEnd = pos + matrix.GetForward();
    if (CCollision::DistToMathematicalLine(&pos, &aimEnd, &entity->GetPosition()) >= 7.0f) {
        return;
    }

    auto sideOffset = matrix.GetRight() * 1.5f;
    if (slot & 1) {
        sideOffset = -sideOffset;
    }
    const auto forward = matrix.GetForward();
    CProjectileInfo::AddProjectile(entityLauncher, WEAPON_ROCKET, sideOffset + forward * 4.0f + pos, 1.0f, &forward, nullptr);
}

// 0x429A70
void CCarCtrl::FlyAIHeliInCertainDirection(CHeli* heli, float arg2, float arg3, bool arg4) {
    plugin::Call<0x429A70, CHeli*, float, float, bool>(heli, arg2, arg3, arg4);
}

// 0x423940
void CCarCtrl::FlyAIHeliToTarget_FixedOrientation(CHeli* heli, float Orientation, CVector posn, bool arg4) {
    plugin::Call<0x423940, CHeli*, float, CVector, bool>(heli, Orientation, posn, arg4);
}

// 0x423000
void CCarCtrl::FlyAIPlaneInCertainDirection(CPlane* pPlane) {
    plugin::Call<0x423000, CPlane*>(pPlane);
}

// 0x424210
bool CCarCtrl::GenerateCarCreationCoors2(CVector posn, float radius, float arg3, float arg4, bool arg5, float arg6, float arg7, CVector* pOrigin, CNodeAddress* pNodeAddress1, CNodeAddress* pNodeAddress12, float* arg11, bool arg12, bool arg13) {
    return plugin::CallAndReturn<bool, 0x424210, CVector, float, float, float, bool, float, float, CVector*, CNodeAddress*, CNodeAddress*, float*, bool, bool>(posn, radius, arg3, arg4, arg5, arg6, arg7, pOrigin, pNodeAddress1, pNodeAddress12, arg11, arg12, arg13);
}

// 0x42F9C0
void CCarCtrl::GenerateEmergencyServicesCar() {
    static_assert(offsetof(CWanted, m_WantedLevel) == 0x2C);

    if (!bAllowEmergencyServicesToBeCreated || CGangWars::GangWarFightingGoingOn()) {
        return;
    }

    // Not while the police is after the player, or inside of an interior
    const auto* const wanted = FindPlayerWanted();
    if (!wanted || (int32)wanted->GetWantedLevel() > 3 || CGame::currArea != AREA_CODE_NORMAL_WORLD || !CTheZones::m_CurrLevel) {
        return;
    }

    // There's a limit to how many cars can be around
    const auto carsInUse = (int32)NumAmbulancesOnDuty + (int32)NumFireTrucksOnDuty + (int32)NumParkedCars + NumMissionCars + (int32)NumLawEnforcerCars + NumRandomCars;
    if (carsInUse > (int32)MaxNumberOfCarsInUse) {
        return;
    }

    if (!NumAmbulancesOnDuty) {
        auto* const accidents = CAccidentManager::GetInstance();
        if (accidents->GetNumberOfFreeAccidents() < 2) {
            CStreaming::StreamAmbulanceAndMedic(false);
        } else {
            auto playerPos = FindPlayerCoors();
            if (auto* const accident = accidents->GetNearestFreeAccident(playerPos, false)) {
                if (CStreaming::StreamAmbulanceAndMedic(true) && CTimer::GetTimeInMS() > (uint32)LastTimeAmbulanceCreated + 30'000) {
                    const auto posn = accident->m_pPed->GetPosition();
                    if (GenerateOneEmergencyServicesCar(CStreaming::ms_aDefaultAmbulanceModel[CTheZones::m_CurrLevel], posn)) {
                        LastTimeAmbulanceCreated = CTimer::GetTimeInMS();
                    }
                }
            }
        }
    }

    if (!NumFireTrucksOnDuty) {
        if (gFireManager.GetNumOfNonScriptFires() < 3) {
            CStreaming::StreamFireEngineAndFireman(false);
            return;
        }
        if (auto* const fire = gFireManager.FindNearestFire(FindPlayerCoors(), true, true)) {
            if (CStreaming::StreamFireEngineAndFireman(true) && CTimer::GetTimeInMS() > (uint32)LastTimeFireTruckCreated + 35'000) {
                if (GenerateOneEmergencyServicesCar(CStreaming::ms_aDefaultFireEngineModel[CTheZones::m_CurrLevel], fire->GetPosition())) {
                    LastTimeFireTruckCreated = CTimer::GetTimeInMS();
                }
            }
        }
    }
}

// 0x42B7D0
CAutomobile* CCarCtrl::GenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    CNodeAddress node1{}, node2{};
    CVector      origin{};
    float        nodeLerp{};

    const auto& centre = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);

    // Find a spot on the road that is on land and not blocked by anything
    auto numTries = 0;
    auto found    = false;
    do {
        if (numTries > 4) {
            return nullptr;
        }
        if (GenerateCarCreationCoors2(centre, 0.70710677f, 0.70710677f, -1.0f, true, 160.0f, 160.0f, &origin, &node1, &node2, &nodeLerp, false, false)
            && !ThePaths.GetPathNode(node1)->m_bWaterNode
        ) {
            int16 numColliding{};
            CWorld::FindObjectsKindaColliding(origin, 10.0f, true, &numColliding, 2, nullptr, false, true, true, false, false);
            found = numColliding == 0;
        }
        numTries++;
    } while (!found);
    if (numTries > 4) { // Also discards a spot found on the very last try
        return nullptr;
    }

    const auto veh = new CAutomobile{ (int32)modelId, RANDOM_VEHICLE, true };
    veh->SetPosn(origin);

    // Face the vehicle towards the target
    const auto toTarget = posn - origin;
    const auto len2D    = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
    float      dirX, dirY;
    if (len2D == 0.0f) {
        dirX = 1.0f;
        dirY = toTarget.y;
    } else {
        dirX = toTarget.x / len2D;
        dirY = toTarget.y / len2D;
    }
    auto& mat = veh->GetMatrix();
    mat.GetForward() = CVector{ dirX, dirY, 0.0f };
    mat.GetRight()   = CVector{ dirY, -dirX, 0.0f };
    mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };

    // The ground must be close to where the path nodes are, pick the closest of the one above and below the spot
    const auto expectedZ = (float)ThePaths.GetPathNode(node2)->m_vPos.z * nodeLerp + (1.0f - nodeLerp) * (float)ThePaths.GetPathNode(node1)->m_vPos.z;
    auto       groundZ   = 1e9f;
    CColPoint  colPoint;
    CEntity*   hitEntity{};
    if (CWorld::ProcessVerticalLine(origin, 1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        groundZ = colPoint.m_vecPoint.z;
    }
    if (CWorld::ProcessVerticalLine(origin, -1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        if (std::abs(colPoint.m_vecPoint.z - expectedZ) < std::abs(groundZ - expectedZ)) {
            groundZ = colPoint.m_vecPoint.z;
        }
    }
    if (groundZ == 1e9f) { // No ground found
        delete veh;
        return nullptr;
    }

    veh->SetPosn(origin.x, origin.y, veh->GetDistanceFromCentreOfMassToBaseOfModel() + groundZ);
    veh->m_vecMoveSpeed = CVector{};
    veh->PlaceOnRoadProperly();
    CWorld::Add(veh);

    switch (modelId) {
    case MODEL_FIRETRUK:
        veh->vehicleFlags.bIsFireTruckOnDuty = true;
        NumFireTrucksOnDuty++;
        CCarAI::AddFiretruckOccupants(veh);
        break;
    case MODEL_AMBULAN:
        veh->vehicleFlags.bIsAmbulanceOnDuty = true;
        NumAmbulancesOnDuty++;
        CCarAI::AddAmbulanceOccupants(veh);
        break;
    case MODEL_ENFORCER:
    case MODEL_COPBIKE:
    case MODEL_COPCARLA:
    case MODEL_COPCARSF:
    case MODEL_COPCARVG:
    case MODEL_COPCARRU:
        CCarAI::AddPoliceCarOccupants(veh, false);
        break;
    default:
        return veh;
    }
    veh->vehicleFlags.bSirenOrAlarm = true;
    return veh;
}

// 0x430050
void CCarCtrl::GenerateOneRandomCar() {
    plugin::Call<0x430050>();
}

// 0x4341C0
void CCarCtrl::GenerateRandomCars() {
    if (CCutsceneMgr::ms_running) {
        CountDownToCarsAtStart = 2;
        return;
    }
    if (CGangWars::DontCreateCivilians() || !CGame::CanSeeOutSideFromCurrArea()) {
        return;
    }

    if (CGameLogic::LaRiotsActiveHere() && TimeNextMadDriverChaseCreated > 480.0f) {
        TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(240.0f, 480.0f);
    }
    TimeNextMadDriverChaseCreated -= (CTimer::GetTimeStep() * 0.02f);

    if (NumRandomCars < 45) {
        if (CountDownToCarsAtStart) {
            CountDownToCarsAtStart--;
            for (auto i = 100; i --> 0;) {
                GenerateOneRandomCar();
            }
            CTheCarGenerators::GenerateEvenIfPlayerIsCloseCounter = 20;
        } else {
            GenerateOneRandomCar();
            GenerateOneRandomCar();
        }
    }

    CTrain::DoTrainGenerationAndRemoval();
    CPlane::DoPlaneGenerationAndRemoval();

    // Only looked at when the time moves into a different 4096 ms block
    if ((CTimer::GetPreviousTimeInMS() ^ CTimer::GetTimeInMS()) & 0xFFFFF000) {
        GenerateEmergencyServicesCar();
    }
}

// 0x42F3C0
void CCarCtrl::GetAIHeliToAttackPlayer(CAutomobile* automobile) {
    auto* const heli      = static_cast<CHeli*>(automobile);
    const auto  playerPos = FindPlayerCoors();
    const auto  toPlayer  = CVector2D{ playerPos } - CVector2D{ heli->GetPosition() };

    auto heading = CGeneral::GetATanOfXY(toPlayer.x, toPlayer.y);
    auto dist    = toPlayer.Magnitude();

    heli->m_fMaxAltitude                  = playerPos.z;
    heli->m_autoPilot.m_vecDestinationCoors = playerPos;

    // Attack from a distance, and fly away (backwards) once too close
    if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER) {
        if (dist < 15.0f) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER_FLY_AWAY;
        }
        dist += 50.0f;
    } else if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER_FLY_AWAY) {
        if (dist > 18.0f) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER;
        }
        heading += std::numbers::pi_v<float>;
    }

    FlyAIHeliInCertainDirection(heli, heading, dist, false);
    TestWhetherToFirePlaneGuns(heli, FindPlayerEntity());
    FireHeliRocketsAtTarget(heli, FindPlayerEntity());
}

// 0x42A730
void CCarCtrl::GetAIHeliToFlyInDirection(CAutomobile* automobile) {
    auto* const heli = static_cast<CHeli*>(automobile);
    FlyAIHeliInCertainDirection(heli, heli->field_9B4, 1000.0f, false);
}

// 0x429780
void CCarCtrl::GetAIPlaneToAttackPlayer(CAutomobile* automobile) {
    auto* const plane = static_cast<CPlane*>(automobile);

    // Aim a bit ahead of where the player is going
    const auto  playerSpeed = FindPlayerSpeed();
    const auto  playerPos   = FindPlayerCoors();
    const auto& pos         = plane->GetPosition();

    plane->m_planeHeading = CGeneral::GetATanOfXY(playerSpeed.x * 50.0f + playerPos.x - pos.x, playerSpeed.y * 50.0f + playerPos.y - pos.y);
    plane->m_maxAltitude  = playerSpeed.z * 50.0f + playerPos.z;
    FlyAIPlaneInCertainDirection(plane);

    if (auto* const playerVeh = FindPlayerVehicle(); playerVeh && playerVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE) {
        TriggerDogFightMoves(plane, playerVeh);
    }
    TestWhetherToFirePlaneGuns(plane, FindPlayerVehicle());
    PossiblyFireHSMissile(plane, FindPlayerVehicle());
}

// 0x429890
void CCarCtrl::GetAIPlaneToDoDogFight(CAutomobile* automobile) {
    auto* const plane  = static_cast<CPlane*>(automobile);
    auto&       ap     = plane->m_autoPilot;
    auto* const target = ap.m_TargetEntity;

    // Aim a bit ahead of where the target is going
    const CVector ahead = target->GetPosition() + target->GetMoveSpeed() * 50.0f;
    const auto&   pos   = plane->GetPosition();

    if (ap.m_bPlaneDogfightSomething) {
        // Fly past the target, to a point it picked earlier
        plane->m_planeHeading = CGeneral::GetATanOfXY(ap.m_vecDestinationCoors.x - pos.x, ap.m_vecDestinationCoors.y - pos.y);
        plane->m_maxAltitude  = ap.m_vecDestinationCoors.z;
        if ((CVector2D{ ap.m_vecDestinationCoors } - CVector2D{ pos }).Magnitude() < 50.0f) {
            ap.m_bPlaneDogfightSomething = false;
        }
    } else {
        plane->m_planeHeading = CGeneral::GetATanOfXY(ahead.x - pos.x, ahead.y - pos.y);
        plane->m_maxAltitude  = ahead.z;
        if ((CGeneral::GetRandomNumber() & 0x3FF) == 500) {
            // Every now and then pick a random point to fly to after the pass
            ap.m_bPlaneDogfightSomething = true;
            ap.m_vecDestinationCoors.x   = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 600.0f + ahead.x - 300.0f;
            ap.m_vecDestinationCoors.y   = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 600.0f + ahead.y - 300.0f;
            ap.m_vecDestinationCoors.z   = ahead.z + 50.0f;
        }
    }

    FlyAIPlaneInCertainDirection(plane);
    TestWhetherToFirePlaneGuns(plane, target);
    PossiblyFireHSMissile(plane, target);
}

// 0x42F370
void CCarCtrl::GetAIPlaneToDoDogFightAgainstPlayer(CAutomobile* automobile) {
    if (auto* const playerVeh = FindPlayerVehicle()) {
        automobile->m_autoPilot.m_TargetEntity = playerVeh;
    } else {
        automobile->m_autoPilot.m_TargetEntity = (CVehicle*)FindPlayerPed();
    }
    GetAIPlaneToDoDogFight(automobile);
}

// 0x421440
CVehicle* CCarCtrl::GetNewVehicleDependingOnCarModel(int32 modelId, uint8 createdBy) {
    switch (CModelInfo::GetModelInfo(modelId)->AsVehicleModelInfoPtr()->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK:
        return new CMonsterTruck(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_QUAD:
        return new CQuadBike(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_HELI:
        return new CHeli(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_PLANE:
        return new CPlane(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_BOAT:
        return new CBoat(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_TRAIN:
        return new CTrain(modelId, (eVehicleCreatedBy)createdBy);
    case VEHICLE_TYPE_BIKE: {
        auto* bike = new CBike(modelId, (eVehicleCreatedBy)createdBy);
        bike->bikeFlags.bOnSideStand = true;
        return bike;
    }
    case VEHICLE_TYPE_BMX: {
        auto* bmx = new CBmx(modelId, (eVehicleCreatedBy)createdBy);
        bmx->bikeFlags.bOnSideStand = true;
        return bmx;
    }
    case VEHICLE_TYPE_TRAILER:
        return new CTrailer(modelId, (eVehicleCreatedBy)createdBy);
    default:
        return new CAutomobile(modelId, (eVehicleCreatedBy)createdBy, true);
    }
}

// 0x42C250
bool CCarCtrl::IsAnyoneParking() {
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        switch (veh.m_autoPilot.m_nCarMission) {
        case eCarMission::MISSION_PARK_PARALLEL:
        case eCarMission::MISSION_PARK_PARALLEL_2:
        case eCarMission::MISSION_PARK_PERPENDICULAR:
        case eCarMission::MISSION_PARK_PERPENDICULAR_2:
            return true;
        }
    }
    return false;
}

// 0x42DAB0
bool CCarCtrl::IsThisAnAppropriateNode(CVehicle* vehicle, CNodeAddress nodeAddress1, CNodeAddress nodeAddress2, CNodeAddress nodeAddress3, bool arg5) {
    const auto areaNodes3 = ThePaths.m_pPathNodes[nodeAddress3.m_wAreaId];
    if (!areaNodes3 || nodeAddress1 == nodeAddress3) {
        return false;
    }

    // `node2` is where the vehicle is coming from, `node3` is the candidate
    const auto& node2 = ThePaths.m_pPathNodes[nodeAddress2.m_wAreaId][nodeAddress2.m_wNodeId];
    const auto& node3 = areaNodes3[nodeAddress3.m_wNodeId];

    if (node2.m_bWaterNode != node3.m_bWaterNode && vehicle->m_nModelIndex != MODEL_VORTEX) {
        return false;
    }

    const auto  driver    = vehicle->m_pDriver;
    const auto  vehClass  = [&] { return CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->AsVehicleModelInfoPtr()->m_nVehicleClass; };
    const auto  vehPos    = vehicle->GetPosition();
    const auto  IsOccupied = [&] { // Checks the things that stop a vehicle from using a roadblock / parking node
        if (vehicle->IsLawEnforcementVehicle()) {
            return true;
        }
        if (vehClass() == VEHICLE_CLASS_BIG) {
            return true;
        }
        if (driver && driver->m_nPedType == PED_TYPE_CRIMINAL) {
            return true;
        }
        return IsAnyoneParking();
    };
    const auto IsNodeFree = [&] {
        int16 numColliding{};
        CWorld::FindObjectsKindaColliding(node3.GetPosition(), 5.0f, true, &numColliding, 2, nullptr, false, true, false, false, false);
        return numColliding == 0;
    };

    switch (node3.m_nBehaviourType) {
    case 1: { // Roadblock
        if (vehicle->m_nCreatedBy == MISSION_VEHICLE) {
            return false;
        }
        if (driver && driver->IsCreatedBy(PED_MISSION)) {
            return false;
        }
        if (vehClass() == VEHICLE_CLASS_BIG) {
            return false;
        }
        if (!arg5) {
            const auto toNode = node3.GetPosition() - vehPos;
            const auto link   = node3.GetPosition() - node2.GetPosition();
            if (CrossProduct(link, toNode).z < 0.0f) {
                return false;
            }
        }
        return !IsOccupied() && IsNodeFree();
    }
    case 2: { // Parking
        if (vehicle->m_nCreatedBy == MISSION_VEHICLE) {
            return false;
        }
        if (driver && driver->IsCreatedBy(PED_MISSION)) {
            return false;
        }
        if (!arg5) {
            const auto toNode = node3.GetPosition() - vehPos;
            if (DotProduct(vehicle->GetForwardVector(), toNode) < 0.0f) {
                return false;
            }
        }
        return !IsOccupied() && IsNodeFree();
    }
    case 5:
    case 10: {
        if (arg5) {
            return false;
        }
        const auto& bb = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox();
        if (bb.m_vecMax.z < 2.0f) {
            return false;
        }
        break;
    }
    case 8:
    case 9: {
        if (arg5) {
            return false;
        }
        const auto& bb = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox();
        if (bb.m_vecMax.z > 1.5f || bb.m_vecMax.x > 2.0f || bb.m_vecMax.y > 4.0f) {
            return false;
        }
        break;
    }
    default:
        break;
    }

    if (vehicle->m_nCreatedBy == MISSION_VEHICLE
        && vehicle->m_autoPilot.m_nCarMission == MISSION_CRUISE
        && node3.m_bDontWander
        && !node2.m_bDontWander
    ) {
        return false;
    }
    if (node3.m_onDeadEnd && !node2.m_onDeadEnd) {
        return false;
    }
    if (node3.m_isSwitchedOff && !node2.m_isSwitchedOff) {
        return false;
    }
    return !arg5;
}

// 0x423EA0
bool CCarCtrl::IsThisVehicleInteresting(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            return true;
        }
    }
    return false;
}

// 0x432CB0
void CCarCtrl::JoinCarWithRoadAccordingToMission(CVehicle* vehicle) {
    switch (vehicle->m_autoPilot.m_nCarMission) {
    case MISSION_NONE:
    case MISSION_CRUISE:
    case MISSION_WAITFORDELETION:
    case MISSION_EMERGENCYVEHICLE_STOP:
    case MISSION_STOP_FOREVER:
    case MISSION_FOLLOW_RECORDED_PATH:
    case MISSION_PARK_PERPENDICULAR:
    case MISSION_PARK_PARALLEL:
    case MISSION_PARK_PERPENDICULAR_2:
    case MISSION_PARK_PARALLEL_2:
        return JoinCarWithRoadSystem(vehicle);
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP:
    case MISSION_BOAT_ATTACKPLAYER:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
    case MISSION_BLOCKPLAYER_FORWARDANDBACK:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_CLOSE:
    case MISSION_BOAT_CIRCLEPLAYER: {
        JoinCarWithRoadSystemGotoCoors(vehicle, FindPlayerCoors(-1), true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE:
    case MISSION_GOTOCOORDINATES_ACCURATE:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE:
    case MISSION_GOTOCOORDINATES_ASTHECROWSWIMS:
    case MISSION_GOTOCOORDINATES_RACING: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_vecDestinationCoors, true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_BLOCKCAR_CLOSE:
    case MISSION_BLOCKCAR_HANDBRAKESTOP:
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_FOLLOWCAR_CLOSE:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_KILLPED_CLOSE:
    case MISSION_DO_DRIVEBY_CLOSE:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_TargetEntity->GetPosition(), true, vehicle->IsSubBoat());
        break;
    }
    }
}

// 0x42F5A0
void CCarCtrl::JoinCarWithRoadSystem(CVehicle* vehicle) {
    static_assert(offsetof(CAutoPilot, m_nPathFindNodesCount) == 0x88);

    auto& autoPilot = vehicle->m_autoPilot;

    // Forget where the vehicle was heading
    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.m_currentAddress.ResetAreaId();
    autoPilot.m_startingRouteNode.ResetAreaId();
    autoPilot.m_nCurrentPathNodeInfo  = CCarPathLinkAddress{};
    autoPilot.m_nPreviousPathNodeInfo = CCarPathLinkAddress{};
    autoPilot.m_nNextPathNodeInfo     = CCarPathLinkAddress{};
    autoPilot.m_nPathFindNodesCount   = 0;

    // The node closest to the vehicle, in the direction it is facing
    const auto closest = ThePaths.FindNodeClosestToCoorsFavourDirection(vehicle->GetPosition(), PATH_TYPE_VEH, CVector2D{ vehicle->GetForward() });
    if (closest.m_wAreaId == UINT16_MAX || !ThePaths.IsAreaLoaded(closest.m_wAreaId)) {
        return;
    }
    const auto& closestNode = *ThePaths.GetPathNode(closest);

    // From there the node it connects to that is the closest
    auto best = 999999.9f;
    CNodeAddress bestAddr{};
    for (auto i = 0u; i < closestNode.m_nNumLinks; i++) {
        const auto link = ThePaths.m_pNodeLinks[closest.m_wAreaId][closestNode.m_wBaseLinkId + i];
        if (!ThePaths.IsAreaLoaded(link.m_wAreaId)) {
            continue;
        }
        const auto dist = (CVector2D{ ThePaths.GetPathNode(link)->GetPosition() } - CVector2D{ closestNode.GetPosition() }).Magnitude();
        if (dist < best) {
            best     = dist;
            bestAddr = link;
        }
    }
    if (bestAddr.m_wAreaId == UINT16_MAX) {
        return;
    }

    // The vehicle goes from the first of the two to the second, whichever way it is facing
    auto forward = CVector2D{ vehicle->GetForward() };
    if (forward.x == 0.0f && forward.y == 0.0f) {
        forward.x = 1.0f;
    }
    const auto& bestNode = *ThePaths.GetPathNode(bestAddr);
    auto        from     = bestAddr;
    auto        to       = closest;
    if ((closestNode.GetPosition().y - bestNode.GetPosition().y) * forward.y + (closestNode.GetPosition().x - bestNode.GetPosition().x) * forward.x < 0.0f) {
        std::swap(from, to);
    }

    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.m_currentAddress    = from;
    autoPilot.m_startingRouteNode = to;
    FindLinksToGoWithTheseNodes(vehicle);
    autoPilot.m_nCurrentLane = 0;
    autoPilot.m_nNextLane    = 0;
}

// 0x42F870
bool CCarCtrl::JoinCarWithRoadSystemGotoCoors(CVehicle* vehicle, const CVector& posn, bool unused, bool bIsBoat) {
    auto& autoPilot = vehicle->m_autoPilot;
    autoPilot.m_vecDestinationCoors = posn;

    // The same global is used as the invalid node, and as the node to avoid
    static auto& invalidNode = StaticRef<CNodeAddress>(0x8A5F44);
    constexpr auto MAX_DISTANCE = 999999.88f;

    auto& numNodes = *reinterpret_cast<int16*>(&autoPilot.m_nPathFindNodesCount);
    ThePaths.DoPathSearch(
        PATH_TYPE_VEH,
        vehicle->GetPosition(),
        invalidNode,
        posn,
        autoPilot.m_aPathFindNodesInfo.data(),
        numNodes,
        8,
        nullptr,
        MAX_DISTANCE,
        nullptr,
        MAX_DISTANCE,
        autoPilot.carCtrlFlags.bCantGoAgainstTraffic,
        invalidNode,
        vehicle->m_nModelIndex == 539, // Vortex (Hovercraft)
        bIsBoat
    );
    ThePaths.RemoveBadStartNode(vehicle->GetPosition(), autoPilot.m_aPathFindNodesInfo.data(), &numNodes);

    if (numNodes >= 2) {
        // Start going towards the first node
        autoPilot.m_currentAddress = autoPilot.m_aPathFindNodesInfo[0];
        autoPilot.m_endingRouteNode.ResetAreaId();
        autoPilot.RemoveOnePathNode();
        autoPilot.m_startingRouteNode = autoPilot.m_aPathFindNodesInfo[0];
        autoPilot.RemoveOnePathNode();
        FindLinksToGoWithTheseNodes(vehicle);
        autoPilot.m_nCurrentLane = 0;
        autoPilot.m_nNextLane    = 0;
        return false;
    }

    JoinCarWithRoadSystem(vehicle);
    numNodes = 0;
    return true;
}

// 0x432B10
bool CCarCtrl::PickNextNodeAccordingStrategy(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    // Remember what kind of road the next node is on (-1 for the parking like ones)
    const auto* const node = ThePaths.GetPathNode(autoPilot.m_startingRouteNode);
    autoPilot.field_41 = (char)(node->m_bNotHighway | (node->m_bHighway << 1));
    if (node->m_nBehaviourType == 8 || node->m_nBehaviourType == 9) {
        autoPilot.field_41 = (char)0xFF;
    }

    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY: {
        const auto playerPos = FindPlayerCoors();
        return PickNextNodeToChaseCar(vehicle, playerPos.x, playerPos.y, playerPos.z);
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_ACCURATE:
        return PickNextNodeToFollowPath(vehicle);
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY: {
        const auto& targetPos = autoPilot.m_TargetEntity->GetPosition();
        return PickNextNodeToChaseCar(vehicle, targetPos.x, targetPos.y, targetPos.z);
    }
    default:
        PickNextNodeRandomly(vehicle);
        return false;
    }
}

// 0x421740
void CCarCtrl::InitSequence(int32 numSequenceElements) {
    SequenceElements = numSequenceElements;
    SequenceRandomOffset = CGeneral::GetRandomNumber() % numSequenceElements;
    bSequenceOtherWay = (CGeneral::GetRandomNumber() / 4) % 2;
}

// 0x42DE80
void CCarCtrl::PickNextNodeRandomly(CVehicle* vehicle) {
    plugin::Call<0x42DE80, CVehicle*>(vehicle);
}

// 0x426EF0
bool CCarCtrl::PickNextNodeToChaseCar(CVehicle* vehicle, float destX, float destY, float destZ) {
    return plugin::CallAndReturn<bool, 0x426EF0, CVehicle*, float, float, float>(vehicle, destX, destY, destZ);
}

// 0x427740
bool CCarCtrl::PickNextNodeToFollowPath(CVehicle* vehicle) {
    return plugin::CallAndReturn<bool, 0x427740, CVehicle*>(vehicle);
}

// 0x429600
void CCarCtrl::PossiblyFireHSMissile(CVehicle* entityLauncher, CEntity* targetEntity) {
    if (!targetEntity)
        return;

    if (CTimer::GetTimeInMS() / 2000u == CTimer::GetPreviousTimeInMS() / 2000u)
        return;

    const CVector launcherPos = entityLauncher->GetPosition();
    const CVector targetPos = targetEntity->GetPosition();
    CVector dir = targetPos - launcherPos;
    const float dist = dir.Magnitude();
    if (dist < 160.0f && dist > 30.0f) {
        CMatrix launcherMat = entityLauncher->GetMatrix();
        CVector dirNormalized = dir;
        dir.Normalise();
        if (DotProduct(launcherMat.GetForward(), dirNormalized) > 0.8f) {
            CProjectileInfo::AddProjectile(
                entityLauncher,
                eWeaponType::WEAPON_ROCKET_HS,
                launcherPos + launcherMat.GetForward() * 4.0f - launcherMat.GetUp() * 3.0f,
                1.0f,
                &entityLauncher->GetMatrix().GetForward(),
                targetEntity
            );
        }
    }
}

// 0x424F80
void CCarCtrl::PossiblyRemoveVehicle(CVehicle* vehicle) {
    plugin::Call<0x424F80, CVehicle*>(vehicle);
}

// 0x423F10
void CCarCtrl::PruneVehiclesOfInterest() {
    ZoneScoped;

    if ((CTimer::GetFrameCounter() % 64) == 19 && FindPlayerCoors(-1).z < 950.0f) {
        for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
            if (apCarsToKeep[i]) {
                if (CTimer::GetTimeInMS() > aCarsToKeepTime[i] + 180000) {
                    apCarsToKeep[i] = nullptr;
                }
            }
        }
    }
}

// 0x42FC40
void CCarCtrl::ReconsiderRoute(CVehicle* vehicle) {
    static_assert(offsetof(CAutoPilot, m_ucCarMissionModeCounter) == 0x35);

    auto& autoPilot = vehicle->m_autoPilot;

    // Only every two seconds, staggered by the vehicle's seed
    const auto seed = (uint32)vehicle->m_nRandomSeed;
    if ((seed + CTimer::GetTimeInMS()) / 2000 == (seed + CTimer::GetPreviousTimeInMS()) / 2000) {
        return;
    }

    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_GOTOCOORDINATES:
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
        break;
    default:
        autoPilot.m_ucCarMissionModeCounter = 0;
        return;
    }

    CNodeAddress nodeA, nodeB;
    FindNodesThisCarIsNearestTo(vehicle, nodeA, nodeB);
    if (nodeA.m_wAreaId == UINT16_MAX) {
        return;
    }

    // Nothing to do while the car is already on the route towards these nodes
    const auto& current = autoPilot.m_currentAddress;
    const auto& start   = autoPilot.m_startingRouteNode;
    const auto& end     = autoPilot.m_endingRouteNode;
    if ((current == nodeA && start == nodeB)
        || (current == nodeB && start == nodeA)
        || (end == nodeA && current == nodeB)
        || (end == nodeB && current == nodeA)
        || current == nodeB
        || end == nodeB) {
        autoPilot.m_ucCarMissionModeCounter = 0;
        return;
    }

    // The route has to be wrong for a few checks in a row before doing anything about it
    if (++autoPilot.m_ucCarMissionModeCounter <= 4) {
        return;
    }

    CVector target;
    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
        target = FindPlayerCoors();
        break;
    case MISSION_GOTOCOORDINATES:
        target = autoPilot.m_vecDestinationCoors;
        break;
    default:
        target = autoPilot.m_TargetEntity->GetPosition();
        break;
    }

    const auto  oneSideOnly           = autoPilot.carCtrlFlags.bCantGoAgainstTraffic;
    const auto  includeNodesNoLinks   = vehicle->m_nModelIndex == MODEL_VORTEX;
    constexpr auto MAX_SEARCH_DISTANCE = 999999.875f;

    // First only measure the new route, and only take it when it is reasonable
    int16 nodesCount = 0;
    float distance   = 0.0f;
    ThePaths.DoPathSearch(
        PATH_TYPE_VEH, vehicle->GetPosition(), nodeB, target, nullptr, nodesCount, 0, &distance,
        MAX_SEARCH_DISTANCE, nullptr, MAX_SEARCH_DISTANCE, oneSideOnly, current, includeNodesNoLinks, false
    );
    if (distance < 90000.0f && nodesCount >= 2) {
        autoPilot.m_currentAddress      = nodeA;
        autoPilot.m_startingRouteNode   = nodeB;
        FindLinksToGoWithTheseNodes(vehicle);

        ThePaths.DoPathSearch(
            PATH_TYPE_VEH, vehicle->GetPosition(), nodeB, target, autoPilot.m_aPathFindNodesInfo.data(), reinterpret_cast<int16&>(autoPilot.m_nPathFindNodesCount), 8, nullptr,
            MAX_SEARCH_DISTANCE, nullptr, MAX_SEARCH_DISTANCE, oneSideOnly, autoPilot.m_currentAddress, includeNodesNoLinks, false
        );
        autoPilot.RemoveOnePathNode();
    }
    autoPilot.m_ucCarMissionModeCounter = 0;
}

// 0x423DE0
void CCarCtrl::RegisterVehicleOfInterest(CVehicle* vehicle) {
    const auto now = CTimer::GetTimeInMS();

    // Already in the list, only refresh the time
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (apCarsToKeep[i] == vehicle) {
            aCarsToKeepTime[i] = now;
            return;
        }
    }

    // A free slot
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (!apCarsToKeep[i]) {
            apCarsToKeep[i]    = vehicle;
            aCarsToKeepTime[i] = now;
            return;
        }
    }

    // Replace the one that was registered the longest time ago
    size_t oldest     = 0;
    auto   oldestTime = std::numeric_limits<uint32>::max();
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (apCarsToKeep[i] && aCarsToKeepTime[i] < oldestTime) {
            oldest     = i;
            oldestTime = aCarsToKeepTime[i];
        }
    }
    apCarsToKeep[oldest]    = vehicle;
    aCarsToKeepTime[oldest] = now;
}

// 0x4322B0
void CCarCtrl::RemoveCarsIfThePoolGetsFull() {
    ZoneScoped;

    if (CTimer::GetFrameCounter() % 8 != 3)
        return;

    if (GetVehiclePool()->GetNoOfFreeSpaces() >= 8)
        return;

    // Find closest deletable vehicle
    const CVector camPos = TheCamera.GetPosition();
    float fClosestDist = std::numeric_limits<float>::max();
    CVehicle* closestVeh = nullptr;
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (IsThisVehicleInteresting(&veh)) {
            continue;
        }
        if (veh.vehicleFlags.bIsLocked) {
            continue;
        }
        if (!veh.CanBeDeleted()) {
            continue;
        }
        if (CCranes::IsThisCarBeingTargettedByAnyCrane(&veh)) {
            continue;
        }

        const float fCamVehDist = (camPos - veh.GetPosition()).Magnitude();
        if (fClosestDist > fCamVehDist) {
            fClosestDist = fCamVehDist;
            closestVeh   = &veh;
        }
    }
    if (closestVeh) {
        CWorld::Remove(closestVeh);
        delete closestVeh;
    }
}

// 0x42CD10
void CCarCtrl::RemoveDistantCars() {
    ZoneScoped;

    // FIXBUGS: First remove vehicles that can be removed
    if (notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
        for (auto& veh : GetVehiclePool()->GetAllValid()) {
            PossiblyRemoveVehicle(&veh);
        }
    }

    //... only then process them, this way we don't do use-after-free
    // only other solution would be `PossiblyRemoveVehicle` returning a `bool`
    // to indicate whenever the vehicle was deleted or not.
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
            PossiblyRemoveVehicle(&veh); // This may or may not invalidate `veh`
        }
        if (!veh.vehicleFlags.bCreateRoadBlockPeds) {
            continue;
        }
        if (DistanceBetweenPoints(FindPlayerCentreOfWorld(), veh.GetPosition()) >= 54.5f) {
            continue;
        }
        CRoadBlocks::GenerateRoadBlockPedsForCar(
            &veh,
            veh.m_nPedsPositionForRoadBlock,
            veh.IsLawEnforcementVehicle() ? PED_TYPE_COP : PED_TYPE_GANG1
        );
        veh.vehicleFlags.bCreateRoadBlockPeds = false;
    }
}

// 0x423ED0
void CCarCtrl::RemoveFromInterestingVehicleList(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            car = nullptr;
            break;
        }
    }
}

// 0x42CE40
void CCarCtrl::ScanForPedDanger(CVehicle* vehicle) {
    // The player's vehicle warns peds from further away
    const auto range = vehicle == FindPlayerVehicle() ? 44.0f : 11.0f;
    const auto honk  = vehicle->m_autoPilot.carCtrlFlags.bHonkAtPed;

    const auto& pos  = vehicle->GetPosition();
    const auto  minX = pos.x - range;
    const auto  minY = pos.y - range;
    const auto  maxX = pos.x + range;
    const auto  maxY = pos.y + range;

    auto speed = (float)vehicle->m_autoPilot.m_nCruiseSpeed;
    ForEachRepeatSectorInRect(minX, minY, maxX, maxY, [&](CRepeatSector& sector) {
        SlowCarDownForPedsSectorList(sector.Peds, vehicle, minX, minY, maxX, maxY, &speed, (float)vehicle->m_autoPilot.m_nCruiseSpeed);
    });

    vehicle->vehicleFlags.bWarnedPeds            = true;
    vehicle->m_autoPilot.carCtrlFlags.bHonkAtPed = honk;
}

// 0x42FBC0
bool CCarCtrl::ScriptGenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    if (CStreaming::IsModelLoaded(modelId)) {
        if (auto pAuto = GenerateOneEmergencyServicesCar(modelId, posn)) {
            pAuto->m_autoPilot.m_vecDestinationCoors = posn;
            pAuto->m_autoPilot.SetCarMission(JoinCarWithRoadSystemGotoCoors(pAuto, posn, false, false) ? MISSION_GOTOCOORDINATES_STRAIGHTLINE : MISSION_GOTOCOORDINATES);
            return true;
        }
    }
    return false;
}

// 0x4342A0
void CCarCtrl::SetCoordsOfScriptCar(CVehicle* vehicle, float x, float y, float z, uint8 arg5, uint8 arg6) {
    plugin::Call<0x4342A0, CVehicle*, float, float, float, uint8, uint8>(vehicle, x, y, z, arg5, arg6);
}

// 0x4217C0
void CCarCtrl::SetUpDriverAndPassengersForVehicle(CVehicle* vehicle, int32 arg2, int32 arg3, bool arg4, bool arg5, int32 passengersNum) {
    // Gang members (pedType 14 to 23) sometimes hold a cigarette
    const auto MaybeGiveCigarette = [&](CPed* ped) {
        if (ped && arg2 >= 14 && arg2 <= 23 && CGeneral::GetRandomNumber() < 0x3FFF) {
            ped->GiveObjectToPedToHold(static_cast<uint16>(ModelIndices::MI_GANG_SMOKE), 1);
        }
    };

    vehicle->SetUpDriver(arg2, arg4, arg5);
    MaybeGiveCigarette(vehicle->m_pDriver);

    // arg3 is the least number of passengers, every seat up to the limit has a chance to get one more
    const int32 maxPassengers = std::min<int32>(vehicle->m_nMaxPassengers, passengersNum);
    int32       numPassengers = arg3;
    if (numPassengers < maxPassengers) {
        for (int32 i = 0; i < maxPassengers - arg3; i++) {
            numPassengers += static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL < 0.125f ? 1 : 0;
        }
        numPassengers = std::min(numPassengers, maxPassengers);
    } else {
        numPassengers = maxPassengers;
    }

    // Vans only get one passenger
    if (CModelInfo::IsCarModel(vehicle->m_nModelIndex)
        && CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetAnimFileIndex() == CAnimManager::GetAnimationBlockIndex("van")
        && numPassengers >= 1
    ) {
        numPassengers = 1;
    }

    for (int32 seat = 0; seat < numPassengers; seat++) {
        if (auto* const passenger = vehicle->SetupPassenger(seat, arg2, arg4, arg5)) {
            passenger->UpdateStatEnteringVehicle();
            MaybeGiveCigarette(passenger);
        }
    }
}

// 0x432420
template<typename PtrListType>
void CCarCtrl::SlowCarDownForCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, float arg3, float arg4, float arg5, float arg6, float* arg7, float arg8) {
    plugin::Call<0x432420, PtrListType&, CVehicle*, float, float, float, float, float*, float>(ptrList, vehicle, arg3, arg4, arg5, arg6, arg7, arg8);
}

// 0x426220
void CCarCtrl::SlowCarDownForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float arg4) {
    const CVector entityDir = entity->GetPosition() - vehicle->GetPosition();
    const float entityHeading = DotProduct(entityDir, vehicle->GetMatrix().GetForward());
    if (entityHeading > 0.0f && entityHeading < 20.0f) {
        if (entity->GetColModel()->GetBoundRadius() + vehicle->GetColModel()->GetBoundingBox().m_vecMax.x > fabs(DotProduct(entityDir, vehicle->GetMatrix().GetRight()))) {
            if (entityHeading >= 7.0f) {
                *arg3 = std::min(*arg3, (1.0f - (entityHeading - 7.0f) / 13.0f)) * arg4; // Original code multiplies by 0.07692308, which is the recp. of 13
            } else {
                *arg3 = 0.0f;
            }
        }
    }
}

// 0x42D4F0
template<typename PtrListType>
void CCarCtrl::SlowCarDownForObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float arg3, float arg4, float arg5, float arg6, float* arg7, float arg8) {
    for (typename PtrListType::NodeType *node = ptrList.GetNode(), *next{}; node; node = next) {
        next = node->Next;

        auto* const object = static_cast<CObject*>(node->Item);
        if (object->IsScanCodeCurrent()) {
            continue;
        }
        object->SetCurrentScanCode();

        // Only the road blocks and works are looked at
        const auto modelId = object->m_nModelIndex;
        if (modelId != ModelIndices::MI_ROADWORKBARRIER1 && modelId != ModelIndices::MI_ROADBLOCKFUCKEDCAR1 && modelId != ModelIndices::MI_ROADBLOCKFUCKEDCAR2) {
            continue;
        }

        const auto center = object->GetBoundCentre();
        if (!(arg3 < center.x && center.x < arg5 && arg4 < center.y && center.y < arg6)) {
            continue;
        }
        if (std::abs(center.z - vehicle->GetPosition().z) >= 10.0f) {
            continue;
        }

        // Is it on the way the vehicle is going, also in height
        const auto  forward = vehicle->GetForward();
        const auto& vehPos  = vehicle->GetPosition();
        const auto  dist    = CCollision::DistAlongLine2D(vehPos.x, vehPos.y, forward.x, forward.y, center.x, center.y);
        if (std::abs(center.z - (dist * forward.z + vehPos.z)) < 3.0f) {
            SlowCarDownForObject(object, vehicle, arg7, arg8);
        }
    }
}

// 0x42D0E0
void CCarCtrl::SlowCarDownForOtherCar(CEntity* car1, CVehicle* car2, float* arg3, float arg4) {
    plugin::Call<0x42D0E0, CEntity*, CVehicle*, float*, float>(car1, car2, arg3, arg4);
}

// 0x425440
template<typename PtrListType>
void CCarCtrl::SlowCarDownForPedsSectorList(PtrListType& PtrList, CVehicle* vehicle, float arg3, float arg4, float arg5, float arg6, float* arg7, float arg8) {
    plugin::Call<0x425440, PtrListType&, CVehicle*, float, float, float, float, float*, float>(PtrList, vehicle, arg3, arg4, arg5, arg6, arg7, arg8);
}

// 0x434790
void CCarCtrl::SlowCarOnRailsDownForTrafficAndLights(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if ((((int8)CTimer::GetFrameCounter() + (int8)(vehicle->m_nRandomSeed)) & 3) == 0) {
        if (CTrafficLights::ShouldCarStopForLight(vehicle, false) || CTrafficLights::ShouldCarStopForBridge(vehicle)) {
            CCarAI::CarHasReasonToStop(vehicle);
            autoPilot.m_fMaxTrafficSpeed = 0.0f;
        } else {
            autoPilot.m_fMaxTrafficSpeed = FindMaximumSpeedForThisCarInTraffic(vehicle);
        }
    }

    if (autoPilot.m_fMaxTrafficSpeed >= autoPilot.m_speed) {
        autoPilot.ModifySpeed(std::min(autoPilot.m_fMaxTrafficSpeed, CTimer::GetTimeStep() * 0.05f + autoPilot.m_speed));
    } else if (autoPilot.m_speed >= 0.1f) {
        autoPilot.ModifySpeed(std::max(autoPilot.m_fMaxTrafficSpeed, autoPilot.m_speed - CTimer::GetTimeStep() * 0.7f));
    } else if (autoPilot.m_speed != 0.0f) {
        autoPilot.ModifySpeed(0.0f);
    }
}

// Shared by the boat steering that follows the player. Returns the signed heading difference toward the aim point.
static float SteerBoatTowardsPoint(CVehicle* vehicle, const CVector2D& aim, float* steering, float* gas, float* handbrake, bool* reverse) {
    const auto& pos = vehicle->GetPosition();

    CVector2D forward{ vehicle->GetForward() };
    if (const auto len = forward.Magnitude(); len == 0.0f) {
        forward.x = 1.0f;
    } else {
        forward *= 1.0f / len;
    }

    auto turn = CGeneral::GetATanOfXY(aim.x - pos.x, aim.y - pos.y) - CGeneral::GetATanOfXY(forward.x, forward.y);
    while (turn < -3.1415927f) {
        turn += 6.2831855f;
    }
    while (turn > 3.1415927f) {
        turn -= 6.2831855f;
    }

    const auto cruiseSpeed = (float)vehicle->m_autoPilot.m_nCruiseSpeed;
    const auto speedDiff   = cruiseSpeed - vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    if (speedDiff > 0.0f) {
        const auto ratio = speedDiff / cruiseSpeed;
        *gas             = ratio <= 0.25f ? 1.0f - (0.25f - ratio) * 4.0f : 1.0f;
    } else {
        *gas = speedDiff < -5.0f ? -0.2f : -0.1f;
    }
    *handbrake = 0.0f;
    *steering  = turn;
    *reverse   = false;
    return turn;
}

// 0x428DE0
void CCarCtrl::SteerAIBoatWithPhysicsAttackingPlayer(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    // arg2: steering, arg3: gas / brake, arg4: handbrake
    const auto& pos       = vehicle->GetPosition();
    const auto  playerPos = FindPlayerCoors();
    const auto  dist      = (playerPos - pos).Magnitude();

    // Aim ahead of where the player is going, the further the more
    const auto lookAhead = std::min(dist * 0.05f, 2.0f);
    const auto playerVel = FindPlayerSpeed();
    const auto turn      = SteerBoatTowardsPoint(
        vehicle,
        { lookAhead * playerVel.x * 60.0f + playerPos.x, lookAhead * playerVel.y * 60.0f + playerPos.y },
        arg2, arg3, arg4, arg5
    );

    // The Predator opens fire once it is close and roughly facing the player
    if (vehicle->m_nModelIndex == 430 && dist < 40.0f && turn < 0.15f) {
        vehicle->FireFixedMachineGuns();
    }
}

// 0x429090
void CCarCtrl::SteerAIBoatWithPhysicsCirclingPlayer(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    const auto& pos       = vehicle->GetPosition();
    const auto  playerPos = FindPlayerCoors();

    // Direction to the player on the ground plane
    CVector2D toPlayer{ playerPos.x - pos.x, playerPos.y - pos.y };
    if (const auto len = toPlayer.Magnitude(); len == 0.0f) {
        toPlayer.x = 1.0f;
    } else {
        toPlayer *= 1.0f / len;
    }

    // Each boat circles to a fixed side, picked from its random seed, aiming at a point off to that side of the player
    const auto side = (vehicle->m_nRandomSeed & 1) ? -12.0f : 26.0f;
    SteerBoatTowardsPoint(vehicle, { toPlayer.y * side + playerPos.x, -toPlayer.x * side + playerPos.y }, arg2, arg3, arg4, arg5);
}

// 0x428BE0
void CCarCtrl::SteerAIBoatWithPhysicsHeadingForTarget(CVehicle* vehicle, float arg2, float arg3, float* arg4, float* arg5, float* arg6) {
    // arg4: steering, arg5: gas / brake, arg6: handbrake
    const auto& pos = vehicle->GetPosition();

    CVector2D forward{ vehicle->GetForward() };
    if (const auto len = forward.Magnitude(); len == 0.0f) {
        forward.x = 1.0f;
    } else {
        forward *= 1.0f / len;
    }

    // How far the boat has to turn, in [-0.5, 0.5]
    const auto targetHeading  = CGeneral::GetATanOfXY(arg2 - pos.x, arg3 - pos.y);
    const auto currentHeading = CGeneral::GetATanOfXY(forward.x, forward.y);
    auto       turn           = targetHeading - currentHeading;
    while (turn < -3.1415927f) {
        turn += 6.2831855f;
    }
    while (turn > 3.1415927f) {
        turn -= 6.2831855f;
    }
    turn = std::clamp(turn, -0.5f, 0.5f);

    const auto cruiseSpeed = (float)vehicle->m_autoPilot.m_nCruiseSpeed;
    const auto speedDiff   = cruiseSpeed - vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    if (speedDiff > 0.0f) {
        // Too slow, so speed up
        const auto ratio = speedDiff / cruiseSpeed;
        *arg5            = ratio <= 0.25f ? 1.0f - (0.25f - ratio) * 4.0f : 1.0f;
        *arg6            = 0.0f;
        *arg4            = turn;
        return;
    }

    // Too fast, so brake and steer the other way
    *arg5 = speedDiff < -5.0f ? -0.2f : -0.1f;
    *arg6 = 0.0f;
    *arg4 = turn * -1.0f;
}

// 0x422B20
void CCarCtrl::SteerAICarBlockingPlayerForwardAndBack(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    // arg2: steering, arg3: gas / reverse, arg4: brake, arg5: handbrake
    *arg2 = 0.0f;
    *arg5 = false;

    // Where the player is heading, a bit more to where they are looking
    auto playerSpeed = FindPlayerSpeed();
    auto* const playerEntity = FindPlayerEntity();
    if (!playerEntity->m_matrix) {
        playerEntity->AllocateMatrix();
        playerEntity->m_placement.UpdateMatrix(playerEntity->m_matrix);
    }
    playerSpeed.x += playerEntity->GetForward().x * 0.1f;
    playerSpeed.y += playerEntity->GetForward().y * 0.1f;

    auto right = vehicle->GetRight();
    right.z    = 0.0f;
    right.Normalise();
    auto forward = vehicle->GetForward();
    forward.z    = 0.0f;
    forward.Normalise();

    const auto toPlayer = FindPlayerCoors() - vehicle->GetPosition();

    // When the player will cross the vehicle's side line
    auto approachSpeed = DotProduct(right, playerSpeed);
    if (approachSpeed == 0.0f) {
        approachSpeed = 0.01f;
    }
    const auto timeToCross = -(DotProduct(toPlayer, right) / approachSpeed);
    if (timeToCross < 0.0f) {
        *arg3 = 0.0f;
        *arg4 = 0.0f;
        return;
    }

    // How far in front of or behind the vehicle the player will be by then
    const auto speedAlong = DotProduct(forward, vehicle->GetMoveSpeed());
    auto       offset     = DotProduct(toPlayer, forward) + DotProduct(forward, playerSpeed) * timeToCross - speedAlong * timeToCross;
    if (offset > 0.0f) {
        *arg3 = std::min(offset * 0.1f, 1.0f);
        *arg4 = 0.0f;
        return;
    }

    if (speedAlong > 0.0f) {
        // Going forward but the player is behind, brake
        const auto brake = std::min(offset * -0.1f, 1.0f);
        *arg3            = 0.0f;
        *arg4            = brake;
        if (brake > 0.95f) {
            *arg5 = true;
        }
        return;
    }

    *arg3 = std::max(offset * 0.1f, -1.0f);
    *arg4 = 0.0f;
}

// 0x433BA0
void CCarCtrl::SteerAICarParkParallel(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    // arg2: steering, arg3: gas, arg4: brake, arg5: handbrake
    auto& autoPilot = vehicle->m_autoPilot;

    if (!ThePaths.IsAreaLoaded(autoPilot.m_startingRouteNode.m_wAreaId) || !ThePaths.IsAreaLoaded(autoPilot.m_currentAddress.m_wAreaId)) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        *arg2 = 0.0f;
        *arg3 = 0.0f;
        *arg4 = 0.0f;
        return;
    }

    const auto& currentNode = *ThePaths.GetPathNode(autoPilot.m_currentAddress);
    const auto& targetNode  = *ThePaths.GetPathNode(autoPilot.m_startingRouteNode);

    // Parallel parking goes a little past the node, along the way the road goes
    CVector target = targetNode.GetPosition();
    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        auto along = targetNode.GetPosition() - currentNode.GetPosition();
        along.Normalise();
        target = targetNode.GetPosition() + along;
    }

    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, arg2, arg3, arg4, arg5);
    autoPilot.m_nCruiseSpeed = std::min<uint8>(autoPilot.m_nCruiseSpeed, 8);

    const auto dist = (CVector2D{ target } - CVector2D{ vehicle->GetPosition() }).Magnitude();
    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        if (dist < 4.0f) {
            autoPilot.m_nCarMission = MISSION_PARK_PARALLEL_2;
        }
        return;
    }

    if (dist >= 2.0f) {
        return;
    }

    // Parked, everybody gets out
    autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->vehicleFlags.bLightsOn = false;
    if (auto* const driver = vehicle->m_pDriver) {
        driver->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar{ 0, true, false }, TASK_PRIMARY_PRIMARY, false);
    }
    for (auto* const passenger : vehicle->m_apPassengers) {
        if (passenger) {
            passenger->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar{ 0, true, false }, TASK_PRIMARY_PRIMARY, false);
        }
    }
}

// 0x433EA0
void CCarCtrl::SteerAICarParkPerpendicular(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    // arg2: steering, arg3: gas, arg4: brake, arg5: handbrake
    auto& autoPilot = vehicle->m_autoPilot;

    if (!ThePaths.IsAreaLoaded(autoPilot.m_startingRouteNode.m_wAreaId) || !ThePaths.IsAreaLoaded(autoPilot.m_currentAddress.m_wAreaId)) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        *arg2 = 0.0f;
        *arg3 = 0.0f;
        *arg4 = 0.0f;
        return;
    }

    const auto& currentNode = *ThePaths.GetPathNode(autoPilot.m_currentAddress);
    const auto& parkNode    = *ThePaths.GetPathNode(autoPilot.m_startingRouteNode);

    // First get in line with the park node, the way the road goes
    if (autoPilot.m_nCarMission == MISSION_PARK_PERPENDICULAR) {
        const CVector parkPos{ parkNode.GetPosition().x, parkNode.GetPosition().y, 0.0f };
        const CVector roadPos{ currentNode.GetPosition().x, currentNode.GetPosition().y, 0.0f };
        const CVector vehPos{ vehicle->GetPosition().x, vehicle->GetPosition().y, 0.0f };
        if (CCollision::DistToMathematicalLine(&parkPos, &roadPos, &vehPos) < 6.0f) {
            autoPilot.m_nCarMission = MISSION_PARK_PERPENDICULAR_2;
        }
    }

    const auto target = autoPilot.m_nCarMission == MISSION_PARK_PERPENDICULAR
        ? currentNode.GetPosition()
        : parkNode.GetPosition();
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, arg2, arg3, arg4, arg5);
    autoPilot.m_nCruiseSpeed = std::min<uint8>(autoPilot.m_nCruiseSpeed, 8);

    const auto dist = (CVector2D{ parkNode.GetPosition() } - CVector2D{ vehicle->GetPosition() }).Magnitude();
    if (dist >= 2.0f) {
        return;
    }

    // Parked, everybody gets out
    autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
    autoPilot.movementFlags.bIsParked = true;
    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->vehicleFlags.bLightsOn = false;
    if (auto* const driver = vehicle->m_pDriver) {
        driver->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar{ 0, true, false }, TASK_PRIMARY_PRIMARY, false);
    }
    for (auto* const passenger : vehicle->m_apPassengers) {
        if (passenger) {
            passenger->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar{ 0, true, false }, TASK_PRIMARY_PRIMARY, false);
        }
    }
}

// 0x4336D0
void CCarCtrl::SteerAICarTowardsPointInEscort(CVehicle* vehicle1, CVehicle* vehicle2, float arg3, float arg4, float* arg5, float* arg6, float* arg7, bool* arg8) {
    plugin::Call<0x4336D0, CVehicle*, CVehicle*, float, float, float*, float*, float*, bool*>(vehicle1, vehicle2, arg3, arg4, arg5, arg6, arg7, arg8);
}

// 0x437C20
void CCarCtrl::SteerAICarWithPhysics(CVehicle* vehicle) {
    plugin::Call<0x437C20, CVehicle*>(vehicle);
}

// 0x434900
void CCarCtrl::SteerAICarWithPhysicsFollowPath(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    plugin::Call<0x434900, CVehicle*, float*, float*, float*, bool*>(vehicle, arg2, arg3, arg4, arg5);
}

// 0x435830
void CCarCtrl::SteerAICarWithPhysicsFollowPath_Racing(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    plugin::Call<0x435830, CVehicle*, float*, float*, float*, bool*>(vehicle, arg2, arg3, arg4, arg5);
}

// 0x432DD0
void CCarCtrl::SteerAICarWithPhysicsFollowPreRecordedPath(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    plugin::Call<0x432DD0, CVehicle*, float*, float*, float*, bool*>(vehicle, arg2, arg3, arg4, arg5);
}

// 0x433280
void CCarCtrl::SteerAICarWithPhysicsHeadingForTarget(CVehicle* vehicle, CPhysical* target, float arg3, float arg4, float* arg5, float* arg6, float* arg7, bool* arg8) {
    static_assert(offsetof(CAutoPilot, m_nCarDrivingStyle) == 0x29);
    static_assert(offsetof(CBmx, m_fControlPedaling) == 0x818);

    // arg3 and arg4 are the target coordinates, arg5 steering, arg6 gas, arg7 brake, arg8 handbrake
    *arg8 = false;

    const auto& pos = vehicle->GetPosition();

    CVector2D forward{ vehicle->GetForward() };
    if (const auto len = forward.Magnitude(); len == 0.0f) {
        forward.x = 1.0f;
    } else {
        forward *= 1.0f / len;
    }

    const auto headingToTarget = CGeneral::GetATanOfXY(arg3 - pos.x, arg4 - pos.y);
    const auto currentHeading  = CGeneral::GetATanOfXY(forward.x, forward.y);

    auto aimHeading = headingToTarget;
    switch (vehicle->m_autoPilot.m_nCarDrivingStyle) {
    case DRIVING_STYLE_AVOID_CARS:
    case DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_OBEYLIGHTS:
    case DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS:
        aimHeading = FindAngleToWeaveThroughTraffic(vehicle, target, headingToTarget, currentHeading, 1.0f);
        break;
    default:
        break;
    }

    auto turn = aimHeading - currentHeading;
    while (turn < -3.1415927f) {
        turn += 6.2831855f;
    }
    while (turn > 3.1415927f) {
        turn -= 6.2831855f;
    }

    // Sharp turns at speed need the handbrake
    if (vehicle->GetMoveSpeed().Magnitude() > 0.3f && std::abs(turn) > 0.7f) {
        *arg8 = true;
    }

    const auto maxSteer = FindMaxSteerAngle(vehicle);
    turn                = std::clamp(turn, -maxSteer, maxSteer);

    // Slow down in proportion to how much the car has to turn
    const auto speedMult = FindSpeedMultiplier(headingToTarget - currentHeading, 0.4f, 1.2f, 0.4f);
    const auto speed     = vehicle->GetMoveSpeed().Magnitude() * 60.0f;
    const auto speedDiff = speedMult * (float)vehicle->m_autoPilot.m_nCruiseSpeed - speed;

    *arg7 = 0.0f;
    if (speedDiff <= 0.0f) {
        *arg6 = 0.0f;
        *arg7 = std::min(speedDiff * -0.05f, 0.5f);
        *arg5 = turn;
        return;
    }

    *arg6 = speed >= 25.0f ? 1.0f : std::min(speedDiff * 0.1f, 1.0f);

    // Make BMX riders pedal when they are far too slow
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX && speedDiff > 3.0f) {
        auto& pedaling = static_cast<CBmx*>(vehicle)->m_fControlPedaling;
        if (pedaling <= 0.0f) {
            pedaling = 10.0f;
        }
    }
    *arg5 = turn;
}

// 0x4335E0
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget(CVehicle* vehicle, CEntity* Unusued, float arg3, float arg4, float arg5, float arg6, float* arg7, float* arg8, float* arg9, bool* arg10) {
    static_assert(offsetof(CVehicle, m_autoPilot) + offsetof(CAutoPilot, m_nCarDrivingStyle) == 0x3B9);

    // arg3 and arg4 are the position of the target, arg5 and arg6 its speed, arg7 steering, arg8 gas, arg9 brake, arg10 handbrake
    auto speedX = arg5;
    auto speedY = arg6;
    if (const auto speed = std::sqrt(speedY * speedY + speedX * speedX); speed > 0.13f) {
        const auto scale = 0.13f / speed;
        speedX *= scale;
        speedY *= scale;
    }

    // Aims for where the target will be in a while
    const auto x = speedX * 60.0f + arg3;
    const auto y = speedY * 60.0f + arg4;

    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, x, y, arg7, arg8, arg9, arg10);

    // Once there stops in front of it
    const auto& pos = vehicle->GetPosition();
    if ((x - pos.x) * (x - pos.x) + (y - pos.y) * (y - pos.y) < 25.0f) {
        auto& mission = vehicle->m_autoPilot.m_nCarMission;
        mission = mission == MISSION_BLOCKCAR_CLOSE ? MISSION_BLOCKCAR_HANDBRAKESTOP : MISSION_BLOCKPLAYER_HANDBRAKESTOP;
    }
}

// 0x428990
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget_Stop(CVehicle* vehicle, float x, float y, float arg4, float arg5, float* arg6, float* arg7, float* arg8, bool* arg9) {
    static_assert(offsetof(CVehicle, m_nCopsInCarTimer) == 0x4DC);
    static_assert(offsetof(CVehicle, m_pDriver) == 0x460);

    // arg4 and arg5 are the speed of the target, arg6 steering, arg7 gas, arg8 brake, arg9 handbrake
    *arg6 = 0.0f;
    *arg7 = 0.0f;
    *arg8 = 1.0f;
    *arg9 = true;

    auto& mission = vehicle->m_autoPilot.m_nCarMission;

    const auto& pos  = vehicle->GetPosition();
    const auto  dist = (pos.x - x) * (pos.x - x) + (pos.y - y) * (pos.y - y);
    if (dist > 100.0f) {
        mission = mission == MISSION_BLOCKCAR_HANDBRAKESTOP ? MISSION_BLOCKCAR_CLOSE : MISSION_BLOCKPLAYER_CLOSE;
        return;
    }

    const auto StopAndLeave = [&] {
        CCarAI::TellOccupantsToLeaveCar(vehicle);
        vehicle->m_autoPilot.m_nCruiseSpeed = 0;
        mission                             = MISSION_NONE;
    };

    if (mission == MISSION_BLOCKCAR_HANDBRAKESTOP) {
        // A driver that is busy with a criminal keeps the car where it is
        if (const auto* driver = vehicle->m_pDriver) {
            if (const auto* task = driver->GetIntelligence()->GetTaskManager().GetActiveTask(); task && task->GetTaskType() == TASK_COMPLEX_KILL_CRIMINAL) {
                return;
            }
        }
        if (vehicle->GetMoveSpeed().SquaredMagnitude2D() >= 0.0001f) {
            return;
        }
        if (arg4 * arg4 + arg5 * arg5 >= 0.0004f) {
            return;
        }
        if (!vehicle->vehicleFlags.bIsLawEnforcer) {
            return;
        }
        StopAndLeave();
        return;
    }

    // Count how long the player has been standing still in a vehicle
    auto* const playerVeh = FindPlayerVehicle();
    if (playerVeh && playerVeh->GetMoveSpeed().Magnitude() < 0.05f) {
        vehicle->m_nCopsInCarTimer = (int16)(int32)(CTimer::GetTimeStep() * 16.666666f + (float)(uint16)vehicle->m_nCopsInCarTimer);
    } else {
        vehicle->m_nCopsInCarTimer = 0;
    }

    if (FindPlayerVehicle() && !FindPlayerVehicle()->IsUpsideDown()) {
        if (FindPlayerVehicle()->GetMoveSpeed().Magnitude() >= 0.05f) {
            return;
        }
        if ((uint16)vehicle->m_nCopsInCarTimer <= 2500) {
            return;
        }
    }

    if (!vehicle->vehicleFlags.bIsLawEnforcer || dist >= 100.0f) {
        return;
    }
    StopAndLeave();
}

// 0x436A90
void CCarCtrl::SteerAICarWithPhysics_OnlyMission(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    plugin::Call<0x436A90, CVehicle*, float*, float*, float*, bool*>(vehicle, arg2, arg3, arg4, arg5);
}

// 0x42AAD0
void CCarCtrl::SteerAIHeliAsPoliceHeli(CAutomobile* automobile) {
    auto* const heli      = static_cast<CHeli*>(automobile);
    const auto& targetPos = heli->m_autoPilot.m_TargetEntity->GetPosition();
    const auto  toTarget  = CVector2D{ targetPos } - CVector2D{ heli->GetPosition() };

    const auto heading = CGeneral::GetATanOfXY(toTarget.x, toTarget.y);
    const auto dist    = toTarget.Magnitude();

    // Stay above the target, and further away it should be higher up
    heli->m_fMaxAltitude = std::max(targetPos.z, 6.0f);
    if (dist > 50.0f) {
        heli->m_fMaxAltitude = std::max(heli->m_fMaxAltitude, 25.0f);
    }
    heli->m_autoPilot.m_vecDestinationCoors = targetPos;

    FlyAIHeliInCertainDirection(heli, heading, dist, true);

    // Leave if the heli is badly damaged
    if (heli->m_fHealth < 230.0f) {
        heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
}

// 0x42ACB0
void CCarCtrl::SteerAIHeliFlyingAwayFromPlayer(CAutomobile* automobile) {
    auto* const heli     = static_cast<CHeli*>(automobile);
    const auto  toPlayer = CVector2D{ FindPlayerCoors() } - CVector2D{ heli->GetPosition() };

    // Head straight away from the player, far enough that it never gets there
    FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(toPlayer.x, toPlayer.y) + 3.1415927f, 1000.0f, false);
}

// 0x4238E0
void CCarCtrl::SteerAIHeliToCrashAndBurn(CAutomobile* automobile) {
    auto* const heli = static_cast<CHeli*>(automobile);
    const bool  odd  = heli->m_nRandomSeed & 1;

    heli->m_fSteeringUpDown          = -0.3f;
    heli->m_fLeftRightSkid           = odd ? heli->field_A14 : -heli->field_A14;
    heli->m_fAccelerationBreakStatus = -0.5f;
    heli->m_fSteeringLeftRight       = odd ? 1.0f : -1.0f;
}

// 0x42A750
void CCarCtrl::SteerAIHeliToFollowEntity(CAutomobile* automobile) {
    static_assert(offsetof(CAutoPilot, field_4A) == 0x4A);
    static_assert(offsetof(CVehicle, m_nCreationTime) == 0x430);

    auto* const heli      = static_cast<CHeli*>(automobile);
    auto&       autoPilot = heli->m_autoPilot;
    auto* const target    = autoPilot.m_TargetEntity;

    autoPilot.m_vecDestinationCoors = target->GetPosition();
    if (autoPilot.field_4A) {
        // Aim ahead of the target, or behind it when the distance is negative
        if (!target->m_matrix) {
            target->AllocateMatrix();
            target->m_placement.UpdateMatrix(target->m_matrix);
        }
        const auto distAhead = (float)autoPilot.field_4A;
        autoPilot.m_vecDestinationCoors.x += target->GetForward().x * distAhead;
        autoPilot.m_vecDestinationCoors.y += target->GetForward().y * distAhead;
    }

    const auto& heliPos   = heli->GetPosition();
    const auto& targetPos = target->GetPosition();
    const auto  heading   = CGeneral::GetATanOfXY(autoPilot.m_vecDestinationCoors.x - heliPos.x, autoPilot.m_vecDestinationCoors.y - heliPos.y);
    const auto  dist      = (CVector2D{ targetPos } - CVector2D{ heliPos }).Magnitude();

    heli->m_fMaxAltitude = std::max(targetPos.z, 6.0f);
    if (dist > 50.0f) {
        heli->m_fMaxAltitude = std::max(heli->m_fMaxAltitude, 25.0f);
    }

    if (heli->m_fForcedOrientation < 0.0f) {
        FlyAIHeliInCertainDirection(heli, heading, dist, true);
    } else {
        FlyAIHeliToTarget_FixedOrientation(heli, heli->m_fForcedOrientation, targetPos, true);
    }

    // Once the heli has caught up, whoever it follows is told to keep going
    if (autoPilot.carCtrlFlags.bDoTargetCatchupCheck) {
        if ((CVector2D{ targetPos } - CVector2D{ heli->GetPosition() }).Magnitude() < 25.0f) {
            if (target->GetType() == ENTITY_TYPE_VEHICLE) {
                if (target->GetStatus() == STATUS_SIMPLE || target->GetStatus() == STATUS_PHYSICS) {
                    auto* const vehicle                    = target->AsVehicle();
                    vehicle->m_autoPilot.m_nCarMission      = MISSION_CRUISE;
                    vehicle->m_autoPilot.m_nCruiseSpeed     = 100;
                    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
                    vehicle->SetStatus(STATUS_PHYSICS);
                }
            } else if (target->GetType() == ENTITY_TYPE_PED) {
                if (auto* const task = target->AsPed()->GetTaskManager().GetActiveTask(); task && task->GetTaskType() == TASK_COMPLEX_WANDER) {
                    static_cast<CTaskComplexWander*>(task)->m_nMoveState = PEDMOVE_SPRINT;
                }
            }
            autoPilot.carCtrlFlags.bDoTargetCatchupCheck = false;
        }
    }

    // Give up following after a while
    if (autoPilot.carCtrlFlags.bHeliFollowTarget && heli->m_nCreationTime + 50000 < CTimer::GetTimeInMS()) {
        autoPilot.carCtrlFlags.bHeliFollowTarget = false;
        autoPilot.m_nCarMission                  = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }

    if (autoPilot.m_nCarMission == MISSION_HELI_NEWS_BEHAVIOUR && heli->m_fHealth < 300.0f) {
        autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
}

// 0x42AEB0
void CCarCtrl::SteerAIHeliToKeepEntityInView(CAutomobile* automobile) {
    static_assert(offsetof(CHeli, m_fLeftRightSkid) == 0x98C);
    static_assert(offsetof(CHeli, m_fSteeringUpDown) == 0x990);
    static_assert(offsetof(CHeli, m_fSteeringLeftRight) == 0x994);
    static_assert(offsetof(CHeli, m_fAccelerationBreakStatus) == 0x998);
    static_assert(offsetof(CHeli, field_9AC) == 0x9AC);
    static_assert(offsetof(CAutoPilot, m_ucHeliTargetDist) == 0x48);

    auto* const heli      = static_cast<CHeli*>(automobile);
    auto&       autoPilot = heli->m_autoPilot;
    const auto& targetPos = autoPilot.m_TargetEntity->GetPosition();
    const auto& heliPos   = heli->GetPosition();

    const auto toTarget  = CVector2D{ targetPos } - CVector2D{ heliPos };
    const auto dist      = toTarget.Magnitude();
    const auto keepDist  = (float)autoPilot.m_ucHeliTargetDist;
    if (dist > keepDist + keepDist) {
        // Too far to just hover around, go after it
        SteerAIHeliToFollowEntity(heli);
        return;
    }

    // Turn so the side of the heli faces the target
    const auto heading = CGeneral::GetATanOfXY(toTarget.x, toTarget.y);
    auto       turn    = (heading + 1.5707964f) - CGeneral::GetATanOfXY(heli->GetForward().x, heli->GetForward().y);
    while (turn > 3.1415927f) {
        turn -= 6.2831855f;
    }
    while (turn < -3.1415927f) {
        turn += 6.2831855f;
    }
    heli->m_fLeftRightSkid = std::clamp(-turn, -1.0f, 1.0f);

    // Hover a bit above the target
    heli->m_fMaxAltitude = targetPos.z + 15.0f;
    heli->field_9AC      = heli->m_fMaxAltitude;
    autoPilot.m_vecDestinationCoors = targetPos;

    const auto altitudeDiff = heli->m_fMaxAltitude - (heli->GetMoveSpeed().z * 100.0f + heliPos.z);
    auto       climb        = altitudeDiff * (altitudeDiff <= 0.0f ? 0.2f : 0.1f) + 0.3f;
    climb += ((float)(CGeneral::GetRandomNumber() & 0xF) - 7.0f) * 0.002f;
    heli->m_fAccelerationBreakStatus = std::clamp(climb, 0.0f, 1.0f);

    // Keep the distance to the target by moving sideways
    if (dist >= keepDist * 0.5f) {
        heli->m_fSteeringLeftRight = dist <= keepDist ? DotProduct(heli->GetRight(), heli->GetMoveSpeed()) : -0.5f;
    } else {
        heli->m_fSteeringLeftRight = 0.5f;
    }

    heli->m_fSteeringUpDown = 0.0f;
    if (dist < keepDist * 1.5f) {
        heli->m_fSteeringUpDown = DotProduct(heli->GetMoveSpeed(), heli->GetForward());
    }
}

// 0x42AD30
void CCarCtrl::SteerAIHeliToLand(CAutomobile* automobile) {
    static_assert(offsetof(CHeli, m_fMinAltitude) == 0x9B0);

    auto* const heli     = static_cast<CHeli*>(automobile);
    const auto  toTarget = CVector2D{ heli->m_autoPilot.m_vecDestinationCoors } - CVector2D{ heli->GetPosition() };
    const auto  dist     = toTarget.Magnitude();

    FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(toTarget.x, toTarget.y), dist, true);

    // Once it is over the spot and has stopped, drop to the ground and cut the controls as soon as a wheel touches
    if (dist < 10.0f && heli->GetMoveSpeed().Magnitude2D() < 0.05f) {
        heli->m_fMinAltitude = 0.0f;
        heli->m_fMaxAltitude = 0.0f;
        if (std::ranges::any_of(heli->m_fWheelsSuspensionCompression, [](float v) { return v < 1.0f; })) {
            heli->m_fAccelerationBreakStatus = 0.0f;
            heli->m_fLeftRightSkid           = 0.0f;
            heli->m_fSteeringUpDown          = 0.0f;
            heli->m_fSteeringLeftRight       = 0.0f;
        }
    }
}

// 0x42A630
void CCarCtrl::SteerAIHeliTowardsTargetCoors(CAutomobile* automobile) {
    auto* const heli   = static_cast<CHeli*>(automobile);
    const auto& target = heli->m_autoPilot.m_vecDestinationCoors;

    // Keep the orientation if it's forced
    if (heli->m_fForcedOrientation >= 0.0f) {
        FlyAIHeliToTarget_FixedOrientation(heli, heli->m_fForcedOrientation, target, true);
        return;
    }

    const auto toTarget = CVector2D{ target } - CVector2D{ heli->GetPosition() };
    FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(toTarget.x, toTarget.y), toTarget.Magnitude(), true);
}

// 0x423880
void CCarCtrl::SteerAIPlaneToCrashAndBurn(CAutomobile* automobile) {
    auto* const plane = static_cast<CPlane*>(automobile);
    const bool  odd   = plane->m_nRandomSeed & 1; // Which way it spins depends on the vehicle

    plane->m_fSteeringUpDown          = -0.3f;
    plane->m_fLeftRightSkid           = odd ? 1.0f : -1.0f;
    plane->m_fAccelerationBreakStatus = 0.0f;
    plane->m_fSteeringLeftRight       = odd ? 1.0f : -1.0f;
}

// 0x4237F0
void CCarCtrl::SteerAIPlaneToFollowEntity(CAutomobile* automobile) {
    auto* const plane  = static_cast<CPlane*>(automobile);
    const auto& target = plane->m_autoPilot.m_TargetEntity->GetPosition();
    const auto& pos    = plane->GetPosition();

    plane->m_planeHeading = CGeneral::GetATanOfXY(target.x - pos.x, target.y - pos.y);
    plane->m_maxAltitude  = target.z;
    FlyAIPlaneInCertainDirection(plane);
}

// 0x423790
void CCarCtrl::SteerAIPlaneTowardsTargetCoors(CAutomobile* automobile) {
    auto* const plane  = static_cast<CPlane*>(automobile);
    const auto& target = plane->m_autoPilot.m_vecDestinationCoors;
    const auto& pos    = plane->GetPosition();

    plane->m_planeHeading = CGeneral::GetATanOfXY(target.x - pos.x, target.y - pos.y);
    FlyAIPlaneInCertainDirection(plane);
}

// 0x422590
bool CCarCtrl::StopCarIfNodesAreInvalid(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    const auto IsLinkValid = [](CCarPathLinkAddress link) {
        return link.IsValid() && ThePaths.IsAreaLoaded(link.m_wAreaId);
    };
    const auto IsNodeValid = [](CNodeAddress node) {
        return node.m_wAreaId != UINT16_MAX && node.m_wAreaId < std::size(ThePaths.m_pPathNodes) && ThePaths.IsAreaLoaded(node.m_wAreaId);
    };

    if (IsLinkValid(autoPilot.m_nCurrentPathNodeInfo)
        && IsLinkValid(autoPilot.m_nNextPathNodeInfo)
        && IsNodeValid(autoPilot.m_currentAddress)
        && IsNodeValid(autoPilot.m_startingRouteNode)
    ) {
        return false;
    }

    autoPilot.movementFlags.bIsStopped = true;
    return true;
}

// 0x4222A0
void CCarCtrl::SwitchBetweenPhysicsAndGhost(CVehicle* vehicle) {
    // Only mission vehicles that wait for the collision to load around them
    if (!vehicle->physicalFlags.bDontLoadCollision || vehicle->m_nCreatedBy != MISSION_VEHICLE) {
        return;
    }
    if (const auto type = vehicle->m_nVehicleSubType; type == VEHICLE_TYPE_BOAT || type == VEHICLE_TYPE_PLANE || type == VEHICLE_TYPE_HELI) {
        return;
    }

    switch (vehicle->GetStatus()) {
    case STATUS_PHYSICS:
        // Collision around the vehicle isn't there, so don't let it fall through the world
        if (!CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
            vehicle->SetStatus(STATUS_GHOST);
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
                for (auto wheel = 0; wheel <= 3; wheel++) {
                    vehicle->AsAutomobile()->GetDamageManager().SetWheelStatus((eCarWheel)wheel, WHEEL_STATUS_OK);
                }
            }
        }
        break;
    case STATUS_GHOST:
        if (CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
            vehicle->SetStatus(STATUS_PHYSICS);
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
                vehicle->AsBike()->PlaceOnRoadProperly();
            } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                vehicle->AsAutomobile()->PlaceOnRoadProperly();
            }
        }
        break;
    default:
        break;
    }
}

// 0x423FC0
void CCarCtrl::SwitchVehicleToRealPhysics(CVehicle* vehicle) {
    vehicle->SetStatus(STATUS_PHYSICS);
    vehicle->m_autoPilot.m_nTempAction              = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nTimeToStartMission      = CTimer::GetTimeInMS() + 2000;
    vehicle->m_autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
    vehicle->m_nFakePhysics                         = 0;
}

// 0x425B30
float CCarCtrl::TestCollisionBetween2MovingRects(CVehicle* vehicle1, CVehicle* vehicle2, float arg3, float arg4, CVector* pos1, CVector* pos2) {
    // arg3 and arg4 are how far vehicle1 moves relative to vehicle2, pos1 is the heading of vehicle2 and pos2 the one of vehicle1.
    // The result is how far into that movement the two are first touching, from 0 to 1.
    const auto& heading2 = *pos1;
    const auto& heading1 = *pos2;

    const auto& bb2 = vehicle2->GetColModel()->GetBoundingBox();
    const auto& bb1 = vehicle1->GetColModel()->GetBoundingBox();
    const auto  maxY2 = bb2.m_vecMax.y;
    const auto  maxX2 = bb2.m_vecMax.x;
    const auto  rear2 = -bb2.m_vecMin.y;
    const auto  maxX1 = bb1.m_vecMax.x;
    const auto  maxY1 = bb1.m_vecMax.y;

    const auto offsetX = vehicle1->GetPosition().x - vehicle2->GetPosition().x;
    const auto offsetY = vehicle1->GetPosition().y - vehicle2->GetPosition().y;

    // How the movement splits up along the sides and the length of vehicle2
    const auto moveSide = arg3 * heading2.y - arg4 * heading2.x;
    const auto moveFwd  = arg4 * heading2.y + arg3 * heading2.x;

    struct Span {
        float enter, exit;
    };
    // When a point at `pos` moving by `vel` is inside of the range `lo`..`hi`
    const auto Slab = [](float pos, float lo, float hi, float vel) -> Span {
        if (pos <= hi) {
            if (pos < lo) {
                if (vel > 0.0f) {
                    const auto enter = -((pos - lo) * (1.0f / vel));
                    if (1.0f <= enter) {
                        return { 1.0f, 1.0f };
                    }
                    const auto exit = (hi - lo) * (1.0f / vel) + enter;
                    return { enter, exit < 1.0f ? exit : 1.0f };
                }
                return { 1.0f, 1.0f };
            }
            auto exit = 1.0f;
            if (vel > 0.0f) {
                exit = (hi - pos) / vel;
            } else if (vel < 0.0f) {
                exit = -((pos - lo) / vel);
            }
            return { 0.0f, exit };
        }
        if (vel >= 0.0f) {
            return { 1.0f, 1.0f };
        }
        const auto enter = -((pos - hi) * (1.0f / vel));
        if (1.0f <= enter) {
            return { 1.0f, 1.0f };
        }
        const auto exit = enter - (hi - lo) * (1.0f / vel);
        return { enter, exit < 1.0f ? exit : 1.0f };
    };

    auto result = 1.0f;
    for (auto i = 0; i < 2; i++) {
        // The front corners of vehicle1, relative to vehicle2
        float cornerX, cornerY;
        if (i == 0) {
            cornerX = maxX1 * heading1.y + maxY1 * heading1.x + offsetX;
            cornerY = (maxY1 * heading1.y + offsetY) - maxX1 * heading1.x;
        } else {
            cornerX = (maxY1 * heading1.x + offsetX) - maxX1 * heading1.y;
            cornerY = maxY1 * heading1.y + maxX1 * heading1.x + offsetY;
        }

        const auto side = Slab(cornerX * heading2.y - cornerY * heading2.x, -maxX2, maxX2, moveSide);
        const auto fwd  = Slab(cornerX * heading2.x + cornerY * heading2.y, -rear2, maxY2, moveFwd);

        const auto enter = std::max(side.enter, fwd.enter);
        if (enter < side.exit && enter < fwd.exit && enter <= result) {
            result = enter;
        }
    }
    return result;
}

// 0x425F70
float CCarCtrl::TestCollisionBetween2MovingRects_OnlyFrontBumper(CVehicle* vehicle1, CVehicle* vehicle2, float arg3, float arg4, CVector* pos1, CVector* pos2) {
    // Same inputs as above, but only the front edge of vehicle2 is tested against the corners of vehicle1
    const auto& heading2 = *pos1;
    const auto& heading1 = *pos2;
    const auto& pos1v    = vehicle1->GetPosition();
    const auto& pos2v    = vehicle2->GetPosition();

    const auto& bb2   = vehicle2->GetColModel()->GetBoundingBox();
    const auto& bb1   = vehicle1->GetColModel()->GetBoundingBox();
    const auto  maxY2 = bb2.m_vecMax.y;
    const auto  maxX2 = bb2.m_vecMax.x;
    const auto  maxY1 = bb1.m_vecMax.y;
    const auto  maxX1 = bb1.m_vecMax.x;
    const auto  rear1 = -bb1.m_vecMin.y;

    // The two ends of the front edge of vehicle2
    const auto sideY2      = maxX2 * heading2.y;
    const auto frontX2     = maxY2 * heading2.x;
    const auto rightEdgeX  = frontX2 + sideY2 + pos2v.x;
    const auto frontY2     = maxY2 * heading2.y;
    const auto sideX2      = maxX2 * heading2.x;
    const auto rightEdgeY  = (frontY2 + pos2v.y) - sideX2;
    const auto leftEdgeX   = (frontX2 + pos2v.x) - sideY2;
    const auto leftEdgeY   = sideX2 + frontY2 + pos2v.y;

    auto result = 1.0f;
    for (auto i = 0; i < 4; i++) {
        float cornerX, cornerY;
        switch (i) {
        case 0:
            cornerX = maxY1 * heading1.x + maxX1 * heading1.y + pos1v.x;
            cornerY = (maxY1 * heading1.y + pos1v.y) - maxX1 * heading1.x;
            break;
        case 1:
            cornerX = (maxY1 * heading1.x + pos1v.x) - maxX1 * heading1.y;
            cornerY = maxX1 * heading1.x + maxY1 * heading1.y + pos1v.y;
            break;
        case 2:
            cornerX = maxX1 * heading1.y + (pos1v.x - rear1 * heading1.x);
            cornerY = (pos1v.y - rear1 * heading1.y) - maxX1 * heading1.x;
            break;
        default:
            cornerX = (pos1v.x - rear1 * heading1.x) - maxX1 * heading1.y;
            cornerY = maxX1 * heading1.x + (pos1v.y - rear1 * heading1.y);
            break;
        }

        // How far in front of the edge the corner is, now and after moving
        const auto before = (cornerX - rightEdgeX) * heading2.x + (cornerY - rightEdgeY) * heading2.y;
        const auto after  = ((cornerX + arg3) - rightEdgeX) * heading2.x + ((cornerY + arg4) - rightEdgeY) * heading2.y;
        if (before > 0.0f && after < 0.0f
            && ((rightEdgeX - cornerX) * arg4 - (rightEdgeY - cornerY) * arg3) * ((leftEdgeX - cornerX) * arg4 - (leftEdgeY - cornerY) * arg3) < 0.0f) {
            if (const auto t = before / (before - after); t <= result) {
                result = t;
            }
        }
    }
    return result;
}

// 0x429520
void CCarCtrl::TestWhetherToFirePlaneGuns(CVehicle* vehicle, CEntity* target) {
    vehicle->vehicleFlags.bFireGun = false;

    if (vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_NOT_USED && vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_HEAVY_GUN) {
        return;
    }
    if (!target) {
        return;
    }

    // Shoots when the target is close and right in front of the plane
    auto toTarget = target->GetPosition() - vehicle->GetPosition();
    if (toTarget.Magnitude() < 150.0f) {
        toTarget.Normalise();
        if (DotProduct(toTarget, vehicle->GetForward()) > 0.8f) {
            vehicle->vehicleFlags.bFireGun = true;
        }
    }
}

// 0x421FE0
bool CCarCtrl::ThisVehicleShouldTryNotToTurn(CVehicle* vehicle) {
    // The long ones
    switch (vehicle->m_nModelIndex) {
    case MODEL_LINERUN:
    case MODEL_DUMPER:
    case MODEL_BUS:
    case MODEL_COACH:
    case MODEL_PACKER:
    case MODEL_FLATBED:
    case MODEL_PETRO:
    case MODEL_RDTRAIN:
    case MODEL_CEMENT:
        return true;
    default:
        return false;
    }
}

// 0x429300
void CCarCtrl::TriggerDogFightMoves(CVehicle* vehicle1, CVehicle* vehicle2) {
    auto& ap = vehicle1->m_autoPilot;
    if (ap.m_nTempAction != TEMPACT_NONE) {
        return;
    }

    auto offset = vehicle1->GetPosition() - vehicle2->GetPosition();
    if (offset.Magnitude() >= 70.0f) {
        return;
    }
    offset.Normalise();

    // Positive if vehicle 1 is in front of vehicle 2
    const auto inFront = offset.Dot(vehicle2->GetForward());
    if (std::abs(vehicle1->GetPosition().z - vehicle2->GetPosition().z) >= 15.0f) {
        return;
    }

    switch (CGeneral::GetRandomNumber() & 0xFF) {
    case 0xC:
        if (inFront > 0.0f) {
            ap.m_nTempAction     = TEMPACT_PLANE_FLY_STRAIGHT;
            ap.m_nTempActionTime = (CGeneral::GetRandomNumber() & 0x3FF) + 1500 + CTimer::GetTimeInMS();
        }
        break;
    case 0xD:
        if (inFront > 0.0f) {
            ap.m_nTempAction     = TEMPACT_PLANE_SHARP_LEFT;
            ap.m_nTempActionTime = (CGeneral::GetRandomNumber() & 0x1FF) + 700 + CTimer::GetTimeInMS();
        }
        break;
    case 0xE:
        if (inFront > 0.0f) {
            ap.m_nTempAction     = TEMPACT_PLANE_SHARP_RIGHT;
            ap.m_nTempActionTime = (CGeneral::GetRandomNumber() & 0x1FF) + 700 + CTimer::GetTimeInMS();
        }
        break;
    case 0xF:
        if (inFront > 0.7f) {
            ap.m_nTempAction     = TEMPACT_PLANE_FLY_UP;
            ap.m_nTempActionTime = (CGeneral::GetRandomNumber() & 0x7FF) + 3000 + CTimer::GetTimeInMS();
        }
        break;
    default:
        break;
    }
}

// 0x424000
void CCarCtrl::UpdateCarCount(CVehicle* vehicle, uint8 bDecrease) {
    // The counts never go under zero when a vehicle is removed
    const auto Decrement = [](auto& count) {
        count--;
        if (static_cast<int32>(count) < 0) {
            count = 0;
        }
    };

    if (!bDecrease) {
        switch (vehicle->m_nCreatedBy) {
        case RANDOM_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) {
                NumLawEnforcerCars++;
            }
            NumRandomCars++;
            break;
        case MISSION_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) {
                vehicle->vehicleFlags.bIsLawEnforcer = false;
                NumLawEnforcerCars--;
            }
            NumMissionCars++;
            break;
        case PARKED_VEHICLE:
            NumParkedCars++;
            break;
        case PERMANENT_VEHICLE:
            NumPermanentVehicles++;
            break;
        default:
            break;
        }
    } else {
        switch (vehicle->m_nCreatedBy) {
        case RANDOM_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) {
                Decrement(NumLawEnforcerCars);
            }
            Decrement(NumRandomCars);
            break;
        case MISSION_VEHICLE:
            Decrement(NumMissionCars);
            break;
        case PARKED_VEHICLE:
            Decrement(NumParkedCars);
            break;
        case PERMANENT_VEHICLE:
            Decrement(NumPermanentVehicles);
            break;
        default:
            break;
        }
    }
}

// 0x436540
void CCarCtrl::UpdateCarOnRails(CVehicle* vehicle) {
    plugin::Call<0x436540, CVehicle*>(vehicle);
}


// Widens the range of angles the vehicle can't steer towards, if the blocker (seen at `heading`, taking up `halfAngle` on both sides) overlaps with it
static void ExtendBlockedAngles(float heading, float halfAngle, float* minAngle, float* maxAngle) {
    // Brings an angle into [-pi, pi]
    const auto Wrap = [](float angle) {
        while (angle < -3.1415927f) {
            angle += 6.2831855f;
        }
        while (angle > 3.1415927f) {
            angle -= 6.2831855f;
        }
        return angle;
    };

    if (std::abs(Wrap(heading - *minAngle)) < halfAngle) {
        *minAngle = heading - halfAngle;
        while (*minAngle < -3.1415927f) {
            *minAngle += 6.2831855f;
        }
    }
    if (std::abs(Wrap(heading - *maxAngle)) < halfAngle) {
        *maxAngle = halfAngle + heading;
        while (*maxAngle > 3.1415927f) {
            *maxAngle -= 6.2831855f;
        }
    }
}

// 0x426970
void CCarCtrl::WeaveForPed(CPed* ped, CVehicle* vehicle, float* arg3, float* arg4) {
    // Vehicles ramming or hunting this very ped don't try to go around it
    const auto mission = vehicle->m_autoPilot.m_nCarMission;
    if ((mission == MISSION_RAMPLAYER_CLOSE && ped == FindPlayerPed())
        || (mission == MISSION_KILLPED_CLOSE && (CEntity*)ped == (CEntity*)vehicle->m_autoPilot.m_TargetEntity)
    ) {
        return;
    }

    const auto delta   = ped->GetPosition() - vehicle->GetPosition();
    const auto heading = CGeneral::GetATanOfXY(delta.x, delta.y);
    const auto dist    = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist < 1.0f) {
        return;
    }

    // Half of the angle the vehicle takes up when seen from the ped
    const auto halfAngle = ((CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.x * 2.4f + 0.8f) / dist) * 0.5f;
    ExtendBlockedAngles(heading, halfAngle, arg3, arg4);
}

// 0x426BC0
void CCarCtrl::WeaveForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float* arg4) {
    // Where the part that's in the way is, relative to the object (right, forward)
    float right{}, forward{};
    const auto modelIndex = entity->m_nModelIndex;
    if (modelIndex == ModelIndices::MI_TRAFFICLIGHTS) {
        right   = 2.957f;
        forward = 0.147f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS1) {
        right = 0.744f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS2) {
        right = 0.043f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS3) {
        right   = 1.143f;
        forward = 0.145f;
    } else if (modelIndex == ModelIndices::MI_DOUBLESTREETLIGHTS) {
        forward = -0.048f;
    } else if (!CModelInfo::GetModelInfo(modelIndex)->SwaysInWind()) {
        return;
    }

    const auto& mat    = entity->GetMatrix();
    const auto& pos    = entity->GetPosition();
    const auto  delta  = CVector2D{
        right * mat.GetRight().x + forward * mat.GetForward().x + pos.x,
        forward * mat.GetForward().y + right * mat.GetRight().y + pos.y
    } - CVector2D{ vehicle->GetPosition() };
    const auto heading = CGeneral::GetATanOfXY(delta.x, delta.y);

    const auto halfAngle = ((CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.x * 2.4f + 0.3f) / delta.Magnitude()) * 0.5f;
    ExtendBlockedAngles(heading, halfAngle, arg3, arg4);
}

// 0x426350
void CCarCtrl::WeaveForOtherCar(CEntity* entity, CVehicle* vehicle, float* arg3, float* arg4) {
    // Tests if the vehicle would hit the other one when it goes in the direction of the angle (0x421A50). Each one is described by two corners and the edge that goes from each
    const auto IsAngleBlocked = [](float angle, CVector* corner1, CVector* edge1, CVector* corner2, CVector* edge2, CVector* corner3, CVector* edge3, CVector* corner4, CVector* edge4, float speedX, float speedY, float speed, bool narrower) {
        return plugin::CallAndReturn<char, 0x421A50, float, CVector*, CVector*, CVector*, CVector*, CVector*, CVector*, CVector*, CVector*, float, float, float, bool>(angle, corner1, edge1, corner2, edge2, corner3, edge3, corner4, edge4, speedX, speedY, speed, narrower) != 0;
    };

    // Vehicles that are meant to be hit, or are chasing this one, are not avoided
    const auto  mission = vehicle->m_autoPilot.m_nCarMission;
    const auto* target  = vehicle->m_autoPilot.m_TargetEntity;
    const auto* otherVeh = static_cast<CVehicle*>(entity);
    if ((mission == MISSION_RAMPLAYER_CLOSE && entity == FindPlayerVehicle(-1, false))
        || (mission == MISSION_RAMCAR_CLOSE && entity == target)
        || (mission == MISSION_FOLLOWCAR_CLOSE && (entity == target || (entity->GetIsTypeVehicle() && otherVeh->vehicleFlags.bPartOfConvoy)))
        || (mission == MISSION_KILLPED_CLOSE && entity->GetIsTypePed() && static_cast<CPed*>(entity)->bInVehicle && static_cast<CPed*>(entity)->m_pVehicle == target)
    ) {
        return;
    }
    switch (otherVeh->m_autoPilot.m_nCarMission) {
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT:
        if (otherVeh->m_autoPilot.m_TargetEntity == vehicle) {
            return;
        }
        break;
    }

    const auto& entityMat = entity->GetMatrix();
    const auto& vehMat    = vehicle->GetMatrix();

    const auto toEntity = entity->GetPosition() - vehicle->GetPosition();

    // Only when the other one is in front of the vehicle
    const auto vehForwardDot = toEntity.x * vehMat.GetForward().x + toEntity.y * vehMat.GetForward().y;
    if (vehForwardDot < 0.0f) {
        return;
    }
    const auto vehRightDot = toEntity.x * vehMat.GetRight().x + toEntity.y * vehMat.GetRight().y;

    const auto& vehBox    = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox();
    const auto& entityBox = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->GetBoundingBox();
    constexpr auto MARGIN = 0.2f;

    // This vehicle
    auto vehCorner1 = vehMat.TransformPoint({ vehBox.m_vecMin.x - MARGIN, vehForwardDot <= 0.0f ? vehBox.m_vecMin.y - MARGIN : vehBox.m_vecMax.y + MARGIN, 0.0f });
    auto vehEdge1   = vehMat.TransformVector({ ((vehBox.m_vecMax.x + MARGIN) - vehBox.m_vecMin.x) - MARGIN, 0.0f, 0.0f });
    auto vehCorner2 = vehMat.TransformPoint({ vehRightDot <= 0.0f ? vehBox.m_vecMin.x - MARGIN : vehBox.m_vecMax.x + MARGIN, vehBox.m_vecMin.y - MARGIN, 0.0f });
    auto vehEdge2   = vehMat.TransformVector({ 0.0f, ((vehBox.m_vecMax.y + MARGIN) - vehBox.m_vecMin.y) - MARGIN, 0.0f });

    // The other one
    const auto entityRightDot   = toEntity.x * entityMat.GetRight().x + toEntity.y * entityMat.GetRight().y;
    const auto entityForwardDot = toEntity.x * entityMat.GetForward().x + toEntity.y * entityMat.GetForward().y;
    auto entityCorner1 = entityMat.TransformPoint({ entityBox.m_vecMin.x - MARGIN, entityForwardDot >= 0.0f ? entityBox.m_vecMin.y - MARGIN : entityBox.m_vecMax.y + MARGIN, 0.0f });
    auto entityEdge1   = entityMat.TransformVector({ ((entityBox.m_vecMax.x + MARGIN) - entityBox.m_vecMin.x) - MARGIN, 0.0f, 0.0f });
    auto entityCorner2 = entityMat.TransformPoint({ entityRightDot >= 0.0f ? entityBox.m_vecMin.x - MARGIN : entityBox.m_vecMax.x + MARGIN, entityBox.m_vecMin.y - MARGIN, 0.0f });
    auto entityEdge2   = entityMat.TransformVector({ 0.0f, ((entityBox.m_vecMax.y + MARGIN) - entityBox.m_vecMin.y) - MARGIN, 0.0f });

    const auto narrower = vehBox.m_vecMax.x < entityBox.m_vecMax.x;
    const auto speed    = std::sqrt(vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y + vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x);
    const auto& entitySpeed = entity->AsPhysical()->m_vecMoveSpeed;

    const auto Blocked = [&](float angle) {
        return IsAngleBlocked(angle, &vehCorner1, &vehEdge1, &vehCorner2, &vehEdge2, &entityCorner1, &entityEdge1, &entityCorner2, &entityEdge2, entitySpeed.x, entitySpeed.y, speed, narrower);
    };

    // Turn the limits away from the other vehicle in steps of 6 degrees
    constexpr auto STEP = 0.10471976f;
    for (auto i = 0; i < 8 && Blocked(*arg3); i++) {
        *arg3 -= STEP;
        if (*arg3 < 0.0f) {
            *arg3 += TWO_PI;
        }
    }
    for (auto i = 0; Blocked(*arg4);) {
        *arg4 += STEP;
        if (*arg4 > TWO_PI) {
            *arg4 -= TWO_PI;
        }
        if (++i > 7) {
            return;
        }
    }
}

// How far above or below the vehicle (in Z) an entity is
static float AbsHeightDifference(const CEntity* entity, const CVehicle* vehicle) {
    return std::abs(entity->GetPosition().z - vehicle->GetPosition().z);
}

// 0x42D680
template<typename PtrListType>
void CCarCtrl::WeaveThroughCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* physical, float minX, float minY, float maxX, float maxY, float* arg8, float* arg9) {
    for (typename PtrListType::NodeType *node = ptrList.GetNode(), *next{}; node; node = next) {
        next = node->Next;

        const auto entity = static_cast<CVehicle*>(node->Item);
        if (entity->IsScanCodeCurrent() || !entity->GetUsesCollision() || entity == physical) {
            continue;
        }
        entity->SetCurrentScanCode();

        const auto center = entity->GetBoundCentre();
        if (!(minX < center.x && center.x < maxX && minY < center.y && center.y < maxY)) {
            continue;
        }
        if (AbsHeightDifference(entity, vehicle) < 8.0f
            && entity != vehicle
            && (!vehicle->vehicleFlags.bPartOfConvoy || !entity->vehicleFlags.bPartOfConvoy)
        ) {
            WeaveForOtherCar(entity, vehicle, arg8, arg9);
        }
    }
}

// 0x42D950
template<typename PtrListType>
void CCarCtrl::WeaveThroughObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* arg7, float* arg8) {
    for (typename PtrListType::NodeType *node = ptrList.GetNode(), *next{}; node; node = next) {
        next = node->Next;

        const auto entity = static_cast<CEntity*>(node->Item);
        if (entity->IsScanCodeCurrent() || !entity->GetUsesCollision()) {
            continue;
        }
        entity->SetCurrentScanCode();

        const auto& pos = entity->GetPosition();
        if (!(minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY)) {
            continue;
        }
        // Only upright objects are in the way
        if (AbsHeightDifference(entity, vehicle) < 8.0f && entity->GetMatrix().GetUp().z > 0.9f) {
            WeaveForObject(entity, vehicle, arg7, arg8);
        }
    }
}

// 0x42D7E0
template<typename PtrListType>
void CCarCtrl::WeaveThroughPedsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* physical, float minX, float minY, float maxX, float maxY, float* arg8, float* arg9) {
    for (typename PtrListType::NodeType *node = ptrList.GetNode(), *next{}; node; node = next) {
        next = node->Next;

        const auto ped = static_cast<CPed*>(node->Item);
        if (ped->IsScanCodeCurrent() || !ped->GetUsesCollision() || ped == physical) {
            continue;
        }
        ped->SetCurrentScanCode();

        const auto& pos = ped->GetPosition();
        if (!(minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY)) {
            continue;
        }
        // Peds standing on or next to the vehicle don't count
        if (AbsHeightDifference(ped, vehicle) < 4.0f && ped->m_pContactEntity != vehicle && ped->m_pAttachedTo != vehicle) {
            WeaveForPed(ped, vehicle, arg8, arg9);
        }
    }
}

// 0x427FE0
float CCarCtrl::FindMaxSteerAngle(CVehicle* veh) {
    return std::clamp(0.9f - veh->GetMoveSpeed().Magnitude(), 0.2f, 0.7f);
}
