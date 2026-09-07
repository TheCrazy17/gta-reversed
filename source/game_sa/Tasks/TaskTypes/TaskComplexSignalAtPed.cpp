#include "StdInc.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleDoHandSignal.h"
#include "TaskSimpleRunAnim.h"

void CTaskComplexSignalAtPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSignalAtPed, 0x86fa8c, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x660A30);
    RH_ScopedInstall(Destructor, 0x660AB0);

    RH_ScopedVMTInstall(Clone, 0x662140);
    RH_ScopedVMTInstall(GetTaskType, 0x660AA0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x660B30);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x660CC0, {.reversed = false});
    RH_ScopedVMTInstall(ControlSubTask, 0x660D80, {.reversed = false});
}

// 0x660A30
CTaskComplexSignalAtPed::CTaskComplexSignalAtPed(CPed* pedToSignalAt, int32 unused1, bool playAnimAtEnd) :
    m_pedToSignalAt{pedToSignalAt},
    m_playAnimAtEnd{playAnimAtEnd}
{
    CEntity::SafeRegisterRef(m_pedToSignalAt);
}

CTaskComplexSignalAtPed::CTaskComplexSignalAtPed(const CTaskComplexSignalAtPed& o) :
    CTaskComplexSignalAtPed{o.m_pedToSignalAt, o.m_initialPause, o.m_playAnimAtEnd}
{
}

// 0x660AB0
CTaskComplexSignalAtPed::~CTaskComplexSignalAtPed() {
    CEntity::SafeCleanUpRef(m_pedToSignalAt);
}

// 0x660B30
CTask* CTaskComplexSignalAtPed::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_TURN_TO_FACE_ENTITY: {
        auto pause = m_initialPause;
        if (pause == -1) {
            pause = CGeneral::GetRandomNumberInRange(0, 2000);
        }
        return new CTaskSimpleStandStill(pause, false, false, 8.0f);
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleDoHandSignal();
    case TASK_SIMPLE_DO_HAND_SIGNAL:
        if (m_areAnimsReferenced && m_playAnimAtEnd) {
            return new CTaskSimpleRunAnim((AssocGroupId)0x34, (AnimationId)(0x117 + CGeneral::GetRandomNumberInRange(-16, 0)), 4.0f, false);
        }
        break;
    default:
        break;
    }
    return nullptr;
}

// 0x660CC0
CTask* CTaskComplexSignalAtPed::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x660CC0, CTaskComplexSignalAtPed*, CPed*>(this, ped);
}

// 0x660D80
CTask* CTaskComplexSignalAtPed::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x660D80, CTaskComplexSignalAtPed*, CPed*>(this, ped);
}
