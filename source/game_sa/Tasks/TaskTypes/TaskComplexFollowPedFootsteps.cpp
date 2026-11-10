#include "StdInc.h"

#include "TaskComplexFollowPedFootsteps.h"
#include "PedGeometryAnalyser.h" // CPointRoute
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleHitHead.h"
#include "TaskSimpleStandStill.h"
#include "SeekEntity/TaskComplexSeekEntityStandard.h"

void CTaskComplexFollowPedFootsteps::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowPedFootsteps, 0x870CC0, 12);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x694E20);
    RH_ScopedInstall(Destructor, 0x695DC0);
    RH_ScopedVMTInstall(Clone, 0x695890);
    RH_ScopedVMTInstall(GetTaskType, 0x694EC0);
    RH_ScopedVMTInstall(MakeAbortable, 0x694ED0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x694EE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x695000);
    RH_ScopedVMTInstall(ControlSubTask, 0x695090);
    RH_ScopedVMTInstall(CreateSubTask, 0x695E40);
}

// 0x694E20
CTaskComplexFollowPedFootsteps::CTaskComplexFollowPedFootsteps(CPed* ped) : CTaskComplex() {
    m_targetPed               = ped;
    m_updateGoToPoint         = false;
    m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();
    m_lineOfSightCheckTimer   = 0;
    m_pointRoute              = nullptr;
    m_moveState               = PEDMOVE_WALK;

    CEntity::SafeRegisterRef(m_targetPed);

    m_pointRoute = new CPointRoute();
}

// 0x695DC0
CTaskComplexFollowPedFootsteps::~CTaskComplexFollowPedFootsteps() {
    CEntity::SafeCleanUpRef(m_targetPed);

    delete m_pointRoute;
    m_pointRoute = nullptr;
}

// 0x694ED0
bool CTaskComplexFollowPedFootsteps::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    // The exe forwards without a null check; the task always has a sub task while it runs.
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x694EE0
CTask* CTaskComplexFollowPedFootsteps::CreateNextSubTask(CPed* ped) {
    if (!m_targetPed) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_NONE:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_SIMPLE_HIT_HEAD: // Made by ControlSubTask when there are too many followers
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    case TASK_SIMPLE_STAND_STILL:
    case TASK_SIMPLE_GO_TO_POINT:
        if (!m_pointRoute || m_pointRoute->IsEmpty()) {
            return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
        }
        return CreateSubTask(TASK_SIMPLE_GO_TO_POINT, ped);
    case TASK_COMPLEX_SEEK_ENTITY: {
        // The exe compares the squared distance with 1.4, it is not squared again
        const auto distSq = (m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude();
        return CreateSubTask(distSq > 1.4f ? TASK_COMPLEX_SEEK_ENTITY : TASK_SIMPLE_STAND_STILL, ped);
    }
    default:
        return CreateSubTask(TASK_FINISHED, ped);
    }
}

