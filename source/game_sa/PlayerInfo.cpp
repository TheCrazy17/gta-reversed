#include "StdInc.h"

#include "PlayerInfo.h"
#include "Cranes.h"
#include "PedGeometryAnalyser.h"
#include "CarEnterExit.h"
#include "FireManager.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "Replay.h"
#include "Enums/eWantedLevel.h"
#include "Pad.h"
#include "Bridge.h"
#include "Stats.h"
#include "GameLogic.h"
#include "CullZones.h"
#include "Scripts/TheScripts.h"
#include "Events/EventScriptCommand.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsDriver.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsPassenger.h"
#include "Tasks/TaskTypes/TaskComplexLeaveCar.h"
#include "Tasks/TaskTypes/TaskComplexGoPickUpEntity.h"
#include "Tasks/TaskTypes/TaskSimpleJetPack.h"
#include "Tasks/TaskTypes/TaskSimpleHoldEntity.h"
#include "Tasks/TaskTypes/TaskSimpleSwim.h"
#include "Entity/Vehicle/Automobile.h"
#include "Entity/Vehicle/Bike.h"
#include "Renderer.h"

void CPlayerInfo::InjectHooks() {
    RH_ScopedClass(CPlayerInfo);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x571920, { .State = HS::RedirectToGTA, .Locked = true }); // hooking ctor will produce bugs with weapons, you will never give weapon through cheat or something
    RH_ScopedInstall(CancelPlayerEnteringCars, 0x56E860);
    RH_ScopedInstall(FindObjectToSteal, 0x56DBD0);
    RH_ScopedInstall(EvaluateCarPosition, 0x56DAD0);
    RH_ScopedInstall(Process, 0x56F8D0);
    RH_ScopedInstall(FindClosestCarSectorList, 0x56F4E0);
    RH_ScopedInstall(Clear, 0x56F330);
    RH_ScopedInstall(StreamParachuteWeapon, 0x56EB30);
    RH_ScopedInstall(AddHealth, 0x56EAB0);
    RH_ScopedInstall(ArrestPlayer, 0x56E5D0);
    RH_ScopedInstall(SetPlayerSkin, 0x5717F0);
    RH_ScopedInstall(LoadPlayerSkin, 0x56F7D0);
    RH_ScopedInstall(DeletePlayerSkin, 0x56EA80);
    RH_ScopedInstall(BlowUpRCBuggy, 0x56EA30);
    RH_ScopedInstall(MakePlayerSafe, 0x56E870);
    RH_ScopedInstall(PlayerFailedCriticalMission, 0x56E830);
    RH_ScopedInstall(WorkOutEnergyFromHunger, 0x56E610);
    RH_ScopedInstall(KillPlayer, 0x56E580);
    RH_ScopedInstall(IsRestartingAfterMissionFailed, 0x56E570);
    RH_ScopedInstall(IsRestartingAfterArrest, 0x56E560);
    RH_ScopedInstall(IsRestartingAfterDeath, 0x56E550);
    RH_ScopedInstall(IsPlayerInRemoteMode, 0x56DAB0);
    RH_ScopedInstall(GetPos_Hook, 0x56DFB0);
    RH_ScopedInstall(GetSpeed_Hook, 0x56DF50);
    RH_ScopedInstall(GivePlayerParachute, 0x56EC40);
    RH_ScopedInstall(SetLastTargetVehicle, 0x56DA80);
    RH_ScopedInstall(Load, 0x5D3B00);
    RH_ScopedInstall(Save, 0x5D3AC0);
}

// 0x571920
CPlayerInfo::CPlayerInfo() {
    plugin::CallMethod<0x571920, CPlayerInfo*>(this); // see hook
    return;

    m_PlayerData = CPlayerPedData();

    m_pSkinTexture = nullptr;
    m_bParachuteReferenced = false;
    m_nRequireParachuteTimer = 0;
}

// 0x571920
CPlayerInfo* CPlayerInfo::Constructor() {
    this->CPlayerInfo::CPlayerInfo();
    return this;
}

CVector* CPlayerInfo::GetSpeed_Hook(CVector* out) {
    *out = GetSpeed();
    return out;
}

CVector* CPlayerInfo::GetPos_Hook(CVector* outPos) {
    *outPos = GetPos();
    return outPos;
}

// 0x56E860
void CPlayerInfo::CancelPlayerEnteringCars(CVehicle* vehicle) {
    // NOP
}

// 0x56DBD0
CEntity* CPlayerInfo::FindObjectToSteal(CPed* ped) {
    const auto& fwd = ped->GetForward();
    const auto& pos = ped->GetPosition();

    const CVector center{
        fwd.x * 0.5f * 3.0f + pos.x,
        fwd.y * 0.5f * 3.0f + pos.y,
        fwd.z * 0.5f * 3.0f + pos.z
    };

    const auto minX = std::max(0, CWorld::GetSectorX(center.x - 3.0f));
    const auto minY = std::max(0, CWorld::GetSectorY(center.y - 3.0f));
    const auto maxX = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(center.x + 3.0f));
    const auto maxY = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(center.y + 3.0f));

    CWorld::AdvanceCurrentScanCode();

    CEntity* found = nullptr;
    if (maxY < minY) {
        return found;
    }

    for (auto y = minY; y <= maxY; y++) {
        if (minX > maxX) {
            continue;
        }
        for (auto x = minX; x <= maxX; x++) {
            for (auto* object : CWorld::GetRepeatSector(x, y).Objects) {
                if (!object->objectFlags.bIsLiftable || object->IsScanCodeCurrent()) {
                    continue;
                }

                const auto delta = object->GetPosition() - center;
                if (delta.SquaredMagnitude() >= 4.5f) {
                    continue;
                }

                auto dot = DotProduct(delta, fwd);
                if (dot < 0.0f) {
                    dot = 10.0f - dot;
                }
                if (dot + std::fabs(DotProduct(delta, ped->GetRight())) * 3.0f < 1000.0f) {
                    found = object;
                }
            }
        }
    }
    return found;
}

// 0x56DAD0
void CPlayerInfo::EvaluateCarPosition(CEntity* car, CPed* ped, float pedToVehDist, float* outDistance, CVehicle** outVehicle) {
    const auto& carPosn = car->GetPosition();
    const auto& pedPosn = ped->GetPosition();
    const auto& forward = ped->GetForward();

    // Find our rotation (so that is, at which angle the forward vector is)
    const auto angleFront = CGeneral::GetATanOfXY(forward.x, forward.y);

    // Find car angle, not relative to our rotation
    const auto carAngle = CGeneral::GetATanOfXY(carPosn.x - pedPosn.x, carPosn.y - pedPosn.y);

    // Make car's angle relative to our rotation (notice: it's an abs value)
    // Basically, we calculate how much the car is in our FOV.
    const auto carAngleFromFront = std::abs(CGeneral::LimitRadianAngle(angleFront - carAngle));

    // Calculate imaginary distance based on the car's angle: The higher the angle the greater the distance
    const auto distance = (1.0f - carAngleFromFront / TWO_PI) * (10.0f - pedToVehDist);
    if (distance >= *outDistance) {
        *outDistance = distance;
        *outVehicle = car->AsVehicle();
    }
}

