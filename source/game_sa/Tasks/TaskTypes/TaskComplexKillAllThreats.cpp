#include "StdInc.h"

#include "TaskComplexKillAllThreats.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskSimpleStandStill.h"
#include "PedIntelligence.h"
#include "Events/EventScanner.h"

void CTaskComplexKillAllThreats::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillAllThreats, 0x86DA1C, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x622180);
    RH_ScopedInstall(Destructor, 0x6221C0);
    RH_ScopedInstall(FindNextThreat, 0x6221D0);
    RH_ScopedVMTInstall(Clone, 0x623750);
    RH_ScopedVMTInstall(GetTaskType, 0x6221B0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6293C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x629480);
    RH_ScopedVMTInstall(ControlSubTask, 0x622230);
}

// 0x622180
CTaskComplexKillAllThreats::CTaskComplexKillAllThreats(int32 pedFlags, int32 actionDelay, int32 actionChance) :
    CTaskComplex(),
    m_PedFlags{ pedFlags },
    m_ActionDelay{ actionDelay },
    m_ActionChance{ actionChance }
{
}

// 0x6221D0
CPed* CTaskComplexKillAllThreats::FindNextThreat(CPed* ped) {
    // The exe keeps the found ped and a zeroed scanner on its stack and hands both to the scan at 0x607A90, a method of the scanner.
    // The scan looks at the 16 peds around the ped, from the strongest acquaintance type (4) down, and stores the ped it settles on
    CPed*                   found{};
    CPedAcquaintanceScanner scanner{};
    CPed* const             pedArg = ped; // The scan wants the address of a CPed*, the exe passes the address of its own copy of the parameter
    plugin::CallMethod<0x607A90, CPedAcquaintanceScanner*, CPed*, int32, CEntity**, int32, CPed**, CPed* const*>(
        &scanner,
        ped,
        4,
        ped->GetIntelligence()->GetPedEntities(),
        16,
        &found,
        &pedArg
    );
    return found;
}

// Both sub task creators start the same attack, the target is the only thing that differs
CTask* CTaskComplexKillAllThreats::CreateKillTask(CPed* target) const {
    return new CTaskComplexKillPedOnFoot{ target, -1, m_PedFlags, m_ActionDelay, m_ActionChance, 1 };
}

// 0x6293C0
CTask* CTaskComplexKillAllThreats::CreateNextSubTask(CPed* ped) {
    // Nothing follows the stand still; after an attack the next threat is picked, and the task ends when there is none
    if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) {
        return nullptr;
    }
    if (auto* const target = FindNextThreat(ped)) {
        return CreateKillTask(target);
    }
    return nullptr;
}

// 0x629480
CTask* CTaskComplexKillAllThreats::CreateFirstSubTask(CPed* ped) {
    if (auto* const target = FindNextThreat(ped)) {
        return CreateKillTask(target);
    }
    return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
}
