/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "WeaponEffects.h"

void CWeaponEffects::InjectHooks() {
    RH_ScopedClass(CWeaponEffects);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x742AB0);
    RH_ScopedInstall(Shutdown, 0x742B80);
    RH_ScopedInstall(IsLockedOn, 0x742BD0);
    RH_ScopedInstall(MarkTarget, 0x742BF0);
    RH_ScopedInstall(ClearCrossHair, 0x742C60);
    RH_ScopedInstall(ClearCrossHairs, 0x742C80);
    RH_ScopedInstall(ClearCrossHairImmediately, 0x742CA0);
    RH_ScopedInstall(ClearCrossHairsImmediately, 0x742CC0);
    RH_ScopedInstall(Render, 0x742CF0);
}

// 0x742AB0
void CWeaponEffects::Init() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_vecPosn = CVector();
        crossHair.m_bActive = false;
        crossHair.m_color = CRGBA(255, 0, 0, 127);
        crossHair.m_fSize = 1.0f;
        crossHair.m_fSpin = 0.0f;
        crossHair.m_nTimeWhenToDeactivate = 0;
        crossHair.m_bClearImmediately = false;
        crossHair.m_fRotation = 0.0f;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));
    gpCrossHairTex          = RwTextureRead("target256",  "target256m");
    gpCrossHairTexFlight[0] = RwTextureRead("lockon",     "lockonA");
    gpCrossHairTexFlight[1] = RwTextureRead("lockonFire", "lockonFireA");
    CTxdStore::PopCurrentTxd();
}

// 0x742B80
void CWeaponEffects::Shutdown() {
    for (auto& crossHair : gpCrossHairTexFlight) {
        RwTextureDestroy(crossHair);
        crossHair = nullptr;
    }

    RwTextureDestroy(gpCrossHairTex);
    gpCrossHairTex = nullptr;
}

// 0x742BD0
bool CWeaponEffects::IsLockedOn(CrossHairId id) {
    return gCrossHair[id].m_fRotation;
}

// 0x742BF0
void CWeaponEffects::MarkTarget(CrossHairId id, CVector posn, uint8 red, uint8 green, uint8 blue, uint8 alpha, float size, bool bClearImmediately) {
    auto& crossHair = gCrossHair[id];
    crossHair.m_vecPosn               = posn;
    crossHair.m_color.r               = red;
    crossHair.m_color.g               = green;
    crossHair.m_color.b               = blue;
    crossHair.m_color.a               = alpha;
    crossHair.m_bActive               = true;
    crossHair.m_fSize                 = size;
    crossHair.m_nTimeWhenToDeactivate = -1;
    crossHair.m_bClearImmediately     = bClearImmediately;
}

// 0x742C60
void CWeaponEffects::ClearCrossHair(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() + 400);
}

// 0x742C80
void CWeaponEffects::ClearCrossHairs() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_bActive = false;
    }
}

// 0x742CA0
void CWeaponEffects::ClearCrossHairImmediately(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() - 100);
    gCrossHair[id].m_bActive = false;
}

// 0x742CC0
void CWeaponEffects::ClearCrossHairsImmediately() {
    for (auto i = 0u; i < gCrossHair.size(); i++) {
        ClearCrossHair(i);
    }
}

