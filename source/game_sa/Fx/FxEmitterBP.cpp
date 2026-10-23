#include "StdInc.h"

#include "FxEmitterBP.h"
#include "FxEmitter.h"
#include "FxPrimBP.h"
#include "FxEmitterPrt.h"
#include "FxInfo.h"
#include "FxInfoManager.h"

#include "Particle.h"
#include "WaterLevel.h"
#include "MovementInfo.h"
#include "FxSystem.h"
#include "FxSystemBP.h"
#include "FxTools.h"
#include "Fx.h"
#include "General.h"
#include "Timer.h"
#include "Camera.h"

void FxEmitterBP_c::InjectHooks() {
    RH_ScopedVirtualClass(FxEmitterBP_c, 0x85A788, 7);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(Constructor, 0x4A18D0);
    RH_ScopedInstall(RenderHeatHaze, 0x4A1940);
    RH_ScopedInstall(UpdateParticle, 0x4A21D0);
    RH_ScopedVMTInstall(CreateInstance, 0x4A2B40);
    RH_ScopedVMTInstall(Update, 0x4A2BC0);
    RH_ScopedVMTInstall(Load, 0x5C25F0);
    RH_ScopedVMTInstall(LoadTextures, 0x5C0A30);
    RH_ScopedVMTInstall(Render, 0x4A2C40);
    RH_ScopedVMTInstall(FreePrtFromPrim, 0x4A2510);
}

// 0x4A18D0
FxEmitterBP_c::FxEmitterBP_c() : FxPrimBP_c() {
    m_Type = 0;
}

