#include "StdInc.h"

#include "Shadows.h"

void CBmx::InjectHooks() {
    RH_ScopedVirtualClass(CBmx, 0x871528, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF820);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6BF9B0);
    RH_ScopedVMTInstall(BurstTyre, 0x6BF9C0);
    RH_ScopedVMTInstall(FindWheelWidth, 0x6C0550);
    RH_ScopedVMTInstall(ProcessControl, 0x6BFA30);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BFB50);
    RH_ScopedVMTInstall(PreRender, 0x6C0810);
    RH_ScopedVMTInstall(ProcessAI, 0x6C1470, {.reversed = false});
    RH_ScopedInstall(ProcessBunnyHop, 0x6C0590);
    RH_ScopedInstall(LaunchBunnyHopCB, 0x6C0390);
}

// 0x6BF820
CBmx::CBmx(int32 modelIndex, eVehicleCreatedBy createdBy) :
    CBike(modelIndex, createdBy) 
{
    auto mi                     = CModelInfo::GetModelInfo(modelIndex);
    m_nVehicleSubType           = VEHICLE_TYPE_BMX;
    m_RideAnimData.AnimGroup = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
    if (m_RideAnimData.AnimGroup < ANIM_GROUP_BMX || m_RideAnimData.AnimGroup > ANIM_GROUP_CHOPPA) {
        m_RideAnimData.AnimGroup = ANIM_GROUP_BMX;
    }

    m_fControlJump     = 0.0f;
    m_fControlPedaling = 0.0f;
    m_fSprintLeanAngle = 0.0f;
    m_fCrankAngle      = 0.0f;
    m_fPedalAngleL     = 0.0f;
    m_fPedalAngleR     = 0.0f;
    m_nFixLeftHand     = false;
    m_nFixRightHand    = false;
    m_bIsFreewheeling  = false;

    const auto Calc = [&](eBmxNodes node) -> float {
        RwMatrix matrix;
        RwFrame* wheelFront = m_aBikeNodes[node];
        matrix              = *RwFrameGetMatrix(wheelFront);

        auto parent = RwFrameGetParent(wheelFront);
        if (parent) {
            do {
                RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
                parent = RwFrameGetParent(parent);
            } while (parent != wheelFront && parent);
        }
        return matrix.pos.y;
    };
    auto wheelFrontPosY = Calc(BMX_WHEEL_FRONT);
    auto wheelRearPosY  = Calc(BMX_WHEEL_REAR);

    m_fMidWheelDistY = wheelFrontPosY - wheelRearPosY;
    m_fMidWheelFracY = wheelFrontPosY / m_fMidWheelDistY;
}

// 0x6BF9D0
CBmx::~CBmx() {
    m_vehicleAudio.Terminate();
}

// 0x6BF9B0
bool CBmx::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6BF9C0
bool CBmx::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6BFA30
void CBmx::ProcessControl() {
    const float BMX_SPRINT_LEANSTART = FRAC_PI_2;
    const float BMX_PEDAL_LEANSTART  = 0.0f;
    const float BMX_SPRINT_LEANMULT  = 0.3f;
    const float MTB_SPRINT_LEANMULT  = 0.087f;
    const float BMX_PEDAL_LEANMULT   = 0.07f;
    const float MTB_PEDAL_LEANMULT   = 0.02f;

    CBike::ProcessControl();

    if (GetWasPostponed() || GetStatus() != STATUS_PLAYER || !m_pDriver) {
        return;
    }

    auto animBikeSprint = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
    bool isMountainBike = GetModelId() == MODEL_MTBIKE;

    if (animBikeSprint && animBikeSprint->GetBlendAmount() > 0.01f) {
        float mult         = isMountainBike ? MTB_SPRINT_LEANMULT : BMX_SPRINT_LEANMULT;
        m_fSprintLeanAngle = std::sin(animBikeSprint->GetCurrentTime() / animBikeSprint->GetHier()->GetTotalTime() * TWO_PI + BMX_SPRINT_LEANSTART) * animBikeSprint->GetBlendAmount() * mult;
    } else {
        auto animBikePedal = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
        if (animBikePedal && animBikePedal->GetBlendAmount() > 0.01f) {
            float mult = isMountainBike ? MTB_PEDAL_LEANMULT : BMX_PEDAL_LEANMULT;
            GetRideAnimData()->LeanAngle += std::sin(animBikePedal->GetCurrentTime() / animBikePedal->GetHier()->GetTotalTime() * TWO_PI + BMX_PEDAL_LEANSTART) * animBikePedal->GetBlendAmount() * mult;
        }
        m_fSprintLeanAngle *= 0.95f;
    }
}