// 0x695000
CTask* CTaskComplexFollowPedFootsteps::CreateFirstSubTask(CPed* ped) {
    if (!m_targetPed) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    const auto distSq = (m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude();
    return CreateSubTask(distSq > 1.4f ? TASK_COMPLEX_SEEK_ENTITY : TASK_SIMPLE_STAND_STILL, ped);
}

// 0x695090
CTask* CTaskComplexFollowPedFootsteps::ControlSubTask(CPed* ped) {
    static auto& s_MaxStaticCounter = StaticRef<int32>(0x86C938);

    auto* const subTask = m_pSubTask;

    // If the sub task cannot be aborted yet it keeps running
    const auto SwitchSubTask = [&](eTaskType taskType) -> CTask* {
        if (subTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return CreateSubTask(taskType, ped);
        }
        return subTask;
    };

    if (!m_targetPed || !m_pointRoute || !m_targetPed->IsAlive()) {
        return SwitchSubTask(TASK_FINISHED);
    }

    // Closing the distance is up to the seek task until it finishes
    const auto subTaskType = subTask->GetTaskType();
    if (subTaskType == TASK_COMPLEX_SEEK_ENTITY) {
        return subTask;
    }

    if (m_targetPed->bInVehicle) {
        return SwitchSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (ped->GetIntelligence()->m_AnotherStaticCounter > s_MaxStaticCounter) {
        return SwitchSubTask(TASK_SIMPLE_HIT_HEAD);
    }

    // With a clear line of sight the ped walks straight to the target instead of following its footsteps
    if (CTimer::GetTimeInMS() - m_lineOfSightCheckTimer > 500) {
        m_lineOfSightCheckTimer = CTimer::GetTimeInMS();
        m_updateGoToPoint       = CWorld::GetIsLineOfSightClear(ped->GetPosition(), m_targetPed->GetPosition(), true, false, false, true, false, false, false);
    }

    const auto targetPos = m_targetPed->GetPosition();
    const auto toTarget  = targetPos - ped->GetPosition();
    const auto distSq    = toTarget.SquaredMagnitude();
    const auto dist2D    = std::sqrt(toTarget.y * toTarget.y + toTarget.x * toTarget.x);

    // Close enough to stop, unless it is already standing still (that case is checked below as well)
    if (distSq < sq(1.4f) && subTaskType != TASK_SIMPLE_STAND_STILL) {
        if (!m_updateGoToPoint) {
            return subTask;
        }
        m_pointRoute->Clear();
        return SwitchSubTask(TASK_SIMPLE_STAND_STILL);
    }

    // Too far away, or on another level
    if (distSq > sq(8.0f) || (dist2D < 1.0f && std::abs(toTarget.z) > 2.0f)) {
        return SwitchSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (m_updateGoToPoint) {
        m_pointRoute->Clear();
        m_pointRoute->AddUnlessFull(m_targetPed->GetPosition());

        // Makes the footstep route update below run right away
        m_subTaskCreateCheckTimer = CTimer::GetTimeInMS() - 332;

        if (CTask::IsGoToTask(ped->GetTaskManager().GetSimplestActiveTask())) {
            static_cast<CTaskSimpleGoToPoint*>(ped->GetTaskManager().GetSimplestActiveTask())->UpdatePoint(m_targetPed->GetPosition(), 0.5f, false);
        }
    }

    if (CTimer::GetTimeInMS() - m_subTaskCreateCheckTimer >= 166 && (int32)m_pointRoute->m_NumEntries < 8) {
        m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();

        auto& route = *m_pointRoute;
        if (route.m_NumEntries == 0) {
            route.AddUnlessFull(targetPos);
        } else {
            const auto lastPoint = route.m_Entries[route.m_NumEntries - 1];
            auto       toNew     = targetPos - lastPoint;

            // Record a new footstep once the target moved far enough from the last one
            if (toNew.SquaredMagnitude() > sq(0.35f)) {
                if ((int32)route.m_NumEntries >= 2) {
                    // Keep going the same way, so the last point is replaced instead of making a bend
                    auto lastDir = lastPoint - route.m_Entries[route.m_NumEntries - 2];
                    lastDir.Normalise();
                    toNew.Normalise();
                    if (!(lastDir.y * toNew.y + lastDir.x * toNew.x + lastDir.z * toNew.z < 0.95f)) {
                        route.m_NumEntries--;
                    }
                }
                route.AddUnlessFull(targetPos);
            }
        }

        // The sum is of squared distances to the first point, and not of the lengths of the segments, like in the exe
        float sum = 0.0f;
        for (uint32 i = 1; i < route.m_NumEntries; i++) {
            sum = (route.m_Entries[i] - route.m_Entries[0]).SquaredMagnitude() + sum;
        }
        sum = (ped->GetPosition() - m_targetPed->GetPosition()).SquaredMagnitude() + sum;

        m_moveState = sum > sq(3.5f) ? PEDMOVE_RUN : PEDMOVE_WALK;

        if (subTask->GetTaskType() == TASK_SIMPLE_GO_TO_POINT) {
            static_cast<CTaskSimpleGoToPoint*>(subTask)->m_moveState = (eMoveState)m_moveState;
        }
        return subTask;
    }

    if (subTaskType == TASK_SIMPLE_STAND_STILL && (int32)m_pointRoute->m_NumEntries > 1) {
        return SwitchSubTask(TASK_SIMPLE_GO_TO_POINT);
    }
    return subTask;
}

// 0x695E40
CTask* CTaskComplexFollowPedFootsteps::CreateSubTask(eTaskType taskType, CPed* ped) {
    if (!m_targetPed || !m_pointRoute) {
        return nullptr;
    }

    switch (taskType) {
    case TASK_SIMPLE_HIT_HEAD:
        return new CTaskSimpleHitHead{};
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 10'000, false, false, 8.0f };
    case TASK_SIMPLE_GO_TO_POINT: {
        // Takes the oldest footstep off the route (0x5F13A0 shifts the other points down)
        auto&      route = *m_pointRoute;
        const auto point = route.m_Entries[0];
        for (uint32 i = 0; i + 1 < route.m_NumEntries; i++) {
            route.m_Entries[i] = route.m_Entries[i + 1];
        }
        route.m_NumEntries--;
        return new CTaskSimpleGoToPoint{ (eMoveState)m_moveState, point, 0.01f, false, false };
    }
    case TASK_COMPLEX_SEEK_ENTITY: {
        // Gives the car more room when the target is in a vehicle
        const auto maxDist2D = m_targetPed->bInVehicle ? 1.0f * 4.0f : 1.0f;
        return new CTaskComplexSeekEntityStandard{ m_targetPed, 50'000, 1'000, maxDist2D, 2.0f, 2.0f, true, true };
    }
    default: // TASK_FINISHED
        return nullptr;
    }
}
