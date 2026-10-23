#include "StdInc.h"

#include "PedGroups.h"

void CPedGroups::InjectHooks() {
    RH_ScopedClass(CPedGroups);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Process, 0x5FC800);
    RH_ScopedInstall(RemoveGroup, 0x5FB870);
    RH_ScopedInstall(AddGroup, 0x5FB800);
    RH_ScopedInstall(RemoveAllFollowersFromGroup, 0x5FB8A0);
    RH_ScopedInstall(Init, 0x5FB8C0);
    RH_ScopedInstall(CleanUpForShutDown, 0x5FB930);
    RH_ScopedInstall(IsGroupLeader, 0x5F7E40);
    RH_ScopedInstall(GetPedsGroup, 0x5F7E80);
    RH_ScopedInstall(GetGroupId, 0x5F7EE0);
    RH_ScopedInstall(IsInPlayersGroup, 0x5F7F10);
    RH_ScopedInstall(AreInSameGroup, 0x5F7F40);
}

#ifdef ANDROID
void CPedGroups::Save() {
    
}

void CPedGroups::Load() {
    
}
#endif

// return the index of the added group , return -1 if failed.
// 0x5FB800
int32 CPedGroups::AddGroup() {
    for (auto i = 0; i < (int32)ms_activeGroups.size(); i++) {
        if (ms_activeGroups[i]) {
            continue;
        }
        ms_activeGroups[i] = true;

        auto& group = GetGroup(i);
        for (auto memberIdx = 0; memberIdx < TOTAL_PED_GROUP_MEMBERS; memberIdx++) {
            if (group.GetMembership().GetMember(memberIdx)) {
                group.GetMembership().RemoveMember(memberIdx);
            }
        }
        group.GetIntelligence().Flush();
        group.m_bIsMissionGroup = false;
        return i;
    }
    return -1;
}

// 0x5FB870
void CPedGroups::RemoveGroup(int32 groupId) {
    auto* const group = &GetGroup(groupId);
    if (!std::exchange(ms_activeGroups[groupId], false)) {
        return;
    }
    group->Flush();
}

// 0x5FB8A0
void CPedGroups::RemoveAllFollowersFromGroup(int32 groupId) {
    if (ms_activeGroups[groupId]) {
        ms_groups[groupId].RemoveAllFollowers();
    }
}

// 0x5FB8C0
void CPedGroups::Init() {
    for (auto i = 0u; i < ms_groups.size(); i++) {
        if (ms_activeGroups[i]) {
            ms_activeGroups[i] = false;

            auto& group = ms_groups[i];
            for (auto memberId = 0; memberId < TOTAL_PED_GROUP_MEMBERS; memberId++) {
                if (group.GetMembership().GetMember(memberId)) {
                    group.GetMembership().RemoveMember(memberId);
                }
            }
            group.GetIntelligence().Flush();
            group.m_bIsMissionGroup = false;
        }
        ScriptReferenceIndex[i] = 1;
    }
}

// 0x5F7E30
void CPedGroups::RegisterKillByPlayer() {
    if (ms_bIsPlayerOnAMission)
        ms_iNoOfPlayerKills++;
}

// 0x5FB930
void CPedGroups::CleanUpForShutDown() {
    for (auto& group : ms_groups) {
        for (auto memberId = 0; memberId < TOTAL_PED_GROUP_MEMBERS; memberId++) {
            if (group.GetMembership().GetMember(memberId)) {
                group.GetMembership().RemoveMember(memberId);
            }
        }
        group.GetIntelligence().Flush();
        group.m_bIsMissionGroup = false;
    }
}

// 0x5F7E40
bool CPedGroups::IsGroupLeader(CPed* ped) {
    if (!ped) {
        return false;
    }
    for (auto i = 0u; i < ms_groups.size(); i++) {
        if (ms_activeGroups[i] && ms_groups[i].GetMembership().IsLeader(ped)) {
            return true;
        }
    }
    return false;
}

// 0x5F7E80
CPedGroup* CPedGroups::GetPedsGroup(const CPed* ped) {
    if (!ped) {
        return nullptr;
    }
    for (auto i = 0u; i < ms_groups.size(); i++) {
        if (ms_activeGroups[i] && ms_groups[i].GetMembership().IsMember(ped)) {
            return &ms_groups[i];
        }
    }
    return nullptr;
}

// 0x5F7EE0
int32 CPedGroups::GetGroupId(const CPedGroup* pedGroup) {
    for (auto i = 0u; i < ms_groups.size(); i++) {
        if (&ms_groups[i] == pedGroup) {
            return (int32)i;
        }
    }
    return -1;
}

// 0x5FC800
void CPedGroups::Process() {
    for (auto&& [i, g] : GetActiveGroupsWithIDs()) {
        g.Process();
        if (!g.GetMembership().CountMembers()) {
            RemoveGroup(i);
        }
    }

    const auto SetGroupsDecisionMakerType = [](eDecisionMakerType dm) {
        for (auto&& [_, g] : GetActiveGroupsWithIDs()) {
            if (!g.m_bIsMissionGroup) { // Mission groups keep the decision maker the script gave them
                g.GetIntelligence().SetGroupDecisionMakerType(dm);
            }
        }
    };
    if (CTheScripts::IsPlayerOnAMission() && !ms_bIsPlayerOnAMission) { // 0x5FC897 - Player started a mission
        ms_iNoOfPlayerKills = 0;
        SetGroupsDecisionMakerType(eDecisionMakerType::GROUP_RANDOM_PASSIVE);
    } else if (!CTheScripts::IsPlayerOnAMission() && ms_bIsPlayerOnAMission) { // 0x5FC8B2 - Player ended a mission (inverted)
        ms_iNoOfPlayerKills = 0;
        SetGroupsDecisionMakerType(eDecisionMakerType::UNKNOWN);
    } else if (CTheScripts::IsPlayerOnAMission() && CPedGroups::ms_iNoOfPlayerKills == 8) { // 0x5FC8BF (inverted)
        SetGroupsDecisionMakerType(eDecisionMakerType::UNKNOWN);
    } else if (CTheScripts::IsPlayerOnAMission()) {
        SetGroupsDecisionMakerType(eDecisionMakerType::GROUP_RANDOM_PASSIVE);
    }

    if (!std::exchange(ms_bIsPlayerOnAMission, CTheScripts::IsPlayerOnAMission())) { // 0x5FC98D
        ms_iNoOfPlayerKills = 0;
    }
}

// 0x5F7F10
bool CPedGroups::IsInPlayersGroup(CPed* ped) {
    // Players are never "in" their own group, only the peds that follow them are
    return !ped->GetPlayerData() && ms_groups[0].GetMembership().IsMember(ped);
}

CPedGroup& CPedGroups::GetGroup(int32 groupId) {
    return ms_groups[groupId];
}

// 0x5F7F40
bool CPedGroups::AreInSameGroup(const CPed* ped1, const CPed* ped2) {
    if (!ped1 || !ped2) {
        return false;
    }
    for (auto i = 0u; i < ms_groups.size(); i++) {
        if (!ms_activeGroups[i]) {
            continue;
        }
        const auto& membership = ms_groups[i].GetMembership();
        if (membership.IsMember(ped1) && membership.IsMember(ped2)) {
            return true;
        }
    }
    return false;
}
