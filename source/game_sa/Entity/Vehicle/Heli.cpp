/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Shadows.h"
#include "Ropes.h"
#include "WindModifiers.h"
#include "Tasks/TaskTypes/TaskComplexUseSwatRope.h"
#include "Tasks/TaskTypes/TaskComplexWanderCop.h"

void CHeli::InjectHooks() {
    RH_ScopedVirtualClass(CHeli, 0x871680, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(InitHelis, 0x6C4560);
    RH_ScopedInstall(AddHeliSearchLight, 0x6C45B0);
    RH_ScopedInstall(Pre_SearchLightCone, 0x6C4650);
    RH_ScopedInstall(Post_SearchLightCone, 0x6C46E0);
    RH_ScopedInstall(FindSwatPositionRelativeToHeli, 0x6C4760);
    RH_ScopedInstall(SwitchPoliceHelis, 0x6C4800);
    RH_ScopedInstall(RenderAllHeliSearchLights, 0x6C7C50);
    RH_ScopedInstall(TestSniperCollision, 0x6C6890);
    RH_ScopedInstall(UpdateHelis, 0x6C79A0);
    RH_ScopedInstall(GenerateHeli, 0x6C6520);
    RH_ScopedInstall(SendDownSwat, 0x6C69C0);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6C4830);
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6C4E60);
    RH_ScopedVMTInstall(PreRender, 0x6C5420);
    RH_ScopedVMTInstall(BlowUpCar, 0x6C6D30);
    RH_ScopedVMTInstall(Render, 0x6C4400);
    RH_ScopedVMTInstall(Fix, 0x6C4530);
    RH_ScopedVMTInstall(BurstTyre, 0x6C4330);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C4320);
}

// 0x6C4190
CHeli::CHeli(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_HELI;

    m_fLeftRightSkid           = 0.0f;
    m_fSteeringUpDown          = 0.0f;
    m_fSteeringLeftRight       = 0.0f;
    m_fAccelerationBreakStatus = 0.0f;

    field_99C = 0;
    m_fRotorZ = 0;
    m_fSecondRotorZ = 0;

    m_fMinAltitude = 10.0f;
    m_fMaxAltitude = 10.0f;

    field_9AC = 10.0f;
    field_9B4 = 0;

    m_nHeliFlags = m_nHeliFlags & 0xFC;
    m_fSearchLightIntensity = 0.0f;
    physicalFlags.bDontCollideWithFlyers = true;

    if (modelIndex == MODEL_HUNTER) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        m_doors[DOOR_LEFT_FRONT].Init((3.0f * PI) / 10.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
    }

    m_nNumSwatOccupants = 4;
    m_aSwatState.fill(0);

    m_nSearchLightTimer = CTimer::GetTimeInMS();

    m_aSearchLightHistoryX.fill(0.0f);
    m_aSearchLightHistoryY.fill(0.0f);

    m_nShootTimer = 0;
    m_nPoliceShoutTimer = CTimer::GetTimeInMS();

    vehicleFlags.bNeverUseSmallerRemovalRange = true; // 0x6C42BD
    m_autoPilot.m_ucHeliTargetDist2 = 10;

    m_ppGunflashFx = nullptr;
    m_nFiringMultiplier = 16;

    field_9B8 = 0;
    m_bSearchLightEnabled = false;
    field_A14 = CGeneral::GetRandomNumberInRange(2.f, 8.f);
}

// 0x6C4340
CHeli::~CHeli() {
    if (m_ppGunflashFx) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& fx = m_ppGunflashFx[i]) {
                fx->Kill();
                g_fxMan.DestroyFxSystem(fx);
            }
        }
        delete[] m_ppGunflashFx;
        m_ppGunflashFx = nullptr;
    }

    m_vehicleAudio.Terminate();
}

// 0x6C4560
void CHeli::InitHelis() {
    std::ranges::fill(pHelis, nullptr);
    for (auto& light : HeliSearchLights) {
        light.Init();
    }
    NumberOfSearchLights = 0;
    bPoliceHelisAllowed = true;
}