// 0x56F7D0
void CPlayerInfo::LoadPlayerSkin() {
    DeletePlayerSkin();
    m_pSkinTexture = CPlayerSkin::GetSkinTexture(m_szSkinName);
}

// 0x56EA80
void CPlayerInfo::DeletePlayerSkin() {
    if (m_pSkinTexture) {
        RwTextureDestroy(m_pSkinTexture);
        m_pSkinTexture = nullptr;
    }
}

// 0x5717F0
void CPlayerInfo::SetPlayerSkin(const char* name) {
    strcpy_s(m_szSkinName, name); // NOTSA: They used `strcpy`, we use `_s` for safety
    LoadPlayerSkin();
}

// 0x56DA80
void CPlayerInfo::SetLastTargetVehicle(CVehicle* vehicle) {
    CEntity::SafeCleanUpRef(m_pLastTargetVehicle);
    m_pLastTargetVehicle = vehicle;
    CEntity::SafeRegisterRef(m_pLastTargetVehicle);
}

// 0x56F8D0
void CPlayerInfo::Process(uint32 playerIndex) {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    auto* const pad = CPad::GetPad(playerIndex);
    auto* const ped = m_pPed;

    // Fare of the taxi driver, one dollar for every second with a passenger
    if (m_bTaxiTimerScore && ped->bInVehicle) {
        auto* const veh = ped->m_pVehicle;
        if ((veh->m_nModelIndex == MODEL_TAXI || veh->m_nModelIndex == MODEL_CABBIE) && veh->m_pDriver == ped && veh->m_nNumPassengers != 0) {
            if (const auto elapsed = CTimer::GetTimeInMS() - m_nTaxiTimer; elapsed >= 1000) {
                const auto seconds = elapsed / 1000;
                m_nMoney += seconds;
                m_nTaxiTimer += seconds * 1000;
            }
        } else {
            m_nTaxiTimer = CTimer::GetTimeInMS();
        }
    } else {
        m_nTaxiTimer = CTimer::GetTimeInMS();
    }

    ProcessStuntStats();

    WorkOutEnergyFromHunger();

    // The money that is shown on the HUD counts up (or down) to the real amount
    if (m_nDisplayMoney != m_nMoney) {
        const auto diff    = m_nMoney - m_nDisplayMoney;
        const auto absDiff = std::abs(diff);
        auto       step    = absDiff > 100000 ? 12345
                           : absDiff > 10000  ? 1234
                           : absDiff > 1000   ? 123
                           : absDiff < 51     ? 1 : 42;
        m_nDisplayMoney += diff < 0 ? -step : step;
    }

    ped->m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    m_nMaxHealth      = (uint8)(int32)ped->m_fMaxHealth;

    // Parachute is needed for the pilots that have enough skill
    if (!ped->bInVehicle || CStats::GetStatValue(STAT_FLYING_SKILL) < 400.0f) {
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced     = false;
            m_nRequireParachuteTimer = 0;
        }
    } else {
        StreamParachuteWeapon(true);
    }

    // Density of the roads around the player
    if ((CTimer::GetFrameCounter() & 0xF) == 0) {
        const auto& pos = ped->bInVehicle ? ped->m_pVehicle->GetPosition() : ped->GetPosition();
        m_fRoadDensityAroundPlayer = ThePaths.CalcRoadDensity(pos.x, pos.y);
    }
    m_fRoadDensityAroundPlayer = std::min(std::max((m_fRoadDensityAroundPlayer - 1.0f) * 0.6f + 1.0f, 0.5f), 1.45f);

    ProcessEnterExitVehicle(playerIndex, pad);

    ProcessRemoteVehicleExplosion();

    ProcessUpsideDownVehicle();

    ProcessMovementStats();

    ProcessChaseValue();

    // Unknown part of the processing of the pad
    plugin::CallMethod<0x56EC80, CPlayerInfo*, uint32, CPad*>(this, playerIndex, pad);

    m_nMoney        = std::min(m_nMoney, 999999999);
    m_nDisplayMoney = std::min(m_nDisplayMoney, 999999999);
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessStuntStats() {
    auto* const ped = m_pPed;

    const auto timeMs = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

    const auto ResetCarLess3Wheels  = [&] { m_nCarLess3WheelCounter = 0; };
    const auto ResetCarTwoWheels    = [&] { m_nTempBufferCounter = 0; m_nCarTwoWheelCounter = 0; };
    const auto ResetBikeCounters    = [&] { m_nBikeRearWheelCounter = 0; m_nBikeFrontWheelCounter = 0; };
    const auto ResetCarTwoWheelsAlt = [&] { m_nTempBufferCounter = 0; m_nCarTwoWheelCounter = 0; m_nCarLess3WheelCounter = 0; };

    // The temp counter is the time the wheels may be on the ground again without ending the stunt
    const auto DecayTempCounter = [&](float factor) {
        m_nTempBufferCounter = (uint32)(int32)std::max(0.0f, (float)m_nTempBufferCounter - (float)timeMs * factor);
    };
    const auto GraceTempCounter = [&] {
        m_nTempBufferCounter += timeMs;
    };

    if (!ped->bInVehicle || !ped->m_pVehicle) {
        ResetCarLess3Wheels();
        ResetCarTwoWheels();
        ResetBikeCounters();
        return;
    }

    auto* const veh = ped->m_pVehicle;
    if (veh->IsAutomobile()) {
        auto* const car = veh->AsAutomobile();

        if (car->m_nNumContactWheels < 3) {
            m_nCarLess3WheelCounter += timeMs;
        } else {
            m_nCarLess3WheelCounter = 0;
        }

        const auto& prev      = car->m_fWheelsSuspensionCompressionPrev;
        const auto  noDamage  = car->m_fDamageIntensity == 0.0f;
        const auto  rightAir  = prev[2] == 1.0f && prev[3] == 1.0f;
        const auto  leftAir   = prev[0] == 1.0f && prev[1] == 1.0f;

        const auto RecordTwoWheels = [&] {
            m_fBestCarTwoWheelsDistM = m_fCarTwoWheelDist;
            m_nBestCarTwoWheelsTimeMs = m_nCarTwoWheelCounter;
            CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_TIME, (float)(m_nCarTwoWheelCounter / 1000));
            CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_DISTANCE, m_fCarTwoWheelDist);
        };

        // Two wheels of the same side on the ground and the other two in the air
        const auto onTwoWheels = rightAir
            ? (prev[0] < 1.0f && prev[1] < 1.0f && noDamage)
            : (leftAir && prev[2] < 1.0f && prev[3] < 1.0f && noDamage);

        if (onTwoWheels) {
            m_nCarTwoWheelCounter += timeMs;
            m_fCarTwoWheelDist    += car->m_fMovingSpeed;
            DecayTempCounter(0.5f);
            ResetBikeCounters();
        } else if (!rightAir && !leftAir) {
            if (m_nCarTwoWheelCounter != 0) {
                if (m_nCarTwoWheelCounter >= 2000) {
                    RecordTwoWheels();
                }
                m_fCarTwoWheelDist = 0.0f;
                ResetCarTwoWheels();
            }
            ResetBikeCounters();
        } else {
            if (m_nCarTwoWheelCounter != 0 && m_nTempBufferCounter < 500) {
                GraceTempCounter();
            } else {
                if (m_nCarTwoWheelCounter >= 2000) {
                    RecordTwoWheels();
                }
                m_fCarTwoWheelDist = 0.0f;
                ResetCarTwoWheels();
            }
            ResetBikeCounters();
        }
        return;
    }

    if (!veh->IsBike()) {
        ResetCarLess3Wheels();
        ResetCarTwoWheels();
        ResetBikeCounters();
        return;
    }

    // Wheelies and stoppies
    auto* const bike     = veh->AsBike();
    const auto& wheels   = bike->m_aRatioHistory;
    const auto  noDamage = bike->m_fDamageIntensity == 0.0f;

    const auto RecordWheelie = [&] {
        m_fBestBikeWheelieDistM  = m_fBikeRearWheelDist;
        m_nBestBikeWheelieTimeMs = m_nBikeRearWheelCounter;
        CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_TIME, (float)(m_nBikeRearWheelCounter / 1000));
        CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_DISTANCE, m_fBikeRearWheelDist);
    };
    const auto RecordStoppie = [&] {
        m_fBestBikeStoppieDistM  = m_fBikeFrontWheelDist;
        m_nBestBikeStoppieTimeMs = m_nBikeFrontWheelCounter;
        CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_TIME, (float)(m_nBikeFrontWheelCounter / 1000));
        CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_DISTANCE, m_fBikeFrontWheelDist);
    };
    const auto ResetAll = [&] { ResetCarTwoWheelsAlt(); };

    if (wheels[0] == 1.0f && wheels[1] == 1.0f && m_nBikeFrontWheelCounter == 0) {
        // Front wheel is up
        if (wheels[2] < 1.0f || (wheels[3] < 1.0f && noDamage)) {
            m_nBikeRearWheelCounter += timeMs;
            m_fBikeRearWheelDist    += bike->m_fMovingSpeed;
            DecayTempCounter(0.2f);
            m_nCarTwoWheelCounter  = 0;
            m_nCarLess3WheelCounter = 0;
        } else if (m_nBikeRearWheelCounter != 0 && m_nTempBufferCounter < 500) {
            GraceTempCounter();
            m_nCarTwoWheelCounter  = 0;
            m_nCarLess3WheelCounter = 0;
        } else {
            if (m_nBikeRearWheelCounter >= 5000) {
                RecordWheelie();
            }
            m_nBikeRearWheelCounter = 0;
            m_fBikeRearWheelDist    = 0.0f;
            ResetAll();
        }
    } else if (m_nBikeRearWheelCounter != 0) {
        if (m_nBikeRearWheelCounter >= 5000) {
            RecordWheelie();
        }
        m_nBikeRearWheelCounter = 0;
        m_fBikeRearWheelDist    = 0.0f;
        ResetAll();
    } else {
        if (wheels[2] == 1.0f && wheels[3] == 1.0f) {
            // Rear wheel is up
            if (wheels[0] < 1.0f || (wheels[1] < 1.0f && noDamage)) {
                m_nBikeFrontWheelCounter += timeMs;
                m_fBikeFrontWheelDist    += bike->m_fMovingSpeed;
                DecayTempCounter(0.2f);
                m_nCarTwoWheelCounter  = 0;
                m_nCarLess3WheelCounter = 0;
                return;
            }
            if (m_nBikeFrontWheelCounter != 0 && m_nTempBufferCounter < 500) {
                GraceTempCounter();
                m_nCarTwoWheelCounter  = 0;
                m_nCarLess3WheelCounter = 0;
                return;
            }
        }
        if (m_nBikeFrontWheelCounter >= 2000) {
            RecordStoppie();
        }
        m_nBikeFrontWheelCounter = 0;
        m_fBikeFrontWheelDist    = 0.0f;
        ResetAll();
    }
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessEnterExitVehicle(uint32 playerIndex, CPad* pad) {
    auto* const ped = m_pPed;

    if (pad->GetTarget() || !ped->bCanExitCar || ped->bUsingMobilePhone) {
        m_bTryingToExitCar = false;
        return;
    }
    if (!pad->ExitVehicleJustDown() && !(m_bTryingToExitCar && pad->GetExitVehicle() && ped->bInVehicle)) {
        m_bTryingToExitCar = false;
        return;
    }

    m_bTryingToExitCar = false;
    auto& taskMgr      = ped->GetIntelligence()->GetTaskManager();

    if (ped->bInVehicle) {
        // Leave the vehicle
        if (m_pRemoteVehicle) {
            return;
        }
        auto* const veh = ped->m_pVehicle;
        if (veh->m_pEntityWeAreOn && CBridge::ThisIsABridgeObjectMovingUp(veh->m_pEntityWeAreOn->m_nModelIndex)) {
            return;
        }
        if (veh->GetStatus() == STATUS_WRECKED || veh->GetStatus() == STATUS_TRAIN_MOVING || veh->m_nDoorLock == CARLOCK_LOCKED_PLAYER_INSIDE) {
            return;
        }
        if (auto* const task = taskMgr.GetActiveTask(); task && !task->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            m_bTryingToExitCar = true;
            return;
        }

        if (veh->IsBoat()) {
            taskMgr.SetTask(new CTaskComplexLeaveCar(veh, 0, 0, true, false), TASK_PRIMARY_PRIMARY, false);
            ped->bTryingToReachDryLand = true;
            return;
        }
        if (veh->CanPedStepOutCar(false) || veh->CanPedJumpOutCar(ped)) {
            taskMgr.SetTask(new CTaskComplexLeaveCar(veh, 0, 0, true, false), TASK_PRIMARY_PRIMARY, false);
            m_bTryingToExitCar = true;
            GivePlayerParachute();
        } else if (veh->GetStatus() == STATUS_PLAYER) {
            m_bTryingToExitCar = true; // Not slow enough yet, tries to leave as soon as it is
        }
        return;
    }

    // On foot, enter something
    if (ped->m_pAttachedTo) {
        return;
    }

    auto* const objectToSteal = FindObjectToSteal(ped);
    auto* const simplestTask  = taskMgr.GetSimplestActiveTask();
    auto* const holdTask      = ped->GetIntelligence()->GetTaskHold(false);
    if (simplestTask->GetTaskType() == TASK_SIMPLE_CLIMB
        || ped->GetIntelligence()->GetTaskFighting()
        || (holdTask && holdTask->m_pEntityToHold)
    ) {
        return;
    }

    if (objectToSteal) {
        if (auto* const task = taskMgr.GetActiveTask(); task && !task->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return;
        }
        auto* const task = (CTaskComplexGoPickUpEntity*)CTask::operator new(sizeof(CTaskComplexGoPickUpEntity));
        plugin::CallMethod<0x6919C0, CTaskComplexGoPickUpEntity*, CEntity*, int32>(task, objectToSteal, 0x51);
        CEventScriptCommand event{ 3, task, false };
        ped->GetIntelligence()->m_eventGroup.Add(&event, false);
        return;
    }

    // Finds the closest vehicle in the surroundings
    const auto pos  = ped->GetPosition();
    const auto minX = pos.x - 10.0f;
    const auto maxX = pos.x + 10.0f;
    const auto minY = pos.y - 10.0f;
    const auto maxY = pos.y + 10.0f;

    const auto ToSector = [](float coord) { return (int32)std::floor(coord * 0.02f + 60.0f); };
    const auto startX   = ToSector(minX);
    const auto startY   = ToSector(minY);
    const auto endX     = ToSector(maxX);
    const auto endY     = ToSector(maxY);

    CVehicle* closestVeh{};
    float     closestDist{};
    CWorld::AdvanceCurrentScanCode();
    for (auto y = startY; y <= endY; y++) {
        for (auto x = startX; x <= endX; x++) {
            FindClosestCarSectorList(CWorld::GetRepeatSector(x, y).Vehicles, ped, minX, minY, maxX, maxY, &closestDist, &closestVeh);
        }
    }
    if (!closestVeh) {
        return;
    }

    auto* targetVeh = closestVeh;
    if (targetVeh->m_nVehicleSubType == VEHICLE_TYPE_TRAILER && targetVeh->m_pTowingVehicle) {
        targetVeh = targetVeh->m_pTowingVehicle;
    }
    if (!targetVeh || !targetVeh->CanBeDriven()) {
        return;
    }

    if (auto* const jetPack = ped->GetIntelligence()->GetTaskJetPack()) {
        jetPack->DropJetPack(ped);
    }

    // Boats are entered as driver only when empty
    if (targetVeh->IsBoat()) {
        if (targetVeh->m_pDriver) {
            return;
        }
        if (auto* const task = taskMgr.GetActiveTask(); task && !task->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return;
        }
        taskMgr.SetTask(new CTaskComplexEnterCarAsDriver(targetVeh), TASK_PRIMARY_PRIMARY, false);
        return;
    }

    if (auto* const task = taskMgr.GetActiveTask()) {
        // Swimming to an aircraft that floats, the door is where the swimmer goes to
        auto swimToDoor = false;
        if (task->GetTaskType() == TASK_COMPLEX_IN_WATER) {
            if (auto* const swim = ped->GetIntelligence()->GetTaskSwim()) {
                const auto model = targetVeh->m_nModelIndex;
                if (model == MODEL_SKIMMER || model == MODEL_VORTEX || model == MODEL_SEASPAR || model == MODEL_LEVIATHN) {
                    CVector doorPos;
                    int32   doorId{};
                    if (CCarEnterExit::GetNearestCarDoor(ped, targetVeh, doorPos, doorId)) {
                        swim->m_vecPos = doorPos;
                        *reinterpret_cast<uint32*>(reinterpret_cast<uint8*>(swim) + 0x58) = 5000;
                        swimToDoor = true;
                    }
                }
            }
        }
        if (!swimToDoor && !task->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return;
        }
    }

    // Without a second player everything is simple
    if (!CWorld::Players[1].m_pPed || CGameLogic::bPlayersCanBeInSeparateCars) {
        taskMgr.SetTask(new CTaskComplexEnterCarAsDriver(targetVeh), TASK_PRIMARY_PRIMARY, false);
        return;
    }

    // The other player decides if the vehicle is entered as driver or as passenger
    auto&       otherInfo   = CWorld::Players[(playerIndex + 1) % 2];
    auto* const otherPed    = otherInfo.m_pPed;
    auto&       otherTasks  = otherPed->GetIntelligence()->GetTaskManager();
    auto* const primaryTask = otherTasks.GetTaskPrimary(TASK_PRIMARY_PRIMARY);
    auto* const defaultTask = otherTasks.GetTaskPrimary(TASK_PRIMARY_DEFAULT);

    const auto primaryType = primaryTask ? primaryTask->GetTaskType() : (eTaskType)0;
    const auto defaultType = defaultTask ? defaultTask->GetTaskType() : (eTaskType)0;

    // Vehicle that the task refers to, it is stored in the same place for all of them
    const auto TaskVehicle = [](CTask* task) { return *reinterpret_cast<CVehicle**>(reinterpret_cast<uint8*>(task) + 0xC); };

    CVehicle* drivenVeh{};
    CVehicle* otherVeh{};
    switch (primaryType) {
    case TASK_COMPLEX_ENTER_CAR_AS_DRIVER:
    case TASK_COMPLEX_STEAL_CAR:
    case TASK_COMPLEX_DRAG_PED_FROM_CAR:
    case TASK_COMPLEX_ENTER_BOAT_AS_DRIVER:
        drivenVeh = TaskVehicle(primaryTask);
        break;
    default:
        break;
    }
    if (defaultType == TASK_SIMPLE_PLAYER_IN_CAR || primaryType == TASK_SIMPLE_PLAYER_IN_CAR || defaultType == TASK_SIMPLE_CAR_DRIVE || primaryType == TASK_SIMPLE_CAR_DRIVE) {
        otherVeh = otherPed->m_pVehicle;
        if (otherVeh->m_pDriver == otherPed) {
            drivenVeh = otherVeh;
        }
    }
    if (primaryType == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) {
        otherVeh = TaskVehicle(primaryTask);
    }

    if (drivenVeh == targetVeh) {
        taskMgr.SetTask(new CTaskComplexEnterCarAsPassenger(targetVeh, 0, false), TASK_PRIMARY_PRIMARY, false);
    } else if (otherVeh == targetVeh || (!drivenVeh && !otherVeh)) {
        taskMgr.SetTask(new CTaskComplexEnterCarAsDriver(targetVeh), TASK_PRIMARY_PRIMARY, false);
    }
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessRemoteVehicleExplosion() {
    if (!m_bAfterRemoteVehicleExplosion) {
        return;
    }

    const auto now     = CTimer::GetTimeInMS();
    const auto sinceMs = now - m_nTimeOfRemoteVehicleExplosion;
    const auto fades   = m_nPlayerState == PLAYERSTATE_PLAYING && m_bFadeAfterRemoteVehicleExplosion;

    if (CTimer::GetPreviousTimeInMS() - m_nTimeOfRemoteVehicleExplosion < 1000 && sinceMs >= 1000 && fades) {
        TheCamera.SetFadeColour(0, 0, 0);
        TheCamera.Fade(1.0f, eFadeFlag::FADE_IN);
    }
    if (sinceMs <= 2000) {
        return;
    }

    if (fades) {
        TheCamera.RestoreWithJumpCut();
        TheCamera.SetFadeColour(0, 0, 0);
        TheCamera.Fade(1.0f, eFadeFlag::FADE_OUT);
        TheCamera.Process();
        CTimer::Stop();
        CRenderer::RequestObjectsInFrustum(nullptr, 0);
        CStreaming::LoadAllRequestedModels(false);
        CTimer::Update();
    }
    m_bAfterRemoteVehicleExplosion = false;

    auto& focused = CWorld::Players[CWorld::PlayerInFocus];
    if (focused.m_pRemoteVehicle) {
        focused.m_pRemoteVehicle->m_bRemoveFromWorld = true;
    }
    focused.m_pRemoteVehicle = nullptr;

    if (auto* const focusedPed = focused.m_pPed; focusedPed && focusedPed->bInVehicle && focusedPed->m_pVehicle) {
        focusedPed->m_pVehicle->SetStatus(STATUS_PLAYER);
    }
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessUpsideDownVehicle() {
    if ((CTimer::GetFrameCounter() & 0x1F) != 0) {
        return;
    }

    auto* const focusedPed = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
    auto* const veh        = focusedPed && focusedPed->bInVehicle ? focusedPed->m_pVehicle : nullptr;

    // Counts how many times in a row the vehicle was found on its roof
    if (!veh || !m_pPed->bInVehicle) {
        m_nTimesUpsideDownInARow = 0;
    } else {
        const auto upZ = veh->GetMatrix().GetUp().z;
        if (upZ >= 0.0f
            || veh->GetMoveSpeed().Magnitude() >= 0.05f
            || (!veh->IsAutomobile() && !veh->IsBoat())
            || veh->physicalFlags.bSubmergedInWater
        ) {
            m_nTimesUpsideDownInARow = 0;
        } else {
            m_nTimesUpsideDownInARow += upZ >= -0.5f ? 1 : 2;
        }
    }

    if (m_nTimesUpsideDownInARow <= 6) {
        return;
    }
    if (veh->vehicleFlags.bCanBeDamaged) {
        veh->m_fHealth = std::min(veh->m_fHealth, 249.0f);
        if (veh->IsAutomobile()) {
            veh->AsAutomobile()->GetDamageManager().SetEngineStatus(225);
            veh->AsAutomobile()->m_pExplosionVictim = nullptr;
        }
    }
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessMovementStats() {
    auto* const focusedPed = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
    auto* const veh        = focusedPed && focusedPed->bInVehicle ? focusedPed->m_pVehicle : nullptr;

    if (!veh) {
        // On foot
        if (focusedPed->m_fMovingSpeed > 0.0f) {
            if (!m_pPed->GetIntelligence()->GetTaskSwim()) {
                if (!m_pPed->GetIntelligence()->GetTaskJetPack()) {
                    CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_ON_FOOT, focusedPed->m_fMovingSpeed);
                } else {
                    CStats::IncrementStat(STAT_TIME_ON_JETPACK, CTimer::GetTimeStep() * 16.0f);
                }
            } else if (CPad::GetPad(0)->GetPedWalkLeftRight() != 0 || CPad::GetPad(0)->GetAccelerate() != 0) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_SWIMMING, focusedPed->m_fMovingSpeed);
            }
        }
    } else {
        // In a vehicle
        const auto speed = veh->m_fMovingSpeed;
        if (veh->m_nModelIndex == MODEL_CADDY) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_GOLF_CART, speed);
        } else if (veh->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BICYCLE, speed);
        } else {
            switch (veh->GetVehicleAppearance()) {
            case VEHICLE_APPEARANCE_HELI:       CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_HELICOPTER, speed); break;
            case VEHICLE_APPEARANCE_PLANE:      CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_PLANE, speed); break;
            case VEHICLE_APPEARANCE_AUTOMOBILE: CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_CAR, speed); break;
            case VEHICLE_APPEARANCE_BIKE:       CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_MOTORBIKE, speed); break;
            case VEHICLE_APPEARANCE_BOAT:       CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BOAT, speed); break;
            default:                            break;
            }
            if ((veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE || veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI)
                && veh->GetMoveSpeed().Magnitude() > 0.2f
            ) {
                CStats::IncrementStat(STAT_FLIGHT_TIME, CTimer::GetTimeStep() * 16.0f);
            }
        }
    }

    // The stats that depend on the way the vehicle moves
    if (!FindPlayerTrain(-1) && veh && veh->GetMoveSpeed().SquaredMagnitude() > 0.0f) {
        switch (veh->GetVehicleAppearance()) {
        case VEHICLE_APPEARANCE_BIKE:
            if (veh->m_nVehicleSubType != VEHICLE_TYPE_BMX) {
                CStats::UpdateStatsWhenOnMotorBike(veh->AsBike());
            }
            break;
        case VEHICLE_APPEARANCE_HELI:
        case VEHICLE_APPEARANCE_PLANE:
            CStats::UpdateStatsWhenFlying(veh);
            break;
        case VEHICLE_APPEARANCE_BOAT:
            break;
        default:
            CStats::UpdateStatsWhenDriving(veh);
            break;
        }
    }
}

