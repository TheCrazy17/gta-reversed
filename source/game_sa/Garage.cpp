#include "StdInc.h"

#include "Garage.h"
#include "Garages.h"
#include "Object.h"
#include "Wanted.h"

void CGarage::InjectHooks() {
    RH_ScopedClass(CGarage);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(BuildRotatedDoorMatrix, 0x4479F0);
    RH_ScopedInstall(TidyUpGarageClose, 0x449D10);
    RH_ScopedInstall(TidyUpGarage, 0x449C50);
    RH_ScopedInstall(StoreAndRemoveCarsForThisHideOut, 0x449900);
    RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690);
    RH_ScopedInstall(IsEntityTouching3D, 0x448EE0);
    RH_ScopedInstall(IsEntityEntirelyOutside, 0x448D30);
    RH_ScopedInstall(IsStaticPlayerCarEntirelyInside, 0x44A830);
    RH_ScopedInstall(IsEntityEntirelyInside3D, 0x448BE0);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "0", 0x448740, bool (CGarage::*)(CVector));
    RH_ScopedInstall(PlayerArrestedOrDied, 0x4486C0);
    RH_ScopedInstall(OpenThisGarage, 0x447D50);
    RH_ScopedInstall(CloseThisGarage, 0x447D70);
    RH_ScopedInstall(InitDoorsAtStart, 0x447600);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "1", 0x4487D0, bool (CGarage::*)(CVector, float));
    RH_ScopedInstall(Update, 0x44AA50);
    RH_ScopedInstall(IsGarageEmpty, 0x44A9C0);
    RH_ScopedInstall(RestoreCarsForThisHideOut, 0x448550);
    RH_ScopedInstall(RestoreCarsForThisImpoundingGarage, 0x4485C0);
    RH_ScopedInstall(RightModTypeForThisGarage, 0x447720);
    RH_ScopedInstall(IsPlayerOutsideGarage, 0x448E50);
    RH_ScopedInstall(IsAnyOtherCarTouchingGarage, 0x449100);
    RH_ScopedInstall(IsAnyCarBlockingDoor, 0x4494F0);
    RH_ScopedInstall(CountCarsWithCenterPointWithinGarage, 0x4495F0);
    RH_ScopedInstall(StoreAndRemoveCarsForThisImpoundingGarage, 0x449A50);
    RH_ScopedInstall(CalcDistToGarageRectangleSquared, 0x447D80);
    RH_ScopedInstall(SlideDoorOpen, 0x44A660);
    RH_ScopedInstall(SlideDoorClosed, 0x44A750);
    RH_ScopedInstall(FindDoorsWithGarage, 0x449FF0);
    RH_ScopedInstall(NeatlyLineUpStoredCars, 0x448330);
    RH_ScopedInstall(CenterCarInGarage, 0x449220);
}

// 0x4479F0
void CGarage::BuildRotatedDoorMatrix(CEntity* entity, float fDoorPosition) {
    const auto fAngle = fDoorPosition * -HALF_PI;
    const auto fSin = sin(fAngle);
    const auto fCos = cos(fAngle);
    CMatrix& matrix = entity->GetMatrix();

    const auto& vecForward = matrix.GetForward();
    matrix.GetUp() = CVector(-fSin * vecForward.y, fSin * vecForward.x, fCos);
    matrix.GetRight() = CrossProduct(vecForward, matrix.GetUp());
}

// 0x449D10
void CGarage::TidyUpGarageClose() {
    // NOTSA: Like the original, this goes through slots size-1 down to 1, slot 0 is never visited
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize() - 1; i != 0; i--) {
        auto* const vehicle = pool->GetAt(i);
        if (!vehicle) {
            continue;
        }
        if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }
        if (vehicle->GetStatus() != STATUS_WRECKED || !IsEntityTouching3D(vehicle)) {
            continue;
        }

        if (m_nDoorState != GARAGE_DOOR_CLOSED) {
            auto notEntirelyInside = false;
            for (const auto& sphere : vehicle->GetColModel()->GetData()->GetSpheres()) {
                const auto worldCenter = vehicle->GetMatrix().TransformPoint(sphere.m_vecCenter);
                if (!IsPointInsideGarage(worldCenter, sphere.m_fRadius)) {
                    notEntirelyInside = true;
                }
            }
            if (!notEntirelyInside) {
                continue;
            }
        }

        CWorld::Remove(vehicle);
        delete vehicle;
    }
}

// 0x449C50
void CGarage::TidyUpGarage() {
    // NOTSA: Like the original, this goes through slots size-1 down to 1, slot 0 is never visited
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize() - 1; i != 0; i--) {
        auto* const vehicle = pool->GetAt(i);
        if (!vehicle) {
            continue;
        }
        if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }
        if (!IsPointInsideGarage(vehicle->GetPosition())) {
            continue;
        }
        // Wrecks and flipped over cars get cleaned up
        if (vehicle->GetStatus() != STATUS_WRECKED && vehicle->GetUp().z >= 0.5f) {
            continue;
        }

        CWorld::Remove(vehicle);
        delete vehicle;
    }
}

