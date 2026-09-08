#include "StdInc.h"

#include "PedGeometryAnalyser.h"

void CPedGeometryAnalyser::InjectHooks() {
    RH_ScopedClass(CPedGeometryAnalyser);
    RH_ScopedCategoryGlobal();

    RH_ScopedOverloadedInstall(CanPedJumpObstacle, "", 0x5F1B00, bool(*)(const CPed&,const CEntity&));
    RH_ScopedOverloadedInstall(CanPedJumpObstacle, "contacted", 0x5F32D0, bool(*)(const CPed&,const CEntity&,const CVector&,const CVector&));
    RH_ScopedInstall(CanPedTargetPed, 0x5F1C40);
    RH_ScopedInstall(CanPedTargetPoint, 0x5F1B70);
    RH_ScopedInstall(ComputeBuildingHitPoints, 0x5F1E30);
    RH_ScopedInstall(ComputeClearTarget, 0x5F5D80);
    RH_ScopedOverloadedInstall(ComputeClosestSurfacePoint, "ped", 0x5F3B70, bool (*)(const CPed& ped, CEntity& entity, CVector& point));
    RH_ScopedOverloadedInstall(ComputeClosestSurfacePoint, "posn", 0x5F36F0, bool(*)(const CVector&,CEntity&,CVector&));
    RH_ScopedOverloadedInstall(ComputeClosestSurfacePoint, "rect", 0x5F2C10, bool(*)(const CVector&,const CVector*,CVector&));
    RH_ScopedInstall(ComputeEntityBoundingBoxCentreUncached, 0x5F1600);
    RH_ScopedInstall(ComputeEntityBoundingBoxCentreUncachedAll, 0x5F3B40);
    RH_ScopedInstall(ComputeEntityBoundingBoxCorners, 0x5F3650);
    RH_ScopedInstall(ComputeEntityBoundingBoxCornersUncached, 0x5F1FA0, { .reversed = false });
    RH_ScopedInstall(ComputeEntityBoundingBoxPlanes, 0x5F3660);
    RH_ScopedInstall(ComputeEntityBoundingBoxPlanesUncached, 0x5F1670);
    RH_ScopedInstall(ComputeEntityBoundingBoxPlanesUncachedAll, 0x5F2B80);
    RH_ScopedInstall(ComputeEntityBoundingBoxSegmentPlanes, 0x5F36A0);
    RH_ScopedInstall(ComputeEntityBoundingBoxSegmentPlanesUncached, 0x5F1750);
    RH_ScopedInstall(ComputeEntityBoundingBoxSegmentPlanesUncachedAll, 0x5F2BC0);
    RH_ScopedInstall(ComputeEntityBoundingSphere, 0x5F3C20);
    RH_ScopedInstall(ComputeMoveDirToAvoidEntity, 0x5F3730);
    RH_ScopedInstall(ComputeEntityDirs, 0x5F1500);
    RH_ScopedOverloadedInstall(ComputeEntityHitSide, "1", 0x5F3BC0, int32 (*)(const CPed& ped, CEntity& entity));
    RH_ScopedOverloadedInstall(ComputeEntityHitSide, "2", 0x5F1450, int32 (*)(const CVector& point, const CVector* planes, const float* planesDot));
    RH_ScopedOverloadedInstall(ComputeEntityHitSide, "3", 0x5F3AC0, int32 (*)(const CVector& point, CEntity& entity));
    RH_ScopedOverloadedInstall(ComputePedHitSide, "physical", 0x5F3640, int32(*)(const CPed&,const CPhysical&));
    RH_ScopedOverloadedInstall(ComputePedHitSide, "posn", 0x5F1E70, int32(*)(const CPed&,const CVector&));
    RH_ScopedInstall(ComputePedShotSide, 0x5F13F0);
    RH_ScopedOverloadedInstall(ComputeRouteRoundEntityBoundingBox, "1", 0x5F6110, int32(*)(const CPed&,CEntity&,const CVector&,CPointRoute&,int32));
    RH_ScopedOverloadedInstall(ComputeRouteRoundEntityBoundingBox, "2", 0x5F3DD0, int32(*)(const CPed&,const CVector&,CEntity&,const CVector&,CPointRoute&,int32), { .reversed = false });
    RH_ScopedInstall(ComputeRouteRoundSphere, 0x5F1890);
    RH_ScopedOverloadedInstall(GetIsLineOfSightClear, "ped", 0x5F5A30, bool(*)(const CPed&,const CVector&,CEntity&,float&));
    RH_ScopedOverloadedInstall(GetIsLineOfSightClear, "v3d", 0x5F2F00, bool(*)(const CVector&,const CVector&,CEntity&), { .reversed = false });
    RH_ScopedInstall(GetNearestPed, 0x5F3590);
    RH_ScopedInstall(IsEntityBlockingTarget, 0x5F3970);
    RH_ScopedInstall(IsInAir, 0x5F1CB0);
    RH_ScopedInstall(IsWanderPathClear, 0x5F2F70);
    RH_ScopedInstall(LiesInsideBoundingBox, 0x5F3880);
}

