#include "StdInc.h"

#include "Plane.h"
#include "Events/EventDanger.h"
#include "CarCtrl.h"
#include "FireManager.h"
#include "Shadows.h"
#include "MotionBlurStreaks.h"

#include <numbers>

auto& HARRIER_NOZZLE_ROTATERATE = StaticRef<float>(0x8D33DC);       // 25.0f
auto& PLANE_DAMAGE_WAVE_COUNTER_VAR = StaticRef<float>(0x8D33E0);   // 0.75f
auto& PLANE_DAMAGE_THRESHHOLD = StaticRef<float>(0x8D33E4);         // 500.0f
auto& PLANE_DAMAGE_SCALE_MASS = StaticRef<float>(0x8D33E8);         // 10000.0f
auto& PLANE_DAMAGE_DESTROY_THRESHHOLD = StaticRef<float>(0x8D33EC); // 5000.0f
auto& vecRCBaronGunPos = StaticRef<CVector>(0x8D33F0);            // <0.0f, 0.45f, 0.0f>
auto& PLANE_HYDRA_HOVER_ACCEL = StaticRef<float>(0x8D341C);         // 0.25f
auto& PLANE_AILERON_PANEL_PULL = StaticRef<float>(0x8D3420);        // 10.0f
auto& PLANE_AILERON_PANEL_SHAKE = StaticRef<float>(0x8D3424);       // 0.002f
auto& PLANE_AILERON_DRIFT = StaticRef<float>(0x8D3428);             // 0.05f
auto& PLANE_ELEVATOR_PANEL_PULL = StaticRef<float>(0x8D342C);       // 10.0f
auto& PLANE_ELEVATOR_PANEL_SHAKE = StaticRef<float>(0x8D3430);      // 0.002f
auto& PLANE_ELEVATOR_DRIFT = StaticRef<float>(0x8D3434);            // 0.05f
auto& PLANE_RUDDER_PANEL_PULL = StaticRef<float>(0x8D3438);         // 10.0f
auto& PLANE_RUDDER_PANEL_SHAKE = StaticRef<float>(0x8D343C);        // 0.002f
auto& PLANE_RUDDER_DRIFT = StaticRef<float>(0x8D3440);              // 0.05f
auto& PLANE_PROP_DAMAGE_SLOWDOWN = StaticRef<float>(0x8D3444);      // 0.2f
auto& PLANE_DAMAGE_WAVE_PERIOD = StaticRef<int32>(0x8D3448);        // 2500
auto& PLANE_PROP_DAMAGE_SPREAD = StaticRef<float>(0x8D344C);        // 0.8f
auto& PLANE_PANEL_SPRING = StaticRef<float>(0x8D33FC);              // 0.03f
auto& PLANE_PANEL_DAMP = StaticRef<float>(0x8D3400);                // 0.98f
auto& PLANE_VORTEX_RISE_LIMIT = StaticRef<float>(0x8D3404);         // 1.3f
auto& PLANE_VORTEX_ROLL_LIMIT = StaticRef<float>(0x8D3408);         // 0.1f
auto& PLANE_VORTEX_PITCH_LIMIT = StaticRef<float>(0x8D340C);        // 0.1f
auto& PLANE_VORTEX_LEAN_KEEP = StaticRef<float>(0x8D3410);          // 0.9f
auto& PLANE_VORTEX_LEAN_LIMIT = StaticRef<float>(0x8D3414);         // 0.3f
auto& PLANE_VORTEX_SPEED_LEAN = StaticRef<float>(0x8D3418);         // 2.0f
auto& PLANE_RUDDER_MAX_ANGLE = StaticRef<float>(0x871914);          // 40.0f, degrees
auto& PLANE_ELEVATOR_MAX_ANGLE = StaticRef<float>(0x871918);        // 25.0f, degrees
auto& PLANE_AILERON_MAX_ANGLE = StaticRef<float>(0x87191C);         // -30.0f, degrees
auto& PLANE_FLAP_EXTEND_SCALE = StaticRef<float>(0x871920);         // 0.5f
auto& PLANE_FLAP_EXTEND_ANGLE = StaticRef<float>(0x871924);         // 0.3f
auto& PLANE_FLAP_SHIFT = StaticRef<float>(0x871928);                // 0.03f
auto& PLANE_GEAR_FOLD_RUSTLER = StaticRef<float>(0x87192C);         // 1.4835299f
auto& PLANE_GEAR_FOLD_HYDRA = StaticRef<float>(0x871930);           // -1.5707964f
auto& PLANE_GEAR_FOLD_HYDRA_FRONT = StaticRef<float>(0x871934);     // -1.3962635f
auto& PLANE_GEAR_FOLD_BIG = StaticRef<float>(0x871938);             // 2.268928f
auto& PLANE_GEAR_FOLD_ANDROM = StaticRef<float>(0x87193C);          // 2.268928f
auto& PLANE_GEAR_FOLD_NEVADA = StaticRef<float>(0x871940);          // 1.3089969f

