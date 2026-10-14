#include "StdInc.h"

#include "Cam.h"
#include "TimeCycle.h"
#include "Camera.h"
#include "Shadows.h"
#include "IdleCam.h"
#include "InterestingEvents.h"
#include "ModelIndices.h"
#include "HandShaker.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskSimpleUseGun.h"
#include "TaskSimpleGunControl.h"
#include "TaskSimpleSwim.h"
#include "TaskSimpleClimb.h"
#include "TaskComplexProstituteSolicit.h"
#include "TaskComplexEnterCar.h"
#include "ControllerConfigManager.h"
#include "TaskSimpleHoldEntity.h"

auto& gbFirstPersonRunThisFrame = StaticRef<bool>(0xB6EC20);
auto& gLastFrameProcessedDWCineyCam = StaticRef<uint32>(0x8CCB9C);
static auto& s_DWCineyCamSceneEndTime = StaticRef<uint32>(0x8CCBA4);

static auto& s_ExitCam = StaticRef<std::array<bool, MODE_SYPHON_CRIM_IN_FRONT + 1>>(0xB6EC5C);

static inline auto& DWCineyCamLastPos = StaticRef<CVector>(0xB6FE8C);
static inline auto& DWCineyCamLastUp = StaticRef<CVector>(0xB6FE98);
static inline auto& DWCineyCamLastRight = StaticRef<CVector>(0xB6FEA4);
static inline auto& DWCineyCamLastFwd = StaticRef<CVector>(0xB6FEB0);

static inline auto& DWCineyCamLastNearClip = StaticRef<float>(0xB6EC08);
static inline auto& DWCineyCamLastFov = StaticRef<float>(0xB6EC0C);

bool IsLampPost(eModelID modelId);

// 0x509AE0
static void WellBufferMe(float target, float& valueToChange, float& speedSoFar, float topSpeed, float speedStep, bool isAnAngle) {
    const auto valueToTargetDiff = [&] {
        auto d = target - valueToChange;
        if (isAnAngle) {
            for (; d >= DegreesToRadians(180.0f); d -= DegreesToRadians(360.0f)) {
                ;
            }
            for (; d < DegreesToRadians(-180.0f); d += DegreesToRadians(360.0f)) {
                ;
            }
        }
        return d;
    }();

    const auto fullSpeedStep = valueToTargetDiff * topSpeed;
    speedSoFar += std::abs(std::abs(fullSpeedStep - speedSoFar) * CTimer::GetTimeStep() * speedStep);

    if (fullSpeedStep >= 0.0f || fullSpeedStep <= speedSoFar) {
        if (fullSpeedStep > 0.0f && fullSpeedStep < speedSoFar) {
            speedSoFar = fullSpeedStep;
        }
    } else {
        speedSoFar = fullSpeedStep;
    }

    valueToChange += std::min(CTimer::GetTimeStep(), 10.0f) * speedSoFar;
}

// 0x509BE0
static void WrapAngle(float& angle) {
    while (angle >= DegreesToRadians(180.0f)) {
        angle -= DegreesToRadians(360.0f);
    }
    while (angle < DegreesToRadians(-180.0f)) {
        angle += DegreesToRadians(360.0f);
    }
}

// 0x5B2330
static void FindSplinePathPositionFloat(float* output, const float* spline, float time, uint32* marker) {
    static auto& minimumSegmentTime = StaticRef<float>(0x8D0F80);
    const auto count = (uint32)spline[0];
    const float duration = (spline[*marker] - spline[*marker - 4]) * 1000.0f;
    const float endTime = spline[(count - 1) * 4 + 1] * 1000.0f;
    if (time < endTime) {
        if ((*marker - 1) / 4 > count) {
            *marker = (count - 1) * 4 + 1;
        } else if (duration <= minimumSegmentTime) {
            *marker += 4;
            if ((*marker - 1) / 4 > count) {
                *marker = (count - 1) * 4 + 1;
            }
        }
    }
    const auto index = *marker;
    float t = std::clamp((time - spline[index - 4] * 1000.0f) / ((spline[index] - spline[index - 4]) * 1000.0f), 0.0f, 1.0f);
    if (time > endTime) {
        t = 1.0f;
    }
    const float start = spline[index - 3];
    const float control = spline[index - 1];
    const float end = spline[index + 1];
    if (control == start) {
        *output = (end - start) * t + start;
    } else {
        const float s = 1.0f - t;
        *output = s * s * s * start + (control * s * s * 3.0f + (spline[index + 2] * s * 3.0f + end * t) * t) * t;
    }
}

// 0x5B2090
static void FindSplinePathPositionVector(CVector* output, const float* spline, float time, uint32* marker) {
    static auto& minimumSegmentTime = StaticRef<float>(0x8D0F80);
    const auto count = (uint32)spline[0];
    const float duration = (spline[*marker] - spline[*marker - 10]) * 1000.0f;
    const float endTime = spline[(count - 1) * 10 + 1] * 1000.0f;
    if (time < endTime) {
        if ((*marker - 1) / 10 > count) {
            *marker = (count - 1) * 10 + 1;
        } else if (duration <= minimumSegmentTime) {
            *marker += 10;
            if ((*marker - 1) / 10 > count) {
                *marker = (count - 1) * 10 + 1;
            }
        }
    }
    const auto index = *marker;
    // The vector evaluator retains the duration from before advancing the marker.
    float t = std::clamp((time - spline[index - 10] * 1000.0f) / duration, 0.0f, 1.0f);
    if (time > endTime) {
        t = 1.0f;
    }
    const CVector start{spline[index - 9], spline[index - 8], spline[index - 7]};
    const CVector end{spline[index + 1], spline[index + 2], spline[index + 3]};
    const CVector control1{spline[index - 3], spline[index - 2], spline[index - 1]};
    const CVector control2{spline[index + 4], spline[index + 5], spline[index + 6]};
    if (control1 == start) {
        *output = (end - start) * t + start;
    } else {
        const float s = 1.0f - t;
        *output = start * (s * s * s) + end * (t * t * t) + (control1 * (t * s * s) + control2 * (s * t * t)) * 3.0f;
    }
    *output += TheCamera.m_vecCutSceneOffset;
}

// NOTSA: camera front vector for the given horizontal (beta) and vertical (alpha) angles.
static CVector CamFrontVector(float beta, float alpha) {
    return {-std::cos(beta) * std::cos(alpha), -std::sin(beta) * std::cos(alpha), std::sin(alpha)};
}

static auto& s_TempSphereColPoints = StaticRef<std::array<CColPoint, 32>>(0xB9B250); // gaTempSphereColPoints

// 0x50A0A0
// Rounds `value` to `places` decimals the way the original does: the integral part is stored as a
// float before the final division, which changes the rounding of large scaled values.
static float RoundCamCoordinate(float value, int32 places) {
    const auto scaled = (double)value * std::pow(10.0, places + 1);
    const auto biased = (scaled + (value >= 0.0f ? 5.0 : -5.0)) * (double)0.1f;
    const auto whole  = (float)std::trunc(biased);
    return (float)((double)whole / std::pow(10.0, places));
}

// 0x50A120
static void RoundCamCoordinates(CVector& position, int32 places) {
    position.x = RoundCamCoordinate(position.x, places);
    position.y = RoundCamCoordinate(position.y, places);
    position.z = RoundCamCoordinate(position.z, places);
}

void CCam::InjectHooks() {
    RH_ScopedClass(CCam);
    RH_ScopedCategory("Camera");

    RH_ScopedInstall(Constructor, 0x517730);
    RH_ScopedInstall(Init, 0x50E490);
    RH_ScopedInstall(CacheLastSettingsDWCineyCam, 0x50D7A0);
    RH_ScopedInstall(DoCamBump, 0x50CB30);
    RH_ScopedInstall(Finalise_DW_CineyCams, 0x50DD70);
    RH_ScopedInstall(GetCoreDataForDWCineyCamMode, 0x517130);
    RH_ScopedInstall(GetLookFromLampPostPos, 0x5161A0);
    RH_ScopedInstall(GetVectorsReadyForRW, 0x509CE0);
    RH_ScopedInstall(Get_TwoPlayer_AimVector, 0x513E40);
    RH_ScopedInstall(IsTimeToExitThisDWCineyCamMode, 0x517400);
    RH_ScopedInstall(KeepTrackOfTheSpeed, 0x509DF0);
    RH_ScopedInstall(GetBoatHandlingCamHeight, 0x509CA0);
    RH_ScopedInstall(LookBehind, 0x520690);
    RH_ScopedInstall(LookRight, 0x520E40);
    RH_ScopedInstall(RotCamIfInFrontCar, 0x50A4F0);
    RH_ScopedInstall(Using3rdPersonMouseCam, 0x50A850);
    RH_ScopedInstall(Process, 0x526FC0, { .Reversed = false });
    RH_ScopedInstall(ProcessArrestCamOne, 0x518500, { .Reversed = false });
    RH_ScopedInstall(ProcessPedsDeadBaby, 0x519250, { .Reversed = false });
    RH_ScopedInstall(Process_1rstPersonPedOnPC, 0x50EB70);
    RH_ScopedInstall(Process_1stPerson, 0x517EA0);
    RH_ScopedInstall(Process_AimWeapon, 0x521500);
    RH_ScopedInstall(Process_AttachedCam, 0x512B10);
    RH_ScopedInstall(Process_Cam_TwoPlayer, 0x525E50, { .Reversed = false });
    RH_ScopedInstall(Process_Cam_TwoPlayer_InCarAndShooting, 0x519810, { .Reversed = false });
    RH_ScopedInstall(Process_Cam_TwoPlayer_Separate_Cars, 0x513510, { .Reversed = false });
    RH_ScopedInstall(Process_Cam_TwoPlayer_Separate_Cars_TopDown, 0x513BE0);
    RH_ScopedInstall(Process_DW_BirdyCam, 0x51B850);
    RH_ScopedInstall(Process_DW_CamManCam, 0x51B120);
    RH_ScopedInstall(Process_DW_HeliChaseCam, 0x51A740);
    RH_ScopedInstall(Process_DW_PlaneCam1, 0x51C760);
    RH_ScopedInstall(Process_DW_PlaneCam2, 0x51CC30);
    RH_ScopedInstall(Process_DW_PlaneCam3, 0x51D100);
    RH_ScopedInstall(Process_DW_PlaneSpotterCam, 0x51C250);
    RH_ScopedInstall(Process_Editor, 0x50F3F0);
    RH_ScopedInstall(Process_Fixed, 0x51D470);
    RH_ScopedInstall(Process_FlyBy, 0x5B25F0);
    RH_ScopedInstall(Process_FollowCar_SA, 0x5245B0);
    RH_ScopedInstall(Process_FollowPedWithMouse, 0x50F970);
    RH_ScopedInstall(Process_FollowPed_SA, 0x522D40);
    RH_ScopedInstall(Process_M16_1stPerson, 0x5105C0);
    RH_ScopedInstall(Process_Rocket, 0x511B50);
    RH_ScopedInstall(Process_SpecialFixedForSyphon, 0x517500, { .Reversed = false });
    RH_ScopedInstall(Process_WheelCam, 0x512110);

    RH_ScopedGlobalInstall(WellBufferMe, 0x509AE0);
    RH_ScopedGlobalInstall(WrapAngle, 0x509BE0);
}

// 0x517730
CCam::CCam() {
    Init();
}

// 0x50E490
void CCam::Init() {
    m_vecFront = CVector(0, 0, -1);
    m_vecUp = CVector(0, 0, 1);
    m_nMode = eCamMode::MODE_FOLLOWPED;
    m_bRotating = false;
    m_nDoCollisionChecksOnFrameNum = 1;
    m_nDoCollisionCheckEveryNumOfFrames = 9;
    m_nFrameNumWereAt = 0;
    m_bCollisionChecksOn = true;
    m_fRealGroundDist = 0.0f;
    m_fBetaSpeed = 0.0f;
    m_fAlphaSpeed = 0.0f;
    m_fCameraHeightMultiplier = 0.75;
    m_fMaxRoleAngle = DegreesToRadians(20.0f);
    m_fDistance = 30.0f;
    m_fDistanceSpeed = 0.0f;
    m_pLastCarEntered = nullptr;
    m_pLastPedLookedAt = nullptr;
    m_bResetStatics = true;
    m_fHorizontalAngle = 0.0f;
    m_fTilt = 0.0f;
    m_fTiltSpeed = 0.0f;
    m_bFixingBeta = false;
    m_fCaMinDistance = 0.0f;
    m_fCaMaxDistance = 0.0f;
    m_bLookingBehind = false;
    m_bLookingLeft = false;
    m_bLookingRight = false;
    m_fPlayerInFrontSyphonAngleOffSet = DegreesToRadians(20.0f);
    m_fSyphonModeTargetZOffSet = 0.5f;
    m_fRadiusForDead = 1.5f;
    m_nDirectionWasLooking = 3; // TODO: enum
    m_bLookBehindCamWasInFront = 0;
    m_fRoll = 0.0f;
    m_fRollSpeed = 0.0f;
    m_fCloseInPedHeightOffset = 0.0f;
    m_fCloseInPedHeightOffsetSpeed = 0.0f;
    m_fCloseInCarHeightOffset = 0.0f;
    m_fCloseInCarHeightOffsetSpeed = 0.0f;
    m_fPedBetweenCameraHeightOffset = 0.0f;
    m_fTargetBeta = 0.0f;
    m_fBufferedTargetBeta = 0.0f;
    m_fBufferedTargetOrientation = 0.0f;
    m_fBufferedTargetOrientationSpeed = 0.0f;
    m_fDimensionOfHighestNearCar = 0.0;
    m_fBeta_Targeting = 0.0f;
    m_fX_Targetting = 0.0f;
    m_fY_Targetting = 0.0f;
    m_pCarWeAreFocussingOn = nullptr;
    m_pCarWeAreFocussingOnI = nullptr;
    m_fCamBumpedHorz = 1.0f;
    m_fCamBumpedVert = 0.0f;
    m_nCamBumpedTime = 0;
    for (int i = 0; i < 4; ++i) {
        m_anTargetHistoryTime[i] = 0;
        m_avecTargetHistoryPos[i] = CVector{};
    }
    m_nCurrentHistoryPoints = 0;
    gPlayerPedVisible = true;
    gbCineyCamMessageDisplayed = 2; // TODO: enum
    gCameraDirection = 3; // TODO: enum
    gCameraMode = (eCamMode)-1;
    gLastTime2PlayerCameraWasOK = 0;
    gLastTime2PlayerCameraCollided = 0;
    TheCamera.m_bCinemaCamera = false;
}

// 0x50D7A0
void CCam::CacheLastSettingsDWCineyCam() {
    DWCineyCamLastUp       = m_vecUp;
    DWCineyCamLastFwd      = m_vecFront;
    DWCineyCamLastRight    = CrossProduct(m_vecFront, m_vecUp);
    DWCineyCamLastFov      = m_fFOV;
    DWCineyCamLastNearClip = RwCameraGetNearClipPlane(Scene.m_pRwCamera);
    DWCineyCamLastPos      = m_vecSource;
}

// 0x50CB30
void CCam::DoCamBump(float horizontal, float vertical) {
    m_fCamBumpedHorz = horizontal;
    m_fCamBumpedVert = vertical;
    m_nCamBumpedTime = CTimer::GetTimeInMS();
}

// 0x50DD70
void CCam::Finalise_DW_CineyCams(const CVector& src, const CVector& dest, float roll, float fov, float nearClip, float shakeDegree) {
    m_vecFront  = (dest - src).Normalized();
    m_vecSource = src;

    // What is this thing?
    {
        auto rightDir = m_vecFront.Cross({ std::sin(roll), 0.0f, std::cos(roll) }).Normalized();
        m_vecUp       = rightDir.Cross(m_vecFront);
        if (m_vecFront.x == 0.0f && m_vecFront.y == 0.0f) {
            m_vecFront.x = m_vecFront.y = 0.0001f;
        }
        rightDir = CrossProduct(m_vecFront, m_vecUp).Normalized();
        m_vecUp  = CrossProduct(rightDir, m_vecFront);
    }

    m_fFOV = fov;
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.4f); // meant to use nearClip here?
    CacheLastSettingsDWCineyCam();
    gLastFrameProcessedDWCineyCam = CTimer::GetFrameCounter();

    gHandShaker[0].Process(shakeDegree);
    m_vecFront = gHandShaker[0].m_resultMat.TransformVector(m_vecFront);
    m_vecFront.Normalise();

    {
        auto rightDir = m_vecFront.Cross({ std::sin(roll), 0.0f, std::cos(roll) }).Normalized();
        m_vecUp       = rightDir.Cross(m_vecFront);
        if (m_vecFront.x == 0.0f && m_vecFront.y == 0.0f) {
            m_vecFront.x = m_vecFront.y = 0.0001f;
        }
        rightDir = m_vecFront.Cross(m_vecUp).Normalized();
        m_vecUp  = rightDir.Cross(m_vecFront);
    }
}

// 0x517130
void CCam::GetCoreDataForDWCineyCamMode(
    CEntity*& entity,
    CVehicle*& vehicle,
    CVector& dest,
    CVector& src,
    CVector& targetUp,
    CVector& targetRight,
    CVector& targetFwd,
    CVector& targetVel,
    float& targetSpeed,
    CVector& targetAngVel,
    float& targetAngSpeed,
    CColSphere& colSphere
) {
    entity         = m_pCamTargetEntity;
    vehicle        = entity->AsVehicle();
    dest           = entity->GetPosition();
    src            = DWCineyCamLastPos;
    targetUp       = entity->GetUpVector();
    targetRight    = entity->GetRightVector();
    targetFwd      = entity->GetForwardVector();
    targetVel      = entity->AsPhysical()->GetMoveSpeed();
    targetSpeed    = targetVel.Magnitude();
    targetAngVel   = entity->AsPhysical()->GetTurnSpeed();
    targetAngSpeed = targetAngVel.Magnitude();

    colSphere.Set(
        entity->GetModelInfo()->GetColModel()->GetBoundRadius(),
        entity->GetBoundCentre(),
        eSurfaceType::SURFACE_DEFAULT
    );
}

// 0x5161A0
bool CCam::GetLookFromLampPostPos(CEntity* target, CPed* cop, const CVector& targetPos, CVector& outPos) {
    static auto& s_BestDistance = StaticRef<float>(0x8CC8D8);

    int16                    count{};
    std::array<CEntity*, 16> entities;
    CWorld::FindObjectsInRange(targetPos, 30.0f, true, &count, 0xF, entities.data(), false, false, false, true, true);

    CEntity* winner{};
    auto     closestDistDiff = 10000.0f;
    for (auto* const entity : entities | rngv::take(count)) {
        if (!entity->m_bIsStatic && !entity->m_bIsStaticWaitingForCollision) {
            continue;
        }
        if (entity->GetMatrix().GetUp().z <= 0.9f) {
            continue;
        }
        if (!IsLampPost((eModelID)entity->GetModelIndex())) {
            continue;
        }

        const auto dist = CVector2D::Dist(entity->GetPosition(), targetPos);
        if (dist <= 5.0f || std::abs(s_BestDistance - dist) >= closestDistDiff) {
            continue;
        }

        const auto topPos = entity->GetMatrix().TransformPoint(entity->GetColModel()->GetBoundingBox().m_vecMax);
        const auto dir    = (topPos - targetPos).Normalized();
        if (!CWorld::GetIsLineOfSightClear(topPos, dir + targetPos, true, false, false, false, false, true, true)) {
            continue;
        }

        winner          = entity;
        outPos          = topPos;
        closestDistDiff = std::abs(s_BestDistance - dist);
    }
    return winner != nullptr;
}

// 0x509CE0
void CCam::GetVectorsReadyForRW() {
    m_vecFront.Normalise();
    if (m_vecFront.x == 0.0f && m_vecFront.y == 0.0f) {
        m_vecFront.x = m_vecFront.y = 0.0001f;
    }
    const auto a = CrossProduct(m_vecFront, { 0.0f, 0.0f, 1.0f }).Normalized();
    m_vecUp = CrossProduct(a, m_vecFront);
}

// 0x513E40 -- not tested
void CCam::Get_TwoPlayer_AimVector(CVector& out) {
    const auto player = [&] {
        auto* p1 = FindPlayerPed(PED_TYPE_PLAYER1);
        if (p1->m_pVehicle && !p1->m_pVehicle->IsDriver(p1)) {
            return FindPlayerPed(PED_TYPE_PLAYER2);
        }
        return p1;
    }();

    const auto weaponInfo = player->GetActiveWeapon().GetWeaponInfo(player);
    const auto nearestTargetEntityInScreen = CWeapon::FindNearestTargetEntityWithScreenCoors(
        m_fX_Targetting,
        m_fY_Targetting,
        2 * weaponInfo.m_fWeaponRange,
        player->GetPosition()
    );

    if (nearestTargetEntityInScreen) {
        out = nearestTargetEntityInScreen->GetPosition() - m_vecSource;
    } else {
        const auto right  = CrossProduct(m_vecFront, m_vecUp);
        const auto tanFov = std::tan(m_fFOV * PI / 360.0f);

        out = m_fX_Targetting * m_fY_Targetting * tanFov * right + m_vecFront - tanFov / CDraw::ms_fAspectRatio * m_vecUp;
    }
    out.Normalise();
}

// 0x517400
bool CCam::IsTimeToExitThisDWCineyCamMode(int32 camId, const CVector& src, const CVector& dst, float t, bool lineOfSightCheck) {
    if (s_ExitCam[camId]) {
        return true;
    }

    if (camId < MODE_FOLLOW_PED_WITH_BIND || camId > MODE_SYPHON_CRIM_IN_FRONT) {
        return false;
    }

    // One for each mode from `MODE_FOLLOW_PED_WITH_BIND` to `MODE_SYPHON_CRIM_IN_FRONT`
    static constexpr std::array<float, 9> s_MinDistances{ 3.0f, 3.0f, 1.0f, 3.0f, 5.0f, 3.0f, 3.0f, 3.0f, 3.0f };                 // 0x8CCBCC
    static constexpr std::array<float, 9> s_MaxDistances{ 185.0f, 100.0f, 100.0f, 100.0f, 30.0f, 30.0f, 100.0f, 100.0f, 100.0f }; // 0x8CCBF0

    const auto dist         = (dst - src).Magnitude();
    const auto isWithinBand = dist >= s_MinDistances[camId - MODE_FOLLOW_PED_WITH_BIND]
                           && dist <= s_MaxDistances[camId - MODE_FOLLOW_PED_WITH_BIND];

    bool isLosClear = true;
    if (lineOfSightCheck) {
        CWorld::pIgnoreEntity = m_pCamTargetEntity;
        CColPoint colPoint{};
        CEntity*  hitEntity{};
        isLosClear = !CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
        CWorld::pIgnoreEntity = nullptr;
    }

    return !isWithinBand || !isLosClear || CTimer::GetTimeInMS() > s_DWCineyCamSceneEndTime;
}

// 0x509DF0
void CCam::KeepTrackOfTheSpeed(const CVector& source, const CVector& target, const CVector& up, const float& alpha, const float& beta, const float& fov) {
    static auto& s_PreviousSource = StaticRef<CVector>(0xB6FF80); // = source;
    static auto& s_PreviousTarget = StaticRef<CVector>(0xB6FF74); // = target;
    static auto& s_PreviousUp     = StaticRef<CVector>(0xB6FF68); // = up;
    static auto& s_PreviousBeta   = StaticRef<float>(0xB6FF64);   // = beta;
    static auto& s_PreviousAlpha  = StaticRef<float>(0xB6FF60);   // = alpha;
    static auto& s_PreviousFov    = StaticRef<float>(0xB6FF5C);   // = fov;

    if (TheCamera.m_bJust_Switched) {
        s_PreviousSource = source;
        s_PreviousTarget = target;
        s_PreviousUp     = up;
    }

    m_vecSourceSpeedOverOneFrame = source - s_PreviousSource;
    m_vecTargetSpeedOverOneFrame = target - s_PreviousTarget;
    m_vecUpOverOneFrame          = up - s_PreviousUp;

    m_fFovSpeedOverOneFrame  = fov - s_PreviousFov;
    m_fBetaSpeedOverOneFrame = beta - s_PreviousBeta;
    WrapAngle(m_fBetaSpeedOverOneFrame);
    m_fAlphaSpeedOverOneFrame = alpha - s_PreviousAlpha;
    WrapAngle(m_fAlphaSpeedOverOneFrame);

    s_PreviousSource = source;
    s_PreviousTarget = target;
    s_PreviousUp     = up;
    s_PreviousBeta   = beta;
    s_PreviousAlpha  = alpha;
    s_PreviousFov    = fov;
}

