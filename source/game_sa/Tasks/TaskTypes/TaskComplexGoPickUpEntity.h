#pragma once

#include "TaskComplex.h"

class CEntity;

class NOTSA_EXPORT_VTABLE CTaskComplexGoPickUpEntity : public CTaskComplex {
public:
    static constexpr auto Type = TASK_COMPLEX_GO_PICKUP_ENTITY;

    static void InjectHooks();

    CTaskComplexGoPickUpEntity(CEntity* entity, AssocGroupId animGroup);
    CTaskComplexGoPickUpEntity(const CTaskComplexGoPickUpEntity&);
    ~CTaskComplexGoPickUpEntity();

    CTask*    Clone() const override { return new CTaskComplexGoPickUpEntity{*this}; } // 0x692C80
    eTaskType GetTaskType() const override { return Type; }                            // 0x691A40

    // Not reversed yet, these forward to the original code
    CTask* CreateNextSubTask(CPed* ped) override;  // 0x691AE0
    CTask* CreateFirstSubTask(CPed* ped) override; // 0x693610
    CTask* ControlSubTask(CPed* ped) override;     // 0x691D50

private: // Wrappers for hooks
    // 0x6919C0
    CTaskComplexGoPickUpEntity* Constructor(CEntity* entity, AssocGroupId animGroup) {
        this->CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(entity, animGroup);
        return this;
    }

    // 0x691A50
    CTaskComplexGoPickUpEntity* Destructor() {
        this->CTaskComplexGoPickUpEntity::~CTaskComplexGoPickUpEntity();
        return this;
    }

public:
    CEntity* m_pEntity;
    CVector  m_vecPosition;
    CVector  m_vecPickupPosition;
    uint32   m_nTimePassedSinceLastSubTaskCreatedInMs;
    int32    m_nAnimGroupId;
    bool     m_bAnimBlockReferenced;
    char     _pad[3];
};

VALIDATE_SIZE(CTaskComplexGoPickUpEntity, 0x34);
