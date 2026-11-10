#include "StdInc.h"

#include "TaskComplexWanderStandard.h"

#include "PedGroup.h"
#include "EventAcquaintancePedDislike.h"
#include "EventChatPartner.h"
#include "EventSexyVehicle.h"

void CTaskComplexWanderStandard::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderStandard, 0x85A200, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x48E4F0);
    RH_ScopedInstall(WillChat, 0x66AED0);
    RH_ScopedInstall(SetNextMinScanTime, 0x66AF60);
    RH_ScopedInstall(LookForSexyCars, 0x66AFD0);
    RH_ScopedInstall(LookForChatPartners, 0x66FDA0);
    RH_ScopedInstall(LookForGangMembers, 0x670100);
    RH_ScopedVMTInstall(Clone, 0x48E530);
    RH_ScopedVMTInstall(GetWanderType, 0x48E5D0);
    RH_ScopedVMTInstall(ScanForStuff, 0x672600);
}

// 0x48E4F0
CTaskComplexWanderStandard::CTaskComplexWanderStandard(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWander(MoveState, Dir, bWanderSensibly)
{
}

// 0x672600
void CTaskComplexWanderStandard::ScanForStuff(CPed* ped) {
    m_TaskTimer.StartIfNotAlready(50);

    // Set by SetNextMinScanTime after the ped reacted to something
    if (CTimer::GetTimeInMS() < m_nMinNextScanTime) {
        return;
    }

    if (m_TaskTimer.IsOutOfTime()) {
        m_TaskTimer.Start(50);
        if (!LookForGangMembers(ped) && !LookForChatPartners(ped)) {
            LookForSexyCars(ped);
        }
    }
}

// 0x66AED0
bool CTaskComplexWanderStandard::WillChat(const CPed& first, const CPed& second) {
    // Criminals, cops, gang members and the player never chat while wandering
    for (const CPed* const ped : { &first, &second }) {
        if (ped->m_nPedType == PED_TYPE_CRIMINAL || ped->m_nPedType == PED_TYPE_COP || ped->IsPlayer() || ped->IsGangster()) {
            return false;
        }
    }
    return CPedIntelligence::AreFriends(first, second);
}

// 0x66AF60
void CTaskComplexWanderStandard::SetNextMinScanTime(CPed* ped) {
    auto* const activeTask = ped->GetTaskManager().GetActiveTask();
    if (!activeTask) {
        return;
    }
    // The ped's active wander task (of the same wander type as this one) stops scanning for 100 seconds after a reaction
    if (activeTask->GetTaskType() == GetTaskType() && static_cast<CTaskComplexWander*>(activeTask)->GetWanderType() == GetWanderType()) {
        static_cast<CTaskComplexWanderStandard*>(activeTask)->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100000;
    }
}

// 0x66AFD0
bool CTaskComplexWanderStandard::LookForSexyCars(CPed* ped) {
    const auto& pedPos = ped->GetPosition();
    for (CEntity* const entity : ped->GetIntelligence()->GetVehicleScanner().m_apEntities) {
        auto* const veh = static_cast<CVehicle*>(entity);
        if (!veh || veh == ped->m_pVehicle) {
            continue;
        }

        // Only valuable cars that are not close to wrecked are worth a look
        if (veh->m_pHandlingData->m_nMonetaryValue <= 40'000u || veh->m_fHealth <= 500.0f) {
            continue;
        }

        const auto& vehPos = veh->GetPosition();
        const auto  toVeh  = vehPos - pedPos;
        if (toVeh.SquaredMagnitude() >= sq(5.0f)) {
            continue;
        }

        // Has to be in front of the ped
        if (toVeh.Dot(ped->GetForward()) <= 0.0f) {
            continue;
        }

        if (!CWorld::GetIsLineOfSightClear(pedPos, vehPos, true, false, false, true, false, false, false)) {
            continue;
        }

        CEventSexyVehicle eventSexyVehicle{ veh };
        ped->GetEventGroup().Add(&eventSexyVehicle, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}

// 0x66FDA0
bool CTaskComplexWanderStandard::LookForChatPartners(CPed* ped) {
    // Peds only stop to chat on pavement
    if (!g_surfaceInfos.IsPavement(ped->m_nContactSurface)) {
        return false;
    }

    // Only while walking or slower
    if (m_nMoveState > PEDMOVE_WALK) {
        return false;
    }

    // No chatting while the player drives past quickly; sq(0.2f) is the exact float the exe compares against
    auto* const player = FindPlayerPed();
    if (player->bInVehicle && player->m_pVehicle && player->m_pVehicle->m_vecMoveSpeed.SquaredMagnitude() > sq(0.2f)) {
        return false;
    }

    if (CStreaming::IsVeryBusy() || CGameLogic::LaRiotsActiveHere()) {
        return false;
    }

    const auto& pedPos   = ped->GetPosition();
    auto* const pedIntel = ped->GetIntelligence();
    for (CEntity* const entity : pedIntel->GetPedScanner().m_apEntities) {
        auto* const other = static_cast<CPed*>(entity);
        if (!other || !g_surfaceInfos.IsPavement(other->m_nContactSurface)) {
            continue;
        }

        // The other ped has to be wandering as well
        auto* const otherTask = other->GetTaskManager().GetActiveTask();
        if (!otherTask || otherTask->GetTaskType() != GetTaskType()) {
            continue;
        }

        auto* const otherIntel = other->GetIntelligence();

        // Neither of them may be chatting already, about to chat, or be a couple
        if (pedIntel->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT) || otherIntel->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT)) {
            continue;
        }
        if (pedIntel->GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER) || otherIntel->GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER)) {
            continue;
        }
        if (pedIntel->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE) || otherIntel->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE)) {
            continue;
        }

        if (!WillChat(*ped, *other)) {
            continue;
        }

        const auto& otherPos = other->GetPosition();
        const auto  toOther  = otherPos - pedPos;
        if (toOther.SquaredMagnitude() >= sq(10.0f)) {
            continue;
        }

        // They have to face each other
        if (toOther.Dot(ped->GetForward()) <= 0.0f) {
            continue;
        }
        if (toOther.Dot(other->GetForward()) >= 0.0f) {
            continue;
        }

        if (!CWorld::GetIsLineOfSightClear(pedPos, otherPos, true, false, false, true, false, false, false)) {
            continue;
        }

        // The ped that found the partner leads the conversation
        CEventChatPartner pedEvent{ true, other };
        pedIntel->GetEventGroup().Add(&pedEvent, false);

        CEventChatPartner otherEvent{ false, ped };
        otherIntel->GetEventGroup().Add(&otherEvent, false);

        SetNextMinScanTime(ped);
        SetNextMinScanTime(other);
        return true;
    }
    return false;
}