// 0x5F1B00
bool CPedGeometryAnalyser::CanPedJumpObstacle(const CPed& ped, const CEntity& entity) {
    if (entity.m_bIsTempBuilding) {
        return false;
    }
    return CWorld::GetIsLineOfSightClear(ped.GetPosition(), ped.GetPosition() + ped.GetForward(), true, false, false, true, false, false, false);
}

// 0x5F32D0
bool CPedGeometryAnalyser::CanPedJumpObstacle(const CPed& ped, const CEntity& entity, const CVector& contactNormal, const CVector& contactPos) {
    if (entity.m_bIsTempBuilding) {
        return false;
    }

    if (g_surfaceInfos.IsShallowWater(ped.m_nContactSurface)) {
        return true;
    }

    auto posn = ped.GetPosition();
    auto offsetVec = ped.GetForward();

    if (contactNormal.z <= 0.17f) {
        if (!CPedGroups::IsInPlayersGroup(const_cast<CPed*>(&ped))) {
            posn.z -= 0.15f;
        }
        // offsetVec stays as ped.GetForward()
    } else {
        if (contactNormal.z > 0.9f) {
            return false;
        }

        const auto& sphere0 = ped.GetColModel()->GetData()->GetSpheres()[0];
        posn.z += sphere0.m_vecCenter.z - sphere0.m_fRadius * contactNormal.z;

        const auto horizMag = std::sqrt(contactNormal.y * contactNormal.y + contactNormal.x * contactNormal.x);

        // NOTSA: the >0.5f branch's vector math was reconstructed by hand-tracing the x87 FPU stack
        // across ~40 raw-disasm instructions - internally consistent, but not spot-checked in-game.
        if (contactNormal.z <= 0.5f) {
            offsetVec = offsetVec + offsetVec * horizMag * sphere0.m_fRadius;
        } else {
            auto v = CVector{ -contactNormal.x, -contactNormal.y, 0.0f } * (1.0f / horizMag);
            offsetVec = offsetVec + v * horizMag * sphere0.m_fRadius;
            offsetVec = offsetVec * std::min(2.0f / horizMag, 4.0f);
        }
    }

    const auto testPoint = posn + offsetVec;
    if (!CWorld::GetIsLineOfSightClear(posn, testPoint, true, false, false, true, false, false, false)) {
        return false;
    }

    const auto scaledTestPoint = posn + offsetVec * 3.0f;
    bool  foundGround{};
    const auto groundZ = CWorld::FindGroundZFor3DCoord(scaledTestPoint, &foundGround, nullptr);
    return foundGround && (scaledTestPoint.z - groundZ) < 3.0f;
}

// 0x5F1C40
bool CPedGeometryAnalyser::CanPedTargetPed(CPed& ped, CPed& targetPed, bool checkDirection) {
    return CanPedTargetPoint(
        ped,
        targetPed.GetPosition() + CVector{ 0.f, 0.f, targetPed.GetTaskManager().GetTaskSecondary(TASK_SECONDARY_DUCK) ? -0.25f : 0.75f }, // 0.75f - 1.f = -0.25f
        checkDirection
    );
}

// 0x5F1B70
bool CPedGeometryAnalyser::CanPedTargetPoint(const CPed& ped, const CVector& point, bool checkDirection) {
    const auto delta = point - ped.GetPosition();

    if (checkDirection && delta.Dot(ped.GetForward()) < 0.0f) {
        return false;
    }

    if (delta.SquaredMagnitude() > 40.0f * 40.0f) {
        return false;
    }

    return CWorld::GetIsLineOfSightClear(ped.GetPosition() + CVector{ 0.0f, 0.0f, 0.75f }, point, true, false, false, true, false, true, false);
}

// 0x5F1E30
// unused
int32 CPedGeometryAnalyser::ComputeBuildingHitPoints(const CVector& a1, const CVector& a2) {
    CEntity *outEntity;
    CColPoint v4;

    CWorld::ProcessLineOfSight(a1, a2, v4, outEntity, true, false, false, false, true, false, false, false);
    return CWorld::ms_iProcessLineNumCrossings;
}

