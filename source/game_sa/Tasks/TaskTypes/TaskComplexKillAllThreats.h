#pragma once

#include "TaskComplex.h"

class CPed;

/// Attacks the threats around the ped one after the other and stands still once none is left
class NOTSA_EXPORT_VTABLE CTaskComplexKillAllThreats : public CTaskComplex {
public:
    static constexpr auto Type = TASK_KILL_ALL_THREATS;

    static void InjectHooks();

    /// The three values are handed to every CTaskComplexKillPedOnFoot this task starts
    CTaskComplexKillAllThreats(int32 pedFlags, int32 actionDelay, int32 actionChance);
    ~CTaskComplexKillAllThreats() override = default;

    CTask*    Clone() const override { return new CTaskComplexKillAllThreats{ m_PedFlags, m_ActionDelay, m_ActionChance }; } // 0x623750
    eTaskType GetTaskType() const override { return Type; }                                                                   // 0x6221B0
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed*) override { return m_pSubTask; } // 0x622230

    /// The ped to attack next, nullptr when no threat is left
    CPed* FindNextThreat(CPed* ped);

private: // Wrappers for hooks
    // 0x622180
    CTaskComplexKillAllThreats* Constructor(int32 pedFlags, int32 actionDelay, int32 actionChance) {
        this->CTaskComplexKillAllThreats::CTaskComplexKillAllThreats(pedFlags, actionDelay, actionChance);
        return this;
    }

    // 0x6221C0
    CTaskComplexKillAllThreats* Destructor() {
        this->CTaskComplexKillAllThreats::~CTaskComplexKillAllThreats();
        return this;
    }

private:
    CTask* CreateKillTask(CPed* target) const;

private:
    int32 m_PedFlags{};
    int32 m_ActionDelay{};
    int32 m_ActionChance{};
};
VALIDATE_SIZE(CTaskComplexKillAllThreats, 0x18);