// 0x6BFB50
void CBmx::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && (!driver || !driver->IsPlayer())) {
        return;
    }

    m_nFixLeftHand  = true;
    m_nFixRightHand = true;

    if (auto* animBack = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_BACK)) {
        const auto blendAmt = animBack->GetBlendAmount();
        m_fCrankAngle = FRAC_PI_2 * blendAmt + (1.0f - blendAmt) * m_fCrankAngle;
    } else {
        if (driver->GetPlayerData()) {
            driver->SetMoveState(PEDMOVE_NONE);
        }

        const auto fwdSpeed = DotProduct(m_vecMoveSpeed, GetForward());

        auto* animPedal   = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
        auto* animSprint  = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
        auto* animLeft    = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_LEFT);
        auto* animRight   = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_RIGHT);
        auto* animFwd     = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_FWD);
        auto* animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYLHS);
        if (!animDriveBy) {
            animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYRHS);
            if (!animDriveBy) {
                animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYFT);
            }
        }

        // Higher lean threshold while actively pedaling.
        const auto leanThreshold = m_fControlPedaling > 5.0f ? 0.7f : 0.4f;

        if (leanThreshold <= std::fabs(m_RideAnimData.LeanAngle) || leanThreshold <= m_RideAnimData.LeanFwd || fwdSpeed <= 0.01f || animDriveBy) {
            const auto isRampingIn = [](CAnimBlendAssociation* a) { return a && a->GetBlendDelta() >= 0.0f && a->GetBlendAmount() > 0.0f; };

            if (isRampingIn(animPedal) || isRampingIn(animSprint)) {
                if (animPedal) {
                    animPedal->SetFlag(ANIMATION_IS_PLAYING, false);
                    animPedal->SetBlendDelta(-8.0f);
                }
                if (animSprint) {
                    animSprint->SetFlag(ANIMATION_IS_PLAYING, false);
                    animSprint->SetBlendDelta(-8.0f);
                }
                // FIX_BUGS candidate: unlike CBike::ProcessRiderAnims's equivalent blend (which uses
                // pow(base, CTimer::GetTimeStep())), this decay is a flat per-frame multiply with no
                // timestep scaling - at very high framerates it collapses to ~0 almost instantly
                // instead of decaying smoothly, which can look like a sudden lean snap.
                m_RideAnimData.AnimLeanLeft *= 0.95f;
                m_RideAnimData.AnimLeanFwd  *= 0.95f;
            } else {
                CBike::ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
            }

            auto* pedalOrSprint = animPedal ? animPedal : animSprint;
            m_fCrankAngle = pedalOrSprint
                ? 0.0f - (pedalOrSprint->GetCurrentTime() / pedalOrSprint->GetHier()->GetTotalTime()) * TWO_PI
                : 0.0f;

            if (animLeft && animLeft->GetBlendAmount() > 0.1f) {
                m_bIsFreewheeling = true;
                m_fCrankAngle = PI * animLeft->GetBlendAmount() + (1.0f - animLeft->GetBlendAmount()) * m_fCrankAngle;
            } else if (animRight && animRight->GetBlendAmount() > 0.1f) {
                m_bIsFreewheeling = true;
                m_fCrankAngle = (1.0f - animRight->GetBlendAmount()) * m_fCrankAngle;
            } else if (animFwd && animFwd->GetBlendAmount() > 0.1f) {
                m_fCrankAngle = FRAC_PI_2 * animFwd->GetBlendAmount() + (1.0f - animFwd->GetBlendAmount()) * m_fCrankAngle;
            } else {
                m_fCrankAngle = std::pow(0.97f, CTimer::GetTimeStep()) * m_fCrankAngle;
            }

            if (animDriveBy) {
                m_nFixRightHand   = false;
                m_bIsFreewheeling = true;
            }
        } else {
            // Not leaning/turning/stopped/drive-by: drive the crank from wheel speed & gear instead of anim time.
            float crankTarget;
            float crankRate;
            if (GetModelId() == MODEL_MTBIKE) {
                crankRate = 2.0f;
                crankTarget = m_nCurrentGear == 0
                    ? 0.0f
                    : (5.0f * fwdSpeed) / (m_nCurrentGear * m_pHandlingData->GetTransmission().m_MaxFlatVelocity - 0.25f);
            } else {
                crankTarget = 3.0f * fwdSpeed;
                crankRate   = 2.5f;
            }

            const auto refreshAssoc = [&](CAnimBlendAssociation*& assoc, AnimationId id) {
                if (!assoc || (assoc->GetBlendAmount() < 1.0f && assoc->GetBlendDelta() != 0.0f)) {
                    assoc = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, id, 4.0f);
                    return true;
                }
                return false;
            };

            CAnimBlendAssociation* activeAssoc;
            bool freshAssoc;
            if (m_fControlPedaling <= 5.0f || crankRate <= crankTarget) {
                freshAssoc = refreshAssoc(animPedal, ANIM_ID_BIKE_PEDAL);
                if (m_GasPedal != 0.0f || m_fControlPedaling > 0.0f || GetStatus() == STATUS_SIMPLE) {
                    animPedal->SetFlag(ANIMATION_IS_PLAYING, true);
                } else {
                    animPedal->SetFlag(ANIMATION_IS_PLAYING, false);
                    if (!vehicleFlags.bIsHandbrakeOn && (m_aRatioHistory[0] < 1.0f || m_aRatioHistory[1] < 1.0f || m_aRatioHistory[2] < 1.0f || m_aRatioHistory[3] < 1.0f)) {
                        m_bIsFreewheeling = true;
                    }
                }
                activeAssoc = animPedal;
            } else {
                freshAssoc = refreshAssoc(animSprint, ANIM_ID_BIKE_SPRINT);
                animSprint->SetFlag(ANIMATION_IS_PLAYING, true);
                if (animPedal) {
                    animPedal->SetFlag(ANIMATION_IS_PLAYING, true);
                    activeAssoc = animPedal;
                } else {
                    activeAssoc = animSprint;
                }
                activeAssoc->SetSpeed(crankTarget); // NOTSA: original writes m_Speed directly at the same offset
            }

            if (!activeAssoc) {
                m_fCrankAngle = std::pow(0.97f, CTimer::GetTimeStep()) * m_fCrankAngle;
            } else {
                // If a directional (left/right/fwd) anim is significantly blended in, dock the crank
                // phase toward that direction's fixed angle instead of following anim time directly.
                auto targetAngle = fwdSpeed;
                auto hasDirectional = false;
                if (animLeft && animLeft->GetBlendAmount() > 0.5f) {
                    targetAngle = PI;
                    hasDirectional = true;
                }
                if (animRight && animRight->GetBlendAmount() > 0.5f) {
                    targetAngle = 0.0f;
                    hasDirectional = true;
                }
                if (animFwd && animFwd->GetBlendAmount() > 0.5f) {
                    targetAngle = FRAC_PI_2;
                    hasDirectional = true;
                }

                if (!freshAssoc || !hasDirectional || targetAngle <= -1000.0f) {
                    m_fCrankAngle = 0.0f - (activeAssoc->GetCurrentTime() / activeAssoc->GetHier()->GetTotalTime()) * TWO_PI;
                } else {
                    auto crankPhase = (0.0f - targetAngle) * (1.0f / TWO_PI);
                    if (crankPhase < 0.0f) {
                        crankPhase += 1.0f;
                    }
                    activeAssoc->SetCurrentTime(activeAssoc->GetHier()->GetTotalTime() * crankPhase);
                    m_fCrankAngle = crankPhase;
                }
            }
        }

        if (std::fabs(m_RideAnimData.AnimLeanLeft) > 0.05f || std::fabs(m_RideAnimData.AnimLeanFwd) > 0.05f) {
            // FIX_BUGS candidate: same unscaled per-frame decay as above.
            m_RideAnimData.AnimLeanLeft *= 0.95f;
            m_RideAnimData.AnimLeanFwd  *= 0.95f;
        }
    }

    // NOTSA: front/rear tunnel-transition bookkeeping (mirrors CAutomobile::PlaceOnRoadProperly's
    // m_bTunnel/m_bTunnelTransition pattern) was not fully re-derived this session - see progress
    // notes (Z:\ghidra_project\cbike_cbmx_progress.md) for the raw addresses if resuming this.

    m_fPedalAngleL = -m_fCrankAngle;
    m_fPedalAngleR = -m_fCrankAngle;
}

