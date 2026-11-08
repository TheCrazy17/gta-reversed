#pragma once

#include "TaskSimple.h"

class CVehicle;

/*!
* Puts a ped out of a vehicle without any animation, used when there is no room to step out
* (CTaskComplexLeaveCar creates it when every door is blocked)
*/
// VMT: 0x86EE94 | Size: 9
class NOTSA_EXPORT_VTABLE CTaskSimpleCarForcePedOut : public CTaskSimple {
public:
    CVehicle* m_pTargetVehicle{};
    int32     m_nTargetDoor{};

public:
    static constexpr auto Type = TASK_SIMPLE_CAR_FORCE_PED_OUT;

    static void InjectHooks();

    CTaskSimpleCarForcePedOut(CVehicle* targetVehicle, int32 nTargetDoor);
    ~CTaskSimpleCarForcePedOut() override;

    eTaskType GetTaskType() const override { return Type; }                                         // 0x647770
    CTask*    Clone() const override { return new CTaskSimpleCarForcePedOut{ m_pTargetVehicle, m_nTargetDoor }; } // 0x649EE0
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return false; } // 0x647780
    bool      ProcessPed(CPed* ped) override;                                                       // 0x6477F0

private: // Wrappers for hooks
    // 0x647710
    CTaskSimpleCarForcePedOut* Constructor(CVehicle* targetVehicle, int32 nTargetDoor) {
        this->CTaskSimpleCarForcePedOut::CTaskSimpleCarForcePedOut(targetVehicle, nTargetDoor);
        return this;
    }

    // 0x647790
    CTaskSimpleCarForcePedOut* Destructor() {
        this->CTaskSimpleCarForcePedOut::~CTaskSimpleCarForcePedOut();
        return this;
    }
};
VALIDATE_SIZE(CTaskSimpleCarForcePedOut, 0x10);
