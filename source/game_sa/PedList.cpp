#include "StdInc.h"
#include "PedList.h"

#include "PedGroupMembership.h"
#include "PedIntelligence.h"
#include "Pools/Pools.h"
#include "Tasks/TaskTypes/TaskComplexKillPedOnFoot.h"

void CPedList::InjectHooks() {
    RH_ScopedClass(CPedList);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Empty, 0x699DB0);
    RH_ScopedInstall(BuildListFromGroup_NoLeader, 0x699DD0);
    RH_ScopedInstall(FillUpHoles, 0x699E20);
    RH_ScopedInstall(ExtractPedsWithGuns, 0x69A4C0);
    RH_ScopedInstall(BuildListFromGroup_NotInCar_NoLeader, 0x69A340);
    RH_ScopedInstall(BuildListOfPedsOfPedType, 0x69A3B0);
    RH_ScopedInstall(RemovePedsAttackingPedType, 0x69A450);
    RH_ScopedInstall(RemovePedsThatDontListenToPlayer, 0x69A420);
}

// 0x699DB0
void CPedList::Empty() {
    *this = {};
}

// 0x699DD0
void CPedList::BuildListFromGroup_NoLeader(CPedGroupMembership& groupMembership) {
    m_count = 0;
    for (auto* const mem : groupMembership.GetMembers(false)) {
        AddMember(mem);
    }
    ClearUnused();
}

// 0x69A4C0
void CPedList::ExtractPedsWithGuns(CPedList& from) {
    for (auto i = 0u; i < from.m_count; i++) {
        if (!from.Get(i)->GetActiveWeapon().IsTypeMelee()) {
            AddMember(from.Get(i));
            from.RemoveMemberNoFill(i);
        }
    }
    from.FillUpHoles();
}


// 0x699E20
// After nulling out a field in the
// array there might be a hole, so it has to be filled
void CPedList::FillUpHoles() {
    rng::fill(rng::remove(m_peds, nullptr), nullptr);
}

// 0x69A340
void CPedList::BuildListFromGroup_NotInCar_NoLeader(CPedGroupMembership* pedGroupMembership) {
    m_count = 0;
    for (auto* const ped : pedGroupMembership->GetFollowers()) {
        if (!ped->GetIntelligence()->IsInACarOrEnteringOne()) {
            AddMember(ped);
        }
    }
    ClearUnused();
}

// 0x69A3B0
void CPedList::BuildListOfPedsOfPedType(int32 pedType) {
    m_count = 0;

    // NOTE: The original code iterates the pool backwards (last slot first),
    // which affects which peds make it into the list if there are more than 30.
    for (auto i = GetPedPool()->GetSize(); i-- > 0;) {
        if (auto* const ped = GetPedPool()->GetAt(i)) {
            if (ped->m_nPedType == (ePedType)pedType && m_count < m_peds.size()) {
                AddMember(ped);
            }
        }
    }

    ClearUnused();
}

// 0x69A450
void CPedList::RemovePedsAttackingPedType(int32 pedType) {
    // The loop bound is the count at entry, even though removals shrink `m_count`
    for (auto i = 0u, numPeds = m_count; i < numPeds; i++) {
        auto* task = m_peds[i]->GetIntelligence()->FindTaskByType(TASK_COMPLEX_KILL_PED_ON_FOOT);
        auto* target = task ? static_cast<CTaskComplexKillPedOnFoot*>(task)->m_target : nullptr;
        if (!target || target->m_nPedType != pedType) {
            m_peds[i] = nullptr;
            m_count--;
        }
    }
    FillUpHoles();
}

// 0x69A420
void CPedList::RemovePedsThatDontListenToPlayer() {
    // NOTE: Going backwards, so `m_count` being modified inside the loop doesn't matter
    for (auto i = (int32)m_count; i-- > 0;) {
        if (m_peds[i]->bDoesntListenToPlayerGroupCommands) {
            RemoveMemberNoFill(i);
        }
    }
    FillUpHoles();
}

//
// NOTSA section
//

// nulls out everything after the first `m_count` elements
void CPedList::ClearUnused() {
    rng::fill(m_peds | std::views::drop(m_count), nullptr);
}

void CPedList::AddMember(CPed* ped) {
    m_peds[m_count++] = ped;
}

// Must call FillUpHoles afterwards!
void CPedList::RemoveMemberNoFill(int32 i) {
    m_peds[i] = nullptr;
    m_count--;
}

CPed* CPedList::Get(int32 i) {
    return m_peds[i];
}