void CPlane::InjectHooks() {
    RH_ScopedVirtualClass(CPlane, 0x871948, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6C8E20);
    RH_ScopedInstall(InitPlaneGenerationAndRemoval, 0x6CAD90);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C9140);
    RH_ScopedVMTInstall(BurstTyre, 0x6C9150);
    RH_ScopedVMTInstall(PreRender, 0x6C94A0);
    RH_ScopedVMTInstall(Render, 0x6CAB70);
    RH_ScopedInstall(IsAlreadyFlying, 0x6CAB90);
    RH_ScopedVMTInstall(Fix, 0x6CABB0);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6CAC10);
    RH_ScopedInstall(SetGearUp, 0x6CAC20);
    RH_ScopedInstall(SetGearDown, 0x6CAC70);
    RH_ScopedVMTInstall(OpenDoor, 0x6CACB0);
    RH_ScopedVMTInstall(ProcessControl, 0x6C9260);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6CADD0);
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6CB7C0);
    RH_ScopedVMTInstall(VehicleDamage, 0x6CC4B0);
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
    m_nLockOnStartTime = 0;
    m_pLockOnTarget    = nullptr;
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
    if (m_nModelIndex == MODEL_VORTEX) {
        CAutomobile::VehicleDamage(damageIntensity, component, damager, vecCollisionCoors, vecCollisionDirection, weapon);
        return;
    }

    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    // An invulnerable plane only takes damage from the player or the player's vehicle.
    // The original checks the last damaging entity here, not the damager argument.
    if (GetStatus() != STATUS_PLAYER && physicalFlags.bInvulnerable) {
        if (m_pDamageEntity != FindPlayerPed(-1) && m_pDamageEntity != FindPlayerVehicle(-1, false)) {
            return;
        }
    }

    float minDamage    = 0.333f;
    float blowUpDamage = PLANE_DAMAGE_THRESHHOLD;
    float forceMult    = 1.0f; // The original leaves this unset when the damage comes from an explosion, where it is multiplied by zero
    bool  isExplosion  = false;

    if (damageIntensity == 0.0f) {
        // Collision damage, taken from what the physics code left behind
        const auto massRatio = m_fMass / PLANE_DAMAGE_SCALE_MASS;
        minDamage    *= std::max(1.0f, massRatio);
        blowUpDamage *= massRatio;

        damageIntensity   = m_fDamageIntensity;
        vecCollisionCoors = &m_vecLastCollisionPosn;

        if (!(damageIntensity > 0.0f) || physicalFlags.bCollisionProof) {
            return;
        }
        if (m_pDamageEntity && m_pDamageEntity->GetIsTypeBuilding()) {
            return;
        }
    } else {
        bool unused{};
        if (!CanVehicleBeDamaged(damager, weapon, unused)) {
            return;
        }

        // Only explosive weapons hurt a plane through this path, and they do not carry an intensity
        switch (weapon) {
        case WEAPON_ROCKET:
        case WEAPON_ROCKET_HS:
        case WEAPON_FREEFALL_BOMB:
        case WEAPON_EXPLOSION:
            break;
        default:
            return;
        }
        isExplosion     = true;
        damageIntensity = 0.0f;
    }

    const auto prevHealth = m_fHealth;
    float      healthLoss = damageIntensity * m_pHandlingData->m_fCollisionDamageMultiplier;
    auto* const playerVeh = FindPlayerVehicle(-1, false);
    if (this == playerVeh) {
        healthLoss *= vehicleFlags.bTakeLessDamage ? 1.0f / 6.0f : 0.5f;
    } else if (vehicleFlags.bTakeLessDamage) {
        healthLoss *= 1.0f / 12.0f;
    } else if (m_pDamageEntity && m_pDamageEntity == playerVeh) {
        healthLoss *= 2.0f / 3.0f;
    } else {
        healthLoss *= 0.25f;
    }

    m_fHealth -= healthLoss;
    if (m_fHealth <= 0.0f && prevHealth > 0.0f) {
        m_fHealth = 1.0f;
    }

    // The plane is not blown up on the hit that takes its health to the limit, it explodes on the next one.
    // The original passes the plane itself as the damager here.
    if (damageIntensity > blowUpDamage || (m_fHealth <= 1.0f && prevHealth > 0.0f)) {
        BlowUpCar(this, false);
        return;
    }

    if (damageIntensity <= minDamage && !isExplosion) {
        return;
    }
    if (GetStatus() == STATUS_WRECKED) {
        return;
    }
    if (!(m_fMass > 1000.0f)) {
        return;
    }

    // Find the component that is closest to the point of impact
    const auto impactLocal = GetMatrix().InverseTransformPoint(*vecCollisionCoors);
    int32      hitNode     = -1;
    float      hitDist     = 1000.0f;
    for (int32 i = 12; i < 25; i++) {
        auto* const node = m_aCarNodes[i];
        if (!node) {
            continue;
        }
        const auto sqDist = (CVector{ RwFrameGetMatrix(node)->pos } - impactLocal).SquaredMagnitude();
        if (sqDist < hitDist * hitDist) {
            hitDist = std::sqrt(sqDist);
            hitNode = i;
        }
    }
    if (hitNode <= -1) {
        return;
    }

    // Hits on the middle part can end up on one of the parts next to it
    if (hitNode == 0x10 && (CGeneral::GetRandomNumber() & 1)) {
        if (m_aCarNodes[0x11] && (CGeneral::GetRandomNumber() & 1)) {
            hitNode = 0x11;
        } else if (m_aCarNodes[0x12]) {
            hitNode = 0x12;
        }
    }

    if (m_damageManager.ProgressAeroplaneDamage(hitNode)) {
        const auto state = m_damageManager.GetAeroplaneCompStatus(hitNode);
        if (state == DAMSTATE_OPENED && hitNode >= 0x10 && hitNode <= 0x16) {
            if (auto* const panel = CheckIfExistsGetFree(static_cast<eCarNodes>(hitNode))) {
                panel->SetPanel(hitNode, 0, -0.02f);
            }
        } else if (m_damageManager.GetAeroplaneCompStatus(hitNode) == DAMSTATE_DAMAGED) {
            SetComponentVisibility(m_aCarNodes[hitNode], ATOMIC_DAMAGED);
        }
    }

    dmgDrawCarCollidingParticles(*vecCollisionCoors, forceMult * damageIntensity, weapon);

    // Set the part that was hit on fire
    const CVector nodePos = RwFrameGetMatrix(m_aCarNodes[hitNode])->pos;
    if (m_pSmokeParticle) {
        m_pSmokeParticle->Kill();
        m_pSmokeParticle = nullptr;
    }
    auto* const matrix = GetModellingMatrix();
    if (!matrix) {
        return;
    }
    m_pSmokeParticle = g_fxMan.CreateFxSystem("fire", nodePos, matrix, false);
    if (!m_pSmokeParticle) {
        return;
    }
    m_pSmokeParticle->Play();
    m_pSmokeParticle->SetVelAdd(-m_vecMoveSpeed * 5.0f);
    m_pSmokeParticle->SetLocalParticles(true);
    m_nSmokeTimer = CGeneral::GetRandomNumberInRange(2000, 4000);
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
    constexpr float TWO_PI     = 6.2831855f;
    constexpr float DEG_TO_RAD = 0.017453292f;

    constexpr float STREAK_LIFT_SCALE = 6400.0f;
    constexpr float STREAK_LIFT_MIN   = 32.0f;

    auto* const mi       = CModelInfo::GetModelInfo(m_nModelIndex)->AsVehicleModelInfoPtr();
    auto* const colModel = GetColModel();
    const auto  dt       = CTimer::GetTimeStep();

    CVehicle::PreRender();

    if (m_nModelIndex == MODEL_VORTEX) {
        DoHoverSuspensionRatios();
    }

    // Wheels, they follow the suspension
    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        for (int32 i = 0; i < 4; i++) {
            const auto compression = m_fWheelsSuspensionCompression[i];
            const auto restRatio   = 1.0f - m_aSuspensionSpringLength[i] / m_aSuspensionLineLength[i];
            const auto pushedIn    = (compression - restRatio) / (1.0f - restRatio);

            CVector wheelPosn;
            mi->GetWheelPosn(i, wheelPosn, true);

            float height = wheelPosn.z + m_pHandlingData->m_fSuspensionUpperLimit;
            if (pushedIn > 0.0f) {
                height -= pushedIn * m_aSuspensionSpringLength[i];
            }
            if (height <= m_wheelPosition[i]
                && (!physicalFlags.bAddMovingCollisionSpeed || !(m_nHandlingFlagsIntValue & VEHICLE_HANDLING_HYDRAULIC_INST))
            ) {
                height = (height - m_wheelPosition[i]) * 0.75f + m_wheelPosition[i];
            }
            m_wheelPosition[i] = height;

            // The Andromada leans its wheels with the ground
            if (m_nModelIndex == MODEL_ANDROM) {
                if (compression >= 1.0f) {
                    m_wheelRotation[i] *= 0.95f;
                } else {
                    const auto dot = DotProduct(m_wheelColPoint[i].m_vecNormal, GetMatrix().GetForward());
                    m_wheelRotation[i] = -std::asin(std::clamp(dot, -1.0f, 1.0f));
                }
            }
        }
    }

    UpdateWheelMatrix(4, 1);
    UpdateWheelMatrix(7, 1);
    UpdateWheelMatrix(2, 1);
    UpdateWheelMatrix(5, m_nModelIndex == MODEL_HYDRA ? 3 : 1);

    // Propeller angle
    field_9C8 = dt * m_fPropSpeed + field_9C8;
    if (field_9C8 > TWO_PI) {
        do {
            field_9C8 -= TWO_PI;
        } while (field_9C8 > TWO_PI);
    }

    // The axis a control surface turns around comes from a dummy of the model, relative to the surface itself
    const auto SurfaceAxis = [&](int32 dummy, const CMatrix& surface) {
        CVector axis = mi->m_pVehicleStruct->m_avDummyPos[dummy];
        if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f) {
            return CVector{ 1.0f, 0.0f, 0.0f };
        }
        axis -= surface.GetPosition();
        axis.Normalise();
        return axis;
    };
    const auto TurnSurface = [](CMatrix& surface, CVector& axis, float angle) {
        CQuaternion rotation;
        rotation.Set(&axis, angle);
        rotation.Get(surface.m_pAttachMatrix);
    };

    if (GetStatus() <= STATUS_PHYSICS) {
        CMatrix surface;

        if (auto* const rudder = m_aCarNodes[PLANE_RUDDER]) {
            surface.Attach(RwFrameGetMatrix(rudder), false);
            surface.UpdateRW();

            auto axis = SurfaceAxis(11, surface);
            TurnSurface(surface, axis, PLANE_RUDDER_MAX_ANGLE * DEG_TO_RAD * m_fLeftRightSkid);

            if (m_nModelIndex == MODEL_VORTEX && m_aCarNodes[PLANE_MISC_B]) {
                surface.Attach(RwFrameGetMatrix(m_aCarNodes[PLANE_MISC_B]), false);
                surface.UpdateRW();

                CQuaternion rotation;
                rotation.Set(&axis, PLANE_RUDDER_MAX_ANGLE * DEG_TO_RAD * m_fLeftRightSkid);
                rotation.Get(surface.m_pAttachMatrix);
            }
        }

        // With the gear down the ailerons double as flaps
        if (auto* const aileronR = m_aCarNodes[PLANE_AILERON_R]) {
            float flapScale = 1.0f;
            float flapAngle = 0.0f;
            if (m_pFlyingHandlingData->m_fGearDownL > 1.0f
                && std::abs(m_LandingGearAngle) < 1.0f
                && (m_aCarNodes[PLANE_GEAR_L] || m_aCarNodes[PLANE_GEAR_R])
            ) {
                flapScale = (1.0f - std::abs(m_LandingGearAngle)) * PLANE_FLAP_EXTEND_SCALE + 1.0f;
                flapAngle = (1.0f - std::abs(m_LandingGearAngle)) * PLANE_FLAP_EXTEND_ANGLE;
            }

            // Stretches the flap, or moves it back along the plane when its axis is not the one of the model
            const auto ExtendFlap = [&](CMatrix& flap, const CVector& axis, float straightAxisX, int32 component) {
                flap.Update();
                if (axis.x == straightAxisX) {
                    flap.GetForward().y *= flapScale;
                } else {
                    CVector original;
                    if (mi->GetOriginalCompPosition(original, component)) {
                        original.y += (1.0f - flapScale) * colModel->m_boundBox.m_vecMax.y * PLANE_FLAP_SHIFT;
                        flap.SetTranslateOnly(original);
                    }
                }
                flap.UpdateRW();
            };

            surface.Attach(RwFrameGetMatrix(aileronR), false);
            surface.UpdateRW();

            auto axis = SurfaceAxis(9, surface);
            TurnSurface(surface, axis, PLANE_AILERON_MAX_ANGLE * DEG_TO_RAD * m_fSteeringLeftRight + flapAngle);
            if (flapScale > 1.0f) {
                ExtendFlap(surface, axis, 1.0f, 0x14);
            }

            if (auto* const aileronL = m_aCarNodes[PLANE_AILERON_L]) {
                surface.Attach(RwFrameGetMatrix(aileronL), false);
                surface.UpdateRW();

                axis.x *= -1.0f;
                TurnSurface(surface, axis, PLANE_AILERON_MAX_ANGLE * DEG_TO_RAD * m_fSteeringLeftRight - flapAngle);
                if (flapScale > 1.0f) {
                    ExtendFlap(surface, axis, -1.0f, 0x13);
                }
            }
        }

        if (auto* const elevatorR = m_aCarNodes[PLANE_ELEVATOR_R]) {
            surface.Attach(RwFrameGetMatrix(elevatorR), false);
            surface.UpdateRW();

            auto axis = SurfaceAxis(10, surface);
            TurnSurface(surface, axis, -(PLANE_ELEVATOR_MAX_ANGLE * DEG_TO_RAD * m_fSteeringUpDown));

            if (auto* const elevatorL = m_aCarNodes[PLANE_ELEVATOR_L]) {
                surface.Attach(RwFrameGetMatrix(elevatorL), false);
                surface.UpdateRW();

                axis.x *= -1.0f;
                TurnSurface(surface, axis, PLANE_ELEVATOR_MAX_ANGLE * DEG_TO_RAD * m_fSteeringUpDown);
            }
        }
    }

    // Propellers, the blurred disc replaces the blades as they speed up
    const auto SetFrameAlpha = [](RwFrame* frame, int32 alpha) {
        RpAtomic* atomic = nullptr;
        RwFrameForAllObjects(frame, GetCurrentAtomicObjectCB, &atomic);
        if (atomic) {
            SetComponentAtomicAlpha(atomic, alpha);
        }
    };
    const auto SpinPropeller = [&](ePlaneNodes blades, ePlaneNodes disc, float direction) {
        if (auto* const frame = m_aCarNodes[blades]) {
            SetComponentRotation(frame, AXIS_Y, direction * field_9C8 + direction * field_9C8, true);
            SetFrameAlpha(frame, 0xFF);
        }
        if (auto* const frame = m_aCarNodes[disc]) {
            SetComponentRotation(frame, AXIS_Y, -(direction * field_9C8), true);
            SetFrameAlpha(frame, 0);
        }
    };
    SpinPropeller(PLANE_STATIC_PROP, PLANE_MOVING_PROP, 1.0f);
    SpinPropeller(PLANE_STATIC_PROP2, PLANE_MOVING_PROP2, -1.0f);

    // Landing gear folding
    {
        bool          foldsGear   = true;
        eRotationAxis gearAxis    = AXIS_X;
        float         leftAngle   = 0.0f;
        float         rightAngle  = 0.0f;
        int32         extraAxis   = -1;
        float         extraAngle  = 0.0f;
        float         lastAngle   = 0.0f;
        switch (m_nModelIndex) {
        case MODEL_RUSTLER:
            gearAxis   = AXIS_Y;
            leftAngle  = -PLANE_GEAR_FOLD_RUSTLER;
            rightAngle = PLANE_GEAR_FOLD_RUSTLER;
            break;
        case MODEL_SHAMAL:
        case MODEL_AT400:
            gearAxis   = AXIS_Y;
            leftAngle  = -PLANE_GEAR_FOLD_RUSTLER;
            rightAngle = PLANE_GEAR_FOLD_RUSTLER;
            extraAxis  = AXIS_X;
            extraAngle = PLANE_GEAR_FOLD_BIG;
            break;
        case MODEL_HYDRA:
            gearAxis   = AXIS_X;
            leftAngle  = PLANE_GEAR_FOLD_HYDRA;
            rightAngle = PLANE_GEAR_FOLD_HYDRA;
            extraAxis  = AXIS_X;
            extraAngle = PLANE_GEAR_FOLD_HYDRA_FRONT;
            lastAngle  = PLANE_GEAR_FOLD_BIG;
            break;
        case MODEL_NEVADA:
            gearAxis   = AXIS_X;
            leftAngle  = PLANE_GEAR_FOLD_NEVADA;
            rightAngle = PLANE_GEAR_FOLD_NEVADA;
            break;
        case MODEL_ANDROM:
            gearAxis   = AXIS_X;
            leftAngle  = PLANE_GEAR_FOLD_ANDROM;
            rightAngle = PLANE_GEAR_FOLD_ANDROM;
            extraAxis  = AXIS_X;
            extraAngle = -PLANE_GEAR_FOLD_ANDROM;
            break;
        default:
            foldsGear = false;
            break;
        }

        if (foldsGear) {
            const auto gear = std::abs(m_LandingGearAngle);
            SetComponentRotation(m_aCarNodes[PLANE_GEAR_L], gearAxis, gear * leftAngle, true);
            SetComponentRotation(m_aCarNodes[PLANE_GEAR_R], gearAxis, gear * rightAngle, true);
            if (extraAxis > -1) {
                SetComponentRotation(m_aCarNodes[PLANE_MISC_A], static_cast<eRotationAxis>(extraAxis), gear * extraAngle, true);
                if (lastAngle > 0.0f) {
                    SetComponentRotation(m_aCarNodes[PLANE_MISC_B], static_cast<eRotationAxis>(extraAxis), gear * lastAngle, true);
                }
            }
        }
    }

    if (m_nModelIndex == MODEL_ANDROM) {
        SetComponentRotation(m_aCarNodes[PLANE_MISC_B], AXIS_X, static_cast<float>(m_wMiscComponentAngle) * ANDROM_COL_ANGLE_MULT, true);
    } else if (m_nModelIndex == MODEL_HYDRA) {
        // Nozzles
        const auto nozzleAngle = static_cast<float>(m_wMiscComponentAngle) * (std::numbers::pi_v<float> / 2.0f) / static_cast<float>(static_cast<int16>(HARRIER_NOZZLE_ROTATE_LIMIT));
        SetComponentRotation(m_aCarNodes[PLANE_WHEEL_LM], AXIS_X, nozzleAngle, true);
        SetComponentRotation(m_aCarNodes[PLANE_WHEEL_RM], AXIS_X, nozzleAngle, true);

        if (vehicleFlags.bEngineOn) {
            const auto thrust = (m_fAccelerationBreakStatus + 1.0f) * 0.5f;

            if (static_cast<int32>(m_wMiscComponentAngle) >= static_cast<int32>(static_cast<int16>(HARRIER_NOZZLE_SWITCH_LIMIT))) {
                DoHeliDustEffect(thrust, 2.0f);
            } else if (m_pDustParticle) {
                m_pDustParticle->Kill();
                m_pDustParticle       = nullptr;
                m_heliDustFxTimeConst = 0.0f;
            }

            auto* const nozzleLeft  = RwFrameGetMatrix(m_aCarNodes[PLANE_WHEEL_LM]);
            auto* const nozzleRight = RwFrameGetMatrix(m_aCarNodes[PLANE_WHEEL_RM]);

            for (auto& jet : m_apJettrusParticles) {
                if (auto* const parent = GetModellingMatrix(); parent && !jet) {
                    jet = g_fxMan.CreateFxSystem("jetthrust", CVector{}, parent, false);
                    if (jet) {
                        jet->Play();
                        jet->SetLocalParticles(true);
                        jet->CopyParentMatrix();
                    }
                }
            }

            const struct {
                FxSystem_c* fx;
                RwMatrix*   nozzle;
                CVector     offset;
            } jets[] = {
                { m_apJettrusParticles[0], nozzleLeft,  {  0.7f,  -0.45f, 0.05f } },
                { m_apJettrusParticles[1], nozzleLeft,  { -0.82f, -0.45f, 0.05f } },
                { m_apJettrusParticles[2], nozzleRight, {  0.63f, -0.45f, 0.07f } },
                { m_apJettrusParticles[3], nozzleRight, { -0.75f, -0.45f, 0.07f } },
            };
            for (const auto& [fx, nozzle, offset] : jets) {
                if (!fx) {
                    continue;
                }
                const CMatrix nozzleMatrix{ nozzle, false };
                const auto    world = GetMatrix() * nozzleMatrix;
                RwMatrix      worldRw{};
                world.CopyToRwMatrix(&worldRw);

                fx->SetMatrix(&worldRw);
                fx->SetOffsetPos(offset);
                fx->SetConstTime(true, thrust);
            }
        } else {
            for (auto& jet : m_apJettrusParticles) {
                if (jet) {
                    jet->Kill();
                    jet = nullptr;
                }
            }
            if (m_pDustParticle) {
                m_pDustParticle->Kill();
                m_pDustParticle       = nullptr;
                m_heliDustFxTimeConst = 0.0f;
            }
        }
    } else if (m_nModelIndex == MODEL_VORTEX && m_aCarNodes[PLANE_MISC_A]) {
        // The hull of the Vortex leans with the speed and the suspension
        auto* const colData = GetColModel()->m_pColData;

        CVector frontWheel;
        mi->GetWheelPosn(0, frontWheel, false);

        CMatrix hull;
        hull.Attach(RwFrameGetMatrix(m_aCarNodes[PLANE_MISC_A]), false);

        const auto leanLimit = PLANE_VORTEX_LEAN_LIMIT;
        const auto& mat      = GetMatrix();
        hull.GetUp().y = std::clamp(DotProduct(m_vecMoveSpeed, mat.GetForward()) * PLANE_VORTEX_SPEED_LEAN, -leanLimit, leanLimit);
        hull.GetUp().x = std::clamp(DotProduct(m_vecMoveSpeed, mat.GetRight())   * PLANE_VORTEX_SPEED_LEAN, -leanLimit, leanLimit);

        const auto keep = std::pow(PLANE_VORTEX_LEAN_KEEP, dt);
        const auto take = 1.0f - keep;
        const auto lines = colData->m_pLines;

        const auto pitch = std::clamp(
            ((m_wheelPosition[0] - m_wheelPosition[1]) + (m_wheelPosition[2] - m_wheelPosition[3])) * 0.5f / (lines[0].m_vecStart.y - lines[1].m_vecStart.y),
            -PLANE_VORTEX_PITCH_LIMIT, PLANE_VORTEX_PITCH_LIMIT
        );
        hull.GetForward().z = hull.GetForward().z * keep + take * pitch;

        const auto roll = std::clamp(
            ((m_wheelPosition[3] - m_wheelPosition[1]) + (m_wheelPosition[2] - m_wheelPosition[0])) * 0.5f / (lines[3].m_vecStart.x - lines[1].m_vecStart.x),
            -PLANE_VORTEX_ROLL_LIMIT, PLANE_VORTEX_ROLL_LIMIT
        );
        hull.GetRight().z = hull.GetRight().z * keep + take * roll;

        const auto rise = std::min(
            1.0f - ((m_wheelPosition[3] + m_wheelPosition[1] + m_wheelPosition[2] + m_wheelPosition[0]) * 0.25f - frontWheel.z) / (mi->m_fWheelSizeFront * 0.5f),
            PLANE_VORTEX_RISE_LIMIT
        );
        hull.GetUp().z = hull.GetUp().z * keep + take * rise;

        hull.UpdateRW();
    }

    for (auto& panel : m_panels) {
        if (static_cast<int16>(panel.m_nFrameId) > -1) {
            panel.ProcessPanel(this, m_aCarNodes[static_cast<int16>(panel.m_nFrameId)], m_moveForce, m_turnForce, PLANE_PANEL_SPRING, PLANE_PANEL_DAMP);
        }
    }
    m_moveForce = m_vecMoveSpeed + m_vecFrictionMoveSpeed;
    m_turnForce = m_vecTurnSpeed + m_vecFrictionTurnSpeed;

    CShadows::StoreShadowForVehicle(
        this,
        m_nModelIndex == MODEL_VORTEX
            ? VEH_SHD_CAR
            : (m_nModelIndex == MODEL_AT400 || m_nModelIndex == MODEL_ANDROM ? VEH_SHD_BIG_PLANE : VEH_SHD_PLANE)
    );

    // Wing tip trails, they show up with the lift
    if (!vehicleFlags.bIsDrowning && m_nModelIndex != MODEL_VORTEX && m_nModelIndex != MODEL_RCBARON) {
        const auto& mat  = GetMatrix();
        const auto  lift = DotProduct(m_vecMoveSpeed, mat.GetUp()) * m_pFlyingHandlingData->m_fAttackLift * STREAK_LIFT_SCALE;
        const auto  alpha = static_cast<uint8>(static_cast<int32>(std::max(0.0f, std::abs(lift) - STREAK_LIFT_MIN)));

        CVector tipRight = mi->m_pVehicleStruct->m_avDummyPos[12];
        CVector tipLeft  = tipRight;
        tipLeft.x -= 0.1f;

        CMotionBlurStreaks::RegisterStreak(reinterpret_cast<uint32>(this), 0xFF, 0xFF, 0xFF, alpha, mat.TransformPoint(tipRight), mat.TransformPoint(tipLeft));

        tipRight.x = -tipRight.x;
        tipLeft.x  = -tipLeft.x;
        CMotionBlurStreaks::RegisterStreak(reinterpret_cast<uint32>(this) + 1, 0xFF, 0xFF, 0xFF, alpha, mat.TransformPoint(tipRight), mat.TransformPoint(tipLeft));
    }

    // Crop duster and Stuntplane smoke
    if (m_bSmokeEjectorEnabled && (m_nModelIndex == MODEL_CROPDUST || m_nModelIndex == MODEL_STUNT)) {
        const auto& mat = GetMatrix();

        CVector pos;
        CVector vel = m_vecMoveSpeed * 10.0f;
        FxPrtMult_c prtMult;
        if (m_nModelIndex == MODEL_CROPDUST) {
            prtMult = FxPrtMult_c{ 1.0f, 1.0f, 1.0f, 0.4f, 1.0f, 1.0f, 0.2f };
            pos     = mat.TransformPoint({ 0.0f, -0.5f, -0.5f });
            vel.z  -= 3.0f;
        } else {
            prtMult = FxPrtMult_c{ 1.0f, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.3f };
            pos     = mat.TransformPoint({ 0.0f, -5.0f, 0.0f });
        }
        g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, prtMult);
    }

    if (m_nModelIndex == MODEL_SKIMMER && physicalFlags.bSubmergedInWater) {
        DoBoatSplashes(m_fDoomHorizontalRotation);
    }
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
    const auto  dt         = CTimer::GetTimeStep();
    auto* const pad        = CPad::GetPad(playerNum);
    const auto  forwardSpd = DotProduct(m_vecMoveSpeed, GetMatrix().GetForward());

    // Eases the steering axes towards what the pad is asking for
    const auto FollowPadSteering = [&] {
        m_nLastControlInput  = eControllerType::KEYBOARD;
        m_fSteeringLeftRight = (static_cast<float>(pad->GetSteeringLeftRight()) / 128.0f - m_fSteeringLeftRight) * dt * 0.2f + m_fSteeringLeftRight;
        m_fSteeringUpDown    = (static_cast<float>(pad->GetSteeringUpDown())    / 128.0f - m_fSteeringUpDown)    * dt * 0.2f + m_fSteeringUpDown;
    };

    if (CCamera::m_bUseMouse3rdPerson && m_bEnableMouseFlying) {
        const auto& mouse      = CPad::NewMouseControllerState.m_AmountMoved;
        const auto  padSteered = pad->GetSteeringLeftRight() != 0 || pad->GetSteeringUpDown() != 0;

        // The mouse keeps the control while the axes are still moving after it stopped
        auto mouseControls = mouse.x != 0.0f || mouse.y != 0.0f;
        if (!mouseControls
            && (m_fSteeringLeftRight != 0.0f || m_fSteeringUpDown != 0.0f)
            && m_nLastControlInput == eControllerType::MOUSE
            && !padSteered
        ) {
            mouseControls = true;
        }

        if (mouseControls) {
            m_nLastControlInput = eControllerType::MOUSE;
            if (pad->NewState.m_bVehicleMouseLook == 0) {
                m_fSteeringLeftRight += mouse.x * 0.0015f;
                m_fSteeringUpDown    += mouse.y * 0.0015f;
            }
            if (std::abs(m_fSteeringLeftRight) < 0.15f) {
                m_fSteeringLeftRight *= std::pow(0.99f, dt);
            }
            if (std::abs(m_fSteeringUpDown) < 0.15f) {
                m_fSteeringUpDown *= std::pow(0.99f, dt);
            }
        } else if (padSteered || m_nLastControlInput != eControllerType::MOUSE) {
            FollowPadSteering();
        }
    } else {
        FollowPadSteering();
    }

    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);
    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown,    -1.0f, 1.0f);

    // The skid axis steers the Vortex with the stick, other aircraft with the look buttons
    float skidTarget;
    if (m_nModelIndex == MODEL_VORTEX) {
        skidTarget = static_cast<float>(pad->GetSteeringLeftRight()) / 128.0f;
    } else if (pad->GetLookLeft()) {
        skidTarget = -1.0f;
    } else {
        skidTarget = pad->GetLookRight() ? 1.0f : 0.0f;
    }
    m_fLeftRightSkid = std::clamp((skidTarget - m_fLeftRightSkid) * dt * 0.2f + m_fLeftRightSkid, -1.0f, 1.0f);

    m_fSteerAngle = -(m_pHandlingData->m_fSteeringLock * 0.017453292f * m_fLeftRightSkid * std::max(0.1f, 1.0f - forwardSpd));
    if (m_nModelIndex == MODEL_VORTEX && pad->GetHandBrake()) {
        m_fSteerAngle    *= 1.5f;
        m_fLeftRightSkid *= 1.5f;
    }

    m_fAccelerationBreakStatus = static_cast<float>(pad->GetAccelerate() - pad->GetBrake()) / 255.0f;

    // Landing gear
    if (pad->IsRightShockPressed()
        && std::ranges::all_of(m_fWheelsSuspensionCompression, [](float v) { return v == 1.0f; })
        && m_pFlyingHandlingData->m_fGearUpR < 1.0f
        && m_nModelIndex != MODEL_RCBARON
    ) {
        if (m_LandingGearAngle == 0.0f) {
            // Raising the gear
            m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_MISSING);
            m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_MISSING);
            m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_MISSING);
            m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_MISSING);
            m_LandingGearAngle += dt * 0.02f;
        } else if (m_LandingGearAngle == 1.0f) {
            // Lowering the gear, a negative angle means it is on its way down
            m_LandingGearAngle = dt * 0.02f - 1.0f;
        } else {
            m_LandingGearAngle = -m_LandingGearAngle;
        }
    } else if (m_LandingGearAngle < 0.0f) {
        m_LandingGearAngle += dt * 0.02f;
        if (!(m_LandingGearAngle < 0.0f)) {
            SetGearDown();
        }
    } else if (m_LandingGearAngle > 0.0f && m_LandingGearAngle < 1.0f) {
        m_LandingGearAngle += dt * 0.02f;
        if (!(m_LandingGearAngle < 1.0f)) {
            SetGearUp();
        }
    }

    // The Hydra tilts its nozzles
    if (m_nModelIndex == MODEL_HYDRA && std::abs(static_cast<float>(pad->GetCarGunUpDown())) > 10.0f) {
        m_wMiscComponentAnglePrev = m_wMiscComponentAngle;

        const auto delta = static_cast<int16>(static_cast<int32>(static_cast<float>(pad->GetCarGunUpDown()) * dt * HARRIER_NOZZLE_ROTATERATE / 128.0f));
        const auto sum   = static_cast<int32>(m_wMiscComponentAngle) + delta;
        m_wMiscComponentAngle = sum < 0 ? 0 : static_cast<uint16>(sum);
        if (m_wMiscComponentAngle > static_cast<int16>(HARRIER_NOZZLE_ROTATE_LIMIT)) {
            m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;
        }
    }

    vehicleFlags.bIsHandbrakeOn = false;
    m_GasPedal                  = 0.0f;

    if (forwardSpd > 0.0f && pad->GetBrake() != 0) {
        m_BrakePedal = static_cast<float>(pad->GetBrake()) * (forwardSpd > 0.35f ? 1.0f / 510.0f : 1.0f / 255.0f);
    } else if (m_vecMoveSpeed.Magnitude() < 0.1f && pad->GetBrake() < 10 && pad->GetAccelerate() < 10) {
        m_BrakePedal = 0.5f;
    } else {
        m_BrakePedal = 0.0f;
    }

    // With the controls disabled the plane is held with the brakes
    if (pad->DisablePlayerControls) {
        vehicleFlags.bIsHandbrakeOn = true;
        m_BrakePedal                = 1.0f;
        m_GasPedal                  = 0.0f;
        FindPlayerPed(-1)->KeepAreaAroundPlayerClear();

        const auto speed = m_vecMoveSpeed.Magnitude();
        if (speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }
}

