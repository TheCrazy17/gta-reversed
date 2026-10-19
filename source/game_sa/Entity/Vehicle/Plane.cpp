#include "StdInc.h"

#include "Plane.h"
#include "Events/EventDanger.h"
#include "CarCtrl.h"
#include "FireManager.h"

auto& HARRIER_NOZZLE_ROTATERATE = StaticRef<float>(0x8D33DC);       // 25.0f
auto& PLANE_DAMAGE_WAVE_COUNTER_VAR = StaticRef<float>(0x8D33E0);   // 0.75f
auto& PLANE_DAMAGE_THRESHHOLD = StaticRef<float>(0x8D33E4);         // 500.0f
auto& PLANE_DAMAGE_SCALE_MASS = StaticRef<float>(0x8D33E8);         // 10000.0f
auto& PLANE_DAMAGE_DESTROY_THRESHHOLD = StaticRef<float>(0x8D33EC); // 5000.0f
auto& vecRCBaronGunPos = StaticRef<CVector>(0x8D33F0);            // <0.0f, 0.45f, 0.0f>

void CPlane::InjectHooks() {
    RH_ScopedVirtualClass(CPlane, 0x871948, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6C8E20);
    RH_ScopedInstall(InitPlaneGenerationAndRemoval, 0x6CAD90);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C9140);
    RH_ScopedVMTInstall(BurstTyre, 0x6C9150);
    RH_ScopedVMTInstall(PreRender, 0x6C94A0, { .Reversed = false });
    RH_ScopedVMTInstall(Render, 0x6CAB70);
    RH_ScopedInstall(IsAlreadyFlying, 0x6CAB90);
    RH_ScopedVMTInstall(Fix, 0x6CABB0);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6CAC10);
    RH_ScopedInstall(SetGearUp, 0x6CAC20);
    RH_ScopedInstall(SetGearDown, 0x6CAC70);
    RH_ScopedVMTInstall(OpenDoor, 0x6CACB0);
    RH_ScopedVMTInstall(ProcessControl, 0x6C9260);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6CADD0, { .Reversed = false });
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6CB7C0, { .Reversed = false });
    RH_ScopedVMTInstall(VehicleDamage, 0x6CC4B0, { .Reversed = false });
    RH_ScopedInstall(CountPlanesAndHelis, 0x6CCA50);
    RH_ScopedInstall(AreWeInNoPlaneZone, 0x6CCAA0);
    RH_ScopedInstall(AreWeInNoBigPlaneZone, 0x6CCBB0);
    RH_ScopedInstall(SwitchAmbientPlanes, 0x6CCC50);
    RH_ScopedVMTInstall(BlowUpCar, 0x6CCCF0);
    RH_ScopedInstall(FindPlaneCreationCoors, 0x6CD090);
    RH_ScopedInstall(DoPlaneGenerationAndRemoval, 0x6CD2F0);
}

