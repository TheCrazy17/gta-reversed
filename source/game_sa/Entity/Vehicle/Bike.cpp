/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Bike.h"

#include "CarCtrl.h"

#include "Buoyancy.h"
#include "Collision/CollisionData.h"
#include "Enums/eSurfaceType.h"
#include "Enums/eWantedLevel.h"
#include "VehicleRecording.h"
#include "Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include "Animation/AnimBlendAssociation.h"



void CBike::InjectHooks() {
    RH_ScopedVirtualClass(CBike, 0x871360, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF430);
    RH_ScopedInstall(Destructor, 0x6B57A0);
    RH_ScopedInstall(dmgDrawCarCollidingParticles, 0x6B5A00);
    RH_ScopedInstall(DamageKnockOffRider, 0x6B5A10);
    RH_ScopedInstall(KnockOffRider, 0x6B5F40);
    RH_ScopedInstall(SetRemoveAnimFlags, 0x6B5F50);
    RH_ScopedInstall(ReduceHornCounter, 0x6B5F90);
    RH_ScopedInstall(ProcessAI, 0x6BC930, { .reversed = false });
    RH_ScopedInstall(ProcessBuoyancy, 0x6B5FB0);
    RH_ScopedInstall(ResetSuspension, 0x6B6740);
    RH_ScopedInstall(GetAllWheelsOffGround, 0x6B6790);
    RH_ScopedInstall(DebugCode, 0x6B67A0);
    RH_ScopedInstall(DoSoftGroundResistance, 0x6B6D40);
    RH_ScopedInstall(PlayHornIfNecessary, 0x6B7130);
    RH_ScopedInstall(CalculateLeanMatrix, 0x6B7150);
    RH_ScopedInstall(ProcessRiderAnims, 0x6B7280, { .reversed = false });
    RH_ScopedInstall(FixHandsToBars, 0x6B7F90, { .reversed = false });
    RH_ScopedInstall(PlaceOnRoadProperly, 0x6BEEB0);
    RH_ScopedInstall(GetCorrectedWorldDoorPosition, 0x6BF230);
    RH_ScopedVMTInstall(Fix, 0x6B7050);
    RH_ScopedVMTInstall(BlowUpCar, 0x6BEA10, { .reversed = false });
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BF400);
    RH_ScopedVMTInstall(BurstTyre, 0x6BEB20);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6BE310);
    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6BDEA0);
    RH_ScopedVMTInstall(Render, 0x6BDE20);
    RH_ScopedVMTInstall(PreRender, 0x6BD090, { .reversed = false });
    RH_ScopedVMTInstall(Teleport, 0x6BCFC0);
    RH_ScopedVMTInstall(ProcessControl, 0x6B9250, { .reversed = false });
    RH_ScopedVMTInstall(VehicleDamage, 0x6B8EC0);
    RH_ScopedVMTInstall(SetupSuspensionLines, 0x6B89B0, { .reversed = false });
    RH_ScopedVMTInstall(SetModelIndex, 0x6B8970);
    RH_ScopedVMTInstall(PlayCarHorn, 0x6B7080);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6B7070);
    RH_ScopedVMTInstall(DoBurstAndSoftGroundRatios, 0x6B6950, { .reversed = false });
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6B67E0);
    RH_ScopedVMTInstall(RemoveRefsToVehicle, 0x6B67B0);
    RH_ScopedVMTInstall(ProcessControlCollisionCheck, 0x6B6620);
    RH_ScopedVMTInstall(GetComponentWorldPosition, 0x6B5990);
    RH_ScopedVMTInstall(ProcessOpenDoor, 0x6B58D0);
}

