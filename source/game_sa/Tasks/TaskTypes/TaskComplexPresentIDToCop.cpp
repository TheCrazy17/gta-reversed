#include "StdInc.h"

#include "TaskComplexPresentIDToCop.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleHandsUp.h"

void CTaskComplexPresentIDToCop::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPresentIDToCop, 0x86F544, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6591D0);
    RH_ScopedInstall(Destructor, 0x659240);
    RH_ScopedInstall(ComputeHeadingToFaceCop, 0x6592A0);
    RH_ScopedInstall(CreateSubTask, 0x65A110);

    RH_ScopedVMTInstall(Clone, 0x659B60);
    RH_ScopedVMTInstall(GetTaskType, 0x659230);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x65AFE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x65B050);
    RH_ScopedVMTInstall(ControlSubTask, 0x65B070);
}

// 0x6591D0
CTaskComplexPresentIDToCop::CTaskComplexPresentIDToCop(CPed* cop) :
    m_Cop{cop}
{
    CEntity::SafeRegisterRef(m_Cop); // The exe does not test for null here
}

// 0x659240
CTaskComplexPresentIDToCop::~CTaskComplexPresentIDToCop() {
    CEntity::SafeCleanUpRef(m_Cop);
}

// 0x6592A0
float CTaskComplexPresentIDToCop::ComputeHeadingToFaceCop(CPed* ped) {
    const auto& copPos = m_Cop->GetPosition();
    const auto& pedPos = ped->GetPosition();
    return CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(copPos.x, copPos.y, pedPos.x, pedPos.y));
}

// 0x65AFE0
CTask* CTaskComplexPresentIDToCop::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_HANDS_UP:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_SIMPLE_ACHIEVE_HEADING:
        ped->Say(CTX_GLOBAL_CRIMINAL_PLEAD, 0, 1.0f, false, false, false);
        return CreateSubTask(TASK_SIMPLE_HANDS_UP, ped);
    default:
        return nullptr;
    }
}

// 0x65B050
CTask* CTaskComplexPresentIDToCop::CreateFirstSubTask(CPed* ped) {
    return CreateSubTask(TASK_SIMPLE_ACHIEVE_HEADING, ped);
}

// 0x65B070
CTask* CTaskComplexPresentIDToCop::ControlSubTask(CPed* ped) {
    if (!m_Cop) { // The cop is gone
        return CreateSubTask(TASK_FINISHED, ped);
    }
    return m_pSubTask;
}

// 0x65A110
CTask* CTaskComplexPresentIDToCop::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_HANDS_UP:
        return new CTaskSimpleHandsUp{3000};
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return new CTaskSimpleAchieveHeading{ComputeHeadingToFaceCop(ped), 0.5f, 0.2f};
    default: // TASK_FINISHED and anything else
        return nullptr;
    }
}
