#include "StdInc.h"
#include "EventInWater.h"

void CEventInWater::InjectHooks() {
    RH_ScopedVirtualClass(CEventInWater, 0x85B4C8, 16);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4B1370);
    RH_ScopedVMTInstall(Clone, 0x4B7810);
    RH_ScopedVMTInstall(AffectsPed, 0x4B13D0);
    RH_ScopedVMTInstall(TakesPriorityOver, 0x4B1420);
    RH_ScopedVMTInstall(GetEventType, 0x4B1390);
    RH_ScopedVMTInstall(GetEventPriority, 0x4B13B0);
    RH_ScopedVMTInstall(GetLifeTime, 0x4B13A0);
}



CEventInWater::CEventInWater(float acceleration) {
    m_acceleration = acceleration;
}

bool CEventInWater::AffectsPed(CPed* ped) {
    CTask* task = ped->GetTaskManager().GetActiveTask();
    if (!ped->IsPlayer() && task && task->GetTaskType() == TASK_COMPLEX_IN_WATER)
        return false;
    return ped->IsAlive();
}

// 0x4B1420
bool CEventInWater::TakesPriorityOver(const CEvent& refEvent) {
    // Only being stuck in the air depends on the acceleration; a knock off bike or a damage event always loses
    // (0x4B142F, 0x4B143B). The exe also compares the priorities itself in the end instead of calling the base function.
    // This used to ask for an acceleration above 1 for all three
    switch (refEvent.GetEventType()) {
    case EVENT_KNOCK_OFF_BIKE:
    case EVENT_DAMAGE:
        return true;
    case EVENT_STUCK_IN_AIR:
        if (m_acceleration > 1.0f) {
            return true;
        }
        break;
    }
    return GetEventPriority() >= refEvent.GetEventPriority();
}