// 0x6BF430
CBike::CBike(int32 modelIndex, eVehicleCreatedBy createdBy) : CVehicle(createdBy) {
    auto mi = CModelInfo::GetModelInfo(modelIndex)->AsVehicleModelInfoPtr();
    if (mi->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        const auto& animationStyle = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
        m_RideAnimData.AnimGroup = animationStyle;
        if (animationStyle < ANIM_GROUP_BIKES || animationStyle > ANIM_GROUP_WAYFARER) {
            m_RideAnimData.AnimGroup = ANIM_GROUP_BIKES;
        }
    }

    m_nVehicleSubType = VEHICLE_TYPE_BIKE;
    m_nVehicleType = VEHICLE_TYPE_BIKE;

    m_BlowUpTimer = 0.0f;
    m_nBrakesOn = false;
    nBikeFlags = 0;
    SetModelIndex(modelIndex);

    m_pHandlingData = gHandlingDataMgr.GetVehiclePointer(mi->m_nHandlingId);
    m_BikeHandling = gHandlingDataMgr.GetBikeHandlingPointer(mi->m_nHandlingId);
    m_nHandlingFlagsIntValue = m_pHandlingData->m_nHandlingFlags;
    m_pFlyingHandlingData = gHandlingDataMgr.GetFlyingPointer(static_cast<uint8>(mi->m_nHandlingId));
    m_fBrakeCount = 20.0f;
    mi->ChooseVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor, 1);
    m_fSwingArmLength = 0.0f;
    m_fForkYOffset = 0.0f;
    m_fForkZOffset = 0.0f;
    m_nFixLeftHand = false;
    m_nFixRightHand = false;
    m_fSteerAngleTan = std::tan(DegreesToRadians(mi->m_fBikeSteerAngle));
    m_fMass = m_pHandlingData->m_fMass;
    m_fTurnMass = m_pHandlingData->m_fTurnMass;
    m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
    m_vecCentreOfMass.z = 0.1f;
    m_fAirResistance = GetDefaultAirResistance();
    m_fElasticity = 0.05f;
    m_fBuoyancyConstant = m_pHandlingData->m_fBuoyancyConstant;
    m_fSteerAngle = 0.0f;
    m_GasPedal = 0.0f;
    m_BrakePedal = 0.0f;
    m_Damager = nullptr;
    m_pWhoInstalledBombOnMe = nullptr;
    m_GasPedalAudioRevs = 0.0f;
    m_fTyreTemp = 1.0f;
    m_fBrakingSlide = 0.0f;
    m_PrevSpeed = 0.0f;

    for (auto i = 0; i < 2; ++i) {
        m_nWheelStatus[i] = 0;
        m_aWheelSkidmarkType[i] = eSkidmarkType::DEFAULT;
        m_bWheelBloody[i] = false;
        m_bMoreSkidMarks[i] = false;
        m_aWheelPitchAngles[i] = 0.0f;
        m_aWheelAngularVelocity[i] = 0.0f;
        m_aWheelSuspensionHeights[i] = 0.0f;
        m_aWheelOrigHeights[i] = 0.0f;
        m_WheelStates[i] = WHEEL_STATE_NORMAL;
    }

    for (auto i = 0; i < 4; ++i) {
        m_aWheelColPoints[i] = {};
        m_aWheelRatios[i] = 1.0f;
        m_aRatioHistory[i] = 0.0f;
        m_WheelCounts[i] = 0.0f;
        m_fSuspensionLength[i] = 0.0f;
        m_fLineLength[i] = 0.0f;
        m_aGroundPhysicalPtrs[i] = nullptr;
        m_aGroundOffsets[i] = CVector{};
    }

    m_nNoOfContactWheels = 0;
    m_NumDriveWheelsOnGround = 0;
    m_NumDriveWheelsOnGroundLastFrame = 0;
    m_fHeightAboveRoad = 0.0f;
    m_fExtraTractionMult = 1.0f;

    if (!mi->m_pColModel->m_pColData->m_pLines) {
        mi->m_pColModel->m_pColData->m_nNumLines = 4;
        mi->m_pColModel->m_pColData->m_pLines = static_cast<CColLine*>(CMemoryMgr::Malloc(4 * sizeof(CColLine)));
        mi->m_pColModel->m_pColData->m_pLines[1].m_vecStart.x = 99'999.99f; // todo: explain this
    }
    mi->m_pColModel->m_pColData->m_pLines[0].m_vecStart.z = 99'999.99f;
    CBike::SetupSuspensionLines();

    m_autoPilot.m_nTempAction = TEMPACT_NONE;
    m_autoPilot.SetCarMission(MISSION_NONE, 0);
    m_autoPilot.carCtrlFlags.bAvoidLevelTransitions = false;

    SetStatus(STATUS_SIMPLE);
    m_nNumPassengers = 0;
    vehicleFlags.bLowVehicle = false;
    vehicleFlags.bIsBig = false;
    vehicleFlags.bIsVan = false;

    m_bLeanMatrixCalculated = false;
    m_mLeanMatrix = *m_matrix;
    m_vecOldSpeedForPlayback = CVector{};
    m_vehicleAudio.Initialise(this);
}

// 0x6B57A0
CBike::~CBike() {
    m_vehicleAudio.Terminate();
}

// 0x6B5A00
void CBike::dmgDrawCarCollidingParticles(const CVector& position, float power, eWeaponType weaponType) {
    // NOP
}

// 0x6B5A10
bool CBike::DamageKnockOffRider(CVehicle* vehicle, float damageIntensity, uint16 pieceType, CEntity* damager, const CVector& collisionPos, const CVector& collisionImpactVelocity) {
    const auto driver    = vehicle->m_pDriver;
    const auto passenger = vehicle->m_apPassengers[0];

    // Impact force relative to the bike's mass
    auto force = damageIntensity / vehicle->m_fMass * 800.0f;

    // A skilled rider resists being knocked off (unless flagged to always come off)
    if (vehicle->GetStatus() != STATUS_PLAYER) {
        if (driver && driver->CantBeKnockedOffBike != CANT_BE_KNOCKED_OFF_ALWAYS_NORMAL) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
        }
    } else {
        force *= 0.75f;
        if (driver) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.5f;
        }
    }

    // Only an actual driver gets knocked off
    if (!driver || !driver->IsStateDriving() || force <= 10.0f) {
        return false;
    }

    // A ped already reacting to a hit isn't also knocked off (cops are exempt)
    if (const auto task = driver->GetIntelligence()->GetTaskManager().GetActiveTask()) {
        if (task->GetTaskType() == TASK_SIMPLE_BE_HIT && !driver->IsCop()) {
            return false;
        }
    }

    const auto impactFwdMag   = vehicle->GetForward().Dot(collisionImpactVelocity);
    const auto impactUpMag    = vehicle->GetUp().Dot(collisionImpactVelocity);
    const auto impactRightMag = vehicle->GetRight().Dot(collisionImpactVelocity);

    // Per-axis weighting of the impact
    auto fwdWeight = 0.6f;
    if (std::abs(impactFwdMag) > 0.85f) {
        const auto vertical = collisionImpactVelocity.z < 0.85f ? 0.0f : collisionImpactVelocity.z;
        fwdWeight = 7.0f * sq(vertical) + 0.6f;
    }
    if (vehicle->GetUp().z < 0.0f) { // bike lying on its side / upside down
        fwdWeight = 5.0f;
    }

    auto backWeight = 1.5f;
    auto upWeight   = 0.05f;
    if (vehicle->m_nModelIndex == MODEL_SANCHEZ) {
        fwdWeight *= 0.65f;
        upWeight  *= 0.75f;
    } else if (vehicle->IsSubQuad()) {
        backWeight = 3.0f;
        fwdWeight *= 0.65f;
        upWeight  *= 0.75f;
    }

    if (impactFwdMag > 0.0f) {
        fwdWeight *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
    }

    force *= std::abs(impactFwdMag) * fwdWeight
           + std::max(impactUpMag, 0.0f) * upWeight
           + std::abs(impactRightMag) * 0.45f
           - std::min(impactUpMag, 0.0f) * backWeight;

    // Don't knock the player off while they're on stairs
    if (driver->IsPlayer() && CCullZones::CamStairsForPlayer() && CCullZones::FindZoneWithStairsAttributeForPlayer()) {
        force = 0.0f;
    }

    // ALWAYS_HARD peds come off at a much lower force threshold
    if (force <= (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_ALWAYS_HARD ? 20.0f : 75.0f)) {
        return false;
    }

    // NEVER peds are never knocked off
    if (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }
    if (passenger && passenger->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }

    // The driver (guaranteed present here) is thrown off, and so is the passenger, both reacting with the driver's facing
    const auto knockOffDir = (uint8)driver->GetLocalDirection(-CVector2D{ collisionImpactVelocity });

    driver->GetEventGroup().Add(CEventKnockOffBike{
        vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, true, false
    });
    if (passenger) {
        passenger->GetEventGroup().Add(CEventKnockOffBike{
            vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, false, false
        });
    }
    return true;
}