// 0x670100
bool CTaskComplexWanderStandard::LookForGangMembers(CPed* ped) {
    // Peds that are in a group do not avoid gangs
    if (CPedGroups::GetPedsGroup(ped)) {
        return false;
    }

    CVector nextNodePos;
    ComputeTargetPos(ped, nextNodePos, m_NextNode);

    const auto& pedPos = ped->GetPosition();
    for (CEntity* const entity : ped->GetIntelligence()->GetPedScanner().m_apEntities) {
        auto* const other = static_cast<CPed*>(entity);
        if (!other) {
            continue;
        }

        // The other ped has to be near the next path node, on the same side of it as this ped
        const auto nodeToPed   = pedPos - nextNodePos;
        const auto nodeToOther = other->GetPosition() - nextNodePos;
        if (nodeToOther.Dot(nodeToPed) <= 0.0f) {
            continue;
        }
        if (nodeToOther.SquaredMagnitude() >= sq(5.0f)) {
            continue;
        }

        auto* const otherGroup = CPedGroups::GetPedsGroup(other);
        if (!otherGroup || otherGroup->GetMembership().CountMembersExcludingLeader() <= 0) {
            continue;
        }

        // Walk away from the group; the event response is overridden to a walking flee
        CEventAcquaintancePedDislike eventDislike{ other, TASK_SMART_FLEE_ENTITY_WALKING };
        ped->GetEventGroup().Add(&eventDislike, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}
