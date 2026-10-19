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
#include "Shadows.h"
#include "RwHelper.h"
#include "SurfaceInfos_c.h"
#include "Events/EventVehicleOnFire.h"
#include "Fx/FxManager.h"
#include "Stats.h"
#include "Cheat.h"
#include "Replay.h"
#include "CullZones.h"
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
    RH_ScopedInstall(ProcessBuoyancy, 0x6B5FB0);
    RH_ScopedInstall(ResetSuspension, 0x6B6740);
    RH_ScopedInstall(GetAllWheelsOffGround, 0x6B6790);
    RH_ScopedInstall(DebugCode, 0x6B67A0);
    RH_ScopedInstall(DoSoftGroundResistance, 0x6B6D40);
    RH_ScopedInstall(PlayHornIfNecessary, 0x6B7130);
    RH_ScopedInstall(CalculateLeanMatrix, 0x6B7150);
    RH_ScopedInstall(ProcessRiderAnims, 0x6B7280);
    RH_ScopedInstall(FixHandsToBars, 0x6B7F90);
    RH_ScopedInstall(PlaceOnRoadProperly, 0x6BEEB0);
    RH_ScopedInstall(GetCorrectedWorldDoorPosition, 0x6BF230);
    RH_ScopedVMTInstall(Fix, 0x6B7050);
    RH_ScopedVMTInstall(BlowUpCar, 0x6BEA10);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BF400);
    RH_ScopedVMTInstall(BurstTyre, 0x6BEB20);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6BE310);
    RH_ScopedVMTInstall(ProcessAI, 0x6BC930);
    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6BDEA0);
    RH_ScopedVMTInstall(Render, 0x6BDE20);
    RH_ScopedVMTInstall(PreRender, 0x6BD090);
    RH_ScopedVMTInstall(Teleport, 0x6BCFC0);
    RH_ScopedVMTInstall(ProcessControl, 0x6B9250);
    RH_ScopedVMTInstall(VehicleDamage, 0x6B8EC0);
    RH_ScopedVMTInstall(SetupSuspensionLines, 0x6B89B0);
    RH_ScopedVMTInstall(SetModelIndex, 0x6B8970);
    RH_ScopedVMTInstall(PlayCarHorn, 0x6B7080);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6B7070);
    RH_ScopedVMTInstall(DoBurstAndSoftGroundRatios, 0x6B6950);
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
    m_autoPilot.carCtrlFlags.bHonkAtCar = false;
    m_autoPilot.carCtrlFlags.bHonkAtPed = false;

    if (m_autoPilot.m_vehicleRecordingId >= 0 && !CVehicleRecording::bUseCarAI[m_autoPilot.m_vehicleRecordingId]) {
        extraHandlingFlags += 2;
        return false;
    }

    switch (GetStatus()) {
    case STATUS_PLAYER: {
        extraHandlingFlags += 2;
        bikeFlags.bGettingPickedUp = false;

        const auto pedState = FindPlayerPed()->m_nPedState;
        if (pedState != PEDSTATE_EXIT_CAR && pedState != PEDSTATE_DRAGGED_FROM_CAR) {
            if (m_pDriver) {
                if (m_pDriver == CWorld::Players[0].m_pPed) {
                    ProcessControlInputs(0);
                } else if (m_pDriver == CWorld::Players[1].m_pPed) {
                    ProcessControlInputs(1);
                }
            }

            const auto speedFrac = std::min(0.1f, GetMoveSpeed().Magnitude());
            if (m_RideAnimData.LeanFwd >= 0.0f) {
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanFwdCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_vecCentreOfMass.y;

                if (m_BrakePedal < 0.0f || m_nNoOfContactWheels == 0) {
                    const auto ratio = std::max(speedFrac / 0.1f, m_BrakePedal);
                    const auto force = -(CTimer::GetTimeStep() * CStats::GetFatAndMuscleModifier(STAT_MOD_11) * m_BikeHandling->m_fLeanFwdForce * m_fTurnMass * m_RideAnimData.LeanFwd * speedFrac * (ratio + m_BrakePedal) * 0.5f);
                    ApplyTurnForce(force * GetUp(), m_vecCentreOfMass + GetForward());
                }
            } else {
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanBakCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_vecCentreOfMass.y;

                if ((m_BrakePedal == 0.0f && !vehicleFlags.bIsHandbrakeOn) || m_nNoOfContactWheels == 0) {
                    const auto ratio = std::max(speedFrac / 0.1f, m_GasPedal);
                    const auto force = -(CTimer::GetTimeStep() * CStats::GetFatAndMuscleModifier(STAT_MOD_11) * m_BikeHandling->m_fLeanBakForce * m_fTurnMass * m_RideAnimData.LeanFwd * speedFrac * (ratio + m_GasPedal) * 0.5f);
                    ApplyTurnForce(force * GetUp(), m_vecCentreOfMass + GetForward());
                }
            }

            PruneReferences();
            if (GetStatus() == STATUS_PLAYER) {
                DoDriveByShootings();
            }
            DoSoftGroundResistance(extraHandlingFlags);
        }

        if (CPad::GetPad(0)->CarGunJustDown()) {
            ActivateBomb();
        }
        return true;
    }
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER:
        extraHandlingFlags += 2;
        return true;
    case STATUS_SIMPLE: {
        CCarAI::UpdateCarAI(this);
        CPhysical::ProcessControl();
        CCarCtrl::UpdateCarOnRails(this);

        const auto simpleSpeed = m_autoPilot.m_speed * 0.02f;
        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels = 2;
        m_NumDriveWheelsOnGround = 2;
        m_pHandlingData->GetTransmission().CalculateGearForSimpleCar(simpleSpeed, m_nCurrentGear);

        auto* const mi = GetVehicleModelInfo();
        m_aWheelPitchAngles[0] += ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), GetMoveSpeed(), mi->m_fWheelSizeFront * 0.5f) * CTimer::GetTimeStep();
        m_aWheelPitchAngles[1] += ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), GetMoveSpeed(), mi->m_fWheelSizeRear * 0.5f) * CTimer::GetTimeStep();

        PlayHornIfNecessary();
        ReduceHornCounter();
        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear = false;
        bikeFlags.bWheelieForCamera = false;
        return true;
    }
    case STATUS_PHYSICS:
    case STATUS_GHOST: {
        CCarAI::UpdateCarAI(this);
        CCarCtrl::SteerAICarWithPhysics(this);
        PlayHornIfNecessary();
        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;

        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal   = 0.0f;
            m_BrakePedal = 1.0f;
            return true;
        }
        bikeFlags.bGettingPickedUp = false;
        return true;
    }
    case STATUS_ABANDONED:
        m_BrakePedal = 0.0f;
        vehicleFlags.bIsHandbrakeOn = GetMoveSpeed().SquaredMagnitude() < 0.01f || bikeFlags.bOnSideStand;
        m_GasPedal    = 0.0f;
        m_HornCounter = 0;
        if ((m_pDriver || m_apPassengers[0] || vehicleFlags.bIsBeingCarJacked) && !bikeFlags.bOnSideStand) {
            extraHandlingFlags += 2;
        }
        bikeFlags.bWheelieForCamera = false;
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal   = 0.0f;
            m_BrakePedal = 1.0f;
        }
        return true;
    case STATUS_WRECKED:
        vehicleFlags.bIsHandbrakeOn = true;
        m_BrakePedal = 0.05f;
        m_fSteerAngle = 0.0f;
        m_GasPedal    = 0.0f;
        m_HornCounter = 0;
        bikeFlags.bWheelieForCamera = false;
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        return true;
    case STATUS_FORCED_STOP:
        if (GetMoveSpeed().SquaredMagnitude() >= 0.01f) {
            vehicleFlags.bIsHandbrakeOn = false;
            m_BrakePedal = 0.0f;
        } else {
            vehicleFlags.bIsHandbrakeOn = true;
            m_BrakePedal = 1.0f;
        }
        m_fSteerAngle = 0.0f;
        m_GasPedal    = 0.0f;
        m_HornCounter = 0;
        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;
        return true;
    default:
        return true;
    }
}

// 0x6BF400
void CBike::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && GetStatus() == STATUS_PLAYER)
        return;

    ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
}

