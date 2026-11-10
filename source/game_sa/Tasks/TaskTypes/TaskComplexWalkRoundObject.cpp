#include "StdInc.h"

#include "TaskComplexWalkRoundObject.h"
#include "PedGeometryAnalyser.h"
#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexWalkRoundObject::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundObject, 0x86F364, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x655020);
    RH_ScopedInstall(Destructor, 0x6550D0);
    RH_ScopedInstall(CreateRouteTask, 0x655140);
    RH_ScopedInstall(ComputeRoute, 0x6551D0);
    RH_ScopedInstall(CreateSubTask, 0x655290);
    RH_ScopedVMTInstall(Clone, 0x655C10);
    RH_ScopedVMTInstall(GetTaskType, 0x6550C0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x657220);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x657380);
    RH_ScopedVMTInstall(ControlSubTask, 0x6575F0);
}

// 0x655020
CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(int32 moveState, const CVector& targetPoint, CEntity* object) : CTaskComplex() {
    m_moveState   = moveState;
    m_targetPoint = targetPoint;
    m_object      = object;

    CEntity::SafeRegisterRef(m_object);

    m_pointRoute = new CPointRoute();
}

// 0x6550D0
CTaskComplexWalkRoundObject::~CTaskComplexWalkRoundObject() {
    CEntity::SafeCleanUpRef(m_object);

    delete m_pointRoute;
}

// 0x6551D0
float CTaskComplexWalkRoundObject::ComputeRoute(CPed* ped) {
    // Keeps the route further away from the corners of the box than usual
    const auto prevTolerance = CPedGeometryAnalyser::ms_BoundingBoxTolerance;
    CPedGeometryAnalyser::ms_BoundingBoxTolerance = 0.7f;
    CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(*ped, *m_object, m_targetPoint, *m_pointRoute, 0);
    CPedGeometryAnalyser::ms_BoundingBoxTolerance = prevTolerance;

    const auto& pedPos = ped->GetPosition();

    CVector planes[4];
    float   planesDot[4];
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(pedPos.z, *m_object, &planes, planesDot);

    const auto side = CPedGeometryAnalyser::ComputeEntityHitSide(pedPos, *m_object);
    return planes[side].z * pedPos.z + planes[side].y * pedPos.y + planes[side].x * pedPos.x + planesDot[side];
}

// 0x655140
CTask* CTaskComplexWalkRoundObject::CreateRouteTask(CPed* ped) {
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{
        static_cast<eMoveState>(m_moveState),
        *m_pointRoute,
        CTaskComplexFollowPointRoute::Mode::ONE_WAY,
        0.5f,
        0.0f,
        true,
        false,
        false
    };
}

// 0x655290
CTask* CTaskComplexWalkRoundObject::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
    case TASK_COMPLEX_FOLLOW_POINT_ROUTE:
        return CreateRouteTask(ped);
    default: // TASK_FINISHED as well
        return nullptr;
    }
}

// 0x657220
CTask* CTaskComplexWalkRoundObject::CreateNextSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return CreateRouteTask(ped);
    case TASK_SIMPLE_ACHIEVE_HEADING: {
        if (!m_pointRoute->IsEmpty()) {
            return CreateRouteTask(ped);
        }

        // The route is gone, so make one from the closest point of the object when the ped and the target are on the same side of it
        CVector closestPoint;
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_object, closestPoint);

        const auto pedSide    = CPedGeometryAnalyser::ComputeEntityHitSide(closestPoint, *m_object);
        const auto targetSide = CPedGeometryAnalyser::ComputeEntityHitSide(m_targetPoint, *m_object);
        if (pedSide != targetSide) {
            return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, closestPoint, 0.5f, 2.0f, false, false };
        }

        m_pointRoute->AddUnlessFull(closestPoint);
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_targetPoint, *m_object, closestPoint);
        m_pointRoute->AddUnlessFull(closestPoint);
        return CreateRouteTask(ped);
    }
    default: // Done walking round (stand still or the route finished)
        return nullptr;
    }
}

// 0x657380
CTask* CTaskComplexWalkRoundObject::CreateFirstSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    m_pointRoute->Clear();
    const auto distToBoxSide = ComputeRoute(ped);
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }

    m_timer.Start(m_moveState == PEDMOVE_WALK ? 8000 : 4000);

    // ControlSubTask drops the route when the object moves or turns away from this
    m_objectPos     = m_object->GetPosition();
    m_objectForward = m_object->GetMatrix().GetForward();
    m_objectRight   = m_object->GetMatrix().GetRight();

    const auto& firstPoint       = m_pointRoute->m_Entries[0];
    auto        toFirstPoint     = firstPoint - ped->GetPosition();
    const auto  distToFirstPoint = toFirstPoint.NormaliseAndMag();

    // How close the ped has to be before it is worth to go round instead of turning first
    const auto closeDist = [&] {
        switch (m_moveState) {
        case PEDMOVE_WALK: return 2.0f;
        case PEDMOVE_RUN:  return 4.0f;
        default:           return 6.0f;
        }
    }();

    // The exe uses the matrix of the ped without checking it
    const auto& pedFwd           = ped->GetForward();
    const auto  facingFirstPoint = toFirstPoint.z * pedFwd.z + toFirstPoint.y * pedFwd.y + toFirstPoint.x * pedFwd.x;
    if (distToFirstPoint > closeDist && distToBoxSide > closeDist && facingFirstPoint > 0.0f) {
        return CreateRouteTask(ped);
    }

    // Face the first point of the route before walking; toFirstPoint was normalised, so the offset is made again
    const auto offset  = firstPoint - ped->GetPosition();
    const auto heading = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(offset.x, offset.y, 0.0f, 0.0f));
    return new CTaskSimpleAchieveHeading{ heading, 1.0f, 0.2f };
}

// 0x6575F0
CTask* CTaskComplexWalkRoundObject::ControlSubTask(CPed* ped) {
    static auto& s_MaxStaticCounter = StaticRef<int32>(0x86C938);

    const auto IsRouteStillValid = [&] {
        if (!m_object) {
            return false;
        }

        const auto subTaskType = m_pSubTask->GetTaskType();
        if (subTaskType == TASK_COMPLEX_FOLLOW_POINT_ROUTE && m_timer.IsOutOfTime()) {
            return false;
        }
        if (ped->GetIntelligence()->m_AnotherStaticCounter > s_MaxStaticCounter) {
            return false;
        }
        if (subTaskType == TASK_SIMPLE_STAND_STILL) {
            return true;
        }

        // Moving or turning the object by a bit makes the route unusable
        const auto& objectMat = m_object->GetMatrix();
        if ((m_objectPos - m_object->GetPosition()).SquaredMagnitude() > 0.0625f) {
            return false;
        }
        if (DotProduct(m_objectForward, objectMat.GetForward()) < 0.9f) {
            return false;
        }
        return !(DotProduct(m_objectRight, objectMat.GetRight()) < 0.9f); // Not `>=`, a NaN keeps the route like in the exe
    };

    if (!IsRouteStillValid() && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    }
    return m_pSubTask;
}
