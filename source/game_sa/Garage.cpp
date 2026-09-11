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
    RH_ScopedInstall(StoreAndRemoveCarsForThisImpoundingGarage, 0x449A50);
    RH_ScopedInstall(RestoreCarsForThisHideOut, 0x156D8D0);
    RH_ScopedInstall(RestoreCarsForThisImpoundingGarage, 0x1561490);
    RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690);
    RH_ScopedInstall(IsPlayerOutsideGarage, 0x1569180);
    RH_ScopedInstall(CountCarsWithCenterPointWithinGarage, 0x1561400);
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
    RH_ScopedInstall(CalcDistToGarageRectangleSquared, 0x447D80);
    RH_ScopedInstall(SlideDoorOpen, 0x44A660);
    RH_ScopedInstall(SlideDoorClosed, 0x44A750);
    RH_ScopedInstall(FindDoorsWithGarage, 0x449FF0);
    RH_ScopedInstall(NeatlyLineUpStoredCars, 0x448330);
    RH_ScopedInstall(CenterCarInGarage, 0x449220);
    RH_ScopedInstall(IsGarageEmpty, 0x44A9C0);
    RH_ScopedInstall(IsAnyCarBlockingDoor, 0x156D610);
    RH_ScopedInstall(IsAnyOtherCarTouchingGarage, 0x1566680);
    RH_ScopedInstall(RightModTypeForThisGarage, 0x1565260);
    RH_ScopedInstall(Update, 0x44AA50);
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
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const vehicle = pool->GetAt(i - 1);
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
    auto* const pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i; i--) {
        auto* const vehicle = pool->GetAt(i - 1);
        if (!vehicle) {
            continue;
        }
        if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }
        if (!IsPointInsideGarage(vehicle->GetPosition())) {
            continue;
        }
        if (vehicle->GetStatus() != STATUS_WRECKED && vehicle->GetForward().z < 0.5f) {
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

// 0x156D8D0 (thunk_FUN_0156d8d0 - a genuine benign function, not SecuROM)
bool CGarage::RestoreCarsForThisHideOut(CStoredCar* car) {
    for (auto i = 0; i < 4; i++) {
        if (!car[i].HasCar()) {
            continue;
        }
        if (auto* const vehicle = car[i].RestoreCar()) {
            vehicle->vehicleFlags.bImpounded = false;
            CWorld::Add(vehicle);

            // NOTSA: `vehicle+0x594` gates this too - exact field/meaning not identified this
            // session (also unresolved in Bike.cpp), kept as a raw offset check + raw forward.
            if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(vehicle) + 0x594) == 0) {
                plugin::Call<0x401C57>();
            } else if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(vehicle) + 0x594) == 9) {
                plugin::Call<0x6BEEB0, CVehicle*>(vehicle);
            }

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

// 0x1561490 (thunk_FUN_01561490 - a genuine benign function, not SecuROM)
bool CGarage::RestoreCarsForThisImpoundingGarage(CStoredCar* car) {
    for (auto i = 0; i < 3; i++) {
        if (!car[i].HasCar()) {
            continue;
        }
        if (auto* const vehicle = car[i].RestoreCar()) {
            vehicle->vehicleFlags.bImpounded = true;
            CWorld::Add(vehicle);

            // NOTSA: see the identical, still-unresolved `+0x594` check in RestoreCarsForThisHideOut above.
            if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(vehicle) + 0x594) == 0) {
                plugin::Call<0x401C57>();
            } else if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(vehicle) + 0x594) == 9) {
                plugin::Call<0x6BEEB0, CVehicle*>(vehicle);
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

// 0x1569180 (thunk_FUN_01569180 - a genuine benign function, not SecuROM)
bool CGarage::IsPlayerOutsideGarage(float fRadius) {
    if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
        return IsEntityEntirelyOutside(playerVehicle, fRadius);
    }
    return IsEntityEntirelyOutside(FindPlayerPed(-1), fRadius);
}

// 0x1561400 (thunk_FUN_01561400 - a genuine benign function, not SecuROM)
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

    return IsEntityTouching3D(playerVehicle);
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
    const auto speed = CTimer::ms_fTimeStep * (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT ? 0.0011f : 0.011f);
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
    const auto speed = CTimer::ms_fTimeStep * (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT ? 0.0013f : 0.013f);
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
        car[i].m_nPackedForwardX = static_cast<uint8>(std::lround(-dir.x * 100.0f));
        car[i].m_nPackedForwardY = 0;
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
            const auto push = Normalized(CVector{ pos.x - centerX, pos.y - centerY, 0.0f }) * PUSH_FORCE * CTimer::ms_fTimeStep;
            otherVehicle->GetMoveSpeed() += push;
            break; // only the first out-of-bounds sphere per vehicle triggers a push
        }
    }
}

