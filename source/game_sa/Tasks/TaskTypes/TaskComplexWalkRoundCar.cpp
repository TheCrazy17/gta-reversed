#include "StdInc.h"

#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexWalkRoundCar.h"
#include "TaskComplexEnterCar.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "PedGeometryAnalyser.h"
#include "CarEnterExit.h"

void CTaskComplexWalkRoundCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundCar, 0x86f308, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6541B0);
    RH_ScopedInstall(Destructor, 0x656B00);

    RH_ScopedInstall(SetNewVehicle, 0x654290);
    RH_ScopedInstall(CreateRouteTask, 0x6542E0);
    RH_ScopedInstall(ComputeRouteRoundSmallCar, 0x6544F0);
    RH_ScopedInstall(GoingForDoor, 0x654720);
    RH_ScopedInstall(ComputeRouteRoundBigCar, 0x656BB0);
    RH_ScopedInstall(ComputeRoute, 0x657B80);

    RH_ScopedVMTInstall(Clone, 0x655B00);
    RH_ScopedVMTInstall(GetTaskType, 0x654280);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x656B70);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x658200);
    RH_ScopedVMTInstall(ControlSubTask, 0x654370);
}

// 0x6541B0
CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(eMoveState moveState, CVector const& targetPt, CVehicle* vehicle, bool isPedGoingForCarDoor, uint8 forceThisDirectionRoundCar) :
    m_MoveState{(uint8)moveState},
    m_bIsPedGoingForCarDoor{isPedGoingForCarDoor},
    m_ForceThisDirectionRoundCar{forceThisDirectionRoundCar},
    m_TargetPt{targetPt},
    m_Veh{vehicle},
    m_Route{new CPointRoute{}}
{
    CEntity::SafeRegisterRef(m_Veh);
}

CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(const CTaskComplexWalkRoundCar& o) :
    CTaskComplexWalkRoundCar{
        (eMoveState)o.m_MoveState,
        o.m_TargetPt,
        o.m_Veh,
        o.m_bIsPedGoingForCarDoor,
        o.m_ForceThisDirectionRoundCar
    }
{
}

// 0x656B00
CTaskComplexWalkRoundCar::~CTaskComplexWalkRoundCar() {
    CEntity::SafeCleanUpRef(m_Veh);
    delete m_Route;
}


// 0x654290
void CTaskComplexWalkRoundCar::SetNewVehicle(CVehicle * vehicle, uint8 forceThisDirectionRoundCar) {
    if (notsa::IsFixBugs()) {
        if (vehicle == m_Veh) {
            return;
        }
    }
    CEntity::ChangeEntityReference(m_Veh, vehicle);
    m_ForceThisDirectionRoundCar    = forceThisDirectionRoundCar;
    m_bFirstSubTaskNeedsToBeCreated = true;
    m_Route->Clear();
}

// 0x6542E0
CTask* CTaskComplexWalkRoundCar::CreateRouteTask(CPed*) const {
    if (m_Route->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{ (eMoveState)m_MoveState, *m_Route };
}

// 0x6544F0
float CTaskComplexWalkRoundCar::ComputeRouteRoundSmallCar(CPed* ped) {
        const auto& pedPos = ped->GetPosition();

    // The corners are only computed so the planes below can be taken from the cache
    CVector corners[4];
    CPedGeometryAnalyser::ComputeEntityBoundingBoxCorners(pedPos.z, *m_Veh, corners);

    CVector planes[4];
    float   planeDots[4];
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(pedPos.z, *m_Veh, &planes, planeDots);

    // How far the ped is from the side of the vehicle that was hit
    const auto hitSide = CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_Veh);
    const auto distToSide = hitSide == -1
        ? 0.0f
        : DotProduct(planes[hitSide], pedPos) + planeDots[hitSide];

    // When going for the door the target must not be inside the vehicle, push it out through the closest side
    if (m_bIsPedGoingForCarDoor) {
        float distToPlane[4];
        auto  i = 0;
        for (; i < 4; i++) {
            distToPlane[i] = DotProduct(planes[i], m_TargetPt) + planeDots[i];
            if (distToPlane[i] >= 0.0f) {
                break;
            }
        }
        if (i == 4) {
            if (const auto side = CPedGeometryAnalyser::ComputeEntityHitSide(m_TargetPt, *m_Veh); side != -1) {
                m_TargetPt += planes[side] * (-distToPlane[side] + 0.05f);
            }
        }
    }

    // The forced direction is a signed 4 bit value
    const auto forcedDir = (int32)(int8)(m_ForceThisDirectionRoundCar << 4) >> 4;
    const auto dir       = CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(*ped, *m_Veh, m_TargetPt, *m_Route, forcedDir);
    m_DirectionGoingRoundCar = (uint8)dir & 0xF;

    return distToSide;
}

