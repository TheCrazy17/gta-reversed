#include "StdInc.h"

#include "ColDisk.h"

void CColDisk::InjectHooks() {
    RH_ScopedClass(CColDisk);
    RH_ScopedCategory("Collision");

    RH_ScopedInstall(Set, 0x156CE20);
}

// 0x156CE20
void CColDisk::Set(float radius, const CVector& center, const CVector& thicknessDir, float thickness, eSurfaceType material, uint8 pieceType, tColLighting lighting) {
    m_fRadius             = radius;
    m_vecCenter           = center;
    m_vThickness          = thicknessDir;
    m_fThickness          = thickness;
    m_Surface.m_nMaterial = material;
    m_Surface.m_nPiece    = pieceType;
    m_Surface.m_nLighting = lighting;
}
