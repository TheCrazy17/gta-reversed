#include "StdInc.h"

#include "TaskComplexProstituteSolicit.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexCarDrive.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "SeekEntity/TaskComplexSeekEntityXYOffset.h"
#include "Audio/Entities/AEPedSpeechAudioEntity.h"
#include "Entity/Ped/PlayerPed.h"
#include "CarEnterExit.h"
#include "Cheat.h"
#include "Stats.h"

void CTaskComplexProstituteSolicit::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexProstituteSolicit, 0x86FB88, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x661A60);
    RH_ScopedInstall(Destructor, 0x661AF0);
    RH_ScopedInstall(GetRidOfPlayerProstitute, 0x661D30);
    RH_ScopedInstall(IsTaskValid, 0x661BB0);
    RH_ScopedInstall(CreateSubTask, 0x666360);
    RH_ScopedVMTInstall(Clone, 0x6622F0);
    RH_ScopedVMTInstall(GetTaskType, 0x661AE0);
    RH_ScopedVMTInstall(MakeAbortable, 0x661B80);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6666A0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x666780);
    RH_ScopedVMTInstall(ControlSubTask, 0x6669D0);
}

// 0x661A60
CTaskComplexProstituteSolicit::CTaskComplexProstituteSolicit(CPed* client) : CTaskComplex() {
    m_nLastSavedTime = 0;
    m_nNextTimeToCheckForSecludedPlace = 0;
    m_nLastPaymentTime = 0;
    m_nCurrentTimer = 0;
    m_pClient = client;
    m_nVehicleMovementTimer = 850;

    // The exe clears the low bits of the flags word and then sets bits 6, 7 and 9
    bSearchingForSecludedPlace = false;
    bSexProcessStarted = false;
    bTaskCanBeFinished = false;
    bPedsCanPotentiallySeeThis = false;
    bPedsCanSeeThis = false;
    bCopsCanSeeThis = false;
    b07 = true;
    b08 = true;
    bMoveCameraDown = false;
    b10 = true;
    bPlayerHasAcceptedSexProposition = false;
    bVehicleShifted = false;
    bSecludedPlaceMessageShown = false;

    CEntity::SafeRegisterRef(m_pClient);
}

// 0x661AF0
CTaskComplexProstituteSolicit::~CTaskComplexProstituteSolicit() {
    auto player = FindPlayerPed();
    if (!player)
        return;

    CEntity::ClearReference(player->GetPlayerData()->m_pCurrentProstitutePed);
    if (bMoveCameraDown) {
        bMoveCameraDown = false;
    }
}

// 0x661B80
bool CTaskComplexProstituteSolicit::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    bool aborted = m_pSubTask->MakeAbortable(ped, priority, event);
    if (aborted) {
        bMoveCameraDown = false;
    }
    return aborted;
}

// 0x661D30
void CTaskComplexProstituteSolicit::GetRidOfPlayerProstitute() {
    auto* prostitute = FindPlayerPed()->GetPlayerData()->m_pCurrentProstitutePed;
    if (!prostitute)
        return;

    const auto intel = prostitute->GetIntelligence();
    auto* task = intel->FindTaskByType(TASK_COMPLEX_PROSTITUTE_SOLICIT);
    if (!task)
        return;

    auto* t = static_cast<CTaskComplexProstituteSolicit*>(task);
    t->bTaskCanBeFinished = true;
    t->m_nCurrentTimer = 0;
}

