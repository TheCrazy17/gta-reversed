#include "StdInc.h"

#include "FxEmitter.h"
#include "EmissionInfo.h"
#include "FxEmitterPrt.h"
#include "FxPrimBP.h"
#include "FxEmitterBP.h"
#include "FxEmitterPrt.h"

void FxEmitter_c::InjectHooks() {
    RH_ScopedVirtualClass(FxEmitter_c, 0x85A7A4, 6);
    RH_ScopedCategory("Fx");

    RH_ScopedVMTInstall(Init, 0x4A2550);
    RH_ScopedVMTInstall(Update, 0x4A4460);
    RH_ScopedVMTInstall(Reset, 0x4A2570);
    RH_ScopedVMTOverloadedInstall(AddParticle, "matrix", 0x4A4050, void(FxEmitter_c::*)(const RwMatrix&, const CVector&, float, const FxPrtMult_c&, float, float, bool));
    RH_ScopedVMTOverloadedInstall(AddParticle, "pos", 0x4A3EA0, void(FxEmitter_c::*)(const CVector&, const CVector&, float, const FxPrtMult_c&, float, float, bool));
    RH_ScopedInstall(CreateParticles, 0x4A41E0);
    RH_ScopedInstall(CreateParticle, 0x4A2580);
}


// 0x4A2550
bool FxEmitter_c::Init(FxPrimBP_c* primBP, FxSystem_c* system) {
    m_PrimBP             = primBP;
    m_System             = system;
    m_fEmissionIntensity = 0.0f;
    return true;
}

// 0x4A4460
void FxEmitter_c::Update(float currentTime, float deltaTime) {
    if (m_bEnabled && !m_System->m_stopParticleCreation) {
        CreateParticles(currentTime, deltaTime);
    }
}

// 0x4A2570
void FxEmitter_c::Reset() {
    m_fEmissionIntensity = 0.0f;
}

// NOTSA - The part of `AddParticle` that is common between both overloads
static void AddParticleWithMatrices(FxEmitter_c& emitter, RwMatrix* localMat, RwMatrix* worldMat, RwMatrix* finalMat, const EmissionInfo_t& emission, const CVector& vel, float timeSince, const FxPrtMult_c& fxMults, float rotZ, float brightness, bool createLocal) {
    RwMatrixUpdate(localMat);
    if (emitter.m_System->m_ParentMatrix) {
        RwMatrixMultiply(worldMat, localMat, emitter.m_System->m_ParentMatrix);
    } else {
        *worldMat = *localMat;
    }

    auto* const primMat = g_fxMan.FxRwMatrixCreate();
    emitter.m_PrimBP->GetRWMatrix(*primMat);
    RwMatrixMultiply(finalMat, primMat, worldMat);
    g_fxMan.FxRwMatrixDestroy(primMat);

    if (auto* const particle = emitter.CreateParticle(emission, *finalMat, &vel, timeSince, fxMults, brightness, createLocal)) {
        if (rotZ >= 0.0f) {
            particle->m_RotZ = (int8)(rotZ * 0.5f);
        }
    }
}