// 0x520690
// 0x509CA0
bool CCam::GetBoatHandlingCamHeight(float* outCamHeight) {
    if (!m_pCamTargetEntity) {
        return false;
    }
    const auto* const boatHandling = gHandlingDataMgr.GetBoatPointer(
        static_cast<uint8>(CModelInfo::GetVehicleModelInfo(m_pCamTargetEntity->m_nModelIndex)->m_nHandlingId)
    );
    if (!boatHandling) {
        return false;
    }
    *outCamHeight = boatHandling->m_fLookLRBehindCamHeight;
    return true;
}

// 0x520690
bool CCam::LookBehind() {
    static auto& gStoredTargetPos = StaticRef<CVector>(0xB6F018);

    auto* const entity     = m_pCamTargetEntity;
    const auto  entityType = entity->GetType();

    const bool isBehindCamVehicle = (m_nMode == MODE_CAM_ON_A_STRING || m_nMode == MODE_BEHINDBOAT || m_nMode == MODE_BEHINDCAR)
        && entityType == ENTITY_TYPE_VEHICLE;
    const bool isFirstPersonVehicle = m_nMode == MODE_1STPERSON && entityType == ENTITY_TYPE_VEHICLE;

    if (!isBehindCamVehicle && !isFirstPersonVehicle && entityType != ENTITY_TYPE_PED) {
        return false;
    }

    const auto entityPos = entity->GetPosition();
    m_vecFront           = entityPos - m_vecSource;

    if (isBehindCamVehicle) {
        auto target      = gStoredTargetPos;
        m_bLookingBehind = true;

        const auto distAway = m_nMode == MODE_CAM_ON_A_STRING ? m_fCaMaxDistance : 15.5f;

        const auto& forward = entity->GetMatrix().GetForward();
        m_vecSource         = target + CVector{ forward.x, forward.y, forward.z + 0.2f } * distAway;

        CWorld::pIgnoreEntity = entity;
        TheCamera.m_nExtraEntitiesCount = 0;
        TheCamera.CameraVehicleModeSpecialCases(entity->AsVehicle());
        TheCamera.CameraColDetAndReact(&m_vecSource, &target);

        m_vecFront = entity->GetPosition() - m_vecSource;
        GetVectorsReadyForRW();
        TheCamera.ImproveNearClip(entity->AsVehicle(), nullptr, &m_vecSource, &target);
        CWorld::pIgnoreEntity = nullptr;
    }

    if (isFirstPersonVehicle) {
        auto* const vehicle = entity->AsVehicle();
        m_bLookingBehind    = true;
        RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.05f);

        m_vecFront = entity->GetMatrix().GetForward();
        m_vecFront.Normalise();

        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
            m_vecSource.z -= 0.5f;
        }

        const auto appearance = vehicle->GetVehicleAppearance();
        if (appearance == VEHICLE_APPEARANCE_BIKE) {
            m_vecSource += m_vecFront * 2.3f;
            m_vecFront = -m_vecFront;
            GetVectorsReadyForRW();
        } else if (appearance == VEHICLE_APPEARANCE_HELI) {
            m_vecFront = entity->GetMatrix().GetUp() * -1.0f;
            m_vecUp    = entity->GetMatrix().GetForward();
            m_vecSource += m_vecFront * 0.25f;
        } else {
            m_vecSource += m_vecFront * 0.25f;
            m_vecFront = -m_vecFront;
        }
    }

    if (entityType == ENTITY_TYPE_PED) {
        // Indexed by the ped zoom level (0x8CCE40 + 4 * zoom, ...): the tables start one entry before these addresses.
        static auto& zoomAddSourceZ         = StaticRef<std::array<float, 4>>(0x8CCE3C);
        static auto& zoomAddTargetZ         = StaticRef<std::array<float, 4>>(0x8CCE30);
        static auto& zoomLerpSwim           = StaticRef<std::array<float, 4>>(0x8CCE24);
        static auto& zoomAddSourceZSwim     = StaticRef<std::array<float, 4>>(0x8CCE18);

        auto* const ped    = entity->AsPed();
        const auto  zoom   = TheCamera.m_nPedZoom;
        auto        target = entityPos;

        m_vecSource   = CVector{ -std::cos(m_fHorizontalAngle), -std::sin(m_fHorizontalAngle), 0.0f };
        m_vecSource.z = 0.3f - m_vecSource.Dot(ped->field_578);
        m_vecSource.Normalise();

        const auto distance = std::max(0.6f, StaticRef<float>(0x8CCE4C) + TheCamera.m_fPedZoomSmoothed);
        m_vecSource         = target + m_vecSource * distance;

        if (ped->GetIntelligence()->GetTaskSwim()) {
            m_vecSource = target + (target - m_vecSource) * zoomLerpSwim[zoom];
            m_vecSource.z += zoomAddSourceZSwim[zoom];
        }
        m_vecSource.z += zoomAddSourceZ[zoom];
        target.z += zoomAddTargetZ[zoom];

        TheCamera.HandleCameraMotionForDucking(ped, &m_vecSource, &target, false);

        TheCamera.m_nExtraEntitiesCount = 0;
        if (auto* const hold = ped->GetIntelligence()->GetTaskHold(false); hold && hold->m_pEntityToHold) {
            TheCamera.m_pExtraEntity[TheCamera.m_nExtraEntitiesCount++] = hold->m_pEntityToHold;
        }

        CCollision::bCamCollideWithVehicles = true;
        CCollision::bCamCollideWithObjects  = true;
        CCollision::bCamCollideWithPeds     = true;

        TheCamera.CameraColDetAndReact(&m_vecSource, &target);
        m_vecFront = target - m_vecSource;
        GetVectorsReadyForRW();
        TheCamera.ImproveNearClip(nullptr, ped, &m_vecSource, &target);

        if (zoom == 1 && RwCameraGetNearClipPlane(Scene.m_pRwCamera) > 0.05f) {
            RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.05f);
        }
    }

    GetVectorsReadyForRW();
    return true;
}

// 0x520E40
bool CCam::LookRight(bool bLookRight) {
    auto* const entity     = m_pCamTargetEntity;
    const auto  entityType = entity->GetType();

    const bool isBehindCamVehicle = (m_nMode == MODE_CAM_ON_A_STRING || m_nMode == MODE_BEHINDBOAT || m_nMode == MODE_BEHINDCAR)
        && entityType == ENTITY_TYPE_VEHICLE;
    const bool isFirstPersonVehicle = m_nMode == MODE_1STPERSON && entityType == ENTITY_TYPE_VEHICLE;

    float sideSign = 1.0f;
    if (!bLookRight) {
        m_bLookingLeft = true;
        sideSign       = -1.0f;
    } else {
        m_bLookingRight = true;
    }

    if (isBehindCamVehicle) {
        auto targetPos = entity->GetPosition();

        float distance = 9.0f;
        if (m_nMode == MODE_CAM_ON_A_STRING) {
            distance = m_fCaMaxDistance;
        } else if (m_nMode == MODE_BEHINDBOAT) {
            float boatCamHeight = 0.0f;
            if (GetBoatHandlingCamHeight(&boatCamHeight) && !CCullZones::Cam1stPersonForPlayer()) {
                m_vecSource.z = targetPos.z + boatCamHeight;
            }
        }

        auto forward = entity->GetMatrix().GetForward();
        forward.Normalise();

        const auto angle = CGeneral::GetATanOfXY(forward.x, forward.y) + sideSign * HALF_PI;
        m_vecSource.x    = std::cos(angle) * distance + targetPos.x;
        m_vecSource.y    = std::sin(angle) * distance + targetPos.y;

        const auto& bounds              = entity->GetColModel()->GetBoundingBox();
        const auto  sourceZBeforeColDet = m_vecSource.z;

        CWorld::pIgnoreEntity           = entity;
        TheCamera.m_nExtraEntitiesCount = 0;
        TheCamera.CameraVehicleModeSpecialCases(entity->AsVehicle());
        TheCamera.CameraColDetAndReact(&m_vecSource, &targetPos);
        CWorld::pIgnoreEntity = nullptr;

        targetPos = entity->GetPosition();

        const auto& matrix = entity->GetMatrix();
        const auto  sideZ  = !bLookRight
            ? bounds.m_vecMax.x * matrix.GetRight().z
            : matrix.GetRight().z * bounds.m_vecMin.x;
        const auto minZ = bounds.m_vecMax.z * matrix.GetUp().z + (targetPos.z + sideZ);

        const auto clampedMinZ = std::max(minZ, m_vecTargetCoorsForFudgeInter.z);
        auto       newZ        = sourceZBeforeColDet;
        if (clampedMinZ + 0.1f <= sourceZBeforeColDet) {
            newZ = clampedMinZ + 0.1f;
        }
        m_vecSource.z = std::max(newZ, m_vecSource.z);

        m_vecFront = targetPos - m_vecSource;
        m_vecFront.z += 1.1f;
        if (m_nMode == MODE_BEHINDBOAT) {
            m_vecFront.z += 1.2f;
        }
        GetVectorsReadyForRW();
        return true;
    }

    if (isFirstPersonVehicle) {
        auto* const vehicle = entity->AsVehicle();
        RwCameraSetNearClipPlane(Scene.m_pRwCamera, StaticRef<float>(0x8CCE50));

        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
            if (auto* const driver = vehicle->m_pDriver) {
                CVector neckPos{};
                driver->SetPedPositionInCar();
                driver->UpdateRwMatrix();
                driver->UpdateRwFrame();
                driver->UpdateRpHAnim();
                driver->GetBonePosition(&neckPos, BONE_NECK, true);
                neckPos += vehicle->GetMatrix().GetRight() * (bLookRight ? StaticRef<float>(0x8CC498) : StaticRef<float>(0x8CC49C));
                neckPos += vehicle->GetMatrix().GetUp() * StaticRef<float>(0x8CC494);
                m_vecSource = neckPos;
            } else {
                m_vecSource.z -= 0.5f;
            }
        }

        if (vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            m_vecSource -= vehicle->GetMatrix().GetRight() * 0.35f;
        }

        m_vecUp = vehicle->GetMatrix().GetUp();
        m_vecUp.Normalise();

        m_vecFront = vehicle->GetMatrix().GetForward();
        m_vecFront.Normalise();

        m_vecFront = !bLookRight ? m_vecUp.Cross(m_vecFront) : m_vecFront.Cross(m_vecUp);
        m_vecFront.Normalise();

        if (vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) {
            m_vecSource -= m_vecFront * 1.45f;
        }
        return true;
    }

    return entityType == ENTITY_TYPE_PED;
}

// 0x50A4F0
bool CCam::RotCamIfInFrontCar(const CVector& target, float orientation) {
    auto* const entity = m_pCamTargetEntity;
    if (!entity->GetIsTypeVehicle()) {
        return false;
    }
    auto* const vehicle = entity->AsVehicle();

    const auto& speed         = vehicle->GetMoveSpeed();
    const auto  movingForward = DotProduct(vehicle->GetMatrix().GetForward(), speed) > 0.1f;
    if (speed.SquaredMagnitude2D() > 0.0036f) {
        orientation = std::atan2(-speed.x, speed.y) - DegreesToRadians(90.0f);
    }

    const auto dist  = CVector2D::Dist(m_vecSource, target);
    auto       delta = orientation - m_fHorizontalAngle;
    WrapAngle(delta);
    if (std::abs(delta) > DegreesToRadians(20.0f)
        && movingForward
        && !TheCamera.m_bTransitionState) {
        m_bFixingBeta = true;
    }

    const auto pad = CPad::GetPad(0);
    if (!pad->GetLookBehindForCar() && !pad->GetLookBehindForPed() && !pad->GetLookLeft() && !pad->GetLookRight() && m_nDirectionWasLooking != LOOKING_DIRECTION_FORWARD) {
        TheCamera.m_bCamDirectlyBehind = true;
    }

    if (!m_bFixingBeta && !TheCamera.m_bUseTransitionBeta && !TheCamera.m_bCamDirectlyBehind && !TheCamera.m_bCamDirectlyInFront) {
        return false;
    }

    const bool wasRequested = (TheCamera.m_bCamDirectlyBehind
                            || TheCamera.m_bCamDirectlyInFront
                            || TheCamera.m_bUseTransitionBeta)
                           && &TheCamera.GetActiveCam() == this;
    if (m_bFixingBeta || wasRequested) {
        WellBufferMe(orientation, m_fHorizontalAngle, m_fBetaSpeed, 0.1f, 0.003f, true);
        if (TheCamera.m_bCamDirectlyBehind && &TheCamera.GetActiveCam() == this) {
            m_fHorizontalAngle = orientation;
        }
        if (TheCamera.m_bCamDirectlyInFront && &TheCamera.GetActiveCam() == this) {
            m_fHorizontalAngle = orientation + DegreesToRadians(180.0f);
        }
        if (TheCamera.m_bUseTransitionBeta && &TheCamera.GetActiveCam() == this) {
            m_fHorizontalAngle = m_fTransitionBeta;
        }
        m_vecSource.x = target.x + std::cos(m_fHorizontalAngle) * dist;
        m_vecSource.y = target.y + std::sin(m_fHorizontalAngle) * dist;

        delta = orientation - m_fHorizontalAngle;
        WrapAngle(delta);
        if (std::abs(delta) < DegreesToRadians(2.0f)) {
            m_bFixingBeta = false;
        }
    }
    TheCamera.m_bCamDirectlyBehind  = false;
    TheCamera.m_bCamDirectlyInFront = false;
    return true;
}

// 0x50A850
bool CCam::Using3rdPersonMouseCam() const {
    return CCamera::m_bUseMouse3rdPerson && m_nMode == MODE_FOLLOWPED;
}

// 0x509DC0
bool CCam::GetWeaponFirstPersonOn() {
    return m_pCamTargetEntity && m_pCamTargetEntity->GetIsTypePed() && m_pCamTargetEntity->AsPed()->GetActiveWeapon().m_IsFirstPersonWeaponModeSelected;
}

// inlined -- alpha = vertical angle
void CCam::ClipAlpha() {
    m_fVerticalAngle = std::clamp(
        m_fVerticalAngle,
        DegreesToRadians(-85.5f),
        DegreesToRadians(+60.0f)
    );
}

// 0x509C50 -- beta = horizontal angle
void CCam::ClipBeta() {
    if (m_fHorizontalAngle > PI) {
        m_fHorizontalAngle -= TWO_PI;
    } else if (m_fHorizontalAngle < -PI) {
        m_fHorizontalAngle += TWO_PI;
    }
}

// 0x526FC0
void CCam::Process() {
    NOTSA_UNREACHABLE();
}

// 0x518500
void CCam::ProcessArrestCamOne() {
    NOTSA_UNREACHABLE();
}

// 0x519250
void CCam::ProcessPedsDeadBaby() {
    NOTSA_UNREACHABLE();
}

// 0x50EB70
void CCam::Process_1rstPersonPedOnPC(const CVector& target, float orientation, float speedVar, float speedVarWanted) {
    static auto& v3d_8CCC54  = StaticRef<CVector>(0x8CCC54); // head point, local to the spine bone
    static auto& byte_B6FFDC = StaticRef<bool>(0xB6FFDC);
    static auto& v3d_B6FFC4  = StaticRef<CVector>(0xB6FFC4);
    static auto& v3d_B6FFD0  = StaticRef<CVector>(0xB6FFD0);

    if (m_nMode != MODE_SNIPER_RUNABOUT) {
        m_fFOV = 70.0f;
    }

    TheCamera.m_b1rstPersonRunCloseToAWall = false;
    if (!m_pCamTargetEntity->GetRwObject()) {
        return;
    }

    if (m_pCamTargetEntity->GetIsTypePed()) {
        auto* const targetPed = m_pCamTargetEntity->AsPed();

        const auto hier = GetAnimHierarchyFromSkinClump(m_pCamTargetEntity->GetRpClump());
        const auto aIdx = RpHAnimIDGetIndex(hier, ConvertPedNode2BoneTag(2)); // todo: enum
        auto&      aMat = RpHAnimHierarchyGetMatrixArray(hier)[aIdx];

        CVector pointIn = v3d_8CCC54;
        RwV3dTransformPoint(&pointIn, &pointIn, &aMat);
        RwV3d v3dZero{ 0.0f };
        RwMatrixScale(&aMat, &v3dZero, rwCOMBINEPRECONCAT); // hide the head

        if (m_bResetStatics) {
            m_fHorizontalAngle          = targetPed->m_fCurrentRotation + DegreesToRadians(90.0f);
            m_fVerticalAngle            = 0.0f;
            m_fInitialPlayerOrientation = m_fHorizontalAngle;
            byte_B6FFDC                 = false;
            v3d_B6FFD0.Reset();
            m_bCollisionChecksOn          = true;
            m_vecBufferedPlayerBodyOffset = v3d_B6FFC4 = pointIn;
        }
        m_vecBufferedPlayerBodyOffset.y = pointIn.y;

        if (TheCamera.m_bHeadBob) {
            const auto catchUp = TheCamera.m_fScriptPercentageInterToStopMoving; // 0xB6FC48
            m_vecBufferedPlayerBodyOffset.x = catchUp * m_vecBufferedPlayerBodyOffset.x + (1.0f - catchUp) * pointIn.x;
            m_vecBufferedPlayerBodyOffset.z = catchUp * m_vecBufferedPlayerBodyOffset.z + (1.0f - catchUp) * pointIn.z;
            m_vecSource = targetPed->GetMatrix().TransformPoint(m_vecBufferedPlayerBodyOffset);
        } else {
            auto forward = targetPed->GetMatrix().GetForward();
            forward.z    = 0.0f;
            forward.Normalise();
            const auto mag = (pointIn - v3d_B6FFC4).Magnitude2D();
            m_vecSource    = forward * mag * 1.23f + targetPed->GetPosition() + CVector{ 0.0f, 0.0f, 0.59f };
        }

        CVector spinePos{};
        targetPed->GetTransformedBonePosition(spinePos, BONE_SPINE1, true);

        auto*      pad   = CPad::GetPad(0);
        const auto fov   = m_fFOV * 0.0125f;
        const auto mouse = pad->NewMouseControllerState.GetAmountMouseMoved();
        if (mouse.IsZero()) {
            const auto h = (float)-pad->LookAroundLeftRightOnPC();
            const auto v = (float)pad->LookAroundUpDownOnPC();
            m_fHorizontalAngle += fov * 0.057142857f * h * h * 0.0001f * (h < 0.0f ? -1.0f : 1.0f) * CTimer::GetTimeStep();
            m_fVerticalAngle += fov * 0.071428575f * v * v * 4.4444445e-05f * (v < 0.0f ? -1.0f : 1.0f) * CTimer::GetTimeStep();
        } else {
            m_fHorizontalAngle += fov * CCamera::m_fMouseAccelHorzntl * mouse.x * -3.0f;
            m_fVerticalAngle += fov * CCamera::m_fMouseAccelVertical * (mouse.y * 4.0f);
        }
        ClipBeta();
        // Inline clamp (upper bound first), not `ClipAlpha`: the range differs.
        m_fVerticalAngle = m_fVerticalAngle > DegreesToRadians(60.0f)
            ? DegreesToRadians(60.0f)
            : (m_fVerticalAngle < DegreesToRadians(-89.5f) ? DegreesToRadians(-89.5f) : m_fVerticalAngle);

        if (const auto* attached = targetPed->m_pAttachedTo; attached && targetPed->IsPlayer()) {
            float reference;
            switch (targetPed->m_fTurretAngleA) {
            case 0:  reference = attached->GetHeading() + HALF_PI; break;
            case 1:  reference = attached->GetHeading() + PI;      break;
            case 2:  reference = attached->GetHeading() - HALF_PI; break;
            case 3:  reference = attached->GetHeading();           break;
            default: reference = orientation;                      break;
            }
            auto delta = m_fHorizontalAngle - reference;
            if (delta > PI) {
                delta -= TWO_PI;
            } else if (delta < -PI) {
                delta += TWO_PI;
            }
            const auto limit = targetPed->m_fTurretAngleB;
            delta = delta > limit ? limit : (delta < -limit ? -limit : delta);
            m_fHorizontalAngle = delta + reference;
        }

        m_vecFront = CVector{
            std::cos(m_fHorizontalAngle) * std::cos(m_fVerticalAngle) * 3.0f,
            std::sin(m_fHorizontalAngle) * std::cos(m_fVerticalAngle) * 3.0f,
            std::sin(m_fVerticalAngle) * 3.0f
        };
        m_vecFront.Normalise();
        m_vecSource += m_vecFront * 0.4f;
        StaticRef<float>(0xB6F174) = m_fVerticalAngle;
        GetVectorsReadyForRW();

        auto* const player = TheCamera.m_pTargetEntity->AsPed();
        player->m_fCurrentRotation = player->m_fAimingRotation = std::atan2(-m_vecFront.x, m_vecFront.y);
        player->SetHeading(player->m_fCurrentRotation);
        if (player->GetRwObject()) {
            player->UpdateRwMatrix();
        }

        if (m_nMode == MODE_SNIPER_RUNABOUT) {
            const auto zoomFactor = (CTimer::GetTimeStep() * 255.0f + 10000.0f) * 0.0001f;
            if (pad->SniperZoomOut()) {
                m_fFOV = zoomFactor * m_fFOV;
            } else if (pad->SniperZoomIn()) {
                m_fFOV = m_fFOV / zoomFactor;
            }
            TheCamera.m_nBlurBlue   = 180;
            TheCamera.m_nBlurGreen  = 255;
            TheCamera.m_nBlurRed    = 180;
            TheCamera.m_nBlurType   = eMotionBlurType::SNIPER;
            TheCamera.m_nMotionBlur = 120;
            if (m_fFOV > 70.0f) {
                m_fFOV = 70.0f;
            }
            if (m_fFOV < 15.0f) {
                m_fFOV = 15.0f;
            }
        }
    }

    m_bResetStatics = false;
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.05f);
}

// 0x517EA0
void CCam::Process_1stPerson(const CVector& target, float orientation, float speedVar, float speedVarWanted) {
    static auto& s_LastWheelieTime = StaticRef<float>(0x8CCD14);
    // Making sure player doesn't see below ground when flipped.
    // Name is made up cuz I found it funny to name it like that.
    static auto& s_GroundFaultProtection = StaticRef<float>(0xB7004C);

    gbFirstPersonRunThisFrame = true;

    m_fFOV = 70.0f;
    if (!m_pCamTargetEntity->GetRwObject()) {
        return;
    }

    if (m_bResetStatics) {
        m_fVerticalAngle   = 0.0f;
        m_fHorizontalAngle = [&] {
            if (m_pCamTargetEntity->GetIsTypePed()) {
                return m_pCamTargetEntity->AsPed()->m_fCurrentRotation + DegreesToRadians(90.0f);
            } else {
                return orientation;
            }
        }();
        m_fInitialPlayerOrientation = m_fHorizontalAngle;

        s_GroundFaultProtection                 = 0.0f;
        TheCamera.m_fAvoidTheGeometryProbsTimer = 0.0f;
    }

    if (m_pCamTargetEntity->GetIsTypePed()) {
        m_bResetStatics = false;
        return;
    }

    const auto wheelieTime = static_cast<float>(CTimer::GetTimeInMS());
    if (s_LastWheelieTime > wheelieTime) {
        s_LastWheelieTime = 0.0f;
    }

    auto* targetVeh = m_pCamTargetEntity->AsVehicle();
    if (targetVeh->IsBike() && targetVeh->AsBike()->bikeFlags.bWheelieForCamera || TheCamera.m_fAvoidTheGeometryProbsTimer > 0.0f) {
        if (wheelieTime - s_LastWheelieTime >= 3000.0f) {
            s_LastWheelieTime = static_cast<float>(CTimer::GetTimeInMS());
        }

        const auto pad1 = CPad::GetPad();
        if (!pad1->NewState.LeftShoulder2 && !pad1->NewState.RightShoulder2) {
            auto* targetBike = targetVeh->AsBike();
            if (Process_WheelCam(target, orientation, speedVar, speedVarWanted)) {
                if (targetBike->bikeFlags.bWheelieForCamera) {
                    TheCamera.m_fAvoidTheGeometryProbsTimer = 50.0f;
                } else {
                    TheCamera.m_fAvoidTheGeometryProbsTimer -= CTimer::GetTimeStep();
                    targetBike->bikeFlags.bWheelieForCamera = true;
                }
                return;
            }
            TheCamera.m_fAvoidTheGeometryProbsTimer = 0.0f;
            targetBike->bikeFlags.bWheelieForCamera = false;

            s_LastWheelieTime = 0.0f;
        }
    }

    const auto& entityWorldMat = [&] {
        if (auto* t = targetVeh->AsBike(); t->IsBike()) {
            t->CalculateLeanMatrix();
            return t->m_mLeanMatrix;
        } else {
            return targetVeh->GetMatrix();
        }
    }();

    const auto dummyPos = [&] {
        const auto* vehStruct = targetVeh->GetVehicleModelInfo()->GetVehicleStruct();
        return vehStruct->m_avDummyPos[targetVeh->IsBoat() ? DUMMY_LIGHT_FRONT_MAIN : DUMMY_SEAT_FRONT] * CVector{0.0f, 1.0f, 1.0f}; // ignore x
    }() + CVector{ 0.0f, 0.08f, 0.62f };

    m_fFOV = 60.0f;
    m_vecSource = entityWorldMat.TransformVector(dummyPos);
    m_vecSource += targetVeh->GetPosition();

    if (targetVeh->IsBike() && targetVeh->m_pDriver) {
        auto*   targetBike = targetVeh->AsBike();
        CVector neckPos{};

        targetVeh->m_pDriver->GetTransformedBonePosition(neckPos, BONE_NECK, true);
        neckPos += targetBike->GetMoveSpeed() * CTimer::GetTimeStep();

        constexpr auto BIKE_1ST_PERSON_ZOFFSET = 0.15f; // 0x8CC7B4
        m_vecSource.z = neckPos.z + BIKE_1ST_PERSON_ZOFFSET;

        const auto right = CrossProduct(m_vecFront, m_vecUp);
        // right *= flt_8CCD0C; (=1.0f)

        if (!CWorld::GetIsLineOfSightClear(
            CrossProduct(m_vecSource, m_vecSource + right),
            CrossProduct(m_vecSource, m_vecSource - right),
            true,
            false,
            false,
            false
        )) {
            m_vecSource = targetBike->GetPosition();
            m_vecSource.z = neckPos.z + BIKE_1ST_PERSON_ZOFFSET + 0.62f;
        }
    } else if (targetVeh->IsBoat()) {
        m_vecSource.z += 0.5f;
    }

    // todo: refactor
    if (targetVeh->IsUpsideDown()) {
        if (s_GroundFaultProtection >= 0.5f) {
            s_GroundFaultProtection = 0.5f;
        } else {
            s_GroundFaultProtection += 0.03f;
        }
    } else if (s_GroundFaultProtection >= 0.0f) {
        s_GroundFaultProtection = 0.0f;
    } else {
        s_GroundFaultProtection -= 0.03f;
    }
    m_vecSource.z += s_GroundFaultProtection;

    m_vecFront = entityWorldMat.GetForward().Normalized();
    m_vecUp    = entityWorldMat.GetUp().Normalized();
    const auto a = CrossProduct(m_vecFront, m_vecUp).Normalized();
    m_vecUp = CrossProduct(a, m_vecFront).Normalized();

    if (float wl{}; CWaterLevel::GetWaterLevel(m_vecSource, wl, true) && m_vecSource.z < wl - 0.3f) {
        ApplyUnderwaterMotionBlur();
    }
    m_bResetStatics = false;
}