namespace {
// The axes of a sprite, `at` is the axis it is spun around
struct ParticleAxes {
    CVector right, up, at;
};

// For a sprite that stands along `dir` and turns to the camera as much as it can
ParticleAxes GetAxesAlongDir(CVector dir, CVector toCam) {
    RwV3dNormalize(&dir, &dir);
    RwV3dNormalize(&toCam, &toCam);

    ParticleAxes axes;
    axes.right = CVector{
        toCam.z * dir.y - toCam.y * dir.z,
        toCam.x * dir.z - toCam.z * dir.x,
        toCam.y * dir.x - toCam.x * dir.y
    };
    axes.at = CVector{
        axes.right.z * dir.y - axes.right.y * dir.z,
        axes.right.x * dir.z - axes.right.z * dir.x,
        axes.right.y * dir.x - axes.right.x * dir.y
    };
    axes.up = dir;
    return axes;
}

// Turns the sprite by the rotation of the particle, which is kept in degrees in the range [0, 360)
void SpinParticleAxes(FxEmitterPrt_c* prt, ParticleAxes& axes) {
    if (prt->m_RotZ != -1) {
        prt->m_CurrentRotation = (float)(uint8)prt->m_RotZ * 2.0f;
    }
    while (prt->m_CurrentRotation < 0.0f) {
        prt->m_CurrentRotation += 360.0f;
    }
    while (prt->m_CurrentRotation >= 360.0f) {
        prt->m_CurrentRotation -= 360.0f;
    }
    if (prt->m_CurrentRotation > 0.0f) {
        CVector rotatedRight;
        RotateVecAboutVec(rotatedRight, axes.right, axes.at, prt->m_CurrentRotation * 0.01745328f);
        axes.right = rotatedRight;
        axes.up    = CVector{
            axes.at.y * axes.right.z - axes.at.z * axes.right.y,
            axes.at.z * axes.right.x - axes.at.x * axes.right.z,
            axes.at.x * axes.right.y - axes.at.y * axes.right.x
        };
    }
}

// The size varies a bit between particles, and it can be scaled down
CVector2D GetParticleSize(const FxEmitterPrt_c* prt, const RenderInfo_t& renderInfo) {
    CVector2D size{
        ((float)prt->m_RandR * 0.003921569f - 0.5f) * renderInfo.m_fSizeXBias + renderInfo.m_fSizeX,
        ((float)prt->m_RandG * 0.003921569f - 0.5f) * renderInfo.m_fSizeYBias + renderInfo.m_fSizeY
    };
    if (const auto multSize = std::bit_cast<uint8>(prt->m_MultSize); multSize != 0xFF) {
        size.x = (float)multSize * 0.003921569f * size.x;
        size.y = (float)multSize * 0.003921569f * size.y;
    }
    return size;
}

// The texture can change over the life of the particle
RwRaster* GetParticleRaster(const std::array<RwTexture*, 4>& textures, const RenderInfo_t& renderInfo, RwRaster* current) {
    auto* tex = textures[0];
    if (renderInfo.m_bHasAnimTextures) {
        const auto id = renderInfo.m_nCurrentTexId;
        if (id >= 2 && id <= 4) {
            if (textures[id - 1]) {
                tex = textures[id - 1];
            }
        } else if (id != 1) {
            return current; // Not a texture, the original used whatever was on the stack
        }
    }
    return RwTextureGetRaster(tex);
}

// Two triangles for the sprite, the sides are scaled by the size
void AddParticleQuad(const CVector& pos, const ParticleAxes& axes, const CVector2D& size, float top, float bottom, float left, float right, const CRGBA& color) {
    const auto& [ax, up, at] = axes;
    const auto kt = top * size.y;
    const auto kb = bottom * size.y;
    const auto kl = left * size.x;
    const auto kr = right * size.x;

    const CVector bottomUp{ up.x * kb, up.y * kb, kb * up.z };
    const CVector leftRight{ ax.x * kl, ax.y * kl, kl * ax.z };
    const CVector rightRight{ ax.x * kr, ax.y * kr, 0.0f }; // The height is added where used

    const CVector topLeft{
        (leftRight.x + up.x * kt) + pos.x,
        (leftRight.y + kt * up.y) + pos.y,
        (leftRight.z + kt * up.z) + pos.z
    };
    const CVector bottomRight{
        (rightRight.x + bottomUp.x) + pos.x,
        (rightRight.y + bottomUp.y) + pos.y,
        (bottomUp.z + kr * ax.z) + pos.z
    };
    const CVector topRight{
        (rightRight.x + up.x * kt) + pos.x,
        (rightRight.y + kt * up.y) + pos.y,
        (kr * ax.z + kt * up.z) + pos.z
    };
    const CVector bottomLeft{
        (leftRight.x + bottomUp.x) + pos.x,
        (leftRight.y + bottomUp.y) + pos.y,
        (leftRight.z + bottomUp.z) + pos.z
    };

    RenderAddTri(topLeft,     bottomRight, topRight,   { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, color, color, color);
    RenderAddTri(bottomRight, topLeft,     bottomLeft, { 1.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f }, color, color, color);
}

// Where the particle is in the world
CVector GetParticleWorldPos(const FxEmitterPrt_c* prt) {
    if (!prt->m_bLocalToSystem) {
        return prt->m_Pos;
    }
    CVector pos;
    auto* const mat = g_fxMan.FxRwMatrixCreate();
    prt->m_System->GetCompositeMatrix(mat);
    RwV3dTransformPoints(&pos, &prt->m_Pos, 1, mat);
    g_fxMan.FxRwMatrixDestroy(mat);
    return pos;
}
} // namespace

// 0x4A1940
void FxEmitterBP_c::RenderHeatHaze(RwCamera* camera, uint32 txdHashKey, float brightness) {
    if (!m_Particles.GetNumItems()) {
        return;
    }

    // The haze is added over the screen
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDONE));

    auto* curRaster = RwTextureGetRaster(m_apTextures[0]);
    RenderBegin(curRaster, nullptr, rwIM3D_VERTEXUV);

    const auto& camMat = *RwFrameGetMatrix(RwCameraGetFrame(camera));
    const CVector camPos{ camMat.pos };

    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        auto* const prt    = it->AsFxEmitterPrt();
        const auto  prtPos = GetParticleWorldPos(prt);

        RenderInfo_t renderInfo;
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        ParticleAxes axes;
        if (!renderInfo.m_bHasDir) {
            const auto& faceMat = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : camMat;
            axes = { CVector{ faceMat.right }, CVector{ faceMat.up }, CVector{ faceMat.at } };
        } else {
            axes = GetAxesAlongDir(renderInfo.m_bUseVel ? prt->m_Velocity : renderInfo.m_Direction, prtPos - camPos);
        }
        SpinParticleAxes(prt, axes);

        auto* const raster = GetParticleRaster(m_apTextures, renderInfo, curRaster);
        if (raster != curRaster) {
            RenderEnd();
            curRaster = raster;
            RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);
        }

        // Black, only the alpha matters
        AddParticleQuad(
            prtPos, axes, GetParticleSize(prt, renderInfo),
            renderInfo.m_fSpriteTop, renderInfo.m_fSpriteBottom, renderInfo.m_fSpriteLeft, renderInfo.m_fSpriteRight,
            CRGBA{ 0, 0, 0, renderInfo.m_Color1.alpha }
        );
    }

    RenderEnd();
}