// 0x656BB0
float CTaskComplexWalkRoundCar::ComputeRouteRoundBigCar(CPed* ped) {
    using PGA = CPedGeometryAnalyser;

    // The closest points are looked for with a wider tolerance than usual
    const auto ClosestSurfacePoints = [&](CVector& pedPoint, CVector& targetPoint) {
        const auto saved = PGA::ms_BoundingBoxTolerance;
        PGA::ms_BoundingBoxTolerance = 0.7f;
        PGA::ComputeClosestSurfacePoint(*ped, *m_Veh, pedPoint);
        PGA::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, targetPoint);
        PGA::ms_BoundingBoxTolerance = saved;
    };

    const auto pedSide    = PGA::ComputeEntityHitSide(*ped, *m_Veh);
    const auto targetSide = PGA::ComputeEntityHitSide(m_TargetPt, *m_Veh);

    CVector lastPoint;
    if (pedSide == targetSide) {
        // Both are on the same side, so it's a straight line along it
        CVector pedPoint;
        ClosestSurfacePoints(pedPoint, lastPoint);
        m_Route->AddUnlessFull(pedPoint);
    } else {
        // Work out the way round with the small car code, while the ped and the target are moved to the closest points of the vehicle
        CVector pedPoint, targetPoint;
        ClosestSurfacePoints(pedPoint, targetPoint);

        const auto savedPedPos = ped->GetPosition();
        const auto savedTarget = m_TargetPt;
        ped->SetPosn(pedPoint);
        m_TargetPt = targetPoint;
        ComputeRouteRoundSmallCar(ped);
        m_TargetPt = savedTarget;
        ped->SetPosn(savedPedPos);

        // The route is rebuilt to start at the ped's point, followed by what the small car code found
        const CPointRoute inner = *m_Route;
        m_Route->Clear();
        m_Route->AddUnlessFull(pedPoint);
        for (const auto& point : inner.GetAll()) {
            m_Route->AddUnlessFull(point);
        }

        const auto saved = PGA::ms_BoundingBoxTolerance;
        PGA::ms_BoundingBoxTolerance = 0.7f;
        PGA::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, lastPoint);
        PGA::ms_BoundingBoxTolerance = saved;
    }
    m_Route->AddUnlessFull(lastPoint);

    return 0.0f;
}

// 0x657B80
float CTaskComplexWalkRoundCar::ComputeRoute(CPed* ped) {
    return m_Veh->vehicleFlags.bIsBig
        ? ComputeRouteRoundBigCar(ped)
        : ComputeRouteRoundSmallCar(ped);
}

// 0x656B70
CTask* CTaskComplexWalkRoundCar::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return CreateRouteTask(ped);
    }
    return nullptr;
}