// 0x521500
void CCam::Process_AimWeapon(const CVector& target, float, float, float) {
    static auto& initialized = StaticRef<uint8>(0xB70110);
    static auto& bufferedTarget = StaticRef<CVector>(0xB70104);
    static auto& meleePitch = StaticRef<float>(0xB70100);
    static auto& meleeYaw = StaticRef<float>(0xB700FC);
    static auto& meleeSideTimer = StaticRef<float>(0xB700F8);
    static auto& meleeTargetBlend = StaticRef<float>(0xB700F4);
    static auto& manualRotation = StaticRef<bool>(0xB6EC44);
    static auto& timeWithoutInput = StaticRef<int32>(0xB6EC48);
    static auto& lastTargetPressed = StaticRef<uint32>(0xB6EC4C);
    static auto& recenterHeading = StaticRef<float>(0x8CC530);
    if (!(initialized & 1)) {
        initialized |= 1;
        bufferedTarget.Reset();
    }
    if (!m_pCamTargetEntity->GetIsTypePed() || !m_pCamTargetEntity->AsPed()->IsPlayer()) {
        return;
    }
    auto* ped = m_pCamTargetEntity->AsPed();
    auto* intelligence = ped->GetIntelligence();
    auto* gunTask = intelligence->GetTaskUseGun();
    auto* weapon = gunTask ? gunTask->m_WeaponInfo : CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, eWeaponSkill::STD);
    auto* vehicle = ped->bInVehicle ? ped->m_pVehicle : nullptr;
    const bool driver = vehicle && vehicle->m_pDriver == ped;
    int32 aimingType{};
    if (ped->bInVehicle) {
        aimingType = vehicle && (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) ? 1 : 2;
    } else if (intelligence->GetTaskJetPack()) {
        aimingType = 1;
    } else if (ped->GetActiveWeapon().IsTypeMelee()) {
        aimingType = 3;
    }
    const auto& settings = StaticRef<std::array<std::array<float, 7>, 4>>(0x8CC4C0)[aimingType];
    const auto timeStep = CTimer::GetTimeStep();
    const auto weaponType = ped->GetActiveWeapon().m_Type;
    auto wantedFOV = 70.0f;
    if (weaponType == WEAPON_AK47 || weaponType == WEAPON_M4) {
        wantedFOV = StaticRef<float>(0x8CC4B4);
    } else if (weaponType == WEAPON_COUNTRYRIFLE) {
        wantedFOV = StaticRef<float>(0x8CC4B8);
    }
    if (!TheCamera.m_bTransitionState) {
        if (m_bResetStatics && weaponType != WEAPON_COUNTRYRIFLE) {
            m_fFOV = wantedFOV;
        } else {
            const auto step = timeStep * StaticRef<float>(0x862F1C);
            m_fFOV = m_fFOV + step < wantedFOV ? m_fFOV + step : std::max(m_fFOV - step, wantedFOV);
        }
    }
    const bool melee = weapon->m_nWeaponFire == WEAPON_FIRE_MELEE;
    float pitchOffset, yawOffset;
    if (melee) {
        if (!(initialized & 2)) {
            initialized |= 2;
            meleePitch = StaticRef<float>(0x862F20);
        }
        if (!(initialized & 4)) {
            initialized |= 4;
            meleeYaw = StaticRef<float>(0x862F24);
        }
        auto pitch = StaticRef<float>(0x862F20);
        auto yaw = StaticRef<float>(0x862F24);
        float blend{};
        if (intelligence->GetTaskFighting() && ped->m_nMoveState < PEDMOVE_WALK && ped->m_pTargetedObject) {
            if (meleeSideTimer > timeStep) {
                meleeSideTimer -= timeStep;
            } else if (meleeSideTimer < -timeStep) {
                meleeSideTimer += timeStep;
            } else {
                const auto origin = ped->GetPosition() + CVector{0.0f, 0.0f, 0.75f};
                auto side = (ped->m_pTargetedObject->GetPosition() - ped->GetPosition()).Cross(CVector{0.0f, 0.0f, 1.0f});
                side *= 2.0f / std::max(0.7f, side.Magnitude());
                meleeSideTimer = CWorld::GetIsLineOfSightClear(origin, origin + side, true, true, false, true, false, true, true) ? 100.0f : -100.0f;
            }
            if (meleeSideTimer >= 0.0f) {
                pitch = StaticRef<float>(0x862F28);
                yaw = StaticRef<float>(0x862F2C);
                blend = 1.0f;
            }
        }
        if (m_bResetStatics) {
            meleePitch = pitch;
            meleeYaw = yaw;
            meleeTargetBlend = 0.0f;
        } else if (!TheCamera.m_bTransitionState) {
            const auto damping = std::pow(StaticRef<float>(0x862F30), timeStep);
            meleePitch = meleePitch * damping + pitch * (1.0f - damping);
            meleeYaw = meleeYaw * damping + yaw * (1.0f - damping);
            meleeTargetBlend = meleeTargetBlend * damping + blend * (1.0f - damping);
        }
        pitchOffset = DegreesToRadians(meleePitch);
        yawOffset = DegreesToRadians(meleeYaw);
    } else {
        meleeTargetBlend = 0.0f;
        const auto halfFOV = std::tan(DegreesToRadians(m_fFOV * 0.5f));
        yawOffset = std::atan((CCamera::m_f3rdPersonCHairMultX - 0.5f) * 2.0f * halfFOV);
        pitchOffset = std::atan((0.5f - CCamera::m_f3rdPersonCHairMultY) * 2.0f * halfFOV / CDraw::ms_fAspectRatio);
    }
    if (m_bResetStatics) {
        TheCamera.ResetDuckingSystem(ped);
        m_bRotating = false;
        m_bCollisionChecksOn = true;
        m_fAlphaSpeed = m_fBetaSpeed = 0.0f;
        manualRotation = true;
        timeWithoutInput = 60000;
        recenterHeading = -1001.0f;
        lastTargetPressed = 0;
        if (!CCamera::m_bUseMouse3rdPerson || ped->m_pTargetedObject) {
            m_fVerticalAngle = settings[3];
            if (vehicle) {
                m_fHorizontalAngle = ped->m_fCurrentRotation - HALF_PI - yawOffset;
                m_fVerticalAngle += std::asin(std::clamp(vehicle->GetMatrix().GetForward().z, -1.0f, 1.0f));
            } else if (!ped->m_pTargetedObject) {
                m_fHorizontalAngle = ped->m_fCurrentRotation - HALF_PI + yawOffset;
                if (ped->bIsStanding) {
                    m_fVerticalAngle -= std::asin(std::clamp(ped->field_578.Dot(ped->GetMatrix().GetForward()), -1.0f, 1.0f));
                    if (weaponType == WEAPON_EXTINGUISHER) {
                        m_fVerticalAngle += StaticRef<float>(0x8D610C);
                    }
                }
            }
        }
    }
    auto& forcedStep = StaticRef<float>(0xA44498);
    const auto forcedHeading = StaticRef<float>(0xA4449C);
    if (forcedStep > 0.0f) {
        auto delta = m_fHorizontalAngle - forcedHeading;
        if (delta < 0.0f) {
            delta += TWO_PI;
        }
        if (delta < forcedStep || TWO_PI - delta < forcedStep) {
            m_fHorizontalAngle = forcedHeading;
            forcedStep = 0.0f;
        } else {
            m_fHorizontalAngle += delta <= TWO_PI - delta ? -forcedStep : forcedStep;
        }
    }

    auto lookAt = target;
    ped->UpdateRpHAnim();
    lookAt.z = ped->GetPosition().z + 0.5f + settings[4];
    if (m_fFOV < 70.0f) {
        lookAt.z += std::min((70.0f - m_fFOV) / (70.0f - StaticRef<float>(0x8CC4B4)), 1.0f) * StaticRef<float>(0x858B1C);
    }
    const auto heightOffset = lookAt.z - target.z;
    auto sideDistance = 0.2f;
    if (!weapon->flags.bAimWithArm && ped->GetPlayerData()->m_pPedClothesDesc->HasVisibleNewHairCut(1)) {
        sideDistance = 0.3f;
    } else if (m_fFOV < 70.0f) {
        sideDistance += std::min((70.0f - m_fFOV) / (70.0f - StaticRef<float>(0x8CC4B8)), 1.0f) * StaticRef<float>(0x858B1C);
    }
    if (StaticRef<bool>(0x8CCE64)) {
        const auto right = m_vecFront.Cross(m_vecUp);
        const auto alignment = std::clamp(right.Dot(ped->GetMatrix().GetRight()), 0.0f, 1.0f);
        lookAt += right * ((1.0f - std::acos(alignment) * StaticRef<float>(0x858FB8)) * sideDistance);
    } else {
        lookAt += ped->GetMatrix().GetRight() * sideDistance;
    }
    if (auto* locked = ped->m_pTargetedObject) {
        CVector position;
        if (locked->GetIsTypePed() && !melee) {
            locked->AsPed()->GetBonePosition(&position, BONE_SPINE1, true);
        } else {
            position = locked->GetPosition();
        }
        if (melee) {
            position.z += heightOffset * StaticRef<float>(0x8CCE60);
        }
        if (!m_bResetStatics && intelligence->GetTaskFighting()) {
            const auto damping = std::pow(StaticRef<float>(0x8CC39C), timeStep);
            bufferedTarget = bufferedTarget * damping + position * (1.0f - damping);
        } else {
            bufferedTarget = position;
        }
    }

    auto* pad = CPad::GetPad(0);
    if (ped->m_pTargetedObject) {
        const auto delta = bufferedTarget - lookAt;
        auto pitch = std::atan2(delta.z, delta.Magnitude2D());
        if (melee) {
            pitch *= std::cos(yawOffset);
        } else {
            const auto distance = std::min((lookAt - m_vecSource).Magnitude(), settings[0]);
            const auto scale = (delta.Magnitude() + distance) / delta.Magnitude();
            pitchOffset *= scale;
            yawOffset *= scale;
        }
        auto yaw = std::atan2(-delta.x, delta.y) - HALF_PI + yawOffset;
        pitch -= pitchOffset;
        if (pitch < -PI) {
            pitch += TWO_PI;
        } else if (pitch > PI) {
            pitch -= TWO_PI;
        }
        const auto step = m_bResetStatics ? 1000.0f : timeStep * StaticRef<float>(0x8CC4A4);
        m_fVerticalAngle += std::clamp(pitch - m_fVerticalAngle, -step, step);
        auto yawDelta = yaw - m_fHorizontalAngle;
        if (yawDelta > PI) {
            yaw -= TWO_PI;
        } else if (yawDelta < -PI) {
            yaw += TWO_PI;
        }
        m_fHorizontalAngle += std::clamp(yaw - m_fHorizontalAngle, -step, step);
        m_fAlphaSpeed = m_fBetaSpeed = 0.0f;
    } else {
        const auto mouse = pad->NewMouseControllerState.GetAmountMouseMoved();
        const auto fovScale = m_fFOV / 80.0f;
        if (CCamera::m_bUseMouse3rdPerson && !pad->DisablePlayerControls && !mouse.IsZero()) {
            m_fHorizontalAngle += mouse.x * -2.5f * fovScale * CCamera::m_fMouseAccelHorzntl;
            m_fVerticalAngle += mouse.y * 4.0f * fovScale * CCamera::m_fMouseAccelVertical;
            m_fAlphaSpeed = m_fBetaSpeed = 0.0f;
        } else {
            const auto h = (float)-pad->AimWeaponLeftRight(ped);
            const auto v = (float)pad->AimWeaponUpDown(ped);
            const auto sensitivity = sq(StaticRef<float>(0x8CC4A0));
            auto horizontal = fovScale * StaticRef<float>(0x859B50) * std::abs(h) * h * timeStep * sensitivity;
            auto vertical = fovScale * StaticRef<float>(0x8631AC) * std::abs(v) * v * timeStep * sensitivity;
            const auto damping = std::pow(StaticRef<float>(std::abs(h) < 2.0f && std::abs(v) < 2.0f ? 0x8CCE58 : 0x8CCE5C), timeStep);
            horizontal = m_fBetaSpeed = horizontal * (1.0f - damping) + m_fBetaSpeed * damping;
            vertical = m_fAlphaSpeed = vertical * (1.0f - damping) + m_fAlphaSpeed * damping;
            if (vehicle && !driver && pad->GetEnterTargeting()) {
                const auto now = CTimer::GetTimeInMS();
                if ((float)(now - lastTargetPressed) < StaticRef<float>(0x8CCE54)) {
                    horizontal = PI;
                    vertical = 0.0f;
                } else {
                    lastTargetPressed = now;
                }
            } else if (driver) {
                if (h == 0.0f && v == 0.0f) {
                    if (!pad->GetWeapon(ped)) {
                        timeWithoutInput += (int32)(timeStep * StaticRef<float>(0x858B38) * StaticRef<float>(0x858C4C));
                    }
                } else {
                    timeWithoutInput = 0;
                }
                if (timeWithoutInput > StaticRef<int32>(0x8CC534)) {
                    manualRotation = false;
                    recenterHeading = ped->m_fCurrentRotation - HALF_PI + yawOffset;
                } else if (timeWithoutInput > StaticRef<int32>(0x8CC538)) {
                    auto delta = ped->m_fCurrentRotation - HALF_PI - yawOffset - m_fHorizontalAngle;
                    if (delta > TWO_PI) {
                        delta -= TWO_PI;
                    } else if (delta < StaticRef<float>(0x863234)) {
                        delta += TWO_PI;
                    }
                    if (delta < StaticRef<float>(0x858F20)) {
                        recenterHeading = ped->m_fCurrentRotation - HALF_PI + yawOffset;
                        timeWithoutInput = StaticRef<int32>(0x8CC534) + 1;
                        manualRotation = false;
                    } else {
                        manualRotation = true;
                    }
                } else {
                    manualRotation = true;
                    if (pad->GetWeapon(ped)) {
                        timeWithoutInput = 0;
                    }
                }
            }
            if (manualRotation) {
                recenterHeading = -1001.0f;
                m_fHorizontalAngle += horizontal;
                m_fVerticalAngle += vertical;
            } else {
                if (recenterHeading < StaticRef<float>(0x859948)) {
                    recenterHeading = ped->m_fCurrentRotation - HALF_PI;
                }
                if (!weapon->flags.bAimWithArm && !melee) {
                    const auto heading = recenterHeading + HALF_PI;
                    ped->m_fAimingRotation = ped->m_fCurrentRotation = heading;
                    ped->SetHeading(heading);
                    ped->UpdateRwMatrix();
                }
                auto step = timeStep * StaticRef<float>(0x8CC4A8);
                auto deadZone = 0.0f;
                if (driver) {
                    step *= StaticRef<float>(0x8CC4AC);
                    deadZone = StaticRef<float>(0x8CC4B0);
                }
                const auto GetCorrection = [&](float desired, float current) {
                    auto delta = desired - current;
                    if (delta > PI) {
                        delta -= TWO_PI;
                    } else if (delta < -PI) {
                        delta += TWO_PI;
                    }
                    return delta > deadZone ? delta - deadZone : delta < -deadZone ? delta + deadZone : 0.0f;
                };
                const auto correction = GetCorrection(recenterHeading - yawOffset, m_fHorizontalAngle);
                if (std::abs(correction) < step) {
                    manualRotation = true;
                }
                m_fHorizontalAngle += std::clamp(correction, -step, step);
                if (driver) {
                    const auto pitch = std::asin(std::clamp(vehicle->GetMatrix().GetForward().z, -1.0f, 1.0f)) + settings[3];
                    m_fVerticalAngle += std::clamp(GetCorrection(pitch, m_fVerticalAngle), -step, step);
                } else {
                    m_fVerticalAngle += vertical;
                }
            }
        }
    }
    ClipBeta();
    m_fVerticalAngle = std::clamp(m_fVerticalAngle, -settings[6], settings[5]);
    const auto distancePitch = m_fVerticalAngle > 0.0f ? std::min(settings[2] * m_fVerticalAngle, HALF_PI) : m_fVerticalAngle;
    const auto distance = std::cos(distancePitch) * settings[1] + settings[0];
    m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
    m_vecSource = lookAt - m_vecFront * distance;
    TheCamera.HandleCameraMotionForDuckingDuringAim(ped, &m_vecSource, &lookAt, false);
    m_vecTargetCoorsForFudgeInter = lookAt;
    CCamera::SetColVarsAimWeapon(aimingType);
    if (gCameraDirection == 3) {
        TheCamera.CameraGenericModeSpecialCases(ped);
        TheCamera.CameraPedAimModeSpecialCases(ped);
        TheCamera.CameraColDetAndReact(&m_vecSource, &lookAt);
        TheCamera.ImproveNearClip(nullptr, ped, &m_vecSource, &lookAt);
    }
    TheCamera.m_bCamDirectlyBehind = TheCamera.m_bCamDirectlyInFront = false;
    if (meleeTargetBlend > 0.0f && ped->m_pTargetedObject) {
        const auto blend = meleeTargetBlend * 0.5f;
        m_vecFront = (lookAt * (1.0f - blend) + bufferedTarget * blend - m_vecSource).Normalized();
    }
    GetVectorsReadyForRW();
    if ((!weapon->flags.bAimWithArm || ped->bIsDucking) && !melee && !ped->bInVehicle) {
        float heading = -1000.0f;
        if (weaponType == WEAPON_SPRAYCAN) {
            heading = std::atan2(-m_vecFront.x, m_vecFront.y) - yawOffset;
        } else if (ped->m_pTargetedObject) {
            const auto delta = ped->m_pTargetedObject->GetPosition() - ped->GetPosition();
            heading = std::atan2(-delta.x, delta.y);
        } else if (manualRotation) {
            heading = std::atan2(-m_vecFront.x, m_vecFront.y) - yawOffset;
        }
        if (heading > -100.0f) {
            ped->m_fCurrentRotation = ped->m_fAimingRotation = heading + StaticRef<float>(0x862F18);
            ped->SetHeading(heading);
            ped->UpdateRwMatrix();
        }
        TheCamera.m_pTargetEntity->AsPed()->GetPlayerData()->m_fLookPitch = TheCamera.Find3rdPersonQuickAimPitch();
    }
    m_bResetStatics = false;
}

// 0x512B10
void CCam::Process_AttachedCam() {
    m_fFOV = 70.0f;
    const float tilt = DegreesToRadians(TheCamera.m_fAttachedCamAngle);
    auto* attached = TheCamera.m_pAttachedEntity;
    m_vecSource = attached->GetMatrix().TransformVector(TheCamera.m_vecAttachedCamOffset) + attached->GetPosition();
    if (TheCamera.m_bLookingAtVector) {
        m_vecFront = attached->GetMatrix().TransformVector(TheCamera.m_vecAttachedCamLookAt) + attached->GetPosition() - m_vecSource;
    } else {
        m_vecFront = TheCamera.m_pTargetEntity->GetPosition() - m_vecSource;
    }
    m_vecFront.Normalise();
    const auto right = m_vecFront.Cross(CVector{0.0f, 0.0f, 1.0f}).Normalized();
    const auto up = right.Cross(m_vecFront).Normalized();

    if (float waterLevel{}; CWaterLevel::GetWaterLevel(m_vecSource, waterLevel, true) && m_vecSource.z < waterLevel - 0.3f) {
        ApplyUnderwaterMotionBlur();
    }
    m_vecUp = up * std::cos(tilt) + right * std::sin(tilt);
    CWorld::pIgnoreEntity = nullptr;
}

// 0x525E50
void CCam::Process_Cam_TwoPlayer() {
    NOTSA_UNREACHABLE();
}

// 0x519810
void CCam::Process_Cam_TwoPlayer_InCarAndShooting() {
    NOTSA_UNREACHABLE();
}

// 0x513510
void CCam::Process_Cam_TwoPlayer_Separate_Cars() {
    NOTSA_UNREACHABLE();
}

// 0x513BE0
void CCam::Process_Cam_TwoPlayer_Separate_Cars_TopDown() {
    m_fFOV = 80.0f;
    const auto p1 = FindPlayerEntity(PED_TYPE_PLAYER1), p2 = FindPlayerEntity(PED_TYPE_PLAYER2);

    const auto p1p2Centroid = (p1->GetPosition() + p2->GetPosition()) / 2.0f;
    const auto p1p2Distance = DistanceBetweenPoints(p1->GetPosition(), p2->GetPosition());

    auto camHeightMult = std::abs([&] {
        if (FindPlayerVehicle(PED_TYPE_PLAYER1)) {
            return FindPlayerVehicle(PED_TYPE_PLAYER2) ? 1.0f : 0.75f;
        }
        return FindPlayerVehicle(PED_TYPE_PLAYER2) ? 0.75f : 0.45f;
    }() - m_fCameraHeightMultiplier);

    if (const auto s = CTimer::GetTimeStep() / 200.0f; camHeightMult >= s) {
        camHeightMult = (camHeightMult >= 0.0f ? s : -s) + m_fCameraHeightMultiplier;
    }
    m_fCameraHeightMultiplier = camHeightMult;

    const auto v21 = std::max(p1p2Distance + 10.0f, 30.0f);
    m_vecSource.Set(
        p1p2Centroid.x,
        p1p2Centroid.y - v21 * std::sin(0.4f),
        p1p2Centroid.z - v21 * -std::cos(0.4f)
    );
    m_vecFront.Set(0.0f, std::sin(0.4f), -std::cos(0.4f));
    m_vecUp.Set(0.0f, m_vecSource.y, std::sin(0.4f));
    m_vecTargetCoorsForFudgeInter = m_vecSource;
}