// 0x5F5D80
void CPedGeometryAnalyser::ComputeClearTarget(const CPed& ped, const CVector& targetPosn, CVector& outTarget) {
    static constexpr auto MAX_RADIUS = 5.00f; // _DAT_0086c6a4
    static constexpr auto STEP       = 0.35f; // DAT_008d22b0

    outTarget = targetPosn;

    const auto& pedPos = ped.GetPosition();

    // Pull `outTarget` back towards the ped, away from any nearby vehicle/ped whose bounding box it
    // lies inside of and which blocks a direct line of sight from the ped to it.
    const auto PullBackFromBlockers = [&](CEntity* const* entities) {
        for (const auto candidate : std::span{ entities, MAX_NUM_ENTITIES }) {
            if (!candidate) {
                continue;
            }

            if (DistanceBetweenPointsSquared(candidate->GetPosition(), outTarget) >= sq(MAX_RADIUS)) {
                continue;
            }

            if (!LiesInsideBoundingBox(ped, outTarget, *candidate)) {
                continue;
            }

            float distToHit{};
            if (GetIsLineOfSightClear(ped, outTarget, *candidate, distToHit)) {
                continue; // LOS clear - nothing to avoid
            }

            outTarget -= Normalized(outTarget - pedPos) * (STEP + distToHit);
        }
    };

    PullBackFromBlockers(ped.GetIntelligence()->GetVehicleEntities());
    PullBackFromBlockers(ped.GetIntelligence()->GetPedEntities());

    // Final pass: `outTarget` might still be embedded inside world geometry (e.g. a building) after
    // the pulls above. Detect that via the parity of `ProcessLineOfSight`'s crossing count between
    // the ped and `outTarget` (an odd crossing count means `outTarget` sits on the "inside" of an odd
    // number of surfaces) and, if so, walk it back towards the ped in `STEP`-sized increments until it
    // clears, leaves `MAX_RADIUS`, overshoots past the ped, or we run out of steps.
    const auto stepVec  = Normalized(pedPos - outTarget) * STEP;
    const auto maxSteps = static_cast<int32>(MAX_RADIUS / STEP); // (int32)(5.0f / 0.35f) == 14

    for (auto step = 0; step <= maxSteps; step++) {
        const auto toPed = pedPos - outTarget;
        if (toPed.SquaredMagnitude() >= sq(MAX_RADIUS)) {
            return;
        }
        if (DotProduct(toPed, stepVec) < 0.0f) {
            return;
        }

        CColPoint colPoint{};
        CEntity*  hitEntity{};
        CWorld::ProcessLineOfSight(pedPos, outTarget, colPoint, hitEntity, true, false, false, false, true, false, false, false);

        if (CWorld::ms_iProcessLineNumCrossings % 2 != 1) {
            return;
        }

        outTarget += stepVec;
    }
}

// 0x5F3B70
bool CPedGeometryAnalyser::ComputeClosestSurfacePoint(const CPed& ped, CEntity& entity, CVector& point) {
    CVector corners[4];
    const auto& posn = ped.GetPosition();
    ComputeEntityBoundingBoxCornersUncached(posn.z, entity, corners);
    return ComputeClosestSurfacePoint(posn, corners, point);
}

// 0x5F36F0
bool CPedGeometryAnalyser::ComputeClosestSurfacePoint(const CVector& posn, CEntity& entity, CVector& point) {
    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(posn.z, entity, corners);
    return ComputeClosestSurfacePoint(posn, corners, point);
}

// 0x5F2C10
bool CPedGeometryAnalyser::ComputeClosestSurfacePoint(const CVector& posn, const CVector* corners, CVector& point) {
    auto bestDistSq = FLT_MAX;
    auto found      = false;

    for (auto i = 0; i < 4; i++) {
        const auto& a = corners[i];
        const auto& b = corners[(i + 1) % 4];

        const auto edge    = b - a;
        const auto edgeLen = edge.Magnitude();
        const auto invLen  = 1.0f / edgeLen;
        const auto t       = DotProduct(posn - a, edge) * invLen;

        if (t >= 0.0f && t < edgeLen) {
            const auto closest = a + edge * (t * invLen);
            if (const auto distSq = DistanceBetweenPointsSquared(posn, closest); distSq < bestDistSq) {
                point      = closest;
                found      = true;
                bestDistSq = distSq;
            }
        }
    }

    if (!found) {
        for (auto i = 0; i < 4; i++) {
            if (const auto distSq = DistanceBetweenPointsSquared(corners[i], posn); distSq < bestDistSq) {
                point      = corners[i];
                found      = true;
                bestDistSq = distSq;
            }
        }
    }

    return found;
}

// inlined into CPedGeometryAnalyser::ComputeEntityBoundingSphere
void CPedGeometryAnalyser::ComputeEntityBoundingBoxCentre(float zPos, CEntity& entity, CVector& center) {
    ComputeEntityBoundingBoxCentreUncachedAll(zPos, entity, center);
}

