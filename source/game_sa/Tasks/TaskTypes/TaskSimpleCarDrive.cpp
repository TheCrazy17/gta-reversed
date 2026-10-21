#include "StdInc.h"
#include "TaskSimpleCarDrive.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskSimpleCarSetPedOut.h"
#include "CarEnterExit.h"
#include "VehicleAnimGroupData.h"
#include "Weather.h"
#include "Entity/Vehicle/Automobile.h"
#include <numbers>
#include "TaskSimpleGangDriveBy.h"
#include "TaskComplexSequence.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexDriveWander.h"
#include "TaskSimpleHoldEntity.h"
#include "Events/EventCarUpsideDown.h"
#include "Events/EventCopCarBeingStolen.h"
#include "Events/EventScriptCommand.h"
#include "PedGroups.h"
#include "Entity/Vehicle/Train.h"
#include "Entity/Ped/PlayerPed.h"
#include "Scripts/TheScripts.h"
#include "Audio/AudioEngine.h"
#include "ModelIndices.h"
#include "RwHelper.h"
#include "GameLogic.h"

// Unknown flag, the driver of a vehicle can only shoot from it when it is set
static auto& s_unkDriveByFlag = StaticRef<bool>(0x969179);

void CTaskSimpleCarDrive::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarDrive, 0x86E904, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x63C340);
    RH_ScopedInstall(Destructor, 0x63C460);

    RH_ScopedInstall(TriggerIK, 0x63C500);
    RH_ScopedInstall(UpdateBopping, 0x63C900);
    RH_ScopedInstall(StartBopping, 0x642760);
    RH_ScopedInstall(ProcessHeadBopping, 0x6428C0);
    RH_ScopedInstall(ProcessArmBopping, 0x642AE0);
    RH_ScopedInstall(ProcessBopping, 0x642E70);
    RH_ScopedVMTInstall(Clone, 0x63DC20);
    RH_ScopedVMTInstall(GetTaskType, 0x63C450);
    RH_ScopedVMTInstall(MakeAbortable, 0x63C670);
    RH_ScopedVMTInstall(ProcessPed, 0x644470);
    RH_ScopedVMTInstall(SetPedPosition, 0x63C770);
}

// 0x63C340
CTaskSimpleCarDrive::CTaskSimpleCarDrive(CVehicle* vehicle, CTaskUtilityLineUpPedWithCar* utilityTask, bool updateCurrentVehicle) : CTaskSimple() {
    m_pVehicle = vehicle;
    m_pAnimCloseDoorRolling = nullptr;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_nTimePassedSinceCarUpSideDown = 0;

    m_bUpdateCurrentVehicle = updateCurrentVehicle;
    m_b08 = true;

    CEntity::SafeRegisterRef(m_pVehicle);

    if (utilityTask) {
        m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar(CVector{}, 0, utilityTask->m_nDoorOpenPosType, utilityTask->m_nDoorIdx);
    }

    m_fHeadBoppingFactor = 0.0f;
    m_fHeadBoppingOrientation = 0.0f;
    m_fRandomHeadBoppingMultiplier = 0.0f;
    m_nBoppingStartTime = -1;
}

// 0x63C460
CTaskSimpleCarDrive::~CTaskSimpleCarDrive() {
    CEntity::SafeCleanUpRef(m_pVehicle);

    if (m_pTaskUtilityLineUpPedWithCar) {
        delete m_pTaskUtilityLineUpPedWithCar;
        m_pTaskUtilityLineUpPedWithCar = nullptr;
    }

    if (m_b20 && m_pAnimCloseDoorRolling) {
        // TODO: FIX ME: Keeps triggering, annoying as fuck
        // Seemingly happens when getting of a motorbike (like cops getting off)
        //assert(m_pAnimCloseDoorRolling);
        m_pAnimCloseDoorRolling->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        if (m_pVehicle) {
            m_pVehicle->ClearGettingOutFlags(1);
        }
    }
}