// 0x449900
void CGarage::StoreAndRemoveCarsForThisHideOut(CStoredCar* storedCars, int32 maxSlot) {
    maxSlot = std::min<int32>(maxSlot, NUM_GARAGE_STORED_CARS);

    for (auto i = 0; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();

    auto pool = GetVehiclePool();
    auto storedCarIdx{0u};
    for (auto i = pool->GetSize(); i; i--) {
        if (auto vehicle = pool->GetAt(i - 1)) {
            if (IsPointInsideGarage(vehicle->GetPosition()) && vehicle->GetCreatedBy() != MISSION_VEHICLE) {
                if (storedCarIdx < static_cast<uint32>(maxSlot) && !EntityHasASphereWayOutsideGarage(vehicle, 1.0f)) {
                    storedCars[storedCarIdx++].StoreCar(vehicle);
                }

                FindPlayerInfo().CancelPlayerEnteringCars(vehicle);
                CWorld::Remove(vehicle);
                delete vehicle;
            }
        }
    }

    // Clear slots with no vehicles in it
    for (auto i = storedCarIdx; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();
}

// 0x449A50
void CGarage::StoreAndRemoveCarsForThisImpoundingGarage(CStoredCar* storedCars, int32 iMaxSlot) {
    iMaxSlot = std::min<int32>(iMaxSlot, NUM_GARAGE_STORED_CARS);

    for (auto i = 0; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();

    auto pool = GetVehiclePool();
    auto storedCarIdx{0u};
    for (auto i = pool->GetSize(); i; i--) {
        if (auto vehicle = pool->GetAt(i - 1)) {
            if (IsPointInsideGarage(vehicle->GetPosition()) && vehicle->GetCreatedBy() != MISSION_VEHICLE) {
                if (storedCarIdx < static_cast<uint32>(iMaxSlot) && !EntityHasASphereWayOutsideGarage(vehicle, 1.0f)) {
                    storedCars[storedCarIdx++].StoreCar(vehicle);
                }

                FindPlayerInfo().CancelPlayerEnteringCars(vehicle);
                CWorld::Remove(vehicle);
                delete vehicle;
            }
        }
    }

    // Clear slots with no vehicles in it
    for (auto i = storedCarIdx; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();
}

// 0x448550
bool CGarage::RestoreCarsForThisHideOut(CStoredCar* car) {
    for (auto i = 0; i < 4; i++) {
        if (!car[i].HasCar()) {
            continue;
        }
        if (auto* const vehicle = car[i].RestoreCar()) {
            vehicle->vehicleFlags.bImpounded = false;
            CWorld::Add(vehicle);
            car[i].Clear();
        }
    }

    for (auto i = 0; i < 4; i++) {
        if (car[i].HasCar()) {
            return false;
        }
    }
    return true;
}

// 0x4485C0
bool CGarage::RestoreCarsForThisImpoundingGarage(CStoredCar* car) {
    for (auto i = 0; i < 3; i++) {
        if (!car[i].HasCar()) {
            continue;
        }
        if (auto* const vehicle = car[i].RestoreCar()) {
            vehicle->vehicleFlags.bImpounded = true;
            CWorld::Add(vehicle);

            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
                vehicle->AsAutomobile()->PlaceOnRoadProperly();
            } else if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE) {
                vehicle->AsBike()->PlaceOnRoadProperly();
            }

            car[i].Clear();
        }
    }

    for (auto i = 0; i < 3; i++) {
        if (car[i].HasCar()) {
            return false;
        }
    }
    return true;
}

// 0x449050
bool CGarage::EntityHasASphereWayOutsideGarage(CEntity* entity, float fRadius) {
    for (const auto& sphere : entity->GetColModel()->GetData()->GetSpheres()) {
        const auto worldCenter = entity->GetMatrix().TransformPoint(sphere.m_vecCenter);
        if (!IsPointInsideGarage(worldCenter, fRadius + sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x448E50
bool CGarage::IsPlayerOutsideGarage(float fRadius) {
    if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
        return IsEntityEntirelyOutside(playerVehicle, fRadius);
    }
    return IsEntityEntirelyOutside(FindPlayerPed(-1), fRadius);
}

// 0x4495F0
int32 CGarage::CountCarsWithCenterPointWithinGarage(CVehicle* ignoredVehicle) {
    int32 count = 0;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (&vehicle != ignoredVehicle && IsPointInsideGarage(vehicle.GetPosition())) {
            count++;
        }
    }
    return count;
}

// 0x449690
void CGarage::RemoveCarsBlockingDoorNotInside() {
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const vehicle = pool->GetAt(i - 1);
        if (!vehicle || !IsEntityTouching3D(vehicle) || IsPointInsideGarage(vehicle->GetPosition())) {
            continue;
        }
        if (vehicle->vehicleFlags.bIsLocked || !vehicle->CanBeDeleted()) {
            continue;
        }
        CWorld::Remove(vehicle);
        delete vehicle;
        return;
    }
}

// 0x448EE0
bool CGarage::IsEntityTouching3D(CEntity* entity) {
    for (const auto& sphere : entity->GetColModel()->GetData()->GetSpheres()) {
        const auto worldCenter = entity->GetMatrix().TransformPoint(sphere.m_vecCenter);
        if (IsPointInsideGarage(worldCenter, sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x448D30
bool CGarage::IsEntityEntirelyOutside(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius < pos.x && pos.x < m_fRightCoord + radius &&
        m_fFrontCoord - radius < pos.y && pos.y < m_fBackCoord + radius) {
        return false;
    }

    for (const auto& sphere : entity->GetColModel()->GetData()->GetSpheres()) {
        const auto worldCenter = entity->GetMatrix().TransformPoint(sphere.m_vecCenter);
        if (IsPointInsideGarage(worldCenter, radius + sphere.m_fRadius)) {
            return false;
        }
    }
    return true;
}

// 0x44A830
bool CGarage::IsStaticPlayerCarEntirelyInside() {
    auto* const playerVehicle = FindPlayerVehicle(-1, false);
    if (!playerVehicle) {
        return false;
    }
    if (playerVehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && playerVehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
        return false;
    }
    if (FindPlayerPed(-1)->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
        return false;
    }

    const auto& pos = playerVehicle->GetPosition();
    if (pos.x < m_fLeftCoord || pos.x > m_fRightCoord) {
        return false;
    }
    if (pos.y < m_fFrontCoord || pos.y > m_fBackCoord) {
        return false;
    }

    const auto& vel = playerVehicle->GetMoveSpeed();
    if (std::abs(vel.x) > 0.01f || std::abs(vel.y) > 0.01f || std::abs(vel.z) > 0.01f) {
        return false;
    }
    if (vel.SquaredMagnitude() > 0.0001f) {
        return false;
    }

    return IsEntityEntirelyInside3D(playerVehicle, 0.0f);
}

// 0x448BE0
bool CGarage::IsEntityEntirelyInside3D(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();
    if (!(m_fLeftCoord - radius <= pos.x && pos.x <= m_fRightCoord + radius &&
          m_fFrontCoord - radius <= pos.y && pos.y <= m_fBackCoord + radius &&
          m_vPosn.z - radius <= pos.z && pos.z <= m_fTopZ + radius)) {
        return false;
    }

    for (const auto& sphere : entity->GetColModel()->GetData()->GetSpheres()) {
        const auto worldCenter = entity->GetMatrix().TransformPoint(sphere.m_vecCenter);
        if (!IsPointInsideGarage(worldCenter, radius - sphere.m_fRadius)) {
            return false;
        }
    }
    return true;
}

// 0x448740
bool CGarage::IsPointInsideGarage(CVector point) {
    if (point.z < m_vPosn.z || point.z > m_fTopZ) {
        return false;
    }

    const auto relX = point.x - m_vPosn.x;
    const auto relY = point.y - m_vPosn.y;

    const auto dotA = relY * m_vDirectionA.y + relX * m_vDirectionA.x;
    if (dotA < 0.0f || dotA > m_fWidth) {
        return false;
    }

    const auto dotB = relY * m_vDirectionB.y + relX * m_vDirectionB.x;
    return dotB >= 0.0f && dotB <= m_fHeight;
}

// 0x4486C0
void CGarage::PlayerArrestedOrDied() {
    if (m_nType == BOMBSHOP_TIMED || m_nType == BOMBSHOP_ENGINE || m_nType == BOMBSHOP_REMOTE || m_nType == PAYNSPRAY || m_nType == 13) {
        if (m_nDoorState == GARAGE_DOOR_CLOSED || m_nDoorState == GARAGE_DOOR_CLOSING || m_nDoorState == GARAGE_DOOR_OPENING) {
            m_nDoorState = GARAGE_DOOR_OPENING;
        }
        return;
    }

    if (m_nDoorState != GARAGE_DOOR_CLOSED && m_nDoorState < GARAGE_DOOR_WAITING_PLAYER_TO_EXIT) {
        m_nDoorState = GARAGE_DOOR_CLOSING;
    }
}

// 0x447D50
void CGarage::OpenThisGarage() {
  if ( m_nDoorState == GARAGE_DOOR_CLOSED
    || m_nDoorState == GARAGE_DOOR_CLOSING
    || m_nDoorState == GARAGE_DOOR_CLOSED_DROPPED_CAR)
  {
    m_nDoorState = GARAGE_DOOR_OPENING;
  }
}

// 0x447D70
void CGarage::CloseThisGarage() {
    if (m_nDoorState == GARAGE_DOOR_OPEN || m_nDoorState == GARAGE_DOOR_OPENING)
        m_nDoorState = GARAGE_DOOR_CLOSING;
}

// 0x447600
void CGarage::InitDoorsAtStart() {
    m_nFlags = (m_nFlags & 0x39) | 0x40;
    m_nDoorState = GARAGE_DOOR_CLOSED;
    m_nTimeToOpen = 0;

    if (m_nType == BOMBSHOP_TIMED || m_nType == BOMBSHOP_ENGINE || m_nType == BOMBSHOP_REMOTE || m_nType == PAYNSPRAY) {
        m_nDoorState = GARAGE_DOOR_OPEN;
        m_fDoorPosition = 1.0f;
    } else {
        m_fDoorPosition = 0.0f;
    }
}

// 0x4487D0
bool CGarage::IsPointInsideGarage(CVector point, float radius) {
    if (point.z < m_vPosn.z - radius || point.z > m_fTopZ + radius) {
        return false;
    }

    const auto relX = point.x - m_vPosn.x;
    const auto relY = point.y - m_vPosn.y;

    const auto dotA = relY * m_vDirectionA.y + relX * m_vDirectionA.x;
    if (dotA < -radius || dotA > m_fWidth + radius) {
        return false;
    }

    const auto dotB = relY * m_vDirectionB.y + relX * m_vDirectionB.x;
    return dotB >= -radius && dotB <= m_fHeight + radius;
}

// 0x44A660
bool CGarage::SlideDoorOpen() {
    const auto speed = CTimer::GetTimeStep() * (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT ? 0.0011f : 0.011f);
    m_fDoorPosition += speed;

    CObject* door1{};
    CObject* door2{};
    if (m_fDoorPosition >= 1.0f) {
        m_fDoorPosition = 1.0f;
        FindDoorsWithGarage(&door1, &door2);
        if (door1) {
            m_GarageAudio.AddAudioEvent(AE_GARAGE_DOOR_OPENED, door1->GetPosition(), 0.0f, 1.0f);
        }
        return true;
    }

    FindDoorsWithGarage(&door1, &door2);
    if (door1) {
        m_GarageAudio.AddAudioEvent(AE_GARAGE_DOOR_OPENING, door1->GetPosition(), 0.0f, 1.0f);
    }
    return false;
}

// 0x44A750
bool CGarage::SlideDoorClosed() {
    const auto speed = CTimer::GetTimeStep() * (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT ? 0.0013f : 0.013f);
    m_fDoorPosition -= speed;

    CObject* door1{};
    CObject* door2{};
    if (m_fDoorPosition <= 0.0f) {
        m_fDoorPosition = 0.0f;
        FindDoorsWithGarage(&door1, &door2);
        if (door1) {
            m_GarageAudio.AddAudioEvent(AE_GARAGE_DOOR_CLOSED, door1->GetPosition(), 0.0f, 1.0f);
        }
        return true;
    }

    FindDoorsWithGarage(&door1, &door2);
    if (door1) {
        m_GarageAudio.AddAudioEvent(AE_GARAGE_DOOR_CLOSING, door1->GetPosition(), 0.0f, 1.0f);
    }
    return false;
}

// 0x449FF0
void CGarage::FindDoorsWithGarage(CObject** ppFirstDoor, CObject** ppSecondDoor) {
    *ppFirstDoor = nullptr;
    *ppSecondDoor = nullptr;

    const auto centerX = m_vPosn.x + (m_vDirectionA.x * m_fWidth + m_vDirectionB.x * m_fHeight) * 0.5f;
    const auto centerY = m_vPosn.y + (m_vDirectionA.y * m_fWidth + m_vDirectionB.y * m_fHeight) * 0.5f;

    const auto garageIndex = static_cast<int8>(this - &CGarages::aGarages[0]);

    auto bestDist   = 99999.9f; // first-door distance
    auto secondDist = 99999.9f; // second-door distance

    auto* const pool = GetObjectPool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const door = pool->GetAt(i - 1);
        if (!door || door->m_nGarageDoorGarageIndex != garageIndex) {
            continue;
        }

        const auto& pos = door->GetPosition();
        const auto dx = centerX - pos.x;
        const auto dy = centerY - pos.y;
        const auto dist = std::sqrt(dx * dx + dy * dy);

        if (*ppFirstDoor) {
            if (bestDist <= dist) {
                if (!*ppSecondDoor || dist < secondDist) {
                    *ppSecondDoor = door;
                    secondDist = dist;
                }
                continue;
            }
            *ppSecondDoor = *ppFirstDoor;
            secondDist = bestDist;
        }
        *ppFirstDoor = door;
        bestDist = dist;
    }
}

// 0x448330
void CGarage::NeatlyLineUpStoredCars(CStoredCar* car) {
    if (!car[0].HasCar()) {
        return;
    }

    const CVector corner{
        m_vPosn.x + (m_vDirectionA.x * m_fWidth + m_vDirectionB.x * m_fHeight) * 0.5f,
        m_vPosn.y + (m_vDirectionA.y * m_fWidth + m_vDirectionB.y * m_fHeight) * 0.5f,
        m_vPosn.z + 0.5f
    };

    const auto dir = Normalized(CVector{ m_vDirectionA.x, m_vDirectionA.y, 0.0f });
    const auto step = dir * 4.0f;
    const auto start = corner - step;

    for (auto i = 0; i <= 2; i++) {
        car[i].m_vPosn = start + step * static_cast<float>(i);
        // The cars face along the perpendicular of the garage direction, packed as truncated 1/100ths
        car[i].m_nPackedForwardX = static_cast<uint8>(static_cast<int32>(dir.y * 100.0f));
        car[i].m_nPackedForwardY = static_cast<uint8>(static_cast<int32>(-dir.x * 100.0f));
        car[i].m_nPackedForwardZ = 0;

        if (i == 2 || !car[i + 1].HasCar()) {
            break;
        }
    }
}

// 0x449220 -> tail-jmp chain through an obfuscated `EBP = GetVehiclePool()` load (SecuROM-style
// split-dword-sum trick applied to a data load rather than a return address) -> real body 0x44922B.
void CGarage::CenterCarInGarage(CVehicle* vehicle) {
    static constexpr auto PUSH_FORCE = 0.02f;

    const auto centerX = (m_fLeftCoord + m_fRightCoord) * 0.5f;
    const auto centerY = (m_fFrontCoord + m_fBackCoord) * 0.5f;

    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const otherVehicle = pool->GetAt(i - 1);
        if (!otherVehicle || otherVehicle == vehicle || !IsEntityTouching3D(otherVehicle)) {
            continue;
        }

        for (const auto& sphere : otherVehicle->GetColModel()->GetData()->GetSpheres()) {
            const auto worldCenter = otherVehicle->GetMatrix().TransformPoint(sphere.m_vecCenter);
            if (IsPointInsideGarage(worldCenter, 0.0f)) { // NOTSA: radius forced to 0.0f here, NOT sphere.m_fRadius (confirmed via raw disasm)
                continue;
            }

            const auto& pos = otherVehicle->GetPosition();
            const auto push = Normalized(CVector{ pos.x - centerX, pos.y - centerY, 0.0f }) * PUSH_FORCE * CTimer::GetTimeStep();
            otherVehicle->GetMoveSpeed() += push;
            break; // only the first out-of-bounds sphere per vehicle triggers a push
        }
    }
}

// 0x4494F0
bool CGarage::IsAnyCarBlockingDoor() {
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const vehicle = pool->GetAt(i - 1);
        if (!vehicle || !IsEntityTouching3D(vehicle)) {
            continue;
        }
        for (const auto& sphere : vehicle->GetColModel()->GetData()->GetSpheres()) {
            const auto worldCenter = vehicle->GetMatrix().TransformPoint(sphere.m_vecCenter);
            if (!IsPointInsideGarage(worldCenter, sphere.m_fRadius)) {
                return true;
            }
        }
    }
    return false;
}

// 0x449100
bool CGarage::IsAnyOtherCarTouchingGarage(CVehicle* ignoredVehicle) {
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const vehicle = pool->GetAt(i - 1);
        if (!vehicle || vehicle == ignoredVehicle || vehicle->GetStatus() == STATUS_WRECKED || !IsEntityTouching3D(vehicle)) {
            continue;
        }
        for (const auto& sphere : vehicle->GetColModel()->GetData()->GetSpheres()) {
            const auto worldCenter = vehicle->GetMatrix().TransformPoint(sphere.m_vecCenter);
            if (IsPointInsideGarage(worldCenter, sphere.m_fRadius)) {
                return true;
            }
        }
    }
    return false;
}

// 0x447720
bool CGarage::RightModTypeForThisGarage(CVehicle* vehicle) {
    if (!vehicle) {
        return false;
    }
    switch (m_nType) {
    case TUNING_LOCO_LOW_CO:
        return vehicle->m_pHandlingData->m_bLowRider;
    case TUNING_WHEEL_ARCH_ANGELS:
        return vehicle->m_pHandlingData->m_bStreetRacer;
    case TUNING_TRANSFENDER:
        return !vehicle->m_pHandlingData->m_bLowRider && !vehicle->m_pHandlingData->m_bStreetRacer;
    default:
        return false;
    }
}

// 0x447D80
float CGarage::CalcDistToGarageRectangleSquared(float x, float y) {
    float dx;
    if (x < m_fLeftCoord) {
        dx = x - m_fLeftCoord;
    } else if (x > m_fRightCoord) {
        dx = x - m_fRightCoord;
    } else {
        dx = 0.0f;
    }

    float dy;
    if (y < m_fFrontCoord) {
        dy = y - m_fFrontCoord;
    } else if (y > m_fBackCoord) {
        dy = y - m_fBackCoord;
    } else {
        dy = 0.0f;
    }

    return dx * dx + dy * dy;
}

// 0x44AA50
void CGarage::Update(int32 garageId) {
    // Camera: tell the camera when the player is inside of this garage
    if (m_nType != 13 && m_nDoorState < 6 && FindPlayerPed() && !m_bCameraFollowsPlayer) {
        auto* const playerVehicle = FindPlayerVehicle(-1, false);
        auto*       candidateEntity = static_cast<CEntity*>(FindPlayerPed());
        auto* const player = FindPlayerPed();
        if (player->bInVehicle && player->m_pVehicle && player->m_pVehicle->GetModelIndex() == MODEL_KART) {
            candidateEntity = player->m_pVehicle;
        }

        if (IsEntityEntirelyInside3D(candidateEntity, 0.25f)) {
            CGarages::bCamShouldBeOutside = true;
            TheCamera.m_pToGarageWeAreIn = this;
        }

        if (playerVehicle) {
            if (!IsEntityEntirelyOutside(playerVehicle, 0.0f)) {
                TheCamera.m_pToGarageWeAreInForHackAvoidFirstPerson = this;
            }
            if (playerVehicle->GetModelIndex() == MODEL_MRWHOOP) {
                const auto& pos = playerVehicle->GetPosition();
                if (m_fLeftCoord - 0.5f < pos.x && pos.x < m_fRightCoord + 0.5f &&
                    m_fFrontCoord - 0.5f < pos.y && pos.y < m_fBackCoord + 0.5f) {
                    CGarages::bCamShouldBeOutside = true;
                    TheCamera.m_pToGarageWeAreIn = this;
                }
            }
        }
    }

    if (m_bInactive && m_nDoorState == GARAGE_DOOR_CLOSED) {
        return;
    }
    if (m_bDoorOpensUp) {
        m_bDoorClosed = !((m_nDoorState == GARAGE_DOOR_OPENING && m_fDoorPosition > 0.4f) || m_nDoorState == GARAGE_DOOR_OPEN);
    }

    const auto SetPlayerInGarage = [](bool inGarage) {
        CPad::GetPad(0)->bPlayerAwaitsInGarage = inGarage;
        FindPlayerWanted(-1)->m_bPoliceBackOffGarage = inGarage;
    };
    const auto CallOffChase = [this] {
        CWorld::CallOffChaseForArea(m_fLeftCoord - 10.0f, m_fFrontCoord - 10.0f, m_fRightCoord + 10.0f, m_fBackCoord + 10.0f);
    };
    // Squared distance of the player to the middle of the garage
    const auto SqDistPlayerToCenter = [this] {
        const auto& coors = FindPlayerCoors(-1);
        const auto  dx    = coors.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
        const auto  dy    = coors.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
        return dx * dx + dy * dy;
    };
    const auto SqDistToRect = [this](const CVector& pos) {
        return CalcDistToGarageRectangleSquared(pos.x, pos.y);
    };
    // Door is opening: finish it and let the garage know it's open
    const auto ProcessOpening = [this] {
        if (SlideDoorOpen()) {
            m_nDoorState = GARAGE_DOOR_OPEN;
        }
    };
    // Door is closing on its own: the closing animation finishes it
    const auto ProcessClosedByAnimation = [this] {
        if (SlideDoorClosed()) {
            m_nDoorState = GARAGE_DOOR_CLOSED;
        }
    };
    const auto MaxCarsInHideout = [this] {
        return m_nType == SAFEHOUSE_GANTON ? 2 : 4;
    };
    const auto PlayerOrPlayerVehicle = []() -> CEntity* {
        if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
            return vehicle;
        }
        return FindPlayerPed(-1);
    };

    // Waits for the player's vehicle to come close enough to the garage
    const auto OpenIfPlayerVehicleIsClose = [&](bool onlyTargetCar) {
        auto* const vehicle = FindPlayerVehicle(-1, false);
        if (onlyTargetCar && (vehicle != m_pTargetCar || !m_pTargetCar)) {
            return;
        }
        if (!vehicle) {
            return;
        }
        if (SqDistToRect(vehicle->GetPosition()) >= 64.0f) {
            return;
        }
        m_nDoorState = GARAGE_DOOR_OPENING;
    };

    // The bomb shops. ONLY_TARGET_VEH garages end up in here for the door states they don't handle.
    const auto ProcessBombShop = [&] {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CTimer::GetTimeInMS() <= m_nTimeToOpen) {
                return;
            }
            if (m_nType == BOMBSHOP_REMOTE && !CStreaming::IsModelLoaded(MODEL_BOMB)) {
                CStreaming::RequestModel(MODEL_BOMB, STREAMING_GAME_REQUIRED);
                return;
            }

            switch (m_nType) {
            case BOMBSHOP_TIMED:  AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_TIMED);                  break;
            case BOMBSHOP_ENGINE: AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_BOOBY_TRAPPED);          break;
            case BOMBSHOP_REMOTE: AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_REMOTE_CONTROLLED);      break;
            default:                                                                                                     break;
            }

            m_nDoorState = GARAGE_DOOR_OPENING;

            if (!CGarages::BombsAreFree) {
                auto& money = FindPlayerInfo().m_nMoney;
                if (money > 0) {
                    money = std::max(money - 500, 0);
                }
            }

            if (auto* const vehicle = FindPlayerVehicle(-1, false);
                vehicle && (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE)) {
                vehicle->m_nBombOnBoard          = (m_nType - 1) & 7;
                vehicle->m_pWhoInstalledBombOnMe = FindPlayerPed();
                if (m_nType == BOMBSHOP_REMOTE) {
                    CGarages::GivePlayerDetonator();
                }
                CStats::IncrementStat(STAT_KGS_OF_EXPLOSIVES_USED, 10.0f);
            }

            const char* key{};
            switch (m_nType) {
            case BOMBSHOP_TIMED:
            case BOMBSHOP_ENGINE: {
                const auto padMode = CPad::GetPad(0)->Mode;
                if (padMode < 3) {
                    key = m_nType == BOMBSHOP_TIMED ? "GA_6" : "GA_7";
                } else if (padMode == 3) {
                    key = m_nType == BOMBSHOP_TIMED ? "GA_6B" : "GA_7B";
                } else {
                    return;
                }
                break;
            }
            case BOMBSHOP_REMOTE:
                key = "GA_8";
                break;
            default:
                return;
            }
            CHud::SetHelpMessage(TheText.Get(key), false, false, true);
            return;
        }
        case GARAGE_DOOR_OPEN: {
            if (!IsStaticPlayerCarEntirelyInside()) {
                return;
            }
            auto* const vehicle = FindPlayerVehicle(-1, false);
            if (!vehicle || vehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                return;
            }
            if (vehicle->m_nBombOnBoard != 0) {
                CGarages::TriggerMessage("GA_5", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_ALREADY_RIGGED);
                return;
            }
            if (!CGarages::BombsAreFree && FindPlayerInfo().m_nMoney < 500) {
                CGarages::TriggerMessage("GA_4", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH);
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSING;
            SetPlayerInGarage(true);
            return;
        }
        case GARAGE_DOOR_CLOSING:
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(vehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState  = GARAGE_DOOR_CLOSED;
                m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
            }
            if (m_nType == BOMBSHOP_REMOTE) {
                CStreaming::RequestModel(MODEL_BOMB, STREAMING_GAME_REQUIRED);
            }
            return;
        case GARAGE_DOOR_OPENING:
            if (SlideDoorOpen()) {
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            }
            if (m_fDoorPosition > 0.5f) {
                SetPlayerInGarage(false);
            }
            return;
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT:
            if (IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPEN;
            }
            return;
        default:
            return;
        }
    };

    // Doors of the garage that are used by the target car: the car is dropped off once the door is closed
    const auto ProcessTargetCarClosing = [&](bool dropTimer) {
        if (m_pTargetCar) {
            CenterCarInGarage(m_pTargetCar);
        }
        if (!SlideDoorClosed()) {
            return;
        }
        if (m_b0x1) {
            m_nDoorState = GARAGE_DOOR_CLOSED;
            return;
        }
        if (m_pTargetCar) {
            m_nDoorState = GARAGE_DOOR_CLOSED_DROPPED_CAR;
            if (dropTimer) {
                m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
                m_pTargetCar  = nullptr;
            } else {
                m_pTargetCar->DestroyVehicleAndDriverAndPassengers(m_pTargetCar);
                m_pTargetCar = nullptr;
            }
        } else {
            m_nDoorState = GARAGE_DOOR_CLOSED;
        }
        SetPlayerInGarage(false);
    };

    switch (m_nType) {
    case ONLY_TARGET_VEH:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED:
            OpenIfPlayerVehicleIsClose(true);
            return;
        case GARAGE_DOOR_OPEN: {
            if (SqDistPlayerToCenter() <= 900.0f) {
                if (FindPlayerVehicle(-1, false) == m_pTargetCar || !m_pTargetCar) {
                    return;
                }
                if (!IsEntityEntirelyInside3D(m_pTargetCar, 0.0f)) {
                    return;
                }
                if (IsEntityEntirelyOutside(PlayerOrPlayerVehicle(), 2.0f)) {
                    SetPlayerInGarage(true);
                    m_b0x1       = false;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
                return;
            }
            if ((CTimer::GetFrameCounter() & 0x1F) != 0) {
                return;
            }
            if (m_pTargetCar && IsEntityTouching3D(m_pTargetCar)) {
                return;
            }
            m_b0x1       = true;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        case GARAGE_DOOR_CLOSING:
            ProcessTargetCarClosing(false);
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            ProcessBombShop();
            return;
        }

    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
        ProcessBombShop();
        return;

    case PAYNSPRAY: {
        if (FindPlayerCoors(-1).z >= 950.0f) {
            return;
        }

        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CGarages::NoResprays) {
                return;
            }
            if (CTimer::GetTimeInMS() <= m_nTimeToOpen) {
                CallOffChase();
                return;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;

            auto  needsRespray = false;
            auto* wanted       = FindPlayerWanted(-1);
            const auto wasWanted = wanted->m_WantedLevel != eWantedLevel::WANTED_CLEAN;
            auto  colourChanged = wasWanted;
            if (wasWanted) {
                needsRespray = true;
                wanted->ClearWantedLevelAndGoOnParole();
            }

            if (auto* const vehicle = FindPlayerVehicle(-1, false);
                vehicle && (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE)) {
                colourChanged = false;
                if (vehicle->m_fHealth < 970.0f) {
                    needsRespray = true;
                }
                vehicle->m_fHealth = std::max(1000.0f, vehicle->m_fHealth);
                if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                    vehicle->AsAutomobile()->m_fBurnTimer = 0.0f;
                } else {
                    vehicle->AsBike()->m_BlowUpTimer = 0.0f;
                }
                vehicle->Fix();
                CStats::IncrementStat(STAT_VEHICLE_RESPRAYS, 1.0f);

                // Put the vehicle back onto its wheels
                if (vehicle->GetMatrix().GetUp().z < 0.0f) {
                    vehicle->GetMatrix().GetUp()    = -vehicle->GetMatrix().GetUp();
                    vehicle->GetMatrix().GetRight() = -vehicle->GetMatrix().GetRight();
                }

                // NOTSA: Like the original, this reads the automobile's flags for every vehicle type (the pool slots are big enough for that)
                if (!vehicle->AsAutomobile()->autoFlags.bShouldNotChangeColour && vehicle->GetRemapIndex() < 0) {
                    uint8 primary, secondary, tertiary, quaternary;
                    vehicle->GetVehicleModelInfo()->ChooseVehicleColour(primary, secondary, tertiary, quaternary, 1);
                    if (vehicle->m_nPrimaryColor != primary || vehicle->m_nSecondaryColor != secondary
                        || vehicle->m_nTertiaryColor != tertiary || vehicle->m_nQuaternaryColor != quaternary) {
                        colourChanged = true;
                    }
                    vehicle->m_nPrimaryColor    = primary;
                    vehicle->m_nSecondaryColor  = secondary;
                    vehicle->m_nTertiaryColor   = tertiary;
                    vehicle->m_nQuaternaryColor = quaternary;
                    vehicle->SetRemap(-1);

                    if (colourChanged) {
                        // A cloud of smoke in the new colour
                        FxPrtMult_c particle{ 1.0f, 0.0f, 0.0f, 0.6f, 0.7f, 1.0f, 0.4f };
                        const auto  colour = CVehicleModelInfo::ms_vehicleColourTable[vehicle->m_nPrimaryColor];
                        particle.m_Color.red   = (float)colour.r / 255.0f;
                        particle.m_Color.green = (float)colour.g / 255.0f;
                        particle.m_Color.blue  = (float)colour.b / 255.0f;
                        for (auto i = 0; i < 10; i++) {
                            CVector pos = vehicle->GetPosition();
                            pos.x += CGeneral::GetRandomNumberInRange(-3.0f, 3.0f);
                            pos.y += CGeneral::GetRandomNumberInRange(-3.0f, 3.0f);
                            const CVector velocity{ 0.0f, 0.0f, CGeneral::GetRandomNumberInRange(0.0f, 0.05f) };
                            g_fx.m_SmokeHuge->AddParticle(pos, velocity, 0.0f, particle, -1.0f, 1.2f, 0.6f, false);
                        }
                    }
                }

                vehicle->m_fDirtLevel = 0.0f;
                vehicle->vehicleFlags.bDisableParticles = false;
            }

            if (m_bRespraysAlwaysFree) {
                CGarages::TriggerMessage("GA_22", -1, 4000, -1);
            } else if (needsRespray && !CGarages::RespraysAreFree) {
                auto& money = FindPlayerInfo().m_nMoney;
                if (money > 0) {
                    money = std::max(money - 100, 0);
                }
                CStats::IncrementStat(STAT_AUTO_REPAIR_AND_PAINTING_BUDGET, 100.0f);
                CGarages::TriggerMessage(wasWanted ? "GA_2" : "GA_XX", -1, 4000, -1);
            } else if (colourChanged) {
                CGarages::TriggerMessage((rand() & 1) == 0 ? "GA_16" : "GA_15", -1, 4000, -1);
            }

            m_bUsedRespray = true;
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                vehicle->vehicleFlags.bHasBeenResprayed = true;
            }
            CallOffChase();
            return;
        }
        case GARAGE_DOOR_OPEN: {
            if (CGarages::NoResprays) {
                return;
            }
            if (IsStaticPlayerCarEntirelyInside()) {
                auto* const vehicle = FindPlayerVehicle(-1, false);
                if (!CGarages::IsCarSprayable(vehicle)) {
                    CGarages::TriggerMessage(vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX ? "GA_1B" : "GA_1", -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_IS_HOT);
                } else if (FindPlayerInfo().m_nMoney < 100 && !CGarages::RespraysAreFree) {
                    CGarages::TriggerMessage("GA_3", -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH);
                } else {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                    vehicle->m_fDirtLevel = 0.0f;
                }
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                CGarages::LastGaragePlayerWasIn = garageId;
            } else if (!IsPlayerOutsideGarage(0.0f)) {
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                CGarages::LastGaragePlayerWasIn = garageId;
            } else if (garageId == CGarages::LastGaragePlayerWasIn) {
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
            }

            auto* const vehicle = FindPlayerVehicle(-1, false);
            if (!vehicle) {
                return;
            }
            if (SqDistToRect(vehicle->GetPosition()) >= 64.0f) {
                return;
            }
            CallOffChase();
            return;
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(vehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_RESPRAY);
                m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
                CStats::IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, CStats::GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
                CStats::SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);
            }
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                // NOTSA: Like the original, this is done for every vehicle type (the pool slots are big enough for it)
                vehicle->AsAutomobile()->m_fBurnTimer = 0.0f;
                vehicle->vehicleFlags.bDisableParticles = true;
            }
            CallOffChase();
            return;
        }
        case GARAGE_DOOR_OPENING:
            if (SlideDoorOpen()) {
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            }
            if (m_fDoorPosition > 0.5f) {
                SetPlayerInGarage(false);
            }
            return;
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT:
            if (IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPEN;
            }
            return;
        default:
            return;
        }
    }

    case UNKN_CLOSESONTOUCH:
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN:
            if (IsGarageEmpty()) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            return;
        case GARAGE_DOOR_CLOSING:
            ProcessClosedByAnimation();
            if (!IsGarageEmpty()) {
                m_nDoorState = GARAGE_DOOR_OPENING;
            }
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            return;
        }

    case OPEN_FOR_TARGET_FREEZE_PLAYER:
    case CLOSE_WITH_CAR_DONT_OPEN_AGAIN:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED:
            if (auto* const vehicle = FindPlayerVehicle(-1, false); vehicle == m_pTargetCar && m_pTargetCar) {
                if (SqDistToRect(vehicle->GetPosition()) < 289.0f) {
                    m_nDoorState = GARAGE_DOOR_OPENING;
                }
            }
            return;
        case GARAGE_DOOR_OPEN:
            if (SqDistPlayerToCenter() <= 900.0f && m_pTargetCar) {
                if (m_pTargetCar != FindPlayerVehicle(-1, false)) {
                    return;
                }
                if (IsStaticPlayerCarEntirelyInside() && !IsAnyCarBlockingDoor()) {
                    SetPlayerInGarage(true);
                    m_b0x1       = false;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
                return;
            }
            m_b0x1       = true;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        case GARAGE_DOOR_CLOSING:
            ProcessTargetCarClosing(true);
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        case GARAGE_DOOR_CLOSED_DROPPED_CAR:
            if (m_nType == OPEN_FOR_TARGET_FREEZE_PLAYER && m_nTimeToOpen < CTimer::GetTimeInMS()) {
                m_nDoorState = GARAGE_DOOR_OPENING;
            }
            return;
        default:
            return;
        }

    case SCRIPT_ONLY_OPEN:
        if (m_nDoorState == GARAGE_DOOR_OPENING) {
            ProcessOpening();
        }
        return;

    case SAFEHOUSE_GANTON:
    case SAFEHOUSE_SANTAMARIA:
    case SAGEHOUSE_ROCKSHORE:
    case SAFEHOUSE_FORTCARSON:
    case SAFEHOUSE_VERDANTMEADOWS:
    case SAFEHOUSE_DILLIMORE:
    case SAFEHOUSE_PRICKLEPINE:
    case SAFEHOUSE_WHITEWOOD:
    case SAFEHOUSE_PALOMINOCREEK:
    case SAFEHOUSE_REDSANDSWEST:
    case SAFEHOUSE_ELCORONA:
    case SAFEHOUSE_MULHOLLAND:
    case SAFEHOUSE_CALTONHEIGHTS:
    case SAFEHOUSE_PARADISO:
    case SAFEHOUSE_DOHERTY:
    case SAFEHOUSE_HASHBURY:
    case HANGAR_AT400:
    case HANGAR_ABANDONED_AIRPORT:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            const auto& coors = FindPlayerCoors(-1);
            if (coors.z >= 950.0f) {
                return;
            }

            const auto sqDist = SqDistToRect(coors);
            if (sqDist >= 12.25f) {
                if (sqDist >= 100.0f) {
                    return;
                }
                auto* const vehicle = FindPlayerVehicle(-1, false);
                if (!vehicle || vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                    return;
                }
            }

            auto* const vehicle = FindPlayerVehicle(-1, false);
            if (!vehicle || m_nType == HANGAR_AT400 || CGarages::CountCarsInHideoutGarage(m_nType) < MaxCarsInHideout()) {
                if (m_nType != HANGAR_AT400) {
                    auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                    if (!RestoreCarsForThisHideOut(cars)) {
                        return; // Some cars are still being streamed in
                    }
                }
                m_nDoorState = GARAGE_DOOR_OPENING;
                return;
            }

            // The safehouse is full: tell the player so when they approach one of the doors
            CObject* door1{};
            CObject* door2{};
            FindDoorsWithGarage(&door1, &door2);
            const auto IsNearDoor = [&](CObject* door) {
                if (!door) {
                    return false;
                }
                const auto dx = door->GetPosition().x - vehicle->GetPosition().x;
                const auto dy = door->GetPosition().y - vehicle->GetPosition().y;
                return dx * dx + dy * dy < 25.0f;
            };
            if (!IsNearDoor(door1) && !IsNearDoor(door2)) {
                return;
            }
            if (CTimer::GetTimeInMS() - CGarages::LastTimeHelpMessage < 18001) {
                return;
            }
            const auto appearance = vehicle->GetVehicleAppearance();
            if (appearance != VEHICLE_APPEARANCE_HELI && appearance != VEHICLE_APPEARANCE_PLANE) {
                CHud::SetHelpMessage(TheText.Get("GA_21"), false, false, true);
                CGarages::LastTimeHelpMessage = CTimer::GetTimeInMS();
            }
            return;
        }
        case GARAGE_DOOR_OPEN: {
            const auto  sqDist  = SqDistToRect(FindPlayerCoors(-1));
            auto* const vehicle = FindPlayerVehicle(-1, false);
            if ((sqDist > 225.0f || (sqDist > 16.0f && (!vehicle || vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX))) && !IsAnyCarBlockingDoor()) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
                return;
            }
            if (vehicle) {
                if (CountCarsWithCenterPointWithinGarage(vehicle) >= MaxCarsInHideout() && IsPlayerOutsideGarage(0.25f)) {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    return;
                }
            }
            if (sqDist > 4900.0f) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
                RemoveCarsBlockingDoorNotInside();
            }
            return;
        }
        case GARAGE_DOOR_CLOSING:
            SlideDoorClosed();
            if (!IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPENING; // The player is back inside
                return;
            }
            if (m_fDoorPosition != 0.0f) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSED;
            if (m_nType != HANGAR_AT400) {
                StoreAndRemoveCarsForThisHideOut(CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType)), 4);
            }
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            return;
        }

    case SCRIPT_CONTROLLED:
        if (m_nDoorState == GARAGE_DOOR_CLOSING) {
            ProcessClosedByAnimation();
        } else if (m_nDoorState == GARAGE_DOOR_OPENING) {
            ProcessOpening();
        }
        return;

    case STAY_OPEN_WITH_CAR_INSIDE:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED:
            OpenIfPlayerVehicleIsClose(true);
            return;
        case GARAGE_DOOR_OPEN:
            if (SqDistPlayerToCenter() > 900.0f && m_pTargetCar && IsEntityEntirelyOutside(m_pTargetCar, 0.0f)) {
                m_b0x1       = true;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            return;
        case GARAGE_DOOR_CLOSING:
            if (m_pTargetCar) {
                CenterCarInGarage(m_pTargetCar);
            }
            ProcessClosedByAnimation();
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            return;
        }

    case SCRIPT_OPEN_FREEZE_WHEN_CLOSING:
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN:
            if (!m_pTargetCar || !IsEntityEntirelyInside3D(m_pTargetCar, 0.0f) || IsAnyCarBlockingDoor()) {
                return;
            }
            if (IsPlayerOutsideGarage(0.0f)) {
                CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                m_b0x1       = false;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            return;
        case GARAGE_DOOR_CLOSING:
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
            }
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            return;
        }

    case IMPOUND_LS:
    case IMPOUND_SF:
    case IMPOUND_LV: {
        // The door never animates here, the cars are put in or taken out as the player gets close or leaves
        const auto& coors   = FindPlayerCoors(-1);
        const auto  sqDist  = SqDistToRect(coors);
        const auto  heightOk = coors.z < m_fTopZ - 2.0f && m_vPosn.z < coors.z;
        if (m_nDoorState == GARAGE_DOOR_CLOSED) {
            if (sqDist < 3600.0f && heightOk) {
                auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                NeatlyLineUpStoredCars(cars);
                if (RestoreCarsForThisImpoundingGarage(cars)) {
                    m_nDoorState = GARAGE_DOOR_OPEN;
                }
            }
        } else if (m_nDoorState < GARAGE_DOOR_OPENING) {
            if (sqDist > 4225.0f || !heightOk || m_nDoorState == GARAGE_DOOR_CLOSING) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                StoreAndRemoveCarsForThisImpoundingGarage(CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType)), 3);
            }
        }
        return;
    }

    case TUNING_LOCO_LOW_CO:
    case TUNING_WHEEL_ARCH_ANGELS:
    case TUNING_TRANSFENDER:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            auto* const vehicle = FindPlayerVehicle(-1, false);
            if (!RightModTypeForThisGarage(vehicle)) {
                return;
            }
            if (SqDistToRect(vehicle->GetPosition()) >= 64.0f) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;
            return;
        }
        case GARAGE_DOOR_OPEN: {
            if (SqDistPlayerToCenter() <= 900.0f) {
                return;
            }
            if ((CTimer::GetFrameCounter() & 0x1F) != 0) {
                return;
            }
            auto* const vehicle = FindPlayerVehicle(-1, false);
            if (RightModTypeForThisGarage(vehicle) && IsEntityTouching3D(vehicle)) {
                return;
            }
            if (IsAnyOtherCarTouchingGarage(nullptr)) {
                return;
            }
            m_b0x1       = true;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        case GARAGE_DOOR_CLOSING:
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(vehicle);
            }
            ProcessClosedByAnimation();
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            break; // The original runs into the BURGLARY code from here
        }
        [[fallthrough]];

    case BURGLARY:
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN:
            if (SqDistPlayerToCenter() <= 900.0f || IsAnyOtherCarTouchingGarage(nullptr)) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        case GARAGE_DOOR_CLOSING:
            if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(vehicle);
            }
            ProcessClosedByAnimation();
            return;
        case GARAGE_DOOR_OPENING:
            ProcessOpening();
            return;
        default:
            return;
        }

    default:
        return;
    }
}

