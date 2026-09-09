#include "StdInc.h"
#include "InteriorGroup_c.h"
#include "Interior_c.h"
#include "InteriorManager_c.h"
#include "PathFind.h"
#include "Population.h"
#include "General.h"
#include "Entity/Ped/Ped.h"

// NOTSA: enables interior ped setup/update - purpose not fully derived, cross-referenced from
// Interior_c::Init's `m_box->m_type != 'c'` branch and here.
static auto& bInteriorPedsEnabled = StaticRef<bool>(0xBB3DC2);

// NOTSA: purpose not fully derived - 16-entry global flag array (one slot per m_peds[] index).
// InteriorGroup_c::Exit() clears slot `i` to false whenever m_peds[i]'s pointer was invalid, or the ped
// had died, right before removing it from the population - likely consulted by SetupPeds()/
// SetupHousePeds() (still unreversed) to know which slots need repopulating. Sits directly before this
// file's own bInteriorPedsEnabled flag and Interior_c.cpp's adjacent flags (0xBB3DAC/0xBB3DC4/0xBB3DC8/
// 0xBB3DE4) - all one small block of interior-related session flags.
static auto& g_abInteriorPedSlotActive = StaticRef<bool[16]>(0xBB3D9C);

void InteriorGroup_c::InjectHooks() {
    RH_ScopedClass(InteriorGroup_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x597FE0, { .reversed = false });
    //RH_ScopedInstall(Destructor, 0x597FF0, { .reversed = false });

    RH_ScopedInstall(Init, 0x5947E0);
    RH_ScopedInstall(Update, 0x5968E0);
    RH_ScopedInstall(SetupPeds, 0x596890);
    RH_ScopedInstall(UpdatePeds, 0x596830);
    RH_ScopedInstall(SetupHousePeds, 0x5965E0, { .reversed = false });
    RH_ScopedInstall(SetupPaths, 0x595590, { .reversed = false });
    RH_ScopedInstall(ArePathsLoaded, 0x595380);
    RH_ScopedInstall(Setup, 0x595320);
    RH_ScopedInstall(Exit, 0x595290);
    RH_ScopedInstall(ContainsInteriorType, 0x595250);
    RH_ScopedInstall(CalcIsVisible, 0x595200);
    RH_ScopedInstall(DereferenceAnims, 0x595160);
    RH_ScopedInstall(ReferenceAnims, 0x5950D0);
    RH_ScopedInstall(UpdateOfficePeds, 0x594E90, { .reversed = false });
    RH_ScopedInstall(RemovePed, 0x594E30);
    RH_ScopedInstall(SetupShopPeds, 0x594C10, { .reversed = false });
    RH_ScopedInstall(SetupOfficePeds, 0x594BF0);
    RH_ScopedInstall(GetEntity, 0x594BD0);
    RH_ScopedInstall(GetPed, 0x594B90);
    RH_ScopedInstall(FindClosestInteriorInfo, 0x594A50);
    RH_ScopedInstall(FindInteriorInfo, 0x594970);
    RH_ScopedInstall(GetNumInteriorInfos, 0x594920);
    RH_ScopedInstall(GetRandomInterior, 0x5948C0);
    RH_ScopedInstall(AddInterior, 0x594840);
}

// 0x5947E0
void InteriorGroup_c::Init(CEntity* entity, uint8 groupId) {
    for (auto& interior : m_interiors) {
        interior = nullptr;
    }
    for (auto i = 0; i < std::size(m_peds); i++) {
        m_peds[i]         = nullptr;
        m_pedsToRemove[i] = nullptr;
    }
    m_numInteriors        = 0;
    m_pathSetupComplete   = 0;
    m_updatePeds          = 0;
    m_isVisible           = false;
    m_lastIsVisible       = false;
    m_animBlockReferenced = 0;
    m_pEntity             = entity;
    m_groupId             = groupId;
}

// 0x5968E0
void InteriorGroup_c::Update() {
    CalcIsVisible();

    if (!m_pathSetupComplete) {
        SetupPaths();
    }
    if (m_pathSetupComplete && !m_updatePeds) {
        SetupPeds();
    }
    if (m_updatePeds) {
        UpdatePeds();
    }

    ReferenceAnims();
}