// dummy function
// 0x6B5F40
CPed* CBike::KnockOffRider(eWeaponType arg0, uint8 arg1, CPed* ped, bool arg3) {
    return ped;
}

// 0x6B5F50
void CBike::SetRemoveAnimFlags(CPed* ped) {
    if (!ped->GetIsTypePed()) {
        return;
    }

    for (auto* assoc = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_SECONDARY_TASK_ANIM); assoc; assoc = RpAnimBlendGetNextAssociation(assoc, ANIMATION_SECONDARY_TASK_ANIM)) {
        assoc->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
    }
}

// 0x6B5F90
void CBike::ReduceHornCounter() {
    if (m_HornCounter)
        m_HornCounter -= 1;
}

// 0x6B5FB0
void CBike::ProcessBuoyancy() {
    CVector vecBuoyancyTurnPoint;
    CVector vecBuoyancyForce;
    if (!mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancyConstant, &vecBuoyancyTurnPoint, &vecBuoyancyForce)) {
        vehicleFlags.bIsDrowning = false;
        physicalFlags.bSubmergedInWater = false;
        physicalFlags.bTouchingWater = false;
        return;
    }

    physicalFlags.bTouchingWater = true;
    ApplyMoveForce(vecBuoyancyForce);
    ApplyTurnForce(vecBuoyancyForce, vecBuoyancyTurnPoint);

    auto fTimeStep = std::max(0.01F, CTimer::GetTimeStep());
    auto fUsedMass = m_fMass / 125.0F;
    auto fBuoyancyForceZ = vecBuoyancyForce.z / (fTimeStep * fUsedMass);

    if (fUsedMass > m_fBuoyancyConstant)
        fBuoyancyForceZ *= 1.05F * fUsedMass / m_fBuoyancyConstant;

    if (physicalFlags.bMakeMassTwiceAsBig)
        fBuoyancyForceZ *= 1.5F;

    auto fBuoyancyForceMult = std::max(0.5F, 1.0F - fBuoyancyForceZ / 20.0F);
    auto fSpeedMult = std::pow(fBuoyancyForceMult, CTimer::GetTimeStep());
    m_vecMoveSpeed *= fSpeedMult;
    m_vecTurnSpeed *= fSpeedMult;

    // 0x6B6443
    if (fBuoyancyForceZ > 0.8F || (fBuoyancyForceZ > 0.4F && IsAnyWheelNotMakingContactWithGround())) {
        vehicleFlags.bIsDrowning = true;
        physicalFlags.bSubmergedInWater = true;

        m_vecMoveSpeed.z = std::max(-0.1F, m_vecMoveSpeed.z);

        if (m_pDriver) {
            ProcessPedInVehicleBuoyancy(m_pDriver->AsPed(), true);
        }
        else {
            vehicleFlags.bEngineOn = false;
        }

        for (const auto passenger : GetPassengers()) {
            ProcessPedInVehicleBuoyancy(passenger, false);
        }
    }
    else {
        vehicleFlags.bIsDrowning = false;
        physicalFlags.bSubmergedInWater = false;
    }
}

inline void CBike::ProcessPedInVehicleBuoyancy(CPed* ped, bool bIsDriver) {
    if (!ped)
        return;

    ped->physicalFlags.bTouchingWater = true;
    if (!ped->IsPlayer() && bikeFlags.bWaterTight)
        return;

    if (ped->IsPlayer())
        ped->AsPlayer()->HandlePlayerBreath(true, 1.0F);

    if (IsAnyWheelMakingContactWithGround()) {
        if (!ped->IsPlayer()) {
            auto pedDamageResponseCalc = CPedDamageResponseCalculator(this, CTimer::GetTimeStep(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, false);
            auto damageEvent = CEventDamage(this, CTimer::GetTimeInMS(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, 0, false, true);
            if (damageEvent.AffectsPed(ped))
                pedDamageResponseCalc.ComputeDamageResponse(ped, damageEvent.m_damageResponse, true);
            else
                damageEvent.m_damageResponse.m_bDamageCalculated = true;

            ped->GetEventGroup().Add(&damageEvent, false);
        }
    } else {
        auto knockOffBikeEvent = CEventKnockOffBike(this, m_vecMoveSpeed, m_vecLastCollisionImpactVelocity, m_fDamageIntensity, 0.0F, KNOCK_OFF_TYPE_FALL, 0, 0, nullptr, bIsDriver, false);
        ped->GetEventGroup().Add(&knockOffBikeEvent);
        if (bIsDriver) {
            vehicleFlags.bEngineOn = false;
        }
    }
}

// 0x6BC930
bool CBike::ProcessAI(uint32& extraHandlingFlags) {
    return plugin::CallMethodAndReturn<bool, 0x6BC930, CBike*, uint32&>(this, extraHandlingFlags);
}

// 0x6BF400
void CBike::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && GetStatus() == STATUS_PLAYER)
        return;

    ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
}

// 0x6B7280
void CBike::ProcessRiderAnims(CPed* rider, CVehicle* vehicle, CRideAnimData* rideData, tBikeHandlingData* handling, int16 a5) {
    plugin::Call<0x6B7280, CPed*, CVehicle*, CRideAnimData*, tBikeHandlingData*, int16>(rider, vehicle, rideData, handling, a5);
}

