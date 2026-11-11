#include "StdInc.h"

#include "EventScanner.h"

#include "PedIntelligence.h"
#include "PedStats.h"
#include "PedGeometryAnalyser.h"
#include "InterestingEvents.h"
#include "FireManager.h"
#include "ModelIndices.h"
#include "PedScriptedTaskRecord.h"
#include "TaskManager.h"
#include "TaskSimpleGoTo.h"
#include "TaskComplexKillPedOnFoot.h"
#include "EventAreaCodes.h"
#include "EventDamage.h"
#include "EventEditableResponse.h"
#include "EventEscalator.h"
#include "EventFireNearby.h"
#include "EventInAir.h"
#include "EventOnFire.h"
#include "EventPedEnteredMyVehicle.h"
#include "EventPotentialWalkIntoFire.h"
#include "EventSexyPed.h"

void CEventScanner::InjectHooks() {
    RH_ScopedClass(CEventScanner);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(ScanForEvents, 0x607E30);
}

void CPedAcquaintanceScanner::InjectHooks() {
    RH_ScopedClass(CPedAcquaintanceScanner);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(IsScanAllowed, 0x603A30);
    RH_ScopedInstall(ScanForPedAcquaintanceEvents, 0x607D80);
}

void CSexyPedScanner::InjectHooks() {
    RH_ScopedClass(CSexyPedScanner);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(ScanForSexyPedEvents, 0x603BF0);
}

void CNearbyFireScanner::InjectHooks() {
    RH_ScopedClass(CNearbyFireScanner);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(ScanForNearbyFireEvents, 0x603E70);
}

// 0x605300
CEventScanner::CEventScanner() {
    m_nNextScanTime = CTimer::GetTimeInMS() + CGeneral::GetRandomNumberInRange(3000u); // Originally should be -3000.0f (float value)
}

void CEventScanner::Clear() {
    m_attractorScanner.Clear();
}

// 0x607E30
void CEventScanner::ScanForEvents(CPed& ped) {
    if (CTimer::GetTimeInMS() <= m_nNextScanTime) {
        return;
    }

    auto* const intel = ped.GetIntelligence();

    m_vehiclePotentialCollisionScanner.ScanForVehiclePotentialCollisionEvents(ped, intel->GetVehicleEntities(), 16);
    ScanForPedPotentialCollisionEvents(ped, intel->GetPedScanner().GetClosestPedInRange());
    m_objectPotentialCollisionScanner.ScanForObjectPotentialCollisionEvents(ped);
    m_pedAcquaintanceScanner.ScanForPedAcquaintanceEvents(&ped, intel->GetPedEntities(), 16);
    m_attractorScanner.ScanForAttractors(ped);
    m_nearbyFireScanner.ScanForNearbyFireEvents(&ped);
    intel->m_mentalState.Process(ped);

    // The event is added every scan while the ped is in the air; the flag is only cleared again once the handler stopped working on it
    if (!ped.bIsStanding && (ped.bIsInTheAir || CPedGeometryAnalyser::IsInAir(ped))) {
        CEventInAir event{};
        intel->GetEventGroup().Add(&event, false);
    } else if (ped.bIsInTheAir) {
        const auto* const currEvent = intel->GetEventHandler().GetHistory().GetCurrentEvent();
        if (!currEvent || currEvent->GetEventType() != EVENT_IN_AIR) {
            ped.bIsInTheAir = false;
        }
    }

    if (const auto* const fire = ped.m_pFire) {
        CEventOnFire event{};

        // A burning ped that can not drop what it does is damaged by the flames right away instead of getting the event queued
        auto* const simplest = ped.IsPlayer() ? nullptr : intel->GetTaskManager().GetSimplestActiveTask();
        if (simplest && !simplest->MakeAbortable(&ped, ABORT_PRIORITY_URGENT, &event)) {
            CWeapon::GenerateDamageEvent(&ped, fire->GetEntityStartedFire(), WEAPON_FLAMETHROWER, 5, PED_PIECE_TORSO, 0);
        } else {
            intel->GetEventGroup().Add(&event, false);
        }
    }

    m_sexyPedScanner.ScanForSexyPedEvents(&ped, intel->GetPedEntities(), 16);

    // The exe tries the primary tasks in this order (CPedIntelligence::FindTaskByType does the same)
    const auto FindKillPedOnFootTask = [&]() -> CTaskComplexKillPedOnFoot* {
        for (const auto idx : { TASK_PRIMARY_DEFAULT, TASK_PRIMARY_PRIMARY, TASK_PRIMARY_EVENT_RESPONSE_TEMP, TASK_PRIMARY_EVENT_RESPONSE_NONTEMP }) {
            if (auto* const task = intel->GetTaskManager().FindTaskByType(idx, TASK_COMPLEX_KILL_PED_ON_FOOT)) {
                return static_cast<CTaskComplexKillPedOnFoot*>(task);
            }
        }
        return nullptr;
    };
    if (const auto* const killTask = FindKillPedOnFootTask()) {
        // Fighting a ped that stands on something in another area (an interior)
        auto* const target = killTask->m_target;
        if (target && target->m_pContactEntity && ped.m_pContactEntity && target->m_pContactEntity->GetAreaCode() != ped.m_pContactEntity->GetAreaCode()) {
            CEventAreaCodes event{ target };
            intel->GetEventGroup().Add(&event, false);
        }
    }

    if (const auto* const contact = ped.m_pContactEntity) {
        const auto modelId = contact->m_nModelIndex;
        if (modelId == ModelIndices::MI_ESCALATORSTEP || modelId == ModelIndices::MI_ESCALATORSTEP8) {
            CEventEscalator event{};
            intel->GetEventGroup().Add(&event, false);
        }
    }

    // The player stands on the boat this ped is sitting in
    if (ped.bInVehicle && ped.m_pVehicle && ped.m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT && !ped.IsPlayer()) {
        auto* const vehicle = ped.m_pVehicle;
        auto* const player  = FindPlayerPed();
        if (player->m_pContactEntity == vehicle) {
            CEventPedEnteredMyVehicle event{ player, vehicle, TARGET_DOOR_DRIVER };
            event.m_TaskId = TASK_COMPLEX_LEAVE_CAR_AND_FLEE;
            intel->GetEventGroup().Add(&event, false);
        }
    }

    // A ped with no health left that is not dead yet is hurt again, at most every 2 seconds, so it does not stay up forever
    if (ped.m_fHealth <= 0.0f && ped.IsAlive() && CTimer::GetTimeInMS() > m_sDeadPedWalkingTimer) {
        CEventDamage event{ nullptr, 0, WEAPON_UNIDENTIFIED, PED_PIECE_TORSO, 0, false, false };
        if (!intel->GetEventGroup().HasEventOfType(&event)) {
            CWeapon::GenerateDamageEvent(&ped, nullptr, WEAPON_FALL, 10, PED_PIECE_TORSO, 0);
            m_sDeadPedWalkingTimer = CTimer::GetTimeInMS() + 2000;
        }
    }
}

