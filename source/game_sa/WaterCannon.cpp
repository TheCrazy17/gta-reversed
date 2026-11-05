#include "StdInc.h"

#include <limits>

#include "WaterCannon.h"
#include "FireManager.h"

void CWaterCannon::InjectHooks() {
    RH_ScopedClass(CWaterCannon);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x728B40);
    RH_ScopedInstall(Update_OncePerFrame, 0x72A280);
    RH_ScopedInstall(Update_NewInput, 0x728C20);
    RH_ScopedInstall(PushPeds, 0x7295E0);
    RH_ScopedInstall(Render, 0x728DA0);
}

// 0x728B40
void CWaterCannon::Init() {
    m_nId = 0;
    m_nSectionsCount = 0;
    m_nCreationTime = CTimer::GetTimeInMS();
    std::ranges::fill(m_abUsed, false);
    m_auRenderIndices = { 0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6, 8, 9, 10, 9, 11, 10 };
    m_Audio.Initialise(this);
}

// 0x72A280
void CWaterCannon::Update_OncePerFrame(int16 index) {
    const auto LIFETIME = 150;

    if (CTimer::GetTimeInMS() > m_nCreationTime + LIFETIME) {
        m_nSectionsCount = (m_nSectionsCount + 1) % SECTIONS_COUNT;
        m_abUsed[m_nSectionsCount] = false;
    }

    for (auto i = 0; i < SECTIONS_COUNT; i++) {
        if (m_abUsed[i]) {
            CVector& speed = m_sectionMoveSpeed[i];
            speed.z -= CTimer::GetTimeStep() / 250.0f;

            CVector& point = m_sectionPoint[i];
            point += speed * CTimer::GetTimeStep();

            // Originally done in a separate loop, but we do it here
            gFireManager.ExtinguishPointWithWater(point, 2.0f, 0.5f);
        }
    }

    if ((uint8)(CTimer::GetFrameCounter() + index) % 4 == 0) { // Notice cast to byte
        PushPeds();
    }

    // free if unused
    for (auto& used : m_abUsed) {
        if (used) {
            return;
        }
    }
    m_nId = 0;
}

// 0x728C20
void CWaterCannon::Update_NewInput(const CVector* start, const CVector* end) {
    m_sectionPoint[m_nSectionsCount]     = *start;
    m_sectionMoveSpeed[m_nSectionsCount] = *end;
    m_abUsed[m_nSectionsCount]           = true;
}

// 0x7295E0
void CWaterCannon::PushPeds() {
    // FIX_BUGS: R* originally used 10'000.f here, but thats bug prone (if map size gets increased)
    // The upper bound used to start at FLT_MIN, which is the smallest *positive* float, so a jet that
    // stays below 0 on an axis got a box that reached up to 0 instead of ending at the jet.
    constexpr auto FLOAT_MAX = std::numeric_limits<float>::max();
    CVector min{ FLOAT_MAX, FLOAT_MAX, FLOAT_MAX }, max{ -FLOAT_MAX, -FLOAT_MAX, -FLOAT_MAX };
    CBoundingBox bbox(min, max);

    for (auto i = 0; i < SECTIONS_COUNT; i++) {
        if (!IsSectionActive(i))
            continue;

        auto& secPos = GetSectionPosn(i);

        bbox.m_vecMin.x = std::min(bbox.m_vecMin.x, secPos.x);
        bbox.m_vecMax.x = std::max(bbox.m_vecMax.x, secPos.x);

        bbox.m_vecMin.y = std::min(bbox.m_vecMin.y, secPos.y);
        bbox.m_vecMax.y = std::max(bbox.m_vecMax.y, secPos.y);

        bbox.m_vecMin.z = std::min(bbox.m_vecMin.z, secPos.z);
        bbox.m_vecMax.z = std::max(bbox.m_vecMax.z, secPos.z);
    }

    // The original tests the ped against the box with STRICT comparisons (0x729732..0x7297C3), unlike
    // CBoundingBox::IsPointWithin which is inclusive. That makes a box that is flat on one axis (min == max)
    // contain nothing: a jet fired exactly along +Y never changes its X, so all of its sections share
    // one X and it can't push anybody, however close they stand.
    const auto IsPointStrictlyWithin = [&bbox](const CVector& point) {
        return point.x > bbox.m_vecMin.x && point.x < bbox.m_vecMax.x
            && point.y > bbox.m_vecMin.y && point.y < bbox.m_vecMax.y
            && point.z > bbox.m_vecMin.z && point.z < bbox.m_vecMax.z;
    };

    for (auto& ped : GetPedPool()->GetAllValid()) {
        // 0x729719: the original skips peds with collision turned off. It has no check on physicalFlags
        // (the bMakeMassTwiceAsBig test that used to be here is not in the executable)
        if (!ped.m_bUsesCollision)
            continue;

        const CVector pedPosn = ped.GetPosition();
        if (!IsPointStrictlyWithin(pedPosn))
            continue;

        for (auto i = 0; i < SECTIONS_COUNT; i++) {
            const CVector& secPosn = GetSectionPosn(i);

            if ((pedPosn - secPosn).SquaredMagnitude() >= 5.0f)
                continue;

            const CVector secMoveSpeed = GetSectionMoveSpeed(i);

            {
                CEventHitByWaterCannon event(secPosn, secMoveSpeed);
                ped.GetEventGroup().Add(&event, false);
            }

            ped.bWasStanding = false;
            ped.bIsStanding = false;

            ped.ApplyMoveForce({ 0.0f, 0.0f, CTimer::GetTimeStep() });

            {
                // Moves the ped's speed a tenth of the way towards a tenth of the section's speed (0x7298CA..0x729969)
                const CVector2D applyableMoveSpeed = (secMoveSpeed * 0.1f - ped.m_vecMoveSpeed) * 0.1f;

                // Check if directions are the same (eg, + / +, - / -),
                // differing sign bits will always yield a negative result
                /// (unless 0, but that is handled by > (instead of >=))
                // The step is added to the ped's own speed. It used to be added to the section's speed, which
                // threw the ped at the jet's speed instead of nudging it
                if (secMoveSpeed.x * applyableMoveSpeed.x > 0.0f)
                    ped.m_vecMoveSpeed.x += applyableMoveSpeed.x;

                if (secMoveSpeed.y * applyableMoveSpeed.y > 0.0f)
                    ped.m_vecMoveSpeed.y += applyableMoveSpeed.y;
            }

            FxPrtMult_c prtInfo{ 1.0f, 1.0f, 1.0f, 0.6f, 0.75f, 0.0f, 0.2f };

            g_fx.m_SmokeII3expand->AddParticle(pedPosn, ped.m_vecMoveSpeed * 0.3f, 0.0f, prtInfo);

            CVector particleVelocity2 = ped.m_vecMoveSpeed * -0.3f;
            particleVelocity2.z += 0.5f;
            g_fx.m_SmokeII3expand->AddParticle(pedPosn, particleVelocity2, 0.0f, prtInfo);

            break;
        }
    }
}