// 0x6C8E20
CPlane::CPlane(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_PLANE;

    m_fLeftRightSkid               = 0.0f;
    m_fSteeringUpDown              = 0.0f;
    m_fSteeringLeftRight           = 0.0f;
    m_fAccelerationBreakStatus     = 0.0f;
    m_fAccelerationBreakStatusPrev = 1.0f;
    m_fPropSpeed                   = 0.0f;
    field_9C8                      = 0.0f;
    m_LandingGearAngle             = 0.0f;
    m_StallCounter                 = 0;
    m_planeCreationHeading         = 0.0f;
    m_planeHeading                 = 0.0f;
    m_planeHeadingPrev             = 0.0f;
    m_maxAltitude                  = 15.0f;
    m_altitude                     = 25.0f;
    m_minAltitude                  = 20.0f;
    m_forwardZ                     = 0;
    m_nStartedFlyingTime           = 0;
    m_fSteeringFactor              = 0.0f;

    if (m_nModelIndex != MODEL_VORTEX)
        physicalFlags.bDontCollideWithFlyers = true;

    m_nExtendedRemovalRange = 255;
    vehicleFlags.bNeverUseSmallerRemovalRange = true;
    vehicleFlags.bIsBig = true;

    auto& leftDoor = m_doors[DOOR_LEFT_FRONT];
    switch (modelIndex) {
    case MODEL_HYDRA:
    case MODEL_RUSTLER:
    case MODEL_CROPDUST:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        break;
    case MODEL_SHAMAL:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-((3.0f * PI) / 4.0f), 0.0f, DOOR_AXIS_Z, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LF]), 0);
        break;
    case MODEL_NEVADA:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-TWO_PI / 5.0f, 0.0f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
        break;
    case MODEL_VORTEX:
        if (m_panels[FRONT_LEFT_PANEL].m_nFrameId == (uint16)-1)
            m_panels[FRONT_LEFT_PANEL].SetPanel(PLANE_GEAR_L, 1, -0.25f);
        break;
    case MODEL_STUNT:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LB]), 0);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_RB]), 0);
        break;
    }

    CVector modelPos, localPos;
    for (auto wheelId = 0; wheelId < 4; wheelId++) {
        GetVehicleModelInfo()->GetWheelPosn(wheelId, modelPos, false);
        GetVehicleModelInfo()->GetWheelPosn(wheelId, localPos, true);
        m_wheelPosition[wheelId] = m_wheelPosition[wheelId] - modelPos.z + localPos.z;
    }

    m_planeDamageWave = 0;
    m_pGunParticles = nullptr;
    m_nFiringMultiplier = 16;
    field_9DC = 0;
    field_9E0 = 0;
    m_apJettrusParticles.fill(nullptr);

    m_pSmokeParticle = nullptr;

    if (m_nModelIndex == MODEL_HYDRA)
        m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;

    m_bSmokeEjectorEnabled = false;
}

// 0x6C9160
CPlane::~CPlane() {
    if (m_pGunParticles) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& particle = m_pGunParticles[i]) {
                particle->Kill();
                g_fxMan.DestroyFxSystem(particle);
            }
        }
        delete[] m_pGunParticles;
        m_pGunParticles = nullptr;
    }

    for (auto particle : m_apJettrusParticles) {
        if (particle) {
            FxSystem_c::KillAndClear(particle);
        }
    }

    FxSystem_c::SafeKillAndClear(m_pSmokeParticle);

    m_vehicleAudio.Terminate();
}

// 0x6CAD90
void CPlane::InitPlaneGenerationAndRemoval() {
    GenPlane_Status = 0;
    GenPlane_LastTimeGenerated = 0;
    GenPlane_Active = true;
}

// 0x6CCCF0
void CPlane::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    // Planes that are not being flown, or are not yet falling down, first crash and burn
    if (GetStatus() != STATUS_PLAYER && m_autoPilot.m_nCarMission != MISSION_PLANE_CRASH_AND_BURN && m_nModelIndex != MODEL_RCBARON) {
        m_autoPilot.m_nCarMission = MISSION_PLANE_CRASH_AND_BURN;
        m_fHealth                 = 0.0f;
        return;
    }

    if (damager == FindPlayerPed(-1) || damager == FindPlayerVehicle(-1, false)) {
        auto& playerInfo = CWorld::Players[CWorld::PlayerInFocus];
        playerInfo.m_nHavocCaused += 20;
        playerInfo.m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, static_cast<float>(CGeneral::GetRandomNumber() % 6000 + 4000));
    }

    if (GetStatus() == STATUS_PLAYER) {
        if (m_pDriver) {
            m_pDriver->bDontRender = true;
        }
        for (auto* const passenger : m_apPassengers) {
            if (passenger) {
                passenger->bDontRender = true;
            }
        }
        m_bUsesCollision = false;
        m_bIsVisible     = false;
        ResetMoveSpeed();
        ResetTurnSpeed();
    }

    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp           = CTimer::GetTimeInMS();
    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), ATOMIC_PIPE_NO_EXTRA_PASSES);
    m_damageManager.FuckCarCompletely(false);

    if (m_nModelIndex != MODEL_RCBARON) {
        SetBumperDamage(FRONT_BUMPER, false);
        SetBumperDamage(REAR_BUMPER, false);
        SetDoorDamage(DOOR_BONNET, false);
        SetDoorDamage(DOOR_BOOT, false);
        SetDoorDamage(DOOR_LEFT_FRONT, false);
        SetDoorDamage(DOOR_RIGHT_FRONT, false);
        SetDoorDamage(DOOR_LEFT_REAR, false);
        SetDoorDamage(DOOR_RIGHT_REAR, false);
        SpawnFlyingComponent(static_cast<eCarNodes>(5), 1);

        // The component that was thrown off stays on the plane, hide it
        if (auto* const node = m_aCarNodes[5]) {
            RpAtomic* atomic = nullptr;
            RwFrameForAllObjects(node, GetCurrentAtomicObjectCB, &atomic);
            if (atomic) {
                rwObjectSetFlags(atomic, 0);
            }
        }
    }

    m_nBombOnBoard   = 0;
    m_fHealth        = 0.0f;
    m_wBombTimer     = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    m_nOverrideLights               = 0;
    vehicleFlags.bEngineOn          = false;
    vehicleFlags.bLightsOn          = false;
    m_bSmokeEjectorEnabled          = false;
    vehicleFlags.bTakeLessDamage    = false;
    autoFlags.bTaxiLight            = false;

    if (vehicleFlags.bIsAmbulanceOnDuty) {
        vehicleFlags.bIsAmbulanceOnDuty = false;
        CCarCtrl::NumAmbulancesOnDuty--;
    }
    if (vehicleFlags.bIsFireTruckOnDuty) {
        vehicleFlags.bIsFireTruckOnDuty = false;
        CCarCtrl::NumFireTrucksOnDuty--;
    }

    ChangeLawEnforcerState(false);
    gFireManager.StartFire(this, damager, 0.8f, true, 7000, 0);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);

    CExplosion::AddExplosion(this, damager, m_nModelIndex == MODEL_RCBARON ? EXPLOSION_RC_VEHICLE : EXPLOSION_AIRCRAFT, GetPosition(), 0, true, -1.0f, false);
}