// 0x6053D0
void CEventScanner::ScanForEventsNow(const CPed& ped, bool bDontScan) {
    if (bDontScan)
        return;

    auto scanner = &m_vehiclePotentialCollisionScanner;
    if (scanner->m_timer.m_bStarted) { // todo: inlined?
        scanner->m_timer.m_nStartTime = CTimer::GetTimeInMS();
        scanner->m_timer.m_nInterval = -1;
        scanner->m_timer.m_bStarted = true;
    }
    scanner->ScanForVehiclePotentialCollisionEvents(ped, ped.GetIntelligence()->GetVehicleEntities(), 16);
}

// 0x606580
void CEventScanner::ScanForPedPotentialCollisionEvents(CPed& ped, CPed* closestPed) {
    // The function never reads the `this` the exe gives it (the address of its own argument slot), so none is passed here either
    plugin::CallMethod<0x606580, CEventScanner*, CPed&, CPed*>(nullptr, ped, closestPed);
}

// 0x603720
void CVehiclePotentialCollisionScanner::ScanForVehiclePotentialCollisionEvents(const CPed& ped, CEntity** entities, int32 count) {
    plugin::CallMethod<0x603720, CVehiclePotentialCollisionScanner*, const CPed&, CEntity**, int32>(this, ped, entities, count);
}

// 0x606890
void CObjectPotentialCollisionScanner::ScanForObjectPotentialCollisionEvents(CPed& ped) {
    plugin::CallMethod<0x606890, CObjectPotentialCollisionScanner*, CPed&>(this, ped);
}

// 0x603A30
bool CPedAcquaintanceScanner::IsScanAllowed(CPed* ped) {
    if (!ped->IsAlive()) {
        return false;
    }

    // Mission peds only scan when their script allows it, in the cases below
    auto isAllowed = !ped->IsCreatedBy(PED_MISSION) || m_bScanAllowedScriptPed;
    if (!isAllowed) {
        isAllowed = ped->bInVehicle && m_bScanAllowedInVehicle;
        if (CPedScriptedTaskRecord::GetStatus(ped) == eScriptedTaskStatus::TASK_ASSOCIATED && m_bScanAllowedScriptedTask) {
            isAllowed = true;
        }
    }
    if (!isAllowed) {
        return false;
    }

    // A ped already reacting to somebody it hates keeps scanning only while it still has peds to inform and respected friends in range
    auto* const intel     = ped->GetIntelligence();
    auto* const currEvent = intel->GetEventHandler().GetHistory().GetCurrentEvent();
    if (!currEvent || currEvent->GetEventType() != EVENT_ACQUAINTANCE_PED_HATE) {
        return true;
    }
    if (!intel->m_nDmNumPedsToScan) {
        return false;
    }
    if (!static_cast<CEventEditableResponse*>(currEvent)->ComputeResponseTaskOfType(ped, TASK_SIMPLE_INFORM_RESPECTED_FRIENDS)) {
        return false;
    }
    return intel->FindRespectedFriendInInformRange();
}