// 0x658200
CTask* CTaskComplexWalkRoundCar::CreateFirstSubTask(CPed* ped) {
    m_Route->Clear();
    const auto distToSide = m_Veh->vehicleFlags.bIsBig ? ComputeRouteRoundBigCar(ped) : ComputeRouteRoundSmallCar(ped);
    if (m_Route->IsEmpty()) {
        return nullptr;
    }

    // Give up after a while, much later when the vehicle is towing something or being towed
    auto timeMs = m_MoveState == PEDMOVE_WALK ? 20'000 : 15'000;
    if (m_Veh->m_pVehicleBeingTowed || m_Veh->m_pTowingVehicle) {
        timeMs = (int32)((float)timeMs * 4.0f);
    }
    m_Timer.Start(timeMs);

    // Remember how the vehicle stands, the route is only good while it stays like this
    const auto& vehMatrix = m_Veh->GetMatrix();
    m_VehPos      = m_Veh->GetPosition();
    m_VehMatFwd   = vehMatrix.GetForward();
    m_VehMatRight = vehMatrix.GetRight();

    const auto& pedPos      = ped->GetPosition();
    auto        toFirst     = m_Route->m_Entries[0] - pedPos;
    const auto  distToFirst = toFirst.NormaliseAndMag();
    const auto  facing      = DotProduct(toFirst, ped->GetForward());

    const auto closeEnough = m_MoveState == PEDMOVE_WALK ? 2.0f : m_MoveState == PEDMOVE_RUN ? 4.0f : 6.0f;

    if (ped->IsPlayer()) {
        if (const auto* const task = ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_ENTER_CAR_AS_DRIVER)) {
            m_EnterCarStartTime = static_cast<const CTaskComplexEnterCar*>(task)->GetEnterCarStartTime();
        }
    }

    // Already facing the way and far enough from the vehicle, no need to turn first
    if (!m_Veh->vehicleFlags.bIsBig && distToFirst > closeEnough && distToSide > closeEnough && facing > 0.0f) {
        return CreateRouteTask(ped);
    }

    // Stop first, then turn towards the start of the route
    CTaskSimpleStandStill{ 0, false, false, 8.0f }.ProcessPed(ped);

    const auto firstPoint = m_Route->m_Entries[0];
    const auto heading    = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(firstPoint.x - pedPos.x, firstPoint.y - pedPos.y, 0.0f, 0.0f));
    return new CTaskSimpleAchieveHeading{ heading, 1.0f, 0.1f };
}

// 0x654370
CTask* CTaskComplexWalkRoundCar::ControlSubTask(CPed* ped) {
    if (m_bFirstSubTaskNeedsToBeCreated) {
        m_bFirstSubTaskNeedsToBeCreated = false;
        return CreateFirstSubTask(ped);
    }

    // The walk is cut short when it takes too long, or when the player gives up on entering the vehicle
    bool shouldAbort    = false;
    bool playerQuitting = false;
    if (ped->IsPlayer() && m_Veh && m_EnterCarStartTime != -1 && CCarEnterExit::IsPlayerToQuitCarEnter(ped, m_Veh, m_EnterCarStartTime, m_pSubTask)
        && m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE) {
        shouldAbort    = true;
        playerQuitting = true;
    } else if (m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE && m_Timer.IsOutOfTime()) {
        shouldAbort = true;
    }

    // Keep going while the vehicle hasn't moved or turned since the route was made
    if (m_Veh && !shouldAbort) {
        const auto& matrix = m_Veh->GetMatrix();
        if ((m_VehPos - m_Veh->GetPosition()).SquaredMagnitude() <= 0.0625f
            && DotProduct(matrix.GetForward(), m_VehMatFwd) >= 0.9f
            && DotProduct(matrix.GetRight(), m_VehMatRight) >= 0.9f) {
            return m_pSubTask;
        }
    }

    if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT)) {
        if (playerQuitting) {
            if (auto* const primary = ped->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_PRIMARY); primary && primary->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
                primary->MakeAbortable(ped, ABORT_PRIORITY_LEISURE);
            }
        }
        return nullptr;
    }
    return m_pSubTask;
}