// 0x5F1600
void CPedGeometryAnalyser::ComputeEntityBoundingBoxCentreUncached(float zPos, const CVector* corners, CVector& center) {
    center.Set(0.0f, 0.0f, zPos);

    center.x = corners[0].x;
    center.y = corners[0].y;

    center.x += corners[1].x;
    center.y += corners[1].y;

    center.x += corners[2].x;
    center.y += corners[2].y;

    center.x += corners[3].x;
    center.y += corners[3].y;

    center.x *= 0.25f;
    center.y *= 0.25f;
}

// 0x5F3B40
void CPedGeometryAnalyser::ComputeEntityBoundingBoxCentreUncachedAll(float zPos, CEntity& entity, CVector& center) {
    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);
    ComputeEntityBoundingBoxCentreUncached(zPos, corners, center);
}

// 0x5F3650
void CPedGeometryAnalyser::ComputeEntityBoundingBoxCorners(float zPos, CEntity& entity, CVector* corners) {
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);
}

// 0x5F1FA0
void CPedGeometryAnalyser::ComputeEntityBoundingBoxCornersUncached(float zPos, CEntity& entity, CVector* corners) {
    plugin::Call<0x5F1FA0, float, CEntity&, void*>(zPos, entity, corners);
}

// 0x5F3660
void CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(float zPos, CEntity& entity, CVector(*outPlanes)[4], float* outPlanesDot) {
    CVector corners[4];
    ComputeEntityBoundingBoxCorners(zPos, entity, corners);
    ComputeEntityBoundingBoxPlanesUncached(zPos, corners, outPlanes, outPlanesDot);
}

// 0x5F1670
void CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanesUncached(float zPos, const CVector* corners, CVector(*outPlanes)[4], float* outPlanesDot) {
    const CVector* corner2 = &corners[3];
    for (auto i = 0; i < 4; i++) {
        const CVector& corner = corners[i];
        CVector& plane = (*outPlanes)[i];
        CVector direction = corner - *corner2;
        direction.Normalise();
        plane.x = direction.y;
        plane.y = -direction.x;
        plane.z = 0.0f;
        // point-normal plane equation:
        // ax + by + cz + d = 0
        // d = - n . P
        outPlanesDot[i] = -DotProduct(plane, *corner2);

        corner2 = &corner;
    }
}

// 0x5F2B80
void CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanesUncachedAll(float zPos, CEntity& entity, CVector (*outPlanes)[4], float* outPlanesDot) {
    CVector corners[4];
    CPedGeometryAnalyser::ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanesUncached(zPos, corners, outPlanes, outPlanesDot);
}

// 0x5F36A0
void CPedGeometryAnalyser::ComputeEntityBoundingBoxSegmentPlanes(float zPos, CEntity& entity, CVector* normals, float* dots) {
    ComputeEntityBoundingBoxSegmentPlanesUncachedAll(zPos, entity, normals, dots);
}

// 0x5F1750
CVector* CPedGeometryAnalyser::ComputeEntityBoundingBoxSegmentPlanesUncached(const CVector* corners, CVector& center, CVector* a3, float* a4) {
    for (auto i = 0; i < 4; i++) {
        const auto& corner = corners[i];
        auto&       plane  = a3[i];
        plane.x = -(corner.y - center.y);
        plane.y = corner.x - center.x;
        plane.z = 0.0f;
        a4[i]   = -DotProduct(plane, corner);
    }
    return a3;
}

// 0x5F2BC0
CVector* CPedGeometryAnalyser::ComputeEntityBoundingBoxSegmentPlanesUncachedAll(float zPos, CEntity& entity, CVector* a3, float* a4) {
    CVector corners[4];
    CVector center;

    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);
    ComputeEntityBoundingBoxCentreUncached(zPos, corners, center);
    return ComputeEntityBoundingBoxSegmentPlanesUncached(corners, center, a3, a4);
}

// 0x5F3C20
void CPedGeometryAnalyser::ComputeEntityBoundingSphere(const CPed& ped, CEntity& entity, CColSphere& outSphere) {
    const auto zPos = ped.GetPosition().z;

    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);

    CVector center;
    ComputeEntityBoundingBoxCentreUncachedAll(zPos, entity, center);

    auto maxDistSq = 0.0f;
    for (const auto& corner : corners) {
        maxDistSq = std::max(maxDistSq, DistanceBetweenPointsSquared(corner, center));
    }

    outSphere.Set(std::sqrt(maxDistSq) * 1.1f, center, SURFACE_DEFAULT);
}