// NOTSA, part of 0x56F8D0
void CPlayerInfo::ProcessChaseValue() {
    static auto& isNearPathNode      = StaticRef<bool>(0x8CDF20);
    static auto& isFarFromLastCheck  = StaticRef<bool>(0x8CDF21);
    static auto& lastCheckedPos      = StaticRef<CVector>(0xB9B994);

    auto* const wanted = m_pPed->GetPlayerWanted();
    if (wanted->GetWantedLevel() == eWantedLevel::WANTED_CLEAN || CTheScripts::IsPlayerOnAMission()) {
        m_fCurrentChaseValue = 0.0f;
        return;
    }

    // Every 20 seconds it checks if the player moved and if it is close to the roads
    if (CTimer::GetTimeInMS() / 20000 != CTimer::GetPreviousTimeInMS() / 20000) {
        isFarFromLastCheck = (FindPlayerCoors(-1) - lastCheckedPos).Magnitude() >= 10.0f;
        lastCheckedPos     = FindPlayerCoors(-1);

        const auto node = ThePaths.FindNodeClosestToCoors(FindPlayerCoors(-1), PATH_TYPE_VEH, 60.0f, true, false, false, false, false);
        isNearPathNode  = node.m_wAreaId != (uint16)-1;
    }

    float target{};
    switch (wanted->GetWantedLevel()) {
    case eWantedLevel::WANTED_LEVEL_1: target = 31.0f; break;
    case eWantedLevel::WANTED_LEVEL_2: target = 62.0f; break;
    case eWantedLevel::WANTED_LEVEL_3: target = 125.0f; break;
    case eWantedLevel::WANTED_LEVEL_4: target = 250.0f; break;
    case eWantedLevel::WANTED_LEVEL_5: target = 500.0f; break;
    case eWantedLevel::WANTED_LEVEL_6: target = 1000.0f; break;
    default:             break;
    }

    const auto delta = (target - m_fCurrentChaseValue) * CTimer::GetTimeStep() * 0.0001f;
    if (delta < 0.0f
        || (isFarFromLastCheck && isNearPathNode && !CCullZones::NoPolice() && !CCullZones::PoliceAbandonCars() && CGame::currArea == AREA_CODE_NORMAL_WORLD)
    ) {
        m_fCurrentChaseValue += delta;
    }
}