// 0x51B850
bool CCam::Process_DW_BirdyCam(bool) {
    auto& lastCamMode = StaticRef<int32>(0x8CC488);
    auto& sceneStartTime = StaticRef<uint32>(0x8CCBA0);
    auto& clearFrames = StaticRef<int32>(0xB7007C);
    auto& positions = StaticRef<std::array<CVector, 2>>(0xB70080);
    auto& initialized = StaticRef<uint8>(0xB70098);
    auto& exitCam = StaticRef<bool>(0xB6EC72);
    const auto maxClearFrames = StaticRef<int32>(0x8CCD74);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }
    CEntity* entity{};
    CVehicle* vehicle{};
    CVector target, source, up, right, forward, velocity, angularVelocity;
    float speed{}, angularSpeed{};
    CColSphere sphere{};
    GetCoreDataForDWCineyCamMode(entity, vehicle, target, source, up, right, forward, velocity, speed, angularVelocity, angularSpeed, sphere);
    const auto now = CTimer::GetTimeInMS();
    initialized |= 1;
    if (!(initialized & 2)) {
        initialized |= 2;
        clearFrames = maxClearFrames;
    }

    if (lastCamMode != MODE_DW_BIRDY || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_BIRDY;
        s_DWCineyCamSceneEndTime = now + StaticRef<uint32>(0x8CCBB0);
        sceneStartTime = now;
        clearFrames = maxClearFrames;
        exitCam = false;
        gHandShaker[0].Reset();

        CEntity* ahead[128];
        CEntity* behind[128];
        int16 aheadCount{}, behindCount{};
        const auto aheadRadius = StaticRef<float>(0x8CCD70);
        const auto behindRadius = StaticRef<float>(0x8CCD68);
        const auto aheadCentre = target + forward * aheadRadius * StaticRef<float>(0x8CCD6C);
        const auto behindCentre = target - forward * behindRadius * StaticRef<float>(0x8CCD64);
        CWorld::FindObjectsInRange(aheadCentre, aheadRadius, true, &aheadCount, 127, ahead, false, false, false, true, true);
        CWorld::FindObjectsInRange(behindCentre, behindRadius, true, &behindCount, 127, behind, false, false, false, true, true);

        const auto isSuitable = [](CEntity* candidate) {
            return (candidate->m_bIsStatic || candidate->m_bIsStaticWaitingForCollision)
                && candidate->GetMatrix().GetUp().z > 0.9f
                && IsLampPost((eModelID)candidate->GetModelIndex());
        };
        CEntity* aheadPosts[128];
        CEntity* behindPosts[128];
        int16 aheadPostCount{}, behindPostCount{};
        for (int16 i = 0; i < aheadCount; i++) {
            if (isSuitable(ahead[i])) {
                aheadPosts[aheadPostCount++] = ahead[i];
            }
        }
        for (int16 i = 0; i < behindCount; i++) {
            if (isSuitable(behind[i])) {
                behindPosts[behindPostCount++] = behind[i];
            }
        }
        const auto getPosition = [](CEntity* post) {
            const auto& bounds = post->GetColModel()->m_boundBox;
            auto position = post->GetMatrix().TransformPoint(bounds.m_vecMax);
            const auto maxOffset = bounds.m_vecMax.z - bounds.m_vecMin.z * 0.5f;
            position.z -= 1.0f + (maxOffset - 1.0f) * (float)CGeneral::GetRandomNumber() * 3.0518509e-05f;
            return position;
        };
        bool found = false;
        for (int16 i = 0; i < aheadPostCount && !found; i++) {
            if (!isSuitable(aheadPosts[i])) {
                continue;
            }
            const auto start = getPosition(aheadPosts[i]);
            const auto nearTarget = target + (start - target).Normalized();
            if (!(0.0f < StaticRef<float>(0x8CCD60))) {
                continue;
            }
            // The inner search starts at the outer candidate's index in the original.
            for (int16 j = i; j < behindPostCount && !found; j++) {
                if (!isSuitable(behindPosts[j])) {
                    continue;
                }
                const auto end = getPosition(behindPosts[j]);
                if (CWorld::GetIsLineOfSightClear(start, end, true, false, false, false, false, true, true)
                    && CWorld::GetIsLineOfSightClear(end, nearTarget, true, false, false, false, false, true, true)) {
                    source = positions[0] = start;
                    positions[1] = end;
                    found = true;
                }
            }
        }
        if (!found) {
            exitCam = true;
            return false;
        }
    }

    const auto t = (float)(int32)(now - sceneStartTime) / (float)(int32)(s_DWCineyCamSceneEndTime - sceneStartTime);
    const auto offset = (positions[1] - positions[0]).Normalized() * StaticRef<float>(0x8CCD5C);
    const auto start = positions[0] + offset * 2.0f;
    const auto end = positions[1] - offset;
    if (!exitCam) {
        const auto blend = (1.0f + std::sin(DegreesToRadians(270.0f - t * 180.0f))) * 0.5f;
        source = end + (start - end) * blend;
    }
    if (IsTimeToExitThisDWCineyCamMode(MODE_BEHINDBOAT, source, target, t, false)) {
        exitCam = true;
        return false;
    }

    CColPoint collision{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const auto obstructed = CWorld::ProcessLineOfSight(target, source, collision, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        clearFrames = maxClearFrames;
    }
    Finalise_DW_CineyCams(source, target, 0.0f, 70.0f, 0.3f, 0.0f);
    return true;
}

// 0x51B120
bool CCam::Process_DW_CamManCam(bool) {
    auto& lastCamMode = StaticRef<int32>(0x8CC488);
    auto& sceneStartTime = StaticRef<uint32>(0x8CCBA0);
    auto& cameraPosition = StaticRef<CVector>(0xB70068);
    auto& clearFrames = StaticRef<int32>(0xB70074);
    auto& initialized = StaticRef<uint32>(0xB70078);
    auto& exitCam = StaticRef<bool>(0xB6EC71);
    const auto maxClearFrames = StaticRef<int32>(0x8CCD58);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }
    CEntity* entity{};
    CVehicle* vehicle{};
    CVector target, source, up, right, forward, velocity, angularVelocity;
    float speed{}, angularSpeed{};
    CColSphere sphere{};
    GetCoreDataForDWCineyCamMode(entity, vehicle, target, source, up, right, forward, velocity, speed, angularVelocity, angularSpeed, sphere);
    const auto now = CTimer::GetTimeInMS();
    initialized |= 1;
    if (!(initialized & 2)) {
        initialized |= 2;
        clearFrames = maxClearFrames;
    }
    if (lastCamMode != MODE_DW_CAM_MAN || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_CAM_MAN;
        s_DWCineyCamSceneEndTime = now + StaticRef<uint32>(0x8CCBAC);
        sceneStartTime = now;
        clearFrames = maxClearFrames;
        exitCam = false;
        gHandShaker[0].Reset();

        const auto searchRadius = StaticRef<float>(0x8CCD54);
        const auto searchCentre = target + forward * searchRadius;
        CEntity* candidates[16];
        int16 count{};
        CWorld::FindObjectsInRange(searchCentre, searchRadius, true, &count, 15, candidates, false, false, false, true, true);
        bool found = false;
        float closestDistance = 10000.0f;
        for (int16 i = 0; i < count; i++) {
            auto* candidate = candidates[i];
            if (!candidate->m_bIsStatic && !candidate->m_bIsStaticWaitingForCollision) {
                continue;
            }
            if (candidate->GetMatrix().GetUp().z <= 0.9f || !IsLampPost((eModelID)candidate->GetModelIndex())) {
                continue;
            }
            const auto distance = (candidate->GetPosition() - target).Magnitude2D();
            if (distance >= closestDistance || distance <= StaticRef<float>(0x8CCD50)) {
                continue;
            }
            const auto& bounds = candidate->GetColModel()->m_boundBox;
            auto position = candidate->GetMatrix().TransformPoint(bounds.m_vecMax);
            position.z -= bounds.m_vecMax.z;
            position.z += bounds.m_vecMin.z * 0.5f;
            const auto nearTarget = target + (position - target).Normalized();
            // The executable compares a constant here, rather than the direction's height.
            if (!(0.0f < StaticRef<float>(0x8CCD4C))
                || !CWorld::GetIsLineOfSightClear(position, nearTarget, true, false, false, false, false, true, true)) {
                continue;
            }
            found = true;
            closestDistance = distance;
            source = cameraPosition = position;
        }
        if (!found) {
            exitCam = true;
            return false;
        }
    }

    const auto t = (float)(int32)(now - sceneStartTime) / (float)(int32)(s_DWCineyCamSceneEndTime - sceneStartTime);
    if (!exitCam) {
        source = cameraPosition + (target - cameraPosition).Normalized() * StaticRef<float>(0x8CCD48);
    }
    const auto distanceFraction = std::clamp((target - source).Magnitude() / StaticRef<float>(0x8CCD44), 0.0f, 1.0f);
    const auto distanceBlend = (1.0f + std::sin(DegreesToRadians(270.0f - distanceFraction * 180.0f))) * 0.5f;
    auto fov = lerp(StaticRef<float>(0x8CCD3C), StaticRef<float>(0x8CCD40), distanceBlend);
    const auto introDuration = StaticRef<float>(0x8CCD38);
    if (t < introDuration) {
        const auto introFraction = std::clamp(t / introDuration, 0.0f, 1.0f);
        const auto introBlend = (1.0f + std::sin(DegreesToRadians(270.0f - introFraction * 180.0f))) * 0.5f;
        fov = lerp(StaticRef<float>(0x8CCD34), fov, introBlend);
    }
    if (IsTimeToExitThisDWCineyCamMode(MODE_CHRIS, source, target, t, false)) {
        exitCam = true;
        return false;
    }

    CColPoint collision{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const auto obstructed = CWorld::ProcessLineOfSight(target, source, collision, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        clearFrames = maxClearFrames;
    }
    const auto shake = std::clamp(std::max(velocity.Magnitude() * StaticRef<float>(0x8CCD30), StaticRef<float>(0x8CCD2C)), 0.0f, 1.0f);
    Finalise_DW_CineyCams(source, target, 0.0f, fov, 10.0f - fov * (1.0f / 70.0f) * 9.7f, shake);
    return true;
}

// Per-scene state of the helicopter chase camera: 0x9C bytes at 0xB6FEC0.
struct DWHeliChaseState {
    CVector end;
    CVector start;
    float forwardDistance;
    float backwardDistance;
    float height;
    float sideDistance;
    float zoomInFraction;
    float wideFOV;
    float closeFOV;
    float lookAhead;
    float roll;
    float nearClip;
    bool targetFrozen;
    int32 frozenFrames;
    int32 maxFrozenFrames;
    CVector frozenTarget;
    int32 searchAttempts;
    bool sourceObstructed;
    int32 maxClearFrames;
    int32 clearFrames;
    float zoomDistanceStart;
    float zoomDistanceEnd;
    float distanceFOVReduction;
    float minimumDistance;
    float searchSphereRadius;
    float FOVRange;
    bool targetObstructed;
    bool skipZoomIn;
    float zoomOutStartFOV;
    bool zoomingOut;
    uint32 zoomOutStartTime;
    uint32 zoomOutEndTime;
    float zoomOutFraction;
    uint32 zoomOutDuration;

    // 0x50E180
    void SetDefaults() {
        forwardDistance = sideDistance = 50.0f;
        backwardDistance = 30.0f;
        height = 55.0f;
        zoomInFraction = 0.05f;
        wideFOV = 70.0f;
        closeFOV = 22.0f;
        lookAhead = 1.0f;
        roll = 0.0f;
        nearClip = 10.0f;
        maxFrozenFrames = 30;
        searchAttempts = 8;
        maxClearFrames = 60;
        zoomDistanceStart = 100.0f;
        zoomDistanceEnd = 110.0f;
        distanceFOVReduction = 10.0f;
        minimumDistance = 5.0f;
        searchSphereRadius = 12.0f;
        zoomOutFraction = 0.75f;
        zoomOutDuration = 4000;
        targetFrozen = sourceObstructed = targetObstructed = skipZoomIn = zoomingOut = false;
        FOVRange = wideFOV - closeFOV;
        clearFrames = maxClearFrames;
        frozenFrames = maxFrozenFrames;
    }

    // 0x50E090
    void Randomise() {
        const auto randomScale = 3.0518509e-05f /* 0x858C7C */;
        forwardDistance *= (float)CGeneral::GetRandomNumber() * randomScale * 1.4f + 0.1f;
        backwardDistance *= (float)CGeneral::GetRandomNumber() * randomScale * 0.5f + 0.5f;
        height *= (float)CGeneral::GetRandomNumber() * randomScale * 0.5f + 0.5f;
        sideDistance *= (float)CGeneral::GetRandomNumber() * randomScale * 0.5f + 0.5f;
        zoomInFraction *= (float)CGeneral::GetRandomNumber() * randomScale * 1.5f + 0.5f;
    }
};
VALIDATE_SIZE(DWHeliChaseState, 0x9C);

// 0x51A740
bool CCam::Process_DW_HeliChaseCam(bool) {
    auto& state = StaticRef<DWHeliChaseState>(0xB6FEC0);
    auto& lastMode = StaticRef<int32>(0x8CC488);
    auto& startTime = StaticRef<uint32>(0x8CCBA0);
    auto& exitCam = StaticRef<bool>(0xB6EC70);
    auto& obstructionPosition = StaticRef<CVector>(0xB70058);
    auto& obstructionFrames = StaticRef<int32>(0x8CCD24);
    TheCamera.m_bUseNearClipScript = false;
    // The original consumes a random number to select from a single configuration.
    CGeneral::GetRandomNumber();
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }
    CEntity* entity{};
    CVehicle* vehicle{};
    CVector target, source, up, right, forward, velocity, angularVelocity;
    float speed{}, angularSpeed{};
    CColSphere sphere{};
    GetCoreDataForDWCineyCamMode(entity, vehicle, target, source, up, right, forward, velocity, speed, angularVelocity, angularSpeed, sphere);
    const auto now = CTimer::GetTimeInMS();
    if (lastMode != MODE_DW_HELI_CHASE || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastMode = MODE_DW_HELI_CHASE;
        startTime = now;
        s_DWCineyCamSceneEndTime = now + StaticRef<uint32>(0x8CCBA8);
        exitCam = false;
        state.SetDefaults();
        state.Randomise();
        gHandShaker[0].Reset();

        bool found = false;
        for (int32 i = 0; i < state.searchAttempts; i++) {
            state.start = target - forward * state.backwardDistance;
            state.end = target + forward * state.forwardDistance;
            state.start.z += state.height;
            state.end.z += state.height;
            const auto startSide = CGeneral::GetRandomNumber() < 0x3FFF ? -1.0f : 1.0f;
            const auto endSide = CGeneral::GetRandomNumber() < 0x3FFF ? -1.0f : 1.0f;
            const CVector horizontalRight{right.x, right.y, 0.0f};
            state.start += horizontalRight * state.sideDistance * startSide;
            state.end += horizontalRight * state.sideDistance * endSide;
            if (CWorld::TestSphereAgainstWorld(state.start, state.searchSphereRadius, nullptr, true, true, false, false, false, false)) {
                continue;
            }
            CColPoint collision{};
            CEntity* hitEntity{};
            CWorld::pIgnoreEntity = entity;
            const auto obstructed = CWorld::ProcessLineOfSight(target, state.start, collision, hitEntity, true, true, false, false, false, false, false, false);
            CWorld::pIgnoreEntity = nullptr;
            if (!obstructed) {
                state.skipZoomIn = CGeneral::GetRandomNumber() < 0x3FFF;
                state.zoomingOut = CGeneral::GetRandomNumber() < 0x3FFF;
                found = true;
                break;
            }
        }
        if (!found) {
            state.targetFrozen = state.sourceObstructed = state.targetObstructed = state.skipZoomIn = state.zoomingOut = false;
            state.FOVRange = state.wideFOV - state.closeFOV;
            state.clearFrames = state.maxClearFrames;
            state.frozenFrames = state.maxFrozenFrames;
            exitCam = true;
            return false;
        }
    }
    if (exitCam) {
        return false;
    }

    const auto t = (float)(int32)(now - startTime) / (float)(int32)(s_DWCineyCamSceneEndTime - startTime);
    source = state.start + (state.end - state.start) * t;
    target += forward * (speed * state.lookAhead) + forward;
    const auto distance2D = (target - source).Magnitude2D();
    if (distance2D < state.minimumDistance) {
        const auto direction = (target - source) / distance2D;
        source.x = target.x - direction.x * state.minimumDistance;
        source.y = target.y - direction.y * state.minimumDistance;
    }
    const auto smoothFraction = [](float fraction) {
        return (1.0f + std::sin(DegreesToRadians(270.0f - fraction * 180.0f))) * 0.5f;
    };
    auto fov = state.closeFOV;
    if (t < state.zoomInFraction && !state.skipZoomIn) {
        fov = lerp(state.wideFOV, state.closeFOV, smoothFraction(t / state.zoomInFraction));
    }
    const auto distance = (source - target).Magnitude();
    if (distance > state.zoomDistanceStart) {
        const auto fraction = std::clamp((distance - state.zoomDistanceStart) / (state.zoomDistanceEnd - state.zoomDistanceStart), 0.0f, 1.0f);
        fov -= smoothFraction(fraction) * state.distanceFOVReduction;
    }
    const auto roll = t * state.roll;

    if (state.sourceObstructed || CWorld::TestSphereAgainstWorld(source, StaticRef<float>(0x8CCD28), nullptr, true, true, false, false, false, false)) {
        StaticRef<uint32>(0xB70064) |= 1;
        if (!state.sourceObstructed) {
            obstructionPosition = source;
            state.sourceObstructed = true;
            obstructionFrames = 100;
        }
        if (obstructionFrames < 0) {
            --obstructionFrames;
            exitCam = true;
            return false;
        }
        source = obstructionPosition + (source - obstructionPosition) * StaticRef<float>(0x8CCD20);
        --obstructionFrames;
    }

    if (!state.targetFrozen) {
        CColPoint collision{};
        CEntity* hitEntity{};
        CWorld::pIgnoreEntity = entity;
        const auto obstructed = CWorld::ProcessLineOfSight(target, source, collision, hitEntity, true, true, false, false, false, false, false, false);
        CWorld::pIgnoreEntity = nullptr;
        if (obstructed) {
            state.targetObstructed = true;
            if (!state.zoomingOut && state.clearFrames < state.maxClearFrames / 4) {
                state.zoomOutStartFOV = fov;
                state.zoomingOut = true;
                state.zoomOutStartTime = now;
                state.zoomOutEndTime = now + state.zoomOutDuration;
            }
            if (state.clearFrames-- == 0) {
                state.frozenTarget = target;
                state.targetFrozen = true;
            }
        } else {
            state.clearFrames = std::min(state.clearFrames + 1, state.maxClearFrames);
        }
    } else {
        target = state.frozenTarget;
        if (state.frozenFrames-- == 0) {
            exitCam = true;
            return false;
        }
    }
    if (!state.zoomingOut && t >= state.zoomOutFraction) {
        state.zoomOutStartFOV = fov;
        state.zoomingOut = true;
        state.zoomOutStartTime = now;
        state.zoomOutEndTime = now + state.zoomOutDuration;
    }
    if (state.zoomingOut) {
        const auto fraction = std::clamp(
            (float)(((double)(int32)now - (double)(int32)state.zoomOutStartTime) / ((double)(int32)state.zoomOutEndTime - (double)(int32)state.zoomOutStartTime)),
            0.0f, 1.0f
        );
        fov = lerp(state.zoomOutStartFOV, state.wideFOV, smoothFraction(fraction));
    }
    if (IsTimeToExitThisDWCineyCamMode(MODE_FOLLOW_PED_WITH_BIND, source, target, t, false)) {
        exitCam = true;
        return false;
    }
    Finalise_DW_CineyCams(source, target, roll, fov, state.nearClip, 1.0f);
    return true;
}

// 0x51C760
bool CCam::Process_DW_PlaneCam1(bool) {
    static auto& lastCamMode       = StaticRef<int32>(0x8CC488);
    static auto& sceneStartTime    = StaticRef<uint32>(0x8CCBA0);
    static auto& sceneDuration     = StaticRef<uint32>(0x8CCBC0);
    static auto& maxClearFrames    = StaticRef<int32>(0x8CCD98);
    static auto& heightOffset      = StaticRef<float>(0x8CCDA8);
    static auto& forwardOffset     = StaticRef<float>(0x8CCDB0);
    static auto& minHeight         = StaticRef<float>(0x8CCDBC);
    static auto& exitCam           = StaticRef<bool>(0xB6EC76);
    static auto& clearFrames       = StaticRef<int32>(0xB700B8);
    static auto& staticsInitialized = StaticRef<uint32>(0xB700BC);

    static auto& waveAmplitude = StaticRef<float>(0x8CCD9C);
    static auto& waveFrequency = StaticRef<int32>(0x8CCDA0);
    static auto& sideOffset = StaticRef<float>(0x8CCDAC);
    static auto& sideSign = StaticRef<float>(0x8CCDB4);
    static auto& heightSign = StaticRef<float>(0xB700C0);
    static auto& heightTravel = StaticRef<float>(0x8CCDA4);
    static auto& minGroundDistance = StaticRef<float>(0x8CCDB8);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }

    CEntity* entity{};
    CVehicle* vehicle{};
    CVector dst, src, targetUp, targetRight, targetFwd, targetVel, targetAngVel;
    float targetSpeed{}, targetAngSpeed{};
    CColSphere colSphere{};
    GetCoreDataForDWCineyCamMode(
        entity, vehicle, dst, src, targetUp, targetRight, targetFwd,
        targetVel, targetSpeed, targetAngVel, targetAngSpeed, colSphere
    );

    if (dst.z < minHeight) {
        exitCam = true;
        return false;
    }

    const auto now = CTimer::GetTimeInMS();
    if (lastCamMode != MODE_DW_PLANECAM1 || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_PLANECAM1;
        s_DWCineyCamSceneEndTime = now + sceneDuration;
        exitCam = false;
        sceneStartTime = now;

        CColPoint colPoint{};
        CEntity* hitEntity{};
        CWorld::pIgnoreEntity = entity;
        const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
        CWorld::pIgnoreEntity = nullptr;
        if (obstructed) {
            exitCam = true;
            return false;
        }
        heightSign = CGeneral::GetRandomNumber() < 0x3FFF ? -1.0f : 1.0f;
        if (CGeneral::GetRandomNumber() < 0x3FFF) {
            sideSign = -1.0f;
        }
    } else if (TheCamera.GetRoughDistanceToGround() < minGroundDistance) {
        exitCam = true;
        return false;
    }

    const auto t = static_cast<float>(
        static_cast<double>(static_cast<int32>(now - sceneStartTime))
        / static_cast<int32>(s_DWCineyCamSceneEndTime - sceneStartTime)
    );
    const auto forward = targetFwd.Normalized();
    const auto right = targetRight.Normalized() * sideSign;
    src = dst + forward * forwardOffset + right * sideOffset + targetUp * heightOffset;
    src += targetUp * heightTravel * (t - 0.5f) * heightSign;
    // Windows converts each timestamp to a signed integer before subtracting.
    const auto waveStart = static_cast<double>(static_cast<int32>(sceneStartTime));
    const auto waveT = (static_cast<double>(static_cast<int32>(now)) - waveStart)
                    / (static_cast<double>(static_cast<int32>(s_DWCineyCamSceneEndTime)) - waveStart);
    const auto wave = std::sin(waveT * waveFrequency * 360.0 * 0.0174532924f);
    src += targetUp * waveAmplitude * static_cast<float>(wave);

    CColPoint colPoint{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;

    if (!(staticsInitialized & 1)) {
        staticsInitialized |= 1;
        clearFrames = maxClearFrames;
    }
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        // The original compares before incrementing, allowing maxClearFrames + 1.
        clearFrames = maxClearFrames;
    }

    if (IsTimeToExitThisDWCineyCamMode(MODE_BLOOD_ON_THE_TRACKS, src, dst, t, false)) {
        exitCam = true;
        return false;
    }
    Finalise_DW_CineyCams(src, dst, 0.0f, 70.0f, 5.0f, 1.0f);
    return true;
}