// 0x5F3730
int32 CPedGeometryAnalyser::ComputeMoveDirToAvoidEntity(const CPed& ped, CEntity& entity, CVector& outDirToAvoidEntity) {
    const auto zPos = ped.GetPosition().z;

    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);

    CVector planes[4];
    float   planesDot[4];
    ComputeEntityBoundingBoxPlanesUncached(zPos, corners, &planes, planesDot);

    const auto& pedPos = ped.GetPosition();
    const auto  d0      = DotProduct(planes[1], pedPos) + planesDot[1];
    const auto  d1      = DotProduct(planes[3], pedPos) + planesDot[3];

    if (d0 > 0.0f) {
        outDirToAvoidEntity = planes[1];
    } else if (d1 > 0.0f) {
        outDirToAvoidEntity = planes[3];
    } else if (d0 > d1) {
        outDirToAvoidEntity = planes[1];
    } else {
        outDirToAvoidEntity = planes[3];
    }

    // NOTSA: the original never deliberately sets a return value in any branch (EAX at RET is
    // incidental register content, not a real result); the only caller
    // (CEventHandler::ComputeVehiclePotentialCollisionResponse) discards it.
    return 0;
}

//! @notsa
CVector CPedGeometryAnalyser::ComputeEntityDir(const CEntity& entity, eDirection dir) {
    switch (dir) {
    case eDirection::FORWARD:  return entity.GetForward();
    case eDirection::LEFT:     return -entity.GetRight();
    case eDirection::BACKWARD: return -entity.GetForward();
    case eDirection::RIGHT:    return entity.GetRight();
    default:                   NOTSA_UNREACHABLE();
    }
}

// 0x5F1500
CVector* CPedGeometryAnalyser::ComputeEntityDirs(const CEntity& entity, CVector* posn) {
    const auto fwd = entity.m_matrix
        ? entity.m_matrix->GetForward()
        : CVector{ -std::sin(entity.m_placement.m_fHeading), std::cos(entity.m_placement.m_fHeading), 0.0f };
    const auto right = entity.m_matrix
        ? entity.m_matrix->GetRight()
        : CVector{ std::cos(entity.m_placement.m_fHeading), std::sin(entity.m_placement.m_fHeading), 0.0f };

    posn[0] = fwd;
    posn[1] = -right;
    posn[2] = -fwd;
    posn[3] = right;
    return posn;
}

// 0x5F3BC0
int32 CPedGeometryAnalyser::ComputeEntityHitSide(const CPed& ped, CEntity& entity) {
    return ComputeEntityHitSide(ped.GetPosition(), entity);
}

// 0x5F1450
int32 CPedGeometryAnalyser::ComputeEntityHitSide(const CVector& point, const CVector* planes, const float* planesDot) {
    // Find which of the 4 planes' "wedge" (shared with its predecessor in the cyclic order) `point`
    // falls into - i.e. the first `i` for which `point` is on the outward side of BOTH planes[i] and
    // its predecessor planes[(i-1)%4]. Scans the 4-plane cycle twice (8 iterations) so the search is
    // independent of where in the cycle it starts.
    for (auto i = 0; i < 8; i++) {
        const auto prevIdx = (i + 3) % 4;
        const auto curIdx  = i % 4;

        if (DotProduct(planes[prevIdx], point) + planesDot[prevIdx] >= 0.0f &&
            DotProduct(planes[curIdx], point) + planesDot[curIdx] >= 0.0f) {
            return curIdx; // eDirection
        }
    }
    return 0; // eDirection::FORWARD
}

// 0x5F3AC0
int32 CPedGeometryAnalyser::ComputeEntityHitSide(const CVector& point, CEntity& entity) {
    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(point.z, entity, corners);

    CVector center;
    ComputeEntityBoundingBoxCentreUncached(point.z, corners, center);

    CVector planes[4];
    float   planesDot[4];
    ComputeEntityBoundingBoxSegmentPlanesUncached(corners, center, planes, planesDot);

    return ComputeEntityHitSide(point, planes, planesDot);
}

// 0x5F3640
int32 CPedGeometryAnalyser::ComputePedHitSide(const CPed& ped, const CPhysical& physical) {
    return ComputePedHitSide(ped, physical.m_vecMoveSpeed);
}

// 0x5F1E70
int32 CPedGeometryAnalyser::ComputePedHitSide(const CPed& ped, const CVector& posn) {
    const auto negPosn = Normalized(-posn);

    CVector dirs[4];
    ComputeEntityDirs(ped, dirs);

    auto bestIdx = 0;
    auto bestDot = -1.0f;
    for (auto i = 0; i < 4; i++) {
        if (const auto dot = DotProduct(dirs[i], negPosn); dot >= bestDot) {
            bestDot = dot;
            bestIdx = i;
        }
    }
    return bestIdx; // eDirection
}

// 0x5F13F0
int32 CPedGeometryAnalyser::ComputePedShotSide(const CPed& ped, const CVector& posn) {
    const auto& pedPos = ped.GetPosition();
    return static_cast<int32>(std::atan2(-(posn.x - pedPos.x), posn.y - pedPos.y));
}

