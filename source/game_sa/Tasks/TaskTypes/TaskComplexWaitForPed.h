#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"

class CPed;
class CEvent;

/// The ped running this task stands still and waits for `m_ped` to catch up. It finishes when `m_ped` is within `m_radius` (of the runner,
/// or of the spot beside the runner that `m_ped`'s walk alongside task aims for), when the timeout ran out, or when `m_ped` died (the runner then gets
/// an investigate dead ped event).
class NOTSA_EXPORT_VTABLE CTaskComplexWaitForPed : public CTaskComplex {
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_WAIT_FOR_PED;

    static void InjectHooks();

    CTaskComplexWaitForPed(CPed* ped, float radius, uint32 timeInMs, bool rotateOtherPedsToWaitingPed);
    ~CTaskComplexWaitForPed() override;

    CTask*    Clone() const override { return new CTaskComplexWaitForPed{m_ped, m_radius, m_timeInMs, m_bRotateOtherPedsToWaitingPed}; } // 0x683950
    eTaskType GetTaskType() const override { return Type; }                                                                              // 0x6833C0
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return true; } // 0x683430
    CTask*    CreateNextSubTask(CPed* ped) override { return nullptr; }                                                                  // 0x683440
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

private: // Wrappers for hooks
    // 0x683340
    CTaskComplexWaitForPed* Constructor(CPed* ped, float radius, uint32 timeInMs, bool rotateOtherPedsToWaitingPed) {
        this->CTaskComplexWaitForPed::CTaskComplexWaitForPed(ped, radius, timeInMs, rotateOtherPedsToWaitingPed);
        return this;
    }

    // 0x6833D0
    CTaskComplexWaitForPed* Destructor() {
        this->CTaskComplexWaitForPed::~CTaskComplexWaitForPed();
        return this;
    }

private:
    CPed*      m_ped{};                            //< The ped that is waited for
    float      m_radius{};                         //< The task finishes once `m_ped` is closer than this
    uint32     m_timeInMs{};                       //< Timeout, started in `CreateFirstSubTask`
    bool       m_bRotateOtherPedsToWaitingPed{};   //< Turn the ped running the task towards `m_ped` (throttled to every 11th frame)
    CTaskTimer m_timer{};
    int32      m_framesToWaitForSettingRotation{}; //< Frame counter (0..10) of the rotation throttling
};
VALIDATE_SIZE(CTaskComplexWaitForPed, 0x2C);
