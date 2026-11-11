#include "StdInc.h"

#include "TaskSimpleCarDriveTimed.h"

void CTaskSimpleCarDriveTimed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarDriveTimed, 0x859E50, 9);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedVMTInstall(ProcessPed, 0x46F610);
    RH_ScopedVMTInstall(GetTaskType, 0x46F600);
}

CTaskSimpleCarDriveTimed* CTaskSimpleCarDriveTimed::Constructor(CVehicle* vehicle, int32 nTime) {
    this->CTaskSimpleCarDriveTimed::CTaskSimpleCarDriveTimed(vehicle, nTime);
    return this;
}

// 0x5FF940
CTaskSimpleCarDriveTimed::CTaskSimpleCarDriveTimed(CVehicle* vehicle, int32 nTime) : CTaskSimpleCarDrive(vehicle, nullptr, false), m_nTimer() {
    m_nTime = nTime;
}

// 0x46F610
bool CTaskSimpleCarDriveTimed::ProcessPed(CPed* ped) {
    // The timer starts with the first tick, even for the default time of -1 (which is out of time at once)
    m_nTimer.StartIfNotAlready(m_nTime);
    if (m_nTimer.IsOutOfTime()) {
        return true;
    }
    return CTaskSimpleCarDrive::ProcessPed(ped);
}