// 0x5F6110
int32 CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(const CPed& ped, CEntity& entity, const CVector& posn, CPointRoute& pointRoute, int32 a5) {
    return ComputeRouteRoundEntityBoundingBox(ped, ped.GetPosition(), entity, posn, pointRoute, a5);
}

// 0x5F3DD0
int32 CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(const CPed& ped, const CVector& a2, CEntity& entity, const CVector& a4, CPointRoute& pointRoute, int32 a6){
    return plugin::CallAndReturn<int32, 0x5F3DD0, const CPed&, const CVector&, CEntity&, const CVector&, CPointRoute&, int32>(ped, a2, entity, a4, pointRoute, a6);
}

// 0x5F1890
bool CPedGeometryAnalyser::ComputeRouteRoundSphere(const CPed& ped, const CColSphere& sphere, const CVector& start, const CVector& target, CVector& newTarget, CVector& detourPoint) {
    const auto& pedPos = ped.GetPosition();

    newTarget = target;

    // If `target` itself is inside the sphere we're meant to avoid, pull it back to
    // where the start->target line exits the far side of the sphere.
    if (sphere.IntersectPoint(target)) {
        const auto dir = Normalized(target - start);

        CVector nearPt{}, farPt{};
        if (sphere.IntersectRay(pedPos, dir, nearPt, farPt)) {
            newTarget = farPt;
        }
    }

    // Does the direct line from `newTarget` to the ped even come near the sphere?
    const auto toNewTargetDir = Normalized(newTarget - pedPos);

    CVector nearPt{}, farPt{};
    if (!sphere.IntersectRay(newTarget, toNewTargetDir, nearPt, farPt)) {
        detourPoint = newTarget;
        return false;
    }

    // `newTarget` is already closer to the ped than the sphere's near surface point,
    // so the direct path doesn't actually reach the obstacle - no detour needed.
    if (DistanceBetweenPointsSquared(newTarget, pedPos) < DistanceBetweenPointsSquared(nearPt, pedPos)) {
        detourPoint = newTarget;
        return false;
    }

    // Otherwise steer around it: find the point on the sphere's surface closest to
    // the ped->newTarget line, and route through there.
    if (sphere.IntersectRay(pedPos, toNewTargetDir, nearPt, farPt)) {
        const auto t                 = DotProduct(sphere.m_vecCenter - pedPos, toNewTargetDir);
        const auto closestPointOnRay = pedPos + toNewTargetDir * t;
        const auto offset            = Normalized(closestPointOnRay - sphere.m_vecCenter);
        detourPoint = sphere.m_vecCenter + offset * sphere.m_fRadius;
    }
    // NOTSA: if this 3rd IntersectRay call somehow fails (shouldn't happen - we already
    // know the ped->newTarget line intersects the sphere), `detourPoint` is left
    // untouched here, matching the original binary's behaviour exactly.

    return true;
}

// 0x5F5A30
bool CPedGeometryAnalyser::GetIsLineOfSightClear(const CPed& ped, const CVector& target, CEntity& entity, float& outDist) {
    static constexpr auto TOLERANCE = 0.35f;  // DAT_008d22b0 - dead-zone half-width around each plane for the inside/outside/on classification
    static constexpr auto DENOM_EPS = 0.001f; // _DAT_00858cdc - guards the clip-parameter division against a near-parallel/zero denominator

    auto segStart = ped.GetPosition();
    auto segEnd   = target;

    CColSphere sphere;
    ComputeEntityBoundingSphere(ped, entity, sphere);

    auto dir = segEnd - segStart;
    dir.Normalise(); // Original calls NormaliseAndMag() and discards the returned pre-normalise length

    CVector unusedNear{}, unusedFar{};
    if (!sphere.IntersectRay(segStart, dir, unusedNear, unusedFar)) {
        return true; // Doesn't even reach the (coarse) bounding sphere - can't reach the (tighter) box either
    }

    // NOTSA: `zPos` is `ped`'s position.z, not `entity`'s - a redundant re-derivation of `pedPos.z`
    // (confirmed via raw disasm), matching the identical quirk in this file's `LiesInsideBoundingBox`/
    // `ComputeMoveDirToAvoidEntity`.
    const auto zPos = ped.GetPosition().z;

    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);

    CVector planes[4];
    float   planesDot[4];
    ComputeEntityBoundingBoxPlanesUncached(zPos, corners, &planes, planesDot);

    outDist = 0.0f;

    for (auto i = 0; i < 4; i++) {
        const auto dStart = DotProduct(planes[i], segStart) + planesDot[i];
        const auto dEnd   = DotProduct(planes[i], segEnd) + planesDot[i];

        // -1 = clearly inside this face's half-space, 0 = within TOLERANCE of the plane ("on" it), +1 = clearly outside
        const auto startSide = dStart > TOLERANCE ? 1 : (dStart < -TOLERANCE ? -1 : 0);
        const auto endSide   = dEnd > TOLERANCE ? 1 : (dEnd < -TOLERANCE ? -1 : 0);

        const auto denom = DotProduct(planes[i], dir);
        const auto canClip = denom > DENOM_EPS;

        if (startSide < 0) {
            if (endSide > 0 && canClip) { // segStart inside, segEnd clearly outside -> pull segEnd back to the crossing point
                segEnd = segStart + dir * ((-1.0f / denom) * dStart);
            }
        } else if (endSide >= 0) {
            return true; // Both endpoints on/outside plane i - can't lie inside every plane's half-space at once, so not blocked
        } else if (startSide == 1 && canClip) { // segStart clearly outside, segEnd inside -> pull segStart forward to the crossing point
            segStart = segStart + dir * ((-1.0f / denom) * dStart);
        }
        // else: segStart "on" plane i (within TOLERANCE) and segEnd inside - no-op, matches the original's fallthrough
    }

    // The segment survived clipping against all 4 planes with a non-empty piece remaining inside - blocked.
    outDist = (segEnd - segStart).Magnitude();
    return false;
}