// 0x63C500
void CTaskSimpleCarDrive::TriggerIK(CPed* ped) const {
    if (!m_pVehicle) {
        return;
    }

    // Made an early-out of this, as this same code is used in both possible cases.
    if (g_ikChainMan.IsLooking(ped) || CGeneral::GetRandomNumberInRange(0, 100) >= 5) {
        return;
    }

    switch (m_pVehicle->m_autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP: { // Make ped look at player ped
        g_ikChainMan.LookAt("DriveCar", ped, FindPlayerPed(0), 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE: {
        if (const auto vehTargetCar = m_pVehicle->m_autoPilot.m_TargetEntity) {
            if (vehTargetCar->GetIsTypeVehicle()) {
                if (const auto driver = vehTargetCar->m_pDriver) { // Make ped look at target car or it's driver (if any)
                    g_ikChainMan.LookAt("DriveCar", ped, driver, 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
                } else {
                    g_ikChainMan.LookAt("DriveCar", ped, vehTargetCar, 3000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
                }
            }
        }
        break;
    }
    }
}

// 0x63C900
void CTaskSimpleCarDrive::UpdateBopping() {
    const auto timeDelta = (int32)CTimer::GetTimeInMS() - m_nBoppingStartTime;
    m_fBoppingProgress = (float)(timeDelta % m_nBoppingEndTime) / (float)m_nBoppingEndTime;
    m_nBoppingCompletedTimes = timeDelta / m_nBoppingEndTime % 2;
}

// 0x642760
void CTaskSimpleCarDrive::StartBopping(CPed* ped) {
    CTaskSimpleCarDrive* found = nullptr;

    const auto tryOccupant = [&](CPed* occupant) {
        if (occupant && occupant != ped) {
            if (auto* task = static_cast<CTaskSimpleCarDrive*>(occupant->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_DRIVE))) {
                found = task;
            }
        }
    };

    tryOccupant(ped->m_pVehicle->m_pDriver);
    for (auto i = 0; (!found || found->m_nBoppingStartTime == -1) && i < 3; i++) {
        tryOccupant(ped->m_pVehicle->m_apPassengers[i]);
    }

    if (!found || found->m_nBoppingStartTime == -1) {
        m_nBoppingStartTime = CTimer::GetTimeInMS();

        const auto r = static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32768.0f);
        const auto y = 60 - std::lround(-60.0f * r);
        m_nBoppingEndTime = std::lround(1000.0f / (static_cast<float>(y) * (1.0f / 60.0f)));

        m_fBoppingProgress = 0.0f;
        return;
    }

    m_nBoppingStartTime = found->m_nBoppingStartTime;
    m_nBoppingEndTime   = found->m_nBoppingEndTime;
    UpdateBopping();
}

// 0x6428C0
void CTaskSimpleCarDrive::ProcessHeadBopping(CPed* ped, bool a3, float a4) {
    float wave = 0.0f;
    if (a3) {
        if (!m_b01) {
            // Random chance to start bopping the head
            if (m_fHeadBoppingFactor == 0.0f && CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
                if (m_nBoppingStartTime == -1) {
                    StartBopping(ped);
                    m_nHeadBoppingStartTime = m_nBoppingStartTime;
                } else {
                    m_nHeadBoppingStartTime = CTimer::GetTimeInMS();
                }
                m_nHeadBoppingDirection        = CGeneral::GetRandomNumberInRange(1, 3);
                m_fRandomHeadBoppingMultiplier = CGeneral::GetRandomNumberInRange(3.0f, 8.0f);
                m_fHeadBoppingFactor           = 0.0f;
                m_fHeadBoppingOrientation      = 0.0f;
                wave                           = 0.0f;
            } else {
                a3 = false;
            }
        } else {
            wave = std::sin(m_fBoppingProgress * std::numbers::pi_v<float>);
            // Keep going for at least 5 seconds, then there is a small chance to stop on every frame
            if (CTimer::GetTimeInMS() - (uint32)m_nHeadBoppingStartTime > 5000 && CGeneral::GetRandomNumberInRange(0, 1000) >= 996) {
                a3 = false;
            }
        }
    }
    m_b01 = a3;

    float delta;
    if (!m_b01) {
        if (m_fHeadBoppingFactor > 0.0f) {
            m_fHeadBoppingFactor -= 0.05f;
        }
        if (m_fHeadBoppingFactor < 0.0f) {
            m_fHeadBoppingFactor = 0.0f;
        }
        delta = (1.0f - m_fHeadBoppingFactor) * (wave - m_fHeadBoppingOrientation);
    } else {
        if (m_fHeadBoppingFactor < 1.0f) {
            m_fHeadBoppingFactor += 0.05f;
        }
        if (m_fHeadBoppingFactor > 1.0f) {
            m_fHeadBoppingFactor = 1.0f;
        }
        delta = (wave - m_fHeadBoppingOrientation) * m_fHeadBoppingFactor;
    }
    m_fHeadBoppingOrientation += delta;

    if (a4 < 64.0f && m_fHeadBoppingOrientation > 0.0f) {
        auto angle = m_fRandomHeadBoppingMultiplier * m_fHeadBoppingOrientation;
        const auto frame = ped->m_apBones[PED_NODE_HEAD]->Frame;
        if (m_nHeadBoppingDirection > 0) {
            RwV3d axisX{ 1.0f, 0.0f, 0.0f };
            RwFrameRotate(frame, &axisX, m_nBoppingCompletedTimes != 0 ? -angle : angle, rwCOMBINEPRECONCAT);
        }
        if (m_nHeadBoppingDirection != 2) {
            angle = -angle;
        }
        RwV3d axisZ{ 0.0f, 0.0f, 1.0f };
        RwFrameRotate(frame, &axisZ, angle, rwCOMBINEPRECONCAT);
        ped->bUpdateMatricesRequired = true;
    }
}

// 0x642AE0
void CTaskSimpleCarDrive::ProcessArmBopping(CPed* ped, bool a3, float a4) {
    if (ped->IsPlayer()) {
        return;
    }

    auto* const veh = ped->m_pVehicle;
    if (CWeather::Rain <= 0.0f) {
        if (veh->m_pDriver == ped) {
            veh->SetWindowOpenFlag(CAR_DOOR_LF);
        } else if (veh->m_apPassengers[0] == ped) {
            veh->SetWindowOpenFlag(CAR_DOOR_RF);
        } else if (veh->m_apPassengers[1] == ped) {
            veh->SetWindowOpenFlag(CAR_DOOR_LR);
        } else if (veh->m_apPassengers[2] == ped) {
            veh->SetWindowOpenFlag(CAR_DOOR_RR);
        }
    } else {
        // Windows get closed while it rains
        veh->ClearWindowOpenFlag(CAR_DOOR_LF);
        veh->ClearWindowOpenFlag(CAR_DOOR_RF);
        veh->ClearWindowOpenFlag(CAR_DOOR_LR);
        veh->ClearWindowOpenFlag(CAR_DOOR_RR);
        a3 = false;
    }

    // The right side passengers use the other arm
    const auto animId = (AnimationId)(veh->m_apPassengers[0] == ped || veh->m_apPassengers[2] == ped ? 0xA3 : 0xA2);
    auto* const assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), animId);

    if (!a3) {
        if (assoc) {
            assoc->SetBlendDelta(-4.0f);
        }
        m_b02 = false;
        return;
    }

    if (!m_b02) {
        if (CGeneral::GetRandomNumberInRange(0, 1000) <= 995) {
            return;
        }

        bool canStart = true;
        if (m_pVehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
            const auto door = CCarEnterExit::ComputeTargetDoorToExit(m_pVehicle, ped);
            if (m_pVehicle->AsAutomobile()->m_damageManager.GetDoorStatus_Component((tComponent)door) != DAMSTATE_OK
                || CCarEnterExit::CarHasDoorToClose(m_pVehicle, door)
                || !m_pVehicle->CanPedLeanOut(ped)
            ) {
                canStart = false;
            }

            const auto& dummies = CModelInfo::GetModelInfo(m_pVehicle->m_nModelIndex)->AsVehicleModelInfoPtr()->m_pVehicleStruct->m_avDummyPos;
            const auto& handRest = dummies[DUMMY_HAND_REST];
            if (handRest.x == 0.0f && handRest.y == 0.0f && handRest.z == 0.0f) {
                return;
            }
            if (m_pVehicle->vehicleFlags.bLowVehicle) {
                return;
            }

            // The hand rest has to be within a certain height above the seat
            const auto heightAboveSeat = handRest.z - dummies[DUMMY_SEAT_FRONT].z;
            if (CVehicleAnimGroupData::GetVehicleAnimGroup(m_pVehicle->m_pHandlingData->m_nAnimGroup).m_animFlags.bCanEnterFrontDoors) {
                if (!(heightAboveSeat >= 0.40f) || heightAboveSeat > 0.44f) {
                    return;
                }
            } else {
                if (!(heightAboveSeat >= 0.39f) || heightAboveSeat > 0.46f) {
                    return;
                }
            }
            if (!canStart) {
                return;
            }
        }

        CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, animId, 4.0f);
        if (m_nBoppingStartTime == -1) {
            StartBopping(ped);
            m_nArmBoppingStartTime = m_nBoppingStartTime;
        } else {
            m_nArmBoppingStartTime = CTimer::GetTimeInMS();
        }
        m_b02 = true;
        return;
    }

    if (!assoc) {
        m_b02 = false;
        return;
    }

    if (a4 < 64.0f) {
        // Rotate the arm during the last quarter of the bopping cycle
        const auto angle = m_fBoppingProgress >= 0.75f ? (m_fBoppingProgress - 0.75f) * 45.0f : 0.0f;
        const auto node  = animId == 0xA2 ? PED_NODE_LEFT_HAND : PED_NODE_RIGHT_HAND;
        RwV3d axisZ{ 0.0f, 0.0f, 1.0f };
        RwFrameRotate(ped->m_apBones[node]->Frame, &axisZ, -angle, rwCOMBINEPRECONCAT);
        ped->bUpdateMatricesRequired = true;
    }

    if (CTimer::GetTimeInMS() - (uint32)m_nArmBoppingStartTime > 5000 && CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
        assoc->SetBlendDelta(-4.0f);
        m_b02 = false;
    }
}

