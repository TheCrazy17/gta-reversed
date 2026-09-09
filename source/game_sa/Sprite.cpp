#include "StdInc.h"
#include "Sprite.h"

static inline auto& nSpriteBufferIndex = StaticRef<int32>(0xC6A158);
static inline auto& s_XLUSpriteVertices = StaticRef<std::array<RwIm2DVertex, 4>>(0xC4B8E0);

void CSprite::InjectHooks() {
    RH_ScopedClass(CSprite);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Initialise, 0x70CE10);
    RH_ScopedInstall(InitSpriteBuffer, 0x70CFB0);
    RH_ScopedInstall(FlushSpriteBuffer, 0x70CF20);
    RH_ScopedInstall(CalcScreenCoors, 0x70CE30);
    RH_ScopedInstall(CalcHorizonCoors, 0x70E3E0);
    RH_ScopedOverloadedInstall(Set4Vertices2D, "CRect", 0x70E1C0, void (*)(RwIm2DVertex*, const CRect&, const CRGBA&, const CRGBA&, const CRGBA&, const CRGBA&));
    // RH_ScopedOverloadedInstall(Set4Vertices2D, "1", 0x70E2D0, void (*)(RwD3D9Vertex*, float, float, float, float, float, float, float, float, const CRGBA&, const CRGBA&, const CRGBA&, const CRGBA&));
    RH_ScopedInstall(RenderOneXLUSprite, 0x70D000);
    RH_ScopedInstall(RenderOneXLUSprite_Triangle, 0x70D320);
    RH_ScopedInstall(RenderOneXLUSprite_Rotate_Aspect, 0x70D490);
    RH_ScopedInstall(RenderOneXLUSprite2D, 0x70F540);
    RH_ScopedInstall(RenderBufferedOneXLUSprite, 0x70E4A0);
    RH_ScopedInstall(RenderBufferedOneXLUSprite_Rotate_Aspect, 0x70E780);
    RH_ScopedInstall(RenderBufferedOneXLUSprite_Rotate_Dimension, 0x70EAB0);
    RH_ScopedInstall(RenderBufferedOneXLUSprite_Rotate_2Colours, 0x70EDE0);
    RH_ScopedInstall(RenderBufferedOneXLUSprite2D, 0x70F440);
}

// 0x70CE10
void CSprite::Initialise() {
    // NOP
}

// 0x70CFB0
void CSprite::InitSpriteBuffer() {
    m_f2DNearScreenZ = RwIm2DGetNearScreenZ();
    m_f2DFarScreenZ  = RwIm2DGetFarScreenZ();
}

// unused
// 0x70CFD0
void CSprite::InitSpriteBuffer2D() {
    m_fRecipNearClipPlane = 1.0f / RwCameraGetNearClipPlane(Scene.m_pRwCamera);
    InitSpriteBuffer();
}

// 0x70CF20
void CSprite::FlushSpriteBuffer() {
    if (nSpriteBufferIndex <= 0) {
        return;
    }

    if (m_bFlushSpriteBufferSwitchZTest) {
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
    }

    RwIm2DRenderIndexedPrimitive(
        rwPRIMTYPETRILIST,
        TempBufferVertices.m_2d,
        4 * nSpriteBufferIndex,
        aTempBufferIndices,
        6 * nSpriteBufferIndex
    );

    if (m_bFlushSpriteBufferSwitchZTest) {
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(TRUE));
    }

    nSpriteBufferIndex = 0;
}

// unused
// 0x70CE20
void CSprite::Draw3DSprite(float, float, float, float, float, float, float, float, float) {
    // NOP
}

// 0x70CE30
bool CSprite::CalcScreenCoors(const RwV3d& posn, RwV3d* out, float* w, float* h, bool checkMaxVisible, bool checkMinVisible) {
    *out = TheCamera.GetViewMatrix().TransformPoint(posn);

    if (out->z <= CDraw::GetNearClipZ() + 1.0f && checkMinVisible)
        return false;

    if (out->z >= CDraw::GetFarClipZ() && checkMaxVisible)
        return false;

    const float rd = 1.0f / out->z; // reciprocal of depth

    out->x = SCREEN_WIDTH * rd * out->x;
    out->y = SCREEN_HEIGHT * rd * out->y;

    *w = SCREEN_WIDTH  * rd / CDraw::GetFOV() * 70.0f;
    *h = SCREEN_HEIGHT * rd / CDraw::GetFOV() * 70.0f;

    return true;
}

