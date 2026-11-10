#pragma once

#include "TaskSimple.h"
#include "TaskTimer.h"

class CEvent;
class CPed;

/// Does nothing for 2 seconds, then finishes
class NOTSA_EXPORT_VTABLE CTaskSimpleWaitForPizza : public CTaskSimple {
public:
    static constexpr auto Type = TASK_SIMPLE_WAIT_FOR_PIZZA;

    static void InjectHooks();

    CTaskSimpleWaitForPizza() = default;
    ~CTaskSimpleWaitForPizza() override = default;

    CTask*    Clone() const override { return new CTaskSimpleWaitForPizza{}; } // 0x636B40
    eTaskType GetTaskType() const override { return Type; }                    // 0x632B50
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return true; } // 0x632B60
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x632B30
    CTaskSimpleWaitForPizza* Constructor() {
        this->CTaskSimpleWaitForPizza::CTaskSimpleWaitForPizza();
        return this;
    }

    // 0x632B70
    CTaskSimpleWaitForPizza* Destructor() {
        this->CTaskSimpleWaitForPizza::~CTaskSimpleWaitForPizza();
        return this;
    }

private:
    CTaskTimer m_Timer{}; //< Started on the first `ProcessPed`
};
VALIDATE_SIZE(CTaskSimpleWaitForPizza, 0x14);
