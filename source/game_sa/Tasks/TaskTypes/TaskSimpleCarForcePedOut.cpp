#include "StdInc.h"

#include "TaskSimpleCarForcePedOut.h"
#include "CarEnterExit.h"

void CTaskSimpleCarForcePedOut::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarForcePedOut, 0x86EE94, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x647710);
    RH_ScopedInstall(Destructor, 0x647790);
    RH_ScopedVMTInstall(GetTaskType, 0x647770);
    RH_ScopedVMTInstall(Clone, 0x649EE0);
    RH_ScopedVMTInstall(MakeAbortable, 0x647780);
    RH_ScopedVMTInstall(ProcessPed, 0x6477F0);
}

// 0x647710
CTaskSimpleCarForcePedOut::CTaskSimpleCarForcePedOut(CVehicle* targetVehicle, int32 nTargetDoor) :
    m_pTargetVehicle{ targetVehicle },
    m_nTargetDoor{ nTargetDoor }
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// 0x647790
CTaskSimpleCarForcePedOut::~CTaskSimpleCarForcePedOut() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x6477F0
bool CTaskSimpleCarForcePedOut::ProcessPed(CPed* ped) {
    if (!m_pTargetVehicle) {
        return true;
    }

    if (!m_pTargetVehicle->IsDriver(ped)) {
        // Not yet while someone leaves through the opposite door and the driver or a passenger of an earlier seat is still inside
        if (CCarEnterExit::ComputeOppositeDoorFlag(m_pTargetVehicle, m_nTargetDoor, false) & m_pTargetVehicle->m_nGettingOutFlags) {
            if (m_pTargetVehicle->m_pDriver) {
                return false;
            }

            const int32 seatCount = CCarEnterExit::ComputePassengerIndexFromCarDoor(m_pTargetVehicle, m_nTargetDoor);
            for (int32 seat = 0; seat < seatCount; seat++) {
                if (m_pTargetVehicle->m_apPassengers[seat]) {
                    return false;
                }
            }
        }
    }

    ped->PositionPedOutOfCollision(m_nTargetDoor, m_pTargetVehicle, true);
    return true;
}