bool CGarage::IsHideOut() const {
    switch (m_nType) {
    case eGarageType::SAFEHOUSE_GANTON:
    case eGarageType::SAFEHOUSE_SANTAMARIA:
    case eGarageType::SAGEHOUSE_ROCKSHORE:
    case eGarageType::SAFEHOUSE_FORTCARSON:
    case eGarageType::SAFEHOUSE_VERDANTMEADOWS:
    case eGarageType::SAFEHOUSE_DILLIMORE:
    case eGarageType::SAFEHOUSE_PRICKLEPINE:
    case eGarageType::SAFEHOUSE_WHITEWOOD:
    case eGarageType::SAFEHOUSE_PALOMINOCREEK:
    case eGarageType::SAFEHOUSE_REDSANDSWEST:
    case eGarageType::SAFEHOUSE_ELCORONA:
    case eGarageType::SAFEHOUSE_MULHOLLAND:
    case eGarageType::SAFEHOUSE_CALTONHEIGHTS:
    case eGarageType::SAFEHOUSE_PARADISO:
    case eGarageType::SAFEHOUSE_DOHERTY:
    case eGarageType::SAFEHOUSE_HASHBURY:
    case eGarageType::HANGAR_ABANDONED_AIRPORT:
        return true;
    default:
        return false;
    }
}

