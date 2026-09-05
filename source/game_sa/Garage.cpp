#include "StdInc.h"

#include "Garage.h"
#include "Object.h"

void CGarage::InjectHooks() {
    RH_ScopedClass(CGarage);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(BuildRotatedDoorMatrix, 0x4479F0);
    // RH_ScopedInstall(TidyUpGarageClose, 0x449D10);
    // RH_ScopedInstall(TidyUpGarage, 0x449C50);
    RH_ScopedInstall(StoreAndRemoveCarsForThisHideOut, 0x449900);
    RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690, { .reversed = false });
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
    RH_ScopedInstall(NeatlyLineUpStoredCars, 0x448330, { .reversed = false });
    RH_ScopedInstall(CenterCarInGarage, 0x449220, { .reversed = false });
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
    plugin::CallMethod<0x449D10, CGarage*>(this);
}

// 0x449C50
void CGarage::TidyUpGarage() {
    plugin::CallMethod<0x449C50, CGarage*>(this);
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
    plugin::CallMethod<0x449690, CGarage*>(this);
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
    plugin::CallMethod<0x448330, CGarage*, CStoredCar*>(this, car);
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
    plugin::CallMethod<0x449760, CStoredCar*, CVehicle*>(this, vehicle);
}

// 0x447E40
CVehicle* CStoredCar::RestoreCar() {
    return plugin::CallMethodAndReturn<CVehicle*, 0x447E40, CStoredCar*>(this);
}
