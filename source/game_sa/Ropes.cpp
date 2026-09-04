/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"
#include "Timer.h"

void CRopes::InjectHooks() {
    RH_ScopedClass(CRopes);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x555DC0);
    RH_ScopedInstall(Shutdown, 0x556B10);
    RH_ScopedInstall(Update, 0x558D70);
    RH_ScopedInstall(Render, 0x556AE0);
    RH_ScopedInstall(RegisterRope, 0x556B40);
    RH_ScopedInstall(FindPickupHeight, 0x556760);
    RH_ScopedInstall(FindRope, 0x556000);
    RH_ScopedInstall(FindCoorsAlongRope, 0x555E40);
    RH_ScopedInstall(CreateRopeForSwatPed, 0x558D10);
    RH_ScopedInstall(IsCarriedByRope, 0x555F80);
    RH_ScopedInstall(SetSpeedOfTopNode, 0x555DF0);
}

// 0x555DC0
void CRopes::Init() {
    for (auto& rope : aRopes) {
        rope.m_nType = eRopeType::NONE;
    }
    PlayerControlsCrane = eControlledCrane::NONE;
}

// 0x556B10
void CRopes::Shutdown() {
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE)
            continue;

        rope.Remove();
    }
}

// 0x558D70
void CRopes::Update() {
    ZoneScoped;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Update();
    }
}

// 0x556AE0
void CRopes::Render() {
    ZoneScoped;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Render();
    }
}

// Must be used in loop to make attached to holder
// 0x556B40
bool CRopes::RegisterRope(uint32 ropeID, uint32 ropeType, CVector startPos, bool bExpires, uint8 segmentCount, uint8 flags, CPhysical* holder, uint32 timeExpire) {
    // If a rope with this ID is already registered, just collapse all its segments back to the start position
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE || rope.m_nId != ropeID) {
            continue;
        }

        rope.m_nFlags2 |= 1;
        rope.m_nSegments = segmentCount;
        for (auto i = 0u; i <= rope.m_nSegments; i++) { // NOTSA: `<=` matches original
            rope.m_aSegments[i] = startPos;
            rope.m_aSpeed[i] = CVector{};
        }
        rope.CreateHookObjectForRope();
        return true;
    }

    // Otherwise register it into the first free slot
    const auto freeRope = std::ranges::find(aRopes, eRopeType::NONE, &CRope::m_nType);
    if (freeRope == aRopes.end()) {
        return false;
    }
    auto& rope = *freeRope;

    rope.m_nId               = ropeID;
    rope.m_aSegments[0]      = startPos;
    rope.m_aSpeed[0]         = CVector{};
    rope.m_nSegments         = segmentCount;
    rope.m_nFlags2           = (uint8)((flags & 1) << 2) | (rope.m_nFlags2 & 0xF9) | 1; // NOTSA: unclear leftover flag bits, see m_nFlags2's header comment
    rope.m_fGroundZ          = 0.0f;
    rope.m_pAttachedEntity   = nullptr;
    rope.m_pRopeAttachObject = nullptr;
    rope.m_fSegmentLength    = (holder && holder->GetIsTypeVehicle()) ? 0.9f : 0.5f;
    rope.m_nFlags1           = 0;
    rope.m_pRopeHolder       = holder;
    rope.m_nType             = (eRopeType)ropeType;
    if (holder) {
        CEntity::RegisterReference(rope.m_pRopeHolder);
    }
    rope.m_nTime = bExpires ? CTimer::m_snTimeInMilliseconds + timeExpire : 0;

    switch (rope.m_nType) {
    case eRopeType::MAGNET:
        rope.m_fMass = 10.0f;
        rope.m_fTotalLength = 0.3225806f;
        break;
    case eRopeType::CRANE_MAGNO:
        rope.m_fMass = 50.0f;
        rope.m_fTotalLength = 1.612903f;
        break;
    case eRopeType::WRECKING_BALL:
    case eRopeType::QUARRY_CRANE_ARM:
    case eRopeType::CRANE_TROLLEY:
        rope.m_fMass = 68.0f;
        rope.m_fTotalLength = 2.193548f;
        break;
    default:
        rope.m_fMass = 20.0f;
        rope.m_fTotalLength = 0.6451613f;
        break;
    }

    // Cranes with a heavy hanging load dangle straight down; other rope types zigzag horizontally for visual slack
    const auto hangsStraight = rope.m_nType >= eRopeType::CRANE_MAGNO && rope.m_nType <= eRopeType::CRANE_TROLLEY;
    for (auto i = 1u; i < NUM_ROPE_SEGMENTS; i++) {
        const auto& prev = rope.m_aSegments[i - 1];
        rope.m_aSegments[i] = hangsStraight
            ? CVector{ prev.x, prev.y, prev.z - rope.m_fTotalLength }
            : CVector{ prev.x + ((i & 1) ? rope.m_fTotalLength : -rope.m_fTotalLength), prev.y, prev.z };
        rope.m_aSpeed[i] = CVector{};
    }

    rope.CreateHookObjectForRope();
    return true;
}

// 0x556760
float CRopes::FindPickupHeight(CEntity* entity) {
    return CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
}

// Returns id to array
// 0x556000
int32 CRopes::FindRope(uint32 id) {
    for (auto ropeId = 0; ropeId < MAX_NUM_ROPES; ropeId++) {
        if (aRopes[ropeId].m_nType != eRopeType::NONE && aRopes[ropeId].m_nId == id)
            return ropeId;
    }
    return -1;
}

// a4 always nullptr
// 0x555E40
bool CRopes::FindCoorsAlongRope(uint32 ropeId, float fDistAlongRope, CVector* outPosn, CVector* outSpeed) {
    const auto ropeIdx = FindRope(ropeId);
    if (ropeIdx == -1) {
        return false;
    }

    auto& rope = aRopes[ropeIdx];
    const auto node = (int32)fDistAlongRope;
    const auto t     = fDistAlongRope - (float)node;

    *outPosn = rope.m_aSegments[node] * (1.0f - t) + rope.m_aSegments[node + 1] * t;

    if (outSpeed) {
        *outSpeed = rope.m_aSpeed[node + 1];
    }
    return true;
}

// 0x558D10
int32 CRopes::CreateRopeForSwatPed(const CVector& startPos) {
    int32 newRopeId = m_nRopeIdCreationCounter + 100;
    if (RegisterRope(newRopeId, static_cast<uint32>(eRopeType::SWAT), startPos, true, 0, 0, nullptr, 4000)) {
        return -1;
    }

    m_nRopeIdCreationCounter += 1;
    return newRopeId;
}

// 0x555F80
bool CRopes::IsCarriedByRope(CPhysical* entity) {
    if (!entity)
        return false;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_pRopeAttachObject == entity)
            return true;
    }
    return false;
}

// 0x555DF0
void CRopes::SetSpeedOfTopNode(uint32 ropeId, CVector dirSpeed) {
    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_nId == ropeId) {
            rope.m_aSpeed[0] = dirSpeed;
            return;
        }
    }
}