// data is a ptr to CBmx
// 0x6C0390
void CBmx::LaunchBunnyHopCB(CAnimBlendAssociation* assoc, void* data) {
    auto bmx = static_cast<CBmx*>(data);
    if ((bmx->m_WheelCounts[0] > 0.0f || bmx->m_WheelCounts[1] > 0.0f) &&
        (bmx->m_WheelCounts[2] > 0.0f || bmx->m_WheelCounts[3] > 0.0f)
    ) {
        auto power = std::min(bmx->m_fControlJump / 25.0f, 1.0f) + 1.0f;
        if (bmx->GetStatus() == STATUS_PLAYER) {
            power *= CStats::GetFatAndMuscleModifier(STAT_MOD_6);
        }
        if (CCheat::IsActive(CHEAT_HUGE_BUNNY_HOP)) {
            power *= 5.0f;
        }
        bmx->ApplyMoveForce(0.06f * bmx->m_fMass * power * bmx->m_matrix->GetUp());
        bmx->ApplyTurnForce(0.01f * bmx->m_fTurnMass * power * bmx->m_matrix->GetUp(), bmx->m_matrix->GetForward());
    }
}

// 0x6C0500 | inlined | see 0x6C11F3
void CBmx::GetFrameOffset(float& fZOffset, float& fAngleOffset) {
    const auto d1 = m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0];
    const auto d2 = m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1];

    fZOffset     = (1.0f - m_fMidWheelFracY) * d1 + d2 * m_fMidWheelFracY;
    fAngleOffset = std::atan2(d1 - d2, m_fMidWheelDistY);
}