// 0x666360
CTask* CTaskComplexProstituteSolicit::CreateSubTask(eTaskType taskType, CPed* prostitute) {
    switch (taskType) {
    case TASK_COMPLEX_CAR_DRIVE:
        // The exe also clears the sex flag here
        bSearchingForSecludedPlace = true;
        bSexProcessStarted = false;
        return new CTaskComplexCarDrive(m_pClient->m_pVehicle);

    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill(5000, false, false, 8.0f);

    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger(m_pClient->m_pVehicle, 8, false);

    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar(m_pClient->m_pVehicle, 0, 0, true, false);

    case TASK_COMPLEX_SEEK_ENTITY: {
        // Walks to the front or the rear door of the car, whichever is closer to the prostitute; the target is kept
        // in the space of the car so it follows the car if it moves
        auto* const veh = m_pClient->m_pVehicle;
        const auto  invMat = Invert(*veh->m_matrix);

        const CVector frontDoor = CCarEnterExit::GetPositionToOpenCarDoor(veh, 10);
        const CVector rearDoor  = CCarEnterExit::GetPositionToOpenCarDoor(veh, 8);

        const CVector& prostitutePos = prostitute->GetPosition();
        const bool     bRearIsCloser = (prostitutePos - rearDoor).SquaredMagnitude() <= (prostitutePos - frontDoor).SquaredMagnitude();

        return new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
            veh,
            50'000,
            1000,
            1.0f,
            2.0f,
            2.0f,
            false,
            false,
            CEntitySeekPosCalculatorXYOffset{ invMat.TransformPoint(bRearIsCloser ? rearDoor : frontDoor) }
        };
    }

    case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
        return new CTaskComplexTurnToFaceEntityOrCoord(m_pClient, 0.5f, 0.2f);

    default:
        return nullptr;
    }
}

// 0x661BB0
bool CTaskComplexProstituteSolicit::IsTaskValid(CPed* prostitute, CPed* ped) {
    if (FindPlayerPed() != ped)
        return false;

    if (!ped)
        return false;

    if (!ped->IsInVehicle())
        return false;

    if (ped->bIsBeingArrested)
        return false;

    if (ped->GetPlayerData()->m_pCurrentProstitutePed && ped->GetPlayerData()->m_pCurrentProstitutePed != prostitute)
        return false;

    if (ped->m_pVehicle->GetVehicleAppearance() != VEHICLE_APPEARANCE_AUTOMOBILE)
        return false;

    if (ped->m_pVehicle->IsUpsideDown())
        return false;

    if (ped->m_pVehicle->IsOnItsSide())
        return false;

    auto task = ped->GetTaskManager().GetSimplestActiveTask();
    if (task->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (ped->m_pVehicle->m_pDriver != ped)
        return false;

    if (prostitute->m_pVehicle) {
        if (prostitute->m_pVehicle != ped->m_pVehicle || prostitute->m_pVehicle->m_nNumPassengers != 1)
            return false;
    } else if (ped->m_pVehicle->m_nNumPassengers) {
        return false;
    }

    if (!ped->m_pVehicle->m_nMaxPassengers || ped->m_pVehicle->m_pHandlingData->m_bTandemSeats)
        return false;

    CVector out = ped->GetPosition() - prostitute->GetPosition();
    if (out.SquaredMagnitude() > 100.0f || CTheScripts::IsPlayerOnAMission() || CGameLogic::IsCoopGameGoingOn()) {
        return false;
    }
    return true;
}

// 0x6666A0
CTask* CTaskComplexProstituteSolicit::CreateFirstSubTask(CPed* ped) {
    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
        return nullptr;
    }

    m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();

    // The prostitute is stored first and registered through the player's data afterwards, like the exe does
    m_pClient->GetPlayerData()->m_pCurrentProstitutePed = ped;
    CEntity::SafeRegisterRef(FindPlayerPed()->GetPlayerData()->m_pCurrentProstitutePed);

    auto* const clientData = m_pClient->GetPlayerData();
    if (clientData->m_pLastProstituteShagged != ped) {
        CEntity::SafeCleanUpRef(clientData->m_pLastProstituteShagged);
        clientData->m_pLastProstituteShagged = ped;
        CEntity::SafeRegisterRef(clientData->m_pLastProstituteShagged);
    }

    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
}