// 0x51CC30
bool CCam::Process_DW_PlaneCam2(bool) {
    static auto& lastCamMode       = StaticRef<int32>(0x8CC488);
    static auto& sceneStartTime    = StaticRef<uint32>(0x8CCBA0);
    static auto& sceneDuration     = StaticRef<uint32>(0x8CCBC4);
    static auto& maxClearFrames    = StaticRef<int32>(0x8CCDC0);
    static auto& heightOffset      = StaticRef<float>(0x8CCDCC);
    static auto& forwardOffset     = StaticRef<float>(0x8CCDD4);
    static auto& minHeight         = StaticRef<float>(0x8CCDDC);
    static auto& exitCam           = StaticRef<bool>(0xB6EC77);
    static auto& clearFrames       = StaticRef<int32>(0xB700C4);
    static auto& staticsInitialized = StaticRef<uint32>(0xB700C8);

    static auto& waveAmplitude = StaticRef<float>(0x8CCDC4);
    static auto& waveFrequency = StaticRef<int32>(0x8CCDC8);
    static auto& sideOffset = StaticRef<float>(0x8CCDD0);
    static auto& sideSign = StaticRef<float>(0x8CCDD8);
    static auto& forwardSign = StaticRef<float>(0xB700CC);
    static auto& unusedSign = StaticRef<float>(0xB700D0);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }

    CEntity* entity{};
    CVehicle* vehicle{};
    CVector dst, src, targetUp, targetRight, targetFwd, targetVel, targetAngVel;
    float targetSpeed{}, targetAngSpeed{};
    CColSphere colSphere{};
    GetCoreDataForDWCineyCamMode(
        entity, vehicle, dst, src, targetUp, targetRight, targetFwd,
        targetVel, targetSpeed, targetAngVel, targetAngSpeed, colSphere
    );

    if (dst.z < minHeight) {
        exitCam = true;
        return false;
    }

    const auto now = CTimer::GetTimeInMS();
    if (lastCamMode != MODE_DW_PLANECAM2 || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_PLANECAM2;
        s_DWCineyCamSceneEndTime = now + sceneDuration;
        exitCam = false;
        sceneStartTime = now;

        CColPoint colPoint{};
        CEntity* hitEntity{};
        CWorld::pIgnoreEntity = entity;
        const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
        CWorld::pIgnoreEntity = nullptr;
        if (obstructed) {
            exitCam = true;
            return false;
        }
        unusedSign = CGeneral::GetRandomNumber() < 0x3FFF ? -1.0f : 1.0f;
        forwardSign = CGeneral::GetRandomNumber() < 0x3FFF ? -1.0f : 1.0f;
        if (CGeneral::GetRandomNumber() < 0x3FFF) {
            sideSign = -1.0f;
        }
    }

    const auto t = static_cast<float>(
        static_cast<double>(static_cast<int32>(now - sceneStartTime))
        / static_cast<int32>(s_DWCineyCamSceneEndTime - sceneStartTime)
    );
    const auto forward = targetFwd.Normalized() * (1.0f - (t + t)) * forwardSign;
    const auto right = targetRight.Normalized() * (1.0f - t) * sideSign;
    src = dst + forward * forwardOffset + right * sideOffset + targetUp * heightOffset;
    // Windows converts each timestamp to a signed integer before subtracting.
    const auto waveStart = static_cast<double>(static_cast<int32>(sceneStartTime));
    const auto waveT = (static_cast<double>(static_cast<int32>(now)) - waveStart)
                    / (static_cast<double>(static_cast<int32>(s_DWCineyCamSceneEndTime)) - waveStart);
    const auto wave = std::sin(waveT * waveFrequency * 360.0 * 0.0174532924f);
    src += targetUp * waveAmplitude * static_cast<float>(wave);

    CColPoint colPoint{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;

    if (!(staticsInitialized & 1)) {
        staticsInitialized |= 1;
        clearFrames = maxClearFrames;
    }
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        // The original compares before incrementing, allowing maxClearFrames + 1.
        clearFrames = maxClearFrames;
    }

    if (IsTimeToExitThisDWCineyCamMode(MODE_IM_THE_PASSENGER_WOOWOO, src, dst, t, false)) {
        exitCam = true;
        return false;
    }
    Finalise_DW_CineyCams(src, dst, 0.0f, 70.0f, 5.0f, 1.0f);
    return true;
}

// 0x51D100
bool CCam::Process_DW_PlaneCam3(bool) {
    static auto& lastCamMode       = StaticRef<int32>(0x8CC488);
    static auto& sceneStartTime    = StaticRef<uint32>(0x8CCBA0);
    static auto& sceneDuration     = StaticRef<uint32>(0x8CCBC8);
    static auto& maxClearFrames    = StaticRef<int32>(0x8CCDE0);
    static auto& heightOffset      = StaticRef<float>(0x8CCDE4);
    static auto& forwardOffset     = StaticRef<float>(0x8CCDE8);
    static auto& minHeight         = StaticRef<float>(0x8CCDEC);
    static auto& exitCam           = StaticRef<bool>(0xB6EC78);
    static auto& clearFrames       = StaticRef<int32>(0xB700D4);
    static auto& staticsInitialized = StaticRef<uint32>(0xB700D8);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }

    CEntity* entity{};
    CVehicle* vehicle{};
    CVector dst, src, targetUp, targetRight, targetFwd, targetVel, targetAngVel;
    float targetSpeed{}, targetAngSpeed{};
    CColSphere colSphere{};
    GetCoreDataForDWCineyCamMode(
        entity, vehicle, dst, src, targetUp, targetRight, targetFwd,
        targetVel, targetSpeed, targetAngVel, targetAngSpeed, colSphere
    );

    if (dst.z < minHeight) {
        exitCam = true;
        return false;
    }

    const auto now = CTimer::GetTimeInMS();
    if (lastCamMode != MODE_DW_PLANECAM3 || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_PLANECAM3;
        s_DWCineyCamSceneEndTime = now + sceneDuration;
        exitCam = false;
        sceneStartTime = now;

        CColPoint colPoint{};
        CEntity* hitEntity{};
        CWorld::pIgnoreEntity = entity;
        const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
        CWorld::pIgnoreEntity = nullptr;
        if (obstructed) {
            exitCam = true;
            return false;
        }
    }

    const auto t = static_cast<float>(static_cast<int32>(now - sceneStartTime))
                 / static_cast<float>(static_cast<int32>(s_DWCineyCamSceneEndTime - sceneStartTime));
    const auto& bounds = entity->GetColModel()->GetBoundingBox();
    forwardOffset = (bounds.m_vecMax.y - bounds.m_vecMin.y) * 0.5f;
    forwardOffset += forwardOffset;
    src = dst + targetFwd * forwardOffset + targetUp * heightOffset;

    CColPoint colPoint{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const auto obstructed = CWorld::ProcessLineOfSight(dst, src, colPoint, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;

    if (!(staticsInitialized & 1)) {
        staticsInitialized |= 1;
        clearFrames = maxClearFrames;
    }
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        // The original compares before incrementing, allowing maxClearFrames + 1.
        clearFrames = maxClearFrames;
    }

    if (IsTimeToExitThisDWCineyCamMode(MODE_SYPHON_CRIM_IN_FRONT, src, dst, t, false)) {
        exitCam = true;
        return false;
    }
    Finalise_DW_CineyCams(src, dst, 0.0f, 70.0f, 5.0f, 1.0f);
    return true;
}

// 0x51C250
bool CCam::Process_DW_PlaneSpotterCam(bool) {
    static auto& lastCamMode = StaticRef<int32>(0x8CC488);
    static auto& sceneStartTime = StaticRef<uint32>(0x8CCBA0);
    static auto& sceneDuration = StaticRef<uint32>(0x8CCBB4);
    static auto& maxClearFrames = StaticRef<int32>(0x8CCD78);
    static auto& zoomStartFraction = StaticRef<float>(0x8CCD7C);
    static auto& initialFov = StaticRef<float>(0x8CCD80);
    static auto& distantFov = StaticRef<float>(0x8CCD84);
    static auto& zoomDistance = StaticRef<float>(0x8CCD88);
    static auto& searchDistance = StaticRef<float>(0x8CCD8C);
    static auto& searchDepth = StaticRef<float>(0x8CCD90);
    static auto& searchAttempts = StaticRef<int32>(0x8CCD94);
    static auto& clearFrames = StaticRef<int32>(0xB7009C);
    static auto& startingFov = StaticRef<float>(0xB700A0);
    static auto& useZoom = StaticRef<bool>(0xB700A4);
    static auto& cameraPosition = StaticRef<CVector>(0xB700A8);
    static auto& initialized = StaticRef<uint32>(0xB700B4);
    static auto& exitCam = StaticRef<bool>(0xB6EC73);

    TheCamera.m_bUseNearClipScript = false;
    if (!m_pCamTargetEntity || !m_pCamTargetEntity->GetIsTypeVehicle()) {
        return false;
    }
    CEntity* entity{};
    CVehicle* vehicle{};
    CVector target, source, up, right, forward, velocity, angularVelocity;
    float speed{}, angularSpeed{};
    CColSphere sphere{};
    GetCoreDataForDWCineyCamMode(entity, vehicle, target, source, up, right, forward, velocity, speed, angularVelocity, angularSpeed, sphere);
    const auto now = CTimer::GetTimeInMS();
    initialized |= 1;
    if (lastCamMode != MODE_DW_PLANE_SPOTTER || gLastFrameProcessedDWCineyCam < CTimer::GetFrameCounter() - 1u) {
        lastCamMode = MODE_DW_PLANE_SPOTTER;
        s_DWCineyCamSceneEndTime = now + sceneDuration;
        exitCam = false;
        sceneStartTime = now;
        if (searchAttempts > 0) {
            bool found = false;
            for (int32 i = 0; i < searchAttempts; i++) {
                source = target;
                source.z -= searchDepth;
                source.x += CGeneral::GetRandomNumberInRange(searchDistance * 0.5f, searchDistance);
                source.y += CGeneral::GetRandomNumberInRange(searchDistance * 0.5f, searchDistance);
                CColPoint collision{};
                CEntity* hitEntity{};
                CWorld::pIgnoreEntity = entity;
                found = CWorld::ProcessLineOfSight(target, source, collision, hitEntity, true, true, false, false, false, false, false, false);
                CWorld::pIgnoreEntity = nullptr;
                if (found) {
                    cameraPosition = collision.m_vecPoint;
                    cameraPosition.z += 2.0f;
                    break;
                }
            }
            if (!found) {
                exitCam = true;
                return false;
            }
        }
        useZoom = CGeneral::GetRandomNumber() < 0x3FFF;
    }

    const float t = (float)(int32)(now - sceneStartTime) / (float)(int32)(s_DWCineyCamSceneEndTime - sceneStartTime);
    source = cameraPosition;
    if ((source - target).Magnitude2D() < 5.0f) {
        exitCam = true;
        return false;
    }
    const auto ease = [](float ratio) {
        return (std::sin(DegreesToRadians(270.0f - std::clamp(ratio, 0.0f, 1.0f) * 180.0f)) + 1.0f) * 0.5f;
    };
    float fov = 70.0f;
    if (useZoom) {
        fov = (distantFov - initialFov) * ease(DistanceBetweenPoints(target, source) / zoomDistance) + initialFov;
        if (t < zoomStartFraction) {
            if (!(initialized & 2)) {
                initialized |= 2;
                startingFov = initialFov;
            }
            fov = (fov - startingFov) * ease(t / zoomStartFraction) + startingFov;
        }
    }
    CColPoint collision{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = entity;
    const bool obstructed = CWorld::ProcessLineOfSight(target, source, collision, hitEntity, true, true, false, false, false, false, false, false);
    CWorld::pIgnoreEntity = nullptr;
    if (!(initialized & 4)) {
        initialized |= 4;
        clearFrames = maxClearFrames;
    }
    if (obstructed) {
        if (clearFrames-- == 0) {
            exitCam = true;
            return false;
        }
    } else if (clearFrames++ > maxClearFrames) {
        clearFrames = maxClearFrames;
    }
    if (IsTimeToExitThisDWCineyCamMode(MODE_PLAYER_FALLEN_WATER, source, target, t, false)) {
        exitCam = true;
        return false;
    }
    Finalise_DW_CineyCams(source, target, 0.0f, fov, 10.0f - fov * (1.0f / 70.0f) * 9.7f, 1.0f);
    return true;
}

// 0x50F3F0 - debug
void CCam::Process_Editor(const CVector& target, float orientation, float speedVar, float speedVarWanted) {
    static auto& s_LookAtAngle     = StaticRef<float>(0xB6FFE4);
    static auto& s_DoRenderShadows = StaticRef<bool>(0xB7295A);

    if (m_bResetStatics) {
        m_vecSource.Set(796.0f, -937.0f, 40.0f);
        CEntity::SafeCleanUpRef(m_pCamTargetEntity);
        m_pCamTargetEntity = nullptr;
        m_bResetStatics    = false;
    }
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.3f);
    m_fFOV = 70.0f;

    static constexpr float _90DEG_PER_HOUR_IN_RAD_PER_MIN = 0.02617994f;
    const auto* pad = CPad::GetPad(1);
    m_fHorizontalAngle += pad->GetLeftStickX() * _90DEG_PER_HOUR_IN_RAD_PER_MIN / 19.0f;
    m_fVerticalAngle   += DegreesToRadians(static_cast<float>(pad->GetLeftStickY())) / 50.0f;

    m_fVerticalAngle = std::max(m_fVerticalAngle, DegreesToRadians(85.0f));
    if (m_fVerticalAngle >= DegreesToRadians(-85.0f)) {
        if (pad->IsSquareDown()) {
            s_LookAtAngle += 0.1f;
        } else if (pad->IsCrossDown()) {
            s_LookAtAngle -= 0.1f;
        } else {
            s_LookAtAngle = 0.0f;
        }
    } else {
        m_fVerticalAngle = DegreesToRadians(-85.0f);
    }
    s_LookAtAngle = std::clamp(s_LookAtAngle, -70.0f, 70.0f);

    m_vecFront = (m_pCamTargetEntity ? m_pCamTargetEntity->GetPosition() : m_vecSource - m_vecSource).Normalized();
    m_vecSource += s_LookAtAngle * m_vecFront;
    m_vecSource.z = std::min(m_vecSource.z, -450.0f);

    if (pad->IsRightShoulder2Pressed()) {
        FindPlayerEntity()->Teleport(m_vecSource, false);
    }

    const auto ClampByLoop = [](float& value, float min, float max) {
        while (value > max) {
            value -= 1.0f;
        }
        while (value < min) {
            value += 1.0f;
        }
    };
    ClampByLoop(m_vecSource.x, 5.0f, 115.0f);
    ClampByLoop(m_vecSource.y, 5.0f, 115.0f);

    GetVectorsReadyForRW();

    if (!pad->IsLeftShockPressed() && s_DoRenderShadows) {
        CShadows::StoreShadowToBeRendered(
            eShadowType::SHADOW_ADDITIVE,
            gpShadowExplosionTex,
            m_vecSource,
            {12.0f, 0.0f},
            {0.0f, -12.0f},
            128,
            128,
            128,
            128,
            1000.0f,
            false,
            1.0f,
            nullptr,
            false
        );
    }

    if (CHud::m_Wants_To_Draw_Hud) {
        NOTSA_LOG_DEBUG("CamX: {:0.3f} CamY: {:0.3f}  CamZ:  {:0.3f}", m_vecSource.x, m_vecSource.y, m_vecSource.z);
        NOTSA_LOG_DEBUG("Frontx: {:0.3f}, Fronty: {:0.3f}, Frontz: {:0.3f} ", m_vecFront.x, m_vecFront.y, m_vecFront.z);
        NOTSA_LOG_DEBUG("LookAT: {:0.3f}, LookAT: {:0.3f}, LookAT: {:0.3f} ", m_vecSource.x + m_vecFront.x, m_vecSource.y + m_vecFront.y, m_vecSource.z + m_vecFront.z);
    }
}

// 0x51D470
void CCam::Process_Fixed(const CVector& target, float orientation, float speedVar, float speedVarWanted) {
    if (m_nDirectionWasLooking != 3) {
        m_nDirectionWasLooking = 3; // todo: enum
    }

    m_vecSource = m_vecCamFixedModeSource;
    m_vecFront = (target - m_vecSource).Normalized();
    m_vecTargetCoorsForFudgeInter = target;
    GetVectorsReadyForRW();

    // inlined?
    const auto a = CrossProduct(
        m_vecFront,
        (m_vecCamFixedModeUpOffSet + CVector{ 0.0f, 0.0f, 1.0f }).Normalized()
    ).Normalized();
    m_vecUp = CrossProduct(
        a,
        m_vecFront
    );
    m_fFOV = 70.0f;

    if (float wl{}; CWaterLevel::GetWaterLevel(m_vecSource, wl, true) && m_vecSource.z < wl) {
        ApplyUnderwaterMotionBlur();
    }

    if (gAllowScriptedFixedCameraCollision) {
        const auto savedIgnoreEntity = CWorld::pIgnoreEntity;

        CWorld::pIgnoreEntity = FindPlayerVehicle();
        CVector out{};
        float   outDist{1.0f};
        if (TheCamera.ConeCastCollisionResolve(m_vecSource, target, out, 2.0f, 0.1f, outDist)) {
            m_vecSource.y = out.y;
            m_vecSource.z = out.z;
        }

        CWorld::pIgnoreEntity = savedIgnoreEntity;
    }
}

// 0x5B25F0
void CCam::Process_FlyBy(const CVector&, float, float, float) {
    static auto& fov = StaticRef<float>(0xBC4074);
    static auto& firstFov = StaticRef<float>(0xBC4078);
    static auto& targetMarker = StaticRef<uint32>(0xBC407C);
    static auto& sourceMarker = StaticRef<uint32>(0xBC4080);
    static auto& fovMarker = StaticRef<uint32>(0xBC4084);
    static auto& upMarker = StaticRef<uint32>(0xBC4088);

    if (TheCamera.m_bCutsceneFinished) {
        return;
    }
    const auto* fovPath = TheCamera.m_aPathArray[0].m_pArrPathData;
    const auto* upPath = TheCamera.m_aPathArray[1].m_pArrPathData;
    const auto* sourcePath = TheCamera.m_aPathArray[2].m_pArrPathData;
    const auto* targetPath = TheCamera.m_aPathArray[3].m_pArrPathData;
    m_vecUp = CVector{0.0f, 0.0f, 1.0f};
    if (!TheCamera.m_bStartingSpline) {
        m_fTimeElapsedFloat = 0.0f;
        m_nFinishTime = (uint32)(sourcePath[((int32)sourcePath[0] - 1) * 10 + 1] * 1000.0f);
        upMarker = fovMarker = 5;
        sourceMarker = targetMarker = 11;
        TheCamera.m_bStartingSpline = true;
        firstFov = fov = fovPath[2];
    } else {
        m_fTimeElapsedFloat += CTimer::GetTimeStepNonClipped() * 0.02f * 1000.0f;
    }
    const float time = (float)(uint32)m_fTimeElapsedFloat;
    const bool finished = time >= (float)m_nFinishTime;
    if (finished) {
        sourceMarker = ((uint32)sourcePath[0] - 1) * 10 + 1;
        targetMarker = ((uint32)targetPath[0] - 1) * 10 + 1;
        upMarker = ((uint32)upPath[0] - 1) * 4 + 1;
        fovMarker = ((uint32)fovPath[0] - 1) * 4 + 1;
    } else {
        TheCamera.m_fPositionAlongSpline = time / (float)m_nFinishTime;
        while (time >= (sourcePath[sourceMarker] - sourcePath[1]) * 1000.0f) {
            sourceMarker += 10;
        }
    }
    FindSplinePathPositionVector(&m_vecSource, sourcePath, time, &sourceMarker);
    if (!finished) {
        while (time >= (targetPath[targetMarker] - targetPath[1]) * 1000.0f) {
            targetMarker += 10;
        }
    }
    FindSplinePathPositionVector(&m_vecFront, targetPath, time, &targetMarker);
    if (!finished) {
        while (time >= (upPath[upMarker] - upPath[1]) * 1000.0f) {
            upMarker += 4;
        }
    }
    float upAngle{};
    FindSplinePathPositionFloat(&upAngle, upPath, time, &upMarker);
    upAngle = DegreesToRadians(upAngle) + HALF_PI;
    m_vecUp.x = std::cos(upAngle);
    m_vecUp.z = std::sin(upAngle);
    if (!finished) {
        while (time >= (fovPath[fovMarker] - fovPath[1]) * 1000.0f) {
            fovMarker += 4;
        }
    }
    FindSplinePathPositionFloat(&fov, fovPath, time, &fovMarker);
    if (finished) {
        TheCamera.m_fPositionAlongSpline = 1.0f;
        sourceMarker = targetMarker = upMarker = fovMarker = 0;
    }
    m_vecTargetCoorsForFudgeInter = m_vecFront;
    m_vecFront -= m_vecSource;
    m_vecFront.Normalise();
    const auto left = m_vecUp.Cross(m_vecFront);
    m_vecUp = m_vecFront.Cross(left).Normalized();
    m_fFOV = fov;
}

namespace {
// Values read from gta_sa.exe .rdata/.data (address in comment).
constexpr float kPassengerBlendRate     = 0.02f;        // 0x8CCEE8
constexpr float kBikePassengerHeight    = 0.4f;         // 0x8CCEE0
constexpr float kTrailerLengthScale     = 0.5f;         // 0x8CCEE4
constexpr float kHeliLookAtUpScale      = 0.6f;         // 0x8CC53C
constexpr float kHeightPitchFactor      = 0.3f;         // 0x8CCEDC
constexpr float kFovSpeedThreshold      = 0.4f;         // 0x8CC540
constexpr float kFovDecayBase           = 0.98f;        // 0x8CC544
constexpr float kLookSensitivity        = 0.007f;       // 0x8CC4A0
constexpr float kLookHorzScale          = 1.0f / 14.0f; // 0x859B50 (0.071428575)
constexpr float kLookVertScale          = 3.0f / 70.0f; // 0x8631AC (0.042857144)
constexpr float kSteerUpDownThreshold   = 120.0f;       // 0x858BB0
constexpr float kMouseHorzMultiplier    = -2.0f;        // 0x858B18
constexpr float kMouseTimerSeconds      = 50.0f;        // 0x858B40 (* 1.0 @ 0x8CCECC)
constexpr float kMovementHeadingMinSpd  = 0.02f;        // 0x858B38
constexpr float kStillSpeedSqThreshold  = 0.04f;        // 0x863244
constexpr float kClearanceFrontBase     = 1.5f;         // 0x8CCED4
constexpr float kClearanceSideBase      = 1.2f;         // 0x8CCED8
constexpr float kClearanceHeightScale   = 1.2f;         // 0x8CCED0
constexpr float kProstituteCamMargin    = 0.1f;         // 0x8CCEC8
constexpr float kProstituteCamSpeed     = 0.0035f;      // 0x8CCEC4
constexpr float kPitchFixGain           = 0.05f;        // 0x8CCEC0
constexpr float kPitchFixTolerance      = 0.05f;        // literal in the binary
constexpr float kBetaSpeedDeadzone      = 0.0001f;      // 0x8CCEBC
constexpr float kContactWheelsPitchGain = 0.075f;       // 0x8CCEB8
constexpr float kAlphaSpeedDeadzone     = 0.0001f;      // 0x8CCEB4
constexpr float kAlphaSnapEpsilon       = 0.0001f;      // 0x8CCEAC
constexpr float kBetaSnapEpsilon        = 0.0001f;      // 0x8CCEA4

// Per-category tuning, 7 categories x 15 floats @ 0x8CC600 (stride 0x3C).
enum CamSetting : size_t {
    S_HEIGHT_FACTOR = 0, S_BASE_DISTANCE = 1, S_HEIGHT_OFFSET = 2, S_MIN_DISTANCE_FACTOR = 3,
    S_HISTORY_DISTANCE = 4, S_PITCH_SMOOTH_BASE = 5, S_PITCH_MAX_STEP = 6,
    S_BETA_SMOOTH_BASE = 8, S_MAX_ANGULAR_SPEED = 9, S_LATERAL_TURN_GAIN = 10, S_TURN_RATE = 11,
    S_INPUT_SCALE = 12, S_UPPER_PITCH = 13, S_LOWER_PITCH = 14,
};
}