// 0x728DA0
void CWaterCannon::Render() {
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,        RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATER));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));

    size_t prevIdx = m_nSectionsCount % SECTIONS_COUNT;
    size_t currIdx = prevIdx == 0 ? SECTIONS_COUNT - 1 : prevIdx - 1;

    bool hasCalculatedMatrix = false;

    CVector right{}, fwd{}, up{};

    m_Audio.ClearSplashInfo();

    for (auto i = 0; i < SECTIONS_COUNT; i++) {
        if (IsSectionActive(prevIdx) && IsSectionActive(currIdx)) {
            const CVector& prevPosn = GetSectionPosn(prevIdx);
            const CVector& currPosn = GetSectionPosn(currIdx);

            const CVector currToPrevDir = prevPosn - currPosn;
            if (currToPrevDir.SquaredMagnitude() < 25.0f) {
                if (!hasCalculatedMatrix) {
                    hasCalculatedMatrix = true;

                    up = Normalized(CrossProduct(prevPosn - currPosn, TheCamera.GetForward())) / 20.0f;
                    right = Normalized(CVector::Random(0.0f, 1.0f)) / 20.0f;
                    fwd = Normalized(CVector::Random(0.0f, 1.0f)) / 20.0f;
                }

                const float size = (float)(i * i) / (float)SECTIONS_COUNT + 3.0f;

                static RxObjSpace3DVertex vertices[12]{}; // 0xC80550

                // Set alpha depending on current `i`. The higher, the lower the alpha.
                const float progress = (float)i / (float)SECTIONS_COUNT;
                const auto  alpha = (RwUInt8)(64.0f * (1.0f - progress));
                RwRGBA color{ 200, 200, 255, alpha };
                for (auto& vertice : vertices) {
                    RxObjSpace3DVertexSetPreLitColor(&vertice, &color);
                }

                const CVector thisUp = up * size, thisRight = right * size, thisFwd = fwd * size;
                const CVector pos[std::size(vertices)] = {
                    currPosn - thisUp,
                    currPosn + thisUp,
                    prevPosn - thisUp,
                    prevPosn + thisUp,

                    currPosn - thisRight,
                    currPosn + thisRight,
                    prevPosn - thisRight,
                    prevPosn + thisRight,

                    currPosn - thisFwd,
                    currPosn + thisFwd,
                    prevPosn - thisFwd,
                    prevPosn + thisFwd,
                };
                for (size_t v = 0; v < std::size(vertices); v++) {
                    RxObjSpace3DVertexSetPos(&vertices[v], &pos[v]);
                }

                CColPoint colPoint{};
                CEntity* hitEntity{};
                const bool hasSectionHit = CWorld::ProcessLineOfSight(prevPosn, currPosn, colPoint, hitEntity, true, true, false, false, false, false, false, false);
                if (hasSectionHit) {
                    FxPrtMult_c prtInfo{ 1.0f, 1.0f, 1.0f, 0.15f, 0.75f, 1.0f, 0.2f };
                    CVector velocity0 = colPoint.m_vecNormal * 3.0f * CVector::Random(0.2f, 1.8f);

                    for (auto n = 0; n < 2; n++) {
                        const auto unk = (float)n / CTimer::GetTimeStepInMS();

                        g_fx.m_WaterSplash->AddParticle(colPoint.m_vecPoint, velocity0, unk, prtInfo);
                        g_fx.m_WaterSplash->AddParticle(colPoint.m_vecPoint, velocity0 * 0.6f, unk, prtInfo);
                    }

                    m_Audio.SetSplashInfo(colPoint.m_vecPoint, velocity0.Magnitude());

                    break;
                }


                if (RwIm3DTransform(vertices, std::size(vertices), nullptr, rwIM3D_VERTEXRGBA))
                {
                    RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, m_auRenderIndices.data(), m_auRenderIndices.size());
                    RwIm3DEnd();
                }
            }
        }
        currIdx = prevIdx;
        if (prevIdx == 0)
            prevIdx = SECTIONS_COUNT - 1;
        else
            prevIdx--;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         RWRSTATE(FALSE));
}

bool CWaterCannon::IsSectionActive(size_t idx) const {
    return m_abUsed[idx];
}

CVector& CWaterCannon::GetSectionPosn(size_t idx) {
    return m_sectionPoint[idx];
}

CVector& CWaterCannon::GetSectionMoveSpeed(size_t idx) {
    return m_sectionMoveSpeed[idx];
}