// 0x4A21D0
bool FxEmitterBP_c::UpdateParticle(float deltaTime, FxEmitterPrt_c* emitter) {
    auto* const system = emitter->m_System;

    // Returns true when the particle is done and has to be removed
    deltaTime = (float)system->m_nTimeMult * 0.001f * deltaTime;
    emitter->m_fCurrentLife += deltaTime;
    if (emitter->m_fTotalLife <= emitter->m_fCurrentLife) {
        return true;
    }

    emitter->m_Pos.x += deltaTime * emitter->m_Velocity.x;
    emitter->m_Pos.y += deltaTime * emitter->m_Velocity.y;
    emitter->m_Pos.z += deltaTime * emitter->m_Velocity.z;

    MovementInfo_t movement;
    movement.m_Pos = emitter->m_Pos;
    movement.m_Vel = emitter->m_Velocity;
    m_FxInfoManager.ProcessMovementInfo(system->m_fCurrentTime, emitter->m_fCurrentLife / emitter->m_fTotalLife, deltaTime, system->m_SystemBP->m_fLength, false, &movement);
    emitter->m_Pos      = movement.m_Pos;
    emitter->m_Velocity = movement.m_Vel;

    if (movement.m_bHasFloatInfo || movement.m_bHasUnderwaterInfo) {
        float      waterLevel;
        const auto isWater = CWaterLevel::GetWaterLevel(emitter->m_Pos.x, emitter->m_Pos.y, emitter->m_Pos.z, waterLevel, true, nullptr);
        if (movement.m_bHasFloatInfo && isWater && emitter->m_Pos.z < waterLevel) {
            emitter->m_Pos.z = waterLevel;
        }
        if (movement.m_bHasUnderwaterInfo && (!isWater || waterLevel < emitter->m_Pos.z)) {
            return true;
        }
    }

    // Spinning, the range is picked by the random value of the particle
    const auto& [rotMinA, rotMaxA, rotMinB, rotMaxB] = movement.m_Rot;
    const auto  random = (float)emitter->m_RandR;
    const auto  mult   = (float)emitter->m_MultRot;
    const auto  Spin   = [&](float min, float max, float t) {
        return ((max - min) * t + min) * mult;
    };

    float spin;
    if ((rotMinA <= 0.0f && rotMaxA <= 0.0f) || (rotMinB <= 0.0f && rotMaxB <= 0.0f)) {
        if (rotMinA > 0.0f || rotMaxA > 0.0f) {
            emitter->m_CurrentRotation += Spin(rotMinA, rotMaxA, random / 255.0f) * deltaTime;
            return false;
        }
        if (rotMinB <= 0.0f && rotMaxB <= 0.0f) {
            return false;
        }
        spin = Spin(rotMinB, rotMaxB, random / 255.0f);
    } else {
        if (emitter->m_RandR < 0x80) {
            emitter->m_CurrentRotation += Spin(rotMinA, rotMaxA, random * 0.0078125f) * deltaTime;
            return false;
        }
        spin = Spin(rotMinB, rotMaxB, (random - 128.0f) * 0.0078125f);
    }
    emitter->m_CurrentRotation -= spin * deltaTime;
    return false;
}