// 0x6C45B0
void CHeli::AddHeliSearchLight(const CVector& origin, const CVector& target, float targetRadius, float power, uint32 coronaIndex, uint8 unknownFlag, uint8 drawShadow) {
    auto& light = HeliSearchLights[NumberOfSearchLights];

    light.m_vecOrigin     = origin;
    light.m_vecTarget     = target;
    light.m_fTargetRadius = targetRadius;
    light.m_fPower        = power;
    light.m_nCoronaIndex  = coronaIndex;
    light.field_24        = unknownFlag;
    light.m_bDrawShadow   = drawShadow;

    NumberOfSearchLights += 1;
}

// 0x6C4640
void CHeli::PreRenderAlways() {
    // NOP
}

// 0x6C4650
void CHeli::Pre_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,        RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,            RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATEREQUAL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));
}

// 0x6C46E0
void CHeli::Post_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,             RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATER));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(2u));
}

// 0x6C4750
void CHeli::SpecialHeliPreRender() {
    // NOP
}

// 0x6C4760
CVector CHeli::FindSwatPositionRelativeToHeli(int32 swatNumber) {
    switch ( swatNumber ) {
    case 0:
        return { -1.2f, -1.0f, -0.5f };
    case 1:
        return { 1.2f,  -1.0f, -0.5f };
    case 2:
        return { -1.2f, 1.0f,  -0.5f };
    case 3:
        return { 1.2f,  1.0f,  -0.5f };
    default:
        return { 0.0f,  0.0f,  0.0f  };
    }
}

// 0x6C4800
void CHeli::SwitchPoliceHelis(bool enable) {
    bPoliceHelisAllowed = enable;
}

// 0x6C58E0
void CHeli::SearchLightCone(int32 coronaIndex,
                            CVector origin,
                            CVector target,
                            float targetRadius,
                            float power,
                            uint8 unknownFlag,
                            uint8 drawShadow,
                            CVector& useless0,
                            CVector& useless1,
                            CVector& useless2,
                            bool a11,
                            float baseRadius,
                            float a13,
                            float a14,
                            float a15
) {
    ((void(__cdecl*)(int32, CVector, CVector, float, float, uint8, uint8, CVector&, CVector&, CVector&, bool, float, float, float, float))0x6C58E0)(coronaIndex, origin, target, targetRadius, power, unknownFlag, drawShadow, useless0, useless1, useless2, a11, baseRadius, a13, a14, a15);
}

// 0x6C6520
CHeli* CHeli::GenerateHeli(CPed* target, bool newsHeli) {
    auto* heli = new CHeli(newsHeli ? MODEL_VCNMAV : MODEL_POLMAV, PERMANENT_VEHICLE);

    // The heli appears 250 units away from the target, in a random direction
    const auto targetPos = target->GetPosition();
    const auto angleStep = (double)(CGeneral::GetRandomNumber() & 0xFF) * 0.02453125f;
    auto       angle     = (float)angleStep;
    CVector    pos{
        (float)(std::cos(angleStep) * 250.0 + targetPos.x),
        (float)(std::sin((double)angle) * 250.0 + targetPos.y),
        targetPos.z
    };

    // Outside of the map, so flip the direction
    if (pos.x < -3000.0f || pos.x > 3000.0f || pos.y < -3000.0f || pos.y > 3000.0f) {
        angle += PI;
        pos = CVector{
            (float)(std::cos((double)angle) * 250.0 + targetPos.x),
            (float)(std::sin((double)angle) * 250.0 + targetPos.y),
            targetPos.z
        };
    }
    pos.z += 50.0f;

    // Near Mount Chiliad the heli would spawn inside the mountain, so it appears above the target instead
    const auto dx = (double)pos.x - -2322.0;
    const auto dy = (double)pos.y - -1653.0;
    if (std::sqrt(dx * dx + dy * dy) < 350.0) {
        pos = CVector{ targetPos.x, targetPos.y, std::max(targetPos.z + 200.0f, 560.0f) };
    }

    const auto minZ = (double)CWorld::FindGroundZForCoord(pos.x, pos.y) + 20.0;
    if ((double)pos.z <= minZ) {
        pos.z = (float)minZ;
    }
    heli->GetMatrix().SetTranslate(pos);

    heli->vehicleFlags.bIsLocked = true;
    heli->SetStatus(STATUS_PHYSICS);

    if (newsHeli) {
        heli->m_autoPilot.m_nCarMission  = MISSION_HELI_NEWS_BEHAVIOUR;
        heli->m_autoPilot.m_nCruiseSpeed = 35;
    } else {
        heli->m_autoPilot.m_nCarMission  = MISSION_HELI_POLICE_BEHAVIOUR;
        heli->m_autoPilot.m_nCruiseSpeed = 70;
    }
    heli->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(target); // The target is a ped here
    heli->m_fMaxAltitude             = newsHeli ? 30.0f : 20.0f;
    heli->m_fMinAltitude             = newsHeli ? 27.0f : 12.0f;
    heli->m_fHeliRotorSpeed          = 0.165f;

    const auto heading = (double)(angle + PI);
    const auto sinA    = (float)std::sin(heading);
    const auto cosA    = (float)std::cos(heading);
    auto&      matrix  = heli->GetMatrix();
    matrix.GetRight()   = CVector{ sinA, -cosA, 0.0f };
    matrix.GetForward() = CVector{ cosA, sinA, 0.0f };
    matrix.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };

    CWorld::Add(heli);
    heli->SetUpDriver(-1, false, false);
    return heli;
}