// 0x594840
int32 InteriorGroup_c::AddInterior(Interior_c* interior) {
    for (auto i = 0; i < std::size(m_interiors); i++) {
        if (!m_interiors[i]) {
            m_interiors[i] = interior;
            m_numInteriors++;
            return i;
        }
    }
    return -1;
}

// 0x596890
void InteriorGroup_c::SetupPeds() {
    if (!m_EnEx || !bInteriorPedsEnabled) {
        return;
    }
    switch ((eInteriorGroupType)m_groupType) {
    case eInteriorGroupType::HOUSE:  SetupHousePeds(); break;
    case eInteriorGroupType::SHOP:   SetupShopPeds();  break;
    case eInteriorGroupType::OFFICE: SetupOfficePeds(); break;
    }
    m_updatePeds = true;
}

// 0x596830
void InteriorGroup_c::UpdatePeds() {
    if (!m_EnEx || !bInteriorPedsEnabled) {
        return;
    }
    for (auto& ped : m_pedsToRemove) {
        if (!ped) {
            continue;
        }
        if (ped->IsPointerValid()) {
            RemovePed(ped);
        }
        ped = nullptr;
    }
    if (m_groupType == (uint8)eInteriorGroupType::OFFICE) {
        UpdateOfficePeds();
    }
}

// 0x5965E0
int32 InteriorGroup_c::SetupHousePeds() {
    return plugin::CallMethodAndReturn<int32, 0x5965E0, InteriorGroup_c*>(this);
}

// 0x595590
int8 InteriorGroup_c::SetupPaths() {
    return plugin::CallMethodAndReturn<int8, 0x595590, InteriorGroup_c*>(this);
}

// 0x595380
int8 InteriorGroup_c::ArePathsLoaded() {
    const auto& box    = m_pEntity->GetModelInfo()->GetColModel()->GetBoundingBox();
    const auto& matrix = m_pEntity->GetMatrix(); // lazy-allocates m_matrix if unset

    CVector worldMin{999999.0f, 999999.0f, 999999.0f};
    CVector worldMax{-999999.0f, -999999.0f, -999999.0f};

    // All 8 corners of the (local-space) collision bounding box, transformed to world space.
    for (auto i = 0; i < 8; i++) {
        const CVector corner{
            (i & 1) ? box.m_vecMax.x : box.m_vecMin.x,
            (i & 2) ? box.m_vecMax.y : box.m_vecMin.y,
            (i & 4) ? box.m_vecMax.z : box.m_vecMin.z,
        };
        const auto worldPt = matrix.TransformPoint(corner);

        worldMin.x = std::min(worldMin.x, worldPt.x);
        worldMin.y = std::min(worldMin.y, worldPt.y);
        worldMin.z = std::min(worldMin.z, worldPt.z);
        worldMax.x = std::max(worldMax.x, worldPt.x);
        worldMax.y = std::max(worldMax.y, worldPt.y);
        worldMax.z = std::max(worldMax.z, worldPt.z);
    }

    // Z extent is computed but never consumed - AreNodesLoadedForArea only checks XY.
    return ThePaths.AreNodesLoadedForArea(worldMin.x, worldMax.x, worldMin.y, worldMax.y);
}

// 0x595320
void InteriorGroup_c::Setup() {
    if (ContainsInteriorType(2)) {
        m_groupType = (uint8)eInteriorGroupType::HOUSE;
    } else if (ContainsInteriorType(0) || ContainsInteriorType(6)) {
        m_groupType = (uint8)eInteriorGroupType::SHOP;
    } else {
        m_groupType = ContainsInteriorType(1) ? (uint8)eInteriorGroupType::OFFICE : (uint8)-1;
    }
    ReferenceAnims();
}

