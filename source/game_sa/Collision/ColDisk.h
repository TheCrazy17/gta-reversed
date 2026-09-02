/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "Vector.h"
#include "ColSurface.h"

class CColDisk : public CColSphere {
public:
    CVector     m_vThickness{};
    float       m_fThickness{};

public:
    static void InjectHooks();

    // 0x156CE20
    void Set(float radius, const CVector& center, const CVector& thicknessDir, float thickness, eSurfaceType material, uint8 pieceType, tColLighting lighting = tColLighting{ 0xFF });
};
VALIDATE_SIZE(CColDisk, 0x24);
