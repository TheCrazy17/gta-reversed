#pragma once

#include "TaskComplex.h"

class CPed;
class CEvent;

/// Ped falls and then stays down (the fall task is created with a practically endless down time)
class NOTSA_EXPORT_VTABLE CTaskComplexFallAndStayDown : public CTaskComplex {
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_FALL_AND_STAY_DOWN;

    static void InjectHooks();

    CTaskComplexFallAndStayDown(AnimationId fallAnimId, AssocGroupId fallAnimGroup);
    explicit CTaskComplexFallAndStayDown(int32 dir);
    ~CTaskComplexFallAndStayDown() override = default;

    CTask*    CreateSubTask(eTaskType taskType);

    CTask*    Clone() const override { return new CTaskComplexFallAndStayDown{m_FallAnimId, m_FallAnimGroup}; } // 0x67C270
    eTaskType GetTaskType() const override { return Type; }                                                      // 0x678A00
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override { return m_pSubTask; } // 0x678BC0

private: // Wrappers for hooks
    // 0x6789D0
    CTaskComplexFallAndStayDown* Constructor(AnimationId fallAnimId, AssocGroupId fallAnimGroup) {
        this->CTaskComplexFallAndStayDown::CTaskComplexFallAndStayDown(fallAnimId, fallAnimGroup);
        return this;
    }

    // 0x678A10
    CTaskComplexFallAndStayDown* Constructor2(int32 dir) {
        this->CTaskComplexFallAndStayDown::CTaskComplexFallAndStayDown(dir);
        return this;
    }

private:
    AnimationId  m_FallAnimId{};
    AssocGroupId m_FallAnimGroup{};
};
VALIDATE_SIZE(CTaskComplexFallAndStayDown, 0x14);