// 0x6C0550
float CBmx::FindWheelWidth(bool bRear) {
    return 0.07f;
}

// 0x6C0560
void CBmx::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    // NOP
}

// 0x6C0590
void CBmx::ProcessBunnyHop() {
    auto* anim = m_pDriver
        ? RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_BUNNYHOP)
        : nullptr;

    if (GetStatus() != STATUS_PLAYER || !m_pDriver || !m_pDriver->IsPlayer()) {
        if (anim) {
            anim->SetFlag(ANIMATION_IS_PLAYING, true);
            anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE, true);
            anim->SetBlendDelta(-8.0f);
        }
        return;
    }

    auto pad = m_pDriver->AsPlayer()->GetPadFromPlayer();

    if (pad->IsLeftShoulder1Pressed() && !pad->DisablePlayerControls && m_fControlJump == 0.0f) {
        m_fControlJump += CTimer::GetTimeStep();
        anim = CAnimManager::BlendAnimation(m_pDriver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_BUNNYHOP, 8.0f);
        if (anim) {
            anim->SetCurrentTime(0.0f);
            anim->SetFlag(ANIMATION_IS_PLAYING, false);
        }
    }

    if (m_fControlJump > 0.0f) {
        if (!pad->DisablePlayerControls) {
            if (!anim) {
                m_fControlJump = 0.0f;
                return;
            }

            if (pad->IsLeftShoulder1()) {
                if (!anim->IsPlaying()) {
                    m_fControlJump = std::min(m_fControlJump + CTimer::GetTimeStep(), 25.0f);
                    anim->SetCurrentTime(m_fControlJump / 25.0f * 0.2f);
                }
            } else if (!anim->IsPlaying()) {
                if (anim->GetCurrentTime() < 0.2f) {
                    anim->SetCurrentTime((0.2f - anim->GetCurrentTime()) / 0.2f * (anim->GetHier()->GetTotalTime() - 0.2f) + 0.2f);
                }
                anim->SetFlag(ANIMATION_IS_PLAYING, true);
                anim->SetSpeed(1.5f);
                anim->SetFinishCallback(CBmx::LaunchBunnyHopCB, this);
            }
        } else {
            m_fControlJump = 0.0f;
        }
    }

    if (anim) {
        if (anim->GetBlendAmount() > 0.5f) {
            m_GasPedal                                   = 0.0f;
            FindPlayerPed()->GetPlayerData()->m_fMoveSpeed = 0.0f;
            if (!vehicleFlags.bIsHandbrakeOn && (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f || m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
                m_bIsFreewheeling = true;
            }
        }
    }
}