// 0x6CABB0
void CPlane::Fix() {
    m_damageManager.ResetDamageStatus();
    if (m_pHandlingData->m_bNoDoors) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_LEFT_REAR, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_REAR, DAMSTATE_NOTPRESENT);
    }
    SetupDamageAfterLoad();
}

// 0x6CACB0
void CPlane::OpenDoor(CPed* ped, int32 componentId, eDoors door, float doorOpenRatio, bool playSound) {
    CAutomobile::OpenDoor(ped, componentId, door, doorOpenRatio, playSound);

    if (m_nModelIndex != MODEL_STUNT)
        return;

    // Unfinished code R*, which removed in Android
    if (false) // byte_C1CAFC
    {
        CMatrix matrix(RwFrameGetMatrix(m_aCarNodes[componentId]), false);
        const auto y = m_doors[door].m_angle - m_doors[door].m_prevAngle + matrix.GetPosition().y;
        matrix.SetTranslate({matrix.GetPosition().x, y, matrix.GetPosition().z});
        matrix.UpdateRW();
    }
}

// 0x6CAC10
void CPlane::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6CC4B0
void CPlane::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    plugin::CallMethod<0x6CC4B0, CPlane*, float, eVehicleCollisionComponent, CEntity*, CVector*, CVector*, eWeaponType>(this, damageIntensity, component, damager, vecCollisionCoors, vecCollisionDirection, weapon);
}

// 0x6CAB90
void CPlane::IsAlreadyFlying() {
    m_nStartedFlyingTime = CTimer::GetTimeInMS() - 20000;
}

// 0x6CAC20
void CPlane::SetGearUp() {
    m_LandingGearAngle = 1.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f * m_pFlyingHandlingData->m_fGearUpR;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_MISSING);
}

// 0x6CAC70
void CPlane::SetGearDown() {
    m_LandingGearAngle = 0.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_OK);
}

// 0x6CCA50
uint32 CPlane::CountPlanesAndHelis() {
    uint32 counter = 0;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsSubHeli() || vehicle.IsSubPlane()) {
            counter++;
        }
    }
    return counter;
}

// 0x6CCAA0
bool CPlane::AreWeInNoPlaneZone() {
    const auto& camPos = TheCamera.GetPosition();
    constexpr CVector vec1 = { -1073.0f, -675.0f, 50.0f };

    return DistanceBetweenPoints(vec1, camPos) < 200.0f ||
           camPos.x > -2743.0f && camPos.x < -2626.0f && camPos.y > 1300.0f && camPos.y < 2200.0f || // todo: Is point inside
           camPos.x > -1668.0f && camPos.x < -1122.0f && camPos.y > 541.0f && camPos.y < 1118.0f;
}

