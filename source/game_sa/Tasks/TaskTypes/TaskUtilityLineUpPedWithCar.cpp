#include "StdInc.h"

#include "TaskUtilityLineUpPedWithCar.h"

#include "PedPlacement.h"
#include "VehicleAnimGroupData.h"
#include "rtslerp.h"

void CTaskUtilityLineUpPedWithCar::InjectHooks() {
    RH_ScopedClass(CTaskUtilityLineUpPedWithCar);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x64FBB0);
    RH_ScopedInstall(Destructor, 0x64FC00);

    RH_ScopedInstall(GetLocalPositionToOpenCarDoor, 0x64FC10);
    RH_ScopedInstall(GetPositionToOpenCarDoor, 0x650A80);
    RH_ScopedInstall(ProcessPed, 0x6513A0);
}

// 0x64FBB0
CTaskUtilityLineUpPedWithCar::CTaskUtilityLineUpPedWithCar(const CVector& offset, int32 time, int32 doorOpenPosType, int32 doorIdx) {
    m_Offset = offset;
    m_fDoorOpenPosZ = -999.99f;
    m_fTime = time;
    m_nDoorOpenPosType = doorOpenPosType;
    m_nDoorIdx = doorIdx;
}

// The following 2 functions seem to have copy elision on the returned CVector, that the compiled functions
// took a vector ptr as their first arg. Now, hopefully our code will compile to the same stuff.
// If not, it might crash here, in that case a wrapper function should be used.

