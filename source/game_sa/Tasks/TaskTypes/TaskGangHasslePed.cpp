#include "SeekEntity/TaskComplexSeekEntityStandard.h"
#include "StdInc.h"

#include "TaskGangHasslePed.h"

void CTaskGangHasslePed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHasslePed, 0x86FA00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FED0);
    RH_ScopedInstall(Destructor, 0x65FF60);

    RH_ScopedVMTInstall(CreateNextSubTask, 0x6642C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x664380);
    RH_ScopedVMTInstall(ControlSubTask, 0x65FFE0, { .reversed = false });
}

// 0x65FED0
CTaskGangHasslePed::CTaskGangHasslePed(CPed* ped, int32 a3, int32 a4, int32 a5) : CTaskComplex() {
    m_nTime = 0;
    m_nSomeRandomShit = 0;
    m_bFirstSubTaskInitialised = 0;
    m_bRefreshTime = 0;
    dword10 = a3;
    m_RndMin = a4;
    m_Ped = ped;
    m_RndMax = a5;
    m_bAnimBlockRefAdded = false;
    CEntity::SafeRegisterRef(m_Ped);
}

// 0x65FF60
CTaskGangHasslePed::~CTaskGangHasslePed() {
    CEntity::SafeCleanUpRef(m_Ped);

    if (m_bAnimBlockRefAdded) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bAnimBlockRefAdded = false;
    }
}

namespace {
auto& s_HassleMaxEntityDistMult = StaticRef<float>(0x86FC2C);
auto& s_HassleMoveStateRadius   = StaticRef<float>(0x86FC28);
auto& s_HassleFollowNodeThresholdHeightChange = StaticRef<float>(0x86FC30);
}; // namespace

// 0x6642C0
CTask* CTaskGangHasslePed::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_KILL_PED_ON_FOOT) {
        return nullptr;
    }

    const auto radius = CGeneral::GetRandomNumberInRange(3.0f, 5.0f); // strange math here
    return new CTaskComplexSeekEntityStandard(m_Ped, 999'999, 1000, s_HassleMaxEntityDistMult * radius, s_HassleMoveStateRadius, s_HassleFollowNodeThresholdHeightChange, false, true);
}

// 0x664380
CTask* CTaskGangHasslePed::CreateFirstSubTask(CPed* ped) {
    if (!m_Ped) {
        return nullptr;
    }

    const auto radius = CGeneral::GetRandomNumberInRange(3.0f, 5.0f); // strange math here
    m_nTime = CTimer::GetTimeInMS();
    m_nSomeRandomShit = CGeneral::GetRandomNumberInRange(m_RndMin, m_RndMax);
    m_bFirstSubTaskInitialised = true;

    return new CTaskComplexSeekEntityStandard(m_Ped, 999'999, 1000, s_HassleMaxEntityDistMult * radius, s_HassleMoveStateRadius, s_HassleFollowNodeThresholdHeightChange, false, true);
}

// 0x65FFE0
CTask* CTaskGangHasslePed::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x65FFE0, CTaskGangHasslePed*, CPed*>(this, ped);
}