// 0x6CCBB0
bool CPlane::AreWeInNoBigPlaneZone() {
    // untested
    const auto& camPos = TheCamera.GetPosition();
    return DistanceBetweenPoints2D({ +1522.0f, -1237.0f }, camPos) < 800.0f ||
           DistanceBetweenPoints2D({ -1836.0f, +659.0f }, camPos) < 800.0f;
}

// 0x6CCC50
void CPlane::SwitchAmbientPlanes(bool enable) {
    if (GenPlane_Active && !enable) {
        for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
            if ((vehicle.IsSubHeli() || vehicle.IsSubPlane()) && vehicle.m_nCreatedBy == RANDOM_VEHICLE) {
                CWorld::Remove(&vehicle);
                delete &vehicle;
            }
        }
    }
    GenPlane_Active = enable;
}

// 0x6CD090
void CPlane::FindPlaneCreationCoors(CVector* outCoors, CVector* outTargetCoors, float* outPlaneOrientation, float* outFlightHeight, bool isBigPlane) {
    const auto radius     = isBigPlane ? 340.0f : 140.0f;
    const auto baseHeight = isBigPlane ? 200.0f : 25.0f;

    for (auto attempt = 0; attempt < 500; attempt += 10) {
        // Position on a circle around the player, with a random height
        const auto angle = static_cast<float>(CGeneral::GetRandomNumber() % 360) * 0.017453292f;
        *outCoors        = FindPlayerCoors(-1);
        *outFlightHeight = static_cast<float>(CGeneral::GetRandomNumber() & 0xF) + static_cast<float>(attempt) + baseHeight;
        outCoors->x += std::cos(angle) * radius;
        outCoors->y += std::sin(angle) * radius;
        outCoors->z += *outFlightHeight;

        // The plane heads for a point ahead of where the camera looks
        *outTargetCoors = FindPlayerCoors(-1);
        const auto ahead = static_cast<float>(CGeneral::GetRandomNumber() & 0x1F) + 20.0f;
        const auto& camForward = TheCamera.m_mCameraMatrix.GetForward();
        outTargetCoors->x = camForward.x * ahead + outTargetCoors->x;
        outTargetCoors->y = camForward.y * ahead + outTargetCoors->y;
        outTargetCoors->z = camForward.z * ahead + outTargetCoors->z;
        outTargetCoors->z += *outFlightHeight;

        *outPlaneOrientation = CGeneral::GetATanOfXY(outTargetCoors->x - outCoors->x, outTargetCoors->y - outCoors->y);

        // The line of sight is tested past the target
        const auto lineEnd = *outTargetCoors + (*outTargetCoors - *outCoors);
        CWorld::AdvanceCurrentScanCode();

        CColSphere sphere;
        sphere.Set(15.0f, *outCoors, SURFACE_DEFAULT);
        if (!CWorld::GetIsLineOfSightClear(*outCoors, lineEnd, true, false, false, false, false, false, false)) {
            continue;
        }

        // The original passes the world position instead of a sector, and a box it never filled in
        const CColBox box{};
        if (!CCollision::CheckCameraCollisionBuildings(static_cast<int32>(outCoors->x), static_cast<int32>(outCoors->y), box, sphere, sphere, sphere)) {
            break;
        }
    }

    *outFlightHeight += 20.0f;
    outTargetCoors->z += 20.0f;
}