// 0x6BEB20
bool CBike::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    if (vehicleFlags.bTyresDontBurst || physicalFlags.bRenderScorched) {
        return false;
    }

    auto wheelIdx = tyreComponentId;
    if (tyreComponentId == 0xD) {
        wheelIdx = 0;
    } else if (tyreComponentId == 0xF) {
        wheelIdx = 1;
    }

    auto result = false;
    if (!m_nWheelStatus[wheelIdx]) {
        m_nWheelStatus[wheelIdx] = 1;
        m_vehicleAudio.AddAudioEvent(AE_TYRE_BURST, 0.0f);

        if (GetStatus() == STATUS_SIMPLE) {
            CCarCtrl::SwitchVehicleToRealPhysics(this);
        }

        if (bPhysicalEffect) {
            ApplyMoveForce(CGeneral::GetRandomNumberInRange(-0.02f, 0.02f) * m_fMass * GetRight());
            ApplyTurnForce(CGeneral::GetRandomNumberInRange(-0.02f, 0.02f) * m_fTurnMass * GetRight(), GetForward());
        }

        result = true;
    }

    if (!m_pDriver) {
        return result;
    }

    // NOTSA: the two branches below only ever run when `tyreComponentId` is literally 0xD/0xE -
    // the two real callers (front=0xD, rear=0xF wheel component IDs) get remapped to wheelIdx 0/1
    // above, and this second check re-tests the ORIGINAL (unmapped) tyreComponentId against 0xD/0xE
    // (confirmed via raw disassembly, not just the decompile - `CMP BL,0xD` / `CMP BL,0xE` against
    // the same register the remap above wrote 0/1 into), so for both real wheels this is
    // unreachable. Kept for fidelity to the original binary.
    auto shouldApplyForce = true;
    if (tyreComponentId == 0xD) {
        if (m_aRatioHistory[0] >= 1.0f) {
            shouldApplyForce = m_aRatioHistory[1] < 1.0f;
        }
    } else {
        if (tyreComponentId != 0xE) {
            return result;
        }
        if (m_aRatioHistory[2] >= 1.0f) {
            shouldApplyForce = m_aRatioHistory[3] < 1.0f;
        }
    }

    if (const auto speed = m_vecMoveSpeed.Magnitude(); shouldApplyForce && speed > 0.3f && (GetStatus() != STATUS_SIMPLE || speed > 0.55f)) {
        if (tyreComponentId == 0xD) {
            // NOTSA: unresolved - original constructs and adds some CEvent here (twice: once for
            // m_pDriver, once for m_apPassengers[0] if present), using m_vecMoveSpeed as a position
            // argument. Not translated - see progress notes.
        } else {
            ApplyTurnForce(0.04f * m_fTurnMass * GetRight(), GetForward());
        }
    }

    return result;
}

