#include "StdInc.h"

#include "TaskAllocatorKillThreatsBasic.h"
#include <InterestingEvents.h>
#include <TaskComplexKillPedGroupOnFoot.h>
#include <TaskComplexSequence.h>
#include <TaskSimpleLookAbout.h>

// 0x69C710
CTaskAllocatorKillThreatsBasic::CTaskAllocatorKillThreatsBasic(CPed* threat) :
    m_Threat{threat}
{
}

// 0x69D170
void CTaskAllocatorKillThreatsBasic::AllocateTasks(CPedGroupIntelligence* intel) {
    intel->FlushTasks(intel->GetPedTaskPairs(), nullptr);
    intel->FlushTasks(intel->GetSecondaryPedTaskPairs(), nullptr);

    if (!m_Threat) {
        return;
    }

    auto* const group = &intel->GetPedGroup();
    if (auto* const threatsGroup = m_Threat->GetGroup()) {
        if (threatsGroup == group) {
            NOTSA_LOG_DEBUG("ComputeKillThreatsBasicResponse() - threat ped already in group"); // vanilla
        } else {
            CPed* closest[TOTAL_PED_GROUP_MEMBERS]{};
            ComputeClosestPeds(*group, *threatsGroup, closest);
            for (int32 i = 0; i < TOTAL_PED_GROUP_MEMBERS; i++) {
                auto* mem = group->GetMembership().GetMember(i);
                if (!mem || mem->IsPlayer()) {
                    continue;
                }
                intel->SetEventResponseTask(
                    mem,
                    CTaskComplexKillPedGroupOnFoot{ CPedGroups::GetGroupId(threatsGroup), closest[i] }
                );
            }
            g_InterestingEvents.Add(CInterestingEvents::GANG_FIGHT, group->GetMembership().GetLeader()); // 0x69D436
        }
    } else { // 0x69D2EB
        for (auto* const mem : group->GetMembership().GetMembers()) {
            if (mem->IsPlayer()) {
                continue;
            }
            intel->SetEventResponseTask(
                mem,
                CTaskComplexSequence{
                    new CTaskComplexKillPedOnFoot{ m_Threat },
                    new CTaskSimpleLookAbout{ CGeneral::GetRandomNumberInRange(1'000u, 2'000u) },
                }
            );
        }
        g_InterestingEvents.Add(CInterestingEvents::GANG_ATTACKING_PED, group->GetMembership().GetLeader()); // 0x69D436
    }
}

// 0x69C7E0
CTaskAllocator* CTaskAllocatorKillThreatsBasic::ProcessGroup(CPedGroupIntelligence* intel) {
    m_Timer.StartIfNotAlready(0);
    if (m_Timer.IsOutOfTime()) {
        m_Timer.Start(5'000);
        AllocateTasks(intel);
    }
    return this;
}

// 0x69C850
void CTaskAllocatorKillThreatsBasic::ComputeClosestPeds(CPedGroup& group1, CPedGroup& group2, CPed** peds) {
    std::fill_n(peds, TOTAL_PED_GROUP_MEMBERS, nullptr);

    float distSq[TOTAL_PED_GROUP_MEMBERS][TOTAL_PED_GROUP_MEMBERS];
    rng::fill(distSq[0], distSq[0] + TOTAL_PED_GROUP_MEMBERS * TOTAL_PED_GROUP_MEMBERS, std::numeric_limits<float>::max());

    for (auto i = 0; i < TOTAL_PED_GROUP_MEMBERS; i++) {
        auto* memA = group1.GetMembership().GetMember(i);
        if (!memA || !memA->IsAlive() || memA->IsPlayer()) {
            continue;
        }
        for (auto j = 0; j < TOTAL_PED_GROUP_MEMBERS; j++) {
            auto* memB = group2.GetMembership().GetMember(j);
            if (!memB || !memB->IsAlive()) {
                continue;
            }
            distSq[i][j] = (memB->GetPosition() - memA->GetPosition()).SquaredMagnitude();
        }
    }

    // Greedy nearest-pair matching: repeatedly pick the globally-closest (group1 member, group2
    // member) pair, assign it, then exclude that row/column from further consideration.
    for (auto pass = 0; pass < TOTAL_PED_GROUP_MEMBERS; pass++) {
        auto  bestRow  = -1;
        auto  bestCol  = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (auto row = 0; row < TOTAL_PED_GROUP_MEMBERS; row++) {
            for (auto col = 0; col < TOTAL_PED_GROUP_MEMBERS; col++) {
                if (distSq[row][col] < bestDist) {
                    bestDist = distSq[row][col];
                    bestRow  = row;
                    bestCol  = col;
                }
            }
        }
        if (bestRow == -1 || bestCol == -1) {
            continue;
        }
        for (auto k = 0; k < TOTAL_PED_GROUP_MEMBERS; k++) {
            distSq[bestRow][k] = std::numeric_limits<float>::max();
            distSq[k][bestCol] = std::numeric_limits<float>::max();
        }
        peds[bestRow] = group2.GetMembership().GetMember(bestCol);
    }

    // Fallback: any group1 member still unmatched (e.g. no living group2 member was found) gets
    // pointed at group2's leader, or failing that the first living group2 follower.
    if (auto* leader = group2.GetMembership().GetLeader(); leader && leader->IsAlive()) {
        for (auto i = 0; i < TOTAL_PED_GROUP_MEMBERS; i++) {
            if (group1.GetMembership().GetMember(i) && !peds[i]) {
                peds[i] = leader;
            }
        }
        return;
    }

    for (auto j = 0; j < CPedGroupMembership::LEADER_MEM_ID; j++) {
        auto* mem = group2.GetMembership().GetMember(j);
        if (!mem || !mem->IsAlive()) {
            continue;
        }
        for (auto i = 0; i < TOTAL_PED_GROUP_MEMBERS; i++) {
            if (group1.GetMembership().GetMember(i) && !peds[i]) {
                peds[i] = mem;
            }
        }
        return;
    }
}

void CTaskAllocatorKillThreatsBasic::InjectHooks() {
    RH_ScopedVirtualClass(CTaskAllocatorKillThreatsBasic, 0x870e90, 6);
    RH_ScopedCategory("Tasks/Allocators");

    RH_ScopedInstall(Constructor, 0x69C710);
    RH_ScopedInstall(Destructor, 0x69C780);

    RH_ScopedGlobalInstall(ComputeClosestPeds, 0x69C850);
    RH_ScopedVMTInstall(GetType, 0x69C770);
    RH_ScopedVMTInstall(AllocateTasks, 0x69D170);
    RH_ScopedVMTInstall(ProcessGroup, 0x69C7E0);
}