// 0x56F4E0
void CPlayerInfo::FindClosestCarSectorList(CPtrListDoubleLink<CVehicle*>& ptrList, CPed* ped, float minX, float minY, float maxX, float maxY, float* outVehDist, CVehicle** outVehicle) {
    for (auto* node = ptrList.GetNode(), *next = decltype(node){}; node; node = next) {
        next = node->Next;

        auto* const vehicle = node->Item;
        if (vehicle->IsScanCodeCurrent() || !vehicle->GetUsesCollision() || vehicle->GetType() != ENTITY_TYPE_VEHICLE) {
            continue;
        }
        vehicle->SetCurrentScanCode();

        if (vehicle->GetStatus() == STATUS_WRECKED || vehicle->GetStatus() == STATUS_TRAIN_MOVING) {
            continue;
        }

        if (!vehicle->m_matrix) {
            vehicle->AllocateMatrix();
            vehicle->m_placement.UpdateMatrix(vehicle->m_matrix);
        }

        // Vehicles that are on their side can't be used, unless it is a bike
        if (vehicle->GetMatrix().GetUp().z <= 0.3f && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }

        const auto& vehPos = vehicle->GetPosition();
        const auto& pedPos = ped->GetPosition();
        const auto  roadPos = CVector{ vehPos.x, vehPos.y, vehPos.z - vehicle->GetHeightAboveRoad() + 1.0f };
        auto        heightDiff = pedPos.z - roadPos.z;

        // The AT400 is measured against its door instead
        CVector doorPos;
        int32   doorId      = 0;
        bool    closeEnough = false;
        if (vehicle->m_nModelIndex == MODEL_AT400 && CCarEnterExit::GetNearestCarDoor(ped, vehicle, doorPos, doorId)) {
            heightDiff  = std::abs(pedPos.z - doorPos.z);
            closeEnough = heightDiff < 1.0f;
        }
        if (!closeEnough && std::abs(heightDiff) >= 2.0f) {
            // Only boats can be used from this far, as long as the ped is standing on it or right above it
            if (vehicle->m_nVehicleType != VEHICLE_TYPE_BOAT) {
                continue;
            }
            const auto justAbove = roadPos.z < pedPos.z && pedPos.z - 4.0f < roadPos.z;
            if (!justAbove && ped->m_pContactEntity != vehicle) {
                continue;
            }
        }

        if (!vehicle->vehicleFlags.bConsideredByPlayer) {
            continue;
        }
        if (!CCarEnterExit::GetNearestCarDoor(ped, vehicle, doorPos, doorId = 0)) {
            continue;
        }

        auto dist = CVector{ pedPos.x - vehPos.x, pedPos.y - vehPos.y, 0.0f }.Magnitude();
        if (dist > 10.0f) {
            // Too far away, unless the ped is inside the vehicle's bounding box
            if (!CPedGeometryAnalyser::LiesInsideBoundingBox(*ped, pedPos, *vehicle)) {
                continue;
            }
            dist = 10.0f;
        }

        if (CCranes::IsThisCarBeingCarriedByAnyCrane(vehicle)) {
            continue;
        }
        EvaluateCarPosition(vehicle, ped, dist, outVehDist, outVehicle);
    }
}