// 0x6BE310
void CBike::ProcessControlInputs(uint8 playerNum) {
    const auto fwdSpeed = DotProduct(m_vecMoveSpeed, GetForward());
    auto*      pad      = CPad::GetPad(playerNum);

    vehicleFlags.bIsHandbrakeOn = pad->GetExitVehicle() || pad->GetHandBrake() != 0;

    const auto ApplyPadSteer = [&] {
        m_nLastControlInput      = eControllerType::KEYBOARD;
        m_fRawSteerAngle        += (-(float)pad->GetSteeringLeftRight() / 128.0f - m_fRawSteerAngle) * CTimer::GetTimeStep() / 5.0f;
        m_RideAnimData.LeanFwd  += (-(float)pad->GetSteeringUpDown() / 128.0f - m_RideAnimData.LeanFwd) * CTimer::GetTimeStep() / 5.0f;
    };

    if (!TheCamera.m_bUseMouse3rdPerson || !m_bEnableMouseSteering) {
        ApplyPadSteer();
    } else if (!CPad::NewMouseControllerState.m_AmountMoved.IsZero() ||
               (std::fabs(m_fRawSteerAngle) > 0.0f && m_nLastControlInput == eControllerType::MOUSE && !pad->IsSteeringInAnyDirection())
    ) {
        m_nLastControlInput = eControllerType::MOUSE;
        if (!pad->NewState.m_bVehicleMouseLook) {
            m_fRawSteerAngle       -= CPad::NewMouseControllerState.m_AmountMoved.x * 0.0035f;
            m_RideAnimData.LeanFwd -= CPad::NewMouseControllerState.m_AmountMoved.y * 0.0035f;
        }
        if (pad->NewState.m_bVehicleMouseLook || std::fabs(m_fRawSteerAngle) < 0.35f) {
            m_fRawSteerAngle *= std::pow(0.98f, CTimer::GetTimeStep());
        }
        if (pad->NewState.m_bVehicleMouseLook || std::fabs(m_RideAnimData.LeanFwd) < 0.35f) {
            m_RideAnimData.LeanFwd *= std::pow(0.98f, CTimer::GetTimeStep());
        }
    } else if (pad->IsSteeringInAnyDirection() || m_nLastControlInput != eControllerType::MOUSE) {
        ApplyPadSteer();
    }
    // else: no mouse movement, no pad steering input, still in mouse mode last frame -> leave both unchanged

    m_fRawSteerAngle       = std::clamp(m_fRawSteerAngle, -1.0f, 1.0f);
    m_RideAnimData.LeanFwd = std::clamp(m_RideAnimData.LeanFwd, -1.0f, 1.0f);

    const auto padGasBrake = ((float)pad->GetAccelerate() - (float)pad->GetBrake()) / 255.0f;

    if (std::fabs(fwdSpeed) >= 0.01f) {
        if (fwdSpeed < 0.0f) {
            if (padGasBrake >= 0.0f) {
                m_BrakePedal = padGasBrake;
                m_GasPedal   = 0.0f;
            } else {
                m_GasPedal   = padGasBrake;
                m_BrakePedal = 0.0f;
            }
        } else {
            if (padGasBrake < 0.0f) {
                m_GasPedal   = 0.0f;
                m_BrakePedal = -padGasBrake;
            } else {
                m_GasPedal   = padGasBrake;
                m_BrakePedal = 0.0f;
            }
        }
    } else {
        // Nearly stationary: both pedals fully pressed at once triggers a burnout/wheelie-prep hold
        // (NOTSA: `this+0x594 != 10` gates this too - exact field/meaning not identified this
        // session, kept as a raw offset check).
        if (pad->GetAccelerate() > 150 && pad->GetBrake() > 150 && *(int*)((char*)this + 0x594) != 10) {
            m_GasPedal   = (float)pad->GetAccelerate() / 255.0f;
            m_nBrakesOn  = true;
            m_BrakePedal = (float)pad->GetBrake() / 255.0f;
        } else {
            m_GasPedal   = padGasBrake;
            m_BrakePedal = 0.0f;
        }
    }

    // NOTSA: `this+0x424` looks like a vehicle-recording playback slot index (checked against
    // CVehicleRecording::bUseCarAI) gating whether the visual wheel-turn angle updates from live
    // input - exact field name not identified this session, kept as a raw offset check.
    if (const auto recordingSlot = *(char*)((char*)this + 0x424); recordingSlot < 0 || CVehicleRecording::bUseCarAI[recordingSlot]) {
        const auto steerSq = m_fRawSteerAngle < 0.0f ? -(m_fRawSteerAngle * m_fRawSteerAngle) : (m_fRawSteerAngle * m_fRawSteerAngle);
        m_fSteerAngle = m_pHandlingData->m_fSteeringLock * 0.017453f * steerSq; // 0.017453f == DAT_008595EC, ~1 degree in radians
    }

    if (vehicleFlags.bComedyControls) {
        // Periodic "wobble the controls" effect - NOTSA: the counter this reads at 0xB7CB84 wasn't
        // identified this session (it isn't CTimer::m_FrameCounter, that's a different address);
        // kept as a raw address read for fidelity.
        const auto comedyCounter = *(uint32*)0xB7CB84;
        if ((comedyCounter & 0x3C00) < 0x3000) {
            m_GasPedal = 1.0f;
        }
        if ((((char)(comedyCounter >> 10) + 6) & 0xF) < 0xC) {
            m_BrakePedal = 0.0f;
        }
        vehicleFlags.bIsHandbrakeOn = false;
        m_fSteerAngle += (comedyCounter & 0x800) ? 0.03f : -0.08f; // DAT_00858B10 / _DAT_00859018
    }

    // NOTSA: `CPad::GetPad(0)+0x10E` (a int16 field) wasn't identified this session - the original
    // gates this whole "player just backed out of the pause/front-end menu, forcibly brake" block on
    // it being nonzero.
    if (*(int16*)((char*)CPad::GetPad(0) + 0x10E) != 0 && CGameLogic::SkipState != SKIP_IN_PROGRESS) {
        m_BrakePedal                = 1.0f;
        vehicleFlags.bIsHandbrakeOn = true;
        m_GasPedal                  = 0.0f;

        // NOTSA: `CPlayerPed::UNREVERSED` (0x60C1E0, called on `FindPlayerPed(-1)`'s result) isn't
        // reversed anywhere in this codebase yet - kept as a raw call for fidelity.
        if (auto* playerPed = FindPlayerPed(-1)) {
            ((void(__thiscall*)(void*))0x60C1E0)(playerPed);
        }

        if (const auto speed = m_vecMoveSpeed.Magnitude(); speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }
}

// 0x6BDEA0
int32 CBike::ProcessEntityCollision(CEntity* entity, CColPoint* outColPoints) {
    if (GetStatus() != STATUS_SIMPLE) {
        vehicleFlags.bVehicleColProcessed = true;
    }

    const auto tcd = GetColData(),
               ocd = entity->GetColData();

#ifdef FIX_BUGS // Text search for `FIX_BUGS@CAutomobile::ProcessEntityCollision:1`
    if (!tcd || !ocd) {
        return 0;
    }
#endif

    if (physicalFlags.bSkipLineCol || physicalFlags.bProcessingShift || entity->GetIsTypePed()) {
        tcd->m_nNumLines = 0; // Later reset back to original value
    }

    const auto ogWheelRatios = m_aWheelRatios;

    auto numColPts = CCollision::ProcessColModels(
        GetMatrix(), *GetColModel(),
        entity->GetMatrix(), *entity->GetColModel(),
        *(std::array<CColPoint, 32>*)(outColPoints),
        m_aWheelColPoints.data(),
        m_aWheelRatios.data(),
        false
    );

    // Possibly add driver & entity collisions to `outColPoints`
    if (m_pDriver && m_nTestPedCollision) {
        const auto pcd = m_pDriver->GetColData();
        if (!pcd->m_nNumLines) {
            std::array<CColPoint, 32> pedCPs{};

            CMatrix driverMat = GetMatrix();
            driverMat.GetPosition() += GetDriverSeatDummyPositionWS();

            std::array<CColPoint, 32> pedEntityColPts{};
            const auto numPedEntityColPts = CCollision::ProcessColModels(
                driverMat, *m_pDriver->GetColModel(),
                entity->GetMatrix(), *entity->GetColModel(),
                pedEntityColPts,
                nullptr,
                nullptr,
                false
            );

            if (numPedEntityColPts) {
                if (m_nTestPedCollision == 1) {
                    m_nTestPedCollision = 0;
                } else {
                    for (auto i = 0; i < numPedEntityColPts && numColPts < 32; i++) {
                        const auto& pedEntityCP = pedCPs[i];
                        if (pedEntityCP.m_nPieceTypeA == PED_COL_SPHERE_LEG) {
                            continue;
                        }
                        outColPoints[numColPts++] = pedEntityCP;
                    }
                }
            }
        }
    }
    
    size_t numProcessedLines{};
    if (tcd->m_nNumLines) {
        // Process the real wheels
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            const auto& cp = m_aWheelColPoints[i];

            const auto wheelColPtsTouchDist = m_aWheelRatios[i];
            if (wheelColPtsTouchDist >= 1.f || wheelColPtsTouchDist >= ogWheelRatios[i]) {
                continue;
            }

            numProcessedLines++;

            m_anCollisionLighting[i] = cp.m_nLightingB;
            m_nContactSurface = cp.m_nSurfaceTypeB;

            switch (entity->GetType()) {
            case ENTITY_TYPE_VEHICLE:
            case ENTITY_TYPE_OBJECT: {
                CEntity::ChangeEntityReference(m_aGroundPhysicalPtrs[i], entity->AsPhysical());

                m_aGroundOffsets[i] = cp.m_vecPoint - entity->GetPosition();
                if (entity->GetIsTypeVehicle()) {
                    m_anCollisionLighting[i] = entity->AsVehicle()->m_anCollisionLighting[i];
                }
                break;
            }
            case ENTITY_TYPE_BUILDING: {
                m_pEntityWeAreOn = entity;
                m_bTunnel = entity->m_bTunnel;
                m_bTunnelTransition = entity->m_bTunnelTransition;
                break;
            }
            }
        }
    } else {
        tcd->m_nNumLines = NUM_SUSP_LINES;
    }

    if (numColPts > 0 || numProcessedLines > 0) {
        AddCollisionRecord(entity);
        if (!entity->GetIsTypeBuilding()) {
            entity->AsPhysical()->AddCollisionRecord(this);
        }
        if (numColPts > 0) {
            if (   entity->GetIsTypeBuilding()
                || (entity->GetIsTypeObject() && entity->AsPhysical()->physicalFlags.bDisableCollisionForce)
            ) {
                SetHasHitWall(true);
            }
        }
    }

    return numColPts;
}

