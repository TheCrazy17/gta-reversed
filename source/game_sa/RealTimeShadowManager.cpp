#include "StdInc.h"

#include "RealTimeShadowManager.h"
#include "Shadows.h"

auto& g_realTimeShadowMan = StaticRef<CRealTimeShadowManager>(0xC40350);

void CRealTimeShadowManager::InjectHooks() {
    RH_ScopedClass(CRealTimeShadowManager);
    RH_ScopedCategory("Shadows");

    RH_ScopedInstall(Init, 0x7067C0);
    RH_ScopedInstall(ReInit, 0x706870, {.reversed = false});
    RH_ScopedInstall(ReturnRealTimeShadow, 0x705B30);
    RH_ScopedInstall(GetRealTimeShadow, 0x706970);
    RH_ScopedInstall(Update, 0x706AB0);
    RH_ScopedInstall(DoShadowThisFrame, 0x706BA0);
    RH_ScopedInstall(Exit, 0x706A60);
}

// 0x7067C0
void CRealTimeShadowManager::Init() {
    if (m_bInitialised) {
        return;
    }

    for (auto& shdw : m_apShadows) {
        shdw = new CRealTimeShadow();
        shdw->Create(true, 4, true);
    }

    m_BlurCamera.Create(6);

    m_GradientCamera.Create(6);
    m_GradientCamera.MakeGradientRaster();

    m_bInitialised = true;
}

// 0x706A60
void CRealTimeShadowManager::Exit() { // AKA `Shutdown`
    if (!m_bInitialised) {
        return;
    }

    for (auto& shdw : m_apShadows) {
        if (const auto owner = shdw->m_pOwner) {
            delete shdw; // `shdw->m_pOwner` nulled out by this
            delete owner; // Why?
        }
    }

    // Nice hack
    m_BlurCamera.Destroy();
    m_GradientCamera.Destroy();

    m_bInitialised = false;
}

// 0x705B30
void CRealTimeShadowManager::ReturnRealTimeShadow(CRealTimeShadow* shdw) {
    if (m_bInitialised) {
        shdw->m_pOwner->m_pShadowData = nullptr;
        shdw->m_pOwner = nullptr;
    }
}

// 0x706870
void CRealTimeShadowManager::ReInit() {
    plugin::CallMethod<0x706870, CRealTimeShadowManager*>(this);
}

// 0x706AB0
void CRealTimeShadowManager::Update() {
    ZoneScoped;

    if (m_bInitialised && m_bNeedsReinit) {
        ReInit();
        m_bNeedsReinit = false;
    }

    for (const auto shdw : m_apShadows) {
        if (!shdw->m_pOwner) {
            continue;
        }

        assert(shdw->m_pOwner->m_pShadowData == shdw);

        // 0x305eed - 0x305f0f: Update intensity
        constexpr auto INTENSITY_STEP = 3u;
        if (shdw->m_bKeepAlive) {
            shdw->m_nIntensity = std::min<uint8>(100u, shdw->m_nIntensity + INTENSITY_STEP);
        } else { // Fade out
            shdw->m_nIntensity = std::max<uint8>(shdw->m_nIntensity, INTENSITY_STEP) - INTENSITY_STEP; // Avoids underflow
        }

        if (shdw->m_nIntensity) {
            shdw->Update();
            CShadows::StoreRealTimeShadow(
                shdw->m_pOwner,

                CTimeCycle::m_fShadowDisplacementX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowDisplacementY[CTimeCycle::m_CurrentStoredValue],

                CTimeCycle::m_fShadowFrontX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowFrontY[CTimeCycle::m_CurrentStoredValue],

                CTimeCycle::m_fShadowSideX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowSideY[CTimeCycle::m_CurrentStoredValue]
            );
        } else if (m_bInitialised) {
            shdw->m_pOwner->m_pShadowData = nullptr;
            shdw->m_pOwner = nullptr;
        }
    }

    // TODO: ??? - Perhaps debug code left accidentally in?
    for (const auto shdw : m_apShadows) {
        shdw->m_bKeepAlive = false;
    }
}

CRealTimeShadow& CRealTimeShadowManager::GetRealTimeShadow(CPhysical* physical) {
    // NOTSA: `isFirstPlayer` is only ever computed when `physical` is a ped, matching the
    // original's short-circuit - see the `notFirstPlayerPed` derivation below.
    const bool isFirstPlayer = physical->GetIsTypePed() && physical->AsPed()->m_nPedType == PED_TYPE_PLAYER1;
    const bool notFirstPlayerPed = !physical->GetIsTypePed() || !isFirstPlayer;

    // Deny a shadow to anything but the main player while the main player is riding a fast vehicle
    // (likely a perf heuristic - skip real-time shadows for everything else during fast travel).
    bool allowShadow = true;
    if (notFirstPlayerPed) {
        auto* const playerVehicle = FindPlayerPed()->m_pVehicle;
        if (FindPlayerPed()->IsInVehicle() && playerVehicle && playerVehicle->GetMoveSpeed().SquaredMagnitude() > 0.09f /* 0.3^2 */) {
            allowShadow = false;
        }
    }

    CRealTimeShadow* shadow{};
    if (m_bInitialised && allowShadow) {
        if (isFirstPlayer) {
            shadow = m_apShadows[0]; // Slot 0 is reserved for the main player
        } else {
            // NOTSA: keeps scanning after finding a free slot rather than stopping at the first one,
            // so this ends up picking the LAST free slot (highest index), not the first - matches original.
            for (auto i = 1; i < NUM_REALTIME_SHADOWS; i++) {
                if (!m_apShadows[i]->m_pOwner) {
                    shadow = m_apShadows[i];
                }
            }
        }

        if (shadow) {
            shadow->SetupForThisEntity(physical);
            physical->m_pShadowData = shadow;
            shadow->m_bKeepAlive = true;
            shadow->m_nIntensity = 0;
        }
    }

    // NOTSA: `shadow` can be null here (not initialised / not allowed / no free slot) - the original
    // dereferences it unconditionally too (its only caller, DoShadowThisFrame, discards the result).
    return *shadow;
}

// 0x706BA0
void CRealTimeShadowManager::DoShadowThisFrame(CPhysical* physical) {
    switch (g_fx.GetFxQuality()) {
    case FX_QUALITY_VERY_HIGH: // Always render
        break;
    case FX_QUALITY_HIGH: { // Only draw for main player
        if (physical->GetIsTypePed()) {
            if (physical->AsPed()->m_nPedType == PED_TYPE_PLAYER1) {
                break;
            }
        }
        return;
    }
    default: // For any other quality: skip
        return;
    }

    if (const auto shdw = physical->m_pShadowData) {
        shdw->m_bKeepAlive = true;
    } else {
        (void)GetRealTimeShadow(physical); // ???
    }
}