// 0x56F330
void CPlayerInfo::Clear() {
    // TODO: This should just use the constructor and `swap`,
    // like to just swap ourselves to a default constructed object
    // and do all the cleanup in the destructor

    m_pPed = nullptr;
    m_pRemoteVehicle = nullptr;
    if (m_pSpecCar) {
        m_pSpecCar->physicalFlags.bAddMovingCollisionSpeed = false;
        m_pSpecCar = nullptr;
    }
    m_nDisplayMoney = 0;
    m_nMoney = 0;
    m_nPlayerState = PLAYERSTATE_PLAYING;
    m_nCarDensityForCurrentZone = 0;
    m_fRoadDensityAroundPlayer = 1.0f;
    m_bAfterRemoteVehicleExplosion = false;
    m_bCreateRemoteVehicleExplosion = false;
    m_bFadeAfterRemoteVehicleExplosion = false;
    m_bTryingToExitCar = false;
    m_bTaxiTimerScore = false;
    m_nTaxiTimer = 0;
    m_nVehicleTimeCounter = CTimer::GetTimeInMS();
    m_nMaxArmour = 100;
    m_nMaxHealth = 100;
    m_bCanDoDriveBy = true;
    m_nCollectablesPickedUp = 0;
    m_nTotalNumCollectables = 3;
    m_nLastTimeEnergyLost = 0;
    m_nLastTimeArmourLost = 0;
    m_nLastTimeBigGunFired = 0;
    m_nTimesStuckInARow = 0;
    m_nTimesUpsideDownInARow = 0;
    m_nCarTwoWheelCounter = 0;
    m_fCarTwoWheelDist = 0.0f;
    m_nCarLess3WheelCounter = 0;
    m_nBikeRearWheelCounter = 0;
    m_fBikeRearWheelDist = 0.0f;
    m_nBikeFrontWheelCounter = 0;
    m_fBikeFrontWheelDist = 0.0f;
    m_nTempBufferCounter = 0;
    m_nBestCarTwoWheelsTimeMs = 0;
    m_fBestCarTwoWheelsDistM = 0.0f;
    m_nBestBikeWheelieTimeMs = 0;
    m_fBestBikeWheelieDistM = 0.0f;
    m_nBestBikeStoppieTimeMs = 0;
    m_fBestBikeStoppieDistM = 0.0f;
    m_bDoesNotGetTired = false;
    m_bFastReload = false;
    m_bFireProof = false;
    m_bGetOutOfJailFree = false;
    m_bFreeHealthCare = false;
    m_nTimeOfLastCarExplosionCaused = 0;
    m_nExplosionMultiplier = 0;
    m_nHavocCaused = 0;
    FindPlayerInfo().m_nNumHoursDidntEat = 0;
    m_nLastBustMessageNumber = 1;
    m_fCurrentChaseValue = 0.0f;
    m_nBustedAudioStatus = 0;
    m_nCrosshairActivated = 0;

    m_nRequireParachuteTimer = 0;

    if (m_bParachuteReferenced) {
        CGameLogic::IsCoopGameGoingOn();
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
    }
}

