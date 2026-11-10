#pragma once

#include "TaskSimpleStandStill.h"

class CPed;

/// Ped stands on an escalator step; the ped is turned into the direction the step moves
class NOTSA_EXPORT_VTABLE CTaskSimpleOnEscalator : public CTaskSimpleStandStill {
public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_ON_ESCALATOR;

    static void InjectHooks();

    CTaskSimpleOnEscalator();
    ~CTaskSimpleOnEscalator() override = default;

    eTaskType GetTaskType() const override { return Type; }                // 0x4B8720
    CTask*    Clone() const override { return new CTaskSimpleOnEscalator{}; } // 0x4B86C0
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x4B8670
    CTaskSimpleOnEscalator* Constructor() {
        this->CTaskSimpleOnEscalator::CTaskSimpleOnEscalator();
        return this;
    }
};
VALIDATE_SIZE(CTaskSimpleOnEscalator, 0x20);