// 0x44A9C0
bool CGarage::IsGarageEmpty() {
    CVector cornerA = { m_fLeftCoord, m_fFrontCoord, m_vPosn.z };
    CVector cornerB = { m_fRightCoord, m_fBackCoord, m_fTopZ   };

    int16 outCount[2];
    CEntity* outEntities[16];
    CWorld::FindObjectsIntersectingCube(cornerA, cornerB, outCount, static_cast<int16>(std::size(outEntities)), outEntities, false, true, true, false, false);
    if (outCount[0] <= 0)
        return true;

    int16 entityIndex = 0;

    while (!IsEntityTouching3D(outEntities[entityIndex])) {
        if (++entityIndex >= outCount[0])
            return true;
    }
    return false;
}

// 0x5D3020
void CSaveGarage::CopyGarageIntoSaveGarage(Const CGarage& g) {
    m_nType         = g.m_nType;
    m_nDoorState    = g.m_nDoorState;
    m_nFlags        = g.m_nFlags;
    m_vPosn         = g.m_vPosn;
    m_vDirectionA   = g.m_vDirectionA;
    m_vDirectionB   = g.m_vDirectionB;
    m_fTopZ         = g.m_fTopZ;
    m_fWidth        = g.m_fWidth;
    m_fHeight       = g.m_fHeight;
    m_fLeftCoord    = g.m_fLeftCoord;
    m_fRightCoord   = g.m_fRightCoord;
    m_fFrontCoord   = g.m_fFrontCoord;
    m_fBackCoord    = g.m_fBackCoord;
    m_fDoorPosition = g.m_fDoorPosition;
    m_nTimeToOpen   = g.m_nTimeToOpen;
    m_nOriginalType = g.m_nOriginalType;
    strcpy_s(m_anName, g.m_anName);
}

