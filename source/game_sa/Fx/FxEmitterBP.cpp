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
    RH_ScopedVMTInstall(Render, 0x4A2C40, { .Reversed = false });
    RH_ScopedVMTInstall(FreePrtFromPrim, 0x4A2510);
}

// 0x4A18D0
FxEmitterBP_c::FxEmitterBP_c() : FxPrimBP_c() {
    m_Type = 0;
}

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

    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        auto* const prt = it->AsFxEmitterPrt();

        CVector prtPos;
        if (prt->m_bLocalToSystem) {
            auto* const mat = g_fxMan.FxRwMatrixCreate();
            prt->m_System->GetCompositeMatrix(mat);
            RwV3dTransformPoints(&prtPos, &prt->m_Pos, 1, mat);
            g_fxMan.FxRwMatrixDestroy(mat);
        } else {
            prtPos = prt->m_Pos;
        }

        RenderInfo_t renderInfo;
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        // Where the particle faces: `at` is the axis it spins around, `right` and `up` span the sprite
        CVector right, up, at;
        if (!renderInfo.m_bHasDir) {
            const auto& faceMat = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : camMat;
            right = faceMat.right;
            up    = faceMat.up;
            at    = faceMat.at;
        } else {
            // Stands along a direction, turned to the camera as much as it can
            CVector dir = renderInfo.m_bUseVel ? prt->m_Velocity : renderInfo.m_Direction;
            RwV3dNormalize(&dir, &dir);

            CVector toCam{ prtPos.x - camMat.pos.x, prtPos.y - camMat.pos.y, prtPos.z - camMat.pos.z };
            RwV3dNormalize(&toCam, &toCam);

            right = CVector{
                toCam.z * dir.y - toCam.y * dir.z,
                toCam.x * dir.z - toCam.z * dir.x,
                toCam.y * dir.x - toCam.x * dir.y
            };
            at = CVector{
                right.z * dir.y - right.y * dir.z,
                right.x * dir.z - right.z * dir.x,
                right.y * dir.x - right.x * dir.y
            };
            up = dir;
        }

        // Rotation is kept in degrees, in the range [0, 360)
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
            RotateVecAboutVec(rotatedRight, right, at, prt->m_CurrentRotation * 0.01745328f);
            right = rotatedRight;
            up    = CVector{
                at.y * right.z - at.z * right.y,
                at.z * right.x - at.x * right.z,
                at.x * right.y - at.y * right.x
            };
        }

        // The size varies a bit between particles, and it can be scaled down
        auto sizeX = ((float)prt->m_RandR * 0.003921569f - 0.5f) * renderInfo.m_fSizeXBias + renderInfo.m_fSizeX;
        auto sizeY = ((float)prt->m_RandG * 0.003921569f - 0.5f) * renderInfo.m_fSizeYBias + renderInfo.m_fSizeY;
        if (const auto multSize = std::bit_cast<uint8>(prt->m_MultSize); multSize != 0xFF) {
            sizeX = (float)multSize * 0.003921569f * sizeX;
            sizeY = (float)multSize * 0.003921569f * sizeY;
        }

        // The texture can change over the life of the particle
        const auto texId = renderInfo.m_nCurrentTexId;
        auto*      tex   = m_apTextures[0];
        if (renderInfo.m_bHasAnimTextures && texId >= 2 && texId <= 4 && m_apTextures[texId - 1]) {
            tex = m_apTextures[texId - 1];
        }
        auto* const raster = RwTextureGetRaster(tex);
        if (raster != curRaster) {
            RenderEnd();
            curRaster = raster;
            RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);
        }

        // Sides of the sprite, scaled by the size
        const auto top    = renderInfo.m_fSpriteTop * sizeY;
        const auto bottom = renderInfo.m_fSpriteBottom * sizeY;
        const auto left   = renderInfo.m_fSpriteLeft * sizeX;
        const auto rightK = renderInfo.m_fSpriteRight * sizeX;

        const CVector bottomUp{ up.x * bottom, up.y * bottom, bottom * up.z };
        const CVector leftRight{ right.x * left, right.y * left, left * right.z };
        const CVector rightRight{ right.x * rightK, right.y * rightK, 0.0f }; // The height is left out, it is added where used

        const CVector topLeft{
            (leftRight.x + up.x * top) + prtPos.x,
            prtPos.y + (leftRight.y + top * up.y),
            prtPos.z + (leftRight.z + top * up.z)
        };
        const CVector bottomRight{
            (rightRight.x + bottomUp.x) + prtPos.x,
            prtPos.y + (rightRight.y + bottomUp.y),
            prtPos.z + (bottomUp.z + rightK * right.z)
        };
        const CVector topRight{
            (rightRight.x + up.x * top) + prtPos.x,
            (rightRight.y + top * up.y) + prtPos.y,
            (rightK * right.z + top * up.z) + prtPos.z
        };
        const CVector bottomLeft{
            (leftRight.x + bottomUp.x) + prtPos.x,
            (leftRight.y + bottomUp.y) + prtPos.y,
            (leftRight.z + bottomUp.z) + prtPos.z
        };

        // Black, only the alpha matters
        const CRGBA color{ 0, 0, 0, renderInfo.m_Color1.alpha };
        RenderAddTri(topLeft,     bottomRight, topRight,   { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, color, color, color);
        RenderAddTri(bottomRight, topLeft,     bottomLeft, { 1.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f }, color, color, color);
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
    return plugin::CallMethod<0x4A2C40, FxEmitterBP_c*, RwCamera*, uint32, float, bool>(this, camera, txdHashKey, brightness, doHeatHaze);

    /*
    static constexpr RwBlendFunction g_BlendFunctions[] = {
        rwBLENDZERO,      rwBLENDONE,          rwBLENDSRCCOLOR,  rwBLENDINVSRCCOLOR,  rwBLENDSRCALPHA, rwBLENDINVSRCALPHA,
        rwBLENDDESTALPHA, rwBLENDINVDESTALPHA, rwBLENDDESTCOLOR, rwBLENDINVDESTCOLOR, rwBLENDSRCALPHASAT
    };

    if (doHeatHaze) {
        if (m_FxInfoManager.m_bHasHeatHazeParticleEmitter)
            RenderHeatHaze(camera, txdHashKey, brightness);
        return;
    }

    if (IsFxInfoPresent(FX_INFO_HEATHAZE_DATA)) {
        if (m_Particles.GetNumItems())
            g_fxMan.m_bHeatHazeEnabled = true;
        return;
    }

    if (!m_Particles.GetNumItems())
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(m_bAlphaOn));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(g_BlendFunctions[m_bAlphaOn ? m_nSrcBlendId : 1]));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(g_BlendFunctions[m_bAlphaOn ? m_nDstBlendId : 0]));

    auto* raster = RwTextureGetRaster(m_apTextures[0]);
    RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);

    for (auto* prt = (FxEmitterPrt_c*)m_Particles.GetHead(); prt; prt = (FxEmitterPrt_c*)m_Particles.GetNext(prt)) {
        const auto pos = [prt] {
            if (prt->m_bLocalToSystem) {
                // Get updated matrix position.
                CVector out{};
                auto* mat = g_fxMan.FxRwMatrixCreate();
                prt->m_System->GetCompositeMatrix(mat);
                RwV3dTransformPoint(&out, &prt->m_Pos, mat); // SA: RwV3dTransformPoints(...,...,1,...)
                g_fxMan.FxRwMatrixDestroy(mat);
                return out;
            } else {
                return prt->m_Pos;
            }
        }();

        RenderInfo_t renderInfo{};
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        if (renderInfo.m_SmokeType > -1) {
            // RenderSmoke();
            CGeneral::GetRandomNumberInRange(0.0f, 1.0f) *
        }
    }

    RenderEnd();
    */
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