// 0x56EC40
void CPlayerInfo::GivePlayerParachute() const {
    if (m_nRequireParachuteTimer) {
        if (CStreaming::IsModelLoaded(MODEL_GUN_PARA)) {
            m_pPed->GiveWeapon(WEAPON_PARACHUTE, 1, true);
            m_pPed->SetSavedWeapon(WEAPON_PARACHUTE);
        }
    }
}

// 0x56EB30
void CPlayerInfo::StreamParachuteWeapon(bool unk) {
    if (CGameLogic::IsCoopGameGoingOn()) {
        if (unk) {
            return;
        }
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
        return;
    }

    if (m_pPed && m_pPed->IsInVehicle()) {
        if (m_pPed->m_pVehicle->IsSubPlane() || m_pPed->m_pVehicle->IsSubHeli()) {
            if (m_nRequireParachuteTimer <= (uint32)CTimer::GetTimeStepInMS()) {
                const auto groundHeight = TheCamera.CalculateGroundHeight(eGroundHeightType::ENTITY_BB_BOTTOM);
                const auto vehToGroundZDist = m_pPed->m_pVehicle->GetPosition().z - groundHeight;
                m_nRequireParachuteTimer = (vehToGroundZDist <= 50.f) ? 0 : 5000;
            } else {
                m_nRequireParachuteTimer -= (uint32)CTimer::GetTimeStepInMS();
            }
        }
    }

    if (m_nRequireParachuteTimer) {
        CStreaming::RequestModel(MODEL_GUN_PARA, STREAMING_MISSION_REQUIRED);
        m_bParachuteReferenced = true;
        return;
    }

    if (m_bParachuteReferenced) {
        CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
        m_bParachuteReferenced = false;
        m_nRequireParachuteTimer = 0;
    }
}