// 0x64FC10
CVector CTaskUtilityLineUpPedWithCar::GetLocalPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    // 8 is the front right door, 9 the rear right, 10 the front left, 11 the rear left and 18 is the one for the bikes
    static constexpr auto DOOR_RF = 8, DOOR_RR = 9, DOOR_LF = 10, DOOR_LR = 11, DOOR_BIKE = 0x12;

    auto* const modelInfo = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->AsVehicleModelInfoPtr();
    auto* const handling  = vehicle->m_pHandlingData;

    // How far the door is open, the rear doors of the vans are not moved
    const auto doorOpen = vehicle->vehicleFlags.bIsVan && (m_nDoorIdx == DOOR_LR || m_nDoorIdx == DOOR_RR)
        ? 0.0f
        : animProgress * handling->m_fSeatOffsetDistance;

    // Which offset of the animation group, depending on the animation and the door
    auto offsetType = [&]() -> int32 {
        if (!assoc) {
            return 0;
        }
        const auto id = assoc->GetAnimId();
        if (id >= ANIM_ID_CAR_ALIGN_LHS && id <= ANIM_ID_CAR_SHUFFLE_RHS_1) {
            return 0;
        }
        if (id >= ANIM_ID_CAR_GETOUT_LHS_0 && id <= ANIM_ID_CAR_GETOUT_RHS_1) {
            return 3;
        }
        if (id == ANIM_ID_CAR_JACKEDLHS || id == ANIM_ID_CAR_JACKEDRHS) {
            return 5;
        }
        if ((id >= ANIM_ID_CAR_CLOSE_LHS_0 && id <= ANIM_ID_CAR_CLOSE_RHS_1)
            || id == ANIM_ID_CAR_FALLOUT_LHS || id == ANIM_ID_CAR_FALLOUT_RHS
            || id == ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0 || id == ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1) {
            return 3;
        }
        return 0; // The rest of them are not used for this
    }();
    if (offsetType == 0 || offsetType == 3) {
        switch (m_nDoorIdx) {
        case DOOR_LF:
        case DOOR_RF:   offsetType = 0; break;
        case DOOR_LR:
        case DOOR_RR:   offsetType = 1; break;
        case DOOR_BIKE: offsetType = 2; break;
        default:        break;
        }
    } else if (offsetType == 5) {
        offsetType = 0;
    }

    // The offset is mixed with the one of the first group while the car is being jacked and the door is locked
    auto&      animGroup = CVehicleAnimGroupData::m_vehicleAnimGroups[handling->m_nAnimGroup];
    CVector    offset;
    if (assoc && (assoc->GetAnimId() == ANIM_ID_CAR_DOORLOCKED_LHS || assoc->GetAnimId() == ANIM_ID_CAR_DOORLOCKED_RHS) && animGroup.m_animFlags.bCanPulloutPed) {
        const auto blend = assoc->m_BlendAmount;
        const auto own   = animGroup.ComputeAnimDoorOffsets((eVehAnimDoorOffset)offsetType) * (1.0f - blend);
        const auto first = CVehicleAnimGroupData::m_vehicleAnimGroups[0].ComputeAnimDoorOffsets((eVehAnimDoorOffset)offsetType) * blend;
        offset = CVector{ first.x + own.x, first.y + own.y, first.z + own.z };
    } else {
        offset = animGroup.ComputeAnimDoorOffsets((eVehAnimDoorOffset)offsetType);
    }

    // And the position of the door itself
    const auto& frontSeat = modelInfo->GetFrontSeatPosn();
    const auto& rearSeat  = modelInfo->GetModelDummyPosition(DUMMY_SEAT_REAR);
    CVector     result;
    CVector     base;
    switch (m_nDoorIdx) {
    case DOOR_RF:
    case DOOR_RR: {
        const auto& seat = m_nDoorIdx == DOOR_RF ? frontSeat : rearSeat;
        base     = CVector{ seat.x + doorOpen, seat.y, seat.z };
        offset.x = -offset.x;
        result   = CVector{ base.x - offset.x, seat.y - offset.y, seat.z - offset.z };
        break;
    }
    case DOOR_LF:
    case DOOR_LR: {
        const auto& seat = m_nDoorIdx == DOOR_LF ? frontSeat : rearSeat;
        base   = CVector{ -(seat.x + doorOpen), seat.y, seat.z };
        result = CVector{ base.x - offset.x, seat.y - offset.y, seat.z - offset.z };
        break;
    }
    case DOOR_BIKE:
        base   = frontSeat;
        result = CVector{ offset.x + frontSeat.x, frontSeat.y + offset.y, frontSeat.z + offset.z };
        break;
    default:
        base   = frontSeat;
        offset = CVector{ 0.0f, 0.0f, 0.0f };
        result = frontSeat;
        break;
    }

    // The bikes use the position of the door in the world
    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && m_nDoorIdx != DOOR_BIKE) {
        CVector world;
        vehicle->AsBike()->GetCorrectedWorldDoorPosition(world, offset, base);
        return vehicle->GetMatrix().InverseTransformVector(world - vehicle->GetPosition());
    }
    return result;
}

// 0x650A80
CVector CTaskUtilityLineUpPedWithCar::GetPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    return vehicle->GetMatrix().TransformPoint(GetLocalPositionToOpenCarDoor(vehicle, animProgress, assoc));
}