// 0x70E3E0
float CSprite::CalcHorizonCoors() {
    const auto& cameraPosn = TheCamera.GetPosition();
    CVector point{
        cameraPosn.x + TheCamera.m_fCamFrontXNorm * 3000.0f,
        cameraPosn.y + TheCamera.m_fCamFrontYNorm * 3000.0f,
        0.0f,
    };

    const auto viewPoint = TheCamera.GetViewMatrix().TransformPoint(point);
    return 1.0f / viewPoint.z * SCREEN_HEIGHT * viewPoint.y;
}

// 0x70E1C0
void CSprite::Set4Vertices2D(RwIm2DVertex* verts, const CRect& rt, const CRGBA& topLeftColor, const CRGBA& topRightColor, const CRGBA& bottomLeftColor, const CRGBA& bottomRightColor) {
    for (auto i = 0u; i < 4u; i++) {
        auto& vert = verts[i];

        vert.x = (i == 0 || i == 3) ? rt.left : rt.right;
        vert.y = (i == 0 || i == 1) ? rt.bottom : rt.top;
        vert.z = m_f2DNearScreenZ;
        vert.u = (i == 0 || i == 3) ? 0.0f : 1.0f;
        vert.v = (i == 0 || i == 1) ? 0.0f : 1.0f;
        vert.rhw = m_fRecipNearClipPlane;

        vert.emissiveColor = [&] {
        switch (i) {
            case 0: return bottomLeftColor.ToIntARGB();
            case 1: return bottomRightColor.ToIntARGB();
            case 2: return topRightColor.ToIntARGB();
            case 3: return topLeftColor.ToIntARGB();
            default: NOTSA_UNREACHABLE();
        }
        }();
    }
}

// unused
// 0x70E2D0
void CSprite::Set4Vertices2D(RwD3D9Vertex*, float, float, float, float, float, float, float, float, const CRGBA&, const CRGBA&, const CRGBA&, const CRGBA&) {
    assert(false);
}

/* --- XLU Sprite --- */

// 0x70D000
void CSprite::RenderOneXLUSprite(CVector pos, CVector2D halfSize, uint8 r, uint8 g, uint8 b, int16 intensity, float rhw, uint8 a, uint8 udir, uint8 vdir) {
    std::array<float, 4> cornerX{ pos.x - halfSize.x, pos.x - halfSize.x, pos.x + halfSize.x, pos.x + halfSize.x };
    std::array<float, 4> cornerY{ pos.y - halfSize.y, pos.y + halfSize.y, pos.y + halfSize.y, pos.y - halfSize.y };

    std::array<float, 4> u{}, v{};
    if (udir == 0) { u = { 0.f, 0.f, 1.f, 1.f }; } else { u = { 1.f, 1.f, 0.f, 0.f }; }
    if (vdir == 0) { v = { 0.f, 1.f, 1.f, 0.f }; } else { v = { 1.f, 0.f, 0.f, 1.f }; }

    // Manual clip against the screen edges, shifting UVs proportionally so the texture doesn't stretch.
    for (auto i = 0u; i < 4u; i++) {
        if (cornerX[i] < 0.0f) {
            u[i] = (cornerX[i] / halfSize.x) * -0.5f;
            cornerX[i] = 0.0f;
        }
        if (cornerX[i] > SCREEN_WIDTH) {
            u[i] = 1.0f - ((cornerX[i] - SCREEN_WIDTH) * 0.5f) / halfSize.x;
            cornerX[i] = SCREEN_WIDTH;
        }
        if (cornerY[i] < 0.0f) {
            v[i] = (cornerY[i] / halfSize.y) * -0.5f;
            cornerY[i] = 0.0f;
        }
        if (cornerY[i] > SCREEN_HEIGHT) {
            v[i] = 1.0f - ((cornerY[i] - SCREEN_HEIGHT) * 0.5f) / halfSize.y;
            cornerY[i] = SCREEN_HEIGHT;
        }
    }

    const auto z = (RwIm2DGetFarScreenZ() - RwIm2DGetNearScreenZ())
        * (pos.z - CDraw::ms_fNearClipZ)
        * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * pos.z)
        + RwIm2DGetNearScreenZ();

    const auto emissiveColor = CRGBA{
        static_cast<uint8>((r * intensity) >> 8),
        static_cast<uint8>((g * intensity) >> 8),
        static_cast<uint8>((b * intensity) >> 8),
        a
    }.ToIntARGB();

    for (auto i = 0u; i < 4u; i++) {
        s_XLUSpriteVertices[i] = {
            .x = cornerX[i],
            .y = cornerY[i],
            .z = z,
            .rhw = rhw,
            .emissiveColor = emissiveColor,
            .u = u[i],
            .v = v[i]
        };
    }

    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, s_XLUSpriteVertices.data(), 4);
}

