#pragma once

#include "TaskSimpleDuck.h"

class CPed;
class CEvent;

/// Standalone duck (cower animation) used as the response to bullets whizzing by the ped
class NOTSA_EXPORT_VTABLE CTaskSimpleDuckWhileShotsWhizzing : public CTaskSimpleDuck {
public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_DUCK_WHILE_SHOTS_WHIZZING;

    static void InjectHooks();

    explicit CTaskSimpleDuckWhileShotsWhizzing(uint16 lengthOfDuck);
    ~CTaskSimpleDuckWhileShotsWhizzing() override = default;

    eTaskType GetTaskType() const override { return Type; }                                             // 0x6926E0
    CTask*    Clone() const override { return new CTaskSimpleDuckWhileShotsWhizzing{m_LengthOfDuck}; } // 0x692D60
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x692680
    CTaskSimpleDuckWhileShotsWhizzing* Constructor(uint16 lengthOfDuck) {
        this->CTaskSimpleDuckWhileShotsWhizzing::CTaskSimpleDuckWhileShotsWhizzing(lengthOfDuck);
        return this;
    }
};
VALIDATE_SIZE(CTaskSimpleDuckWhileShotsWhizzing, 0x28);