// 0x6513A0
bool CTaskUtilityLineUpPedWithCar::ProcessPed(CPed* ped, CVehicle* vehicle, CAnimBlendAssociation* assoc) {
    static constexpr auto DOOR_RF = 8, DOOR_RR = 9, DOOR_BIKE = 0x12;
    static constexpr auto DOOR_TYPE_NONE = 0, DOOR_TYPE_FALL = 1, DOOR_TYPE_KEEP_POSN = 2;

    if (m_nDoorOpenPosType == DOOR_TYPE_NONE) {
        ped->m_vecMoveSpeed = CVector{};
    }

    // Where the ped is looking at
    auto       isCarUpsideDown = false;
    const auto heading         = vehicle->GetHeading();
    if (vehicle->GetMatrix().GetUp().z <= -0.8f) {
        isCarUpsideDown = true;
        ped->m_fAimingRotation = m_nDoorIdx == DOOR_RF || m_nDoorIdx == DOOR_RR ? heading - PI : heading;
    } else if (m_nDoorIdx == DOOR_BIKE) {
        ped->m_fAimingRotation = heading + PI;
    } else if (m_nDoorOpenPosType != DOOR_TYPE_KEEP_POSN) {
        ped->m_fAimingRotation = heading;
    }

    auto* const handling = vehicle->m_pHandlingData;
    auto&       animGroup = CVehicleAnimGroupData::GetVehicleAnimGroup(handling->m_nAnimGroup);

    // How far the animation is, and how much of it counts for the position of the ped
    const auto AnimTime = [&] { return assoc->m_CurrentTime / assoc->m_BlendHier->GetTotalTime(); };

    auto progress = 0.0f; // Where the ped is between the places of the door
    auto blend    = 0.0f; // How much of the movement is done in the height
    if (assoc) {
        const auto id        = assoc->GetAnimId();
        const auto criticalT = [&] { return animGroup.ComputeCriticalBlendTime(id); };

        switch (id) {
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1:
            progress = AnimTime();
            break;
        case ANIM_ID_CAR_ALIGN_LHS:
        case ANIM_ID_CAR_ALIGN_RHS:
        case ANIM_ID_CAR_ALIGNHI_LHS:
        case ANIM_ID_CAR_ALIGNHI_RHS: {
            progress     = 1.0f;
            const auto c = criticalT();
            if (!(std::abs(c) < 10.0f)) {
                const auto limit = std::abs(c) - 11.0f;
                const auto t     = AnimTime();
                if (c - 11.0f <= 0.0f) {
                    blend = t >= limit ? (t - limit) / (1.0f - limit) : 0.0f;
                } else {
                    blend = t >= limit ? 1.0f : t / limit;
                }
            }
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                blend = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_OPEN_LHS:
        case ANIM_ID_CAR_OPEN_RHS:
        case ANIM_ID_CAR_OPEN_LHS_1:
        case ANIM_ID_CAR_OPEN_RHS_1:
            progress = 1.0f;
            blend    = std::abs(criticalT()) < 10.0f ? 0.0f : 1.0f;
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                blend = 1.0f;
            }
            break;
        case ANIM_ID_CAR_GETIN_LHS_0:
        case ANIM_ID_CAR_GETIN_RHS_0:
        case ANIM_ID_CAR_GETIN_LHS_1:
        case ANIM_ID_CAR_GETIN_RHS_1:
        case ANIM_ID_CAR_GETIN_BIKE_FRONT: {
            progress     = 1.0f - AnimTime();
            const auto c = criticalT();
            if (!(std::abs(c) < 10.0f)) {
                blend = 1.0f;
            } else {
                const auto limit = std::abs(c);
                const auto t     = AnimTime();
                if (c <= 0.0f) {
                    blend = t >= limit ? (t - limit) / (1.0f - limit) : 0.0f;
                } else {
                    blend = t >= limit ? 1.0f : t / limit;
                }
            }
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                blend = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_PULLOUT_LHS:
        case ANIM_ID_CAR_PULLOUT_RHS:
        case ANIM_ID_UNKNOWN_15:
        case ANIM_ID_CAR_CLOSE_LHS_0:
        case ANIM_ID_CAR_CLOSE_RHS_0:
        case ANIM_ID_CAR_CLOSE_LHS_1:
        case ANIM_ID_CAR_CLOSE_RHS_1:
            progress = 1.0f;
            break;
        case ANIM_ID_CAR_CLOSEDOOR_LHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_LHS_1:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_1:
        case ANIM_ID_CAR_SHUFFLE_RHS_0:
        case ANIM_ID_CAR_SHUFFLE_RHS_1:
        case ANIM_ID_CAR_ROLLDOOR:
            blend = 1.0f;
            break;
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_CAR_JACKEDLHS:
        case ANIM_ID_CAR_JACKEDRHS:
        case ANIM_ID_CAR_ROLLOUT_LHS:
        case ANIM_ID_CAR_ROLLOUT_RHS:
        case ANIM_ID_CAR_FALLOUT_LHS:
        case ANIM_ID_CAR_FALLOUT_RHS: {
            progress         = AnimTime();
            const auto c     = criticalT();
            const auto limit = std::abs(c);
            const auto t     = AnimTime();
            if (c <= 0.0f) {
                blend = t >= limit ? 1.0f - (t - limit) / (1.0f - limit) : 1.0f;
            } else {
                blend = t >= limit ? 0.0f : 1.0f - t / limit;
            }
            break;
        }
        case ANIM_ID_CAR_DOORLOCKED_LHS:
        case ANIM_ID_CAR_DOORLOCKED_RHS:
            progress = 1.0f;
            blend    = animGroup.m_specialFlags.bRunSpecialLockedDoor ? 1.0f : 0.0f;
            break;
        default:
            break;
        }
    }

    // Where the ped has to be, now, and when the door is all the way open
    CVector targetNow = m_nDoorOpenPosType == DOOR_TYPE_KEEP_POSN
        ? ped->GetPosition()
        : GetPositionToOpenCarDoor(vehicle, progress, assoc);
    CVector targetOpen = targetNow;
    if (vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE && m_nDoorOpenPosType != DOOR_TYPE_KEEP_POSN) {
        targetOpen = GetPositionToOpenCarDoor(vehicle, 1.0f, assoc);
    }

    // The height of the door is looked for in the ground, unless the car is in the water
    if (!vehicle->physicalFlags.bSubmergedInWater) {
        const auto right  = vehicle->m_matrix
            ? vehicle->m_matrix->GetRight()
            : CVector{ std::cos(vehicle->m_placement.m_fHeading), std::sin(vehicle->m_placement.m_fHeading), 0.0f };
        const auto carPos = vehicle->GetPosition();
        const auto side   = DotProduct(targetOpen - carPos, right);
        const auto height = ((carPos.z - vehicle->GetHeightAboveRoad()) + right.z * side) + 1.0f;

        const auto doorZ = targetOpen.z;
        targetOpen       = CPedPlacement::FindZCoorForPed(targetOpen).first;
        if (!(targetOpen.z >= height - 0.5f)) { // The ground that is found is under the door
            targetOpen.z = doorZ;
        }
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT && vehicle->IsUpsideDown()) {
        targetOpen.z += 1.0f;
    }
    m_fDoorOpenPosZ = targetOpen.z;

    if (m_nDoorOpenPosType == DOOR_TYPE_FALL || m_nDoorOpenPosType == DOOR_TYPE_KEEP_POSN) {
        // The ped falls until it is where the door is
        const auto fallSpeed = ped->m_vecMoveSpeed.z - CTimer::GetTimeStep() * 0.008f;
        if (fallSpeed + ped->GetPosition().z >= targetOpen.z) {
            ped->m_vecMoveSpeed.z = fallSpeed;
            targetNow.z           = ped->GetPosition().z;
        } else {
            targetNow.z    = targetOpen.z;
            ped->m_vecMoveSpeed = CVector{};
        }
    }

    // How high it is, when the door opens
    if (m_fDoorOpenPosZ <= targetNow.z) {
        if (m_nDoorOpenPosType == DOOR_TYPE_NONE) {
            targetNow.z = (targetNow.z - m_fDoorOpenPosZ) * blend + m_fDoorOpenPosZ;
        }
    } else {
        float newZ;
        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && assoc && assoc->GetAnimId() != ANIM_ID_CAR_GETIN_BIKE_FRONT) {
            float bikeBlend;
            switch (assoc->GetAnimId()) {
            case ANIM_ID_CAR_GETOUT_LHS_0:
            case ANIM_ID_CAR_GETOUT_RHS_0:
            case ANIM_ID_CAR_GETOUT_LHS_1:
            case ANIM_ID_CAR_GETOUT_RHS_1:
                bikeBlend = 1.0f - progress;
                break;
            case ANIM_ID_CAR_GETIN_LHS_0:
            case ANIM_ID_CAR_GETIN_RHS_0:
            case ANIM_ID_CAR_GETIN_LHS_1:
            case ANIM_ID_CAR_GETIN_RHS_1:
                bikeBlend = std::min(AnimTime() + AnimTime(), 1.0f);
                break;
            default:
                bikeBlend = 0.0f;
                break;
            }
            newZ = (((vehicle->GetPosition().z - vehicle->GetHeightAboveRoad()) + 1.0f) - m_fDoorOpenPosZ) * bikeBlend;
        } else {
            newZ = (targetNow.z - m_fDoorOpenPosZ) * blend;
        }
        targetNow.z = newZ + m_fDoorOpenPosZ;
    }

    // The ped turns while there is time left
    if (CTimer::GetTimeInMS() < (uint32)m_fTime) {
        auto       turnTo  = CGeneral::LimitRadianAngle(ped->m_fAimingRotation);
        const auto current = ped->m_fCurrentRotation;

        const auto timeLeft = (float)((uint32)m_fTime - CTimer::GetTimeInMS());
        const auto left     = timeLeft * 0.0016666667f;
        if (left <= 0.0f) {
            m_Offset.x = 0.0f;
            m_Offset.y = 0.0f;
        }
        m_Offset.z = 0.0f;
        targetNow.x -= left * m_Offset.x;
        targetNow.y -= left * m_Offset.y;

        if (turnTo <= current + PI) {
            if (turnTo < current - PI) {
                turnTo += TWO_PI;
            }
        } else {
            turnTo -= TWO_PI;
        }
        ped->m_fCurrentRotation = current - (1.0f - left) * (current - turnTo);
    } else {
        ped->m_fCurrentRotation = ped->m_fAimingRotation;
    }

    // The ped is turned like the car while getting in, and the other way around when getting out
    if (assoc) {
        const auto id       = assoc->GetAnimId();
        const auto isGetIn  = id >= ANIM_ID_CAR_GETIN_LHS_0 && id <= ANIM_ID_CAR_GETIN_BIKE_FRONT;
        const auto isGetOut = id >= ANIM_ID_CAR_GETOUT_LHS_0 && id <= ANIM_ID_UNKNOWN_26;
        if (isGetIn || isGetOut) {
            const auto ToQuat = [](const CMatrix& matrix) {
                RwMatrix rwMatrix;
                matrix.CopyToRwMatrix(&rwMatrix);
                RtQuat quat;
                RtQuatConvertFromMatrix(&quat, &rwMatrix);
                return quat;
            };

            CMatrix carMatrix{ vehicle->GetMatrix() };
            CMatrix pedMatrix{ ped->GetMatrix() };
            if (isGetIn) {
                if (id == ANIM_ID_CAR_GETIN_BIKE_FRONT) {
                    CMatrix turn;
                    turn.SetRotateZ(PI);
                    carMatrix *= turn;
                }
            } else {
                pedMatrix.SetRotateZOnly(ped->m_fCurrentRotation);
            }

            auto from = ToQuat(isGetIn ? pedMatrix : carMatrix);
            auto to   = ToQuat(isGetIn ? carMatrix : pedMatrix);

            RtQuatSlerpCache cache;
            RtQuatSetupSlerpCache(&from, &to, &cache);
            RtQuat result;
            RtQuatSlerp(&result, &from, &to, assoc->m_CurrentTime / assoc->m_BlendHier->GetTotalTime(), &cache);

            RwMatrix rwResult{};
            RtQuatConvertToMatrix(&result, &rwResult);
            CMatrix newMatrix;
            newMatrix.UpdateMatrix(&rwResult);
            newMatrix.GetPosition() = targetNow;
            ped->SetMatrix(newMatrix);
            return false;
        }
    }

    if (!(progress <= 0.2f) || isCarUpsideDown || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
        ped->SetPosn(targetNow);
        ped->SetOrientation(0.0f, 0.0f, ped->m_fCurrentRotation);
    } else {
        // The ped is attached to the car, where the door is
        CMatrix carMatrix{ vehicle->GetMatrix() };
        const auto local = GetLocalPositionToOpenCarDoor(vehicle, 0.0f, assoc);
        carMatrix.GetPosition() += carMatrix.TransformVector(local);
        ped->SetMatrix(carMatrix);
    }
    return false;
}