// 0x595290
void InteriorGroup_c::Exit() {
    for (auto* interior : m_interiors) {
        if (!interior) {
            continue;
        }
        interior->Exit();
        g_interiorMan.ReturnInteriorToPool(interior);
    }

    for (auto i = 0; i < std::size(m_peds); i++) {
        auto*& ped = m_peds[i];
        if (!ped) {
            continue;
        }
        if (ped->IsPointerValid()) {
            if (!ped->IsAlive()) {
                g_abInteriorPedSlotActive[i] = false;
            }
            CPopulation::RemovePed(ped);
        } else {
            g_abInteriorPedSlotActive[i] = false;
        }
        ped = nullptr;
    }

    m_numPeds = 0;
    ThePaths.RemoveInterior(m_id); // NOTSA: original sign-extends m_id (MOVSX) into the uint32 param; harmless unless m_id ever exceeds 127
    DereferenceAnims(); // tail-called in the original (JMP, not CALL/RET)
}

// 0x595250
int8 InteriorGroup_c::ContainsInteriorType(int32 type) {
    for (auto* interior : m_interiors) {
        if (interior && interior->m_box->m_type == type) {
            return true;
        }
    }
    return false;
}

// 0x595200
int8 InteriorGroup_c::CalcIsVisible() {
    m_lastIsVisible = m_isVisible;
    m_isVisible = false;
    for (auto* interior : GetInteriors()) {
        if (interior->IsVisible()) {
            m_isVisible = true;
            break;
        }
    }
    return m_isVisible;
}

// 0x595160
// NOTSA: BUGFIX - was calling `AddAnimBlockRef` (copy-paste from ReferenceAnims below), but raw disasm of
// every branch of this function (0x595160) calls 0x4D3FD0 = CAnimManager::RemoveAnimBlockRef, never
// 0x4D3FB0 = AddAnimBlockRef - confirmed independently via direct instruction-level trace, not inferred.
void InteriorGroup_c::DereferenceAnims() {
    if (!m_animBlockReferenced) {
        return;
    }
    if (m_groupType <= (uint8)eInteriorGroupType::OFFICE) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(GetAnimBlockName()));
    } else {
        // NOTSA: matches original - genuinely calls RemoveAnimBlockRef with the raw `this` pointer as a
        // bogus index when m_groupType isn't HOUSE/SHOP/OFFICE (a leftover stack scratch slot that happens
        // to hold `this`); practically unreachable since m_groupType is always one of those 3.
        CAnimManager::RemoveAnimBlockRef((int32)(intptr_t)this);
    }
    m_animBlockReferenced = false;
}

// 0x5950D0
void InteriorGroup_c::ReferenceAnims() {
    if (m_animBlockReferenced) {
        return;
    }
    const auto animBlkIdx = CAnimManager::GetAnimationBlockIndex(GetAnimBlockName());
    if (CStreaming::IsModelLoaded(IFPToModelId(animBlkIdx))) {
        CAnimManager::AddAnimBlockRef(animBlkIdx);
        m_animBlockReferenced = true;
    } else {
        CStreaming::RequestModel(IFPToModelId(animBlkIdx), STREAMING_KEEP_IN_MEMORY);
    }
}

// 0x594E90
void InteriorGroup_c::UpdateOfficePeds() {
    plugin::CallMethod<0x594E90, InteriorGroup_c*>(this);
}

// 0x594E30
int8 InteriorGroup_c::RemovePed(CPed* ped) {
    for (auto i = 0; i < std::size(m_peds); i++) {
        if (m_peds[i] == ped) {
            CPopulation::RemovePed(ped);
            m_peds[i] = nullptr;
            m_numPeds--;
            return true;
        }
    }
    return false;
}

// 0x594C10
int32 InteriorGroup_c::SetupShopPeds() {
    return plugin::CallMethodAndReturn<int32, 0x594C10, InteriorGroup_c*>(this);
}

// 0x594BF0
void InteriorGroup_c::SetupOfficePeds() {
    CStreaming::StreamPedsForInterior(2);
    // NOTSA: pumps the low-level CD-audio streaming state machine (deep in the obfuscated
    // audio-streaming subsystem, same `thunk_FUN_0156xxxx` pattern seen elsewhere - not reversed).
    ((void(__cdecl*)(uint8))0x40EA15)(0);
    m_numPeds = 0;
}

// 0x594BD0
CEntity* InteriorGroup_c::GetEntity() {
    return m_pEntity;
}

// 0x594B90
CPed* InteriorGroup_c::GetPed(int32 idx) {
    return m_peds[idx];
}