// 0x742CF0
void CWeaponEffects::Render() {
    ZoneScoped;

    if (TheCamera.m_bWideScreenOn) {
        return;
    }

    static auto& s_Flicker           = StaticRef<std::array<float, MAX_NUM_WEAPON_CROSSHAIRS>>(0xC8A8A0); // How far the markers of the targets are from their middle
    static auto& s_LockOnPulseCos    = StaticRef<float>(0xC8A898);
    static auto& s_LockOnPulseSin    = StaticRef<float>(0xC8A89C);
    static auto& s_TargetSince       = StaticRef<uint32>(0xC8A890);
    static auto& s_Target            = StaticRef<CEntity*>(0xC8A894);
    static auto& s_MarkerSize        = StaticRef<float>(0x8D6140);
    static auto& s_MarkerGrowing     = StaticRef<std::array<bool, MAX_NUM_WEAPON_CROSSHAIRS>>(0x8D6144);
    static auto& s_RadiusOnScreenMult = StaticRef<float>(0x8D6148);

    const auto screenW = (float)RsGlobal.maximumWidth;
    const auto screenH = (float)RsGlobal.maximumHeight;

    const auto SetRenderStates = [](bool zWrite, bool zTest) {
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(zWrite));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(zTest));
    };
    const auto SetBlending = [](RwBlendFunction src, RwBlendFunction dst) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(src));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(dst));
    };
    const auto SetTexture = [](RwTexture* texture) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(RwTextureGetRaster(texture)));
    };

    for (auto i = 0u; i < gCrossHair.size(); i++) {
        auto& crossHair = gCrossHair[i];
        auto& flicker   = s_Flicker[i];

        if (crossHair.m_nTimeWhenToDeactivate != 0 && (uint32)crossHair.m_nTimeWhenToDeactivate < CTimer::GetTimeInMS()) {
            crossHair.m_bActive               = false;
            crossHair.m_nTimeWhenToDeactivate = 0;
            flicker                           = 0.0f;
        }
        if (crossHair.m_nTimeWhenToDeactivate != -1) {
            flicker = 0.0f;
        }
        if (!CWorld::Players[i].m_pPed) {
            crossHair.m_bActive = false;
        }
        if (!crossHair.m_bActive) {
            continue;
        }

        const auto r = crossHair.m_color.r;
        const auto g = crossHair.m_color.g;
        const auto b = crossHair.m_color.b;
        const auto a = crossHair.m_color.a;

        if (crossHair.m_bClearImmediately) {
            // The lock on of the aircraft
            CVector scrPos;
            float   w, h;
            if (CSprite::CalcScreenCoors(crossHair.m_vecPosn, &scrPos, &w, &h, true, true)) {
                const auto fit = 20.0f / h;
                if (fit > 1.0f) {
                    w *= fit;
                    h *= fit;
                }

                SetRenderStates(false, false);
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(true));
                SetBlending(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
                SetTexture(gpCrossHairTexFlight[0]);

                const float outlineScale[]{ 0.95f, 1.05f };
                for (auto k = 0; k < 2; k++) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(scrPos, { std::min(28.0f, w) * outlineScale[k] * 1.8f, std::min(20.0f, h) * outlineScale[k] * 1.8f }, 0, 0, 0, 255, 0.01f, 0.0f, 255);
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(scrPos, { std::min(28.0f, w) * 1.8f, std::min(20.0f, h) * 1.8f }, r, g, b, 255, 0.01f, 0.0f, a);

                SetTexture(gpCrossHairTexFlight[1]);

                // The ring around it closes in
                const auto angleStep = (double)(CTimer::GetTimeInMS() & 0x3FF) * (double)0.0061359233f;
                const auto angle     = (float)angleStep;
                crossHair.m_fPulse   = std::max(0.0f, crossHair.m_fPulse - (float)(20.0 * angleStep));
                crossHair.m_fRotation = crossHair.m_fPulse == 0.0f ? 1.0f : 0.0f; // Locked on
                s_LockOnPulseSin     = (float)(std::sin(angleStep) * crossHair.m_fPulse);
                s_LockOnPulseCos     = (float)(std::cos(angleStep) * crossHair.m_fPulse);

                for (auto k = 0; k < 2; k++) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(
                        { scrPos.x - s_LockOnPulseSin, scrPos.y - s_LockOnPulseCos, scrPos.z },
                        { std::min(28.0f, w) * outlineScale[k] * 0.8f, std::min(20.0f, h) * outlineScale[k] * 0.8f },
                        0, 0, 0, 255, 0.01f, angle, 255
                    );
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    { scrPos.x - s_LockOnPulseSin, scrPos.y - s_LockOnPulseCos, scrPos.z },
                    { std::min(28.0f, w) * 0.8f, std::min(20.0f, h) * 0.8f },
                    r, g, b, a, 0.01f, angle, 255
                );
            }
            SetRenderStates(true, true);
        } else {
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(false));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(false));
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(true));
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(nullptr));

            CVector scrPos;
            float   w, h;
            CSprite::CalcScreenCoors(crossHair.m_vecPosn, &scrPos, &w, &h, false, false);

            const auto& viewMat = TheCamera.m_mCameraMatrix;
            if (scrPos.x > screenW || scrPos.x < 0.0f || scrPos.y > screenH || scrPos.y < 0.0f) {
                // The target is out of the screen, so an arrow shows the way to it, but only for the second player
                if (CGameLogic::IsCoopGameGoingOn()) {
                    // The direction relative to the camera and the one relative to the player
                    const auto GetAngle = [&](const CVector& from) {
                        const auto delta = crossHair.m_vecPosn - from;
                        return std::atan2((double)viewMat.GetUp().Dot(delta), (double)viewMat.GetRight().Dot(delta));
                    };
                    const auto  camAngle = GetAngle(viewMat.GetPosition());
                    const auto* ped      = CWorld::Players[i].m_pPed;
                    const auto  pedAngle = GetAngle(ped->GetPosition());

                    const auto sinCam = (float)std::sin(camAngle);
                    const auto cosCam = (float)std::cos(camAngle);
                    const auto sinPed = (float)std::sin(pedAngle);
                    const auto cosPed = (float)std::cos(pedAngle);

                    const auto S      = s_MarkerSize;
                    const auto centre = CVector2D{ (1.0f - cosCam) * screenW * 0.5f, (1.0f - sinCam) * screenH * 0.5f };

                    CVector2D p0, p1, p2;
                    const auto tipX = sinPed * 0.5f + cosPed;
                    const auto tipY = cosPed * 0.5f + sinPed;
                    p0 = centre;
                    p1 = { tipX * S + centre.x, S * sinPed + centre.y - cosPed * S * 0.5f };
                    p2 = { cosPed * S + centre.x - S * sinPed * 0.5f, tipY * S + centre.y };
                    CSprite::RenderOneXLUSprite_Triangle(p0, p1, p2, 2.5f, 0, 0, 0, 255, 1.0f, 255);

                    p0.x = cosPed * S * 0.1f + p0.x;
                    p0.y = S * sinPed * 0.1f + p0.y;
                    p1   = { tipX * S * 0.8f + p0.x, (S * sinPed - cosPed * S * 0.5f) * 0.8f + p0.y };
                    p2   = { (cosPed * S - S * sinPed * 0.5f) * 0.8f + p0.x, tipY * S * 0.8f + p0.y };
                    CSprite::RenderOneXLUSprite_Triangle(p0, p1, p2, 2.5f, r, g, b, 255, 1.0f, 255);
                }
            } else {
                // The target is on the screen, a marker that closes in on it
                crossHair.m_fSize = std::max(0.3f, std::min(crossHair.m_fSize, 1.2f));
                auto innerRadius  = 5.0f * crossHair.m_fSize;
                auto outerRadius  = 25.0f * crossHair.m_fSize;
                if (scrPos.z < 2.5f) {
                    scrPos.z = 2.5f;
                }
                if (const auto weaponRadius = CWorld::Players[i].m_pPed->GetWeaponRadiusOnScreen(); weaponRadius > 0.0f) {
                    innerRadius = weaponRadius * s_RadiusOnScreenMult;
                    outerRadius = crossHair.m_fSize * 20.0f + innerRadius;
                }

                SetBlending(rwBLENDZERO, rwBLENDZERO);
                for (auto k = 0; k < 3; k++) {
                    const auto baseAngle = (double)k * 120.0;
                    const auto GetPoint  = [&](double degrees, float radius) {
                        const auto angle = (double)0.017453292f * degrees + (double)crossHair.m_fSpin;
                        return CVector2D{
                            (float)(-std::sin(angle) * (double)(radius + flicker)) + scrPos.x,
                            (float)(-std::cos(angle) * (double)(radius + flicker)) + scrPos.y
                        };
                    };
                    auto p0 = GetPoint(baseAngle, innerRadius);
                    auto p1 = GetPoint(baseAngle + 15.0, outerRadius);
                    auto p2 = GetPoint(baseAngle - 15.0, outerRadius);

                    CSprite::RenderOneXLUSprite_Triangle(p0, p1, p2, scrPos.z, 0, 0, 0, 255, 1.0f, a);

                    // The colored one is smaller
                    const auto centroid = (p0 + p1 + p2) * 0.33333334f;
                    p0 = (p0 - centroid) * 0.75f + centroid;
                    p1 = (p1 - centroid) * 0.75f + centroid;
                    p2 = (p2 - centroid) * 0.75f + centroid;

                    SetBlending(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
                    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));
                    CSprite::RenderOneXLUSprite_Triangle(p0, p1, p2, scrPos.z, r, g, b, 255, 1.0f, a);
                }

                const auto timeToDeactivate = crossHair.m_nTimeWhenToDeactivate;
                if ((uint32)timeToDeactivate > 0 && timeToDeactivate != -1) {
                    // Going away
                    crossHair.m_fSpin += 0.75f;
                    flicker            += 2.0f;
                    crossHair.m_fSize  *= 0.9f;
                } else {
                    crossHair.m_fSpin += 0.05f;
                }
                if (crossHair.m_fSpin > TWO_PI) {
                    crossHair.m_fSpin = 0.0f;
                }

                if (timeToDeactivate == 0) {
                    if (s_MarkerGrowing[i]) {
                        flicker = crossHair.m_fSize * 2.0f + flicker;
                        if (flicker > crossHair.m_fSize * 20.0f) {
                            s_MarkerGrowing[i] = false;
                        }
                    } else {
                        flicker -= 2.0f;
                        if (flicker < 0.0f) {
                            s_MarkerGrowing[i] = true;
                        }
                    }
                }
            }

            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(false));
            SetRenderStates(true, true);
        }
    }

    // The cursor of the second player shooting from a vehicle
    if (TheCamera.GetActiveCam().m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
        SetRenderStates(false, false);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(true));
        SetBlending(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        SetTexture(gpCrossHairTex);

        const auto spin  = (float)((double)(CTimer::GetTimeInMS() & 0x3FF) * (double)0.0061359233f);
        const auto* aim  = reinterpret_cast<const float*>(reinterpret_cast<const uint8*>(&TheCamera.GetActiveCam()) + 0xFC); // Where the cursor is, from -1 to 1
        CSprite::RenderOneXLUSprite_Rotate_Aspect({ (aim[0] + 1.0f) * screenW * 0.5f, (aim[1] + 1.0f) * screenH * 0.5f, 100.0f }, { 10.0f, 10.0f }, 255, 128, 0, 128, 0.01f, spin, 255);

        // The one that is not the driver is the one shooting
        auto* shooter = CWorld::Players[1].m_pPed;
        if (auto* const veh = CWorld::Players[0].m_pPed->m_pVehicle; veh && veh->m_pDriver != CWorld::Players[0].m_pPed) {
            shooter = CWorld::Players[0].m_pPed;
        }

        CEntity* target{};
        if (shooter) {
            const auto* const weaponInfo = CWeaponInfo::GetWeaponInfo(shooter->GetActiveWeapon().m_Type, shooter->GetWeaponSkill());
            target = CWeapon::FindNearestTargetEntityWithScreenCoors(aim[0], aim[1], weaponInfo->m_fWeaponRange * 2.0f, shooter->GetPosition(), nullptr, nullptr);
        }
        if (target != s_Target) {
            s_TargetSince = CTimer::GetTimeInMS();
            s_Target      = target;
        }
        if (target) {
            CVector scrPos;
            float   w, h;
            if (CSprite::CalcScreenCoors(target->GetPosition(), &scrPos, &w, &h, true, true)) {
                const auto fit = 20.0f / h;
                if (fit > 1.0f) {
                    w *= fit;
                    h *= fit;
                }
                const auto elapsed   = (int32)(CTimer::GetTimeInMS() - s_TargetSince);
                const auto sizeMult  = std::max(1.0f, 3.0f - (float)elapsed * 0.001953125f);
                const auto intensity = std::min(255, elapsed / 4 + 70);
                const auto spin2     = (float)((double)(CTimer::GetTimeInMS() & 0x3FF) * (double)0.0061359233f);
                CSprite::RenderOneXLUSprite_Rotate_Aspect(scrPos, { sizeMult * w, h * sizeMult }, (uint8)intensity, 0, 0, intensity, 0.01f, spin2, 255);
            }
        }
        SetRenderStates(true, true);
    }

    for (auto i = 0u; i < 2; i++) {
        plugin::CallMethod<0x56EF90, CPlayerInfo*, int32>(reinterpret_cast<CPlayerInfo*>(0xB7CEF0 + i * sizeof(CPlayerInfo)), i);
    }
}