// 0x6B9250
void CBike::ProcessControl() {
    plugin::CallMethod<0x6B9250, CBike*>(this);
}

// 0x6B6740
void CBike::ResetSuspension() {
    for (auto i = 0u; i < m_aWheelPitchAngles.size(); i++) {
        m_aWheelPitchAngles[i] = 0.0f;
        m_WheelStates[i] = WHEEL_STATE_NORMAL;
    }

    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        m_aWheelRatios[i] = 1.0f;
        m_WheelCounts[i] = 0.0f;
    }
}

// 0x6B6790
bool CBike::GetAllWheelsOffGround() const {
    return m_nNoOfContactWheels == 0;
}

// 0x6B67A0
void CBike::DebugCode() {
    // NOP
}

// 0x6B6D40
void CBike::DoSoftGroundResistance(uint32& arg0) {
    auto isOnSandWithContact = false;
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (m_aWheelRatios[i] < 1.0f && g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[i].m_nSurfaceTypeB) == ADHESION_GROUP_SAND) {
            isOnSandWithContact = true;
            break;
        }
    }

    if (isOnSandWithContact) {
        auto perp = m_vecMoveSpeed - GetUp() * DotProduct(m_vecMoveSpeed, GetUp());

        if (m_GasPedal > 0.3f) {
            if (perp.SquaredMagnitude() < sq(0.3f)) {
                arg0 += 4;
            }
            perp -= GetForward() * DotProduct(perp, GetForward());
        }

        ApplyMoveForce(-CTimer::GetTimeStep() * m_fMass * 0.02f * perp);
        return;
    }

    auto isOnRailTrackWithContact = false;
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (m_aWheelRatios[i] < 1.0f && m_aWheelColPoints[i].m_nSurfaceTypeB == SURFACE_RAILTRACK) {
            isOnRailTrackWithContact = true;
            break;
        }
    }

    if (isOnRailTrackWithContact) {
        const auto perp = m_vecMoveSpeed - GetUp() * DotProduct(m_vecMoveSpeed, GetUp());
        ApplyMoveForce(-CTimer::GetTimeStep() * m_fMass * CVehicle::ms_fRailTrackResistance * perp);
    }
}

// 0x6B7130
void CBike::PlayHornIfNecessary() {
    if (m_autoPilot.carCtrlFlags.bHonkAtCar || m_autoPilot.carCtrlFlags.bHonkAtPed)
        PlayCarHorn();
}

// 0x6B7150
void CBike::CalculateLeanMatrix() {
    if (m_bLeanMatrixCalculated)
        return;

    CMatrix mat;
    mat.SetRotateX(fabs(m_RideAnimData.LeanAngle) * -0.05f);
    mat.RotateY(m_RideAnimData.LeanAngle);
    m_mLeanMatrix = GetMatrix();
    m_mLeanMatrix = m_mLeanMatrix * mat;
    // place wheel back on ground
    m_mLeanMatrix.GetPosition() += GetUp() * (1.0f - cos(m_RideAnimData.LeanAngle)) * GetColModel()->GetBoundingBox().m_vecMin.z;
    m_bLeanMatrixCalculated = true;
}

// 0x6B7F90
void CBike::FixHandsToBars(CPed* rider) {
    ((void(__thiscall*)(CBike*, CPed*))0x6B7F90)(this, rider);
}

// 0x6BEEB0
void CBike::PlaceOnRoadProperly() {
    const auto* cm     = GetColModel();
    const auto  fStartY =  cm->m_boundBox.m_vecMax.y;
    const auto  fEndY   = -cm->m_boundBox.m_vecMin.y;

    const auto& vecPos = GetPosition();

    auto vecFrontCheck = vecPos + GetForward() * fStartY;
    auto vecRearCheck  = vecPos - GetForward() * fEndY;

    CColPoint colPoint{};
    CEntity*  colEntity{};
    if (CWorld::ProcessVerticalLine({ vecFrontCheck.x, vecFrontCheck.y, vecPos.z - 5.0f }, vecPos.z + 5.0f, colPoint, colEntity, true)) {
        m_bTunnel           = colEntity->m_bTunnel;
        m_bTunnelTransition = colEntity->m_bTunnelTransition;
        m_pEntityWeAreOn    = colEntity;

        m_FrontCollPoly.ligthing = colPoint.m_nLightingB;
        vecFrontCheck.z          = colPoint.m_vecPoint.z;
    } else {
        vecFrontCheck.z = vecPos.z;
    }

    if (CWorld::ProcessVerticalLine({ vecRearCheck.x, vecRearCheck.y, vecPos.z - 5.0f }, vecPos.z + 5.0f, colPoint, colEntity, true)) {
        m_bTunnel           = colEntity->m_bTunnel;
        m_bTunnelTransition = colEntity->m_bTunnelTransition;
        m_pEntityWeAreOn    = colEntity;

        m_RearCollPoly.ligthing = colPoint.m_nLightingB;
        vecRearCheck.z          = colPoint.m_vecPoint.z;
    } else {
        vecRearCheck.z = vecPos.z;
    }

    const auto fLength   = fEndY + fStartY;
    const auto fPitch    = std::atan2(vecFrontCheck.z - vecRearCheck.z, fLength);
    const auto fCosPitch = std::cos(fPitch);
    const auto fSinPitch = std::sin(fPitch);

    GetRight().Set(
        (vecFrontCheck.y - vecRearCheck.y) / fLength,
        -((vecFrontCheck.x - vecRearCheck.x) / fLength),
        0.0f
    );

    GetForward().Set(
        -(fCosPitch * GetRight().y),
          fCosPitch * GetRight().x,
          fSinPitch
    );

    GetUp() = CrossProduct(GetRight(), GetForward());

    SetPosn({
        (vecFrontCheck.x + vecRearCheck.x) * 0.5f,
        (vecFrontCheck.y + vecRearCheck.y) * 0.5f,
        GetHeightAboveRoad() + (vecFrontCheck.z + vecRearCheck.z) * 0.5f
    });
}