// 0x594A50
bool InteriorGroup_c::FindClosestInteriorInfo(int32 infoType, CVector point, float maxDist, InteriorInfo_t** outInfo, Interior_c** outInterior, float* outDistSq) {
    const auto maxDistSq = maxDist * maxDist;

    InteriorInfo_t* closest         = nullptr;
    Interior_c*     closestInterior = nullptr;
    auto            closestDistSq   = 999999.0f;

    for (auto* interior : m_interiors) {
        if (!interior || !interior->IsPtInside(point)) {
            continue;
        }
        for (auto i = 0; i < interior->m_interiorInfosCount; i++) {
            auto& info = interior->m_interiorInfos[i];
            if ((infoType != -1 && info.Type != (eInteriorInfoType)infoType) || info.IsInUse) {
                continue;
            }
            const auto distSq = CVector::DistSqr(point, info.Pos);
            if (distSq < maxDistSq && distSq < closestDistSq) {
                closest         = &info;
                closestInterior = interior;
                closestDistSq   = distSq;
            }
        }
    }

    if (!closest) {
        return false;
    }
    *outInfo     = closest;
    *outInterior = closestInterior;
    *outDistSq   = closestDistSq;
    return true;
}

// 0x594970
// NOTSA: Ghidra originally showed this as a bogus 6-byte body (NOP; JMP 0x4075a5) - 0x4075a5 is a shared
// obfuscated-prologue trampoline: it does the function's real `SUB ESP,0x208` then computes an indirect
// return target as `[0x01560874] + [0x004556c6]` (verified by direct memory read == 0x594976, the byte
// right after the JMP) and RETs to it - functionally just an obfuscated prologue, not a real jump elsewhere.
bool InteriorGroup_c::FindInteriorInfo(eInteriorInfoType infoType, InteriorInfo_t** outInfo, Interior_c** outInterior) {
    // NOTSA: matches original - packs matches into two adjacent raw stack buffers sized for only 64
    // entries each even though up to 8 interiors * 16 infos = 128 could theoretically match at once - a
    // latent (practically unreachable) overflow in the original we don't replicate.
    InteriorInfo_t* foundInfos[8 * 16];
    Interior_c*     foundInteriors[8 * 16];
    int32           foundCount = 0;

    for (auto* interior : m_interiors) {
        if (!interior) {
            continue;
        }
        for (auto i = 0; i < interior->m_interiorInfosCount; i++) {
            auto& info = interior->m_interiorInfos[i];
            if (info.Type != infoType || info.IsInUse) {
                continue;
            }
            foundInfos[foundCount]     = &info;
            foundInteriors[foundCount] = interior;
            foundCount++;
        }
    }

    if (foundCount <= 0) {
        *outInfo     = nullptr;
        *outInterior = nullptr;
        return false;
    }

    const auto idx = CGeneral::GetRandomNumberInRange<int32>(0, foundCount);
    *outInfo     = foundInfos[idx];
    *outInterior = foundInteriors[idx];
    return true;
}

// 0x594920
int32 InteriorGroup_c::GetNumInteriorInfos(eInteriorInfoType infoType) {
    int32 count = 0;
    for (auto* interior : m_interiors) {
        if (!interior) {
            continue;
        }
        for (auto i = 0; i < interior->m_interiorInfosCount; i++) {
            if (interior->m_interiorInfos[i].Type == infoType) {
                count++;
            }
        }
    }
    return count;
}

// 0x5948C0
int32 InteriorGroup_c::GetRandomInterior() {
    const auto r = CGeneral::GetRandomNumberInRange<int32>(0, m_numInteriors);
    auto validIdx = 0;
    for (auto* interior : m_interiors) {
        if (interior) {
            if (validIdx == r) {
                return reinterpret_cast<int32>(interior);
            }
            validIdx++;
        }
    }
    return 0;
}

//! @notsa
const char* InteriorGroup_c::GetAnimBlockName() {
    switch ((eInteriorGroupType)m_groupType) {
    case eInteriorGroupType::HOUSE:  return "int_house";
    case eInteriorGroupType::SHOP:   return "int_shop";
    case eInteriorGroupType::OFFICE: return "int_office";
    default:                         NOTSA_UNREACHABLE();
    }
}
