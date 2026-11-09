#include "StdInc.h"

#include "FxFrustumInfo.h"

void FxFrustumInfo_c::InjectHooks() {
    RH_ScopedClass(FxFrustumInfo_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(IsCollision, 0x4AA030);
}

// 0x4AA030
bool FxFrustumInfo_c::IsCollision(FxSphere_c& sphere) {
    if (!m_Sphere.IsCollision(sphere))
        return false;

    // A sphere that is outside of the frustum tends to stay outside of the plane that rejected it, so the planes
    // are tested starting with the one the sphere remembers (it is only updated when it rejects the sphere)
    for (auto i = 0u; i < m_Planes.size(); i++) {
        const auto planeIdx = (sphere.m_nNumPlanesPassed + i) % m_Planes.size();
        if (sphere.GetDistToPlane(m_Planes[planeIdx]) > sphere.m_fRadius) {
            sphere.m_nNumPlanesPassed = planeIdx;
            return false;
        }
    }
    return true;
}