// 0x6CD2F0
void CPlane::DoPlaneGenerationAndRemoval() {
    if ((CTimer::GetFrameCounter() & 0x1F) != 0x1E) {
        return;
    }
    if (!GenPlane_Active) {
        return;
    }

    const auto numPlanesAndHelis = CountPlanesAndHelis();

    // Resets the heading of an aircraft that was just created, (0x6C4130)
    const auto SetHeadingMatrix = [](CVehicle& vehicle, float heading) {
        auto& matrix = vehicle.GetMatrix();
        matrix.GetRight()   = CVector{ std::sin(heading), -std::cos(heading), 0.0f };
        matrix.GetForward() = CVector{ std::cos(heading),  std::sin(heading), 0.0f };
        matrix.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
    };

    // Sends the plane or helicopter that was just created on its way
    const auto SetCruiseVelocity = [](CVehicle& vehicle, float heading) {
        vehicle.SetVelocity(CVector{ std::cos(heading) * 0.6666667f, std::sin(heading) * 0.6666667f, 0.0f });
    };

    switch (GenPlane_Status) {
    case 0: { // Choose what to generate next
        if (CGame::currArea != AREA_CODE_NORMAL_WORLD || numPlanesAndHelis >= 5 || AreWeInNoPlaneZone() || CCutsceneMgr::ms_cutsceneProcessing) {
            GenPlane_LastTimeGenerated = CTimer::GetTimeInMS();
            return;
        }

        const auto timeMS = CTimer::GetTimeInMS();

        // A Hydra is sent after a player who is flying and has a high wanted level
        if (static_cast<int32>(FindPlayerPed(-1)->GetWantedLevel()) > 3) {
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                if ((playerVehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE || playerVehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI)
                    && playerVehicle->m_nModelIndex != MODEL_ANDROM
                    && playerVehicle->m_nModelIndex != MODEL_AT400
                    && timeMS > GenPlane_LastTimeGenerated + 60'000
                ) {
                    int32 numHydras = 0;
                    for (auto i = GetVehiclePool()->GetSize(); i-- > 0;) {
                        auto* const vehicle = GetVehiclePool()->GetAt(i);
                        if (vehicle && vehicle->m_autoPilot.m_nCarMission == MISSION_PLANE_ATTACK_PLAYER_POLICE) {
                            numHydras++;
                        }
                    }
                    if (numHydras < 1 || (numHydras < 2 && static_cast<int32>(FindPlayerPed(-1)->GetWantedLevel()) > 4)) {
                        GenPlane_ModelIndex = MODEL_HYDRA;
                        GenPlane_Status     = 1;
                    }
                }
            }
        }

        if (numPlanesAndHelis >= 1 || GenPlane_Status != 0) {
            return;
        }

        const auto weatherAllowsSmallAircraft = (CWeather::WeatherRegion == WEATHER_REGION_DEFAULT || CWeather::WeatherRegion == WEATHER_REGION_DESERT)
            && CWeather::OldWeatherType != WEATHER_SANDSTORM_DESERT
            && CWeather::NewWeatherType != WEATHER_SANDSTORM_DESERT;
        if (weatherAllowsSmallAircraft) {
            // Small aircraft are not sent while the player is flying one
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            const auto  playerFlying  = playerVehicle && (playerVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI || playerVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE);
            if (!playerFlying && timeMS > GenPlane_LastTimeGenerated + 120'000) {
                switch (CGeneral::GetRandomNumber() & 7) {
                case 3:
                case 4:
                    GenPlane_ModelIndex = MODEL_CROPDUST;
                    break;
                case 5:
                case 6:
                    GenPlane_ModelIndex = MODEL_BEAGLE;
                    break;
                default:
                    GenPlane_ModelIndex = MODEL_RUSTLER;
                    break;
                }
                GenPlane_Status = 1;
                return;
            }
            if (GenPlane_Status != 0) {
                return;
            }
        }

        // Police helicopters in gang areas, and when there are riots
        if (CPopCycle::m_bCurrentZoneIsGangArea) {
            if (!CTheScripts::IsPlayerOnAMission() && timeMS > GenPlane_LastTimeGenerated + 350'000) {
                GenPlane_Status     = 1;
                GenPlane_ModelIndex = MODEL_POLMAV;
                return;
            }
            if (GenPlane_Status != 0) {
                return;
            }
        }
        if (CGameLogic::LaRiotsActiveHere()) {
            GenPlane_Status     = 1;
            GenPlane_ModelIndex = MODEL_POLMAV;
            return;
        }
        if (GenPlane_Status != 0) {
            return;
        }

        // Big planes
        if (CWeather::WeatherRegion != WEATHER_REGION_DEFAULT && CWeather::WeatherRegion != WEATHER_REGION_DESERT
            && !CPopCycle::m_bCurrentZoneIsGangArea
            && CWeather::OldWeatherType != WEATHER_SANDSTORM_DESERT
            && CWeather::NewWeatherType != WEATHER_SANDSTORM_DESERT
            && timeMS > GenPlane_LastTimeGenerated + 200'000
            && !AreWeInNoBigPlaneZone()
        ) {
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (playerVehicle && (playerVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI || playerVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE)) {
                return;
            }
            GenPlane_ModelIndex = (CGeneral::GetRandomNumber() & 1) == 0 ? MODEL_AT400 : MODEL_ANDROM;
            GenPlane_Status     = 1;
        }
        return;
    }
    case 1: { // Wait for the model and create the aircraft
        if (!CStreaming::GetInfo(GenPlane_ModelIndex).IsLoaded()) {
            CStreaming::RequestModel(GenPlane_ModelIndex, STREAMING_KEEP_IN_MEMORY);
            return;
        }

        if (GenPlane_ModelIndex == MODEL_POLMAV) {
            CVector pos, target;
            float   heading;
            float   flightHeight;
            FindPlaneCreationCoors(&pos, &target, &heading, &flightHeight, false);

            auto* const heli = new CHeli(GenPlane_ModelIndex, RANDOM_VEHICLE);
            heli->SetPosn(pos);
            SetHeadingMatrix(*heli, heading);
            CCarCtrl::JoinCarWithRoadSystem(heli);
            heli->m_fHeliRotorSpeed = 0.22f;
            heli->SetStatus(STATUS_PHYSICS);
            CWorld::Add(heli);

            heli->m_fMinAltitude                = flightHeight;
            heli->m_autoPilot.m_nCarMission     = MISSION_HELI_FLYINDIRECTION;
            heli->field_9B4                     = heading;
            heli->m_fMaxAltitude                = flightHeight;
            SetCruiseVelocity(*heli, heading);
            heli->vehicleFlags.bNeverUseSmallerRemovalRange = true;
            heli->m_nExtendedRemovalRange                   = 0xFF;
            heli->m_autoPilot.m_nCruiseSpeed                = 50;
            heli->SetEngineState(true);

            // The helicopter goes after a car or after a ped, chosen at random
            const auto heliPos = heli->GetPosition();
            const auto AimAt = [&](CEntity* target) {
                heli->m_autoPilot.m_TargetEntity = static_cast<CVehicle*>(target);
                heli->m_autoPilot.m_nCarMission  = MISSION_HELI_FOLLOW_ENTITY;
                target->RegisterReference(reinterpret_cast<CEntity**>(&heli->m_autoPilot.m_TargetEntity));
                heli->m_nHeliFlags |= 2;
                heli->m_autoPilot.m_nCarCtrlFlags |= 0xA0;
            };
            if ((CGeneral::GetRandomNumber() & 1) == 0) {
                if (auto* const car = CWorld::FindUnsuspectingTargetCar(heliPos, FindPlayerCoors(-1))) {
                    AimAt(car);
                }
            } else if (auto* const ped = CWorld::FindUnsuspectingTargetPed(heliPos, FindPlayerCoors(-1))) {
                AimAt(ped);

                auto* const group = CPedGroups::GetPedsGroup(ped);
                if (group && !group->m_bIsMissionGroup) {
                    CEventDanger event{ heli, 200.0f };
                    event.m_TaskId = TASK_GROUP_FLEE_THREAT;
                    group->GetIntelligence().AddEvent(&event);
                } else if (!ped->IsPlayer() && !ped->IsCreatedBy(PED_MISSION) && ped->m_nPedType != PED_TYPE_COP) {
                    CEventDanger event{ heli, 200.0f };
                    event.m_TaskId = TASK_COMPLEX_SMART_FLEE_ENTITY;
                    ped->GetIntelligence()->m_eventGroup.Add(&event, false);
                }
            }
        } else {
            const bool isBigPlane = GenPlane_ModelIndex == MODEL_ANDROM || GenPlane_ModelIndex == MODEL_AT400;

            CVector pos, target;
            float   heading;
            float   flightHeight;
            FindPlaneCreationCoors(&pos, &target, &heading, &flightHeight, isBigPlane);

            auto* const plane = new CPlane(GenPlane_ModelIndex, RANDOM_VEHICLE);
            plane->SetPosn(pos);
            plane->m_planeCreationHeading = heading;
            SetHeadingMatrix(*plane, heading);
            CCarCtrl::JoinCarWithRoadSystem(plane);
            plane->SetStatus(STATUS_PHYSICS);
            CWorld::Add(plane);

            plane->m_planeHeading                 = heading;
            plane->m_minAltitude                  = flightHeight;
            plane->m_autoPilot.m_nCarMission      = MISSION_PLANE_FLYINDIRECTION;
            plane->m_planeHeadingPrev             = heading;
            plane->m_maxAltitude                  = flightHeight;
            SetCruiseVelocity(*plane, heading);
            plane->vehicleFlags.bNeverUseSmallerRemovalRange = true;

            if (!isBigPlane && GenPlane_ModelIndex != MODEL_HYDRA) {
                plane->m_nExtendedRemovalRange = 150;
            } else {
                plane->m_nExtendedRemovalRange = 360;
                if (isBigPlane) {
                    plane->m_bUsesCollision = false;
                }
            }
            if (GenPlane_ModelIndex == MODEL_HYDRA) {
                plane->m_autoPilot.m_nCarMission = MISSION_PLANE_ATTACK_PLAYER_POLICE;
                plane->m_wMiscComponentAngle     = 0;
            }

            plane->SetGearUp();
            plane->SetEngineState(true);
            CStreaming::SetModelIsDeletable(GenPlane_ModelIndex);
            CStreaming::SetModelTxdIsDeletable(GenPlane_ModelIndex);
        }

        GenPlane_LastTimeGenerated = CTimer::GetTimeInMS();
        GenPlane_Status            = 0;
        return;
    }
    default:
        return;
    }
}

