#include "StdInc.h"

#include "TaskUtilityLineUpPedWithCar.h"

void CTaskUtilityLineUpPedWithCar::InjectHooks() {
    RH_ScopedClass(CTaskUtilityLineUpPedWithCar);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x64FBB0);
    RH_ScopedInstall(Destructor, 0x64FC00);

    RH_ScopedInstall(GetLocalPositionToOpenCarDoor, 0x64FC10);
    RH_ScopedInstall(GetPositionToOpenCarDoor, 0x650A80);
    RH_ScopedInstall(ProcessPed, 0x6513A0, { .Reversed = false });
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
    return plugin::CallMethodAndReturn<bool, 0x6513A0, CTaskUtilityLineUpPedWithCar*, CPed*, CVehicle*, CAnimBlendAssociation*>(this, ped, vehicle, assoc);
}