// 0x4A2B40


FxPrim_c* FxEmitterBP_c::CreateInstance() {
    return new FxEmitter_c();
}

// 0x4A2BC0


void FxEmitterBP_c::Update(float deltaTime) {
    for (auto* it = m_Particles.GetHead(); it; ) {
        // NB: Must grab `next` before a potential `ReturnParticle()` below, since that
        // adds `it` to another list, overwriting `it`'s own `m_pNext`.
        auto* const next = m_Particles.GetNext(it);

        if (it->m_System->m_nKillStatus == eFxSystemKillStatus::FX_3) {
            it->m_System->m_nKillStatus = eFxSystemKillStatus::FX_KILLED;
        }

        if (it->m_System->m_nPlayStatus != eFxSystemPlayStatus::T2 && UpdateParticle(deltaTime, reinterpret_cast<FxEmitterPrt_c*>(it))) {
            m_Particles.RemoveItem(it);
            g_fxMan.ReturnParticle(reinterpret_cast<FxEmitterPrt_c*>(it));
        }

        it = next;
    }
}

// 0x5C25F0


bool FxEmitterBP_c::Load(FILESTREAM file, int32 version, FxName32_t* textureNames) {
    FxPrimBP_c::Load(file, version, textureNames);

    m_nLodStart = uint16(ReadField<float>(file, "LODSTART:") * 64.0f);
    m_nLodEnd   = uint16(ReadField<float>(file, "LODEND:") * 64.0f);

    return true;
}

// 0x5C0A30


bool FxEmitterBP_c::LoadTextures(FxName32_t* textureNames, int32 version) {
    assert(textureNames);

    const auto LoadTexture = [&](auto ind) -> RwTexture* {
        char mask[64];
        sprintf(mask, "%sm", textureNames[ind]);

        auto* texture = RwTextureRead(textureNames[ind], mask);
        return texture ? texture : RwTextureRead(textureNames[ind], nullptr);
    };

    const auto LoadTextureIfExists = [=](auto ind) -> RwTexture* {
        assert(&textureNames[ind]);
        return strncmp(textureNames[ind], "NULL", 5u) != 0 ? LoadTexture(ind) : nullptr;
    };

    m_apTextures[0] = LoadTexture(0);

    if (version > 101) {
        m_apTextures[1] = LoadTextureIfExists(1);
        m_apTextures[2] = LoadTextureIfExists(2);
        m_apTextures[3] = LoadTextureIfExists(3);
    }

    return true;
}