// 0x4A4050
void FxEmitter_c::AddParticle(const RwMatrix& mat, const CVector& vel, float timeSince, const FxPrtMult_c& fxMults, float rotZ, float brightness, bool createLocal) {
    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(0.0f, 0.0f, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    auto* const finalMat = g_fxMan.FxRwMatrixCreate();
    auto* const worldMat = g_fxMan.FxRwMatrixCreate();
    auto* const localMat = g_fxMan.FxRwMatrixCreate();
    *localMat = mat;

    AddParticleWithMatrices(*this, localMat, worldMat, finalMat, emission, vel, timeSince, fxMults, rotZ, brightness, createLocal);

    g_fxMan.FxRwMatrixDestroy(localMat);
    g_fxMan.FxRwMatrixDestroy(worldMat);
    g_fxMan.FxRwMatrixDestroy(finalMat);
}

// 0x4A3EA0
void FxEmitter_c::AddParticle(const CVector& pos, const CVector& vel, float timeSince, const FxPrtMult_c& fxMults, float rotZ, float brightness, bool createLocal) {
    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(0.0f, 0.0f, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    auto* const finalMat = g_fxMan.FxRwMatrixCreate();
    auto* const worldMat = g_fxMan.FxRwMatrixCreate();
    auto* const localMat = g_fxMan.FxRwMatrixCreate();

    // Identity matrix with the translation set to `pos`
    localMat->right = { 1.0f, 0.0f, 0.0f };
    localMat->up    = { 0.0f, 1.0f, 0.0f };
    localMat->at    = { 0.0f, 0.0f, 1.0f };
    localMat->flags |= 0x20003;
    localMat->pos   = { pos.x, pos.y, pos.z };

    AddParticleWithMatrices(*this, localMat, worldMat, finalMat, emission, vel, timeSince, fxMults, rotZ, brightness, createLocal);

    g_fxMan.FxRwMatrixDestroy(localMat);
    g_fxMan.FxRwMatrixDestroy(worldMat);
    g_fxMan.FxRwMatrixDestroy(finalMat);
}

// 0x4A41E0
void FxEmitter_c::CreateParticles(float currentTime, float deltaTime) {
    static_assert(offsetof(FxSystem_c, m_UseConstTime) == 0x52 && offsetof(FxSystem_c, m_fCameraDistance) == 0x58 && offsetof(FxSystem_c, m_nRateMult) == 0x5E);
    static_assert(offsetof(EmissionInfo_t, m_fMinWind) == 0x58 && offsetof(EmissionInfo_t, m_fMaxRain) == 0x64);

    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(currentTime, deltaTime, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    auto lodStart = (float)m_PrimBP->m_FxInfoManager.m_nLodStart / 64.0f;
    auto lodEnd   = (float)m_PrimBP->m_FxInfoManager.m_nLodEnd   / 64.0f;

    float visibility;
    if (m_System->m_fCameraDistance >= lodStart) {
        if (m_System->m_fCameraDistance <= lodEnd) {
            visibility = 1.0f - (m_System->m_fCameraDistance - lodStart) / (lodEnd - lodStart);
        } else {
            visibility = 0.0f;
        }
    } else {
        visibility = 1.0f;
    }

    m_fEmissionIntensity += emission.m_fCount * visibility * ((float)m_System->m_nRateMult * 0.001f);
    if (   CWeather::Wind >= emission.m_fMinWind
        && CWeather::Wind <= emission.m_fMaxWind
        && CWeather::Rain >= emission.m_fMinRain
        && CWeather::Rain <= emission.m_fMaxRain
        && m_fEmissionIntensity >= 1.0f
    ) {
        auto mat = g_fxMan.FxRwMatrixCreate();
        auto mat1 = g_fxMan.FxRwMatrixCreate();
        RwMatrixUpdate(&m_System->m_LocalMatrix);
        if (m_System->m_ParentMatrix) {
            RwMatrixMultiply(mat1, &m_System->m_LocalMatrix, m_System->m_ParentMatrix);
        } else {
            *mat1 = m_System->m_LocalMatrix;
        }

        auto mat2 = g_fxMan.FxRwMatrixCreate();
        m_PrimBP->GetRWMatrix(*mat2);
        RwMatrixMultiply(mat, mat2, mat1);
        g_fxMan.FxRwMatrixDestroy(mat2);

        auto counter = 0u;
        for (auto step = 0u; counter < (uint32)m_fEmissionIntensity; step = counter) {
            FxPrtMult_c prtMult;
            auto timeSince = (float)step / m_fEmissionIntensity * deltaTime;
            CreateParticle(emission, *mat, nullptr, timeSince, prtMult, 1.2f, m_System->m_createLocal);
            counter++;
        }
        m_fEmissionIntensity -= (float)(int32)m_fEmissionIntensity; // Only the fraction is kept for later

        g_fxMan.FxRwMatrixDestroy(mat1);
        g_fxMan.FxRwMatrixDestroy(mat);
    }
}

// 0x4A2580
FxEmitterPrt_c* FxEmitter_c::CreateParticle(const EmissionInfo_t& emissionInfo, RwMatrix& wldMat, const CVector* velOverride, float timeSince, const FxPrtMult_c& fxMults, float brightness, bool createLocal) {
    // Random number in [0, 9999]. NOTE: The original doesn't use `GetRandomNumberInRange` for this
    const auto Rand10000 = [] { return (float)(CGeneral::GetRandomNumber() % 10'000); };

    auto* particle = reinterpret_cast<FxEmitterPrt_c*>(g_fxMan.GetParticle(0));
    if (!particle) {
        if (!m_System->m_MustCreateParticles) {
            return nullptr;
        }
        g_fxMan.FreeUpParticle();
        particle = reinterpret_cast<FxEmitterPrt_c*>(g_fxMan.GetParticle(0));
        if (!particle) {
            return nullptr;
        }
    }

    particle->m_fCurrentLife = 0.0f;
    particle->m_fTotalLife   = ((Rand10000() * 0.0002f - 1.0f) * emissionInfo.m_fLifeBias + emissionInfo.m_fLife) * fxMults.m_fLife;
    particle->m_System       = m_System;

    particle->m_MultColor = CRGBA{ fxMults.m_Color };
    particle->m_MultSize  = fxMults.m_fSize;
    particle->m_MultRot   = fxMults.m_Rot;

    particle->m_bLocalToSystem = createLocal;

    particle->m_RandR = (uint8)(Rand10000() * 0.0001f * 255.0f);
    particle->m_RandG = (uint8)(Rand10000() * 0.0001f * 255.0f);
    particle->m_RandB = (uint8)(Rand10000() * 0.0001f * 255.0f);
    particle->m_Brightness = brightness;

    particle->m_RotZ            = -1;
    particle->m_CurrentRotation = Rand10000() * 0.0001f * (emissionInfo.m_fRotationMaxAngle - emissionInfo.m_fRotationMinAngle) + emissionInfo.m_fRotationMinAngle;

    if (createLocal) {
        m_PrimBP->GetRWMatrix(wldMat);
    }

    CVector vec;
    if (std::fabs(emissionInfo.m_fRadius) < 0.001f) { // Box
        vec.x = Rand10000() * 0.0001f * (emissionInfo.m_SizeMax.x - emissionInfo.m_SizeMin.x) + emissionInfo.m_SizeMin.x;
        vec.y = Rand10000() * 0.0001f * (emissionInfo.m_SizeMax.y - emissionInfo.m_SizeMin.y) + emissionInfo.m_SizeMin.y;
        vec.z = Rand10000() * 0.0001f * (emissionInfo.m_SizeMax.z - emissionInfo.m_SizeMin.z) + emissionInfo.m_SizeMin.z;
    } else { // Sphere
        vec.x = Rand10000() * 0.0002f - 1.0f;
        vec.y = Rand10000() * 0.0002f - 1.0f;
        vec.z = Rand10000() * 0.0002f - 1.0f;

        const auto invDist = 1.0f / vec.Magnitude();
        const auto scale   = emissionInfo.m_fRadius < 0.0f
            ? invDist
            : Rand10000() * 0.0001f * invDist;
        vec *= scale * emissionInfo.m_fRadius;
    }
    vec += emissionInfo.m_Pos;

    particle->m_Pos = vec.z * wldMat.at + vec.y * wldMat.up + vec.x * wldMat.right + wldMat.pos;

    if (velOverride) {
        particle->m_Velocity = *velOverride;
    } else {
        constexpr float DEG_TO_RAD = 0.017453279f;
        const auto randomAngle = Rand10000() * 0.0001f * 6.28318f;
        const auto minAngle    = emissionInfo.m_fAngleMin * DEG_TO_RAD;
        const auto angle       = (emissionInfo.m_fAngleMax * DEG_TO_RAD - minAngle) * (Rand10000() * 0.0001f) + minAngle;

        const auto sinAngle = CMaths::GetSinFast(angle);
        const CVector randomizedAngleVec{
            CMaths::GetCosFast(randomAngle) * sinAngle,
            CMaths::GetCosFast(angle),
            CMaths::GetSinFast(randomAngle) * sinAngle
        };

        CVector vectorsIn;
        if (emissionInfo.m_Direction.x <= 10.0f) {
            vectorsIn = emissionInfo.m_Direction;
            vectorsIn.Normalise();
        } else {
            vectorsIn = particle->m_Pos;
        }

        CVector vectorsOut;
        RwV3dTransformVectors(&vectorsOut, &vectorsIn, 1, &wldMat);

        CVector rotated;
        RotateVecIntoVec(rotated, randomizedAngleVec, vectorsOut);
        particle->m_Velocity = rotated * ((Rand10000() * 0.0002f - 1.0f) * emissionInfo.m_fSpeedBias + emissionInfo.m_Speed);
    }

    particle->m_Velocity += m_System->m_VelAdd;
    static_cast<FxEmitterBP_c*>(m_PrimBP)->UpdateParticle(timeSince, particle);
    m_PrimBP->m_Particles.AddItem(particle);

    return particle;
}