// 0x156D610
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

// 0x1566680
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

// 0x1565260
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
    if (m_nType != 13 && m_nDoorState < 6 && FindPlayerPed() && !m_bCameraFollowsPlayer) {
        auto* const playerVehicle = FindPlayerVehicle(-1, false);
        auto* candidateEntity = static_cast<CEntity*>(FindPlayerPed());
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

    switch (m_nType) {
    case ONLY_TARGET_VEH:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED:
            if (FindPlayerVehicle(-1, false) != m_pTargetCar || !m_pTargetCar) {
                return;
            }
            if (CalcDistToGarageRectangleSquared(m_pTargetCar->GetPosition().x, m_pTargetCar->GetPosition().y) >= 64.0f) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;
            return;
        case GARAGE_DOOR_OPEN: {
            const auto& pos = FindPlayerCoors(-1);
            const auto dx = pos.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
            const auto dy = pos.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
            if (dx * dx + dy * dy <= 900.0f) {
                if (FindPlayerVehicle(-1, false) == m_pTargetCar) {
                    return;
                }
                if (!m_pTargetCar) {
                    return;
                }
                if (!IsEntityEntirelyInside3D(m_pTargetCar, 0.0f)) {
                    return;
                }
                auto* const otherVehicle = FindPlayerVehicle(-1, false);
                CEntity* const subject = otherVehicle ? static_cast<CEntity*>(otherVehicle) : static_cast<CEntity*>(FindPlayerPed());
                if (IsEntityEntirelyOutside(subject, 2.0f)) {
                    CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                    FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                    m_nFlags &= ~1;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    return;
                }
                return;
            }
            if ((CTimer::m_FrameCounter & 0x1f) != 0) {
                return;
            }
            if (m_pTargetCar && IsEntityTouching3D(m_pTargetCar)) {
                return;
            }
            m_nFlags |= 1;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        case GARAGE_DOOR_CLOSING:
            if (m_pTargetCar) {
                CenterCarInGarage(m_pTargetCar);
            }
            if (!SlideDoorClosed()) {
                return;
            }
            if (m_nFlags & 1) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                return;
            }
            if (m_pTargetCar) {
                m_nDoorState = GARAGE_DOOR_CLOSED_DROPPED_CAR;
                m_pTargetCar->DestroyVehicleAndDriverAndPassengers(m_pTargetCar);
                m_pTargetCar = nullptr;
            } else {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
            FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
            return;
        case GARAGE_DOOR_OPENING:
            if (SlideDoorOpen()) {
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            }
            return;
        default:
            break;
        }
        [[fallthrough]];
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CTimer::m_snTimeInMilliseconds <= m_nTimeToOpen) {
                return;
            }
            if (m_nType == BOMBSHOP_REMOTE /* NOTSA: && !DAT_008e6940-equivalent model-ready flag, not yet mapped */) {
                // NOTSA: original checks a global "detonator model streamed" flag here; if not set,
                // requests MODEL_BOMB and returns to wait. Treated as always-ready (flag assumed true)
                // pending that global being mapped -- SAFE simplification since RequestModel below
                // still runs unconditionally for every bombshop type.
            }
            CStreaming::RequestModel(MODEL_BOMB, 2);

            eAudioEvents soundEvent;
            switch (m_nType) {
            case BOMBSHOP_TIMED:  soundEvent = (eAudioEvents)0x12; break;
            case BOMBSHOP_ENGINE: soundEvent = (eAudioEvents)0x13; break;
            default:              soundEvent = (eAudioEvents)0x14; break;
            }
            AudioEngine.ReportFrontendAudioEvent(soundEvent, 0, 1.0f);
            m_nDoorState = GARAGE_DOOR_OPENING;

            if (!CGarages::BombsAreFree) {
                auto& money = FindPlayerInfo().m_nMoney;
                if (money > 0) {
                    money = std::max(money - 500, 0);
                }
            }

            if (auto* const playerVehicle = FindPlayerVehicle(-1, false);
                playerVehicle && (playerVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || playerVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE)) {
                playerVehicle->m_nBombOnBoard = (m_nType - 1) & 7;
                playerVehicle->m_pWhoInstalledBombOnMe = FindPlayerPed();
                if (m_nType == BOMBSHOP_REMOTE) {
                    // NOTSA: thunk_FUN_01569b90() - no visible args, decompiled body not yet checked; forward raw
                    plugin::Call<0x1569B90>();
                }
                CStats::IncrementStat((eStats)0x7f, 10.0f);
            }

            const char* msgKey = nullptr;
            switch (m_nType) {
            case BOMBSHOP_TIMED:
                if (CPad::GetPad(0)->JustOutOfFrontEnd < 3) { // NOTSA: field guessed, needs verification -- see note
                    msgKey = "GA_6";
                } else if (CPad::GetPad(0)->JustOutOfFrontEnd == 3) {
                    msgKey = "GA_6B";
                } else {
                    return;
                }
                break;
            case BOMBSHOP_ENGINE:
                if (CPad::GetPad(0)->JustOutOfFrontEnd < 3) {
                    msgKey = "GA_7";
                } else if (CPad::GetPad(0)->JustOutOfFrontEnd == 3) {
                    msgKey = "GA_7B";
                } else {
                    return;
                }
                break;
            default:
                msgKey = "GA_8";
                break;
            }
            CHud::SetHelpMessage(TheText.Get(msgKey));
            return;
        }
        case GARAGE_DOOR_OPEN: {
            if (!IsStaticPlayerCarEntirelyInside()) {
                return;
            }
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (!playerVehicle || playerVehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE || playerVehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                break;
            }
            if ((playerVehicle->m_nBombOnBoard & 7) != 0) {
                CGarages::TriggerMessage("GA_5", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent((eAudioEvents)0x11, 0, 1.0f);
                return;
            }
            if (!CGarages::BombsAreFree && FindPlayerInfo().m_nMoney < 500) {
                CGarages::TriggerMessage("GA_4", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent((eAudioEvents)0xe, 0, 1.0f);
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSING;
            CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
            playerVehicle->m_fDirtLevel = 0.0f;
            return;
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(playerVehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                m_nTimeToOpen = CTimer::m_snTimeInMilliseconds + 2000;
            }
            if (m_nType != BOMBSHOP_REMOTE) {
                break;
            }
            return; // NOTSA: original re-checks remote-detonator model streaming here (goto LAB_0044b5ae); simplified to a plain return, see BOMBSHOP door-state-0 note
        }
        case GARAGE_DOOR_OPENING:
            if (SlideDoorOpen()) {
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            }
            if (m_fDoorPosition > 0.5f) {
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
                return;
            }
            break;
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT:
            // thunk_FUN_01569180 confirmed = IsPlayerOutsideGarage (used correctly elsewhere in
            // this file already) - an earlier pass here left this as an unresolved raw forward
            // with a wrong guess in the comment ("bomb-fit transaction"); fixed.
            if (IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPEN;
            }
            return;
        }
        break; // ONLY_TARGET_VEH/BOMBSHOP_*: falls to TAIL #1 (CallOffChaseForArea) below

    case UNKN_CLOSESONTOUCH:
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN:
            if (IsGarageEmpty()) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            return;
        case GARAGE_DOOR_CLOSING:
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            if (!IsGarageEmpty()) {
                m_nDoorState = GARAGE_DOOR_OPENING;
            }
            return;
        default:
            break; // CLOSED/OPENING/WAITING/CLOSED_DROPPED_CAR -> TAIL #2 below, NOT CallOffChaseForArea
        }
        goto tail2;

    case SCRIPT_ONLY_OPEN:
        goto tail2; // this type has NO logic of its own beyond tail #2 (see below)

    case OPEN_FOR_TARGET_FREEZE_PLAYER:
    case CLOSE_WITH_CAR_DONT_OPEN_AGAIN:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (playerVehicle == m_pTargetCar && m_pTargetCar &&
                CalcDistToGarageRectangleSquared(playerVehicle->GetPosition().x, playerVehicle->GetPosition().y) < 289.0f) {
                m_nDoorState = GARAGE_DOOR_OPENING;
                return;
            }
            break; // falls to TAIL #1 below
        }
        case GARAGE_DOOR_OPEN: {
            const auto& playerCoors = FindPlayerCoors(-1);
            const auto dx = playerCoors.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
            const auto dy = playerCoors.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
            if (dx * dx + dy * dy <= 900.0f && m_pTargetCar) { // 30^2, same radius as ONLY_TARGET_VEH's OPEN check
                if (FindPlayerVehicle(-1, false) != m_pTargetCar) {
                    return;
                }
                if (IsStaticPlayerCarEntirelyInside() && !IsAnyCarBlockingDoor()) {
                    CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                    FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                    m_b0x1 = false;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
                return;
            }
            // Not close enough / no target car: same fallback ONLY_TARGET_VEH's OPEN case uses
            // when the player leaves too far away (LAB_0044c662).
            m_b0x1 = true;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        case GARAGE_DOOR_CLOSING:
            if (m_pTargetCar) {
                CenterCarInGarage(m_pTargetCar);
            }
            if (!SlideDoorClosed()) {
                return;
            }
            if (!m_b0x1) {
                if (m_pTargetCar) {
                    m_nDoorState = GARAGE_DOOR_CLOSED_DROPPED_CAR;
                    m_nTimeToOpen = CTimer::m_snTimeInMilliseconds + 2000;
                    m_pTargetCar = nullptr;
                } else {
                    m_nDoorState = GARAGE_DOOR_CLOSED;
                }
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSED;
            return;
        case GARAGE_DOOR_OPENING:
            goto tail2; // confirmed: this door-state explicitly uses TAIL #2, not TAIL #1
        case GARAGE_DOOR_CLOSED_DROPPED_CAR:
            // Only meaningful for OPEN_FOR_TARGET_FREEZE_PLAYER specifically (CLOSE_WITH_CAR_DONT_
            // OPEN_AGAIN never reopens on its own, matching its name) - waits out the same 2000ms
            // timer set above, then reopens.
            if (m_nType == OPEN_FOR_TARGET_FREEZE_PLAYER && CTimer::m_snTimeInMilliseconds >= m_nTimeToOpen) {
                m_nDoorState = GARAGE_DOOR_OPENING;
                return;
            }
            break;
        }
        break; // TAIL #1

    // Generic hideout/safehouse/hangar group (SAFEHOUSE_* + HANGAR_*).
    case SAFEHOUSE_GANTON:       case SAFEHOUSE_SANTAMARIA:   case SAGEHOUSE_ROCKSHORE:
    case SAFEHOUSE_FORTCARSON:   case SAFEHOUSE_VERDANTMEADOWS: case SAFEHOUSE_DILLIMORE:
    case SAFEHOUSE_PRICKLEPINE:  case SAFEHOUSE_WHITEWOOD:    case SAFEHOUSE_PALOMINOCREEK:
    case SAFEHOUSE_REDSANDSWEST: case SAFEHOUSE_ELCORONA:     case SAFEHOUSE_MULHOLLAND:
    case SAFEHOUSE_CALTONHEIGHTS: case SAFEHOUSE_PARADISO:    case SAFEHOUSE_DOHERTY:
    case SAFEHOUSE_HASHBURY:     case HANGAR_AT400:
    case HANGAR_ABANDONED_AIRPORT:
    // NOTE: BURGLARY(43) is NOT part of this switch group, despite an earlier session's draft
    // assuming so - re-verified via raw decompile on 2026-09-07: every "!= ','" / "== ','" check
    // in this whole group compares against 0x2c=44=HANGAR_AT400, not 0x2b=43=BURGLARY. BURGLARY
    // has its own separate case (falls through from the TUNING_* group) - see further below.
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            const auto& playerCoors = FindPlayerCoors(-1);
            if (playerCoors.z >= 950.0f) {
                return;
            }

            const auto distSq = CalcDistToGarageRectangleSquared(playerCoors.x, playerCoors.y);
            if (distSq >= 12.25f) {   // outside 3.5 units
                if (distSq >= 100.0f) { // outside 10 units entirely -> too far
                    return;
                }
                auto* const pv = FindPlayerVehicle(-1, false);
                if (!pv) {
                    return;
                }
                // NOTSA: `pv+0x594` still unidentified (same field flagged in Bike.cpp and in
                // RestoreCarsForThisHideOut/ImpoundingGarage - compared against 10 here, 0/9 there).
                if (*reinterpret_cast<int32*>(reinterpret_cast<char*>(pv) + 0x594) == 10) {
                    return;
                }
            }

            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            const auto slotThreshold = (m_nType != SAFEHOUSE_GANTON) ? 4 : 2;
            if (!playerVehicle || m_nType == HANGAR_AT400 || CGarages::CountCarsInHideoutGarage(m_nType) < slotThreshold) {
                if (m_nType != HANGAR_AT400) {
                    auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                    if (!RestoreCarsForThisHideOut(cars)) {
                        return; // still waiting on model streaming, retry next frame
                    }
                }
                m_nDoorState = GARAGE_DOOR_OPENING;
                return;
            }

            // Safehouse already has enough cars stored - instead of a restore-triggered open,
            // check whether the player's vehicle just got close to one of the garage's 2 door
            // objects (a plain squared-XY-distance proximity test against each door in turn, NOT
            // a dot-product "which side of the plane" test - that was an earlier, unverified guess;
            // corrected via raw disasm of decomp_garage_update.txt lines 829-954 on 2026-09-07).
            {
                CObject *door1, *door2;
                FindDoorsWithGarage(&door1, &door2);

                const auto isNearDoor = [&](CObject* door) {
                    if (!door) {
                        return false;
                    }
                    const auto d = door->GetPosition() - playerVehicle->GetPosition();
                    return d.x * d.x + d.y * d.y < 25.f; // 5 units, 0x858FE8
                };

                if (isNearDoor(door1) || isNearDoor(door2)) {
                    if (CTimer::m_snTimeInMilliseconds - CGarages::LastTimeHelpMessage >= 0x4651) { // 18001ms
                        const auto appearance = playerVehicle->GetVehicleAppearance();
                        if (appearance != VEHICLE_APPEARANCE_HELI && appearance != VEHICLE_APPEARANCE_PLANE) {
                            CHud::SetHelpMessage(TheText.Get("GA_21"));
                            CGarages::LastTimeHelpMessage = CTimer::m_snTimeInMilliseconds;
                        }
                    }
                }
            }
            break; // falls to TAIL #1
        }
        case GARAGE_DOOR_OPEN: {
            const auto& playerCoors = FindPlayerCoors(-1);
            const auto distSq = CalcDistToGarageRectangleSquared(playerCoors.x, playerCoors.y);

            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            const bool noValidVehicleNearby = !playerVehicle ||
                *reinterpret_cast<int32*>(reinterpret_cast<char*>(playerVehicle) + 0x594) == 10;

            const bool anyCarBlockingDoor = IsAnyCarBlockingDoor(); // still a raw forward, see its own definition

            if ((distSq > 225.0f || (distSq > 16.0f && noValidVehicleNearby)) && !anyCarBlockingDoor) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
                return;
            }

            if (playerVehicle) {
                const auto slotThreshold = (m_nType != SAFEHOUSE_GANTON) ? 4 : 2;
                if (CountCarsWithCenterPointWithinGarage(playerVehicle) >= slotThreshold && IsPlayerOutsideGarage(0.25f)) {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    return;
                }
            }

            if (distSq > 4900.0f) {
                m_nDoorState = GARAGE_DOOR_CLOSING;
                RemoveCarsBlockingDoorNotInside();
            }
            return;
        }
        case GARAGE_DOOR_CLOSING:
            SlideDoorClosed();
            if (IsPlayerOutsideGarage(0.0f)) {
                if (m_fDoorPosition != 0.0f) {
                    return; // still animating closed
                }
                m_nDoorState = GARAGE_DOOR_CLOSED;
                if (m_nType != HANGAR_AT400) {
                    auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                    StoreAndRemoveCarsForThisHideOut(cars, 4);
                }
                return;
            }
            // player re-entered mid-close
            m_nDoorState = GARAGE_DOOR_OPENING;
            return;
        case GARAGE_DOOR_OPENING:
            goto tail2; // confirmed: this door-state explicitly uses TAIL #2, not TAIL #1
        }
        break;

    case SCRIPT_CONTROLLED:
        // Fully script-driven (OpenThisGarage/CloseThisGarage) - Update() only finishes whichever
        // door animation is currently in progress; CLOSED/OPEN states do nothing at all.
        if (m_nDoorState == GARAGE_DOOR_CLOSING) {
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            return;
        }
        if (m_nDoorState == GARAGE_DOOR_OPENING) {
            goto tail2; // SlideDoorOpen-only tail (matches this door-state's own goto exactly)
        }
        return; // CLOSED/OPEN/other: no-op, matches switchD_0044ba69_caseD_4's plain return

    case STAY_OPEN_WITH_CAR_INSIDE:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED:
            // NOTSA: reuses ONLY_TARGET_VEH's door-state-CLOSED code verbatim (confirmed via
            // `goto switchD_0044b7fb_caseD_0` in the original) - duplicated here rather than
            // sharing a C++ label across unrelated case blocks.
            if (FindPlayerVehicle(-1, false) != m_pTargetCar || !m_pTargetCar) {
                return;
            }
            if (CalcDistToGarageRectangleSquared(m_pTargetCar->GetPosition().x, m_pTargetCar->GetPosition().y) >= 64.0f) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;
            return;
        case GARAGE_DOOR_OPEN: {
            // Same "player wandered too far from the garage center" shape as
            // OPEN_FOR_TARGET_FREEZE_PLAYER's OPEN case above, just gated on IsEntityEntirelyOutside
            // instead of IsAnyCarBlockingDoor/IsStaticPlayerCarEntirelyInside.
            const auto& playerCoors = FindPlayerCoors(-1);
            const auto dx = playerCoors.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
            const auto dy = playerCoors.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
            if (dx * dx + dy * dy > 900.0f && m_pTargetCar && IsEntityEntirelyOutside(m_pTargetCar, 0.0f)) {
                m_b0x1 = true;
                m_nDoorState = GARAGE_DOOR_CLOSING;
                return;
            }
            break; // falls to TAIL #1
        }
        case GARAGE_DOOR_CLOSING:
            if (m_pTargetCar) {
                CenterCarInGarage(m_pTargetCar);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            return;
        case GARAGE_DOOR_OPENING:
            goto tail2;
        }
        break;

    case SCRIPT_OPEN_FREEZE_WHEN_CLOSING:
        if (m_nDoorState == GARAGE_DOOR_OPEN) {
            if (!m_pTargetCar) {
                return;
            }
            if (!IsEntityEntirelyInside3D(m_pTargetCar, 0.0f)) {
                return;
            }
            if (IsAnyCarBlockingDoor()) { // still a raw forward, see its own definition
                return;
            }
            if (IsPlayerOutsideGarage(0.0f)) {
                CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                m_b0x1 = false;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            return;
        }
        if (m_nDoorState == GARAGE_DOOR_CLOSING) {
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
            }
            return;
        }
        goto tail2; // CLOSED/OPENING -> TAIL #2

    case IMPOUND_LS:
    case IMPOUND_SF:
    case IMPOUND_LV: {
        // NOTSA: no animated door here (no SlideDoorOpen/SlideDoorClosed calls at all in this
        // group) - the door just flips instantly between CLOSED and OPEN.
        const auto& playerCoors = FindPlayerCoors(-1);
        const auto distSq = CalcDistToGarageRectangleSquared(playerCoors.x, playerCoors.y);
        const bool heightOk = playerCoors.z < (m_fTopZ - 2.0f) && m_vPosn.z < playerCoors.z;

        if (m_nDoorState == GARAGE_DOOR_CLOSED) {
            if (distSq < 3600.0f && heightOk) { // within 60 units
                auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                NeatlyLineUpStoredCars(cars);
                if (RestoreCarsForThisImpoundingGarage(cars)) {
                    m_nDoorState = GARAGE_DOOR_OPEN;
                    return;
                }
            }
        } else if (m_nDoorState < GARAGE_DOOR_OPENING) { // OPEN(1) or CLOSING(2)
            if (distSq > 4225.0f || !heightOk || m_nDoorState == GARAGE_DOOR_CLOSING) { // beyond 65 units (hysteresis vs the 60-unit open radius)
                m_nDoorState = GARAGE_DOOR_CLOSED;
                auto* const cars = CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
                StoreAndRemoveCarsForThisImpoundingGarage(cars, 3);
                return;
            }
        }
        break; // TAIL #1
    }

    case PAYNSPRAY: {
        // Gate shared by every door-state: too high up (e.g. on a bridge/roof) -> skip entirely.
        const auto& playerCoorsGate = FindPlayerCoors(-1);
        if (playerCoorsGate.z >= 950.0f) {
            return;
        }

        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CGarages::NoResprays) {
                return;
            }
            if (CTimer::m_snTimeInMilliseconds <= m_nTimeToOpen) {
                break; // falls to TAIL #1
            }
            m_nDoorState = GARAGE_DOOR_OPENING;

            auto needsRespray = false;
            auto* const wanted = FindPlayerWanted(-1);
            const auto wasWanted = wanted->m_WantedLevel != eWantedLevel::WANTED_CLEAN;
            if (wasWanted) {
                needsRespray = true;
                wanted->ClearWantedLevelAndGoOnParole();
            }

            auto colourChanged = false;
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false);
                playerVehicle && (playerVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || playerVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE)) {
                if (playerVehicle->m_fHealth < 970.0f) {
                    needsRespray = true;
                }
                playerVehicle->m_fHealth = std::max(1000.0f, playerVehicle->m_fHealth);
                // NOTSA: the original also zeroes `vehicle+0x8E4` (CAutomobile) or `vehicle+0x7BC`
                // (other types) here - OMITTED, `+0x8E4` exceeds CBike's own size (a genuine OOB
                // write for bike-shaped vehicles in the original binary) and both fields reset minor
                // damage/collision-flag state, not core to respray correctness. See
                // garage_update_progress.md's 2026-09-05/09-07 notes for the full reasoning.
                playerVehicle->Fix();
                CStats::IncrementStat(STAT_VEHICLE_RESPRAYS, 1.0f);

                if (playerVehicle->GetUp().z < 0.0f) { // upside down - flip back onto its wheels
                    playerVehicle->GetUp()    = -playerVehicle->GetUp();
                    playerVehicle->GetRight() = -playerVehicle->GetRight();
                }

                // NOTSA: `vehicle+0x868` bit1 - unmapped flag, gates whether a colour change is
                // even considered here.
                if ((*reinterpret_cast<uint8*>(reinterpret_cast<char*>(playerVehicle) + 0x868) & 2) == 0
                    && plugin::CallMethodAndReturn<int32, 0x6D0B70, CVehicle*>(playerVehicle) < 0) { // FindCurrentColourRemapIndex-ish, see notes
                    uint8 r, g, b, a;
                    auto* const modelInfo = CModelInfo::GetModelInfo(playerVehicle->GetModelIndex());
                    plugin::CallMethod<0x4C8500, CBaseModelInfo*, uint8*, uint8*, uint8*, uint8*, int32>(modelInfo, &r, &g, &b, &a, 1);
                    if (r != playerVehicle->m_nPrimaryColor || g != playerVehicle->m_nSecondaryColor
                        || b != playerVehicle->m_nTertiaryColor || a != playerVehicle->m_nQuaternaryColor) {
                        colourChanged = true;
                    }
                    playerVehicle->m_nPrimaryColor    = r;
                    playerVehicle->m_nSecondaryColor  = g;
                    playerVehicle->m_nTertiaryColor   = b;
                    playerVehicle->m_nQuaternaryColor = a;
                    plugin::CallMethod<0x6D0C00, CVehicle*, int32>(playerVehicle, -1); // apply the new colour remap

                    // NOTSA: original also spawns 10 red "paint flash" spark particles here
                    // (FxPrtMult_c(1,0,0,0.6,0.7,1,0.4) + FxSystem_c::AddParticle, jittered around
                    // the vehicle's position) when `colourChanged` - cosmetic only, OMITTED pending
                    // exact verification of which stack locals feed which AddParticle parameter
                    // (ambiguous from the decompile alone). Every other effect of this branch
                    // (repair, colour, stats, messages) is unaffected by this omission.
                }

                // NOTSA: vehicle+0x4B0 - unmapped, reset to 0 after a completed respray pass.
                *reinterpret_cast<uint32*>(reinterpret_cast<char*>(playerVehicle) + 0x4b0) = 0;
                // NOTSA: vehicle+0x42E bit7 - the same "needs respray"-ish flag set in this garage
                // type's own CLOSING case above, cleared here on a completed respray pass.
                *reinterpret_cast<uint8*>(reinterpret_cast<char*>(playerVehicle) + 0x42e) &= 0x7f;
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
                CGarages::TriggerMessage((rand() & 1) ? "GA_16" : "GA_15", -1, 4000, -1);
            }

            m_bUsedRespray = true;
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (!playerVehicle) {
                break; // falls to TAIL #1
            }
            // NOTSA: vehicle+0x42F bit0 - unmapped flag, set once the respray pass has run.
            *reinterpret_cast<uint8*>(reinterpret_cast<char*>(playerVehicle) + 0x42f) |= 1;
            break; // falls to TAIL #1
        }
        case GARAGE_DOOR_OPEN: {
            if (CGarages::NoResprays) {
                return;
            }
            if (!IsStaticPlayerCarEntirelyInside()) {
                if (!IsPlayerOutsideGarage(0.0f)) {
                    FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                    CGarages::LastGaragePlayerWasIn = garageId;
                } else if (garageId == CGarages::LastGaragePlayerWasIn) {
                    FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
                }
            } else {
                auto* const playerVehicle = FindPlayerVehicle(-1, false);
                // NOTSA: thunk_FUN_0156b1c0 - "is this vehicle eligible for a respray": not a law
                // enforcement vehicle, `pv+0x594` (still unidentified elsewhere in this file) != 10,
                // and not one of the 4 excluded models (ambulance/bus/fire truck/coach - emergency
                // and public-service vehicles you can't repaint).
                const auto modelId = playerVehicle->GetModelId();
                const auto isEligibleForRespray = !playerVehicle->IsLawEnforcementVehicle()
                    && *reinterpret_cast<int32*>(reinterpret_cast<char*>(playerVehicle) + 0x594) != 10
                    && modelId != MODEL_AMBULAN && modelId != MODEL_BUS && modelId != MODEL_FIRETRUK && modelId != MODEL_COACH;

                if (!isEligibleForRespray) {
                    // NOTSA: `pv+0x594` still unidentified, see above.
                    const auto* pcKey = *reinterpret_cast<int32*>(reinterpret_cast<char*>(playerVehicle) + 0x594) == 10 ? "GA_1B" : "GA_1";
                    CGarages::TriggerMessage(pcKey, -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent((eAudioEvents)0xf, 0, 1.0f);
                } else if (FindPlayerInfo().m_nMoney < 100 && !CGarages::RespraysAreFree) {
                    CGarages::TriggerMessage("GA_3", -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent((eAudioEvents)0xe, 0, 1.0f);
                } else {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                    // NOTSA: vehicle+0x4B0 - unmapped, reset to 0 when entering the respray.
                    *reinterpret_cast<uint32*>(reinterpret_cast<char*>(playerVehicle) + 0x4b0) = 0;
                }
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = true;
                CGarages::LastGaragePlayerWasIn = garageId;
            }

            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (!playerVehicle) {
                return;
            }
            const auto& pos = playerVehicle->GetPosition();
            if (CalcDistToGarageRectangleSquared(pos.x, pos.y) >= 64.0f) {
                return;
            }
            break; // falls to TAIL #1
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(playerVehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                AudioEngine.ReportFrontendAudioEvent((eAudioEvents)0x10, 0, 1.0f);
                m_nTimeToOpen = CTimer::m_snTimeInMilliseconds + 2000;
                const auto kills = CStats::GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT);
                CStats::IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, kills);
                CStats::SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.f);
            }
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                // NOTSA: vehicle+0x42E bit7 - unmapped "needs respray"-ish flag (cleared in
                // door-state CLOSED on a successful respray, set here). The `+0x8E4` write the
                // original also does here was OMITTED - see the door-state-CLOSED safety note.
                *reinterpret_cast<uint8*>(reinterpret_cast<char*>(playerVehicle) + 0x42e) |= 0x80;
            }
            break; // falls to TAIL #1
        }
        case GARAGE_DOOR_OPENING:
            // Reused from ONLY_TARGET_VEH/BOMBSHOP_*'s own OPENING code via a cross-case `goto` in
            // the original - duplicated here rather than sharing a C++ label across case blocks.
            if (SlideDoorOpen()) {
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            }
            if (m_fDoorPosition > 0.5f) {
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
                FindPlayerWanted(-1)->m_bPoliceBackOffGarage = false;
                return;
            }
            break; // falls to TAIL #1
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT:
            // Also reused from ONLY_TARGET_VEH/BOMBSHOP_* via `goto`.
            if (IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPEN;
            }
            return;
        }
        break; // TAIL #1
    }

    case TUNING_LOCO_LOW_CO:
    case TUNING_WHEEL_ARCH_ANGELS:
    case TUNING_TRANSFENDER:
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (!RightModTypeForThisGarage(playerVehicle)) { // null-safe, see its own definition
                return;
            }
            const auto& pos = playerVehicle->GetPosition();
            if (CalcDistToGarageRectangleSquared(pos.x, pos.y) >= 64.0f) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;
            return;
        }
        case GARAGE_DOOR_OPEN: {
            const auto& playerCoors = FindPlayerCoors(-1);
            const auto dx = playerCoors.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
            const auto dy = playerCoors.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
            if (dx * dx + dy * dy <= 900.0f) { // 30^2, player still near the garage - stay open
                return;
            }
            if ((CTimer::m_FrameCounter & 0x1f) != 0) { // throttle to once every 32 frames
                return;
            }
            auto* const playerVehicle = FindPlayerVehicle(-1, false);
            if (RightModTypeForThisGarage(playerVehicle) && IsEntityTouching3D(playerVehicle)) {
                return; // player still has the right car for this shop and it's still here
            }
            if (IsAnyOtherCarTouchingGarage(nullptr)) { // still a raw forward, see its own definition
                return; // someone else's car is using the shop - wait
            }
            m_b0x1 = true;
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        case GARAGE_DOOR_CLOSING:
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(playerVehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            return;
        case GARAGE_DOOR_OPENING:
            goto tail2;
        }
        [[fallthrough]]; // unhandled door-state (shouldn't happen) falls into BURGLARY below,
                          // matching the original's own case-block fallthrough exactly.

    case BURGLARY:
        if (m_nDoorState == GARAGE_DOOR_OPEN) {
            const auto& playerCoors = FindPlayerCoors(-1);
            const auto dx = playerCoors.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
            const auto dy = playerCoors.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
            if (dx * dx + dy * dy <= 900.0f) {
                return;
            }
            if (IsAnyOtherCarTouchingGarage(nullptr)) {
                return;
            }
            m_nDoorState = GARAGE_DOOR_CLOSING;
            return;
        }
        if (m_nDoorState == GARAGE_DOOR_CLOSING) {
            // Same CLOSING logic as the TUNING_* group above (reused via `goto` in the original).
            if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
                CenterCarInGarage(playerVehicle);
            }
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
            }
            return;
        }
        goto tail2; // CLOSED/OPENING/other -> TAIL #2

    default:
        break; // INVALID(0) and any other unmapped m_nType - matches the original's own default fallthrough
    }
    // TAIL #1 (LAB_0044b3c1): used by ONLY_TARGET_VEH, BOMBSHOP_*, (presumably) PAYNSPRAY.
    // Do NOT assume this is the universal ending - see garage_update_progress.md part 5.
    CWorld::CallOffChaseForArea(m_fLeftCoord - 10.0f, m_fFrontCoord - 10.0f, m_fRightCoord + 10.0f, m_fBackCoord + 10.0f);
    return;

tail2:
    // TAIL #2 (LAB_0044bcb7/bcb8/switchD_0044ba69_caseD_3): used by SCRIPT_ONLY_OPEN,
    // UNKN_CLOSESONTOUCH's non-OPEN/CLOSING states, and others not yet mapped.
    // NOTE: plain `return` with NO CallOffChaseForArea call - confirmed via decomp_garage_update.txt
    // lines 1206-1222 (switchD_0044ba69_caseD_4: return;).
    if (m_nDoorState == GARAGE_DOOR_OPENING) {
        if (SlideDoorOpen()) {
            m_nDoorState = GARAGE_DOOR_OPEN;
        }
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