// 0x5F2F00
bool CPedGeometryAnalyser::GetIsLineOfSightClear(const CVector& a1, const CVector& a2, CEntity& a3) {
    return plugin::CallAndReturn<bool, 0x5F2F00, const CVector&, const CVector&, CEntity&>(a1, a2, a3);
}

// 0x5F3590
CPed* CPedGeometryAnalyser::GetNearestPed(const CVector& point) {
    CPed* nearest    = nullptr;
    auto  bestDistSq = FLT_MAX;

    for (int32 i = GetPedPool()->GetSize() - 1; i >= 0; i--) {
        const auto ped = GetPedPool()->GetAt(i);
        if (!ped) {
            continue;
        }

        if (const auto distSq = DistanceBetweenPointsSquared(point, ped->GetPosition()); distSq < bestDistSq) {
            bestDistSq = distSq;
            nearest    = ped;
        }
    }

    return nearest;
}

// 0x5F3970
bool CPedGeometryAnalyser::IsEntityBlockingTarget(CEntity* entity, const CVector& point, float distance) {
    const auto& entityPos = entity->GetPosition();

    if (std::abs(entityPos.z - point.z) > 3.0f) {
        return false;
    }

    const auto entityRadius = entity->GetModelInfo()->GetColModel()->GetBoundRadius();
    const auto horizDist    = std::sqrt(sq(entityPos.x - point.x) + sq(entityPos.y - point.y));

    // NOTSA: matches the original bit-for-bit, but comparing a squared sum (distance^2+radius^2)
    // against an UNsquared horizontal distance is dimensionally odd - verified via raw disasm,
    // not a decompiler artifact. Likely a cheap/approximate original-game pre-filter; translated
    // literally rather than "fixed".
    if (sq(distance) + sq(entityRadius) < horizDist) {
        return false;
    }

    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(entityPos.z, *entity, corners);

    CVector planes[4];
    float   planesDot[4];
    ComputeEntityBoundingBoxPlanesUncached(entityPos.z, corners, &planes, planesDot);

    const auto distanceBias = distance * 0.5f;
    for (auto i = 0; i < 4; i++) {
        if (DotProduct(planes[i], point) + planesDot[i] + distanceBias > 0.0f) {
            return false;
        }
    }
    return true;
}

// 0x5F1CB0
bool CPedGeometryAnalyser::IsInAir(const CPed& ped) {
    if (ped.bInVehicle) {
        return false;
    }

    auto& taskMgr = ped.GetTaskManager();
    if (taskMgr.GetActiveTask()) {
        if (ped.GetIntelligence()->GetTaskSwim())    { return false; }
        if (ped.GetIntelligence()->GetTaskJetPack()) { return false; }
        if (const auto* simplest = taskMgr.GetSimplestActiveTask(); simplest && simplest->GetTaskType() == TASK_SIMPLE_CLIMB) {
            return false;
        }
    }

    const auto* activeTask = taskMgr.GetActiveTask();
    const auto  isComplexJumpTask = activeTask && activeTask->GetTaskType() == TASK_COMPLEX_JUMP;

    const auto& posn = ped.GetPosition();

    CColPoint colPoint{};
    CEntity*  hitEntity{};
    if (CWorld::ProcessVerticalLine(posn, posn.z - 1.5f, colPoint, hitEntity, true, true, false, true, false, false, nullptr)) {
        return false;
    }

    if (isComplexJumpTask) {
        return true;
    }

    return !CWorld::TestSphereAgainstWorld({ posn.x, posn.y, posn.z - 1.0f }, 0.15f, const_cast<CPed*>(&ped), true, false, false, false, false, false);
}

