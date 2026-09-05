#include "StdInc.h"

#include "Garage.h"
#include "Object.h"

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
    RH_ScopedInstall(IsEntityTouching3D, 0x448EE0);
    RH_ScopedInstall(IsEntityEntirelyOutside, 0x448D30);
    RH_ScopedInstall(IsStaticPlayerCarEntirelyInside, 0x44A830, { .reversed = false });
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
    RH_ScopedInstall(FindDoorsWithGarage, 0x449FF0, { .reversed = false });
    RH_ScopedInstall(NeatlyLineUpStoredCars, 0x448330);
    RH_ScopedInstall(CenterCarInGarage, 0x449220, { .reversed = false });
    RH_ScopedInstall(IsGarageEmpty, 0x44A9C0);
    // RH_ScopedInstall(Update, 0x44AA50);
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
    return plugin::CallMethodAndReturn<bool, 0x44A830, CGarage*>(this);
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
    plugin::CallMethod<0x449FF0, CGarage*, CObject**, CObject**>(this, ppFirstDoor, ppSecondDoor);
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

// 0x449220
void CGarage::CenterCarInGarage(CVehicle* vehicle) {
    plugin::CallMethod<0x449220, CGarage*, CVehicle*>(this, vehicle);
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
    plugin::CallMethod<0x44AA50, CGarage*>(this, garageId);
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