// 0x6C0810
void CBmx::PreRender() {
    CVehicle::PreRender();

    auto* const colModel = GetColModel();
    auto* const colData  = colModel->m_pColData;

    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        // Smooth wheel suspension heights toward the ground-contact collision lines'
        // Z position right after collision was (re)processed this frame, to avoid visual popping.
        {
            const auto ratioAtLimit = 1.0f - m_fSuspensionLength[0] / m_fLineLength[0];
            auto       ratio        = std::min(m_aWheelRatios[0], m_aWheelRatios[1]);
            ratio = (ratio - ratioAtLimit) / (1.0f - ratioAtLimit);
            auto targetZ = colData->m_pLines[0].m_vecStart.z;
            if (ratio > 0.0f) {
                targetZ -= ratio * m_fSuspensionLength[0];
            }
            m_aWheelSuspensionHeights[0] += (targetZ - m_aWheelSuspensionHeights[0]) * 0.75f;
        }
        {
            const auto ratioAtLimit = 1.0f - m_fSuspensionLength[2] / m_fLineLength[2];
            auto       ratio        = std::min(m_aWheelRatios[2], m_aWheelRatios[3]);
            ratio = (ratio - ratioAtLimit) / (1.0f - ratioAtLimit);
            auto targetZ = colData->m_pLines[2].m_vecStart.z;
            if (ratio > 0.0f) {
                targetZ -= ratio * m_fSuspensionLength[2];
            }
            m_aWheelSuspensionHeights[1] += (targetZ - m_aWheelSuspensionHeights[1]) * 0.75f;
        }
    }

    // Wheel dust/spark effect spawn, front (0) then rear (1) wheel.
    if (GetStatus() <= STATUS_PHYSICS) {
        const auto speed = m_vecMoveSpeed.Magnitude();
        for (auto wheelIdx = 0; wheelIdx < 2; wheelIdx++) {
            int32 selIdx;
            if (wheelIdx == 0) {
                selIdx = (m_aRatioHistory[0] >= 1.0f && m_aRatioHistory[1] < 1.0f) ? 1 : 0;
            } else {
                selIdx = (m_aRatioHistory[3] >= 1.0f && m_aRatioHistory[2] < 1.0f) ? 2 : 3;
            }

            int32 flags = (wheelIdx == 0 || m_WheelStates[1] == WHEEL_STATE_FIXED) ? 4 : 0;

            const auto  dustHeight = std::sin(m_RideAnimData.LeanAngle) * colModel->m_boundBox.m_vecMin.z * 0.8f;
            auto&       wheelPos   = m_aWheelColPoints[selIdx].m_vecPoint;
            const auto& right      = GetRight();

            CVector effectOffset{
                wheelPos.x + dustHeight * right.x,
                wheelPos.y + dustHeight * right.y,
                wheelPos.z + dustHeight * right.z,
            };

            if (m_bWheelBloody[wheelIdx])   { flags += 1; }
            if (m_bMoreSkidMarks[wheelIdx]) { flags += 2; }

            const auto leanSign = m_RideAnimData.LeanAngle <= 0.0f ? 1.0f : -1.0f;

            // NOTSA: forwards to the original (still-unreversed internally) wheel-effects dispatcher
            // with its real 11-dword argument list (confirmed via a fresh decompile of this call site).
            plugin::Call<0x405876, tWheelState, uint8, float, float, CVector*, CVector*, float, int32, eSkidmarkType, bool*, int32>(
                m_WheelStates[wheelIdx],
                m_nWheelStatus[wheelIdx],
                m_aRatioHistory[selIdx],
                speed,
                &wheelPos,
                &effectOffset,
                leanSign,
                wheelIdx,
                m_aWheelSkidmarkType[wheelIdx],
                &m_bWheelBloody[wheelIdx],
                flags
            );
        }
    }

    // Wheel angular velocity / pitch-angle accumulation (drives the wheel-spin texture animation).
    // NOTSA: `rollDir` is approximated as the vehicle's forward vector - the original also factors in
    // `m_fSteerAngle` here (via `cos(m_fSteerAngle)` transformed by the vehicle matrix), which wasn't
    // independently re-verified byte-for-byte; the effect is purely cosmetic (wheel-spin visuals only).
    const auto rollDir = GetForward();
    if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) {
        const auto ratio      = std::min(m_aRatioHistory[0], m_aRatioHistory[1]);
        const auto contactPos = colData->m_pLines[0].m_vecStart - CVector{0.0f, ratio * m_fSuspensionLength[0], 0.0f};
        const auto pointVel   = GetSpeed(contactPos);
        const auto rot        = ProcessWheelRotation(WHEEL_STATE_NORMAL, rollDir, pointVel, GetVehicleModelInfo()->m_fWheelSizeRear * 0.5f);
        m_aWheelAngularVelocity[0] = rot;
        m_aWheelPitchAngles[0] += rot * CTimer::ms_fTimeStep;
    }
    if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) {
        const auto ratio      = std::min(m_aRatioHistory[2], m_aRatioHistory[3]);
        const auto contactPos = colData->m_pLines[2].m_vecStart - CVector{0.0f, ratio * m_fSuspensionLength[2], 0.0f};
        const auto pointVel   = GetSpeed(contactPos);
        const auto rot        = ProcessWheelRotation(m_WheelStates[1], rollDir, pointVel, GetVehicleModelInfo()->m_fWheelSizeFront * 0.5f);
        m_aWheelAngularVelocity[1] = rot;
        m_aWheelPitchAngles[1] += rot * CTimer::ms_fTimeStep;
    }

    m_bLeanMatrixCalculated = false;

    // NOTSA: forwards to the original (still-unreversed internally) crank/fork one-time matrix setup helper.
    plugin::CallMethod<0x4019FC, CBmx*>(this);

    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    // Per-node matrix updates below all follow the same idiom already established in
    // CQuadBike::PreRender: `SetRotate`/`SetTranslate` (but NOT the `...Only` variants) reset the
    // matrix's position as a side effect, so the original code saves the position first and
    // restores it (or a freshly-computed replacement) afterwards.

    // Steering-angle quaternion, reused for both the forks and (when abandoned/wrecked) the handlebars.
    CQuaternion steerQuat;
    if (m_aBikeNodes[BMX_FORKS_FRONT]) {
        const auto steerAngleConst = GetVehicleModelInfo()->m_fBikeSteerAngle * DegreesToRadians(1.0f);
        const auto steerAxis       = CVector{0.0f, std::sin(steerAngleConst), -std::cos(steerAngleConst)}.Normalized();
        RwV3d      axis{steerAxis.x, steerAxis.y, steerAxis.z};
        steerQuat.Set(&axis, -m_RideAnimData.BarSteerAngle);
    }

    if (auto* const forksFrontFrame = m_aBikeNodes[BMX_FORKS_FRONT]) {
        CMatrix forksMatrix;
        forksMatrix.Attach(RwFrameGetMatrix(forksFrontFrame), false);
        const auto savedPos = forksMatrix.GetPosition();
        forksMatrix.SetRotate(steerQuat);
        forksMatrix.GetPosition() = savedPos;
        forksMatrix.UpdateRW();

        if (auto* const handlebarsFrame = m_aBikeNodes[BMX_HANDLEBARS]) {
            CMatrix handlebarsMatrix;
            handlebarsMatrix.Attach(RwFrameGetMatrix(handlebarsFrame), false);
            const auto handlebarsSavedPos = handlebarsMatrix.GetPosition();

            const auto status = GetStatus();
            if (status == STATUS_ABANDONED || status == STATUS_WRECKED) {
                handlebarsMatrix.SetRotate(steerQuat);
                handlebarsMatrix.GetPosition() = handlebarsSavedPos;
            } else {
                handlebarsMatrix.SetTranslate(handlebarsSavedPos);
            }
            handlebarsMatrix.UpdateRW();
        }
    }

    if (auto* const forksRearFrame = m_aBikeNodes[BMX_FORKS_REAR]) {
        const auto rearRatio = (m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength;

        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(forksRearFrame), false);
        const auto savedPos = nodeMatrix.GetPosition();
        nodeMatrix.SetRotate(-std::asin(rearRatio), 0.0f, 0.0f);
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }

    // NOTSA: `g_bmxUnknownFlag` gates an alternate front/rear wheel position override whose purpose
    // wasn't identified this session (no cross-referencing usage found elsewhere in the codebase).
    static auto& g_bmxUnknownFlag = StaticRef<bool>(0xC1C83C);

    {
        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_FRONT]), false);
        auto savedPos = nodeMatrix.GetPosition();
        if (g_bmxUnknownFlag) {
            savedPos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
            savedPos.y = ((colData->m_pLines[1].m_vecStart.y + colData->m_pLines[0].m_vecStart.y) * 0.5f - m_fForkYOffset)
                       - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;
        }
        if (m_nWheelStatus[0] == 1) {
            nodeMatrix.SetRotate(m_aWheelPitchAngles[0], 0.0f, std::sin(m_aWheelPitchAngles[0]) * 0.02f);
        } else {
            nodeMatrix.SetRotateX(m_aWheelPitchAngles[0]);
        }
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }

    {
        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_REAR]), false);
        auto savedPos = nodeMatrix.GetPosition();
        if (g_bmxUnknownFlag && !m_aBikeNodes[BMX_FORKS_REAR]) {
            savedPos.z = m_aWheelSuspensionHeights[1];
        }
        if (m_nWheelStatus[1] == 1) {
            nodeMatrix.SetRotate(m_aWheelPitchAngles[1], 0.0f, std::sin(m_aWheelPitchAngles[1]) * 0.04f);
        } else {
            nodeMatrix.SetRotateX(m_aWheelPitchAngles[1]);
        }
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }

    if (auto* const chassisFrame = m_aBikeNodes[BMX_CHASSIS]) {
        auto leanBlendZ      = 0.0f;
        auto leanAngleOffset = 0.0f;
        if (!g_bmxUnknownFlag) {
            leanBlendZ = (m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) * m_fMidWheelFracY
                       + (1.0f - m_fMidWheelFracY) * (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]);
            leanAngleOffset = std::atan2(
                (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) - (m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]),
                m_fMidWheelDistY
            );
        }

        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(chassisFrame), false);
        const auto savedPos = nodeMatrix.GetPosition();
        const auto newZ     = (1.0f - std::cos(m_RideAnimData.LeanAngle)) * colModel->m_boundBox.m_vecMin.z * 0.9f + leanBlendZ;

        nodeMatrix.SetRotateX(std::abs(m_RideAnimData.LeanAngle) * -0.05f + leanAngleOffset);
        // NOTSA: RotateY can't be hooked directly (its real signature has no NOTSA `bKeepPos` param), forward raw.
        ((void(__thiscall*)(CMatrix*, float))0x59B2C0)(&nodeMatrix, m_fSprintLeanAngle + m_RideAnimData.LeanAngle);
        nodeMatrix.GetPosition() = CVector{savedPos.x, savedPos.y, newZ};
        nodeMatrix.UpdateRW();
    }

    if (auto* const chainsetFrame = m_aBikeNodes[BMX_CHAINSET]) {
        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(chainsetFrame), false);
        const auto savedPos = nodeMatrix.GetPosition();
        nodeMatrix.SetRotate(m_fCrankAngle, 0.0f, 0.0f);
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }

    // NOTSA: matches the original exactly - the PEDAL_R node is rotated using `m_fPedalAngleL` and
    // PEDAL_L using `m_fPedalAngleR` (cross-referenced), not a transcription mistake.
    if (auto* const pedalRFrame = m_aBikeNodes[BMX_PEDAL_R]) {
        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(pedalRFrame), false);
        const auto savedPos = nodeMatrix.GetPosition();
        nodeMatrix.SetRotate(m_fPedalAngleL, 0.0f, 0.0f);
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }
    if (auto* const pedalLFrame = m_aBikeNodes[BMX_PEDAL_L]) {
        CMatrix nodeMatrix;
        nodeMatrix.Attach(RwFrameGetMatrix(pedalLFrame), false);
        const auto savedPos = nodeMatrix.GetPosition();
        nodeMatrix.SetRotate(m_fPedalAngleR, 0.0f, 0.0f);
        nodeMatrix.GetPosition() = savedPos;
        nodeMatrix.UpdateRW();
    }
}

// 0x6C1470
bool CBmx::ProcessAI(uint32& extraHandlingFlags) {
    return plugin::CallMethodAndReturn<bool, 0x6C1470, CBmx*, uint32&>(this, extraHandlingFlags);
}