// 0x6C6890
void CHeli::TestSniperCollision(CVector* origin, CVector* target) {
    CVector point = *target - *origin;

    if (point.z >= point.Magnitude() / 2.0f)
        return;

    for (auto& heli : pHelis) {
        if (!heli || heli->physicalFlags.bBulletProof)
            continue;

        const auto mat = (CMatrix*)heli->m_matrix;
        if (CCollision::DistToLine(*origin, *target, mat->TransformPoint({ -0.43f, 1.49f, 1.5f })) < 0.8f) {
            heli->m_fRotationBalance = (float)(CGeneral::GetRandomNumber() < pow(2, 14) - 1) * 0.1f - 0.05f; // 2^14 - 1 = 16383 [-0.05, 0.05]
            heli->BlowUpCar(FindPlayerPed(), false);
            heli->m_nNumSwatOccupants = 0;
        };
    }
}

// 0x6C69C0
bool CHeli::SendDownSwat() {
    const auto targetPos = m_autoPilot.m_TargetEntity->GetPosition();

    if (m_nNumSwatOccupants == 0 || physicalFlags.bSubmergedInWater || !bSwatAllowed || (CGeneral::GetRandomNumber() & 0x7F) != 0) {
        return false;
    }

    // Only when hovering close to the target
    const auto heliPos = GetPosition();
    if ((heliPos - targetPos).Magnitude() > 50.0f || GetMoveSpeed().Magnitude() > 0.1f) {
        return false;
    }

    const auto swatIndex = m_nNumSwatOccupants - 1;
    const auto spawnPos  = heliPos + GetMatrix().TransformVector(FindSwatPositionRelativeToHeli(swatIndex));

    // The ground below must be close to the target, and not underwater
    const auto groundZ = CWorld::FindGroundZFor3DCoord(spawnPos);
    if (std::abs(targetPos.z - groundZ) >= 2.5f) {
        return false;
    }
    float waterLevel;
    if (CWaterLevel::GetWaterLevelNoWaves(spawnPos, &waterLevel) && !(waterLevel < groundZ)) {
        return false;
    }

    const auto ropeId = (uint32)this + swatIndex;
    CRopes::RegisterRope(ropeId, 8, spawnPos, false, 0, 0, nullptr, 20'000);

    auto* swat = CPopulation::AddPed(PED_TYPE_COP, (eModelID)COP_TYPE_SWAT2, spawnPos, true);

    auto* sequence = new CTaskComplexSequence();
    sequence->AddTask(new CTaskComplexUseSwatRope(ropeId, this));
    sequence->AddTask(new CTaskComplexWanderCop(PEDMOVE_WALK, (uint8)CGeneral::GetRandomNumberInRange(0, 8)));
    swat->GetTaskManager().SetTask(sequence, TASK_PRIMARY_PRIMARY, false);
    swat->m_bUsesCollision = false;

    m_nNumSwatOccupants--;
    m_aSwatState[m_nNumSwatOccupants] = 0xAA;

    CAnimManager::BlendAnimation(swat->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_ABSEIL, 4.0f);
    return true;
}

// 0x6C79A0
void CHeli::UpdateHelis() {
    ZoneScoped;

    NumberOfSearchLights = 0;

    const auto IsAlive = [](CHeli* heli) { return !heli->physicalFlags.bRenderScorched && !heli->vehicleFlags.bIsDrowning; };

    auto numHelisRequired = FindPlayerWanted()->NumOfHelisRequired();

    // A police heli is already around, so the next one can be a news heli
    auto wantNewsHeli = false;
    auto numHelis     = 0;
    for (auto* heli : pHelis) {
        if (!heli) {
            continue;
        }
        numHelis++;
        if (heli->m_nModelIndex == MODEL_POLMAV && IsAlive(heli)) {
            wantNewsHeli = true;
        }
    }

    if (CCullZones::PlayerNoRain() || CGame::currArea != AREA_CODE_NORMAL_WORLD) {
        numHelisRequired = 0;
    }
    if (!bPoliceHelisAllowed) {
        numHelisRequired = 0;
    }
    if (CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT) {
        numHelisRequired = 0;
    }

    for (auto* heli : pHelis) {
        if (heli && heli->m_nModelIndex == MODEL_VCNMAV) {
            wantNewsHeli = false;
        }
    }
    if (!CWanted::UseNewsHeliInAdditionToPolice) {
        wantNewsHeli = false;
    }

    const auto canGenerate = wantNewsHeli ? bNewsHeliGenerationAllowed : bPoliceHeliGenerationAllowed;
    if (canGenerate && TestForNewRandomHelisTimer < CTimer::GetTimeInMS()) {
        TestForNewRandomHelisTimer = CTimer::GetTimeInMS() + 15'000;
        if (numHelis < numHelisRequired) {
            auto* newHeli = GenerateHeli(FindPlayerPed(), wantNewsHeli);
            for (auto& slot : pHelis) {
                if (!slot) {
                    slot = newHeli;
                    newHeli->RegisterReference(reinterpret_cast<CEntity**>(&slot));
                    break;
                }
            }
        }
    }

    for (auto& slot : pHelis) {
        auto* heli = slot;
        if (!heli) {
            continue;
        }
        if (!IsAlive(heli)) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
            slot = nullptr;
        } else if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER) {
            if ((FindPlayerCoors() - heli->GetPosition()).Magnitude() > 170.0f) {
                CWorld::Remove(heli);
                delete heli;
                slot = nullptr;
            }
        }
    }

    // Helis that are no longer needed fly away
    const auto FlyAwayIfNotNeeded = [&](CHeli* heli, bool countAsNeeded) {
        if (!heli || heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER) {
            return;
        }
        if (numHelisRequired < 1) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
            heli->m_fMinAltitude            = 100.0f;
            heli->m_fMaxAltitude            = 100.0f;
        } else if (countAsNeeded) {
            numHelisRequired--;
        }
    };
    FlyAwayIfNotNeeded(pHelis[0], true);
    FlyAwayIfNotNeeded(pHelis[1], false);
}