// 0x70D320
void CSprite::RenderOneXLUSprite_Triangle(CVector2D screen1, CVector2D screen2, CVector2D screen3, float screenZ, uint8 r, uint8 g, uint8 b, int16 intensity, float recipZ, uint8 alpha) {
    if (screenZ < 1.3f) {
        return;
    }
    const uint32 factor = static_cast<uint32>(std::min(255.0f * (screenZ - 1.3f), 255.0f));
    const uint32 R      = (factor * r) >> 8;
    const uint32 G      = (factor * g) >> 8;
    const uint32 B      = (factor * b) >> 8;
    const uint32 depthI = (factor * intensity) >> 8;

    const auto emissiveColor = CRGBA{
        static_cast<uint8>(((R & 0xff) * depthI) >> 8),
        static_cast<uint8>(((G & 0xff) * depthI) >> 8),
        static_cast<uint8>(((B & 0xff) * depthI) >> 8),
        alpha
    }.ToIntARGB();

    const auto z  = (RwIm2DGetFarScreenZ() - RwIm2DGetNearScreenZ())
        * (screenZ - CDraw::ms_fNearClipZ)
        * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * screenZ)
        + RwIm2DGetNearScreenZ();

    s_XLUSpriteVertices[0] = {
        .x = screen1.x,
        .y = screen1.y,
        .z = z,
        .rhw = recipZ,
        .emissiveColor = emissiveColor
    };
    s_XLUSpriteVertices[1] = {
        .x = screen2.x,
        .y = screen2.y,
        .z = z,
        .rhw = recipZ,
        .emissiveColor = emissiveColor
    };
    s_XLUSpriteVertices[2] = {
        .x = screen3.x,
        .y = screen3.y,
        .z = z,
        .rhw = recipZ,
        .emissiveColor = emissiveColor
    };
    RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, s_XLUSpriteVertices.data(), 3);
}

// 0x70D490
void CSprite::RenderOneXLUSprite_Rotate_Aspect(CVector pos, CVector2D size, uint8 r, uint8 g, uint8 b, int16 intensity, float rz, float rotation, uint8 alpha) {
    if (pos.z < 2.3f) {
        if (pos.z < 1.3f) {
            return;
        }
        const auto factor = static_cast<uint32>(std::lround((pos.z - 1.3f) * 255.0f));
        r = static_cast<uint8>((r * factor) >> 8);
        g = static_cast<uint8>((g * factor) >> 8);
        b = static_cast<uint8>((b * factor) >> 8);
        intensity = static_cast<int16>((intensity * factor) >> 8);
    }

    const auto s = std::sin(rotation);
    const auto c = std::cos(rotation);

    const auto v0x = pos.x + (-c - s) * size.x, v0y = pos.y + (s - c) * size.y;
    const auto v1x = pos.x + (s - c) * size.x,  v1y = pos.y + (c + s) * size.y;
    const auto v2x = pos.x + (c + s) * size.x,  v2y = pos.y + (c - s) * size.y;
    const auto v3x = pos.x + (c - s) * size.x,  v3y = pos.y + (-c - s) * size.y;

    const auto anyGE0X    = v0x >= 0.f || v1x >= 0.f || v2x >= 0.f || v3x >= 0.f;
    const auto anyGE0Y    = v0y >= 0.f || v1y >= 0.f || v2y >= 0.f || v3y >= 0.f;
    const auto anyLEWidth  = v0x <= SCREEN_WIDTH  || v1x <= SCREEN_WIDTH  || v2x <= SCREEN_WIDTH  || v3x <= SCREEN_WIDTH;
    const auto anyLEHeight = v0y <= SCREEN_HEIGHT || v1y <= SCREEN_HEIGHT || v2y <= SCREEN_HEIGHT || v3y <= SCREEN_HEIGHT;
    if (!(anyGE0X && anyGE0Y && anyLEWidth && anyLEHeight)) {
        return;
    }

    const auto z = (RwIm2DGetFarScreenZ() - RwIm2DGetNearScreenZ())
        * (pos.z - CDraw::ms_fNearClipZ)
        * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * pos.z)
        + RwIm2DGetNearScreenZ();

    const auto emissiveColor = CRGBA{
        static_cast<uint8>((r * intensity) >> 8),
        static_cast<uint8>((g * intensity) >> 8),
        static_cast<uint8>((b * intensity) >> 8),
        alpha
    }.ToIntARGB();

    s_XLUSpriteVertices[0] = { .x = v0x, .y = v0y, .z = z, .rhw = rz, .emissiveColor = emissiveColor };
    s_XLUSpriteVertices[1] = { .x = v1x, .y = v1y, .z = z, .rhw = rz, .emissiveColor = emissiveColor };
    s_XLUSpriteVertices[2] = { .x = v2x, .y = v2y, .z = z, .rhw = rz, .emissiveColor = emissiveColor };
    s_XLUSpriteVertices[3] = { .x = v3x, .y = v3y, .z = z, .rhw = rz, .emissiveColor = emissiveColor };

    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, s_XLUSpriteVertices.data(), 4);
}

