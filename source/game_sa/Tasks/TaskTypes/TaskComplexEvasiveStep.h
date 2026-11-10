#pragma once

#include "TaskComplex.h"
class CEntity;

class NOTSA_EXPORT_VTABLE CTaskComplexEvasiveStep : public CTaskComplex {
public:
    CVector  m_Pos;
    CEntity* m_Entity;

public:
    static constexpr auto Type = TASK_COMPLEX_EVASIVE_STEP;

    static void InjectHooks();

    CTaskComplexEvasiveStep(CEntity* entity, const CVector& pos);
    ~CTaskComplexEvasiveStep() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask* Clone() const override { return new CTaskComplexEvasiveStep(m_Entity, m_Pos); }
    CTask* CreateNextSubTask(CPed* ped) override;
    CTask* CreateFirstSubTask(CPed* ped) override;
    CTask* ControlSubTask(CPed* ped) override { return m_pSubTask; } // 0x653550

    CTask* CreateSubTask(eTaskType taskType);

private: // Wrappers for hooks
    // 0x6532D0
    CTaskComplexEvasiveStep* Constructor(CEntity* entity, const CVector& pos) {
        this->CTaskComplexEvasiveStep::CTaskComplexEvasiveStep(entity, pos);
        return this;
    }

    // 0x653350
    CTaskComplexEvasiveStep* Destructor() {
        this->CTaskComplexEvasiveStep::~CTaskComplexEvasiveStep();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexEvasiveStep, 0x1C);