// 0x5245B0
void CCam::Process_FollowCar_SA(const CVector& target, float, float, float, bool preserveAngles) {
    if (!m_pCamTargetEntity->GetIsTypeVehicle()) {
        return;
    }
    auto* vehicle = m_pCamTargetEntity->AsVehicle();
    auto* pad = CPad::GetPad(vehicle->m_pDriver && vehicle->m_pDriver->m_nPedType == PED_TYPE_PLAYER2 ? 1 : 0);
    auto lookAt = target;
    TheCamera.ApplyVehicleCameraTweaks(vehicle);
    const auto model = vehicle->m_nModelIndex;
    const auto type = vehicle->m_nVehicleType;
    const auto subtype = vehicle->m_nVehicleSubType;
    const auto appearance = vehicle->GetVehicleAppearance();
    const bool remoteControlled = vehicle->GetStatus() == STATUS_REMOTE_CONTROLLED;

    uint32 category{};
    if (model == MODEL_RCBANDIT || model == MODEL_RCBARON || model == MODEL_RCTIGER || model == MODEL_RCCAM) {
        category = 5;
    } else if (model == MODEL_RCRAIDER || model == MODEL_RCGOBLIN) {
        category = 6;
    } else if (type == VEHICLE_TYPE_BIKE || subtype == VEHICLE_TYPE_QUAD) {
        category = 1;
    } else if (subtype == VEHICLE_TYPE_HELI) {
        category = 2;
    } else if (subtype == VEHICLE_TYPE_PLANE) {
        if (model == MODEL_HYDRA && vehicle->AsAutomobile()->m_wMiscComponentAngle >= StaticRef<int16>(0x8D33C8)) {
            category = 2;
        } else {
            category = model == MODEL_VORTEX ? 0 : 3;
        }
    } else if (subtype == VEHICLE_TYPE_BOAT) {
        category = 4;
    }
    const auto& settings = StaticRef<std::array<std::array<float, 15>, 7>>(0x8CC600)[category];
    const auto timeStep = CTimer::GetTimeStep();

    auto distance = TheCamera.m_fCarZoomSmoothed + settings[S_BASE_DISTANCE];
    int32 zoomIndex{};
    TheCamera.GetArrPosForVehicleType(static_cast<eVehicleType>(appearance), zoomIndex);
    float pitchOffset{};
    if (remoteControlled || TheCamera.m_nCarZoom == 2) {
        pitchOffset = StaticRef<std::array<float, 5>>(0x8CC430)[zoomIndex];
    } else if (TheCamera.m_nCarZoom == 1) {
        pitchOffset = StaticRef<std::array<float, 5>>(0x8CC41C)[zoomIndex];
    } else if (TheCamera.m_nCarZoom == 3) {
        pitchOffset = StaticRef<std::array<float, 5>>(0x8CC444)[zoomIndex];
    }

    const auto& bounds = vehicle->GetColModel()->GetBoundingBox();
    auto height = bounds.m_vecMax.z;
    auto length = std::abs(bounds.m_vecMin.y) * 2.0f;
    auto& passengerBlend = StaticRef<float>(0xB7011C);
    if (auto* trailer = vehicle->m_pVehicleBeingTowed) {
        if (passengerBlend < 1.0f) {
            passengerBlend = std::min(passengerBlend + timeStep * kPassengerBlendRate, 1.0f);
        }
        const auto& trailerBounds = trailer->GetColModel()->GetBoundingBox();
        length += (trailerBounds.m_vecMax - bounds.m_vecMin).Magnitude() * kTrailerLengthScale * passengerBlend;
        height += (std::max(trailerBounds.m_vecMax.z, height) - height) * passengerBlend;
        const auto blend = passengerBlend * 0.5f;
        lookAt = lookAt * (1.0f - blend) + trailer->GetPosition() * blend;
    } else if (subtype == VEHICLE_TYPE_BIKE || subtype == VEHICLE_TYPE_QUAD) {
        if (vehicle->m_apPassengers[0]) {
            if (passengerBlend < 1.0f) {
                passengerBlend = std::min(passengerBlend + timeStep * kPassengerBlendRate, 1.0f);
            }
        } else if (passengerBlend > 0.0f) {
            passengerBlend = std::max(passengerBlend - timeStep * kPassengerBlendRate, 0.0f);
        }
        height += kBikePassengerHeight * passengerBlend;
    } else {
        passengerBlend = 0.0f;
    }
    distance += TheCamera.m_fCurrentTweakDistance * length;
    const auto minimumDistance = TheCamera.m_fCurrentTweakDistance * length * settings[S_MIN_DISTANCE_FACTOR];
    const auto& matrix = vehicle->GetMatrix();
    if (appearance == VEHICLE_APPEARANCE_HELI && !remoteControlled) {
        lookAt += matrix.GetUp() * kHeliLookAtUpScale * height;
    } else {
        const auto heightOffset = height * settings[S_HEIGHT_FACTOR] - settings[S_HEIGHT_OFFSET];
        if (heightOffset > 0.0f) {
            lookAt.z += heightOffset;
            distance += heightOffset;
            pitchOffset += kHeightPitchFactor / distance * heightOffset;
        }
    }
    lookAt.z *= TheCamera.m_fCurrentTweakAltitude;
    pitchOffset += TheCamera.m_fCurrentTweakAngle;

    auto historyDistance = settings[S_HISTORY_DISTANCE];
    if (TheCamera.m_nCarZoom == 1 && category <= 1) {
        historyDistance *= 0.65f;
    }
    historyDistance = std::max(historyDistance, distance);
    m_fCaMaxDistance = distance;
    m_fCaMinDistance = 3.5f;

    const auto speed = vehicle->GetMoveSpeed();
    if (m_bResetStatics) {
        m_fFOV = 70.0f;
    } else {
        const auto forwardSpeed = speed.Dot(matrix.GetForward());
        if ((subtype == VEHICLE_TYPE_AUTOMOBILE || subtype == VEHICLE_TYPE_BIKE) && forwardSpeed > kFovSpeedThreshold) {
            m_fFOV += (forwardSpeed - kFovSpeedThreshold) * timeStep;
        }
        if (m_fFOV > 70.0f) {
            m_fFOV = (m_fFOV - 70.0f) * std::pow(kFovDecayBase, timeStep) + 70.0f;
        }
        m_fFOV = std::clamp(m_fFOV, 70.0f, 100.0f);
    }

    if (m_bResetStatics || TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront) {
        m_bResetStatics = m_bRotating = false;
        m_bCollisionChecksOn = true;
        TheCamera.m_bResetOldMatrix = true;
        if (!TheCamera.m_bJustCameOutOfGarage && !preserveAngles) {
            m_fVerticalAngle = 0.0f;
            m_fHorizontalAngle = vehicle->GetHeading() - HALF_PI;
            if (TheCamera.m_bCamDirectlyInFront) {
                m_fHorizontalAngle += PI;
            }
        }
        m_fBetaSpeed = m_fAlphaSpeed = 0.0f;
        m_fDistance = 1000.0f;
        m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
        m_avecTargetHistoryPos[0] = lookAt - m_vecFront * historyDistance;
        m_anTargetHistoryTime[0] = CTimer::GetTimeInMS();
        m_avecTargetHistoryPos[1] = lookAt - m_vecFront * distance;
        m_nCurrentHistoryPoints = 0;
        if (!TheCamera.m_bJustCameOutOfGarage && !preserveAngles) {
            m_fVerticalAngle = -pitchOffset;
        }
    }

    m_vecFront = (lookAt - m_avecTargetHistoryPos[0]).Normalized();
    const auto previousLength = (lookAt - m_avecTargetHistoryPos[1]).Magnitude();
    auto heading = std::atan2(-m_vecFront.x, m_vecFront.y) - HALF_PI;
    if (heading < -PI) {
        heading += TWO_PI;
    }
    auto movementHeading = heading;
    if (speed.Magnitude2D() > kMovementHeadingMinSpd) {
        movementHeading = std::atan2(-speed.x, speed.y) - HALF_PI;
    }
    if (movementHeading > heading + PI) {
        movementHeading -= TWO_PI;
    } else if (movementHeading < heading - PI) {
        movementHeading += TWO_PI;
    }
    const auto lateralSpeed = speed - m_vecFront * speed.Dot(m_vecFront);
    const auto turnFraction = std::min(timeStep * settings[S_LATERAL_TURN_GAIN] * lateralSpeed.Magnitude(), 1.0f);
    const auto turnStep = timeStep * settings[S_TURN_RATE];
    heading += std::clamp((movementHeading - heading) * turnFraction, -turnStep, turnStep);
    if (heading > m_fHorizontalAngle + PI) {
        heading -= TWO_PI;
    } else if (heading < m_fHorizontalAngle - PI) {
        heading += TWO_PI;
    }
    const auto headingVelocity = (heading - m_fHorizontalAngle) / std::max(timeStep, 1.0f);
    const auto pitch = std::asin(std::clamp(m_vecFront.z, -1.0f, 1.0f));
    if (previousLength < distance && minimumDistance < distance) {
        distance = std::max(minimumDistance, previousLength);
    }

    auto upperLimit = settings[S_UPPER_PITCH];
    const auto lowerLimit = settings[S_LOWER_PITCH];
    // Contact counters live in different layouts: CBike +0x804, CAutomobile (incl. planes) +0x960.
    const auto contactWheels = [&]() -> uint8 {
        if (type == VEHICLE_TYPE_BIKE) {
            return vehicle->AsBike()->m_nNoOfContactWheels;
        }
        if (type == VEHICLE_TYPE_AUTOMOBILE || subtype == VEHICLE_TYPE_PLANE) {
            return vehicle->AsAutomobile()->m_nNumContactWheels;
        }
        return 0;
    }();
    if (speed.SquaredMagnitude() < kStillSpeedSqThreshold && (type != VEHICLE_TYPE_BIKE || contactWheels >= 4)
        && subtype != VEHICLE_TYPE_HELI && (subtype != VEHICLE_TYPE_PLANE || contactWheels != 0)) {
        const auto side = matrix.GetForward().Cross(CVector{0.0f, 0.0f, 1.0f}).Normalized();
        const auto up = side.Cross(matrix.GetForward()).Normalized();
        if (up.Dot(m_vecFront) > 0.0f) {
            const auto heightAboveRoad = lookAt.z - vehicle->GetPosition().z + vehicle->GetHeightAboveRoad();
            const auto relativeHeading = m_fHorizontalAngle - (vehicle->GetHeading() - HALF_PI);
            const auto angle = std::asin(std::abs(std::sin(relativeHeading)));
            const auto cornerAngle = std::atan2(bounds.m_vecMax.x, -bounds.m_vecMin.y);
            const auto clearance = angle > cornerAngle
                ? (kClearanceSideBase + bounds.m_vecMax.x) / std::cos(std::max(0.0f, HALF_PI - angle))
                : (kClearanceFrontBase - bounds.m_vecMin.y) / std::cos(angle);
            upperLimit = std::atan2(heightAboveRoad, clearance * kClearanceHeightScale)
                + std::atan2(matrix.GetForward().z, matrix.GetForward().Magnitude2D()) * std::cos(relativeHeading);
            if (type == VEHICLE_TYPE_AUTOMOBILE && contactWheels > 1 && std::abs(vehicle->GetTurnSpeed().Dot(matrix.GetForward())) < 0.05f) {
                upperLimit += std::atan2(matrix.GetRight().z, matrix.GetRight().Magnitude2D()) * std::cos(relativeHeading + HALF_PI);
            }
        }
    }
    // Binary order: upper bound first, then lower bound (std::clamp is UB when upper < -lower).
    auto desiredPitch = pitch - pitchOffset;
    desiredPitch = desiredPitch > upperLimit ? upperLimit : (desiredPitch < -lowerLimit ? -lowerLimit : desiredPitch);
    const auto pitchStep = timeStep * settings[S_PITCH_MAX_STEP];
    const auto pitchCorrection = std::clamp((desiredPitch - m_fVerticalAngle) * (1.0f - std::pow(settings[S_PITCH_SMOOTH_BASE], timeStep)), -pitchStep, pitchStep);

    // FIX 1: AimWeapon* (0x541040 / 0x5410C0), not LookAround*.
    auto horizontal = (float)-pad->AimWeaponLeftRight(nullptr);
    auto vertical = CCamera::m_bUseMouse3rdPerson ? 0.0f : (float)pad->AimWeaponUpDown(nullptr);
    const auto fovScale = m_fFOV / 80.0f; // m_fFOV * 0.0125f
    const auto sensitivity = sq(kLookSensitivity);
    horizontal *= fovScale * kLookHorzScale * std::abs(horizontal) * sensitivity;
    vertical *= fovScale * kLookVertScale * std::abs(vertical) * sensitivity;
    bool fixPitch = true;
    switch (model) {
    case MODEL_PACKER:
    case MODEL_DOZER:
    case MODEL_DUMPER:
    case MODEL_CEMENT:
    case MODEL_ANDROM:
    case MODEL_HYDRA:
    case MODEL_TOWTRUCK:
    case MODEL_FORKLIFT:
    case MODEL_TRACTOR:
        vertical = 0.0f;
        break;
    default:
        if (model == MODEL_RCTIGER || (type == VEHICLE_TYPE_AUTOMOBILE && vehicle->handlingFlags.bHydraulicInst)) {
            horizontal = vertical = 0.0f;
        } else {
            fixPitch = false;
        }
        break;
    }
    if (gCameraDirection != 3) {
        horizontal = vertical = 0.0f;
    }
    if (category == 0 && std::abs((float)pad->GetSteeringUpDown()) > kSteerUpDownThreshold && vehicle->m_pDriver) {
        auto* task = vehicle->m_pDriver->GetTaskManager().GetActiveTask();
        if (task && task->GetTaskType() != TASK_COMPLEX_LEAVE_CAR) {
            const auto input = (float)pad->GetSteeringUpDown();
            vertical += fovScale * kLookVertScale * std::abs(input) * input * sensitivity * 0.5f;
        }
    }
    if (vertical > 0.0f) {
        vertical *= 0.5f;
    }

    auto& mouseTimer = StaticRef<float>(0xB70118);
    bool mouseControls{};
    if (CCamera::m_bUseMouse3rdPerson && !pad->DisablePlayerControls) {
        const auto mouse = pad->NewMouseControllerState.GetAmountMouseMoved();
        const auto mouseVertical = mouse.y * 2.0f;
        const auto mouseHorizontal = mouse.x * kMouseHorzMultiplier;
        const auto mouseSteering = subtype == VEHICLE_TYPE_PLANE || subtype == VEHICLE_TYPE_HELI ? CVehicle::m_bEnableMouseFlying : CVehicle::m_bEnableMouseSteering;
        if ((mouseHorizontal != 0.0f || mouseVertical != 0.0f) && (pad->NewState.m_bVehicleMouseLook || !mouseSteering)) {
            vertical = mouseVertical * fovScale * CCamera::m_fMouseAccelHorzntl;
            horizontal = mouseHorizontal * fovScale * CCamera::m_fMouseAccelHorzntl;
            m_fBetaSpeed = m_fAlphaSpeed = 0.0f;
            desiredPitch = m_fVerticalAngle;
            mouseTimer = 1.0f * kMouseTimerSeconds;
            mouseControls = true;
        } else if (mouseTimer > 0.0f) {
            desiredPitch = m_fVerticalAngle;
            m_fBetaSpeed = m_fAlphaSpeed = 0.0f;
            horizontal = vertical = 0.0f;
            mouseTimer = std::max(0.0f, mouseTimer - timeStep);
            mouseControls = true;
        }
    }
    if (auto* passenger = vehicle->m_apPassengers[0]) {
        auto* task = passenger->GetTaskManager().GetActiveTask();
        if (task && task->GetTaskType() == TASK_COMPLEX_PROSTITUTE_SOLICIT && static_cast<CTaskComplexProstituteSolicit*>(task)->bMoveCameraDown) {
            vertical = m_fVerticalAngle < upperLimit - kProstituteCamMargin ? timeStep * kProstituteCamSpeed : 0.0f;
        }
    }
    auto& pitchFixed = StaticRef<bool>(0xB70114);
    if (fixPitch) {
        if (gCameraMode != MODE_CAM_ON_A_STRING) {
            pitchFixed = false;
        }
        if (!pitchFixed && std::abs(pitchOffset + m_fVerticalAngle) > kPitchFixTolerance) {
            vertical = (-pitchOffset - m_fVerticalAngle) * kPitchFixGain;
        } else {
            pitchFixed = true;
        }
    }

    horizontal *= settings[S_INPUT_SCALE];
    vertical *= settings[S_INPUT_SCALE];
    const auto damping = std::pow(settings[S_BETA_SMOOTH_BASE], timeStep);
    m_fBetaSpeed = damping * m_fBetaSpeed + (1.0f - damping) * std::clamp(horizontal + headingVelocity, -settings[S_MAX_ANGULAR_SPEED], settings[S_MAX_ANGULAR_SPEED]);
    if (std::abs(m_fBetaSpeed) < kBetaSpeedDeadzone) {
        m_fBetaSpeed = 0.0f;
    }
    if (!mouseControls) {
        horizontal = timeStep * m_fBetaSpeed;
    }
    m_fHorizontalAngle += horizontal;
    if (TheCamera.m_bJustCameOutOfGarage) {
        m_fHorizontalAngle = CGeneral::GetATanOfXY(m_vecFront.x, m_vecFront.y) + PI;
    }
    ClipBeta();

    if (category < 2 && desiredPitch < m_fVerticalAngle && distance <= previousLength
        && (type == VEHICLE_TYPE_AUTOMOBILE || type == VEHICLE_TYPE_BIKE) && contactWheels > 1) {
        vertical += (desiredPitch - m_fVerticalAngle) * kContactWheelsPitchGain;
    }
    m_fAlphaSpeed = damping * m_fAlphaSpeed + (1.0f - damping) * vertical;
    const auto maxVerticalSpeed = vertical > 0.0f ? settings[S_MAX_ANGULAR_SPEED] * 0.5f : settings[S_MAX_ANGULAR_SPEED];
    m_fAlphaSpeed = std::clamp(m_fAlphaSpeed, -maxVerticalSpeed, maxVerticalSpeed);
    if (std::abs(m_fAlphaSpeed) < kAlphaSpeedDeadzone) {
        m_fAlphaSpeed = 0.0f;
    }
    auto correction = vertical;
    if (!mouseControls) {
        vertical = timeStep * m_fAlphaSpeed;
        correction = pitchCorrection;
    }
    desiredPitch += vertical;
    m_fVerticalAngle += correction;
    // Binary order: upper bound first, then lower bound.
    if (m_fVerticalAngle > upperLimit) {
        m_fVerticalAngle = upperLimit;
        m_fAlphaSpeed = 0.0f;
    } else if (m_fVerticalAngle < -lowerLimit) {
        m_fVerticalAngle = -lowerLimit;
        m_fAlphaSpeed = 0.0f;
    }

    auto& previousPitch = StaticRef<float>(0x8CCEB0);
    auto& previousHeading = StaticRef<float>(0x8CCEA8);
    if (std::abs(previousPitch - m_fVerticalAngle) < kAlphaSnapEpsilon) {
        m_fVerticalAngle = previousPitch;
    }
    previousPitch = m_fVerticalAngle;
    if (std::abs(previousHeading - m_fHorizontalAngle) < kBetaSnapEpsilon) {
        m_fHorizontalAngle = previousHeading;
    }
    previousHeading = m_fHorizontalAngle;

    m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
    RoundCamCoordinates(m_vecSource, 4);
    GetVectorsReadyForRW();
    TheCamera.m_bCamDirectlyBehind = TheCamera.m_bCamDirectlyInFront = false;
    m_vecSource = lookAt - m_vecFront * distance;
    m_vecTargetCoorsForFudgeInter = lookAt;
    m_avecTargetHistoryPos[2] = m_avecTargetHistoryPos[0];
    const auto historyPitch = desiredPitch + pitchOffset;
    const auto historyFront = CamFrontVector(m_fHorizontalAngle, historyPitch);
    m_avecTargetHistoryPos[0] = lookAt - historyFront * historyDistance;
    m_avecTargetHistoryPos[1] = lookAt - historyFront * distance;

    CCamera::SetColVarsVehicle(static_cast<eVehicleType>(category), TheCamera.m_nCarZoom);
    if (gCameraDirection == 3) {
        TheCamera.m_nExtraEntitiesCount = 0;
        CWorld::pIgnoreEntity = vehicle;
        TheCamera.CameraVehicleModeSpecialCases(vehicle);
        if (vehicle->vehicleFlags.bIsBig) {
            StaticRef<bool>(0x9655E5) = true;
        }
        TheCamera.CameraColDetAndReact(&m_vecSource, &lookAt);
        TheCamera.ImproveNearClip(vehicle, nullptr, &m_vecSource, &lookAt);
        CWorld::pIgnoreEntity = nullptr;
        RoundCamCoordinates(m_vecSource, 4);
    }
    TheCamera.m_bCamDirectlyBehind = TheCamera.m_bCamDirectlyInFront = false;
    RoundCamCoordinates(m_vecSource, 4);
    GetVectorsReadyForRW();
    StaticRef<CVector>(0xB6F018) = lookAt;
}

// 0x50F970
void CCam::Process_FollowPedWithMouse(const CVector& target, float orientation, float, float) {
    m_fFOV = 70.0f;
    if (!m_pCamTargetEntity->GetIsTypePed()) {
        return;
    }
    auto* pad = CPad::GetPad(0);
    if (m_bResetStatics) {
        m_bRotating = false;
        m_bCollisionChecksOn = true;
        pad->ClearMouseHistory();
        m_bResetStatics = false;
    }
    const auto* vehicle = FindPlayerVehicle();
    const auto onTrain = vehicle && vehicle->IsTrain();
    const auto cameraTarget = target + CVector{0.0f, 0.0f, StaticRef<float>(0x8CC7D0)};
    float horizontal{}, vertical{};
    if (pad->bPlayerSafe) {
        const auto direction = (m_vecSource - cameraTarget).Normalized();
        horizontal = (direction.z >= -0.9f ? std::atan2(direction.y, direction.x) : orientation + PI) - m_fHorizontalAngle;
    } else {
        const auto mouse = CPad::NewMouseControllerState.GetAmountMouseMoved();
        const auto fovScale = m_fFOV / 80.0f;
        if (mouse.IsZero() || pad->DisablePlayerControls) {
            horizontal = fovScale / 14.0f * CTimer::GetTimeStep() * StaticRef<float>(0x8CC7CC) * -(float)pad->LookAroundLeftRightOnPC();
            vertical = fovScale * (3.0f / 70.0f) * CTimer::GetTimeStep() * (float)pad->LookAroundUpDownOnPC() * StaticRef<float>(0x8CC7CC);
        } else {
            horizontal = CCamera::m_fMouseAccelHorzntl * fovScale * mouse.x * -2.5f;
            vertical = fovScale * mouse.y * 4.0f * CCamera::m_fMouseAccelVertical;
        }
    }
    const auto fadingOut = TheCamera.m_bFading && TheCamera.m_nFadeInOutFlag == eFadeFlag::FADE_OUT;
    if ((fadingOut && CDraw::FadeValue > StaticRef<uint32>(0x8CC7D4)) || CDraw::FadeValue > 200 || pad->bPlayerSafe) {
        vertical = std::clamp(StaticRef<float>(0x8CC7D8) - m_fVerticalAngle, -0.05f, 0.05f);
    }
    m_fHorizontalAngle += horizontal;
    m_fVerticalAngle += vertical;
    if (m_fHorizontalAngle > PI) {
        m_fHorizontalAngle -= TWO_PI;
    } else if (m_fHorizontalAngle < -PI) {
        m_fHorizontalAngle += TWO_PI;
    }
    m_fVerticalAngle = std::clamp(m_fVerticalAngle, DegreesToRadians(-89.5f), DegreesToRadians(45.0f));
    const auto distanceAngle = m_fVerticalAngle <= 0.0f
        ? m_fVerticalAngle
        : std::min(StaticRef<float>(0x8CC7C8) * m_fVerticalAngle, HALF_PI);
    const auto desiredDistance = std::cos(distanceAngle) * StaticRef<float>(0x8CC7C4) + StaticRef<float>(0x8CC7C0);
    if (TheCamera.m_bUseTransitionBeta) {
        m_fHorizontalAngle = m_fTransitionBeta;
    }
    if (TheCamera.m_bCamDirectlyBehind) {
        m_fHorizontalAngle = TheCamera.m_fPedOrientForBehindOrInFront + PI;
    }
    if (TheCamera.m_bCamDirectlyInFront) {
        m_fHorizontalAngle = TheCamera.m_fPedOrientForBehindOrInFront;
    }
    if (onTrain) {
        m_fHorizontalAngle = orientation;
    }
    m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
    m_vecSource = cameraTarget - m_vecFront * desiredDistance;
    m_vecTargetCoorsForFudgeInter = cameraTarget;

    CColPoint collision{};
    CEntity* hitEntity{};
    CWorld::pIgnoreEntity = m_pCamTargetEntity;
    if (CWorld::ProcessLineOfSight(cameraTarget, m_vecSource, collision, hitEntity, true, true, true, true, false, false, true, false)) {
        const auto hitDistance = (cameraTarget - collision.m_vecPoint).Magnitude();
        if (!hitEntity->GetIsTypePed() || desiredDistance - hitDistance <= 0.4f) {
            m_vecSource = collision.m_vecPoint;
            if (hitDistance < 0.6f) {
                RwCameraSetNearClipPlane(Scene.m_pRwCamera, std::max(hitDistance - 0.3f, 0.05f));
            }
        } else {
            const auto hitPosition = collision.m_vecPoint;
            if (CWorld::ProcessLineOfSight(hitPosition, m_vecSource, collision, hitEntity, true, true, true, true, false, false, true, false)) {
                const auto nextHitDistance = (cameraTarget - collision.m_vecPoint).Magnitude();
                m_vecSource = collision.m_vecPoint;
                if (nextHitDistance < 0.6f) {
                    RwCameraSetNearClipPlane(Scene.m_pRwCamera, std::max(nextHitDistance - 0.3f, 0.05f));
                }
            } else {
                RwCameraSetNearClipPlane(Scene.m_pRwCamera, std::min(desiredDistance - hitDistance - 0.35f, 0.9f));
            }
        }
    }
    CWorld::pIgnoreEntity = nullptr;

    const auto radiusScale = std::tan(DegreesToRadians(m_fFOV) * 0.5f) * CDraw::ms_fAspectRatio * 1.1f;
    auto nearClip = RwCameraGetNearClipPlane(Scene.m_pRwCamera);
    auto* obstruction = CWorld::TestSphereAgainstWorld(m_vecSource + m_vecFront * nearClip, nearClip * radiusScale, nullptr, true, true, false, true, false, false);
    for (int32 i = 0; i < 6 && obstruction; i++) {
        const auto displacement = s_TempSphereColPoints[0].m_vecPoint - m_vecSource;
        const auto perpendicular = displacement - m_vecFront * displacement.Dot(m_vecFront);
        const auto newClip = std::max(std::min(perpendicular.Magnitude() / radiusScale, nearClip), 0.1f);
        if (newClip < nearClip) {
            RwCameraSetNearClipPlane(Scene.m_pRwCamera, newClip);
        }
        if (newClip == 0.1f) {
            m_vecSource += (cameraTarget - m_vecSource) * 0.3f;
        }
        nearClip = RwCameraGetNearClipPlane(Scene.m_pRwCamera);
        obstruction = CWorld::TestSphereAgainstWorld(m_vecSource + m_vecFront * nearClip, nearClip * radiusScale, nullptr, true, true, false, true, false, false);
    }

    const auto distance = (cameraTarget - m_vecSource).Magnitude();
    if (m_fDistance > distance) {
        m_fDistance = distance;
    } else {
        const auto damping = std::pow(0.92f, CTimer::GetTimeStep());
        m_fDistance = damping * m_fDistance + (1.0f - damping) * distance;
        if (distance > 0.05f) {
            m_vecSource = cameraTarget + (m_vecSource - cameraTarget) * (m_fDistance / distance);
        }
        const auto maxNearClip = m_fDistance - StaticRef<float>(0x8CC38C);
        if (maxNearClip < RwCameraGetNearClipPlane(Scene.m_pRwCamera)) {
            RwCameraSetNearClipPlane(Scene.m_pRwCamera, std::max(maxNearClip, 0.1f));
        }
    }
    TheCamera.m_bCamDirectlyBehind = false;
    TheCamera.m_bCamDirectlyInFront = false;
    GetVectorsReadyForRW();
    if (fadingOut && CDraw::FadeValue > 128) {
        auto* player = TheCamera.m_pTargetEntity->AsPed();
        player->m_fCurrentRotation = player->m_fAimingRotation = std::atan2(-m_vecFront.x, m_vecFront.y);
        player->SetHeading(player->m_fCurrentRotation);
        player->UpdateRwMatrix();
    }
}

