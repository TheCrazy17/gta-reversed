#include "StdInc.h"

#include "TaskSimpleWaitForPizza.h"

void CTaskSimpleWaitForPizza::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleWaitForPizza, 0x86E1DC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x632B30);
    RH_ScopedInstall(Destructor, 0x632B70);

    RH_ScopedVMTInstall(Clone, 0x636B40);
    RH_ScopedVMTInstall(GetTaskType, 0x632B50);
    RH_ScopedVMTInstall(MakeAbortable, 0x632B60);
    RH_ScopedVMTInstall(ProcessPed, 0x632B80);
}

// 0x632B80
bool CTaskSimpleWaitForPizza::ProcessPed(CPed* ped) {
    if (!m_Timer.IsStarted()) {
        m_Timer.Start(2000);
    }

    return m_Timer.IsOutOfTime();
}