// Android
void CSprite::RenderOneXLUSprite_Rotate_Dimension(float, float, float, float, float, uint8, uint8, uint8, int16, float, float, uint8) {
    assert(false);
}

// Android
void CSprite::RenderOneXLUSprite_Rotate_2Colours(float, float, float, float, float, uint8, uint8, uint8, uint8, uint8, uint8, float, float, float, float, uint8) {
    assert(false);
}

// 0x70F540
void CSprite::RenderOneXLUSprite2D(CVector2D screen, CVector2D size, const CRGBA& color, int16 intensity, uint8 alpha) {
    CRGBA vertsColor{};
    for (auto i = 0; i < 4; i++) {
        vertsColor[i] = static_cast<uint8>((intensity * color[i]) >> 8);
    }

    Set4Vertices2D(
        s_XLUSpriteVertices.data(),
        { screen.x - size.x, screen.y - size.y, screen.x + size.x, screen.y + size.y },
        vertsColor,
        vertsColor,
        vertsColor,
        vertsColor
    );

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(false));
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, s_XLUSpriteVertices.data(), 4);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(true));
}

// unused
// 0x70F760
void CSprite::RenderOneXLUSprite2D_Rotate_Dimension(float, float, float, float, const RwRGBA&, int16, float, uint8) {
    assert(false);
}

/* --- Buffered XLU Sprite --- */

// 0x70E4A0
void CSprite::RenderBufferedOneXLUSprite(CVector pos, CVector2D size, uint8 r, uint8 g, uint8 b, int16 intensity, float recipNearZ, uint8 a11) {
    m_bFlushSpriteBufferSwitchZTest = false;

    std::array<float, 4> cornerX{ pos.x - size.x, pos.x - size.x, pos.x + size.x, pos.x + size.x };
    std::array<float, 4> cornerY{ pos.y - size.y, pos.y + size.y, pos.y + size.y, pos.y - size.y };
    std::array<float, 4> u{ 0.f, 0.f, 1.f, 1.f };
    std::array<float, 4> v{ 0.f, 1.f, 1.f, 0.f };

    for (auto i = 0u; i < 4u; i++) {
        if (cornerX[i] < 0.0f) {
            u[i] = (cornerX[i] / size.x) * -0.5f;
            cornerX[i] = 0.0f;
        }
        if (cornerX[i] > SCREEN_WIDTH) {
            u[i] = 1.0f - ((cornerX[i] - SCREEN_WIDTH) * 0.5f) / size.x;
            cornerX[i] = SCREEN_WIDTH;
        }
        if (cornerY[i] < 0.0f) {
            v[i] = (cornerY[i] / size.y) * -0.5f;
            cornerY[i] = 0.0f;
        }
        if (cornerY[i] > SCREEN_HEIGHT) {
            v[i] = 1.0f - ((cornerY[i] - SCREEN_HEIGHT) * 0.5f) / size.y;
            cornerY[i] = SCREEN_HEIGHT;
        }
    }

    const auto z = (pos.z - CDraw::ms_fNearClipZ) * (m_f2DFarScreenZ - m_f2DNearScreenZ) * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * pos.z)
        + m_f2DNearScreenZ;

    const auto emissiveColor = CRGBA{
        static_cast<uint8>((r * intensity) >> 8),
        static_cast<uint8>((g * intensity) >> 8),
        static_cast<uint8>((b * intensity) >> 8),
        a11
    }.ToIntARGB();

    RwD3D9Vertex* vertices = &TempBufferVertices.m_2d[4 * nSpriteBufferIndex];
    for (auto i = 0u; i < 4u; i++) {
        vertices[i] = { .x = cornerX[i], .y = cornerY[i], .z = z, .rhw = recipNearZ, .emissiveColor = emissiveColor, .u = u[i], .v = v[i] };
    }

    auto* indices = &aTempBufferIndices[6 * nSpriteBufferIndex];
    indices[0] = 4 * nSpriteBufferIndex;
    indices[1] = 4 * nSpriteBufferIndex + 1;
    indices[2] = 4 * nSpriteBufferIndex + 2;
    indices[3] = 4 * nSpriteBufferIndex + 3;
    indices[4] = 4 * nSpriteBufferIndex;
    indices[5] = 4 * nSpriteBufferIndex + 2;

    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= 384) {
        CSprite::FlushSpriteBuffer();
    }
}