// 0x4A2C40
void FxEmitterBP_c::Render(RwCamera* camera, uint32 txdHashKey, float brightness, bool doHeatHaze) {
    static constexpr RwBlendFunction BLEND_FUNCTIONS[] = {
        rwBLENDZERO,      rwBLENDONE,          rwBLENDSRCCOLOR,  rwBLENDINVSRCCOLOR,  rwBLENDSRCALPHA, rwBLENDINVSRCALPHA,
        rwBLENDDESTALPHA, rwBLENDINVDESTALPHA, rwBLENDDESTCOLOR, rwBLENDINVDESTCOLOR, rwBLENDSRCALPHASAT
    };

    if (doHeatHaze) {
        if (m_FxInfoManager.m_bHasHeatHazeParticleEmitter) {
            RenderHeatHaze(camera, txdHashKey, brightness);
        }
        return;
    }

    // The heat haze is drawn in a pass of its own, here it is only asked for
    if (IsFxInfoPresent(FX_INFO_HEATHAZE_DATA)) {
        if (m_Particles.GetNumItems() > 0) {
            g_fxMan.m_bHeatHazeEnabled = true;
        }
        return;
    }

    if (!m_Particles.GetNumItems()) {
        return;
    }

    auto* curRaster = RwTextureGetRaster(m_apTextures[0]);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(m_bAlphaOn));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(BLEND_FUNCTIONS[m_bAlphaOn ? m_nSrcBlendId : 1]));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(BLEND_FUNCTIONS[m_bAlphaOn ? m_nDstBlendId : 0]));
    RenderBegin(curRaster, nullptr, rwIM3D_VERTEXUV);

    const auto& camMat = *RwFrameGetMatrix(RwCameraGetFrame(camera));
    const CVector camPos{ camMat.pos };

    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        auto* const prt    = it->AsFxEmitterPrt();
        const auto  prtPos = GetParticleWorldPos(prt);

        RenderInfo_t renderInfo;
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        if (renderInfo.m_SmokeType > -1) {
            // Puffs of smoke are left behind, the color varies a bit
            const auto variation = CGeneral::GetRandomNumberInRange(-renderInfo.m_SmokeBrightness, renderInfo.m_SmokeBrightness) * 0.003921569f;
            const auto Channel   = [&](float value) {
                return std::clamp(value * 0.003921569f + variation, 0.0f, 1.0f);
            };
            const FxPrtMult_c smokeMult{
                Channel(renderInfo.m_SmokeColor.red),
                Channel(renderInfo.m_SmokeColor.green),
                Channel(renderInfo.m_SmokeColor.blue),
                renderInfo.m_SmokeColor.alpha * 0.003921569f,
                renderInfo.m_SmokeSize,
                1.0f,
                renderInfo.m_SmokeLife
            };
            g_fx.m_SmokeHuge->AddParticle(prt->m_Pos, CVector{ 0.0f, 0.0f, 0.0f }, 0.0f, smokeMult, -1.0f, 1.2f, 0.6f, false);
        }

        // The trail and the direction work with the position of the particle in its own space,
        // even if it's not in the same one as the camera
        const CVector& rawPos = prt->m_Pos;

        ParticleAxes axes;
        float        trailLength = 0.0f;
        if (renderInfo.m_nTrailScreenMode > 0) {
            CVector trail;
            if (renderInfo.m_nTrailScreenMode == 2) {
                // Takes the movement of the camera since the last frame into account
                const auto step   = CTimer::GetTimeStep() * 0.02f;
                const auto toCam  = rawPos - camPos;
                const auto before = (rawPos - prt->m_Velocity * step) - TheCamera.m_mCameraMatrixOld.GetPosition();
                trail             = (toCam - before) * renderInfo.m_fTrailTime;
            } else {
                trail = rawPos - (rawPos - prt->m_Velocity * renderInfo.m_fTrailTime);
            }
            trailLength = RwV3dLength(&trail);
            if (trail.IsZero()) {
                trail.z = 1.0f;
            }
            axes = GetAxesAlongDir(trail, rawPos - camPos);
        } else if (renderInfo.m_bHasDir) {
            CVector dir = renderInfo.m_bUseVel ? prt->m_Velocity : renderInfo.m_Direction;
            if (renderInfo.m_bUseVel && dir.IsZero()) {
                dir.z = 1.0f;
            }
            axes = GetAxesAlongDir(dir, prtPos - camPos);
        } else {
            const auto& faceMat = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : camMat;
            axes = { CVector{ faceMat.right }, CVector{ faceMat.up }, CVector{ faceMat.at } };
        }
        SpinParticleAxes(prt, axes);

        const auto size = GetParticleSize(prt, renderInfo);

        // The color, it can vary with the particle
        float red   = renderInfo.m_Color1.red;
        float green = renderInfo.m_Color1.green;
        float blue  = renderInfo.m_Color1.blue;
        float alpha = renderInfo.m_Color1.alpha;
        switch (renderInfo.m_nColorType) {
        case ERenderColorType::RANGE:
            red   = ((float)renderInfo.m_Color2.red - red) * ((float)prt->m_RandR * 0.003921569f) + red;
            green = ((float)renderInfo.m_Color2.green - green) * ((float)prt->m_RandG * 0.003921569f) + green;
            blue  = ((float)renderInfo.m_Color2.blue - blue) * ((float)prt->m_RandB * 0.003921569f) + blue;
            break;
        case ERenderColorType::BRIGHT: {
            const auto delta = ((float)prt->m_RandR * 0.0078125f - 1.0f) * (float)renderInfo.m_Color2.alpha;
            red   = std::clamp(red + delta, 0.0f, 255.0f);
            green = std::clamp(green + delta, 0.0f, 255.0f);
            blue  = std::clamp(blue + delta, 0.0f, 255.0f);
            break;
        }
        default:
            break;
        }
        if (prt->m_MultColor.r < 0xFF) { red   = (float)prt->m_MultColor.r * 0.003921569f * red; }
        if (prt->m_MultColor.g < 0xFF) { green = (float)prt->m_MultColor.g * 0.003921569f * green; }
        if (prt->m_MultColor.b < 0xFF) { blue  = (float)prt->m_MultColor.b * 0.003921569f * blue; }
        if (prt->m_MultColor.a < 0xFF) { alpha = (float)prt->m_MultColor.a * 0.003921569f * alpha; }

        // Not lit by itself, so the brightness of the particle goes in. It stays for the next
        // particles, if the brightness of one of them is out of range
        float litRed = red, litGreen = green, litBlue = blue;
        if (!renderInfo.m_bSelfLit) {
            if (const auto particleBrightness = std::bit_cast<uint8>(prt->m_Brightness); particleBrightness <= 100) {
                brightness = (float)particleBrightness * 0.01f;
            }
            litRed   = red * brightness;
            litGreen = green * brightness;
            litBlue  = blue * brightness;
        }
        const CRGBA color{ (uint8)(int32)litRed, (uint8)(int32)litGreen, (uint8)(int32)litBlue, (uint8)(int32)alpha };

        auto* const raster = GetParticleRaster(m_apTextures, renderInfo, curRaster);
        if (raster != curRaster) {
            RenderEnd();
            curRaster = raster;
            RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);
        }

        // A trail is as long as the movement, and starts where the particle is
        const auto top    = renderInfo.m_nTrailScreenMode > 0 ? trailLength : renderInfo.m_fSpriteTop;
        const auto bottom = renderInfo.m_nTrailScreenMode > 0 ? 0.0f : renderInfo.m_fSpriteBottom;
        AddParticleQuad(prtPos, axes, size, top, bottom, renderInfo.m_fSpriteLeft, renderInfo.m_fSpriteRight, color);
    }

    RenderEnd();
}

// 0x4A2510

// 0x0
bool FxEmitterBP_c::FreePrtFromPrim(FxSystem_c* system) {
    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        if (it->m_System == system) {
            m_Particles.RemoveItem(it);
            g_fxMan.ReturnParticle(reinterpret_cast<FxEmitterPrt_c*>(it));
            return true;
        }
    }
    return false;
}

// todo: eFxInfo
// 0x4A24D0
bool FxEmitterBP_c::IsFxInfoPresent(eFxInfoType type) const {
    if (m_FxInfoManager.m_nNumInfos <= 0)
        return false;

    for (auto& info : m_FxInfoManager.GetInfos()) {
        if (info->m_nType == type) {
            return true;
        }
    }
    return false;
}