// 0x522D40
void CCam::Process_FollowPed_SA(const CVector& target, float, float, float, bool preserveAngles) {
    if (!m_pCamTargetEntity->GetIsTypePed() || !m_pCamTargetEntity->AsPed()->IsPlayer()) {
        return;
    }
    auto* ped = m_pCamTargetEntity->AsPed();
    auto* intelligence = ped->GetIntelligence();
    auto* pad = CPad::GetPad(ped->m_nPedType == PED_TYPE_PLAYER2 ? 1 : 0);
    const auto interior = CGame::currArea != 0;
    const auto& settings = StaticRef<std::array<std::array<float, 15>, 2>>(0x8CC548)[interior];
    const auto timeStep = CTimer::GetTimeStep();
    auto lookAt = target;
    auto distanceOffset = settings[1];
    if (!ped->bIsStanding && TheCamera.m_nPedZoom == 3 && intelligence->GetUsingParachute()) {
        distanceOffset *= 2.0f;
    }
    auto distance = TheCamera.m_fPedZoomSmoothed + distanceOffset;
    auto minimumDistance = settings[3];
    auto& previousDistance = StaticRef<float>(0xB6EC50);
    if (previousDistance < distance) {
        minimumDistance = distance;
    }
    previousDistance = distance;
    auto pitchOffset = settings[2];
    switch (TheCamera.m_nPedZoom) {
    case 1: pitchOffset += m_fTargetZoomOneZExtra; break;
    case 2: pitchOffset += interior ? m_fTargetZoomTwoInteriorZExtra : m_fTargetZoomTwoZExtra; break;
    case 3: pitchOffset += m_fTargetZoomThreeZExtra; break;
    }
    const auto upperLimit = settings[13], lowerLimit = settings[14];
    auto* swim = intelligence->GetTaskSwim();
    auto* jetpack = intelligence->GetTaskJetPack();
    float headingWeight{}, pitchWeight{};
    if (swim) {
        headingWeight = 1.0f;
        if (swim->m_nSwimState != SWIM_UNDERWATER_SPRINTING) {
            pitchWeight = StaticRef<float>(0x862F34);
        }
    } else if (jetpack) {
        headingWeight = 0.5f;
        if (!ped->bIsStanding) {
            pitchWeight = StaticRef<float>(0x862F3C);
        }
    }
    if (!TheCamera.m_bTransitionState) {
        if (m_bResetStatics) {
            m_fFOV = 70.0f;
        } else {
            const auto step = timeStep * StaticRef<float>(0x862F1C);
            m_fFOV = m_fFOV + step < 70.0f ? m_fFOV + step : std::max(m_fFOV - step, 70.0f);
        }
    }
    lookAt.z += settings[0];
    const auto historyDistance = std::max(distance, settings[4]);
    auto& forceBehind = StaticRef<bool>(0xB6EC54);
    const bool reset = m_bResetStatics || TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront || preserveAngles;
    if (reset) {
        if (preserveAngles) {
            StaticRef<CVector>(0x8CCC3C) = ped->GetPosition();
            StaticRef<CVector>(0xB6EC7C).Reset();
            lookAt = ped->GetPosition() + CVector{0.0f, 0.0f, settings[0]};
        }
        TheCamera.ResetDuckingSystem(ped);
        m_bRotating = false;
        forceBehind = false;
        m_bCollisionChecksOn = true;
        if (!TheCamera.m_bJustCameOutOfGarage && !preserveAngles) {
            m_fHorizontalAngle = ped->GetHeading() - HALF_PI;
            if (TheCamera.m_bCamDirectlyInFront) {
                m_fHorizontalAngle += PI;
            }
        }
        m_fBetaSpeed = m_fAlphaSpeed = 0.0f;
        m_fDistance = 1000.0f;
        if (distance == TheCamera.m_fPedZoomBase) {
            distance = TheCamera.m_fPedZoomSmoothed = TheCamera.m_fPedZoomTotal;
        }
        if (!TheCamera.m_bJustCameOutOfGarage && !preserveAngles) {
            m_fVerticalAngle = ped->bIsStanding ? -std::asin(std::clamp(ped->field_578.Dot(ped->GetMatrix().GetForward()), -1.0f, 1.0f)) : 0.0f;
        }
        m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
        m_avecTargetHistoryPos[0] = lookAt - m_vecFront * historyDistance;
        m_anTargetHistoryTime[0] = CTimer::GetTimeInMS();
        m_avecTargetHistoryPos[1] = lookAt - m_vecFront * distance;
        m_nCurrentHistoryPoints = 0;
        if (!TheCamera.m_bJustCameOutOfGarage && !preserveAngles) {
            m_fVerticalAngle = -pitchOffset;
        }
        if (swim && swim->m_nSwimState != SWIM_UNDERWATER_SPRINTING) {
            m_fVerticalAngle += StaticRef<float>(0x862F38);
        } else if (jetpack) {
            m_fVerticalAngle += StaticRef<float>(0x862F40);
        }
        CPad::GetPad(0)->ClearMouseHistory();
    } else if (auto* standingOn = ped->m_standingOnEntity) {
        auto* physical = standingOn->AsPhysical();
        const auto IsTrain = [](CEntity* entity) {
            return entity && entity->GetIsTypeVehicle() && entity->AsVehicle()->IsSubTrain();
        };
        if (IsTrain(standingOn) || IsTrain(physical->m_pAttachedTo)) {
            const auto speed = physical->GetMoveSpeed().Magnitude();
            const auto threshold = StaticRef<float>(0x8CCEA0);
            const auto fraction = std::max(0.0f, speed - threshold) / std::max(threshold, speed);
            const auto displacement = physical->GetMoveSpeed() * fraction * timeStep;
            m_avecTargetHistoryPos[0] += displacement;
            m_avecTargetHistoryPos[1] += displacement;
        }
    }
    m_vecFront = (lookAt - m_avecTargetHistoryPos[0]).Normalized();
    const auto previousLength = (lookAt - m_avecTargetHistoryPos[1]).Magnitude();
    if (previousLength < distance && settings[3] < distance) {
        distance = std::max(previousLength, minimumDistance);
    }
    auto heading = std::atan2(-m_vecFront.x, m_vecFront.y) - HALF_PI;
    if (heading < -PI) {
        heading += TWO_PI;
    }
    auto pedHeading = ped->GetHeading() - HALF_PI;
    if (pedHeading - heading > PI) {
        pedHeading -= TWO_PI;
    } else if (pedHeading - heading < -PI) {
        pedHeading += TWO_PI;
    }
    if (pad->GetForceCameraBehindPlayer()) {
        forceBehind = true;
    } else if (forceBehind && (ped->GetMoveSpeed().SquaredMagnitude() > StaticRef<float>(0x858CDC)
        || std::abs(pedHeading - heading) < StaticRef<float>(0x858C58)
        || pad->AimWeaponLeftRight(ped) || pad->AimWeaponUpDown(ped))) {
        forceBehind = false;
    }
    float rotationFraction{}, rotationStep{};
    if (std::abs(pedHeading - heading) < StaticRef<float>(0x8CCE9C) || forceBehind || headingWeight != 0.0f) {
        rotationFraction = timeStep * settings[10];
        rotationStep = timeStep * settings[11];
        if (!forceBehind && headingWeight == 0.0f) {
            const auto relativeSpeed = ped->m_standingOnEntity ? ped->GetMoveSpeed() - ped->m_standingOnEntity->AsPhysical()->GetMoveSpeed() : ped->GetMoveSpeed();
            rotationFraction = std::min(relativeSpeed.Magnitude() * rotationFraction, 1.0f);
        } else {
            rotationFraction = std::min(rotationFraction * StaticRef<float>(0x8CCE98), 1.0f);
            rotationStep *= StaticRef<float>(0x8CCE94);
            if (headingWeight != 0.0f) {
                rotationFraction *= headingWeight;
                rotationStep *= headingWeight;
            }
        }
        heading += std::clamp((pedHeading - heading) * rotationFraction, -rotationStep, rotationStep);
    }
    if (heading > m_fHorizontalAngle + PI) {
        heading -= TWO_PI;
    } else if (heading < m_fHorizontalAngle - PI) {
        heading += TWO_PI;
    }
    const auto headingVelocity = (heading - m_fHorizontalAngle) / std::max(timeStep, 1.0f);
    auto pitch = std::asin(std::clamp(m_vecFront.z, -1.0f, 1.0f));
    const auto headingDifference = std::abs(pedHeading - heading);
    if (headingDifference > StaticRef<float>(0x8CCE90) && ped->GetMoveSpeed().SquaredMagnitude() > StaticRef<float>(0x858F44)) {
        const auto fraction = std::min((headingDifference - StaticRef<float>(0x8CCE90)) * StaticRef<float>(0x858F08) / (PI - StaticRef<float>(0x8CCE90)), 1.0f);
        const auto limit = HALF_PI - (HALF_PI - StaticRef<float>(0x8CCE8C)) * fraction;
        const auto damping = std::pow(StaticRef<float>(0x8CCE88), timeStep);
        if (pitch > limit) {
            pitch = pitch * damping + limit * (1.0f - damping);
        } else if (pitch < -limit) {
            pitch = pitch * damping - limit * (1.0f - damping);
        }
    }
    if (pitchWeight != 0.0f || (forceBehind && ped->bIsStanding)) {
        float desired{};
        if (jetpack) {
            desired = StaticRef<float>(0x862F40);
        } else if (swim) {
            desired = StaticRef<float>(0x862F38);
        } else if (ped->bIsStanding) {
            desired = -std::asin(std::clamp(ped->field_578.Dot(ped->GetMatrix().GetForward()), -1.0f, 1.0f));
        }
        auto fraction = std::min(rotationFraction * StaticRef<float>(0x8CCE84), 1.0f);
        auto step = rotationStep * StaticRef<float>(0x8CCE80);
        if (pitchWeight != 0.0f) {
            fraction *= pitchWeight;
            step *= pitchWeight;
        }
        pitch += std::clamp((desired - pitch) * fraction, -step, step);
    }
    pitch = std::clamp(pitch - pitchOffset, -lowerLimit, upperLimit);
    const auto pitchStep = timeStep * settings[6];
    const auto pitchCorrection = std::clamp((pitch - m_fVerticalAngle) * (1.0f - std::pow(settings[5], timeStep)), -pitchStep, pitchStep);
    auto horizontal = (float)-pad->AimWeaponLeftRight(ped);
    auto vertical = (float)pad->AimWeaponUpDown(ped);
    if (auto* gun = intelligence->GetTaskUseGun(); gun && (gun->m_LastCmd == eGunCommand::FIRE || gun->m_LastCmd == eGunCommand::FIREBURST)
        && gun->m_WeaponInfo && !gun->m_WeaponInfo->flags.bAimWithArm) {
        if (std::abs(horizontal) < std::abs((float)pad->GetPedWalkLeftRight())) {
            horizontal = (float)-pad->GetPedWalkLeftRight();
        }
    }
    if ((vertical != 0.0f || horizontal != 0.0f) && !pad->GetPedWalkLeftRight() && !pad->GetPedWalkUpDown()) {
        auto* player = FindPlayerPed(PED_TYPE_PLAYER1);
        const auto forward = TheCamera.GetForwardVector();
        if (forward.Dot(player->GetMatrix().GetForward()) > StaticRef<float>(0x858C24)) {
            auto position = player->GetPosition() + forward * 5.0f;
            g_ikChainMan.LookAt("FollowPedSA", player, nullptr, 1500, BONE_UNKNOWN, &position, false, 0.25f, 500, 3, false);
        }
    }
    const auto scale = m_fFOV / 80.0f * sq(StaticRef<float>(0x8CC4A0));
    horizontal *= scale * StaticRef<float>(0x859B50) * std::abs(horizontal);
    vertical *= scale * StaticRef<float>(0x8631AC) * std::abs(vertical);
    if (auto* climb = intelligence->GetTaskClimb()) {
        climb->GetCameraStickModifier(ped, m_fVerticalAngle, m_fHorizontalAngle, vertical, horizontal);
    } else if (auto* task = ped->GetTaskManager().GetActiveTask(); task && task->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
        static_cast<CTaskComplexEnterCar*>(task)->GetCameraStickModifier(ped, distance, m_fVerticalAngle, m_fHorizontalAngle, vertical, horizontal);
    }
    const auto damping = std::pow(settings[8], timeStep);
    m_fBetaSpeed = damping * m_fBetaSpeed + (1.0f - damping) * std::clamp(horizontal + headingVelocity, -settings[9], settings[9]);
    if (std::abs(m_fBetaSpeed) < StaticRef<float>(0x8CCE7C)) {
        m_fBetaSpeed = 0.0f;
    }
    const bool mouseControls = CCamera::m_bUseMouse3rdPerson && !pad->DisablePlayerControls;
    const auto mouse = pad->NewMouseControllerState.GetAmountMouseMoved();
    if (mouseControls) {
        horizontal = m_fFOV / 80.0f * mouse.x * -2.5f * CCamera::m_fMouseAccelHorzntl;
        m_fBetaSpeed = 0.0f;
    } else {
        horizontal = timeStep * m_fBetaSpeed;
    }
    m_fHorizontalAngle += horizontal;
    ClipBeta();
    // The original uses the opposite damping weights for vertical input here.
    m_fAlphaSpeed = std::clamp(vertical * damping + m_fAlphaSpeed * (1.0f - damping), -settings[9], settings[9]);
    if (std::abs(m_fAlphaSpeed) < StaticRef<float>(0x8CCE78)) {
        m_fAlphaSpeed = 0.0f;
    }
    const auto verticalMotion = timeStep * m_fAlphaSpeed;
    auto correction = pitchCorrection;
    if (mouseControls) {
        correction = m_fFOV / 80.0f * mouse.y * StaticRef<float>(0x858FA0) * CCamera::m_fMouseAccelHorzntl;
        if ((TheCamera.m_bFading && TheCamera.GetFadingDirection() == 1 && CDraw::FadeValue > 45) || CDraw::FadeValue > 200) {
            correction = std::clamp(-pitchOffset - m_fVerticalAngle, -0.05f, 0.05f);
        }
        m_fAlphaSpeed = 0.0f;
    }
    m_fVerticalAngle += correction;
    if (m_fVerticalAngle > upperLimit || m_fVerticalAngle < -lowerLimit) {
        m_fVerticalAngle = std::clamp(m_fVerticalAngle, -lowerLimit, upperLimit);
        m_fAlphaSpeed = 0.0f;
    }
    auto& previousPitch = StaticRef<float>(0x8CCE74);
    auto& previousHeading = StaticRef<float>(0x8CCE6C);
    if (std::abs(previousPitch - m_fVerticalAngle) < StaticRef<float>(0x8CCE70)) {
        m_fVerticalAngle = previousPitch;
    }
    previousPitch = m_fVerticalAngle;
    if (std::abs(previousHeading - m_fHorizontalAngle) < StaticRef<float>(0x8CCE68)) {
        m_fHorizontalAngle = previousHeading;
    }
    previousHeading = m_fHorizontalAngle;
    m_vecFront = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);
    GetVectorsReadyForRW();
    TheCamera.m_bCamDirectlyBehind = TheCamera.m_bCamDirectlyInFront = false;
    m_vecSource = lookAt - m_vecFront * distance;
    RoundCamCoordinates(m_vecSource, 4);
    const auto historyPitch = verticalMotion + pitch + pitchOffset;
    const auto historyFront = CamFrontVector(m_fHorizontalAngle, historyPitch);
    m_avecTargetHistoryPos[0] = lookAt - historyFront * historyDistance;
    m_avecTargetHistoryPos[1] = lookAt - historyFront * distance;
    if (pad->GetForceCameraBehindPlayer() && pad->AimWeaponLeftRight(nullptr)) {
        auto delta = m_fHorizontalAngle - (ped->m_fCurrentRotation - HALF_PI);
        if (delta > PI) {
            delta -= TWO_PI;
        } else if (delta < -PI) {
            delta += TWO_PI;
        }
        if (std::abs(delta) < timeStep * StaticRef<float>(0x858B1C)) {
            ped->m_fAimingRotation = m_fHorizontalAngle + HALF_PI;
        }
    }
    TheCamera.HandleCameraMotionForDucking(ped, &m_vecSource, &lookAt, false);
    m_vecTargetCoorsForFudgeInter = lookAt;
    RoundCamCoordinates(m_vecSource, 4);
    CCamera::SetColVarsPed(interior ? PED_TYPE_PLAYER2 : PED_TYPE_PLAYER1, TheCamera.m_nPedZoom);
    if (gCameraDirection == 3) {
        TheCamera.CameraGenericModeSpecialCases(ped);
        TheCamera.CameraPedModeSpecialCases();
        TheCamera.CameraColDetAndReact(&m_vecSource, &lookAt);
        TheCamera.ImproveNearClip(nullptr, ped, &m_vecSource, &lookAt);
        RoundCamCoordinates(m_vecSource, 4);
    }
    TheCamera.m_bCamDirectlyBehind = TheCamera.m_bCamDirectlyInFront = false;
    RoundCamCoordinates(m_vecSource, 4);
    GetVectorsReadyForRW();
    if (!interior && TheCamera.m_nWhoIsInControlOfTheCamera != 1 && ped->bIsStanding && !CGameLogic::IsCoopGameGoingOn()
        && !TheCamera.m_bFOVLerpProcessed && !TheCamera.m_bVecMoveLinearProcessed && !TheCamera.m_bVecTrackLinearProcessed
        && ped->GetMoveSpeed().SquaredMagnitude() <= StaticRef<float>(0x858FC4)) {
        gIdleCam.Process();
    } else {
        gIdleCam.m_IdleTickerFrames = 0;
    }
    m_bResetStatics = false;
}