// 0x70E780
void CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intensity, float recipNearZ, float angle, uint8 a12) {
    m_bFlushSpriteBufferSwitchZTest = false;

    const auto s = std::sin(angle);
    const auto c = std::cos(angle);

    const auto v0x = x + (-c - s) * w, v0y = y + (s - c) * h;
    const auto v1x = x + (s - c) * w,  v1y = y + (c + s) * h;
    const auto v2x = x + (c + s) * w,  v2y = y + (c - s) * h;
    const auto v3x = x + (c - s) * w,  v3y = y + (-c - s) * h;

    const auto anyGE0X     = v0x >= 0.f || v1x >= 0.f || v2x >= 0.f || v3x >= 0.f;
    const auto anyGE0Y     = v0y >= 0.f || v1y >= 0.f || v2y >= 0.f || v3y >= 0.f;
    const auto anyLEWidth  = v0x <= SCREEN_WIDTH  || v1x <= SCREEN_WIDTH  || v2x <= SCREEN_WIDTH  || v3x <= SCREEN_WIDTH;
    const auto anyLEHeight = v0y <= SCREEN_HEIGHT || v1y <= SCREEN_HEIGHT || v2y <= SCREEN_HEIGHT || v3y <= SCREEN_HEIGHT;
    if (!(anyGE0X && anyGE0Y && anyLEWidth && anyLEHeight)) {
        return;
    }

    const auto zRemap = (z - CDraw::ms_fNearClipZ) * (m_f2DFarScreenZ - m_f2DNearScreenZ) * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * z)
        + m_f2DNearScreenZ;

    const auto emissiveColor = CRGBA{
        static_cast<uint8>((r * intensity) >> 8),
        static_cast<uint8>((g * intensity) >> 8),
        static_cast<uint8>((b * intensity) >> 8),
        a12
    }.ToIntARGB();

    RwD3D9Vertex* vertices = &TempBufferVertices.m_2d[4 * nSpriteBufferIndex];
    vertices[0] = { .x = v0x, .y = v0y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = emissiveColor };
    vertices[1] = { .x = v1x, .y = v1y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = emissiveColor };
    vertices[2] = { .x = v2x, .y = v2y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = emissiveColor };
    vertices[3] = { .x = v3x, .y = v3y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = emissiveColor };

    auto* indices = &aTempBufferIndices[6 * nSpriteBufferIndex];
    indices[0] = 4 * nSpriteBufferIndex;
    indices[1] = 4 * nSpriteBufferIndex + 1;
    indices[2] = 4 * nSpriteBufferIndex + 2;
    indices[3] = 4 * nSpriteBufferIndex + 3;
    indices[4] = 4 * nSpriteBufferIndex;
    indices[5] = 4 * nSpriteBufferIndex + 2;

    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= 384) {
        CSprite::FlushSpriteBuffer();
    }
}