// 0x6C7C50
void CHeli::RenderAllHeliSearchLights() {
    ZoneScoped;

    for (auto& light : HeliSearchLights) {
        SearchLightCone(
            light.m_nCoronaIndex,
            light.m_vecOrigin,
            light.m_vecTarget,
            light.m_fTargetRadius,
            light.m_fPower,
            light.field_24,
            light.m_bDrawShadow,
            light.m_vecUseless[0],
            light.m_vecUseless[1],
            light.m_vecUseless[2],
            false,
            0.05f,
            0.0f,
            0.0f,
            1.0f
        );
    }
}

// 0x6C6D30
void CHeli::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    const auto isRCModel = notsa::contains({ MODEL_RCRAIDER, MODEL_RCGOBLIN }, GetModelId());

    // AI controlled helis crash and burn first, they blow up once they hit something afterwards
    if (GetStatus() != STATUS_PLAYER && m_autoPilot.m_nCarMission != MISSION_HELI_CRASH_AND_BURN && !isRCModel) {
        m_autoPilot.SetCarMission(MISSION_HELI_CRASH_AND_BURN);
        m_fHealth = 0.0f;
        return;
    }

    if (damager == FindPlayerPed() || damager == FindPlayerVehicle()) {
        auto& playerInfo = FindPlayerInfo();
        playerInfo.m_nHavocCaused += 20;
        playerInfo.m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, (float)(CGeneral::GetRandomNumber() % 6000 + 4000));
    }

    if (m_nModelIndex == MODEL_VCNMAV) {
        CWanted::UseNewsHeliInAdditionToPolice = false;
    }

    if (GetStatus() == STATUS_PLAYER) {
        m_bUsesCollision = false;
        m_bIsVisible     = false;
        m_vecMoveSpeed   = CVector{};
        m_vecTurnSpeed   = CVector{};
    }

    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp           = CTimer::GetTimeInMS();

    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), eAtomicComponentFlag::ATOMIC_PIPE_NO_EXTRA_PASSES);
    m_damageManager.FuckCarCompletely(false);

    if (!isRCModel) {
        for (auto bumper : { FRONT_BUMPER, REAR_BUMPER }) {
            SetBumperDamage(bumper, false);
        }
        for (auto door : { DOOR_BONNET, DOOR_BOOT, DOOR_LEFT_FRONT, DOOR_RIGHT_FRONT, DOOR_LEFT_REAR, DOOR_RIGHT_REAR }) {
            SetDoorDamage(door, false);
        }
        SpawnFlyingComponent(CAR_WHEEL_LF, 1);
        if (const auto atomic = GetCurrentAtomicObject(m_aCarNodes[CAR_WHEEL_LF])) {
            RpAtomicSetFlags(atomic, 0);
        }
    }

    m_nBombOnBoard = 0;
    m_fHealth      = 0.0f;
    m_wBombTimer   = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    vehicleFlags.bLightsOn     = false;
    vehicleFlags.bEngineOn     = false;
    vehicleFlags.bSirenOrAlarm = false;
    m_nOverrideLights          = NO_CAR_LIGHT_OVERRIDE;
    autoFlags.bTaxiLight       = false;

    if (vehicleFlags.bIsAmbulanceOnDuty) {
        vehicleFlags.bIsAmbulanceOnDuty = false;
        CCarCtrl::NumAmbulancesOnDuty--;
    }
    if (vehicleFlags.bIsFireTruckOnDuty) {
        vehicleFlags.bIsFireTruckOnDuty = false;
        CCarCtrl::NumFireTrucksOnDuty--;
    }

    ChangeLawEnforcerState(false);

    gFireManager.StartFire(this, damager);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);
    CExplosion::AddExplosion(this, damager, isRCModel ? EXPLOSION_RC_VEHICLE : EXPLOSION_AIRCRAFT, GetPosition(), 0, true, -1.0f, false);
}