// 0x642E70
void CTaskSimpleCarDrive::ProcessBopping(CPed* ped, bool a3) {
    if (ped->m_pVehicle->m_pDriver == FindPlayerPed(0)
        || ped->m_nPedType == PED_TYPE_COP
        || ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
    ) {
        return;
    }

    auto* vehicle = ped->m_pVehicle;
    if (vehicle->IsAutomobile() && !vehicle->IsSubQuad() && !ped->IsCreatedByMission()) {
        if (m_nBoppingStartTime != -1) { // IsBopping
            UpdateBopping();
        }

        const auto dist = DistanceBetweenPointsSquared(TheCamera.GetPosition(), ped->GetPosition());
        ProcessHeadBopping(ped, a3, dist);
        ProcessArmBopping(ped, a3, dist);
        if (m_nBoppingStartTime != -1 && !m_b01 && !m_b02) {
            m_nBoppingStartTime = -1;
        }
    }
}

// 0x63DC20


CTask* CTaskSimpleCarDrive::Clone() const {
    auto task = new CTaskSimpleCarDrive(m_pVehicle);
    task->m_bUpdateCurrentVehicle = m_bUpdateCurrentVehicle;
    return task;
}

// 0x63C670


bool CTaskSimpleCarDrive::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority != ABORT_PRIORITY_IMMEDIATE) {
        // Wait for the door closing anim to finish, then get out of the way
        m_b08 = true;
        return !m_b20;
    }

    if (ped->bInVehicle && ped->m_pVehicle) {
        const auto door = CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped);
        CTaskSimpleCarSetPedOut{ m_pVehicle, (eTargetDoor)door, false }.ProcessPed(ped);
    }
    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250);
    }
    return true;
}