// 0x70EAB0
void CSprite::RenderBufferedOneXLUSprite_Rotate_Dimension(CVector pos, CVector2D size, uint8 r, uint8 g, uint8 b, int16 intensity, float rz, float rotation, uint8 a) {
    m_bFlushSpriteBufferSwitchZTest = false;

    const auto s = std::sin(rotation);
    const auto c = std::cos(rotation);

    const auto v0x = pos.x - size.x * c - size.y * s, v0y = pos.y - size.y * c + size.x * s;
    const auto v1x = pos.x - size.x * c + size.y * s, v1y = pos.y + size.y * c + size.x * s;
    const auto v2x = pos.x + size.x * c + size.y * s, v2y = pos.y + size.y * c - size.x * s;
    const auto v3x = pos.x + size.x * c - size.y * s, v3y = pos.y - size.y * c - size.x * s;

    const auto anyGE0X     = v0x >= 0.f || v1x >= 0.f || v2x >= 0.f || v3x >= 0.f;
    const auto anyGE0Y     = v0y >= 0.f || v1y >= 0.f || v2y >= 0.f || v3y >= 0.f;
    const auto anyLEWidth  = v0x <= SCREEN_WIDTH  || v1x <= SCREEN_WIDTH  || v2x <= SCREEN_WIDTH  || v3x <= SCREEN_WIDTH;
    const auto anyLEHeight = v0y <= SCREEN_HEIGHT || v1y <= SCREEN_HEIGHT || v2y <= SCREEN_HEIGHT || v3y <= SCREEN_HEIGHT;
    if (!(anyGE0X && anyGE0Y && anyLEWidth && anyLEHeight)) {
        return;
    }

    const auto zRemap = (pos.z - CDraw::ms_fNearClipZ) * (m_f2DFarScreenZ - m_f2DNearScreenZ) * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * pos.z)
        + m_f2DNearScreenZ;

    const auto emissiveColor = CRGBA{
        static_cast<uint8>((r * intensity) >> 8),
        static_cast<uint8>((g * intensity) >> 8),
        static_cast<uint8>((b * intensity) >> 8),
        a
    }.ToIntARGB();

    RwD3D9Vertex* vertices = &TempBufferVertices.m_2d[4 * nSpriteBufferIndex];
    vertices[0] = { .x = v0x, .y = v0y, .z = zRemap, .rhw = rz, .emissiveColor = emissiveColor };
    vertices[1] = { .x = v1x, .y = v1y, .z = zRemap, .rhw = rz, .emissiveColor = emissiveColor };
    vertices[2] = { .x = v2x, .y = v2y, .z = zRemap, .rhw = rz, .emissiveColor = emissiveColor };
    vertices[3] = { .x = v3x, .y = v3y, .z = zRemap, .rhw = rz, .emissiveColor = emissiveColor };

    auto* indices = &aTempBufferIndices[6 * nSpriteBufferIndex];
    indices[0] = 4 * nSpriteBufferIndex;
    indices[1] = 4 * nSpriteBufferIndex + 1;
    indices[2] = 4 * nSpriteBufferIndex + 2;
    indices[3] = 4 * nSpriteBufferIndex + 3;
    indices[4] = 4 * nSpriteBufferIndex;
    indices[5] = 4 * nSpriteBufferIndex + 2;

    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= 384) {
        CSprite::FlushSpriteBuffer();
    }
}