// 0x6C9140
bool CPlane::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C9150
bool CPlane::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C94A0
void CPlane::PreRender() {
    plugin::CallMethod<0x6C94A0, CPlane*>(this);
}

// 0x6CAB70
void CPlane::Render() {
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    CVehicle::Render();
}

// 0x6C9260
void CPlane::ProcessControl() {
    // The crop duster and the stunt plane toggle their smoke with the right stick button
    if (GetStatus() == STATUS_PLAYER && (m_nModelIndex == MODEL_CROPDUST || m_nModelIndex == MODEL_STUNT)) {
        const auto padIndex = m_pDriver && m_pDriver->m_nPedType == PED_TYPE_PLAYER2 ? 1 : 0;
        if (CPad::GetPad(padIndex)->IsRightShockPressed()) {
            m_bSmokeEjectorEnabled = !m_bSmokeEjectorEnabled;
        }
    }

    if (m_bSmokeEjectorEnabled) {
        if (!vehicleFlags.bEngineOn || vehicleFlags.bIsDrowning || !m_pDriver) {
            m_bSmokeEjectorEnabled = false;
        }
    }

    if (m_nModelIndex == MODEL_SKIMMER) {
        m_damageManager.SetAllWheelsState(WHEEL_STATUS_MISSING);
    }

    CAutomobile::ProcessControl();

    m_vehicleAudio.m_DoCountStalls = static_cast<int16>(m_StallCounter);
    if (m_StallCounter) {
        m_StallCounter = 0;
    }

    ProcessWeapons();

    if (m_nModelIndex == MODEL_VORTEX) {
        m_WheelStates.fill(WHEEL_STATE_NORMAL);
    }

    if (m_pSmokeParticle) {
        m_nSmokeTimer += static_cast<uint32>(static_cast<int32>(CTimer::GetTimeStep() * 0.02f * -1000.0f));

        RwMatrix mat;
        m_pSmokeParticle->GetCompositeMatrix(&mat);

        const auto velocity     = -m_vecMoveSpeed * 5.0f;
        const FxPrtMult_c prtMult{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.1f };
        g_fx.m_SmokeHuge->AddParticle(CVector{ mat.pos }, velocity, 0.0f, prtMult);
        g_fx.m_SmokeHuge->AddParticle(CVector{ mat.pos }, velocity, 0.05f, prtMult);

        if (static_cast<int32>(m_nSmokeTimer) <= 0 || vehicleFlags.bIsDrowning) {
            m_pSmokeParticle->Kill();
            m_pSmokeParticle = nullptr;
        }
    }
}

// 0x6CADD0
void CPlane::ProcessControlInputs(uint8 playerNum) {
    plugin::CallMethod<0x6CADD0, CPlane*, uint8>(this, playerNum);
}

// 0x6CB7C0
void CPlane::ProcessFlyingCarStuff() {
    plugin::CallMethod<0x6CB7C0, CPlane*>(this);
}