// 0x6B7280
void CBike::ProcessRiderAnims(CPed* rider, CVehicle* vehicle, CRideAnimData* rideData, tBikeHandlingData* handling, int16 a5) {
    const auto isPlayer = rider->IsPlayer();
    auto* const clump = rider->GetRpClump();

    CBike*       bike{};
    CAutomobile* quad{};
    uint8        contactWheels{};
    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        bike          = vehicle->AsBike();
        contactWheels = bike->m_nNoOfContactWheels;
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        quad          = vehicle->AsAutomobile();
        contactWheels = quad->m_nNumContactWheels;
    }

    // Share of the idle pose that is left once the other blending anims took theirs
    float animWeight = 1.0f;
    // Target for the forward lean; -1 means leaning back
    float leanFwdTarget = 0.0f;

    const auto GetAnim = [&](AnimationId id) { return RpAnimBlendClumpGetAssociation(clump, (uint32)id); };

    auto* leftAnim  = GetAnim(ANIM_ID_BIKE_LEFT);
    auto* rightAnim = GetAnim(ANIM_ID_BIKE_RIGHT);
    auto* stillAnim = GetAnim(ANIM_ID_BIKE_STILL);

    CAnimBlendAssociation *fwdAnim{}, *backAnim{};
    if (isPlayer) {
        fwdAnim  = GetAnim(ANIM_ID_BIKE_FWD);
        backAnim = GetAnim(ANIM_ID_BIKE_BACK);
    }

    auto* pushesAnim  = GetAnim(ANIM_ID_BIKE_PUSHES);
    auto* driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYLHS);
    if (!driveByAnim) {
        driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYRHS);
        if (!driveByAnim) {
            driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYFT);
        }
    }

    const auto fwdSpeed = DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward());

    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX && (pushesAnim || driveByAnim)) {
        static_cast<CBike*>(vehicle)->m_nFixRightHand = 0;
    }

    const auto KnockOffBoth = [&] {
        const CVector impactVelocity{ 0.0f, 0.0f, 1.0f };
        rider->GetEventGroup().Add(CEventKnockOffBike{
            vehicle, vehicle->m_vecMoveSpeed, impactVelocity, 0.0f, 0.0f, KNOCK_OFF_TYPE_SKIDBACK_FALLR, 2, 0, nullptr, true, false
        });
        if (auto* const passenger = vehicle->m_apPassengers[0]; passenger && passenger != rider) {
            passenger->GetEventGroup().Add(CEventKnockOffBike{
                vehicle, vehicle->m_vecMoveSpeed, impactVelocity, 0.0f, 0.0f, KNOCK_OFF_TYPE_SKIDBACK_FALLR, 2, 0, nullptr, false, false
            });
        }
    };

    // Spinning too fast for the rider's skill throws everyone off
    const auto skill        = rider->GetBikeRidingSkill();
    const auto maxTurnSpeed = (skill + 1.0f) * 0.3f;
    if (sq(maxTurnSpeed) < vehicle->m_vecTurnSpeed.SquaredMagnitude()) {
        KnockOffBoth();
    }

    // True when the anim is missing or not on its way in
    const auto NeedsBlendIn = [](const CAnimBlendAssociation* a) {
        return !a || (a->m_BlendAmount < 1.0f && a->m_BlendDelta <= 0.0f);
    };
    const auto FadeOut = [](CAnimBlendAssociation* a) {
        if (a && a->m_BlendDelta >= 0.0f) {
            a->m_BlendDelta = -4.0f;
        }
    };

    if (driveByAnim || std::abs(fwdSpeed) >= 0.02f) {
        if (fwdSpeed >= 0.0f) {
            FadeOut(stillAnim);
            FadeOut(pushesAnim);
        } else {
            const auto maxReverseSpeed = vehicle->m_pHandlingData->m_transmissionData.m_MaxReverseVelocity;

            auto reverseLimit = (skill + 1.0f) * 3.5f;
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
                reverseLimit *= 2.0f;
            }

            const auto tooFastInReverse = reverseLimit * maxReverseSpeed > fwdSpeed
                && (contactWheels > 0 || (vehicle->GetUp().z < -0.5f && CTimer::GetTimeInMS() - vehicle->m_nLastCollisionTime < 100));
            if (tooFastInReverse) {
                KnockOffBoth();
            } else if (vehicle->m_GasPedal >= 0.0f || fwdSpeed <= maxReverseSpeed * 1.5f) {
                if (isPlayer && fwdSpeed < maxReverseSpeed * 1.5f) {
                    leanFwdTarget = -1.0f;
                }
                FadeOut(stillAnim);
                FadeOut(pushesAnim);
            } else if (NeedsBlendIn(pushesAnim)) {
                pushesAnim = CAnimManager::BlendAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_PUSHES, 4.0f);
            }
        }
    } else if (NeedsBlendIn(stillAnim)) {
        stillAnim = CAnimManager::BlendAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_STILL, 2.0f);
    }

    // What is left of the idle pose after the blending anims of this frame
    for (const auto* const anim : { stillAnim, driveByAnim, pushesAnim }) {
        if (anim) {
            const auto blend = CTimer::GetTimeStepNonClipped() * 0.02f * anim->m_BlendDelta + anim->m_BlendAmount;
            animWeight -= std::min(blend, animWeight);
        }
    }

    // Sideways lean
    float lean = rideData->LeanAngle / handling->m_fFullAnimLean;
    lean = leanFwdTarget == -1.0f ? 0.0f : std::clamp(lean, -1.0f, 1.0f);

    const auto sideSmoothing = std::pow(0.86f, CTimer::GetTimeStep());
    rideData->AnimLeanLeft = sideSmoothing * rideData->AnimLeanLeft + (1.0f - sideSmoothing) * lean;

    // Forward lean
    if (!isPlayer || vehicle->m_apPassengers[0]) {
        leanFwdTarget = 0.0f;
        if (bike) {
            bike->bikeFlags.bWheelieForCamera = false;
        }
    } else if (leanFwdTarget > -1.0f) {
        leanFwdTarget = rideData->LeanFwd;

        // Distance of the nose to the wheelie angle, when the front is lifted
        float wheelieGap{};
        auto  isLifted = false;
        if (bike) {
            bike->bikeFlags.bWheelieForCamera = false;

            const auto& wheelCounts = bike->m_WheelCounts;
            const auto  noseZ       = bike->GetForward().z;
            if (wheelCounts[0] <= 0.0f && wheelCounts[1] <= 0.0f && noseZ > 0.0f && (wheelCounts[2] > 0.0f || wheelCounts[3] > 0.0f)) {
                wheelieGap = handling->m_fWheelieAng - noseZ;
                if (wheelieGap < handling->m_fWheelieAng * 0.5f) {
                    bike->bikeFlags.bWheelieForCamera = true;
                }
                isLifted = true;
            } else if (wheelCounts[2] <= 0.0f && wheelCounts[3] <= 0.0f && noseZ < 0.0f && (wheelCounts[0] > 0.0f || wheelCounts[1] > 0.0f)) {
                if (handling->m_fStoppieAng * 0.6f < handling->m_fStoppieAng - noseZ) {
                    bike->bikeFlags.bWheelieForCamera = true;
                }
            }
        } else if (quad) {
            const auto& wheelCounts = quad->m_WheelCounts;
            const auto  noseZ       = quad->GetForward().z;
            if (wheelCounts[0] <= 0.0f && wheelCounts[1] <= 0.0f && noseZ > 0.0f && (wheelCounts[2] > 0.0f || wheelCounts[3] > 0.0f)) {
                wheelieGap = handling->m_fWheelieAng - noseZ;
                isLifted   = true;
            }
        }

        if (isLifted && wheelieGap < 0.15f) {
            leanFwdTarget = std::max(leanFwdTarget, 0.25f);
        } else if (vehicle->m_BrakePedal > 0.5f && fwdSpeed > 0.01f) {
            leanFwdTarget = std::max(leanFwdTarget, 0.1f);
        } else if (vehicle->m_GasPedal > 0.5f && leanFwdTarget <= 0.0f
                   && fwdSpeed < vehicle->m_pHandlingData->m_transmissionData.m_MaxFlatVelocity * 0.3f
                   && leanFwdTarget >= -0.3f) {
            leanFwdTarget = -0.3f;
        }

        // Leaning far to the side leaves less room to lean forward
        if (std::abs(lean) > 0.3f) {
            const auto room = std::max(0.0f, 1.0f - (std::abs(lean) - 0.3f) / (0.56f - 0.3f));
            leanFwdTarget *= room;
        }
    }

    if (isPlayer) {
        const auto fwdSmoothing = std::pow(0.89f, CTimer::GetTimeStep());
        rideData->AnimLeanFwd = fwdSmoothing * rideData->AnimLeanFwd + (1.0f - fwdSmoothing) * leanFwdTarget;
    } else {
        rideData->AnimLeanFwd = 0.0f;
    }

    // How the lean is split between the sideways and the forward anims
    float sideShare = 1.0f, fwdShare = 0.0f;
    if (isPlayer && std::abs(rideData->AnimLeanLeft) <= 0.56f) {
        if (std::abs(rideData->AnimLeanFwd) > 0.56f) {
            sideShare = 0.0f;
            fwdShare  = 1.0f;
        } else {
            const auto x   = rideData->AnimLeanLeft;
            const auto y   = rideData->AnimLeanFwd;
            const auto len = std::sqrt(y * y + x * x);
            if (len <= 0.01f) {
                sideShare = std::abs(x);
                fwdShare  = std::abs(y);
            } else {
                const auto invLen = 1.0f / len;
                sideShare = std::abs(invLen * x);
                fwdShare  = std::abs(y * invLen);
            }
        }
    }
    sideShare *= animWeight;
    fwdShare  *= animWeight;

    // These anims are never played; their time is set by hand from the amount of lean
    const auto PoseAnim = [](CAnimBlendAssociation* anim, float share, float time) {
        anim->m_BlendAmount = share;
        anim->SetCurrentTime(time);
        anim->m_Flags &= ~ANIMATION_IS_PLAYING;
    };

    if (isPlayer) {
        if (!fwdAnim) {
            fwdAnim = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_FWD);
        }
        if (!backAnim) {
            backAnim = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_BACK);
        }
        if (rideData->AnimLeanFwd < 0.0f) {
            PoseAnim(backAnim, fwdShare, -(backAnim->m_BlendHier->m_fTotalTime * rideData->AnimLeanFwd));
            fwdAnim->m_BlendAmount = 0.0f;
        } else {
            PoseAnim(fwdAnim, fwdShare, fwdAnim->m_BlendHier->m_fTotalTime * rideData->AnimLeanFwd);
            backAnim->m_BlendAmount = 0.0f;
        }
    }

    if (!leftAnim) {
        leftAnim = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_LEFT);
    }
    if (!rightAnim) {
        rightAnim = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_RIGHT);
    }
    if (rideData->AnimLeanLeft < 0.0f) {
        PoseAnim(leftAnim, sideShare, -(leftAnim->m_BlendHier->m_fTotalTime * rideData->AnimLeanLeft));
        rightAnim->m_BlendAmount = 0.0f;
    } else {
        PoseAnim(rightAnim, sideShare, rightAnim->m_BlendHier->m_fTotalTime * rideData->AnimLeanLeft);
        leftAnim->m_BlendAmount = 0.0f;
    }

    // At speed the wind shakes the head around
    if (fwdSpeed > 0.3f) {
        auto* const headOrientation = &rider->m_apBones[PED_NODE_HEAD]->KeyFrame->q;
        const auto  maxAngle        = fwdSpeed * 6.0f;

        RwV3d axisX{ 1.0f, 0.0f, 0.0f };
        RtQuatRotate(headOrientation, &axisX, CGeneral::GetRandomNumberInRange(-maxAngle, maxAngle), rwCOMBINEPOSTCONCAT);

        RwV3d axisY{ 0.0f, 1.0f, 0.0f };
        RtQuatRotate(headOrientation, &axisY, CGeneral::GetRandomNumberInRange(-maxAngle, maxAngle), rwCOMBINEPOSTCONCAT);

        rider->bUpdateMatricesRequired = true;
    }
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
    auto* const colModel = GetColModel();
    auto* const colData  = colModel->m_pColData;
    auto* const mi       = GetVehicleModelInfo();
    const auto  ts       = CTimer::GetTimeStep();

    uint32 extraHandlingFlags = 0;

    m_vehicleAudio.Service();

    vehicleFlags.bWarnedPeds        = false;
    m_bLeanMatrixCalculated         = false;
    m_nBrakesOn                     = 0;
    bikeFlags.bPlayerBoost          = false;
    vehicleFlags.bRestingOnPhysical = false;

    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    ProcessCarAlarm();
    ActivateBombWhenEntered();
    UpdateClumpAlpha();

    // Peds in the bike only collide with it after a frame of grace
    if (!m_pDriver || (!m_pDriver->IsPlayer() && (!m_apPassengers[0] || !m_apPassengers[0]->IsPlayer()))) {
        m_nTestPedCollision = 0;
    } else if (m_nTestPedCollision == 1) {
        m_nTestPedCollision = 2;
    } else if (m_nTestPedCollision == 0) {
        m_nTestPedCollision = 1;
    }

    ProcessAI(extraHandlingFlags);
    if (GetStatus() == STATUS_SIMPLE) {
        return;
    }

    if (bikeFlags.bOnSideStand) {
        if (std::abs(GetRight().z) > 0.35f || std::abs(GetForward().z) > 0.5f) {
            bikeFlags.bOnSideStand = false;
        }
    }

    // Held up (side stand, being picked up or forced by the AI): the bike gets rotational damping and the centre of mass of the model
    const auto IsHeldUp = [&] { return (extraHandlingFlags & 2) || bikeFlags.bGettingPickedUp || bikeFlags.bOnSideStand; };
    if (!IsHeldUp()) {
        m_vecCentreOfMass.x = m_pHandlingData->m_vecCentreOfMass.x;
        m_vecCentreOfMass.y = m_pHandlingData->m_vecCentreOfMass.y;
        m_vecCentreOfMass.z = m_BikeHandling->m_fNoPlayerCOMz;
    } else {
        const auto localTurn = GetMatrix().InverseTransformVector(m_vecTurnSpeed);

        float pitchDamping = 0.9995f;
        float rollDamping  = 0.9f;
        float pitchWeight  = 1.0f;
        if (GetStatus() == STATUS_PLAYER) {
            if (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f) {
                // Front wheel on the ground, rear wheel in the air (a stoppie)
                if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f) {
                    pitchWeight = 100.0f;
                    const auto limit  = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.075f;
                    const auto weight = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.25f;
                    if (GetForward().z < 0.0f) {
                        const auto amount = std::abs(m_BikeHandling->m_fStoppieAng - GetForward().z) * weight;
                        pitchDamping = (std::min(amount, limit) + 0.9f) * pitchDamping;
                    }
                }
            } else {
                // Front wheel in the air (a wheelie)
                pitchWeight = 100.0f;
                const auto weight = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.2f;
                if ((m_aWheelRatios[2] >= 1.0f && m_aWheelRatios[3] >= 1.0f) || GetForward().z <= 0.0f) {
                    pitchDamping = 0.98f;
                } else {
                    const auto amount = std::abs(m_BikeHandling->m_fWheelieAng - GetForward().z) * weight;
                    pitchDamping = amount <= 0.05f ? pitchDamping - amount : pitchDamping - 0.05f;
                }
            }
        }

        pitchDamping = pitchDamping / (localTurn.x * localTurn.x * pitchWeight + 1.0f);
        rollDamping  = rollDamping / (1000.0f * localTurn.y * localTurn.y + 1.0f);
        pitchDamping = std::pow(pitchDamping, ts);
        rollDamping  = std::pow(rollDamping, ts);

        const auto pitchChange = localTurn.x * pitchDamping - localTurn.x;
        const auto rollChange  = localTurn.y * rollDamping - localTurn.y;

        ApplyTurnForce(-GetUp() * rollChange * m_fTurnMass, GetMatrix().TransformVector(m_vecCentreOfMass) + GetRight());
        ApplyTurnForce(GetUp() * pitchChange * m_fTurnMass, GetMatrix().TransformVector(m_vecCentreOfMass) + GetForward());

        if (GetStatus() != STATUS_PLAYER) {
            m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
        }
    }

    // Wrecked and abandoned bikes that stopped moving get their speed zeroed after a few frames
    auto speedZeroed = false;
    if (!m_bIsStuck && (GetStatus() == STATUS_ABANDONED || GetStatus() == STATUS_WRECKED) && !bikeFlags.bGettingPickedUp) {
        const auto isIdle = !vehicleFlags.bVehicleColProcessed
            && m_vecMoveSpeed.x == 0.0f && m_vecMoveSpeed.y == 0.0f && m_vecMoveSpeed.z == 0.0f
            && m_aRatioHistory[3] != 1.0f;

        float forceLimit, torqueLimit, movingLimit;
        if (GetStatus() == STATUS_WRECKED) {
            forceLimit  = 0.006f;
            torqueLimit = 0.0015f;
            movingLimit = 0.015f;
        } else {
            forceLimit  = 0.003f;
            torqueLimit = 0.0009f;
            movingLimit = 0.005f;
        }

        m_vecForce  = (m_vecForce + m_vecMoveSpeed) / 2.0f;
        m_vecTorque = (m_vecTorque + m_vecTurnSpeed) / 2.0f;

        forceLimit *= ts;
        const auto isSlowForce = !(forceLimit * forceLimit < m_vecForce.SquaredMagnitude());
        torqueLimit *= ts;
        const auto isSlowTorque = !(torqueLimit * torqueLimit < m_vecTorque.SquaredMagnitude());

        if ((isSlowForce && isSlowTorque && m_fMovingSpeed < movingLimit) || isIdle) {
            m_nFakePhysics++;
            if (m_nFakePhysics > 10 || isIdle) {
                if (!isIdle || m_nFakePhysics > 10) {
                    m_nFakePhysics = 10;
                }
                m_vecMoveSpeed = CVector{};
                m_vecTurnSpeed = CVector{};
                speedZeroed    = true;
            }
        } else {
            m_nFakePhysics = 0;
        }
    }

    for (auto* const phys : m_aGroundPhysicalPtrs) {
        if (!phys) {
            continue;
        }
        vehicleFlags.bRestingOnPhysical = true;
        if (CWorld::bForceProcessControl && phys->m_bIsInSafePosition) {
            SetWasPostponed(true);
            return;
        }
    }
    if (vehicleFlags.bRestingOnPhysical) {
        speedZeroed    = false;
        m_nFakePhysics = 0;
    }

    VehicleDamage(0.0f, eVehicleCollisionComponent::DEFAULT, nullptr, nullptr, nullptr, WEAPON_RAMMEDBYCAR);

    // Hit from the side, standing still or being picked up
    auto hitFromSide = false;
    if (m_fDamageIntensity > 0.0f
        && std::abs(DotProduct(m_vecLastCollisionImpactVelocity, GetRight())) > 0.5f
        && m_vecMoveSpeed.SquaredMagnitude() < 0.1f) {
        hitFromSide = true;
    }
    if (bikeFlags.bGettingPickedUp) {
        hitFromSide = true;
    }

    // Ground contact of the wheels is read through these in every part that follows
    static auto& wheelThrust     = StaticRef<float>(0xC1C27C);
    static auto& frontWheelState = StaticRef<tWheelState>(0xC1C26C);
    static auto& rearWheelState  = StaticRef<tWheelState>(0xC1C270);

    if (speedZeroed) {
        SkipPhysics();
        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;

        if (bikeFlags.bOnSideStand) {
            if (m_RideAnimData.BarSteerAngle < 0.34906587f) {
                m_RideAnimData.BarSteerAngle += ts * 0.017453292f;
            }

            // The bike leans on the stand, the lean follows the way it sits
            const auto smoothing = (float)std::pow(0.97, (double)ts);
            const auto sideness  = std::clamp(GetRight().z, -1.0f, 1.0f);
            const auto lean      = smoothing * m_RideAnimData.DesiredLeanAngle - (std::asin(sideness) + 0.2617994f) * (1.0f - smoothing);
            m_RideAnimData.DesiredLeanAngle = lean;
            m_RideAnimData.LeanAngle        = lean;
        }
    } else {
        if (!vehicleFlags.bVehicleColProcessed) {
            ProcessControlCollisionCheck(true);
        }

        // Steers by itself towards the side it falls to
        if (!IsHeldUp()) {
            if (GetRight().z >= 0.0f) {
                if (m_fSteerAngle < 0.43633232f) {
                    m_fSteerAngle += ts * 0.008726646f;
                }
            } else if (m_fSteerAngle > -0.43633232f) {
                m_fSteerAngle -= ts * 0.008726646f;
            }
        }

        // The wheelie animation of the player lowers the air resistance for the time it lasts
        const auto airResistance = m_fAirResistance;
        if (GetStatus() == STATUS_PLAYER && m_pDriver) {
            if (auto* const assoc = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), (uint32)ANIM_ID_BIKE_FWD);
                assoc && assoc->m_BlendAmount > 0.5f && assoc->m_CurrentTime > 0.06f && assoc->m_CurrentTime < 0.14f) {
                m_fAirResistance *= CCullZones::DoExtraAirResistanceForPlayer() ? 0.85f : 0.6f;
                if (m_GasPedal > 0.5f && DotProduct(m_vecMoveSpeed, GetForward()) > 0.25f) {
                    ApplyMoveForce(GetForward() * (ts * m_fMass * 0.0016f));
                    bikeFlags.bPlayerBoost = true;
                }
            }
        }

        const auto wasSubmerged = physicalFlags.bSubmergedInWater;
        CPhysical::ProcessControl();
        m_fAirResistance = airResistance;
        ProcessBuoyancy();

        if (!wasSubmerged && physicalFlags.bSubmergedInWater) {
            if (m_pDriver && m_pDriver->IsPlayer()) {
                m_pDriver->AsPlayer()->ResetPlayerBreath();
            } else {
                for (auto i = 0; i < m_nMaxPassengers; i++) {
                    if (m_apPassengers[i] && m_apPassengers[i]->IsPlayer()) {
                        m_apPassengers[i]->AsPlayer()->ResetPlayerBreath();
                    }
                }
            }
        }

        for (size_t i = 0; i < 4; i++) {
            const auto restRatio = 1.0f - m_fSuspensionLength[i] / m_fLineLength[i];
            m_aWheelRatios[i]    = (m_aWheelRatios[i] - restRatio) / (1.0f - restRatio);
        }

        const auto* const lines = colData->m_pLines;

        std::array<CVector, 4> springDirs{};     // Direction of the suspension line of each wheel
        std::array<CVector, 4> contactPoints{};  // Where the wheel touches, relative to the bike
        std::array<CVector, 4> contactSpeeds{};  // Speed of that point
        std::array<float, 4>   springForces{};

        for (size_t i = 0; i < 4; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                contactPoints[i] = m_aWheelColPoints[i].m_vecPoint - GetPosition();
                springDirs[i]    = GetMatrix().TransformVector(lines[i].m_vecEnd - lines[i].m_vecStart);
                springDirs[i].Normalise();
            }
        }

        m_aWheelSkidmarkType[0] = m_aWheelSkidmarkType[1] = (eSkidmarkType)0;
        m_bMoreSkidMarks[0]     = m_bMoreSkidMarks[1]     = false;
        for (size_t i = 0; i < 4; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                auto bias = m_pHandlingData->m_fSuspensionBiasBetweenFrontAndRear;
                if (i >= 2) {
                    bias = 1.0f - bias;
                }

                if (m_aWheelColPoints[i].m_vecNormal.z <= 0.35f) {
                    ApplySpringCollision(m_pHandlingData->m_fSuspensionForceLevel, springDirs[i], contactPoints[i], m_aWheelRatios[i], bias, springForces[i]);
                } else {
                    ApplySpringCollisionAlt(m_pHandlingData->m_fSuspensionForceLevel, springDirs[i], contactPoints[i], m_aWheelRatios[i], bias, m_aWheelColPoints[i].m_vecNormal, springForces[i]);
                }

                const auto skidmark = g_surfaceInfos.GetSkidmarkType(m_aWheelColPoints[i].m_nSurfaceTypeB);
                m_aWheelSkidmarkType[i >= 2 ? 1 : 0] = (eSkidmarkType)skidmark;
                if (skidmark == 2) {
                    m_bMoreSkidMarks[i >= 2 ? 1 : 0] = true;
                }
            } else {
                contactPoints[i] = GetMatrix().TransformVector(lines[i].m_vecEnd);
            }
        }

        // Speed of every contact point, relative to what the wheel stands on
        const auto UpdateContactSpeeds = [&] {
            for (size_t i = 0; i < 4; i++) {
                contactSpeeds[i] = GetSpeed(contactPoints[i]);
                if (m_aGroundPhysicalPtrs[i]) {
                    contactSpeeds[i] -= m_aGroundPhysicalPtrs[i]->GetSpeed(m_aGroundOffsets[i]);
                }
            }
        };
        UpdateContactSpeeds();

        // Wheels that touch a steep surface push along its normal instead of the suspension line
        const auto UseNormalAsSpringDir = [&](size_t wheel, size_t colPoint) {
            if (m_aWheelColPoints[colPoint].m_vecNormal.z > 0.35f) {
                springDirs[wheel] = -m_aWheelColPoints[colPoint].m_vecNormal;
            }
        };
        if (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f) {
            UseNormalAsSpringDir(0, m_aWheelRatios[0] >= 1.0f ? 1 : 0);
            UseNormalAsSpringDir(1, m_aWheelRatios[1] >= 1.0f ? 0 : 1);
        }
        if (m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f) {
            UseNormalAsSpringDir(2, m_aWheelRatios[2] >= 1.0f ? 3 : 2);
            UseNormalAsSpringDir(3, m_aWheelRatios[3] >= 1.0f ? 2 : 3);
        }

        for (size_t i = 0; i < 4; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                ApplySpringDampening(m_pHandlingData->m_fSuspensionDampingLevel, springForces[i], springDirs[i], contactPoints[i], contactSpeeds[i]);
            }
        }
        UpdateContactSpeeds();

        // Engine and brakes
        auto speedForward = DotProduct(m_vecMoveSpeed, GetForward());
        auto acceleration = m_pHandlingData->GetTransmission().CalculateDriveAcceleration(
            m_GasPedal,
            m_nCurrentGear,
            m_fGearChangeCount,
            speedForward,
            nullptr,
            nullptr,
            m_NumDriveWheelsOnGround,
            CCheat::IsActive(CHEAT_PERFECT_HANDLING)
        );
        acceleration /= m_fVelocityFrequency;

        auto brake = m_pHandlingData->m_fBrakeDeceleration * m_BrakePedal * ts;

        float frontBrakeShare, rearBrakeShare, frontTractionShare, rearTractionShare;
        if (GetStatus() != STATUS_PLAYER && GetStatus() != STATUS_REMOTE_CONTROLLED && m_pHandlingData->m_bNpcNeutralHandl) {
            frontBrakeShare = rearBrakeShare = frontTractionShare = rearTractionShare = 1.0f;
        } else {
            frontBrakeShare    = m_pHandlingData->m_fBrakeBias * 2.0f;
            rearBrakeShare     = (1.0f - m_pHandlingData->m_fBrakeBias) * 2.0f;
            frontTractionShare = m_pHandlingData->m_fTractionBias * 2.0f;
            rearTractionShare  = 2.0f - frontTractionShare;
        }

        // Which wheels touch the ground, for how long, and the mean of their normals
        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels              = 0;
        m_NumDriveWheelsOnGround          = 0;
        for (size_t i = 0; i < 4; i++) {
            if (m_aWheelRatios[i] >= 1.0f) {
                m_WheelCounts[i] = std::max(m_WheelCounts[i] - ts, 0.0f);
                if (m_WheelCounts[i] <= 0.0f) {
                    continue;
                }
            } else {
                m_WheelCounts[i] = 4.0f;
            }

            m_nNoOfContactWheels++;
            if (i == 2 || i == 3) {
                m_NumDriveWheelsOnGround = 1;
            }
            if (m_nNoOfContactWheels == 1) {
                m_vecAveGroundNormal = m_aWheelColPoints[i].m_vecNormal;
            } else {
                m_vecAveGroundNormal += m_aWheelColPoints[i].m_vecNormal;
            }
        }
        if (m_nNoOfContactWheels == 0) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
        } else {
            m_vecAveGroundNormal *= 1.0f / (float)m_nNoOfContactWheels;
            if (DotProduct(GetUp(), m_vecAveGroundNormal) < -0.5f) {
                m_vecAveGroundNormal *= -1.0f;
            }
        }

        // The point of the front and rear wheel that touches the ground, relative to the bike
        const size_t frontLine = m_aWheelRatios[1] <= m_aWheelRatios[0] ? 1 : 0;
        const size_t rearLine  = m_aWheelRatios[3] <= m_aWheelRatios[2] ? 3 : 2;

        const CVector frontContactLocal{
            0.0f,
            lines[0].m_vecStart.y,
            lines[0].m_vecStart.z - m_aWheelRatios[frontLine] * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        const auto frontContact = GetMatrix().TransformVector(frontContactLocal);

        const CVector rearContactLocal{
            0.0f,
            lines[3].m_vecStart.y,
            lines[2].m_vecStart.z - m_aWheelRatios[rearLine] * m_fSuspensionLength[2] - mi->m_fWheelSizeRear * 0.5f
        };
        const auto rearContact = GetMatrix().TransformVector(rearContactLocal);

        constexpr auto TRACTION_UNIT = (1.0f / (50.0f * 50.0f)) * 10.0f;
        const auto     traction      = m_pHandlingData->m_fTractionMultiplier * m_fExtraTractionMult * TRACTION_UNIT * 0.25f;

        // Handlebars
        if (GetStatus() != STATUS_PLAYER && bikeFlags.bOnSideStand && !bikeFlags.bGettingPickedUp) {
            if (m_RideAnimData.BarSteerAngle < 0.34906587f) {
                m_RideAnimData.BarSteerAngle += ts * 0.02617994f;
            }
        } else if (std::abs(m_vecMoveSpeed.x) < 0.01f && std::abs(m_vecMoveSpeed.y) < 0.01f && m_fSteerAngle == 0.0f) {
            m_RideAnimData.BarSteerAngle = (float)std::pow(0.96, (double)ts) * m_RideAnimData.BarSteerAngle;
        } else {
            auto steerShare = 1.0f;
            if (speedForward > 0.01f && (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) && GetStatus() == STATUS_PLAYER) {
                CColPoint adhesionProbe{};
                adhesionProbe.m_nSurfaceTypeA = (eSurfaceType)0x3C;
                adhesionProbe.m_nSurfaceTypeB = (eSurfaceType)1;
                auto limit = g_surfaceInfos.GetAdhesiveLimit(&adhesionProbe) * m_BikeHandling->m_fSpeedSteer * traction * 4.0f;

                const auto adhesionGroup = g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[rearLine].m_nSurfaceTypeB);
                if (adhesionGroup == 3 || adhesionGroup == 4) {
                    limit *= m_BikeHandling->m_fSlipSteer;
                }

                const auto ratio = std::min(limit / (speedForward * speedForward), 1.0f);
                steerShare = std::asin(ratio) / (m_pHandlingData->m_fSteeringLock * 0.017453292f);
                if ((m_fSteerAngle < 0.0f && m_RideAnimData.LeanAngle < 0.0f) || (m_fSteerAngle > 0.0f && m_RideAnimData.LeanAngle > 0.0f)) {
                    steerShare *= 2.0f;
                }
                if (steerShare > 1.0f) {
                    steerShare = 1.0f;
                }
            }
            if (GetStatus() != STATUS_PLAYER) {
                steerShare = 1.0f;
            }
            m_RideAnimData.BarSteerAngle = steerShare * m_fSteerAngle;
        }

        const auto moveSpeedBefore = m_vecMoveSpeed;
        const auto rearWheelFirst  = m_pHandlingData->m_bProcRearwheelFirst;

        // Projects the forward of the bike on the ground the wheel stands on, the result is the way the tyre rolls and slides
        const auto GetWheelAxes = [&](const CVector& forward, size_t colPoint, CVector& outFwd, CVector& outRight) {
            const auto& normal = m_aWheelColPoints[colPoint].m_vecNormal;
            outFwd = forward - normal * DotProduct(forward, normal);
            outFwd.Normalise();
            outRight = CrossProduct(outFwd, normal);
            outRight.Normalise();
            if (hitFromSide) {
                outRight.z = 0.0f;
            }
        };

        // Grip of the tyre of the given wheel. The thrust and the surface type of the wheel are set before the lookup
        const auto GetWheelAdhesion = [&](size_t colPoint, float thrust) {
            wheelThrust = thrust;
            m_aWheelColPoints[colPoint].m_nSurfaceTypeA = (eSurfaceType)0x3C;
            return g_surfaceInfos.GetAdhesiveLimit(&m_aWheelColPoints[colPoint]);
        };

        // How much of the grip is kept while sliding on the given surface
        const auto GetDestabilisation = [&](size_t colPoint) {
            auto destab = 1.0f;
            if (m_fBrakingSlide > 0.0f) {
                switch (g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[colPoint].m_nSurfaceTypeB)) {
                case 1:
                case 3: destab = 0.9f; break;
                case 2: destab = 0.7f; break;
                default: break;
                }
            }
            return destab;
        };

        const auto GetWheelContactSpeed = [&](const CVector& contact, size_t groundIdx) {
            auto speed = GetSpeed(contact);
            if (m_aGroundPhysicalPtrs[groundIdx]) {
                speed -= m_aGroundPhysicalPtrs[groundIdx]->GetSpeed(m_aGroundOffsets[groundIdx]);
            }
            return speed;
        };

        // The front wheel
        const auto ProcessFrontWheel = [&](bool decayWithTime) {
            if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) {
                CVector steerDir = GetMatrix().TransformVector(CVector{ -std::sin(m_RideAnimData.BarSteerAngle), std::cos(m_RideAnimData.BarSteerAngle), 0.0f });

                CVector wheelFwd, wheelRight;
                GetWheelAxes(steerDir, frontLine, wheelFwd, wheelRight);

                auto adhesion = GetWheelAdhesion(frontLine, 0.0f) * traction;
                const auto destab = GetDestabilisation(frontLine);
                if (GetStatus() == STATUS_PLAYER) {
                    adhesion *= g_surfaceInfos.GetWetMultiplier(m_aWheelColPoints[frontLine].m_nSurfaceTypeB);
                }
                if (m_nWheelStatus[0] == 1) {
                    adhesion *= 0.4f;
                }

                frontWheelState = m_WheelStates[0];
                auto contactSpeed = GetWheelContactSpeed(frontContact, frontLine);
                auto contact      = frontContact;
                ProcessBikeWheel(wheelFwd, wheelRight, contactSpeed, contact, 2, wheelThrust, brake * frontBrakeShare, adhesion * frontTractionShare, destab, 0, &m_aWheelAngularVelocity[0], &frontWheelState, 0, m_nWheelStatus[0]);
                if ((extraHandlingFlags & 4) && (frontWheelState == WHEEL_STATE_SPINNING || frontWheelState == WHEEL_STATE_SKIDDING)) {
                    frontWheelState = WHEEL_STATE_NORMAL;
                }
            } else {
                m_aWheelAngularVelocity[0] *= 0.95f;
                m_aWheelPitchAngles[0] += decayWithTime ? m_aWheelAngularVelocity[0] * ts : m_aWheelAngularVelocity[0];
            }
        };

        // Whether the original runs the front wheel before or after the rear one depends on the handling
        if (!rearWheelFirst) {
            ProcessFrontWheel(false);
        }

        // The rear wheel
        if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f) {
            if (vehicleFlags.bIsHandbrakeOn) {
                m_aWheelAngularVelocity[1] = 0.0f;
            } else if (acceleration != 0.0f) {
                if (acceleration > 0.0f) {
                    if (m_aWheelAngularVelocity[1] < 1.0f) {
                        m_aWheelAngularVelocity[1] -= 0.1f;
                    }
                } else if (m_aWheelAngularVelocity[1] > -1.0f) {
                    m_aWheelAngularVelocity[1] += 0.05f;
                }
            }
            m_aWheelPitchAngles[1] += ts * m_aWheelAngularVelocity[1];
        } else {
            CVector wheelFwd, wheelRight;
            GetWheelAxes(GetForward(), rearLine, wheelFwd, wheelRight);

            // The rear wheel works on its own copy of the brake and the traction
            auto rearBrake    = brake;
            auto rearTraction = traction;
            if (vehicleFlags.bIsHandbrakeOn) {
                rearBrake   = 20000.0f;
                m_fTyreTemp = 1.0f;
            } else if (m_nBrakesOn) {
                // The original passes the contact point as the force here, so the lever and the push swap places
                const auto side = GetRight() * (m_fSteerAngle * m_fTurnMass * -0.0007f) * ts;
                rearBrake       = 0.0f;
                rearTraction    = 0.0f;
                ApplyTurnForce(contactPoints[2], side);
            } else if (m_fTyreTemp < 1.0f && m_GasPedal > 0.75f) {
                rearTraction    = traction * m_fTyreTemp;
                const auto side = GetRight() * ((1.0f - m_fTyreTemp) * m_fSteerAngle * m_fTurnMass * -0.0007f) * ts;
                ApplyTurnForce(contactPoints[2], side);
            }

            if (wheelThrust > 0.0f && brake > 0.0f) {
                brake = 0.0f;
            }

            auto adhesion = GetWheelAdhesion(rearLine, acceleration) * rearTraction;
            const auto destab = GetDestabilisation(rearLine);
            if (GetStatus() == STATUS_PLAYER) {
                adhesion *= g_surfaceInfos.GetWetMultiplier(m_aWheelColPoints[rearLine].m_nSurfaceTypeB);
            }
            if (m_nWheelStatus[1] == 1) {
                adhesion *= 0.4f;
            }

            rearWheelState = m_WheelStates[1];
            auto contactSpeed = GetWheelContactSpeed(rearContact, rearLine);
            auto contact      = rearContact;
            ProcessBikeWheel(wheelFwd, wheelRight, contactSpeed, contact, 2, wheelThrust, rearBrake * rearBrakeShare, adhesion * rearTractionShare, destab, 1, &m_aWheelAngularVelocity[1], &rearWheelState, 1, m_nWheelStatus[1]);
            if ((extraHandlingFlags & 4) && (rearWheelState == WHEEL_STATE_SPINNING || rearWheelState == WHEEL_STATE_SKIDDING)) {
                rearWheelState = WHEEL_STATE_NORMAL;
            }
        }

        if (m_nBrakesOn && m_WheelStates[1] == WHEEL_STATE_SPINNING) {
            m_fTyreTemp -= ts * 0.002f;
            if (m_fTyreTemp < 0.0f) {
                m_fTyreTemp = 0.0f;
            }
        } else if (m_fTyreTemp < 1.0f) {
            m_fTyreTemp += ts * 0.005f;
        }

        if (rearWheelFirst) {
            ProcessFrontWheel(true);
        }

        m_aGroundPhysicalPtrs.fill(nullptr);

        // How far the rider has leaned forward in the animation
        auto animLean = 0.0f;
        if (m_pDriver) {
            if (auto* const assoc = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), (uint32)ANIM_ID_BIKE_STILL)) {
                animLean = assoc->m_BlendAmount * 0.17453294f;
            }
        }

        if (hitFromSide) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
            auto sideAxis = CrossProduct(GetForward(), m_vecAveGroundNormal);
            sideAxis.Normalise();
            m_vecAveGroundNormal = CrossProduct(sideAxis, GetForward());
            m_vecAveGroundNormal.Normalise();
        }

        float lean;
        if ((extraHandlingFlags & 2) || bikeFlags.bGettingPickedUp) {
            m_vecGroundRight = CrossProduct(GetForward(), m_vecAveGroundNormal);
            m_vecGroundRight.Normalise();

            float lateral;
            if (m_pAttachedTo) {
                lateral = 0.0f;
            } else if (m_nNoOfContactWheels == 0) {
                lateral = m_fSteerAngle / (m_pHandlingData->m_fSteeringLock * 0.017453292f) * ts * -0.004f;
            } else {
                CVector change;
                if (physicalFlags.bDisableCollisionForce) {
                    change = moveSpeedBefore - m_vecOldSpeedForPlayback;
                    m_vecOldSpeedForPlayback = moveSpeedBefore;
                } else {
                    change = m_vecMoveSpeed - moveSpeedBefore;
                }
                lateral = DotProduct(change, m_vecGroundRight);
            }

            auto roll = lateral / (std::max(ts, 0.01f) * 0.008f);

            const auto maxLean = (m_nWheelStatus[0] == 1 ? 0.4f : 1.0f) * m_BikeHandling->m_fMaxLean;
            roll = std::max(-maxLean, std::min(roll, maxLean));

            const auto smoothing = std::pow(m_BikeHandling->m_fDesLean, ts);
            lean = (std::asin(roll) - animLean) * (1.0f - smoothing) + smoothing * m_RideAnimData.DesiredLeanAngle;
        } else if (bikeFlags.bOnSideStand) {
            const auto smoothing = (float)std::pow(0.97, (double)ts);
            lean = smoothing * m_RideAnimData.DesiredLeanAngle - (std::asin(GetRight().z) + animLean + 0.2617994f) * (1.0f - smoothing);
        } else {
            lean = (float)std::pow(0.95, (double)ts) * m_RideAnimData.DesiredLeanAngle;
        }
        m_RideAnimData.DesiredLeanAngle = lean;
        m_RideAnimData.LeanAngle        = lean;

        m_WheelStates[0] = frontWheelState;
        m_WheelStates[1] = rearWheelState;

        if (m_GasPedal < 0.0f && m_WheelStates[1] == WHEEL_STATE_SPINNING) {
            m_WheelStates[1] = WHEEL_STATE_NORMAL;
        }

        if (GetStatus() == STATUS_PLAYER) {
            ProcessSirenAndHorn(true);
        } else if (m_HornCounter) {
            m_HornCounter--;
        }
    }

    // On fire
    if (m_fHealth < 250.0f && GetStatus() != STATUS_WRECKED) {
        if (m_nVehicleSubType != VEHICLE_TYPE_BMX) {
            auto* const modellingMatrix = GetModellingMatrix();
            if (!m_pFireParticle && modellingMatrix) {
                m_pFireParticle = g_fxMan.CreateFxSystem("fire_bike", GetDummyPositionObjSpace(DUMMY_ENGINE), modellingMatrix, false);
                if (m_pFireParticle) {
                    m_pFireParticle->Play();
                    GetEventGlobalGroup()->Add(CEventVehicleOnFire{ this });
                }
            }
        }

        m_BlowUpTimer += (float)(uint32)(int32)(ts * 0.02f * 1000.0f);
        if (m_BlowUpTimer > 5000.0f) {
            BlowUpCar(m_Damager, false);
        }
    } else {
        m_BlowUpTimer = 0.0f;
        FxSystem_c::SafeKillAndClear(m_pFireParticle);
    }

    ProcessDelayedExplosion();

    // Hard landings and rough ground shake the pad of the player
    const auto speedSq   = m_vecMoveSpeed.SquaredMagnitude();
    auto       landing   = 0.0f;
    auto       roughness = 0.0f;
    for (size_t i = 0; i < 4; i++) {
        const auto drop = m_aRatioHistory[i] - m_aWheelRatios[i];
        if (drop > 0.3f && (i == 0 || i == 2) && speedSq > 0.04f && (GetStatus() == STATUS_PLAYER || GetStatus() == STATUS_PHYSICS) && drop > landing) {
            landing = drop;
        }
        if (m_aWheelRatios[i] < 1.0f && GetStatus() == STATUS_PLAYER) {
            roughness = std::max(roughness, (float)g_surfaceInfos.GetRoughness(m_aWheelColPoints[i].m_nSurfaceTypeB) * 0.1f);
        }
        m_aRatioHistory[i] = m_aWheelRatios[i];
        m_aWheelRatios[i]  = 1.0f;
    }
    if ((CTimer::GetTimeInMS() & 0x7FF) > 800) {
        if (roughness >= 0.29f) {
            landing = 0.0f;
        }
        roughness = 0.0f;
    }
    if ((landing > 0.0f || roughness > 0.0f) && GetStatus() == STATUS_PLAYER && speedSq > 0.01f) {
        const auto speed = std::sqrt(speedSq);
        if (landing > 0.0f) {
            const auto strength = (uint8)(int32)std::min(speed / m_fMass * landing * 400000.0f + 100.0f, 250.0f);
            const auto time     = (int32)(ts * 20000.0f / (float)strength);
            CPad::GetPad(0)->StartShake((int16)time, strength, 0);
        } else {
            const auto strength = (uint8)(int32)std::min(speed / m_fMass * roughness * 400000.0f + 40.0f, 150.0f);
            const auto time     = (int32)(ts * 5000.0f / (float)strength);
            CPad::GetPad(0)->StartShake((int16)time, strength, 0);
        }
    }

    vehicleFlags.bVehicleColProcessed = false;
    vehicleFlags.bAudioChangingGear   = false;
    if (!vehicleFlags.bWarnedPeds) {
        CCarCtrl::ScanForPedDanger(this);
    }

    if (physicalFlags.bDisableCollisionForce && physicalFlags.bCollidable) {
        m_vecMoveSpeed         = CVector{};
        m_vecTurnSpeed         = CVector{};
        m_vecFrictionMoveSpeed = CVector{};
        m_vecFrictionTurnSpeed = CVector{};
    } else if (!speedZeroed
               && (m_GasPedal == 0.0f || GetStatus() == STATUS_WRECKED)
               && std::abs(m_vecMoveSpeed.x) < 0.005f && std::abs(m_vecMoveSpeed.y) < 0.005f && std::abs(m_vecMoveSpeed.z) < 0.005f
               && (m_fDamageIntensity <= 0.0f || m_pDamageEntity != FindPlayerPed(-1))) {
        m_vecTurnSpeed.z = 0.0f;
        m_vecMoveSpeed   = CVector{};
    }

    if (!IsHeldUp()) {
        return;
    }

    // Held up: pull the bike towards the way the ground tells it to stand
    const auto groundSide = std::clamp(DotProduct(m_vecAveGroundNormal, GetRight()), -1.0f, 1.0f);
    const auto comOffset  = GetMatrix().TransformVector(m_vecCentreOfMass);
    const auto sidePoint  = comOffset + GetRight();
    if (extraHandlingFlags & 2) {
        ApplyTurnForce(GetUp() * (groundSide * m_fTurnMass * -0.07f) * ts, sidePoint);
        bikeFlags.bOnSideStand = false;
    } else {
        ApplyTurnForce(GetUp() * (groundSide * m_fTurnMass * -0.1f) * ts, sidePoint);
    }

    if (GetStatus() != STATUS_PLAYER) {
        return;
    }

    const auto& forward = GetForward();
    const auto  nose    = comOffset + forward;
    const auto  speedLength = m_vecMoveSpeed.Magnitude();

    // How far the nose is from the wheelie or stoppie angle decides how strongly the bike is held
    if (m_WheelCounts[0] <= 0.0f && m_WheelCounts[1] <= 0.0f && forward.z > 0.0f && (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f)) {
        const auto diff = m_BikeHandling->m_fWheelieAng - forward.z;
        float error;
        if (diff > 0.15f) {
            error = std::max(0.3f - diff, 0.0f);
        } else if (diff >= -0.08f) {
            error = diff;
        } else {
            error = std::min(-0.14f - diff, 0.0f);
        }

        const auto limitedSpeed = std::min(speedLength, 0.1f);
        const auto fat          = CStats::GetFatAndMuscleModifier(STAT_MOD_12);
        const auto holdForce    = fat * (error * m_BikeHandling->m_fWheelieStabMult * limitedSpeed) * m_fTurnMass * ts * 0.5f;
        ApplyTurnForce(GetUp() * holdForce, nose);

        const auto steerForce = m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * m_fTurnMass * ts * 0.5f;
        ApplyTurnForce(GetRight() * steerForce, nose);

        const auto sideForce = speedLength * m_fMass * m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * ts * 0.01f;
        ApplyMoveForce(GetRight() * sideForce * speedLength);

        m_RideAnimData.LeanAngle += ts * m_RideAnimData.BarSteerAngle * -0.1f;
    } else if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f && forward.z < 0.0f && (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f)) {
        const auto diff = m_BikeHandling->m_fStoppieAng - forward.z;
        float error;
        if (diff > 0.15f) {
            error = std::max(0.3f - diff, 0.0f);
        } else if (diff >= -0.15f) {
            error = diff;
        } else {
            error = std::min(-0.3f - diff, 0.0f);
        }

        const auto limitedSpeed = std::min(speedLength, 0.1f);
        const auto fat          = CStats::GetFatAndMuscleModifier(STAT_MOD_12);
        const auto holdForce    = fat * (error * m_BikeHandling->m_fStoppieStabMult * limitedSpeed) * m_fTurnMass * ts * 0.5f;
        ApplyTurnForce(GetUp() * holdForce, nose);

        auto heading = CrossProduct(CVector{ 0.0f, 0.0f, 1.0f }, GetRight());
        heading.Normalise();
        const auto headingForce = DotProduct(m_vecMoveSpeed, GetRight()) * m_fTurnMass * ts * 0.05f;
        ApplyTurnForce(GetRight() * -headingForce, -heading);
    }
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
    if (!m_nFixRightHand && !m_nFixLeftHand) {
        return;
    }

    auto* const chassis = m_aBikeNodes[BIKE_CHASSIS];
    if (!chassis) {
        return;
    }

    // Chassis matrix in world space.
    CMatrix chassisLocal{ RwFrameGetMatrix(chassis), false };
    CMatrix chassisWorld{};
    chassisWorld = GetMatrix();
    chassisWorld *= chassisLocal;

    auto* const hier = GetAnimHierarchyFromSkinClump(rider->GetRpClump());

    // Grip position on the bars, in chassis space.
    CVector gripPos = GetVehicleModelInfo()->m_pVehicleStruct->m_avDummyPos[DUMMY_HAND_REST];
    if (gripPos.IsZero()) {
        switch (m_nModelIndex) {
        case MODEL_MTBIKE: gripPos = CVector{ 0.25f, 0.29f, 0.525f }; break;
        case MODEL_BIKE:   gripPos = CVector{ 0.25f, 0.21f, 0.69f }; break;
        default:           gripPos = CVector{ 0.25f, 0.29f, 0.525f }; break;
        }
    } else {
        // Dummy positions are authored for the hand bone's pose; add that bone's offset, taken into chassis space.
        CMatrix handMat{ &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_R_HAND)], false };
        gripPos += chassisWorld.InverseTransformVector(handMat.TransformVector(CVector{ -0.075f, -0.03f, 0.0f }));
    }

    const auto GetBoneMatrix = [&](int32 boneTag) -> RwMatrix& {
        return RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, boneTag)];
    };

    const auto Move = [](RwMatrix& mat, CVector offset) {
        mat.pos.x += offset.x;
        mat.pos.y += offset.y;
        mat.pos.z += offset.z;
    };

    // Moves the hand to `target`; the rest of the arm follows by the given factors.
    const auto MoveArm = [&](CVector target, eBoneTag hand, eBoneTag finger, eBoneTag finger01, eBoneTag foreArm, float foreArmFactor, eBoneTag upperArm, float upperArmFactor, int32 extraBone) {
        auto&         handMat = GetBoneMatrix(hand);
        const CVector delta   = target - CVector{ handMat.pos.x, handMat.pos.y, handMat.pos.z };

        Move(handMat, delta);
        Move(GetBoneMatrix(finger), delta);
        Move(GetBoneMatrix(finger01), delta);
        Move(GetBoneMatrix(foreArm), delta * foreArmFactor);
        Move(GetBoneMatrix(upperArm), delta * upperArmFactor);
        if (rider->IsPlayer()) {
            Move(GetBoneMatrix(extraBone), delta * upperArmFactor);
        }
    };

    if (m_nFixRightHand) {
        MoveArm(chassisWorld.TransformPoint(gripPos), BONE_R_HAND, BONE_R_FINGER, BONE_R_FINGER_01, BONE_R_FORE_ARM, 0.667f, BONE_R_UPPER_ARM, 0.333f, 0x12D);
    }
    if (m_nFixLeftHand) {
        gripPos.x = -gripPos.x;
        MoveArm(chassisWorld.TransformPoint(gripPos), BONE_L_HAND, BONE_L_FINGER, BONE_L_FINGER_01, BONE_L_FORE_ARM, 0.75f, BONE_L_UPPER_ARM, 0.4f, 0x12E);
    }

    m_nFixLeftHand  = false;
    m_nFixRightHand = false;
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
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    m_vecMoveSpeed.z += 0.13f;
    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), 0x4000);

    const auto pos = GetPosition();
    m_fHealth    = 0.0f;
    m_wBombTimer = 0;

    TheCamera.CamShake(0.4f, pos);
    KillPedsInVehicle();
    m_nOverrideLights      = NO_CAR_LIGHT_OVERRIDE;
    vehicleFlags.bEngineOn = false;
    vehicleFlags.bLightsOn = false;
    ChangeLawEnforcerState(false);

    CExplosion::AddExplosion(this, damager, EXPLOSION_CAR, pos, 0, true, -1.0f, bHideExplosion);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);
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
    CVehicle::PreRender();

    auto* const colModel = GetColModel();
    const auto* const lines = colModel->m_pColData->m_pLines;

    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        // Eases the visual wheel height towards where the suspension line currently puts it
        const auto EaseWheelHeight = [&](size_t wheel, size_t line, float ratioA, float ratioB) {
            const auto restRatio   = 1.0f - m_fSuspensionLength[line] / m_fLineLength[line];
            const auto compression = (std::min(ratioA, ratioB) - restRatio) / (1.0f - restRatio);

            auto height = lines[line].m_vecStart.z;
            if (compression > 0.0f) {
                height -= compression * m_fSuspensionLength[line];
            }
            m_aWheelSuspensionHeights[wheel] = (height - m_aWheelSuspensionHeights[wheel]) * 0.75f + m_aWheelSuspensionHeights[wheel];
        };
        EaseWheelHeight(0, 0, m_aWheelRatios[0], m_aWheelRatios[1]);
        EaseWheelHeight(1, 2, m_aWheelRatios[3], m_aWheelRatios[2]);
    }

    const auto status = GetStatus();
    if (status == STATUS_PHYSICS || status == STATUS_PLAYER || status == STATUS_PLAYER_PLAYBACK_FROM_BUFFER || status == STATUS_SIMPLE) {
        const auto rearWheelSkidding = m_WheelStates[1] == WHEEL_STATE_SKIDDING;
        const auto speed             = m_vecMoveSpeed.Magnitude();

        for (auto wheel = 0; wheel < 2; wheel++) {
            // The ground contact of the wheel, picked from its two suspension lines
            size_t colPoint;
            if (wheel == 0) {
                colPoint = (m_aRatioHistory[0] >= 1.0f && m_aRatioHistory[1] < 1.0f) ? 1 : 0;
            } else {
                colPoint = (m_aRatioHistory[3] >= 1.0f && m_aRatioHistory[2] < 1.0f) ? 2 : 3;
            }

            uint32 particleFlags = (wheel == 0 && !rearWheelSkidding) ? 4 : 0;
            if (m_bWheelBloody[wheel]) {
                particleFlags += 1;
            }
            if (m_bMoreSkidMarks[wheel]) {
                particleFlags += 2;
            }

            const auto sideOffset = std::sin(m_RideAnimData.LeanAngle) * GetColModel()->GetBoundingBox().m_vecMin.z * 0.8f;
            auto       wheelPos   = m_aWheelColPoints[colPoint].m_vecPoint + GetRight() * sideOffset;

            AddSingleWheelParticles(
                m_WheelStates[wheel],
                m_nWheelStatus[wheel],
                m_aRatioHistory[colPoint],
                speed,
                &m_aWheelColPoints[colPoint],
                &wheelPos,
                m_RideAnimData.LeanAngle <= 0.0f ? 1.0f : -1.0f,
                wheel,
                (uint32)m_aWheelSkidmarkType[wheel],
                &m_bWheelBloody[wheel],
                particleFlags
            );
        }
    }

    m_bLeanMatrixCalculated = false;
    CalculateLeanMatrix();

    // Game speed to km/h
    constexpr auto SPEED_TO_KMH_DIVISOR = 0.27777800f / 50.0f;
    const auto     speedKmh             = DotProduct(GetForward(), m_vecMoveSpeed) / SPEED_TO_KMH_DIVISOR;
    if (vehicleFlags.bEngineOn && !m_pHandlingData->m_bNoExhaust && speedKmh < 130.0f && !vehicleFlags.bIsDrowning) {
        AddExhaustParticles();
    }

    AddDamagedVehicleParticles();

    // The siren of the police bike swaps between a red and a blue light
    if (m_nModelIndex == MODEL_COPBIKE && vehicleFlags.bSirenOrAlarm && vehicleFlags.bEngineOn && !physicalFlags.bRenderScorched) {
        const auto phase      = CTimer::GetTimeInMS() & 0x1FF;
        const auto brightness = (uint8)(int32)(CTimeCycle::m_CurrentColours.m_fSpriteBrightness * 25.5f);

        uint8 lightMain = 0xFF;
        uint8 pulse;
        if (phase < 0x100) {
            pulse = (uint8)phase;
            lightMain -= pulse;

            const CVector coronaPos{ -0.28f, 0.6f, 0.3f };
            CCoronas::RegisterCorona(
                (uint32)(uintptr_t)this + 0x15, this, brightness, 0, 0, 0xFF, coronaPos, 0.4f, 40.0f,
                CORONATYPE_SHINYSTAR, FLARETYPE_NONE, CORREFL_NONE, LOSCHECK_OFF, TRAIL_OFF, 0.0f, false, 1.5f, false, 30.0f, false, true
            );
        } else {
            pulse = (uint8)(-(int32)(phase & 0xFF));
            lightMain -= pulse;

            const CVector coronaPos{ 0.28f, 0.6f, 0.3f };
            CCoronas::RegisterCorona(
                (uint32)(uintptr_t)this + 0x16, this, 0, 0, brightness, 0xFF, coronaPos, 0.4f, 40.0f,
                CORONATYPE_SHINYSTAR, FLARETYPE_NONE, CORREFL_NONE, LOSCHECK_OFF, TRAIL_OFF, 0.0f, false, 1.5f, false, 30.0f, false, true
            );
        }

        const auto lightPos = GetPosition() + GetForward() + GetUp() * 0.5f;
        CPointLights::AddLight(
            PLTYPE_POINTLIGHT,
            lightPos,
            CVector{},
            10.0f,
            (float)pulse / 1024.0f,
            0.0f,
            (float)lightMain / 1024.0f,
            0,
            true,
            nullptr
        );
    }

    DoVehicleLights(m_mLeanMatrix, VEHICLE_LIGHTS_IGNORE_DAMAGE);
    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    auto* const mi = GetVehicleModelInfo();

    // Spin of the wheels from the speed of their contact point
    const auto steeredForward = GetMatrix().TransformVector(CVector{ -std::sin(m_fSteerAngle), std::cos(m_fSteerAngle), 0.0f });
    const auto forward        = GetForward();

    if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) {
        const CVector contact{
            0.0f,
            (lines[1].m_vecStart.y + lines[0].m_vecStart.y) * 0.5f,
            lines[0].m_vecStart.z - std::min(m_aRatioHistory[0], m_aRatioHistory[1]) * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        const auto spin = ProcessWheelRotation(WHEEL_STATE_NORMAL, steeredForward, GetSpeed(contact), mi->m_fWheelSizeFront * 0.5f);
        m_aWheelAngularVelocity[0] = spin;
        m_aWheelPitchAngles[0] += spin * CTimer::GetTimeStep();
    }
    if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) {
        const CVector contact{
            0.0f,
            (lines[3].m_vecStart.y + lines[2].m_vecStart.y) * 0.5f,
            lines[2].m_vecStart.z - std::min(m_aRatioHistory[2], m_aRatioHistory[3]) * m_fSuspensionLength[2] - mi->m_fWheelSizeRear * 0.5f
        };
        const auto spin = ProcessWheelRotation(m_WheelStates[1], forward, GetSpeed(contact), mi->m_fWheelSizeRear * 0.5f);
        m_aWheelAngularVelocity[1] = spin;
        m_aWheelPitchAngles[1] += spin * CTimer::GetTimeStep();
    }

    // The frames are moved on top of what the model has, so every one starts from its own matrix
    const auto AttachFrame = [](CMatrix& mat, RwFrame* frame) { mat.Attach(RwFrameGetMatrix(frame), false); };

    if (auto* const forksFront = m_aBikeNodes[BIKE_FORKS_FRONT]) {
        CMatrix mat{};
        AttachFrame(mat, forksFront);
        const auto savedPos = mat.GetPosition();

        // Rotation of the steering, around the axis the forks are leaning on
        RwMatrix rwRot{};
        CMatrix  rot{};
        rot.Attach(&rwRot, false);
        rot.SetUnity();
        rot.UpdateRW();

        const auto steerRad = mi->m_fBikeSteerAngle * 0.017453292f;
        CVector    axisDir{ 0.0f, std::sin(steerRad), -std::cos(steerRad) };
        axisDir.Normalise();
        RwV3d axis{ axisDir.x, axisDir.y, axisDir.z };

        CQuaternion steerQuat;
        steerQuat.Set(&axis, -m_RideAnimData.BarSteerAngle);
        steerQuat.Get(&rwRot);
        rot.Update();

        mat.SetUnity();
        mat = mat * rot;
        mat.GetPosition() += savedPos;
        mat.UpdateRW();

        if (auto* const handlebars = m_aBikeNodes[BIKE_HANDLEBARS]) {
            CMatrix handlebarsMat{};
            AttachFrame(handlebarsMat, handlebars);
            const auto handlebarsPos = handlebarsMat.GetPosition();

            const auto st = GetStatus();
            if (st == STATUS_ABANDONED || st == STATUS_WRECKED) {
                handlebarsMat.SetUnity();
                handlebarsMat *= rot;
                handlebarsMat.GetPosition() += handlebarsPos;
            } else {
                handlebarsMat.SetTranslate(handlebarsPos);
            }
            handlebarsMat.UpdateRW();
        }
    }

    if (auto* const forksRear = m_aBikeNodes[BIKE_FORKS_REAR]) {
        // Swing arm, tilted by how far the rear wheel sits from its rest height
        const auto swingAngle = -std::asin((m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength);

        CMatrix mat{};
        AttachFrame(mat, forksRear);
        const auto savedPos = mat.GetPosition();
        mat.SetRotate(swingAngle, 0.0f, 0.0f);
        mat.GetPosition() += savedPos;
        mat.UpdateRW();
    }

    CVector frontWheelPos;
    {
        CMatrix mat{};
        AttachFrame(mat, m_aBikeNodes[BIKE_WHEEL_FRONT]);
        frontWheelPos = mat.GetPosition();

        frontWheelPos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
        frontWheelPos.y = (lines[1].m_vecStart.y + lines[0].m_vecStart.y) * 0.5f - m_fForkYOffset
            - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;

        if (m_nWheelStatus[0] == 1) {
            mat.SetRotate(m_aWheelPitchAngles[0], 0.0f, std::sin(m_aWheelPitchAngles[0]) * 0.05f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[0]);
        }
        mat.GetPosition() += frontWheelPos;
        mat.UpdateRW();
    }

    {
        // The mudguard follows the front wheel
        CMatrix mat{};
        AttachFrame(mat, m_aBikeNodes[BIKE_MUDGUARD]);
        mat.GetPosition() = frontWheelPos;
        mat.UpdateRW();
    }

    {
        CMatrix mat{};
        AttachFrame(mat, m_aBikeNodes[BIKE_WHEEL_REAR]);
        const auto savedPos = mat.GetPosition();

        if (m_nWheelStatus[1] == 1) {
            mat.SetRotate(m_aWheelPitchAngles[1], 0.0f, std::sin(m_aWheelPitchAngles[1]) * 0.07f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[1]);
        }
        mat.GetPosition() += savedPos;
        mat.UpdateRW();
    }

    if (auto* const chassis = m_aBikeNodes[BIKE_CHASSIS]) {
        CMatrix mat{};
        AttachFrame(mat, chassis);

        // The chassis tilts with the lean and sinks a little into it
        auto pos = mat.GetPosition();
        pos.z = (1.0f - std::cos(m_RideAnimData.LeanAngle)) * colModel->GetBoundingBox().m_vecMin.z * 0.9f;

        mat.SetRotateX(std::abs(m_RideAnimData.LeanAngle) * -0.05f);
        mat.RotateY(m_RideAnimData.LeanAngle);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }
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
        CVehicle::DoHeadLightBeam(eVehicleLightId::MAIN, m_mLeanMatrix, true);
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
    // Lines that were never set up hold this value in `start.z`.
    constexpr float UNSET_LINE_Z = 99999.992f; // 0x47C34FFF

    auto& mi = *GetVehicleModelInfo();
    auto& cm = *mi.GetColModel();
    auto& cd = *cm.m_pColData;
    const auto& handling = *m_pHandlingData;

    const bool linesAreSetUp     = cd.m_pLines[0].m_vecStart.z != UNSET_LINE_Z;
    const bool linesWereNotSetUp = cd.m_pLines[1].m_vecStart.z == UNSET_LINE_Z; // The original checks the second line here.

    // Position of `frame` with its ancestors' matrices applied, stopping before `stopFrame`.
    const auto GetFramePosn = [](RwFrame* frame, RwFrame* stopFrame) {
        RwMatrix mat = *RwFrameGetMatrix(frame);
        for (auto* parent = RwFrameGetParent(frame); parent;) {
            RwMatrixTransform(&mat, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
            parent = RwFrameGetParent(parent);
            if (parent == stopFrame) {
                break;
            }
        }
        return mat.pos;
    };
    auto* const rootFrame = RpClumpGetFrame(GetRpClump());

    for (auto i = 0; i < NUM_SUSP_LINES; i++) {
        const auto isFront   = i < 2;
        const auto wheelSize = isFront ? mi.m_fWheelSizeFront : mi.m_fWheelSizeRear;

        CVector pos;
        if (linesAreSetUp) {
            pos = CVector{ cd.m_pLines[i].m_vecStart.x, cd.m_pLines[i].m_vecStart.y, m_aWheelOrigHeights[isFront ? 0 : 1] };
        } else {
            pos = GetFramePosn(m_aBikeNodes[isFront ? BIKE_WHEEL_FRONT : BIKE_WHEEL_REAR], rootFrame);

            if (i == 0) {
                m_aWheelOrigHeights[0] = pos.z;
            } else if (i == 2) {
                m_aWheelOrigHeights[1] = pos.z;
                if (auto* const forkRear = m_aBikeNodes[BIKE_FORKS_REAR]) {
                    const auto forkPos = GetFramePosn(forkRear, rootFrame);
                    m_fSwingArmLength  = std::hypot(pos.y - forkPos.y, pos.z - forkPos.z);
                } else {
                    m_fSwingArmLength = 0.0f;
                }
            }

            // Offset the line to the front or rear of the wheel.
            pos.y += (i % 2 == 0 ? 0.25f : -0.25f) * wheelSize;
        }
        pos.z += handling.m_fSuspensionUpperLimit;

        auto& line      = cd.m_pLines[i];
        line.m_vecStart = pos;
        line.m_vecEnd   = CVector{ pos.x, pos.y, ((handling.m_fSuspensionLowerLimit - handling.m_fSuspensionUpperLimit) - wheelSize * 0.5f) + pos.z };

        m_fSuspensionLength[i] = handling.m_fSuspensionUpperLimit - handling.m_fSuspensionLowerLimit;
        m_fLineLength[i]       = line.m_vecStart.z - line.m_vecEnd.z;
    }

    if (!linesAreSetUp) {
        // Unlike the wheels, this walks all the way up; the root frame is included.
        const auto forkPos = GetFramePosn(m_aBikeNodes[BIKE_FORKS_FRONT], nullptr);
        m_fForkYOffset     = forkPos.y;
        m_fForkZOffset     = forkPos.z;
    }

    m_fHeightAboveRoad = (mi.m_fWheelSizeFront * 0.5f - cd.m_pLines[0].m_vecStart.z)
        + (1.0f - 1.0f / (handling.m_fSuspensionForceLevel * 4.0f)) * m_fSuspensionLength[0];

    m_aWheelSuspensionHeights[0] = mi.m_fWheelSizeFront * 0.5f - m_fHeightAboveRoad;
    m_aWheelSuspensionHeights[1] = mi.m_fWheelSizeRear * 0.5f - m_fHeightAboveRoad;

    if (cd.m_pLines[0].m_vecEnd.z < cm.m_boundBox.m_vecMin.z) {
        cm.m_boundBox.m_vecMin.z = cd.m_pLines[0].m_vecEnd.z;
    }
    cm.m_boundSphere.m_fRadius = std::max({ cm.m_boundSphere.m_fRadius, cm.m_boundBox.m_vecMin.Magnitude(), cm.m_boundBox.m_vecMax.Magnitude() });

    if (handling.m_bForceGroundClearance && linesWereNotSetUp) {
        const auto sphereBottomMinZ = 0.25f - m_fHeightAboveRoad;
        for (auto&& sph : cd.GetSpheres()) {
            if (sph.m_vecCenter.z - sph.m_fRadius < sphereBottomMinZ) {
                if (sph.m_fRadius > 0.4f) {
                    sph.m_fRadius = std::max(0.4f, sph.m_vecCenter.z - sphereBottomMinZ);
                }
                sph.m_vecCenter.z = sphereBottomMinZ + sph.m_fRadius;
            }
        }
    }
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
    auto* mi = GetVehicleModelInfo();
    const auto speedToFwdRatio = std::fabs(DotProduct(m_vecMoveSpeed, GetForward()));

    std::array<bool, NUM_SUSP_LINES> lineEligibleForSandSoften{ true, true, true, true };

    for (auto wheel = 0; wheel < 2; wheel++) {
        const auto line0 = wheel == 0 ? 0 : 2;
        const auto line1 = wheel == 0 ? 1 : 3;

        switch (m_nWheelStatus[wheel]) {
        case WHEEL_STATUS_MISSING:
            m_aWheelRatios[line0] = 1.0f;
            m_aWheelRatios[line1] = 1.0f;
            lineEligibleForSandSoften[line0] = false;
            lineEligibleForSandSoften[line1] = false;
            break;

        case WHEEL_STATUS_BURST: {
            const auto val = CGeneral::GetRandomNumberInRange(0, int32(speedToFwdRatio * 40.0f) + 98);
            if (val < 100) {
                const auto remaining0 = (m_fLineLength[line0] - m_fSuspensionLength[line0]) / m_fLineLength[line0];
                const auto delta = remaining0 * 0.2f;
                m_aWheelRatios[line0] = std::min(1.0f, m_aWheelRatios[line0] + delta);
                m_aWheelRatios[line1] = std::min(1.0f, m_aWheelRatios[line1] + delta);
            }
            lineEligibleForSandSoften[line0] = false;
            lineEligibleForSandSoften[line1] = false;
            break;
        }

        default: // WHEEL_STATUS_OK
            if ((m_aWheelRatios[line0] < 1.0f && m_aWheelColPoints[line0].m_nSurfaceTypeB == SURFACE_RAILTRACK) ||
                (m_aWheelRatios[line1] < 1.0f && m_aWheelColPoints[line1].m_nSurfaceTypeB == SURFACE_RAILTRACK)) {

                const auto wheelSize = wheel == 0 ? mi->m_fWheelSizeFront : mi->m_fWheelSizeRear;
                auto wheelSizeFactor = 1.5f / (wheelSize * 0.5f);
                if (speedToFwdRatio > 0.3f) {
                    wheelSizeFactor *= speedToFwdRatio / 0.3f;
                }
                const auto wheelSizeInv = 1.0f / wheelSizeFactor;

                const auto wheelRotFactor = wheelSizeInv * m_aWheelPitchAngles[wheel];
                const auto wheelRotFactorFract = wheelRotFactor - std::floor(wheelRotFactor);

                const auto timeSpeedRotFactor = (CTimer::GetTimeStep() * m_aWheelAngularVelocity[wheel] + m_aWheelPitchAngles[wheel]) * wheelSizeInv;
                const auto timeSpeedRotFactorFract = timeSpeedRotFactor - std::floor(timeSpeedRotFactor);

                if ((m_aWheelAngularVelocity[wheel] > 0.0f && timeSpeedRotFactorFract < wheelRotFactorFract) ||
                    (m_aWheelAngularVelocity[wheel] < 0.0f && timeSpeedRotFactorFract > wheelRotFactorFract)) {
                    const auto remaining0 = (m_fLineLength[line0] - m_fSuspensionLength[line0]) / m_fLineLength[line0];
                    const auto delta = remaining0 * 0.3f;
                    m_aWheelRatios[line0] = std::max(0.2f, m_aWheelRatios[line0] - delta);
                    m_aWheelRatios[line1] = std::max(0.2f, m_aWheelRatios[line1] - delta);
                }
                lineEligibleForSandSoften[line0] = false;
                lineEligibleForSandSoften[line1] = false;
            }
            break;
        }
    }

    // Second pass: soften ratios for lines resting on sand
    for (auto line = 0; line < NUM_SUSP_LINES; line++) {
        if (!lineEligibleForSandSoften[line]) continue;
        if (m_aWheelRatios[line] >= 1.0f) continue;
        if (g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[line].m_nSurfaceTypeB) != ADHESION_GROUP_SAND) continue;
        if (m_nModelIndex == MODEL_RHINO) continue; // NOTSA: dead for CBike - copied verbatim from the shared/original code, kept for fidelity

        float offroadFactor;
        if (handlingFlags.bOffroadAbility2) {
            offroadFactor = 0.1f;
        } else if (handlingFlags.bOffroadAbility) {
            offroadFactor = 0.15f;
        } else {
            offroadFactor = 0.25f;
        }

        auto adhesionFactor = 1.0f - (speedToFwdRatio / 0.3f) * 0.7f - CWeather::WetRoads * 0.7f;
        adhesionFactor = std::max(0.4f, adhesionFactor);

        const auto remaining = (m_fLineLength[line] - m_fSuspensionLength[line]) / m_fLineLength[line];
        m_aWheelRatios[line] = std::min(1.0f, m_aWheelRatios[line] + offroadFactor * remaining * adhesionFactor);
    }
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