// 0x5F2F70
CPedGeometryAnalyser::WanderPathClearness CPedGeometryAnalyser::IsWanderPathClear(const CVector& from, const CVector& to, float maxHeightChange, int32 maxSamples) {
    if (std::abs(from.z - to.z) > maxHeightChange) {
        return WanderPathClearness::BLOCKED_HEIGHT;
    }

    // Line-of-sight check done at the lower of the two Z's (height diff already covered above).
    const auto minZ = std::min(from.z, to.z);
    if (!CWorld::GetIsLineOfSightClear({ from.x, from.y, minZ }, { to.x, to.y, minZ }, true, false, false, false, false, false, false)) {
        return WanderPathClearness::BLOCKED_LOS;
    }

    auto       delta    = to - from;
    const auto numSteps = std::min(maxSamples, (int32)delta.Magnitude());
    if (numSteps == 0) {
        return WanderPathClearness::CLEAR;
    }

    delta.Normalise();

    const auto maxZ = std::max(from.z, to.z);

    // Walk the path, and if any sample point is over water, make sure there's something
    // (e.g. a bridge) above the water up to maxZ - otherwise it's open water, so blocked.
    for (auto step = 1; step < numSteps; step++) {
        const auto samplePos = from + delta * (float)step;

        float waterLevel;
        if (CWaterLevel::GetWaterLevel(samplePos.x, samplePos.y, samplePos.z, waterLevel, false, nullptr)) {
            CColPoint colPoint{};
            CEntity*  hitEntity{};
            if (!CWorld::ProcessVerticalLine({ samplePos.x, samplePos.y, waterLevel }, maxZ, colPoint, hitEntity, true, false, false, false, false, false, nullptr)) {
                return WanderPathClearness::BLOCKED_WATER;
            }
        }
    }

    // Make sure there's ground within 5 units below the start point.
    CColPoint colPoint{};
    CEntity*  hitEntity{};
    if (!CWorld::ProcessVerticalLine(from, from.z - 5.0f, colPoint, hitEntity, true, false, false, false, false, false, nullptr)) {
        return WanderPathClearness::BLOCKED_SHARP_DROP;
    }
    auto expectedGroundZ = colPoint.m_vecPoint.z + 0.5f;

    // Walk the path again, tracking the running ground height and rejecting any sudden drop/rise.
    for (auto step = 1; step < numSteps; step++) {
        const auto sampleX = from.x + delta.x * (float)step;
        const auto sampleY = from.y + delta.y * (float)step;

        if (!CWorld::ProcessVerticalLine({ sampleX, sampleY, expectedGroundZ }, expectedGroundZ - 2.0f, colPoint, hitEntity, true, false, false, false, false, false, nullptr)) {
            return WanderPathClearness::BLOCKED_SHARP_DROP;
        }

        if (std::abs(colPoint.m_vecPoint.z - expectedGroundZ) > 1.0f) {
            return WanderPathClearness::BLOCKED_SHARP_DROP;
        }

        expectedGroundZ = colPoint.m_vecPoint.z + 0.5f;
    }

    return WanderPathClearness::CLEAR;
}

// 0x5F3880
bool CPedGeometryAnalyser::LiesInsideBoundingBox(const CPed& ped, const CVector& posn, CEntity& entity) {
    if (CVector::DistSqr(posn, entity.GetPosition()) >= sq(entity.GetModelInfo()->GetColModel()->GetBoundRadius())) {
        return false;
    }

    const auto zPos = ped.GetPosition().z;

    CVector corners[4];
    ComputeEntityBoundingBoxCornersUncached(zPos, entity, corners);

    CVector planes[4];
    float   planesDot[4];
    ComputeEntityBoundingBoxPlanesUncached(zPos, corners, &planes, planesDot);

    // NOTSA: matches the original bit-for-bit, but this OR-on-any-negative-plane early exit is a
    // near-no-op for a convex box (opposite side-pairs have anti-parallel normals, so almost any
    // point that passes the sphere check above trips this) - flagging as a likely genuine original
    // quirk rather than "fixing" it into a real polygon-containment test.
    for (auto i = 0; i < 4; i++) {
        if (DotProduct(planes[i], posn) + planesDot[i] < 0.0f) {
            return true;
        }
    }
    return false;
}

// 0x41B7C0
void* CPointRoute::operator new(uint32 size) {
    return GetPointRoutePool()->New();
}

// 0x41B7D0
void CPointRoute::operator delete(void* ptr, size_t sz) {
    GetPointRoutePool()->Delete(reinterpret_cast<CPointRoute*>(ptr));
}