// 0x5105C0
void CCam::Process_M16_1stPerson(const CVector& target, float, float, float) {
    static auto& s_TargetFOV   = StaticRef<float>(0xB6FFE8);
    static auto& s_Obstructed  = StaticRef<bool>(0xB6FFF4);
    static auto& s_Unused_FFF0 = StaticRef<uint32>(0xB6FFF0);
    static auto& s_Unused_FFEC = StaticRef<uint32>(0xB6FFEC);
    static auto& forcedStep    = StaticRef<float>(0xA44498);
    static auto& forcedHeading = StaticRef<float>(0xA4449C);

    if (!m_pCamTargetEntity->GetIsTypePed()) {
        return;
    }
    auto* const ped = m_pCamTargetEntity->AsPed();

    auto pitchUpperLimit = DegreesToRadians(60.0f);
    auto pitchLowerLimit = 1.4922565f; // 85.5 degrees
    auto offsetUp        = 0.1f;
    auto offsetBack      = 0.3f;
    auto* const attached = ped->IsPlayer() ? ped->m_pAttachedTo : nullptr;

    if (m_bResetStatics) {
        if (!CCamera::m_bUseMouse3rdPerson || ped->m_pTargetedObject) {
            m_fHorizontalAngle = attached ? forcedHeading : ped->m_fCurrentRotation - HALF_PI;
            m_fVerticalAngle   = 0.0f;
        } else if (attached) {
            m_fHorizontalAngle = forcedHeading;
            m_fVerticalAngle   = 0.0f;
        }
        s_TargetFOV                 = m_fFOV;
        m_bResetStatics             = false;
        s_Obstructed                = false;
        s_Unused_FFF0               = 0;
        m_fInitialPlayerOrientation = ped->m_fCurrentRotation - HALF_PI;
        s_Unused_FFEC               = 0;
        m_bCollisionChecksOn        = true;
        m_fFOVSpeed                 = 0.0f;
        m_fAlphaSpeed               = 0.0f;
        m_fBetaSpeed                = 0.0f;
    }

    auto* const pad = CPad::GetPad(0);

    if (m_nMode == MODE_SNIPER || m_nMode == MODE_CAMERA) {
        const auto zoomInButton  = (int32)ControlsManager.GetMouseButtonAssociatedWithAction(eControllerAction::PED_SNIPER_ZOOM_IN);
        const auto zoomOutButton = (int32)ControlsManager.GetMouseButtonAssociatedWithAction(eControllerAction::PED_SNIPER_ZOOM_OUT);
        const auto wheel         = CPad::NewMouseControllerState.m_fWheelMoved;
        const auto wheelMatches  = [&](int32 button) { return (wheel > 0.0f && button == 4) || (wheel < 0.0f && button == 5); };

        bool zoomedByWheel = false;
        if (wheelMatches(zoomOutButton)) {
            s_TargetFOV   = (std::abs(wheel) * 7.0f + 10000.0f) * s_TargetFOV * 0.0001f;
            zoomedByWheel = true;
        } else if (wheelMatches(zoomInButton)) {
            s_TargetFOV   = s_TargetFOV / ((std::abs(wheel) * 7.0f + 10000.0f) * 0.0001f);
            zoomedByWheel = true;
        }

        const auto zoomStep = (CTimer::GetTimeStep() * 255.0f + 10000.0f) * 0.0001f;
        if (!pad->SniperZoomOut() || zoomedByWheel) {
            if (pad->SniperZoomIn() && !zoomedByWheel) {
                m_fFOV /= zoomStep;
                s_TargetFOV = m_fFOV;
                m_fFOVSpeed = 0.0f;
            } else if (std::abs(s_TargetFOV - m_fFOV) <= 0.5f) {
                m_fFOVSpeed = 0.0f;
            } else {
                WellBufferMe(s_TargetFOV, m_fFOV, m_fFOVSpeed, 0.5f, 0.25f, false);
            }
        } else {
            m_fFOV      = zoomStep * m_fFOV;
            s_TargetFOV = m_fFOV;
            m_fFOVSpeed = 0.0f;
        }

        if (m_fFOV > 70.0f) {
            m_fFOV = 70.0f;
        }
        if (s_TargetFOV <= 70.0f) {
            const auto minFOV = m_nMode == MODE_CAMERA ? 3.0f : 15.0f;
            m_fFOV            = std::max(m_fFOV, minFOV);
            s_TargetFOV       = std::max(s_TargetFOV, minFOV);
        } else {
            s_TargetFOV = 70.0f;
        }

        TheCamera.m_nBlurRed    = 180;
        TheCamera.m_nBlurGreen  = 255;
        TheCamera.m_nBlurBlue   = 180;
        TheCamera.m_nBlurType   = eMotionBlurType::SNIPER;
        TheCamera.m_nMotionBlur = 120;
    } else {
        m_fFOV = 70.0f;
    }

    if (attached && forcedStep > 0.0f) {
        auto delta = m_fHorizontalAngle - forcedHeading;
        if (delta < 0.0f) {
            delta += TWO_PI;
        }
        if (delta < forcedStep || TWO_PI - delta < forcedStep) {
            m_fHorizontalAngle = forcedHeading;
            forcedStep         = 0.0f;
        } else {
            m_fHorizontalAngle += delta <= TWO_PI - delta ? -forcedStep : forcedStep;
        }
    }

    const auto timeStep = CTimer::GetTimeStep();
    const auto fovScale = m_fFOV * 0.0125f;
    const auto mouse    = CPad::NewMouseControllerState.GetAmountMouseMoved();
    float      deltaBeta, deltaAlpha;
    if (mouse.IsZero()) {
        const auto h = (float)-pad->LookAroundLeftRight(ped);
        const auto v = (float)pad->LookAroundUpDown(ped);
        float      inputBeta, inputAlpha;
        if (attached) {
            inputBeta  = fovScale * 0.04f * std::abs(h * 0.0078125f) * timeStep * (h * 0.0078125f);
            inputAlpha = fovScale * 0.034285713f * std::abs(v * 0.0078125f) * timeStep * (v * 0.0078125f);
        } else {
            inputBeta  = fovScale * 0.057142857f * h * h * 0.0001f * timeStep * (h < 0.0f ? -1.0f : 1.0f);
            inputAlpha = fovScale * 0.071428575f * v * v * 4.4444445e-05f * timeStep * (v < 0.0f ? -1.0f : 1.0f);
        }
        const auto damping = std::pow(std::abs(h) < 2.0f && std::abs(v) < 2.0f ? 0.5f : 0.8f, timeStep);
        m_fBetaSpeed       = damping * m_fBetaSpeed + (1.0f - damping) * inputBeta;
        m_fAlphaSpeed      = (1.0f - damping) * inputAlpha + damping * m_fAlphaSpeed;
        deltaBeta          = m_fBetaSpeed;
        deltaAlpha         = m_fAlphaSpeed;
    } else {
        auto mouseBeta  = mouse.x * -3.0f;
        auto mouseAlpha = mouse.y * 3.0f;
        if (pad->DisablePlayerControls || pad->JustOutOfFrontEnd || timeStep <= 0.0f) {
            mouseBeta = mouseAlpha = 0.0f;
        }
        deltaBeta     = fovScale * CCamera::m_fMouseAccelHorzntl * mouseBeta;
        deltaAlpha    = fovScale * CCamera::m_fMouseAccelVertical * mouseAlpha;
        m_fAlphaSpeed = 0.0f;
        m_fBetaSpeed  = 0.0f;
    }
    m_fHorizontalAngle += deltaBeta;
    m_fVerticalAngle += deltaAlpha;
    ClipBeta();

    // Camera bump (see `DoCamBump`)
    if (const auto bumpTime = (int32)m_nCamBumpedTime; bumpTime > 0) {
        const auto elapsed = (float)(uint32)(CTimer::GetTimeInMS() - m_nCamBumpedTime);
        const auto phase   = std::cos(elapsed / 800.0f * TWO_PI);
        m_fHorizontalAngle += 0.1f * phase * m_fCamBumpedHorz;
        m_fVerticalAngle += phase * m_fCamBumpedVert * 0.1f;
        m_fCamBumpedHorz *= std::pow(0.95f, timeStep);
        m_fCamBumpedVert *= std::pow(0.95f, timeStep);
        if ((uint32)(bumpTime + 600) < CTimer::GetTimeInMS()) {
            m_nCamBumpedTime = 0;
        }
    }

    if (ped->bIsDucking) {
        offsetBack = 0.8f;
    }

    if (attached) {
        float      reference, pitchTarget;
        const auto Clamped = [](float v) { return std::clamp(v, -1.0f, 1.0f); };
        switch (ped->m_fTurretAngleA) {
        default:
            pitchTarget = std::asin(Clamped(attached->GetMatrix().GetForward().z));
            reference   = attached->GetHeading() - HALF_PI;
            break;
        case 1:
            pitchTarget = std::asin(Clamped(-attached->GetMatrix().GetRight().z));
            reference   = attached->GetHeading();
            break;
        case 2:
            pitchTarget = std::asin(Clamped(-attached->GetMatrix().GetForward().z));
            reference   = attached->GetHeading() + HALF_PI;
            break;
        case 3:
            pitchTarget = std::asin(Clamped(attached->GetMatrix().GetRight().z));
            reference   = attached->GetHeading() - PI;
            break;
        }

        ped->PositionAttachedPed();
        if (ped->GetRwObject()) {
            ped->UpdateRwMatrix();
        }
        ped->UpdateRwFrame();
        ped->UpdateRpHAnim();

        CVector headPos{};
        ped->GetBonePosition(&headPos, BONE_HEAD, true);
        if (attached->GetIsTypeVehicle() && attached->AsVehicle()->m_nVehicleType == VEHICLE_TYPE_BIKE) {
            offsetUp        = 0.0f;
            offsetBack      = 0.0f;
            pitchUpperLimit = ped->m_nTurretPosnMode;
            pitchLowerLimit = pitchUpperLimit;
        }
        m_vecSource = headPos;
        m_vecSource += ped->GetMatrix().GetUp() * offsetUp;
        m_vecSource -= ped->GetMatrix().GetForward() * offsetBack;

        const auto turnRange = ped->m_fTurretAngleB;
        const auto turnLimit = turnRange * 0.75f;
        const auto turnEase  = turnRange * 0.25f;
        if (reference - m_fHorizontalAngle > PI) {
            reference -= TWO_PI;
        } else if (reference - m_fHorizontalAngle < -PI) {
            reference += TWO_PI;
        }
        auto* const heli = attached->GetIsTypeVehicle() && attached->AsVehicle()->m_nVehicleSubType == VEHICLE_TYPE_HELI
            ? attached->AsAutomobile()
            : nullptr;

        auto excess  = reference - m_fHorizontalAngle;
        auto outside = true;
        if (excess <= turnLimit) {
            if (excess < -turnLimit) {
                excess += turnLimit;
            } else {
                outside = false;
            }
        } else {
            excess -= turnLimit;
        }
        if (outside) {
            const auto magnitude = std::abs(excess);
            auto       step      = timeStep * 0.05f * magnitude;
            if (turnEase + step < magnitude) {
                step = magnitude - turnEase;
            }
            if (heli && heli->m_fForcedOrientation > 0.0f) {
                heli->SetHeliOrientation(heli->m_fForcedOrientation - timeStep * excess * 0.1f);
            }
            if (excess != 0.0f) {
                m_fHorizontalAngle += excess >= 0.0f ? step : -step;
            }
        }

        const auto upperRange = pitchUpperLimit * 0.75f;
        if (upperRange + pitchTarget < m_fVerticalAngle) {
            const auto diff = pitchTarget - m_fVerticalAngle;
            if (diff < -upperRange) {
                const auto magnitude = std::abs(diff + upperRange);
                auto       step      = timeStep * 0.05f * magnitude;
                if (pitchUpperLimit * 0.25f + step < magnitude) {
                    step = magnitude - pitchUpperLimit * 0.25f;
                }
                m_fVerticalAngle -= step;
            }
        }
        const auto lowerRange = pitchLowerLimit * 0.75f;
        if (m_fVerticalAngle < pitchTarget - lowerRange) {
            const auto diff = pitchTarget - m_fVerticalAngle;
            if (lowerRange < diff) {
                const auto magnitude = std::abs(diff - lowerRange);
                auto       step      = timeStep * 0.05f * magnitude;
                if (pitchLowerLimit * 0.25f + step < magnitude) {
                    step = magnitude - pitchLowerLimit * 0.25f;
                }
                m_fVerticalAngle += step;
            }
        }
    } else {
        // Upper bound first, like the original.
        m_fVerticalAngle = m_fVerticalAngle > DegreesToRadians(60.0f)
            ? DegreesToRadians(60.0f)
            : (m_fVerticalAngle < -1.4922565f ? -1.4922565f : m_fVerticalAngle);

        if (ped->GetRwObject()) {
            ped->UpdateRwMatrix();
        }
        ped->UpdateRwFrame();
        ped->UpdateRpHAnim();

        CVector headPos{};
        ped->GetBonePosition(&headPos, BONE_HEAD, true);
        m_vecSource = headPos;
        m_vecSource.z += 0.1f;
        if (!ped->bIsDucking) {
            m_vecSource.x -= offsetBack * ped->GetMatrix().GetForward().x;
            m_vecSource.y -= offsetBack * ped->GetMatrix().GetForward().y;
        } else {
            m_vecSource.x -= 0.5f * ped->GetMatrix().GetForward().x;
            m_vecSource.y -= 0.5f * ped->GetMatrix().GetForward().y;
            m_vecSource.x -= 0.18f * ped->GetMatrix().GetRight().x;
            m_vecSource.y -= 0.18f * ped->GetMatrix().GetRight().y;
        }
    }

    constexpr auto MAX_PITCH = 1.2f; // 0x8CCC90
    m_fVerticalAngle         = std::clamp(m_fVerticalAngle, -MAX_PITCH, MAX_PITCH);
    m_vecFront               = CamFrontVector(m_fHorizontalAngle, m_fVerticalAngle);

    // Line-of-sight probes from the view direction and 35 degrees to either side.
    const auto rayEnd = m_vecFront * 3.0f + m_vecSource;
    m_vecSource += m_vecFront * 0.4f;
    const auto RayBlocked = [&](const CVector& end) {
        return !CWorld::GetIsLineOfSightClear(end, m_vecSource, true, true, false, true, false, true, true);
    };
    const auto ProbeBlocked = [&]() {
        if (RayBlocked(rayEnd)) {
            return true;
        }
        for (const auto betaOffset : { DegreesToRadians(35.0f), -DegreesToRadians(35.0f) }) {
            const auto    beta  = m_fHorizontalAngle + betaOffset;
            const auto    alpha = m_fVerticalAngle - DegreesToRadians(20.0f);
            const CVector end{
                std::cos(beta) * std::cos(alpha) * 3.0f + m_vecSource.x,
                std::sin(beta) * std::cos(alpha) * 3.0f + m_vecSource.y,
                std::sin(alpha) * 3.0f + m_vecSource.z
            };
            if (RayBlocked(end)) {
                return true;
            }
        }
        return false;
    };

    if (m_bCollisionChecksOn) {
        s_Obstructed = ProbeBlocked();
        if (s_Obstructed) {
            RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.3f);
        }
    }
    if (!s_Obstructed && m_nMode == MODE_CAMERA) {
        const auto fov = std::min(m_fFOV, 15.0f);
        RwCameraSetNearClipPlane(Scene.m_pRwCamera, ((15.0f - fov) * 0.15f + 1.0f) * 0.3f);
    }

    m_vecSource -= m_vecFront * 0.4f;
    GetVectorsReadyForRW();
    auto* const player         = TheCamera.m_pTargetEntity->AsPed();
    player->m_fCurrentRotation = std::atan2(-m_vecFront.x, m_vecFront.y);
    player->m_fAimingRotation  = player->m_fCurrentRotation;
}

// 0x511B50
void CCam::Process_Rocket(const CVector& target, float orientation, float speedVar, float speedVarWanted, bool isHeatSeeking) {
    static auto& dword_B6FFF8 = StaticRef<uint32>(0xB6FFF8);
    static auto& dword_B6FFFC = StaticRef<uint32>(0xB6FFFC);
    static auto& byte_B70000  = StaticRef<bool>(0xB70000);

    if (!m_pCamTargetEntity->GetIsTypePed()) {
        return;
    }

    auto* targetPed = m_pCamTargetEntity->AsPed();
    m_fFOV = 70.0f;
    if (m_bResetStatics) {
        if (!CCamera::m_bUseMouse3rdPerson || targetPed->m_pTargetedObject) {
            m_fVerticalAngle = 0.0f;
            m_fHorizontalAngle = targetPed->m_fCurrentRotation - DegreesToRadians(90.0f);
        }
        m_fInitialPlayerOrientation = m_fHorizontalAngle;
        m_bResetStatics             = 0;
        m_bCollisionChecksOn        = true;
        byte_B70000                 = 0;
        dword_B6FFFC                = 0;
        dword_B6FFF8                = 0;
    }
    m_pCamTargetEntity->UpdateRwMatrix();
    m_pCamTargetEntity->UpdateRwFrame();
    CVector headPosition{};
    targetPed->GetTransformedBonePosition(headPosition, eBoneTag::BONE_HEAD, true);
    m_vecSource = headPosition + CVector{0.0f, 0.0f, 0.1f};

    auto*      pad1   = CPad::GetPad(0);
    const auto fov    = m_fFOV / 80.0f;
    const auto amountMouseMoved = pad1->NewMouseControllerState.GetAmountMouseMoved();
    
    if (!amountMouseMoved.IsZero()) {
        m_fHorizontalAngle += -3.0f * amountMouseMoved.x * fov * CCamera::m_fMouseAccelHorzntl;
        m_fVerticalAngle += +4.0f * amountMouseMoved.y * fov * CCamera::m_fMouseAccelVertical;
    } else {
        const auto hv  = (float)-pad1->LookAroundLeftRight(targetPed);
        const auto vv  = (float)pad1->LookAroundUpDown(targetPed);

        m_fHorizontalAngle += sq(hv) / 10000.0f * fov / 17.5f * CTimer::GetTimeStep() * (hv < 0.0f ? -1.0f : 1.0f);
        m_fVerticalAngle   += sq(vv) / 22500.0f * fov / 14.0f * CTimer::GetTimeStep() * (vv < 0.0f ? -1.0f : 1.0f);
    }
    ClipBeta();
    ClipAlpha();

    m_vecFront.Set(
        -(std::cos(m_fHorizontalAngle) * std::cos(m_fVerticalAngle)),
        -(std::sin(m_fHorizontalAngle) * std::cos(m_fVerticalAngle)),
        std::sin(m_fVerticalAngle)
    );
    GetVectorsReadyForRW();

    const auto heading = CGeneral::GetATanOfXY(m_vecFront.x, m_vecFront.y) - DegreesToRadians(90.0f);
    TheCamera.m_pTargetEntity->AsPed()->m_fCurrentRotation = heading;
    TheCamera.m_pTargetEntity->AsPed()->m_fAimingRotation  = heading;

    if (isHeatSeeking) {
        auto* player     = FindPlayerPed();
        auto* playerData = player->GetPlayerData();
        if (!playerData->m_nFireHSMissilePressedTime) {
            playerData->m_nFireHSMissilePressedTime = CTimer::GetTimeInMS();
        }

        const auto hsTarget = CWeapon::PickTargetForHeatSeekingMissile(
            m_vecSource,
            m_vecFront,
            1.2f,
            player,
            false,
            playerData->m_LastHSMissileTarget
        );

        // NOTE: not sure about the second one
        if (hsTarget && CTimer::GetTimeInMS() - playerData->m_nLastHSMissileLOSTime > 1'000) {
            playerData->m_nLastHSMissileLOSTime = CTimer::GetTimeInMS();

            const auto targetUsesCollision = hsTarget->GetUsesCollision();
            const auto playerUsesCollision = player->GetUsesCollision();
            hsTarget->SetUsesCollision(false);
            player->SetUsesCollision(false);

            const auto isClear = CWorld::GetIsLineOfSightClear(
                player->GetPosition(),
                hsTarget->GetPosition(),
                true,
                true,
                false,
                true,
                false,
                true
            );
            player->SetUsesCollision(playerUsesCollision);
            hsTarget->SetUsesCollision(targetUsesCollision);
            playerData->m_bLastHSMissileLOS = isClear;
        }

        if (!playerData->m_bLastHSMissileLOS || !hsTarget || hsTarget != playerData->m_LastHSMissileTarget) {
            playerData->m_nFireHSMissilePressedTime = CTimer::GetTimeInMS();
        }

        if (hsTarget) {
            CWeaponEffects::MarkTarget(
                CrossHairId(0),
                hsTarget->GetPosition(),
                255,
                255,
                255,
                100,
                1.3f,
                true
            );
        }

        auto& crosshair = gCrossHair[CrossHairId(0)];
        const auto time = CTimer::GetTimeInMS() - playerData->m_nFireHSMissilePressedTime;

        crosshair.m_nTimeWhenToDeactivate = 0;
        crosshair.m_color.Set(
            255,
            time <= 1'500 ? 255 : 0,
            time <= 1'500 ? 255 : 0
        );
        crosshair.m_fRotation = time <= 1'500 ? 0.0f : 1.0f;
        playerData->m_LastHSMissileTarget = hsTarget;
    }

    constexpr auto ROCKET_CAM_NEARCLIP_PLANE = 0.15f; // 0x8CCC9C
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, ROCKET_CAM_NEARCLIP_PLANE);
}

// 0x517500
void CCam::Process_SpecialFixedForSyphon(const CVector&, float, float, float) {
    NOTSA_UNREACHABLE();
}

// 0x512110
bool CCam::Process_WheelCam(const CVector&, float, float, float) {
    auto* const entity = m_pCamTargetEntity;
    m_fFOV             = 70.0f;

    // The camera's right and up directions, before the sway is applied.
    CVector right{}, up{};

    CVector sourceOffset{ -0.3f, -0.5f, 0.1f };
    if (entity->GetIsTypePed()) {
        m_vecSource = entity->GetMatrix().TransformVector(sourceOffset) + entity->GetPosition();
        m_vecFront  = CVector{ 1.0f, 0.0f, 0.0f };
    } else {
        sourceOffset = CVector{ entity->GetColModel()->GetBoundingBox().m_vecMin.x - 0.33f, -2.3f, 0.3f }; // 0x8CC7DC
        m_vecSource  = entity->GetMatrix().TransformPoint(sourceOffset);
        m_vecFront   = entity->GetMatrix().GetForward();
    }

    auto* const vehicle = entity->GetIsTypeVehicle() ? entity->AsVehicle() : nullptr;
    const auto  appearance = vehicle ? vehicle->GetVehicleAppearance() : VEHICLE_APPEARANCE_NONE;

    const auto SetRightAndUpFromFront = [&] {
        right = m_vecFront.Cross(CVector{ 0.0f, 0.0f, 1.0f });
        right.Normalise();
        up = right.Cross(m_vecFront);
        up.Normalise();
    };

    if (vehicle && (appearance == VEHICLE_APPEARANCE_HELI || appearance == VEHICLE_APPEARANCE_PLANE)) {
        const auto& matrix = vehicle->GetMatrix();
        right              = matrix.GetRight();
        up                 = matrix.GetUp();
        m_vecSource        = matrix.TransformPoint(CVector{ -1.55f, sourceOffset.y, sourceOffset.z });
    } else {
        bool handled = false;
        if (vehicle) {
            const auto& matrix = vehicle->GetMatrix();
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
                SetRightAndUpFromFront();
                if (auto* const driver = vehicle->m_pDriver) {
                    CVector headPos{};
                    driver->GetBonePosition(&headPos, BONE_HEAD, true);
                    headPos += right * -0.5f + vehicle->GetMoveSpeed() * CTimer::GetTimeStep(); // 0x8CCC60
                    headPos += matrix.GetForward() * -0.8f;                                     // 0x8CCC64
                    headPos.z += 0.3f;                                                          // 0x8CCC68
                    if (vehicle->m_nModelIndex == MODEL_PREDATOR) {
                        headPos += right * 0.2f; // 0x8CCC6C
                        const auto back = (matrix.GetForward() * -1.0f) * -0.2f; // 0x8CCCA4, 0x8CCC70
                        headPos.x += back.x;
                        headPos.y += back.y;
                        headPos.z = 1.0f * -0.3f + headPos.z + back.z; // 0x8CCCA0 * 0x8CCC74
                    }
                    m_vecSource = headPos;
                } else {
                    m_vecSource.z += 0.3f + 0.3f;
                }
                handled = true;
            } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
                right = matrix.GetRight();
                up    = CVector{ 0.0f, 0.0f, 1.0f };
                m_vecFront = m_vecUp.Cross(right);
                m_vecFront.Normalise();
                const auto sideOffset = (0.33f - 0.2f) + sourceOffset.x; // (0x8CC7DC - 0x8CC7E0) + offset
                m_vecSource           = vehicle->GetPosition();
                m_vecSource += right * sideOffset;
                m_vecSource += m_vecFront * sourceOffset.y;
                m_vecSource += m_vecUp * sourceOffset.z;
                handled = true;
            } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_TRAIN && vehicle->GetMoveSpeed().Dot(m_vecFront) < 0.0f) {
                m_vecFront = -m_vecFront;
            }
        }
        if (!handled) {
            SetRightAndUpFromFront();
        }
    }

    // Underwater tint
    if (float waterLevel{}; CWaterLevel::GetWaterLevel(m_vecSource, waterLevel, true) && m_vecSource.z < waterLevel - 0.3f) {
        ApplyUnderwaterMotionBlur();
    }

    // Slow roll sway around the view direction
    const auto swayPhase = std::cos((float)(CTimer::GetTimeInMS() & 0x1FFFF) * 4.79369e-05f);
    const auto sine      = std::sin(swayPhase * 0.4f);
    const auto cosine    = std::cos(swayPhase * 0.4f);
    m_vecUp              = up * cosine + right * sine;
    m_vecUp.Normalise();
    m_vecFront.Normalise();

    CWorld::pIgnoreEntity = entity;
    CColPoint  colPoint{};
    CEntity*   hitEntity{};
    const auto blocked = CWorld::ProcessLineOfSight(m_vecSource, entity->GetPosition(), colPoint, hitEntity, true, false, false, true, false, false, true, false);
    CWorld::pIgnoreEntity = nullptr;
    return !blocked;
}

// based on 0x51847C - 0x5184EC
void CCam::ApplyUnderwaterMotionBlur() {
    static constexpr uint32 UNDERWATER_CAM_BLUR      = 20;    // 0x8CC7A4
    static constexpr float  UNDERWATER_CAM_MAG_LIMIT = 10.0f; // 0x8CC7A8

    const auto colorMag = std::sqrt(
        sq(CTimeCycle::GetWaterRed()) +
        sq(CTimeCycle::GetWaterGreen()) +
        sq(CTimeCycle::GetWaterBlue())
    );

    const auto factor = (colorMag <= UNDERWATER_CAM_MAG_LIMIT) ? 1.0f : UNDERWATER_CAM_MAG_LIMIT / colorMag;

    TheCamera.SetMotionBlur(
        static_cast<uint32>(factor * CTimeCycle::GetWaterRed()),
        static_cast<uint32>(factor * CTimeCycle::GetWaterGreen()),
        static_cast<uint32>(factor * CTimeCycle::GetWaterBlue()),
        UNDERWATER_CAM_BLUR,
        eMotionBlurType::LIGHT_SCENE
    );
}

// 0x4D58A0
int32 ConvertPedNode2BoneTag(int32 simpleId) {
    const auto map = notsa::make_mapping<int32, int32>({
        { 1,  3 },
        { 2,  5 },
        { 3,  32},
        { 4,  22},
        { 5,  34},
        { 6,  24},
        { 7,  41},
        { 8,  51},
        { 9,  43},
        { 10, 53},
        { 11, 52},
        { 12, 42},
        { 13, 33},
        { 14, 23},
        { 15, 31},
        { 16, 21},
        { 17, 4 },
        { 18, 8 },
    });
    return notsa::find_value_or(map, simpleId, -1);
}

// 0x509A30
bool IsLampPost(eModelID modelId) {
    using namespace ModelIndices;

    return notsa::contains<eModelID>(
        {
            MI_SINGLESTREETLIGHTS1,
            MI_SINGLESTREETLIGHTS2,
            MI_SINGLESTREETLIGHTS3,
            MI_BOLLARDLIGHT,
            MI_MLAMPPOST,
            MI_STREETLAMP1,
            MI_STREETLAMP2,
            MI_TELPOLE02,
            MI_TRAFFICLIGHTS_MIAMI,
            MI_TRAFFICLIGHTS_TWOVERTICAL,
            MI_TRAFFICLIGHTS_3,
            MI_TRAFFICLIGHTS_4,
            MI_TRAFFICLIGHTS_GAY,
            MI_TRAFFICLIGHTS_5,
        },
        modelId
    );
}
