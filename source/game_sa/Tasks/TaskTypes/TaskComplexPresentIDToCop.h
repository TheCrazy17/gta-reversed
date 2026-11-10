#pragma once

#include "TaskComplex.h"

class CPed;

/// Ped turns towards a cop, pleads and holds the hands up for a moment (to show the ID)
class NOTSA_EXPORT_VTABLE CTaskComplexPresentIDToCop : public CTaskComplex {
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_PRESENT_ID_TO_COP;

    static void InjectHooks();

    explicit CTaskComplexPresentIDToCop(CPed* cop);
    ~CTaskComplexPresentIDToCop() override;

    CTask*    CreateSubTask(eTaskType taskType, CPed* ped);
    float     ComputeHeadingToFaceCop(CPed* ped);

    CTask*    Clone() const override { return new CTaskComplexPresentIDToCop{m_Cop}; } // 0x659B60
    eTaskType GetTaskType() const override { return Type; }                            // 0x659230
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

private: // Wrappers for hooks
    // 0x6591D0
    CTaskComplexPresentIDToCop* Constructor(CPed* cop) {
        this->CTaskComplexPresentIDToCop::CTaskComplexPresentIDToCop(cop);
        return this;
    }

    // 0x659240
    CTaskComplexPresentIDToCop* Destructor() {
        this->CTaskComplexPresentIDToCop::~CTaskComplexPresentIDToCop();
        return this;
    }

private:
    CPed* m_Cop{}; //< The cop the ID is presented to; cleared when the cop is deleted (the task then finishes)
};
VALIDATE_SIZE(CTaskComplexPresentIDToCop, 0x10);