// 0x666780
CTask* CTaskComplexProstituteSolicit::CreateNextSubTask(CPed* ped) {
    if (!m_pClient) {
        return nullptr;
    }

    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_CAR_DRIVE:
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);

    case TASK_SIMPLE_STAND_STILL: {
        if (!bPlayerHasAcceptedSexProposition) {
            return CreateSubTask(TASK_FINISHED, ped);
        }

        if (CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU)) {
            return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);
        }

        auto* const player = FindPlayerPed();
        if (player->GetPlayerInfoForThisPlayerPed()->m_nMoney >= 20) {
            return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);
        }
        CMessages::ClearMessages(false);
        CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true); // You've got money right?
        CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true); // Stop wasting my time!
        return CreateSubTask(TASK_FINISHED, ped);
    }

    case TASK_COMPLEX_LEAVE_CAR:
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 2500, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_FINISHED, ped);

    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        ped->Say(CTX_GLOBAL_SOLICIT_THANKS);
        m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();
        return CreateSubTask(TASK_COMPLEX_CAR_DRIVE, ped);

    case TASK_COMPLEX_SEEK_ENTITY:
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 5000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_COMPLEX_TURN_TO_FACE_ENTITY, ped);

    case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
        ped->Say(CTX_GLOBAL_SOLICIT);
        CMessages::AddMessageQ(TheText.Get("PROS_04"), 5000, 1, true); // You want a good time, honey?
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);

    default:
        return nullptr;
    }
}

