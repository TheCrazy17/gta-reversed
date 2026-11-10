#include "StdInc.h"

#include "TaskComplexWaitForPed.h"
#include "TaskSimpleStandStill.h"
#include "Events/EventDeadPed.h"

void CTaskComplexWaitForPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWaitForPed, 0x8706F8, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x683340);
    RH_ScopedInstall(Destructor, 0x6833D0);

    RH_ScopedVMTInstall(Clone, 0x683950);
    RH_ScopedVMTInstall(GetTaskType, 0x6833C0);
    RH_ScopedVMTInstall(MakeAbortable, 0x683430);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683440);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x683450);
    RH_ScopedVMTInstall(ControlSubTask, 0x6834E0);
}

// 0x683340
CTaskComplexWaitForPed::CTaskComplexWaitForPed(CPed* ped, float radius, uint32 timeInMs, bool rotateOtherPedsToWaitingPed) :
    m_ped{ped},
    m_radius{radius},
    m_timeInMs{timeInMs},
    m_bRotateOtherPedsToWaitingPed{rotateOtherPedsToWaitingPed}
{
    CEntity::SafeRegisterRef(m_ped); // The exe does not test for null here
}

// 0x6833D0
CTaskComplexWaitForPed::~CTaskComplexWaitForPed() {
    CEntity::SafeCleanUpRef(m_ped);
}

// 0x683450
CTask* CTaskComplexWaitForPed::CreateFirstSubTask(CPed* ped) {
    if (!m_ped) {
        return nullptr;
    }

    // Not `CTaskTimer::Start`: the exe sets the fields directly, so the timer also starts for a timeout >= 2^31 (negative interval)
    m_timer.m_nStartTime = CTimer::GetTimeInMS();
    m_timer.m_nInterval  = (int32)m_timeInMs;
    m_timer.m_bStarted   = true;

    m_framesToWaitForSettingRotation = 0;

    return new CTaskSimpleStandStill{999'999, false, false, 8.0f};
}

// 0x6834E0
CTask* CTaskComplexWaitForPed::ControlSubTask(CPed* ped) {
    if (!m_ped) {
        return nullptr;
    }

    if (!m_ped->IsAlive()) {
        // Let the waiting ped investigate the corpse of the ped it was waiting for
        CEventDeadPed event{m_ped, false, CTimer::GetTimeInMS()};
        event.m_TaskId = TASK_COMPLEX_INVESTIGATE_DEAD_PED;
        ped->GetEventGroup().Add(&event, false);
        return nullptr;
    }

    if (m_timer.IsOutOfTime()) {
        return nullptr;
    }

    // If `m_ped` is walking alongside someone, measure from the spot it is heading for (relative to `ped`) instead of from `ped`
    const auto walkAlongside = m_ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WALK_ALONGSIDE_PED);
    const auto toTargetPos   = [&] {
        if (walkAlongside) {
            // `CTaskComplexWalkAlongsidePed::m_Offset` is private; it is the CVector at +0x14
            const auto& offset = *reinterpret_cast<const CVector*>(reinterpret_cast<const uint8*>(walkAlongside) + 0x14);
            return ped->GetMatrix().TransformPoint(offset) - m_ped->GetPosition();
        }
        return ped->GetPosition() - m_ped->GetPosition();
    }();

    if (toTargetPos.SquaredMagnitude() < sq(m_radius)) { // Close enough
        return nullptr;
    }

    if (m_bRotateOtherPedsToWaitingPed && m_framesToWaitForSettingRotation == 0) {
        const auto pedToWaitedFor = m_ped->GetPosition() - ped->GetPosition();
        ped->m_fAimingRotation    = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(pedToWaitedFor.x, pedToWaitedFor.y, 0.0f, 0.0f));
    }

    // Only every 11th frame
    if (++m_framesToWaitForSettingRotation > 10) {
        m_framesToWaitForSettingRotation = 0;
    }

    return m_pSubTask;
}
