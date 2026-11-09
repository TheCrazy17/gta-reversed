#include "StdInc.h"
#include "TaskCategories.h"

#include "TaskComplexFleeEntity.h"
#include "TaskComplexSmartFleeEntity.h"

void CTaskCategories::InjectHooks() {
    RH_ScopedClass(CTaskCategories);
    RH_ScopedCategory("Tasks");

    RH_ScopedInstall(ComputeTaskCategory, 0x698680);
    RH_ScopedInstall(IsFleePedTask, 0x698620);
    RH_ScopedInstall(IsFollowPedTask, 0x698610);
    RH_ScopedInstall(IsKillPedTask, 0x6985E0);
}

// 0x698680
int8 CTaskCategories::ComputeTaskCategory(const CTask* task) {
    // The exe calls GetTaskType three times, each time testing the 'fight / gun / throw control' range and
    // overwriting the category from the previous test; the result is the same as testing the range once.
    // Any task in that range is category 4 and any other task is 1 (the exe starts from `task != nullptr`,
    // but it crashes on a null task when it asks it for its type, so that is always 1)
    const auto type = task->GetTaskType();
    if (type >= TASK_SIMPLE_FIGHT_CTRL && type <= TASK_SIMPLE_THROW_CTRL) {
        return 4;
    }

    return 1;
}

// 0x698620
void CTaskCategories::IsFleePedTask(const CTask* task, bool& isFleeingFromPed, bool& isFleeingFromPlayer) {
    isFleeingFromPed    = false;
    isFleeingFromPlayer = false;

    // Both flee tasks keep the entity they run from in their first own field (offset 0xC)
    const CEntity* fleeFrom;
    switch (task->GetTaskType()) {
    case TASK_COMPLEX_FLEE_ENTITY:
        fleeFrom = static_cast<const CTaskComplexFleeEntity*>(task)->m_entity;
        break;
    case TASK_COMPLEX_SMART_FLEE_ENTITY:
        fleeFrom = static_cast<const CTaskComplexSmartFleeEntity*>(task)->m_fleeFrom;
        break;
    default:
        return;
    }

    if (!fleeFrom || !fleeFrom->GetIsTypePed()) {
        return;
    }

    isFleeingFromPed = true;
    if (fleeFrom->AsPed()->IsPlayer()) {
        isFleeingFromPlayer = true;
    }
}

// 0x698610
void CTaskCategories::IsFollowPedTask(const CTask* task, bool& a2, bool& a3) {
    a2 = false;
    a3 = false;
}

// 0x6985E0
void CTaskCategories::IsKillPedTask(const CTask* task, bool& a2, bool& a3) {
    a2 = false;
    a3 = false;

    const auto type = task->GetTaskType();
    if (type >= TASK_SIMPLE_FIGHT_CTRL && type <= TASK_SIMPLE_THROW_CTRL) {
        a2 = true;
    }
}
