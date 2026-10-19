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
    RH_ScopedVMTInstall(ProcessPed, 0x644470, { .Reversed = false });
    RH_ScopedVMTInstall(SetPedPosition, 0x63C770);
}

// 0x63C340
CTaskSimpleCarDrive::CTaskSimpleCarDrive(CVehicle* vehicle, CTaskUtilityLineUpPedWithCar* utilityTask, bool updateCurrentVehicle) : CTaskSimple() {
    m_pVehicle = vehicle;
    m_pAnimCloseDoorRolling = nullptr;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_TaskTimer = nullptr;
    field_18 = 0;
    field_1C = 0;
    field_1D = 0;
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
    return plugin::CallMethodAndReturn<bool, 0x644470, CTaskSimpleCarDrive*, CPed*>(this, ped);
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