// 0x6BF230
void CBike::GetCorrectedWorldDoorPosition(CVector& out, CVector arg1, CVector arg2) {
    const auto fwdCrossUp = CrossProduct(GetForward(), CVector{ 0.0f, 0.0f, 1.0f });
    const auto basis      = CrossProduct(fwdCrossUp, GetForward());

    auto heightCorrection = 0.0f;
    if (const auto& bbMax = GetColModel()->m_boundBox.m_vecMax; bbMax.x < bbMax.z) {
        heightCorrection = bbMax.z - bbMax.x;
    }

    out  = (arg2.y - arg1.y) * GetForward();
    out += (heightCorrection * DotProduct(fwdCrossUp, GetUp()) + (arg2.x - arg1.x)) * fwdCrossUp;
    out += (arg2.z - arg1.z) * basis;
    out += GetPosition();
}

// 0x6BEA10
void CBike::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    plugin::CallMethod<0x6BEA10, CBike*, CEntity*, uint8>(this, damager, bHideExplosion);
}

// 0x6B7050
void CBike::Fix() {
    vehicleFlags.bIsDamaged = false;
    bikeFlags.bEngineOnFire = false;
    m_nWheelStatus[0] = 0;
    m_nWheelStatus[1] = 0;
}

// 0x6BD090
void CBike::PreRender() {
    plugin::CallMethod<0x6BD090, CBike*>(this);
}

// 0x6BDE20
void CBike::Render() {
    auto savedRef = 0;
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &savedRef);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(1));

    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    CVehicle::Render();

    if (m_renderLights.m_bRightFront) {
        CalculateLeanMatrix();
        CVehicle::DoHeadLightBeam(DUMMY_LIGHT_FRONT_MAIN, m_mLeanMatrix, true);
    }

    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(savedRef));
}

// 0x6BCFC0
void CBike::Teleport(CVector destination, bool resetRotation) {
    CWorld::Remove(this);

    GetPosition() = destination;
    if (resetRotation)
        SetOrientation(0.0f, 0.0f, 0.0f);

    ResetMoveSpeed();
    ResetTurnSpeed();
    ResetSuspension();

    CWorld::Add(this);
}

// NOTSA: `CReferences::AddReference`-equivalent (0x571B70) isn't reversed as a named method in
// this codebase yet (it's part of the yet-unreversed CReferences pool-management internals) - kept
// as a raw call for fidelity rather than guessing at a signature.
static void NOTSA_AddReferenceRaw(void* refSlot, void* entity) {
    ((void(__thiscall*)(void*, void*))0x571B70)(refSlot, entity);
}

// 0x6B8EC0
void CBike::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    if (damageIntensity > 0.0f || m_fDamageIntensity < 1.0f || !vehicleFlags.bCanBeDamaged) {
        return;
    }

    auto intensity = m_fDamageIntensity;

    if (GetStatus() == STATUS_PLAYER && CStats::GetPercentageProgress() >= 100.0f) {
        intensity *= 0.5f;
    }

    if (bikeFlags.bOnSideStand && intensity > 20.0f) {
        bikeFlags.bOnSideStand = false;
    }

    DamageKnockOffRider(this, m_fDamageIntensity, m_nPieceType, m_pDamageEntity, m_vecLastCollisionPosn, m_vecLastCollisionImpactVelocity);

    if (m_pDamageEntity && m_pDamageEntity->GetIsTypeVehicle()) {
        m_nLastWeaponDamageType = WEAPON_RAMMEDBYCAR;
        m_pLastDamageEntity     = m_pDamageEntity;
        NOTSA_AddReferenceRaw(&m_pLastDamageEntity, m_pDamageEntity);
    }

    if (physicalFlags.bCollisionProof ||
        (m_pDamageEntity && m_pDamageEntity->GetIsTypeBuilding() && DotProduct(m_vecLastCollisionPosn, GetUp()) > 0.6f)) {
        return;
    }

    if (intensity > 25.0f && GetStatus() != STATUS_WRECKED) {
        if (vehicleFlags.bIsLawEnforcer && FindPlayerVehicle() && m_pDamageEntity == FindPlayerVehicle() && GetStatus() != STATUS_SIMPLE) {
            if (m_vecMoveSpeed.Magnitude() <= FindPlayerVehicle()->m_vecMoveSpeed.Magnitude() &&
                FindPlayerVehicle()->m_vecMoveSpeed.Magnitude() > 0.1f
            ) {
                FindPlayerPed()->SetWantedLevelNoDrop(eWantedLevel::WANTED_LEVEL_1);
            }
        }

        auto damageDelta = (intensity - 25.0f) * m_pHandlingData->m_fCollisionDamageMultiplier;
        if (damageDelta > 0.0f) {
            if (damageDelta > 5.0f && m_pDriver && m_pDamageEntity && m_pDamageEntity->GetIsTypeVehicle() &&
                (FindPlayerVehicle() != this || m_pDamageEntity->AsVehicle()->m_nCreatedBy != MISSION_VEHICLE) &&
                m_pDamageEntity->AsVehicle()->m_pDriver
            ) {
                m_pDriver->Say(CTX_GLOBAL_CRASH_BIKE, 0, 1.0f);
            }

            const auto healthWasAlive = m_fHealth >= 1.0f;
            if (FindPlayerVehicle() == this) {
                damageDelta *= vehicleFlags.bTakeLessDamage ? (1.0f / 6.0f) : 0.5f;
            } else if (vehicleFlags.bTakeLessDamage) {
                damageDelta *= (1.0f / 12.0f);
            } else if (m_pDamageEntity && m_pDamageEntity == FindPlayerVehicle()) {
                damageDelta *= (2.0f / 3.0f);
            } else {
                damageDelta *= 0.25f;
            }

            m_fHealth -= damageDelta;
            if (m_fHealth < 1.0f && healthWasAlive) {
                m_fHealth = 1.0f;
            }
        }
    }

    if (m_fHealth < 250.0f && !bikeFlags.bEngineOnFire) {
        bikeFlags.bEngineOnFire = true;
        m_BlowUpTimer           = 0.0f;
        m_Damager                = m_pLastDamageEntity;
        if (m_pLastDamageEntity) {
            NOTSA_AddReferenceRaw(&m_Damager, m_pLastDamageEntity);
        }
    }
}