// 0x5D30C0
void CSaveGarage::CopyGarageOutOfSaveGarage(CGarage& g) const {
    g.m_nType         = m_nType;
    g.m_nDoorState    = m_nDoorState;
    g.m_nFlags        = m_nFlags;
    g.m_vPosn         = m_vPosn;
    g.m_vDirectionA   = m_vDirectionA;
    g.m_vDirectionB   = m_vDirectionB;
    g.m_fTopZ         = m_fTopZ;
    g.m_fWidth        = m_fWidth;
    g.m_fHeight       = m_fHeight;
    g.m_fLeftCoord    = m_fLeftCoord;
    g.m_fRightCoord   = m_fRightCoord;
    g.m_fFrontCoord   = m_fFrontCoord;
    g.m_fBackCoord    = m_fBackCoord;
    g.m_fDoorPosition = m_fDoorPosition;
    g.m_nTimeToOpen   = m_nTimeToOpen;
    g.m_nOriginalType = m_nOriginalType;
    g.m_pTargetCar    = nullptr;
    strcpy_s(g.m_anName, m_anName);
}

// todo move
// 0x449760
void CStoredCar::StoreCar(CVehicle* vehicle) {
    m_wModelIndex = static_cast<uint16>(vehicle->GetModelIndex());
    m_vPosn = vehicle->GetPosition();

    const auto& up = vehicle->GetUp();
    m_nPackedForwardX = static_cast<uint8>(std::lround(up.x * 100.0f));
    m_nPackedForwardY = static_cast<uint8>(std::lround(up.y * 100.0f));
    m_nPackedForwardZ = static_cast<uint8>(std::lround(up.z * 100.0f));

    m_nPrimaryColor    = vehicle->m_nPrimaryColor;
    m_nSecondaryColor  = vehicle->m_nSecondaryColor;
    m_nTertiaryColor   = vehicle->m_nTertiaryColor;
    m_nQuaternaryColor = vehicle->m_nQuaternaryColor;
    m_nRadioStation     = *reinterpret_cast<uint8*>(reinterpret_cast<char*>(vehicle) + 0x1D2); // NOTSA: CVehicle's radio station field isn't mapped yet
    m_nHandlingFlags    = *reinterpret_cast<uint32*>(reinterpret_cast<char*>(vehicle) + 0x38C); // NOTSA: opaque handling-flags bag, isn't mapped yet
    m_anCompsToUse[0]   = vehicle->m_anExtras[0];
    m_anCompsToUse[1]   = vehicle->m_anExtras[1];

    m_nStoredCarFlags = 0;
    if (vehicle->physicalFlags.bBulletProof)    m_nStoredCarFlags |= 1;
    if (vehicle->physicalFlags.bFireProof)      m_nStoredCarFlags |= 2;
    if (vehicle->physicalFlags.bExplosionProof) m_nStoredCarFlags |= 4;
    if (vehicle->physicalFlags.bCollisionProof) m_nStoredCarFlags |= 8;
    if (vehicle->physicalFlags.bMeleeProof)     m_nStoredCarFlags |= 0x10;
    if (vehicle->vehicleFlags.bUpgradedStereo)  m_nStoredCarFlags |= 0x20;
    if (m_nHandlingFlags & 0x20000)             m_nStoredCarFlags |= 0x40;
    if (m_nHandlingFlags & 0x80000)             m_nStoredCarFlags |= 0x80;

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        m_nBombType = vehicle->m_nBombOnBoard & 7;
    }

    for (auto i = 0u; i < std::size(m_awCarMods); i++) {
        m_awCarMods[i] = vehicle->m_anUpgrades[i];
    }

    m_nPaintJob    = static_cast<uint8>(vehicle->GetRemapIndex());
    m_nNitroBoosts = vehicle->m_nNitroBoosts;
}