// 0x56EAB0
void CPlayerInfo::AddHealth(int32 amount) const {
    const auto newValue = std::min((float)m_nMaxHealth, m_pPed->m_fHealth + (float)amount); // Clamp to m_nMaxHealth
    m_pPed->m_fHealth = std::max(newValue, m_pPed->m_fHealth); // Don't change health to a lower value
}

// 0x56EA30
void CPlayerInfo::BlowUpRCBuggy(bool bExplode) const {
    if (m_pRemoteVehicle && !m_pRemoteVehicle->m_bRemoveFromWorld) {
        CRemote::TakeRemoteControlledCarFromPlayer(bExplode);
        if (bExplode)
            m_pRemoteVehicle->BlowUpCar(FindPlayerPed(), false);
    }
}

// 0x56E870
void CPlayerInfo::MakePlayerSafe(bool enable, float radius) {
    // Not quite SA, but this is the way to do it (instead of copy pasting it twice)
    auto& flags = m_pPed->physicalFlags;
    flags.bInvulnerable = enable;
    flags.bBulletProof = enable;
    flags.bFireProof = enable;
    flags.bExplosionProof = enable;
    flags.bCollisionProof = enable;
    flags.bMeleeProof = enable;
    m_PlayerData.m_bCanBeDamaged = !enable;
    m_PlayerData.m_pWanted->m_bEverybodyBackOff = enable;
    m_pPed->GetPadFromPlayer()->bPlayerSafe = enable;
    CWorld::SetAllCarsCanBeDamaged(!enable);

    if (enable) {
        CWorld::StopAllLawEnforcersInTheirTracks();
        CPad::StopPadsShaking();

        m_pPed->ClearAdrenaline();
        m_PlayerData.m_fTimeCanRun = std::max(m_PlayerData.m_fTimeCanRun, 0.f);
        m_pPed->GetIntelligence()->ClearTasks(true, false);

        gFireManager.ExtinguishPoint(GetPos(), radius);
        CExplosion::RemoveAllExplosionsInArea(GetPos(), 4000.f);
        CProjectileInfo::RemoveAllProjectiles();
        CWorld::ExtinguishAllCarFiresInArea(GetPos(), radius);
        CReplay::DisableReplays();
        m_pPed->ClearWeaponTarget();
    } else {
        CReplay::EnableReplays();
    }
}

// 0x56E830
void CPlayerInfo::PlayerFailedCriticalMission() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_FAILED_MISSION;
        CGameLogic::SetMissionFailed();
        CDarkel::ResetOnPlayerDeath();
    }
}

// 0x56E610
void CPlayerInfo::WorkOutEnergyFromHunger() {

    static auto& s_lastTimeHungryStateProcessedInitialized = StaticRef<bool>(0xB9B8F4); // false
    static auto& s_lastTimeHungryStateProcessed = StaticRef<uint8>(0xB9B8F2);
    static auto& s_LastHungryState = StaticRef<int8>(0xB9B8F1);
    static auto& s_bHungryMessageShown = StaticRef<bool>(0xB9B8F0);

    if (CCheat::IsActive(CHEAT_NEVER_GET_HUNGRY)) {
        return;
    }

    if (!s_lastTimeHungryStateProcessedInitialized) {
        s_lastTimeHungryStateProcessedInitialized = true;
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }

    auto pad = CPad::GetPad();
    if (   pad->ArePlayerControlsDisabled()
        || CMenuSystem::num_menus_in_use
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_running
        || CGameLogic::IsCoopGameGoingOn()
        || m_pRemoteVehicle
    ) {
        return;
    }

    if (!m_pPed)
        return;

    if (m_pPed->m_pAttachedTo)
        return;

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        if (!m_nNumHoursDidntEat)
            s_LastHungryState = 0;
        m_nNumHoursDidntEat += 1;
    }

    if (m_nNumHoursDidntEat <= 48) {
        s_bHungryMessageShown = false;
    } else {
        if (CClock::GetGameClockHours() == s_lastTimeHungryStateProcessed)
            return;

        m_pPed->Say(CTX_GLOBAL_STOMACH_RUMBLE);
        pad->StartShake(400, 110u, 0);

        if (s_bHungryMessageShown) {
            bool bDecreaseHealth{};
            if (CStats::GetStatValue(STAT_FAT) > 0.0f) {
                CStats::DecrementStat(STAT_FAT, 25.0f);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, 25.0f);
                bDecreaseHealth = true;
                if (!s_LastHungryState) {
                    s_LastHungryState = m_nNumHoursDidntEat + 24;
                }
            }

            if (CStats::GetStatValue(STAT_MUSCLE) <= 0.0f || m_nNumHoursDidntEat <= s_LastHungryState && s_LastHungryState) {
                if (!bDecreaseHealth) {
                    m_pPed->m_fHealth -= 2.0f;
                }
            } else {
                CStats::DecrementStat(STAT_MUSCLE, 25.0);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, 25.0f);
            }
        } else {
            CHud::SetHelpMessage(TheText.Get("NOTEAT"), true, false, true);
            s_bHungryMessageShown = true;
        }
    }

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }
}