// 0x70EDE0
void CSprite::RenderBufferedOneXLUSprite_Rotate_2Colours(float x, float y, float z, float w, float h, uint8 r1, uint8 g1, uint8 b1, uint8 r2, uint8 g2, uint8 b2, float dirX, float dirY, float recipNearZ, float rotation, uint8 a) {
    m_bFlushSpriteBufferSwitchZTest = false;

    const auto s = std::sin(rotation);
    const auto c = std::cos(rotation);

    // Same "Aspect" quad shape as RenderBufferedOneXLUSprite_Rotate_Aspect; kept as the raw unit
    // offsets too, since they double as the per-vertex gradient-projection axis below.
    const auto V0X = -c - s, V0Y = s - c;
    const auto V1X = s - c,  V1Y = c + s;
    const auto V2X = c + s,  V2Y = c - s;
    const auto V3X = c - s,  V3Y = -c - s;

    const auto v0x = x + V0X * w, v0y = y + V0Y * h;
    const auto v1x = x + V1X * w, v1y = y + V1Y * h;
    const auto v2x = x + V2X * w, v2y = y + V2Y * h;
    const auto v3x = x + V3X * w, v3y = y + V3Y * h;

    const auto anyGE0X     = v0x >= 0.f || v1x >= 0.f || v2x >= 0.f || v3x >= 0.f;
    const auto anyGE0Y     = v0y >= 0.f || v1y >= 0.f || v2y >= 0.f || v3y >= 0.f;
    const auto anyLEWidth  = v0x <= SCREEN_WIDTH  || v1x <= SCREEN_WIDTH  || v2x <= SCREEN_WIDTH  || v3x <= SCREEN_WIDTH;
    const auto anyLEHeight = v0y <= SCREEN_HEIGHT || v1y <= SCREEN_HEIGHT || v2y <= SCREEN_HEIGHT || v3y <= SCREEN_HEIGHT;
    if (!(anyGE0X && anyGE0Y && anyLEWidth && anyLEHeight)) {
        return;
    }

    const auto zRemap = (z - CDraw::ms_fNearClipZ) * (m_f2DFarScreenZ - m_f2DNearScreenZ) * CDraw::ms_fFarClipZ
        / ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * z)
        + m_f2DNearScreenZ;

    // Blend weight for this corner = how far it projects onto the (dirX, dirY) gradient axis,
    // remapped from a [-1, 1]-ish dot product into [0, 1] and clamped.
    const auto BlendCorner = [&](float cx, float cy, uint8 c1, uint8 c2) {
        const auto bf = std::clamp(0.5f * (1.0f + dirX * cx + dirY * cy), 0.0f, 1.0f);
        return static_cast<uint8>(std::lround(c2 * (1.0f - bf) + c1 * bf));
    };
    const auto MakeColor = [&](float cx, float cy) {
        return CRGBA{ BlendCorner(cx, cy, r1, r2), BlendCorner(cx, cy, g1, g2), BlendCorner(cx, cy, b1, b2), a }.ToIntARGB();
    };

    RwD3D9Vertex* vertices = &TempBufferVertices.m_2d[4 * nSpriteBufferIndex];
    vertices[0] = { .x = v0x, .y = v0y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = MakeColor(V0X, V0Y), .u = 0.0f, .v = 0.0f };
    vertices[1] = { .x = v1x, .y = v1y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = MakeColor(V1X, V1Y), .u = 0.0f, .v = 1.0f };
    vertices[2] = { .x = v2x, .y = v2y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = MakeColor(V2X, V2Y), .u = 1.0f, .v = 1.0f };
    vertices[3] = { .x = v3x, .y = v3y, .z = zRemap, .rhw = recipNearZ, .emissiveColor = MakeColor(V3X, V3Y), .u = 1.0f, .v = 0.0f };

    auto* indices = &aTempBufferIndices[6 * nSpriteBufferIndex];
    indices[0] = 4 * nSpriteBufferIndex;
    indices[1] = 4 * nSpriteBufferIndex + 1;
    indices[2] = 4 * nSpriteBufferIndex + 2;
    indices[3] = 4 * nSpriteBufferIndex + 3;
    indices[4] = 4 * nSpriteBufferIndex;
    indices[5] = 4 * nSpriteBufferIndex + 2;

    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= 384) {
        CSprite::FlushSpriteBuffer();
    }
}

// 0x70F440
void CSprite::RenderBufferedOneXLUSprite2D(CVector2D pos, CVector2D size, const RwRGBA& color, int16 intensity, uint8 alpha) {
    m_bFlushSpriteBufferSwitchZTest = true;
    const CRect rect(pos, size.x);
    const CRGBA scaledColor(
        (color.red * intensity) >> 8,
        (color.green * intensity) >> 8,
        (color.blue * intensity) >> 8,
        alpha
    );
    RwD3D9Vertex* vertices = &TempBufferVertices.m_2d[4 * nSpriteBufferIndex];
    Set4Vertices2D(
        vertices, rect, scaledColor, scaledColor, scaledColor, scaledColor
    );

    auto* indices = &aTempBufferIndices[6 * nSpriteBufferIndex];
    indices[0] = 4 * nSpriteBufferIndex;
    indices[1] = 4 * nSpriteBufferIndex + 1;
    indices[2] = 4 * nSpriteBufferIndex + 2;
    indices[3] = 4 * nSpriteBufferIndex + 2;
    indices[4] = 4 * nSpriteBufferIndex;
    indices[5] = 4 * nSpriteBufferIndex + 3;
    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= 384) {
        CSprite::FlushSpriteBuffer();
    }
}

// unused
// 0x70F600
void CSprite::RenderBufferedOneXLUSprite2D_Rotate_Dimension(float, float, float, float, const RwRGBA&, int16, float, uint8) {
    assert(false);
}