// 0x644470
bool CTaskSimpleCarDrive::ProcessPed(CPed* ped) {
    ZoneScoped;

    const auto radioRetuned = AudioEngine.HasRadioRetuneJustStarted();

    // Follow the vehicle the ped is in now
    if (m_bUpdateCurrentVehicle && ped->bInVehicle && ped->m_pVehicle && ped->m_pVehicle != m_pVehicle) {
        CEntity::SafeCleanUpRef(m_pVehicle);
        m_pVehicle = ped->m_pVehicle;
        CEntity::SafeRegisterRef(m_pVehicle);
    }

    if (!m_pVehicle || !ped->bInVehicle) {
        ped->bInVehicle = false;
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        return true;
    }

    auto* const group = CPedGroups::GetPedsGroup(ped);

    // The vehicle being upside down for long enough is an event
    if (!CTheScripts::UpsideDownCars.IsCarUpsideDown(m_pVehicle)) {
        m_nTimePassedSinceCarUpSideDown = 0;
    } else {
        auto* const player = FindPlayerPed(-1);
        if (ped != player && player->bInVehicle && player->m_pVehicle == m_pVehicle) {
            ped->Say(CTX_GLOBAL_CAR_FLIPPED);
        }
        if (!group || group->GetMembership().IsLeader(ped)) {
            m_nTimePassedSinceCarUpSideDown += (int32)((double)CTimer::GetTimeStep() * (double)0.02f * (double)1000.0f);
            if ((uint32)m_nTimePassedSinceCarUpSideDown > 2000) {
                CEventCarUpsideDown event{ m_pVehicle };
                ped->GetIntelligence()->GetEventGroup().Add(&event, false);
            }
        }
    }

    // The cops know about their car being stolen
    if (ped->IsPlayer() && m_pVehicle->IsLawEnforcementVehicle() && (!m_copCarStolenTimer.m_bStarted || m_copCarStolenTimer.IsOutOfTime())) {
        m_copCarStolenTimer.Start(2000);
        CEventCopCarBeingStolen event{ ped, m_pVehicle };
        GetEventGlobalGroup()->Add(&event, false);
    }

    if (m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || m_pVehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD || m_pVehicle->m_nModelIndex == MODEL_TRACTOR) {
        ped->bTestForShotInVehicle = true;
    }

    // Can the ped shoot from the vehicle with the weapon it has
    const auto CanDriveBy = [ped, this] {
        const auto& weapon = ped->GetWeaponInSlot((size_t)4);
        return weapon.m_Type != WEAPON_UNARMED && weapon.m_TotalAmmo > 0 && m_pVehicle->CanPedLeanOut(ped);
    };
    const auto StartDriveBy = [ped](bool seatRHS) {
        auto* const task = new CTaskSimpleGangDriveBy{ nullptr, nullptr, 100.0f, 100, eDrivebyStyle::AI_ALL_DIRN, seatRHS };
        CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, task, false };
        ped->GetIntelligence()->GetEventGroup().Add(&event, false);
    };

    // Returns false when the whole thing is done for this frame
    const auto ProcessVehicleAnims = [&]() -> bool {
        if (ped->IsPlayer() && m_pVehicle->m_pDriver == ped) {
            auto* const pad = static_cast<CPlayerPed*>(ped)->GetPadFromPlayer();

            if (s_unkDriveByFlag && CanDriveBy()) {
                StartDriveBy(false);
            }

            const auto IsPadIdle = [pad] {
                return !pad || (pad->GetAccelerate() == 0 && pad->GetSteeringLeftRight() == 0 && pad->GetBrake() == 0);
            };

            // Trains open their doors when stopped
            auto* const pedVeh = ped->m_pVehicle;
            if (pedVeh->m_nModelIndex != MODEL_STREAKC && pedVeh->m_nVehicleType == VEHICLE_TYPE_TRAIN && !static_cast<CTrain*>(pedVeh)->m_aDoors[2].IsClosed()) {
                if (IsPadIdle()) {
                    const auto animGroup = CVehicleAnimGroupData::GetGroupForAnim((AssocGroupId)m_pVehicle->m_pHandlingData->m_nAnimGroup, ANIM_ID_CAR_ROLLDOOR);
                    pedVeh->ProcessOpenDoor(ped, 10, animGroup, ANIM_ID_CAR_ROLLDOOR, 1.0f);
                }
                return true;
            }

            // The driver's door is open, close it when nothing is going on
            if (pedVeh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && static_cast<CAutomobile*>(pedVeh)->m_damageManager.GetDoorStatus(DOOR_LEFT_FRONT) == DAMSTATE_OPENED) {
                if (pedVeh->IsDoorMissing(DOOR_LEFT_FRONT)) {
                    return true;
                }

                auto* const rollDoorAnim = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_ID_CAR_ROLLDOOR);
                if (!(pedVeh->m_nGettingOutFlags & 1) && !(pedVeh->m_nGettingInFlags & 1) && !rollDoorAnim) {
                    if (IsPadIdle()) {
                        pedVeh->SetGettingOutFlags(1);
                        m_b20 = true;
                        plugin::CallMethod<0x642700, CTaskSimpleCarDrive*, CPed*>(this, ped); // Starts the anim of the door rolling closed
                        return false;
                    }
                } else if (rollDoorAnim) {
                    const auto animGroup = CVehicleAnimGroupData::GetGroupForAnim((AssocGroupId)m_pVehicle->m_pHandlingData->m_nAnimGroup, rollDoorAnim->m_AnimId);
                    pedVeh->ProcessOpenDoor(ped, 10, animGroup, ANIM_ID_CAR_ROLLDOOR, rollDoorAnim->m_CurrentTime);
                }
                ProcessBopping(ped, false);
                return true;
            }

            m_pVehicle->ProcessDrivingAnims(ped, radioRetuned);
            ProcessBopping(ped, true);
            return true;
        }

        if (m_pVehicle->m_pDriver != ped) {
            if (!m_pVehicle->IsPassenger(ped)) {
                return true;
            }
            if (!m_b08 && !m_b10) {
                if (auto* const rideData = m_pVehicle->GetRideAnimData()) {
                    CAnimManager::BlendAnimation(ped->m_pRwClump, rideData->AnimGroup, ANIM_ID_BIKE_PASSENGER, 8.0f);
                }
                m_b10 = true;
            }
            ProcessBopping(ped, true);

            if (ped->IsPlayer() && CGameLogic::IsCoopGameGoingOn() && CanDriveBy() && TheCamera.GetActiveCam().m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
                StartDriveBy(m_pVehicle->m_apPassengers[1] != ped);
            }
            return true;
        }

        m_pVehicle->ProcessDrivingAnims(ped, radioRetuned);
        ProcessBopping(ped, true);
        return true;
    };
    if (!ProcessVehicleAnims()) {
        return false;
    }

    // Takes a sip of the cigarette
    if (auto* const held = ped->GetEntityThatThisPedIsHolding(); held && held->m_nModelIndex == MI_GANG_SMOKE && !g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 60) == 15) {
        if (auto* const holdTask = static_cast<CTaskSimpleHoldEntity*>(ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY))) {
            holdTask->PlayAnim(m_pVehicle->IsPassenger(ped) ? ANIM_ID_PASS_SMOKE_IN_CAR : ANIM_ID_SMOKE_IN_CAR, (AssocGroupId)0);
        }
    }

    if (!RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_ID_SMOKE_IN_CAR)) {
        TriggerIK(ped);
    } else {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        if (m_nBoppingStartTime != -1) {
            m_nBoppingStartTime = -1;
        }
    }

    // Peds that are alone in the passenger seat of a vehicle without a driver leave it after a while
    if (ped->IsCreatedBy(PED_GAME) && !CPedGroups::GetPedsGroup(ped)) {
        if (!m_pVehicle->m_pDriver && m_pVehicle->m_apPassengers[0] == ped) {
            m_TaskTimer.StartIfNotAlready(4000);
            if (m_TaskTimer.IsOutOfTime()) {
                auto* const sequence = new CTaskComplexSequence{};
                sequence->AddTask(new CTaskComplexLeaveCar{ m_pVehicle, 0, 0, true, false });
                sequence->AddTask(new CTaskComplexCarDriveWander{ m_pVehicle, DRIVING_STYLE_STOP_FOR_CARS, 10.0f });
                CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, sequence, false };
                ped->GetIntelligence()->GetEventGroup().Add(&event, false);
            }
        } else {
            m_TaskTimer.Stop();
        }
    }

    // The driver comments on the way the vehicle moves
    if ((((uint32)ped->m_nRandomSeed + CTimer::GetFrameCounter()) & 0x3FFF) == 0) {
        if (auto* const driver = m_pVehicle->m_pDriver; driver && driver != ped && ped->IsPlayer()) {
            const auto speed = m_pVehicle->m_vecMoveSpeed.Magnitude2D();
            if (speed > 0.7f) {
                ped->Say(CTX_GLOBAL_CAR_FAST);
            }
            if (m_pVehicle->m_vecMoveSpeed.Magnitude2D() < 0.1f) {
                ped->Say(CTX_GLOBAL_CAR_SLOW);
            }
            ped->Say(CTX_GLOBAL_CAR_SINGALONG);
        }
    }
    return false;
}