// 0x56E5D0
void CPlayerInfo::ArrestPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_BEEN_ARRESTED;
        m_nBustedAudioStatus = 0;
        CDarkel::ResetOnPlayerDeath();
        CStats::IncrementStat(STAT_TIMES_BUSTED, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E580
void CPlayerInfo::KillPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_DIED;
        CDarkel::ResetOnPlayerDeath();
        CMessages::AddBigMessage(TheText.Get("DEAD"), 4000, STYLE_WHITE_MIDDLE);
        CStats::IncrementStat(STAT_NUMBER_OF_HOSPITAL_VISITS, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E570
bool CPlayerInfo::IsRestartingAfterMissionFailed() const {
    return m_nPlayerState == PLAYERSTATE_FAILED_MISSION;
}

// 0x56E560
bool CPlayerInfo::IsRestartingAfterArrest() const {
    return m_nPlayerState == PLAYERSTATE_HAS_BEEN_ARRESTED;
}

// 0x56E550
bool CPlayerInfo::IsRestartingAfterDeath() const {
    return m_nPlayerState == PLAYERSTATE_HAS_DIED;
}

// 0x56DAB0
bool CPlayerInfo::IsPlayerInRemoteMode() const {
    return m_pRemoteVehicle || m_bAfterRemoteVehicleExplosion;
}

// 0x56DFB0
// Return occupied vehicle's (if in any) or player's ped position
CVector CPlayerInfo::GetPos() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetPosition() : m_pPed->GetPosition();
}

// 0x56DF50
// Return occupied vehicle's (if in any) or player's ped move speed
CVector CPlayerInfo::GetSpeed() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetMoveSpeed() : m_pPed->GetMoveSpeed();
}

// 0x5D3B00
bool CPlayerInfo::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer<int32>(); // Discarded
    auto data = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerInfoSaveStructure>();
    data.Extract(this);
    return true;
}

// 0x5D3AC0
bool CPlayerInfo::Save() {
    CPlayerInfoSaveStructure data;
    data.Construct(this);
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerInfoSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(data);
    return true;
}

// 0x45DEF0
CPlayerInfo& CPlayerInfo::operator=(const CPlayerInfo& rhs) {
    m_pPed                             = rhs.m_pPed;
    m_PlayerData                       = rhs.m_PlayerData;
    m_pRemoteVehicle                   = rhs.m_pRemoteVehicle;
    m_pSpecCar                         = rhs.m_pSpecCar;
    m_nMoney                           = rhs.m_nMoney;
    m_nDisplayMoney                    = rhs.m_nDisplayMoney;
    m_nCollectablesPickedUp            = rhs.m_nCollectablesPickedUp;
    m_nTotalNumCollectables            = rhs.m_nTotalNumCollectables;
    m_nLastBumpPlayerCarTimer          = rhs.m_nLastBumpPlayerCarTimer;
    m_nTaxiTimer                       = rhs.m_nTaxiTimer;
    m_nVehicleTimeCounter              = rhs.m_nVehicleTimeCounter;
    m_bTaxiTimerScore                  = rhs.m_bTaxiTimerScore;
    m_bTryingToExitCar                 = rhs.m_bTryingToExitCar;
    m_pLastTargetVehicle               = rhs.m_pLastTargetVehicle;
    m_nPlayerState                     = rhs.m_nPlayerState;
    m_bAfterRemoteVehicleExplosion     = rhs.m_bAfterRemoteVehicleExplosion;
    m_bCreateRemoteVehicleExplosion    = rhs.m_bCreateRemoteVehicleExplosion;
    m_bFadeAfterRemoteVehicleExplosion = rhs.m_bFadeAfterRemoteVehicleExplosion;
    m_nTimeOfRemoteVehicleExplosion    = rhs.m_nTimeOfRemoteVehicleExplosion;
    m_nLastTimeEnergyLost              = rhs.m_nLastTimeEnergyLost;
    m_nLastTimeArmourLost              = rhs.m_nLastTimeArmourLost;
    m_nLastTimeBigGunFired             = rhs.m_nLastTimeBigGunFired;
    m_nTimesUpsideDownInARow           = rhs.m_nTimesUpsideDownInARow;
    m_nTimesStuckInARow                = rhs.m_nTimesStuckInARow;
    m_nCarTwoWheelCounter              = rhs.m_nCarTwoWheelCounter;
    m_fCarTwoWheelDist                 = rhs.m_fCarTwoWheelDist;
    m_nCarLess3WheelCounter            = rhs.m_nCarLess3WheelCounter;
    m_nBikeRearWheelCounter            = rhs.m_nBikeRearWheelCounter;
    m_fBikeRearWheelDist               = rhs.m_fBikeRearWheelDist;
    m_nBikeFrontWheelCounter           = rhs.m_nBikeFrontWheelCounter;
    m_fBikeFrontWheelDist              = rhs.m_fBikeFrontWheelDist;
    m_nTempBufferCounter               = rhs.m_nTempBufferCounter;
    m_nBestCarTwoWheelsTimeMs          = rhs.m_nBestCarTwoWheelsTimeMs;
    m_fBestCarTwoWheelsDistM           = rhs.m_fBestCarTwoWheelsDistM;
    m_nBestBikeWheelieTimeMs           = rhs.m_nBestBikeWheelieTimeMs;
    m_fBestBikeWheelieDistM            = rhs.m_fBestBikeWheelieDistM;
    m_nBestBikeStoppieTimeMs           = rhs.m_nBestBikeStoppieTimeMs;
    m_fBestBikeStoppieDistM            = rhs.m_fBestBikeStoppieDistM;
    m_nCarDensityForCurrentZone        = rhs.m_nCarDensityForCurrentZone;
    m_fRoadDensityAroundPlayer         = rhs.m_fRoadDensityAroundPlayer;
    m_nTimeOfLastCarExplosionCaused    = rhs.m_nTimeOfLastCarExplosionCaused;
    m_nExplosionMultiplier             = rhs.m_nExplosionMultiplier;
    m_nHavocCaused                     = rhs.m_nHavocCaused;
    m_nNumHoursDidntEat                = rhs.m_nNumHoursDidntEat;
    m_fCurrentChaseValue               = rhs.m_fCurrentChaseValue;
    m_bDoesNotGetTired                 = rhs.m_bDoesNotGetTired;
    m_bFastReload                      = rhs.m_bFastReload;
    m_bFireProof                       = rhs.m_bFireProof;
    m_nMaxHealth                       = rhs.m_nMaxHealth;
    m_nMaxArmour                       = rhs.m_nMaxArmour;
    m_bGetOutOfJailFree                = rhs.m_bGetOutOfJailFree;
    m_bFreeHealthCare                  = rhs.m_bFreeHealthCare;
    m_bCanDoDriveBy                    = rhs.m_bCanDoDriveBy;
    m_nBustedAudioStatus               = rhs.m_nBustedAudioStatus;
    m_nLastBustMessageNumber           = rhs.m_nLastBustMessageNumber;
    m_nCrosshairActivated              = rhs.m_nCrosshairActivated;
    m_vecCrosshairTarget               = rhs.m_vecCrosshairTarget;
    m_pSkinTexture                     = rhs.m_pSkinTexture;
    m_bParachuteReferenced             = rhs.m_bParachuteReferenced;
    m_nRequireParachuteTimer           = rhs.m_nRequireParachuteTimer;
    strcpy_s(m_szSkinName, rhs.m_szSkinName);
    return *this;
}