// 0x6B89B0
void CBike::SetupSuspensionLines() {
    plugin::CallMethod<0x6B89B0, CBike*>(this);
}

// 0x6B8970
void CBike::SetModelIndex(uint32 index) {
    CVehicle::SetModelIndex(index);
    SetupModelNodes();
}

// 0x6B5960
void CBike::SetupModelNodes() {
    std::ranges::fill(m_aBikeNodes, nullptr);
    CClumpModelInfo::FillFrameArray(GetRpClump(), m_aBikeNodes.data());
}

// 0x6B7080
void CBike::PlayCarHorn() {
    if (!((m_nAlarmState == 0 || m_nAlarmState == (uint16)-1 || GetStatus() == STATUS_WRECKED) && m_HornCounter == 0)) {
        return;
    }

    if (m_nCarHornTimer != 0) {
        m_nCarHornTimer--;
        return;
    }

    m_nCarHornTimer = (int8)((rand() & 0x7F) + 150);
    const auto pattern = m_nCarHornTimer & 7;

    if (pattern > 1) {
        if (pattern > 3) {
            if (!m_pDriver) {
                return;
            }
            m_pDriver->Say(CTX_GLOBAL_BLOCKED, 0, 1.0f);
            return;
        }

        if (m_pDriver && m_autoPilot.carCtrlFlags.bHonkAtCar) {
            m_pDriver->Say(CTX_GLOBAL_BLOCKED, 0, 1.0f);
        }
    }

    m_HornCounter = 0x2D;
}

// 0x6B7070
void CBike::SetupDamageAfterLoad() {
    // NOP
}

// 0x6B6950
void CBike::DoBurstAndSoftGroundRatios() {
    plugin::CallMethod<0x6B6950, CBike*>(this);
}

// 0x6B67E0
bool CBike::SetUpWheelColModel(CColModel* wheelCol) {
    auto* mi      = GetVehicleModelInfo();
    auto* colData = wheelCol->GetData();

    wheelCol->m_boundBox    = GetColModel()->m_boundBox;
    wheelCol->m_boundSphere = GetColModel()->m_boundSphere;

    // Wheel position relative to the chassis - the wheel frame's own local matrix, with every
    // ancestor's local matrix concatenated in, stopping once the chassis frame itself is reached
    // (its own matrix isn't included).
    const auto GetWheelPosnRelativeToChassis = [this](RwFrame* wheelFrame) {
        RwMatrix mat   = *RwFrameGetMatrix(wheelFrame);
        auto*    frame = RwFrameGetParent(wheelFrame);
        while (frame) {
            RwMatrixTransform(&mat, RwFrameGetMatrix(frame), rwCOMBINEPOSTCONCAT);
            frame = RwFrameGetParent(frame);
            if (frame == m_aBikeNodes[BIKE_CHASSIS]) {
                break;
            }
        }
        return mat.pos;
    };

    colData->m_pSpheres[0].Set(mi->m_fWheelSizeFront * 0.5f, GetWheelPosnRelativeToChassis(m_aBikeNodes[BIKE_WHEEL_FRONT]), SURFACE_RUBBER, 0xd, tColLighting{ 0xFF });
    colData->m_pSpheres[1].Set(mi->m_fWheelSizeRear  * 0.5f, GetWheelPosnRelativeToChassis(m_aBikeNodes[BIKE_WHEEL_REAR]),  SURFACE_RUBBER, 0xf, tColLighting{ 0xFF });

    colData->m_nNumSpheres = 2;
    return true;
}

// 0x6B67B0
void CBike::RemoveRefsToVehicle(CEntity* entityToRemove) {
    for (auto& entity: m_aGroundPhysicalPtrs) {
        if (entity == entityToRemove)
            entity = nullptr;
    }
}

// 0x6B6620
void CBike::ProcessControlCollisionCheck(bool applySpeed) {
    const CMatrix oldMat = GetMatrix();
    SetIsStuck(false);
    SkipPhysics();
    physicalFlags.bSkipLineCol     = false;
    physicalFlags.bProcessingShift = false;
    m_fMovingSpeed                 = 0.0f;
    rng::fill(m_aWheelRatios, 1.0f);

    if (applySpeed) {
        ApplyMoveSpeed();
        ApplyTurnSpeed();

        for (auto i = 0; CheckCollision() && i < 5; i++) {
            GetMatrix() = oldMat;
            ApplyMoveSpeed();
            ApplyTurnSpeed();
        }
    } else {
        const auto usesCollision = GetUsesCollision();
        SetUsesCollision(false);
        CheckCollision();
        SetUsesCollision(usesCollision);
    }

    SetIsStuck(false);
    SetIsInSafePosition(true);
}

// 0x6B5990
void CBike::GetComponentWorldPosition(int32 componentId, CVector& outPos) {
    if (IsComponentPresent(componentId))
        outPos = RwFrameGetLTM(m_aBikeNodes[componentId])->pos;
    else
        NOTSA_LOG_DEBUG("BikeNode missing: model={}, nodeIdx={}", m_nModelIndex, componentId);
}

// 0x6B58D0
void CBike::ProcessOpenDoor(CPed* ped, uint32 doorComponentId, uint32 animGroup, uint32 animId, float fTime) {
    // NOP
}