// 0x63C770

// 0x0
bool CTaskSimpleCarDrive::SetPedPosition(CPed* ped) {
    if (m_b08) {
        // Hand the door line up task over from the other car drive task, if there is one
        const auto other = notsa::dyn_cast_if_present<CTaskSimpleCarDrive>(ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_DEFAULT, TASK_SIMPLE_CAR_DRIVE));
        if (other && other != this && other->m_pTaskUtilityLineUpPedWithCar) {
            m_pTaskUtilityLineUpPedWithCar = other->m_pTaskUtilityLineUpPedWithCar;
            other->m_pTaskUtilityLineUpPedWithCar = nullptr;
        }
        m_b08 = false;
    }

    if (!m_pTaskUtilityLineUpPedWithCar) {
        if (m_pVehicle && ped->m_pVehicle == m_pVehicle && ped->bInVehicle) {
            ped->SetPedPositionInCar();
        }
        return true;
    }

    static constexpr AnimationId anims[]{
        ANIM_ID_CAR_GETIN_LHS_0, ANIM_ID_CAR_GETIN_RHS_0, ANIM_ID_CAR_GETIN_LHS_1, ANIM_ID_CAR_GETIN_RHS_1, ANIM_ID_CAR_GETIN_BIKE_FRONT,
        ANIM_ID_CAR_CLOSEDOOR_LHS_0, ANIM_ID_CAR_CLOSEDOOR_RHS_0, ANIM_ID_CAR_CLOSEDOOR_LHS_1, ANIM_ID_CAR_CLOSEDOOR_RHS_1,
        ANIM_ID_CAR_SHUFFLE_RHS_0, ANIM_ID_CAR_SHUFFLE_RHS_1, ANIM_ID_CAR_ROLLDOOR,
    };
    static_assert(ANIM_ID_CAR_GETIN_LHS_0 == 359 && ANIM_ID_CAR_GETIN_BIKE_FRONT == 363 && ANIM_ID_CAR_CLOSEDOOR_LHS_0 == 367 && ANIM_ID_CAR_SHUFFLE_RHS_1 == 372 && ANIM_ID_CAR_ROLLDOOR == 386);

    CAnimBlendAssociation* assoc{};
    for (const auto anim : anims) {
        if ((assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), anim))) {
            break;
        }
    }
    m_pTaskUtilityLineUpPedWithCar->ProcessPed(ped, m_pVehicle, assoc);

    delete m_pTaskUtilityLineUpPedWithCar;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    return true;
}