// 0x6CB7C0
void CPlane::ProcessFlyingCarStuff() {
    const auto dt = CTimer::GetTimeStep();
    if (dt <= 0.0f) {
        return;
    }

    // Phase of the shaking that the damaged parts apply, it moves by a random amount each frame
    m_planeDamageWave += static_cast<int32>(
        CGeneral::GetRandomNumberInRange(1.0f - PLANE_DAMAGE_WAVE_COUNTER_VAR, PLANE_DAMAGE_WAVE_COUNTER_VAR + 1.0f)
        * static_cast<float>(static_cast<uint32>(static_cast<int32>(dt * 0.02f * 1000.0f)))
    );

    const auto speedFactor = std::min(m_vecMoveSpeed.Magnitude() * 3.0f, 1.0f);

    // The inputs that end up in the flight model, they can be pushed by the damage
    float skid      = m_fLeftRightSkid;
    float upDown    = m_fSteeringUpDown;
    float leftRight = m_fSteeringLeftRight;

    if (m_fHealth < 250.0f && GetStatus() == STATUS_PLAYER) {
        m_damageManager.SetAeroplaneCompStatus(PLANE_STATIC_PROP, DAMSTATE_DAMAGED);
        m_damageManager.SetAeroplaneCompStatus(PLANE_MOVING_PROP, DAMSTATE_DAMAGED);

        skid += 0.5f;
        if (std::abs(GetRoll()) < 3.0f * std::numbers::pi_v<float> / 4.0f) {
            leftRight += 0.75f;
        }
        if (std::abs(GetRoll()) > std::numbers::pi_v<float> / 2.0f) {
            upDown += 0.5f;
        }
    }

    if (m_nModelIndex != MODEL_RCBARON) {
        for (int32 node = PLANE_STATIC_PROP; node < PLANE_NUM_NODES; node++) {
            const auto state = static_cast<int32>(m_damageManager.GetAeroplaneCompStatus(node));
            if (!m_aCarNodes[node] || state <= 0) {
                continue;
            }

            // The bouncing panel of this part, if it has one. The original also looks one entry past the panels.
            int32 panelIdx = -1;
            for (int32 i = 0; i < static_cast<int32>(m_panels.size()); i++) {
                if (static_cast<int16>(m_panels[i].m_nFrameId) == node) {
                    panelIdx = i;
                    break;
                }
            }

            const auto  stateF  = static_cast<float>(state);
            const auto  stateSq = static_cast<float>(state * state);
            float       amount;

            // The control surfaces drift on their own and pull their panel along
            const auto DriftSurface = [&](float& axis, float& axisCopy, float drift, float panelShake, float panelPull) {
                axis += CGeneral::GetRandomNumberInRange(-drift, drift) * stateSq * dt * speedFactor;
                axisCopy = axis;
                if (panelIdx >= 0) {
                    auto& panel = m_panels[panelIdx];
                    panel.m_vecPos.y += CGeneral::GetRandomNumberInRange(-panelShake, panelShake) * stateSq * dt * speedFactor;

                    const auto pull = panelPull * panel.m_vecRotation.y;
                    if (node == PLANE_AILERON_L) {
                        axisCopy += pull;
                    } else {
                        axisCopy -= pull;
                    }
                }
            };

            switch (node) {
            case PLANE_STATIC_PROP:
            case PLANE_MOVING_PROP:
            case PLANE_STATIC_PROP2:
            case PLANE_MOVING_PROP2: {
                if (m_fAccelerationBreakStatus > 0.0f) {
                    const auto spread = CGeneral::GetRandomNumberInRange(1.0f - PLANE_PROP_DAMAGE_SPREAD, PLANE_PROP_DAMAGE_SPREAD + 1.0f);
                    const auto wave   = std::sin(static_cast<float>(static_cast<uint32>(m_planeDamageWave)) * (2.0f * std::numbers::pi_v<float>) / static_cast<float>(static_cast<uint32>(PLANE_DAMAGE_WAVE_PERIOD)));
                    m_fAccelerationBreakStatus += (wave - 1.0f) * spread * stateF * stateF * dt * PLANE_PROP_DAMAGE_SLOWDOWN;
                }
                amount = stateF * 0.5f;
                break;
            }
            case PLANE_RUDDER: {
                const auto s = stateF * 0.2f;
                amount = s;
                m_fLeftRightSkid *= 1.0f - s;
                DriftSurface(m_fLeftRightSkid, skid, PLANE_RUDDER_DRIFT, PLANE_RUDDER_PANEL_SHAKE, PLANE_RUDDER_PANEL_PULL);
                break;
            }
            case PLANE_ELEVATOR_L:
            case PLANE_ELEVATOR_R: {
                m_fSteeringUpDown *= 1.0f - stateF * 0.2f;
                DriftSurface(m_fSteeringUpDown, upDown, PLANE_ELEVATOR_DRIFT, PLANE_ELEVATOR_PANEL_SHAKE, PLANE_ELEVATOR_PANEL_PULL);
                amount = stateF * 0.15f;
                break;
            }
            case PLANE_AILERON_L:
            case PLANE_AILERON_R: {
                m_fSteeringLeftRight *= 1.0f - stateF * 0.2f;
                DriftSurface(m_fSteeringLeftRight, leftRight, PLANE_AILERON_DRIFT, PLANE_AILERON_PANEL_SHAKE, PLANE_AILERON_PANEL_PULL);
                amount = stateF * 0.25f;
                break;
            }
            default:
                continue;
            }

            if (amount <= 0.0f || vehicleFlags.bIsDrowning) {
                continue;
            }

            // Smoke is not made every frame at low speed, nor for planes far from the camera unless it is the player's
            if (!(speedFactor > 0.3f) && (CGeneral::GetRandomNumber() & 7)) {
                continue;
            }
            if (!((GetPosition() - TheCamera.GetPosition()).SquaredMagnitude() < sq(80.0f)) && GetStatus() != STATUS_PLAYER) {
                continue;
            }

            const auto pos = GetMatrix().TransformPoint(CVector{ RwFrameGetMatrix(m_aCarNodes[node])->pos });

            FxPrtMult_c prtMult{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, amount };

            CVector vel = m_vecMoveSpeed * 0.25f * 50.0f;
            vel.x *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
            vel.y *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
            vel.z *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);

            // Replaces the life that was given to the constructor
            prtMult.m_fLife = CGeneral::GetRandomNumberInRange(0.0f, 1.0f);

            const CVector spawnPos{
                pos.x + CGeneral::GetRandomNumberInRange(-amount, amount),
                pos.y + CGeneral::GetRandomNumberInRange(-amount, amount),
                pos.z + CGeneral::GetRandomNumberInRange(-amount, amount),
            };
            g_fx.m_SmokeHuge->AddParticle(spawnPos, vel, 0.0f, prtMult);
        }
    }

    const auto status = GetStatus();
    if (status != STATUS_PLAYER && status != STATUS_PHYSICS && status != STATUS_REMOTE_CONTROLLED) {
        // Nobody flies it, the propeller winds down
        const auto windDown = dt * 0.001f;
        if (m_fPropSpeed <= windDown) {
            vehicleFlags.bEngineOn = false;
            m_fPropSpeed           = 0.0f;
            return;
        }
        m_nFakePhysics = 0;
        m_fPropSpeed -= m_fPropSpeed > PLANE_STD_PROP_SPEED ? dt * 0.003f : windDown;
        return;
    }

    float targetPropSpeed = PLANE_STD_PROP_SPEED;
    if (m_fAccelerationBreakStatus > 0.0f) {
        targetPropSpeed = (PLANE_MAX_PROP_SPEED - PLANE_STD_PROP_SPEED) * m_fAccelerationBreakStatus + PLANE_STD_PROP_SPEED;
    } else if (m_fAccelerationBreakStatus < 0.0f) {
        targetPropSpeed = (PLANE_STD_PROP_SPEED - PLANE_MIN_PROP_SPEED) * m_fAccelerationBreakStatus + PLANE_STD_PROP_SPEED;
    }

    // Above the ceiling the engine loses power
    if (status == STATUS_PLAYER || status == STATUS_REMOTE_CONTROLLED) {
        const auto flightModel = m_nModelIndex != MODEL_RCBARON ? FLIGHTMODEL_PLANE : FLIGHTMODEL_RCPLANE;
        const auto aboveCeiling = HeightAboveCeiling(GetPosition().z, flightModel);
        if (aboveCeiling > 0.0f) {
            targetPropSpeed *= std::max(0.0f, aboveCeiling * 0.02f);
            m_fAccelerationBreakStatus = std::max(-1.0f, m_fAccelerationBreakStatus - aboveCeiling * 0.04f);
        }
    }

    m_fPropSpeed += (targetPropSpeed - m_fPropSpeed) * dt * PLANE_ROC_PROP_SPEED;

    if (vehicleFlags.bIsDrowning) {
        vehicleFlags.bEngineOn     = false;
        m_fPropSpeed               = 0.0f;
        m_fAccelerationBreakStatus = 0.0f;
    }

    if (m_nModelIndex == MODEL_RCBARON) {
        if (!vehicleFlags.bIsDrowning && vehicleFlags.bEngineOn) {
            FlyingControl(FLIGHTMODEL_RCPLANE, skid, upDown, leftRight, m_fAccelerationBreakStatus);
        }
        return;
    }

    if (!vehicleFlags.bEngineOn) {
        return;
    }
    if (m_fPropSpeed <= PLANE_MIN_PROP_SPEED && m_vecMoveSpeed.SquaredMagnitude() <= 0.05f) {
        return;
    }

    // The Hydra flies like the Hunter while its nozzles point down
    if (m_nModelIndex == MODEL_HYDRA && GetStatus() == STATUS_PLAYER
        && static_cast<int32>(m_wMiscComponentAngle) >= static_cast<int16>(HARRIER_NOZZLE_SWITCH_LIMIT)
    ) {
        auto* const planeHandling = m_pFlyingHandlingData;
        m_pFlyingHandlingData = gHandlingDataMgr.GetFlyingPointer(static_cast<uint8>(CModelInfo::GetVehicleModelInfo(MODEL_HUNTER)->m_nHandlingId));

        if (m_fAccelerationBreakStatus > 0.0f || (m_nNumContactWheels < 4 && !physicalFlags.bTouchingWater)) {
            FlyingControl(FLIGHTMODEL_HELI, skid, upDown, -leftRight, PLANE_HYDRA_HOVER_ACCEL * m_fAccelerationBreakStatus);
        }

        m_pFlyingHandlingData = planeHandling;
        return;
    }

    FlyingControl(FLIGHTMODEL_PLANE, skid, upDown, leftRight, m_fAccelerationBreakStatus);
}
