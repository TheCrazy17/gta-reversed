#include "StdInc.h"

#include "TaskComplexFallAndStayDown.h"
#include "TaskSimpleFall.h"

void CTaskComplexFallAndStayDown::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFallAndStayDown, 0x870480, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6789D0);
    RH_ScopedInstall(Constructor2, 0x678A10);
    RH_ScopedInstall(CreateSubTask, 0x678BD0);

    RH_ScopedVMTInstall(Clone, 0x67C270);
    RH_ScopedVMTInstall(GetTaskType, 0x678A00);
    RH_ScopedVMTInstall(MakeAbortable, 0x678AA0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x67CBE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x678B20);
    RH_ScopedVMTInstall(ControlSubTask, 0x678BC0);
}

// 0x6789D0
CTaskComplexFallAndStayDown::CTaskComplexFallAndStayDown(AnimationId fallAnimId, AssocGroupId fallAnimGroup) :
    m_FallAnimId{fallAnimId},
    m_FallAnimGroup{fallAnimGroup}
{
}

// 0x678A10
CTaskComplexFallAndStayDown::CTaskComplexFallAndStayDown(int32 dir) :
    m_FallAnimGroup{ANIM_GROUP_DEFAULT}
{
    // Same mapping as `CTaskComplexFallAndGetUp`; the exe leaves the animation uninitialised for `dir` > 3
    switch (dir) {
    case 0: m_FallAnimId = ANIM_ID_KO_SKID_FRONT; break;
    case 1: m_FallAnimId = ANIM_ID_KO_SPIN_R;     break;
    case 2: m_FallAnimId = ANIM_ID_KO_SKID_BACK;  break;
    case 3: m_FallAnimId = ANIM_ID_KO_SPIN_L;     break;
    default: break;
    }
}

// 0x678AA0
bool CTaskComplexFallAndStayDown::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        return m_pSubTask->MakeAbortable(ped, priority, event);
    }

    // Nothing but IMMEDIATE gets through while the fall task is still the active sub task
    const auto isFalling = m_pSubTask->GetTaskType() == TASK_SIMPLE_FALL;
    if (priority == ABORT_PRIORITY_URGENT) {
        return !isFalling && m_pSubTask->MakeAbortable(ped, priority, event);
    }

    // `LEISURE`: the request is forwarded, but its result is discarded; the task never reports itself as abortable
    if (!isFalling) {
        m_pSubTask->MakeAbortable(ped, priority, event);
    }
    return false;
}

// 0x67CBE0
CTask* CTaskComplexFallAndStayDown::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_FALL) {
        return CreateSubTask(TASK_FINISHED);
    }
    return nullptr;
}

// 0x678B20
CTask* CTaskComplexFallAndStayDown::CreateFirstSubTask(CPed* ped) {
    return CreateSubTask(TASK_SIMPLE_FALL);
}

// 0x678BD0
CTask* CTaskComplexFallAndStayDown::CreateSubTask(eTaskType taskType) {
    switch (taskType) {
    case TASK_SIMPLE_FALL:
        return new CTaskSimpleFall{m_FallAnimId, m_FallAnimGroup, 99'999'999}; // Practically forever
    default: // TASK_FINISHED and anything else
        return nullptr;
    }
}
