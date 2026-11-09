#include "StdInc.h"

#include "TaskComplexGoPickUpEntity.h"

void CTaskComplexGoPickUpEntity::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGoPickUpEntity, 0x870B98, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6919C0);
    RH_ScopedInstall(Destructor, 0x691A50);

    RH_ScopedVMTInstall(Clone, 0x692C80);
    RH_ScopedVMTInstall(GetTaskType, 0x691A40);
}

// 0x6919C0
CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(CEntity* entity, AssocGroupId animGroup) :
    CTaskComplex{},
    m_pEntity{entity},
    m_vecPosition{-999.0f, 0.0f, 0.0f},        // 0xC479C000, not set yet
    m_vecPickupPosition{-999.0f, 0.0f, 0.0f},  // 0xC479C000, not set yet
    m_nTimePassedSinceLastSubTaskCreatedInMs{0},
    m_nAnimGroupId{static_cast<int32>(animGroup)},
    m_bAnimBlockReferenced{false}
{
    CEntity::SafeRegisterRef(m_pEntity);
}

CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(const CTaskComplexGoPickUpEntity& o) :
    CTaskComplexGoPickUpEntity{o.m_pEntity, (AssocGroupId)o.m_nAnimGroupId}
{
}

// 0x691A50
CTaskComplexGoPickUpEntity::~CTaskComplexGoPickUpEntity() {
    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bAnimBlockReferenced) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex((AssocGroupId)m_nAnimGroupId));
        m_bAnimBlockReferenced = false;
    }
}

// 0x691AE0
CTask* CTaskComplexGoPickUpEntity::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x691AE0, CTaskComplexGoPickUpEntity*, CPed*>(this, ped);
}

// 0x693610
CTask* CTaskComplexGoPickUpEntity::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x693610, CTaskComplexGoPickUpEntity*, CPed*>(this, ped);
}

// 0x691D50
CTask* CTaskComplexGoPickUpEntity::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x691D50, CTaskComplexGoPickUpEntity*, CPed*>(this, ped);
}