// 0x6C4530
void CHeli::Fix() {
    m_damageManager.ResetDamageStatus();
    SetupDamageAfterLoad();
}

// 0x6C4330
bool CHeli::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C4320
bool CHeli::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C4830
void CHeli::ProcessControlInputs(uint8 playerNum) {
    auto* pad = CPad::GetPad(playerNum);

    m_fAccelerationBreakStatus = (float)(pad->GetAccelerate() - pad->GetBrake()) * (1.0f / 255.0f);

    const auto SetSteeringFromPad = [&] {
        m_fSteeringUpDown    = (float)pad->GetSteeringUpDown() * (1.0f / 128.0f);
        m_fSteeringLeftRight = (float)-pad->GetSteeringLeftRight() * (1.0f / 128.0f);
    };

    if (CCamera::m_bUseMouse3rdPerson && m_bEnableMouseFlying) {
        const auto& mouse = CPad::NewMouseControllerState.m_AmountMoved;

        // Mouse steering is used once the mouse moves, and kept until the pad is touched
        auto useMouse = mouse.x != 0.0f || mouse.y != 0.0f;
        if (!useMouse && (std::abs(m_fSteeringLeftRight) > 0.0f || std::abs(m_fSteeringUpDown) > 0.0f) && m_nLastControlInput == eControllerType::MOUSE) {
            useMouse = pad->GetSteeringLeftRight() == 0 && pad->GetSteeringUpDown() == 0;
        }

        if (useMouse) {
            m_nLastControlInput = eControllerType::MOUSE;
            if (!pad->NewState.m_bVehicleMouseLook) {
                m_fSteeringLeftRight -= mouse.x * 0.0025f;
                m_fSteeringUpDown += mouse.y * 0.0025f;
            }

            // Steering returns to the center
            if (std::abs(m_fSteeringLeftRight) < 0.5f) {
                m_fSteeringLeftRight = (float)((double)m_fSteeringLeftRight * std::pow(0.98f, (double)CTimer::GetTimeStep()));
            }
            if (std::abs(m_fSteeringUpDown) < 0.5f) {
                m_fSteeringUpDown = (float)((double)m_fSteeringUpDown * std::pow(0.98f, (double)CTimer::GetTimeStep()));
            }
        } else if (pad->GetSteeringLeftRight() != 0 || pad->GetSteeringUpDown() != 0 || m_nLastControlInput != eControllerType::MOUSE) {
            m_nLastControlInput = eControllerType::KEYBOARD;
            SetSteeringFromPad();
        }
    } else {
        m_nLastControlInput = eControllerType::KEYBOARD;
        SetSteeringFromPad();
    }

    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown, -1.0f, 1.0f);
    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);

    m_fLeftRightSkid = (float)pad->GetLookRight();
    if (pad->GetLookLeft()) {
        m_fLeftRightSkid = -1.0f;
    }

    // Holding the horn levels the heli out
    if (pad->GetHorn() && GetMatrix().GetUp().z > 0.0f) {
        m_fLeftRightSkid = 0.0f;

        const auto speed = GetMoveSpeed();
        const auto Counteract = [&](CVector dir, float stability) {
            dir.Normalise();
            const auto along = (double)dir.y * speed.y + (double)dir.z * speed.z + (double)dir.x * speed.x;
            return (float)std::clamp(along * stability, -2.0, 2.0);
        };
        m_fSteeringUpDown    = Counteract(CrossProduct(CVector{ 0.0f, 0.0f, 1.0f }, GetMatrix().GetRight()), m_pFlyingHandlingData->m_fPitchStab);
        m_fSteeringLeftRight = Counteract(CrossProduct(GetMatrix().GetForward(), CVector{ 0.0f, 0.0f, 1.0f }), m_pFlyingHandlingData->m_fRollStab);
    }

    vehicleFlags.bIsHandbrakeOn = false;
    m_fSteerAngle               = 0.0f;
    m_GasPedal                  = 0.0f;
    m_BrakePedal                = 1.0f;

    if (pad->DisablePlayerControls) {
        FindPlayerPed()->KeepAreaAroundPlayerClear();

        const auto speed = GetMoveSpeed().Magnitude();
        if (speed > 0.28f) {
            GetMoveSpeed() *= 0.28f / speed;
        }
    }

    // A badly damaged heli loses control
    if (m_fHealth < 250.0f) {
        m_fAccelerationBreakStatus = -0.1f;
        m_fLeftRightSkid += 0.5f;
    }
}