// 0x607D80
void CPedAcquaintanceScanner::ScanForPedAcquaintanceEvents(CPed* ped, CEntity** entities, int32 count) {
    // Scans at most twice a second (500 is read from 0x8D2358 by the exe)
    m_timer.StartIfNotAlready(500);
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    if (!IsScanAllowed(ped)) {
        return;
    }

    // The results are not looked at, the scan itself adds the events to the ped
    CPed* foundPed{};
    int32 foundType{ -1 };
    ScanForAcquaintances(ped, -1, entities, count, &foundPed, &foundType);
}

// 0x607A90
void CPedAcquaintanceScanner::ScanForAcquaintances(CPed* ped, int32 acquaintanceType, CEntity** entities, int32 numEntities, CPed** outPed, int32* outType) {
    plugin::CallMethod<0x607A90, CPedAcquaintanceScanner*, CPed*, int32, CEntity**, int32, CPed**, int32*>(this, ped, acquaintanceType, entities, numEntities, outPed, outType);
}

// 0x603BF0
void CSexyPedScanner::ScanForSexyPedEvents(CPed* ped, CEntity** entities, int32 count) {
    m_timer.StartIfNotAlready(500);
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    // Peds that sit in a vehicle do not look for anybody, except for mission peds
    if (!ped->IsCreatedBy(PED_MISSION) && ped->bInVehicle) {
        return;
    }

    const auto pedPos = ped->GetPosition();

    CPed* closestPed{};
    auto  closestDist2 = 1e10f; // 0x501502F9
    for (auto i = 0; i < count; i++) {
        auto* const other = static_cast<CPed*>(entities[i]);
        if (!other || other->m_nPedType != PED_TYPE_CIVFEMALE) {
            continue;
        }

        // The exe compares the signed bytes
        if (static_cast<int8>(other->m_pStats->m_nSexiness) <= static_cast<int8>(ped->m_pStats->m_nSexiness)) {
            continue;
        }

        if (other->bInVehicle) {
            continue;
        }

        const auto otherPos = other->GetPosition();
        const auto diff     = otherPos - pedPos;
        const auto dist2    = diff.SquaredMagnitude();
        if (!(dist2 < 10000.0f)) { // 100 units, read from 0x859AA4
            continue;
        }

        if (ped->GetPlayerData()) {
            g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_9, other);
        }

        if (!(dist2 < closestDist2)) {
            continue;
        }

        // Only peds in front of this ped, that can be seen past buildings and objects
        if (!(DotProduct(diff, ped->GetForward()) > 0.0f)) {
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, otherPos, true, false, false, true, false, false, false)) {
            continue;
        }

        closestPed   = other;
        closestDist2 = dist2;
    }

    if (!closestPed) {
        return;
    }

    CEventSexyPed event{ closestPed };
    ped->GetIntelligence()->GetEventGroup().Add(&event, false);

    // Waits longer after a sexy ped was found
    m_timer.Start(3000);
}

// 0x603E70
void CNearbyFireScanner::ScanForNearbyFireEvents(CPed* ped) {
    m_timer.StartIfNotAlready(0);
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(100);

    auto* const intel = ped->GetIntelligence();

    auto* const simplest  = intel->GetTaskManager().GetSimplestActiveTask();
    const auto  moveState = simplest && CTask::IsGoToTask(simplest)
        ? static_cast<CTaskSimpleGoTo*>(simplest)->m_moveState
        : PEDMOVE_STILL;

    auto* const activeTask = intel->GetTaskManager().GetActiveTask();

    auto* const fire = gFireManager.FindNearestFire(ped->GetPosition(), false, false);

    // Only read when there is a fire
    auto dist2 = 0.0f;
    auto diffZ = 0.0f;
    if (fire) {
        const auto diff = fire->GetPosition() - ped->GetPosition();
        diffZ = diff.z;
        dist2 = diff.SquaredMagnitude();
        if (dist2 < sq(20.0f) && std::fabs(diffZ) < 2.0f) {
            CEventFireNearby event{ fire->GetPosition() };
            intel->GetEventGroup().Add(&event, false);
        }
    }

    // Walking straight into a fire: only while a go to task is running, and not when the ped already walks around the fire
    if (!activeTask || activeTask->GetTaskType() == TASK_COMPLEX_WALK_ROUND_FIRE) {
        return;
    }
    auto* const simplestNow = intel->GetTaskManager().GetSimplestActiveTask();
    if (!simplestNow || !CTask::IsGoToTask(simplestNow) || !fire) {
        return;
    }
    if (dist2 < sq(4.0f) && std::fabs(diffZ) < 2.0f) {
        CEventPotentialWalkIntoFire event{ fire->GetPosition(), fire->GetStrength(), moveState };
        intel->GetEventGroup().Add(&event, false);
    }
}