// 0x6669D0
CTask* CTaskComplexProstituteSolicit::ControlSubTask(CPed* ped) {
    // The camera moves down while the sex is going on
    bMoveCameraDown = bSexProcessStarted;

    if (!IsTaskValid(ped, m_pClient)) {
        bMoveCameraDown = false;
        bTaskCanBeFinished = true;
    }

    // Milliseconds of this frame; the 16 bit timers below wrap like the exe's word arithmetic
    const auto timeStepMS = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

    if (bTaskCanBeFinished) {
        if (m_nCurrentTimer) {
            m_nCurrentTimer = (int16)(m_nCurrentTimer - timeStepMS);
            if (m_nCurrentTimer > 0) {
                return m_pSubTask;
            }
            CAEPedSpeechAudioEntity::SetCJMood(MOOD_WR, 120'000, -1, -1, -1);
            m_pClient->Say(CTX_GLOBAL_AFTER_SEX);
            m_nCurrentTimer = 0;
            return m_pSubTask;
        }

        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_LEAVE_CAR) {
            return m_pSubTask;
        }
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
        }
        return m_pSubTask;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();

    if (subTaskType == TASK_SIMPLE_STAND_STILL) {
        auto& pad = CPad::Pads[0];
        if (pad.ConversationYesJustDown()) {
            bPlayerHasAcceptedSexProposition = true;
            if (m_pClient) {
                m_pClient->Say(CTX_GLOBAL_SOLICIT_PRO_YES);
            }
            return CreateNextSubTask(ped);
        }
        if (pad.ConversationNoJustDown()) {
            if (m_pClient) {
                m_pClient->Say(CTX_GLOBAL_SOLICIT_PRO_NO);
            }
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    }

    if (subTaskType == TASK_COMPLEX_SEEK_ENTITY || subTaskType == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) {
        // Gives up when the car is driven away from where the proposal started
        if ((m_pClient->m_pVehicle->GetPosition() - m_vecVehiclePosn).SquaredMagnitude() > 16.0f) {
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    }

    if (subTaskType != TASK_COMPLEX_CAR_DRIVE) {
        return m_pSubTask;
    }

    const auto now = CTimer::GetTimeInMS();

    // TheCamera + 0xB0; CCam::Process_FollowPed_SA reads it as the ped zoom, but CCamera's layout in the repo
    // does not line up at this offset, so it is read by address
    static auto& s_CamZoom = StaticRef<int32>(0xB6F0D8);
    if (s_CamZoom == 1) {
        m_nLastSavedTime = now;
        m_nCurrentTimer = 8000;
        bMoveCameraDown = false;
        b08 = true;
        bVehicleShifted = true;
        return m_pSubTask;
    }

    // The car counts as stopped below a squared speed (scaled by 50) of 0.5625
    const bool bCarStopped = (m_pClient->m_pVehicle->m_vecMoveSpeed * 50.0f).SquaredMagnitude() < 0.5625f;
    if (!bCarStopped || b08) {
        b08 = false;
        m_nLastSavedTime = now;
    }

    auto* const wanted = FindPlayerWanted();

    if (now > (uint32)m_nNextTimeToCheckForSecludedPlace) {
        bPedsCanPotentiallySeeThis = false;
        bPedsCanSeeThis = false;
        bCopsCanSeeThis = false;
        m_nNextTimeToCheckForSecludedPlace = now + 1000;

        // The peds around the prostitute are the ones that could see the car
        auto* const nearbyPeds = ped->GetIntelligence()->GetPedEntities();
        for (size_t i = 0; i < MAX_NUM_ENTITIES; i++) {
            auto* const other = static_cast<CPed*>(nearbyPeds[i]);
            if (!other || other == m_pClient || other->m_nPedType == PED_TYPE_PROSTITUTE) {
                continue;
            }

            const float distSq = (other->GetPosition() - m_pClient->GetPosition()).SquaredMagnitude();
            if (distSq < 56.25f) {
                bPedsCanSeeThis = true;
            }
            if (distSq < 400.0f) {
                bPedsCanPotentiallySeeThis = true;
            }

            if (other->m_nPedType == PED_TYPE_COP && bSexProcessStarted && wanted && (int32)wanted->m_WantedLevel < 1) {
                // The player's own car must not block the line of sight
                auto* const veh = m_pClient->m_pVehicle;
                const bool  bHadCollision = veh->m_bUsesCollision;
                veh->m_bUsesCollision = false;
                const bool bClear = CWorld::GetIsLineOfSightClear(other->GetPosition(), m_pClient->GetPosition(), true, true, false, true, false, true, false);
                veh->m_bUsesCollision = bHadCollision;
                if (bClear) {
                    bCopsCanSeeThis = true;
                }
            }
        }
    }

    if (bSearchingForSecludedPlace) {
        if (!bCarStopped) {
            return m_pSubTask;
        }
        if (now - (uint32)m_nLastSavedTime <= 4000) {
            return m_pSubTask;
        }

        if (bPedsCanPotentiallySeeThis) {
            // Peds could see the car; tell the player once and keep waiting
            if (bSecludedPlaceMessageShown) {
                return m_pSubTask;
            }
            CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
            bSecludedPlaceMessageShown = true;
            return m_pSubTask;
        }

        bSearchingForSecludedPlace = false;
        bSexProcessStarted = true;
        m_nLastPaymentTime = now;
        CMessages::AddMessageQ(TheText.Get("PROS_02"), 2000, 1, true);
        return m_pSubTask;
    }

    if (!bSexProcessStarted) {
        return m_pSubTask;
    }

    if (b07) {
        b07 = false;
        m_nCurrentTimer = 15'000;
        CStats::IncrementStat(STAT_NUMBER_OF_PROSTITUTES_VISITED, 1.0f);
    }

    auto&      pad = CPad::Pads[0];
    const bool bAccelOrBrake = pad.GetAccelerate() || pad.GetBrake();
    const auto rnd = (uint32)CGeneral::GetRandomNumber();

    bool bWantedRaised = false;
    if (bCopsCanSeeThis && wanted && (int32)wanted->m_WantedLevel < 1) {
        FindPlayerWanted()->SetWantedLevel(static_cast<eWantedLevel>(1));
        bWantedRaised = true;
    }

    if (bAccelOrBrake || bPedsCanSeeThis || bWantedRaised) {
        // The car moved, someone saw it or the cops did; back to looking for a place
        m_nLastSavedTime = now;
        bSexProcessStarted = false;
        bSearchingForSecludedPlace = true;
        bVehicleShifted = true;

        if (bAccelOrBrake) {
            if (rnd >= 0x1FFFFFFF && m_nCurrentTimer >= 3000) {
                m_nCurrentTimer = 8000;
                return m_pSubTask;
            }
            bTaskCanBeFinished = true;
            m_nCurrentTimer = 0;
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true);
            return m_pSubTask;
        }

        if (m_nCurrentTimer < 3000) {
            bTaskCanBeFinished = true;
            m_nCurrentTimer = 0;
            return m_pSubTask;
        }
        CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
        m_nCurrentTimer = 8000;
        return m_pSubTask;
    }

    // The car bounces while the sex is going on, faster at the start and calmer at the end
    m_nVehicleMovementTimer = (int16)(m_nVehicleMovementTimer - timeStepMS);
    if (m_nVehicleMovementTimer <= 0) {
        float forceFactor = CGeneral::GetRandomNumberInRange(-0.5f, -0.9f);
        if (m_nCurrentTimer > 10'000) {
            m_nVehicleMovementTimer = 850;
        } else if (m_nCurrentTimer > 5000) {
            m_nVehicleMovementTimer = 450;
        } else if (m_nCurrentTimer > 1000) {
            m_nVehicleMovementTimer = 120;
        } else {
            forceFactor *= 0.5f;
            m_nVehicleMovementTimer = 850;
            CPad::GetPad(0)->StartShake(1000, 120, 0);
        }

        auto* const veh = m_pClient->m_pVehicle;
        const float force = std::max(std::min(150.0f, veh->m_fMass * 0.06666667f), m_pClient->m_fMass);

        // The force is applied at the player or at the prostitute, relative to the car
        const CVector& pointSource = (rnd & 1) ? m_pClient->GetPosition() : ped->GetPosition();
        const CVector  offset = pointSource - veh->GetPosition();
        veh->ApplyTurnForce(CVector{ 0.0f, 0.0f, force * forceFactor }, CVector{ offset.x, offset.y, 0.0f });
        veh->m_vehicleAudio.AddAudioEvent(AE_SUSPENSION_BOUNCE, 0.0f);

        // rand() never exceeds 32767, so this speech never plays in the exe; kept to match it
        if (b10 && rnd > 0x0FFFFFFF) {
            ped->Say((rnd & 0xFFFF) < 0xFF ? CTX_GLOBAL_GIVING_HEAD : CTX_GLOBAL_HAVING_SEX, 0, 0.5f);
        }
    }

    m_nCurrentTimer = (int16)(m_nCurrentTimer - timeStepMS);
    if (m_nCurrentTimer <= 0) {
        m_nCurrentTimer = 3000;
        bSexProcessStarted = false;
        bTaskCanBeFinished = true;
    }

    // Every second the player pays
    if (now - (uint32)m_nLastPaymentTime <= 1000) {
        return m_pSubTask;
    }
    m_nLastPaymentTime = now;

    auto* const playerInfo = static_cast<CPlayerPed*>(m_pClient)->GetPlayerInfoForThisPlayerPed();
    if (CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU)) {
        playerInfo->m_nMoney += 2;
    } else if (playerInfo->m_nMoney >= 2) {
        playerInfo->m_nMoney -= 2;
        CStats::IncrementStat(STAT_PROSTITUTE_BUDGET, 2.0f);
        ped->m_nMoneyCount++;
    } else {
        // Out of money
        playerInfo->m_nMoney = 0;
        m_nCurrentTimer = 0;
        bSexProcessStarted = false;
        bTaskCanBeFinished = true;
        CMessages::ClearMessages(false);
        CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true);
        CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true);
    }

    if (!bVehicleShifted) {
        playerInfo->AddHealth(2);
    }
    return m_pSubTask;
}