// 0x6C4400
void CHeli::Render() {
    auto* mi = GetVehicleModelInfo();
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    mi->SetVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor);

    auto staticRotor = m_aCarNodes[HELI_STATIC_ROTOR];
    RpAtomic* data = nullptr;
    if (staticRotor) {
        RwFrameForAllObjects(staticRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto staticRotor2 = m_aCarNodes[HELI_STATIC_ROTOR2];
    data = nullptr;
    if (staticRotor2) {
        RwFrameForAllObjects(staticRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto movingRotor = m_aCarNodes[HELI_MOVING_ROTOR];
    data = nullptr;
    if (movingRotor) {
        RwFrameForAllObjects(movingRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    auto movingRotor2 = m_aCarNodes[HELI_MOVING_ROTOR2];
    data = nullptr;
    if (movingRotor2) {
        RwFrameForAllObjects(movingRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    CEntity::Render(); // exactly CEntity
}

// 0x6C4550
void CHeli::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6C4E60
void CHeli::ProcessFlyingCarStuff() {
    const auto status    = GetStatus();
    const auto isRCModel = notsa::contains({ MODEL_RCRAIDER, MODEL_RCGOBLIN }, GetModelId());

    if (status == STATUS_PLAYER || status == STATUS_REMOTE_CONTROLLED || status == STATUS_PHYSICS) {
        if (m_fHeliRotorSpeed < 0.22f && !physicalFlags.bSubmergedInWater) {
            m_fHeliRotorSpeed += isRCModel ? 0.003f : 0.001f;
        }

        if (m_fHeliRotorSpeed > 0.15f) {
            const auto isFloatingOnWater = physicalFlags.bTouchingWater && notsa::contains({ MODEL_SEASPAR, MODEL_LEVIATHN }, GetModelId());
            if (vehicleFlags.bIsRCVehicle) {
                FlyingControl(FLIGHTMODEL_RCHELI, m_fLeftRightSkid, m_fSteeringUpDown, m_fSteeringLeftRight, m_fAccelerationBreakStatus);
            } else {
                const auto needsControl = (m_nNumContactWheels < 4 && !isFloatingOnWater)
                    || m_fAccelerationBreakStatus > 0.0f
                    || std::abs(GetMoveSpeed().x) > 0.02f
                    || std::abs(GetMoveSpeed().y) > 0.02f
                    || std::abs(GetMoveSpeed().z) > 0.02f;
                if (needsControl) {
                    FlyingControl(FLIGHTMODEL_HELI, m_fLeftRightSkid, m_fSteeringUpDown, m_fSteeringLeftRight, m_fAccelerationBreakStatus);
                }
            }
        }

        if (m_fHeliRotorSpeed > 0.015f) {
            if (auto* const rotorFrame = m_aCarNodes[CAR_BUMP_FRONT]) {
                CMatrix rotorMatrix;
                rotorMatrix.Attach(RwFrameGetMatrix(rotorFrame), false);

                // The blades hurt whatever they touch
                if (auto* const rotor = (RpAtomic*)GetCurrentAtomicObject(rotorFrame)) {
                    const auto radius = RpAtomicGetBoundingSphere(rotor)->radius;
                    if (radius > 0.1f) {
                        auto damageMult = 1.0f;
                        if (isRCModel) {
                            damageMult = 0.9f;
                        } else if (notsa::contains({ MODEL_SPARROW, MODEL_SEASPAR }, GetModelId())) {
                            damageMult = 0.8f;
                        } else if (m_nModelIndex == MODEL_HUNTER) {
                            damageMult = 0.5f;
                        }
                        if (status == STATUS_PLAYER || status == STATUS_REMOTE_CONTROLLED) {
                            DoBladeCollision(rotorMatrix.GetPosition(), GetMatrix(), -3, radius, damageMult);
                        }
                    }
                }

                // The spinning rotor pushes the wind around
                if ((status == STATUS_PLAYER || status == STATUS_PHYSICS) && m_fHeliRotorSpeed > 0.0075f) {
                    CWindModifiers::RegisterOne(GetPosition(), 1, std::min(m_fHeliRotorSpeed * (20.0f / 3.0f), 1.0f));
                }
            }
        }
    } else {
        // Nobody is flying it, so the rotor spins down
        if (!m_pHandlingData->m_bIsHeli) {
            return;
        }
        vehicleFlags.bEngineOn = false;

        const auto spinDown = (double)CTimer::GetTimeStep() * 0.00055f;
        if (spinDown < m_fHeliRotorSpeed) {
            m_fHeliRotorSpeed = (float)((double)m_fHeliRotorSpeed - spinDown);
            m_nFakePhysics = 0;
        } else {
            m_fHeliRotorSpeed = 0.0f;
        }
    }

    // Blades passing close to the camera
    if (isRCModel || !(m_fHeliRotorSpeed < 0.154f && m_fHeliRotorSpeed > 0.0044f)) {
        return;
    }
    auto* const rotorFrame = m_aCarNodes[CAR_BUMP_FRONT];
    if (!rotorFrame) {
        return;
    }

    const auto toCamera = TheCamera.GetPosition() - GetPosition();
    const auto distSq   = (float)((double)toCamera.x * toCamera.x + (double)toCamera.y * toCamera.y + (double)toCamera.z * toCamera.z);
    if (!(distSq < 400.0f) || !(std::abs(m_fPropRotate - m_wheelRotation[1]) > 0.5235988f)) {
        return;
    }

    CMatrix rotorMatrix;
    rotorMatrix.Attach(RwFrameGetMatrix(rotorFrame), false);
    const auto bladeDir = GetMatrix().TransformVector(rotorMatrix.GetRight());
    const auto invDist  = 1.0 / std::max((double)std::sqrt((double)distSq), 0.01);
    const auto facing   = (double)bladeDir.z * (toCamera.z * invDist) + (double)bladeDir.y * (toCamera.y * invDist) + (double)bladeDir.x * (toCamera.x * invDist);
    if (std::abs(facing) > 0.95) {
        m_vehicleAudio.AddAudioEvent(AE_HELI_BLADE, 0.0f);
        m_fPropRotate = m_wheelRotation[1];
    }
}

// 0x6C5420
void CHeli::PreRender() {
    CVehicle::PreRender();

    if (m_bSearchLightEnabled && m_fSearchLightIntensity > 0.0f && CClock::GetIsTimeInRange(19, 6)) {
        AddHeliSearchLight(
            GetMatrix().TransformPoint(CVector{ 0.0f, 3.5f, -0.3f }),
            m_vecSearchLightTarget,
            20.0f,
            m_fSearchLightIntensity,
            (uint32)this + 0xB,
            1,
            1
        );
    }

    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        const auto* mi = GetVehicleModelInfo();
        for (auto i = 0; i < 4; i++) {
            // How relaxed the suspension is past its resting length
            const auto restRatio  = 1.0 - (double)m_aSuspensionSpringLength[i] / m_aSuspensionLineLength[i];
            const auto relaxation = (float)(((double)m_fWheelsSuspensionCompression[i] - restRatio) / (1.0 - restRatio));

            CVector wheelPos;
            mi->GetWheelPosn(i, wheelPos, true);

            auto& currentPos = m_wheelPosition[i];
            auto  pos        = (double)wheelPos.z + m_pHandlingData->m_fSuspensionUpperLimit;
            if (relaxation > 0.0f) {
                pos -= (double)relaxation * m_aSuspensionSpringLength[i];
            }
            if (pos <= currentPos && (!physicalFlags.bAddMovingCollisionSpeed || !(m_nHandlingFlagsIntValue & VEHICLE_HANDLING_HYDRAULIC_INST))) {
                pos = (pos - currentPos) * 0.75 + currentPos;
            }
            currentPos = (float)pos;
        }
    }

    for (const auto node : { CAR_WHEEL_RB, CAR_WHEEL_LB, CAR_WHEEL_RF, CAR_WHEEL_LF }) {
        UpdateWheelMatrix(node, 1);
    }

    if (m_nModelIndex != MODEL_RCRAIDER && m_nModelIndex != MODEL_RCGOBLIN) {
        DoHeliDustEffect(1.0f, 1.0f);
    }

    // Spin the rotors
    constexpr float FULL_TURN = 2.0f * PI;

    auto mainRotorStep = (double)CTimer::GetTimeStep() * m_fHeliRotorSpeed;
    if (notsa::contains({ MODEL_SPARROW, MODEL_SEASPAR, MODEL_MAVERICK, MODEL_VCNMAV, MODEL_POLMAV }, GetModelId())) {
        mainRotorStep *= 1.66f;
    }
    m_fRotorZ = (float)((double)m_fRotorZ - mainRotorStep);
    if (m_fRotorZ < -FULL_TURN) {
        auto angle = (double)m_fRotorZ;
        do {
            angle += FULL_TURN;
        } while (angle < -FULL_TURN);
        m_fRotorZ = (float)angle;
    }

    auto secondRotorStep = (double)CTimer::GetTimeStep() * m_fHeliRotorSpeed;
    if (m_nModelIndex == MODEL_LEVIATHN) {
        secondRotorStep += secondRotorStep;
    } else {
        secondRotorStep *= 2.3f;
    }
    m_fSecondRotorZ = (float)((double)m_fSecondRotorZ - secondRotorStep);
    if (m_fSecondRotorZ > FULL_TURN) {
        auto angle = (double)m_fSecondRotorZ;
        do {
            angle -= FULL_TURN;
        } while (angle > FULL_TURN);
        m_fSecondRotorZ = (float)angle;
    }

    CMatrix matrix;
    const auto RotateNode = [&](eCarNodes node, auto&& setRotation) {
        if (auto* frame = m_aCarNodes[node]) {
            matrix.Attach(RwFrameGetMatrix(frame), false);
            const auto pos = matrix.GetPosition();
            setRotation();
            matrix.GetPosition() += pos;
            matrix.UpdateRW();
        }
    };
    RotateNode(CAR_BUMP_FRONT, [&] { matrix.SetRotateZ(m_fRotorZ); });
    RotateNode(CAR_BUMP_REAR, [&] { matrix.SetRotateZ(m_fRotorZ); });
    RotateNode(CAR_WING_RF, [&] { matrix.SetRotateX(m_fSecondRotorZ); });
    RotateNode(CAR_WING_LF, [&] { matrix.SetRotateX(m_fSecondRotorZ); });

    CShadows::StoreShadowForVehicle(this, VEH_SHD_HELI);
}

// 0x6C7050
void CHeli::ProcessControl() {
    plugin::CallMethod<0x6C7050, CHeli*>(this);
}