// NOTSA: per-model "is this model (and its LOD/twin variant) streamed in" gate; stride 0x14 bytes, not otherwise mapped
static bool IsVehicleModelReady(int32 modelIndex) {
    return *reinterpret_cast<int8*>(0x8E4CD0 + modelIndex * 0x14) == 1;
}

// 0x447E40
CVehicle* CStoredCar::RestoreCar() {
    CStreaming::RequestModel(m_wModelIndex, 8);
    for (const auto modId : m_awCarMods) {
        if (modId != -1) {
            plugin::Call<0x408C70, int32, int32>(modId, 0); // NOTSA: requests the mod model + its LOD/twin variant (thunk_FUN_0156c970)
        }
    }

    if (!IsVehicleModelReady(m_wModelIndex)) {
        return nullptr;
    }

    for (const auto modId : m_awCarMods) {
        if (modId != -1 && !plugin::CallAndReturn<bool, 0x407820, int32>(modId)) { // NOTSA: verifies mod model + LOD/twin are loaded (thunk_FUN_0156a100)
            return nullptr;
        }
    }

    // NOTSA: staged globals the vehicle constructor reads for its initial extra-comps setup
    *reinterpret_cast<uint8*>(0x8A6459) = m_anCompsToUse[1];
    *reinterpret_cast<uint8*>(0x8A6458) = m_anCompsToUse[0];

    auto* const modelInfo = CModelInfo::GetVehicleModelInfo(m_wModelIndex);

    CVehicle* vehicle;
    switch (modelInfo->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK: vehicle = new CMonsterTruck(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_QUAD:   vehicle = new CQuadBike(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_HELI:   vehicle = new CHeli(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_PLANE:  vehicle = new CPlane(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_BOAT:   vehicle = new CBoat(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_BIKE:
        vehicle = new CBike(m_wModelIndex, RANDOM_VEHICLE);
        *reinterpret_cast<uint8*>(reinterpret_cast<char*>(vehicle) + 0x614) |= 0x10; // NOTSA: unmapped CBike field
        break;
    case VEHICLE_TYPE_BMX:
        vehicle = new CBmx(m_wModelIndex, RANDOM_VEHICLE);
        *reinterpret_cast<uint8*>(reinterpret_cast<char*>(vehicle) + 0x614) |= 0x10; // NOTSA: unmapped CBmx field
        break;
    case VEHICLE_TYPE_TRAILER: vehicle = new CTrailer(m_wModelIndex, RANDOM_VEHICLE); break;
    default:                   vehicle = new CAutomobile(m_wModelIndex, RANDOM_VEHICLE, true); break;
    }

    vehicle->GetPosition() = m_vPosn;

    const CVector up{
        static_cast<int8>(m_nPackedForwardX) * 0.01f,
        static_cast<int8>(m_nPackedForwardY) * 0.01f,
        static_cast<int8>(m_nPackedForwardZ) * 0.01f
    };
    auto& matrix = vehicle->GetMatrix();
    matrix.GetUp()      = up;
    matrix.GetRight()   = CVector(up.y, -up.x, 0.0f);
    matrix.GetForward() = CVector(0.0f, 0.0f, 1.0f);

    vehicle->SetStatus(STATUS_ABANDONED);

    vehicle->vehicleFlags.bFreebies             = false;
    vehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;
    *reinterpret_cast<uint8*>(reinterpret_cast<char*>(vehicle) + 0x1D2)  = m_nRadioStation;  // NOTSA: CVehicle radio station field not mapped
    auto& handlingFlagsOpaque = *reinterpret_cast<uint32*>(reinterpret_cast<char*>(vehicle) + 0x38C); // NOTSA: opaque handling-flags bag not mapped
    handlingFlagsOpaque = m_nHandlingFlags;

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        vehicle->m_nBombOnBoard = m_nBombType & 7;
    }

    vehicle->m_nDoorLock = CARLOCK_UNLOCKED;

    if (m_nStoredCarFlags & 1)    vehicle->physicalFlags.bBulletProof    = true;
    if (m_nStoredCarFlags & 2)    vehicle->physicalFlags.bFireProof      = true;
    if (m_nStoredCarFlags & 4)    vehicle->physicalFlags.bExplosionProof = true;
    if (m_nStoredCarFlags & 8)    vehicle->physicalFlags.bCollisionProof = true;
    if (m_nStoredCarFlags & 0x10) vehicle->physicalFlags.bMeleeProof     = true;
    if (m_nStoredCarFlags & 0x20) vehicle->vehicleFlags.bUpgradedStereo  = true;
    if (m_nStoredCarFlags & 0x40) handlingFlagsOpaque |= 0x20000;
    if (m_nStoredCarFlags & 0x80) handlingFlagsOpaque |= 0x80000;

    for (auto i = 0u; i < std::size(m_awCarMods); i++) {
        vehicle->m_anUpgrades[i] = m_awCarMods[i];
    }
    vehicle->SetupUpgradesAfterLoad();
    vehicle->SetRemap(static_cast<int8>(m_nPaintJob)); // sign-extend: -1 (0xFF) means "no remap"

    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->m_nNitroBoosts         = m_nNitroBoosts;
    vehicle->m_nPrimaryColor        = m_nPrimaryColor;
    vehicle->m_nSecondaryColor      = m_nSecondaryColor;
    vehicle->m_nTertiaryColor       = m_nTertiaryColor;
    vehicle->m_nQuaternaryColor     = m_nQuaternaryColor;
    vehicle->vehicleFlags.bDontSetColourWhenRemapping = true;

    return vehicle;
}
