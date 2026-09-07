#pragma once

#include <Base.h>
#include <PluginBase.h>
#include <reversiblehooks/ReversibleHooks.h>

#include "./PedGroupDefaultTaskAllocator.h"
#include "TaskComplexGangFollower.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexFollowLeaderInFormation.h"

class NOTSA_EXPORT_VTABLE CPedGroupDefaultTaskAllocatorRandom final : public CPedGroupDefaultTaskAllocator {
public:
    /* no virtual destructor */

    // 0x5F6E90
    void AllocateDefaultTasks(CPedGroup* pedGroup, CPed* ped) const override {
        auto* const leader = pedGroup->GetMembership().GetLeader();
        auto& taskPairs = pedGroup->GetIntelligence().GetDefaultPedTaskPairs();
        for (auto i = 0; i < CPedGroupMembership::LEADER_MEM_ID; i++) {
            auto& tp = taskPairs[i];
            if (!(tp.Ped && (!ped || tp.Ped == ped))) {
                continue;
            }
            const auto& offs = CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[i];
            auto* const task = new CTaskComplexGangFollower{ pedGroup, leader, (uint8)i, CVector{ offs.x, offs.y, 0.0f }, 10.0f };
            task->m_Flags = (task->m_Flags & ~4) | (pedGroup->m_bMembersEnterLeadersVehicle ? 4 : 0);
            tp.Task = task;
        }
        auto& leaderTp = taskPairs[CPedGroupMembership::LEADER_MEM_ID];
        if (leaderTp.Ped && (!ped || leaderTp.Ped == ped)) {
            leaderTp.Task = new CTaskComplexGangLeader{ pedGroup };
        }
    };
    ePedGroupDefaultTaskAllocatorType GetType() const override { return ePedGroupDefaultTaskAllocatorType::RANDOM; }; // 0x5F6530

public:
    static inline void InjectHooks() {
        RH_ScopedVirtualClass(CPedGroupDefaultTaskAllocatorRandom, 0x86C77C, 2);
        RH_ScopedCategory("Tasks/Allocators/PedGroup");

        RH_ScopedVMTInstall(AllocateDefaultTasks, 0x5F6E90);
        RH_ScopedVMTInstall(GetType, 0x5F6530);
    }
};
VALIDATE_SIZE(CPedGroupDefaultTaskAllocatorRandom, sizeof(void*)); /* vtable only */
