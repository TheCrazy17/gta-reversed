#include "StdInc.h"
#include "FurnitureSubGroup_c.h"
#include "Furniture_c.h"
#include "FurnitureManager_c.h"
#include "Models/ModelInfo.h"
#include "Collision/ColModel.h"

static auto& g_currFurnitureId = StaticRef<uint32>(0xBAB378);

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0);
    RH_ScopedInstall(GetRandomId, 0x590FD0);
    RH_ScopedInstall(AddFurniture, 0x5C00C0);
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
    if (g_currFurnitureId >= g_furnitureStore.size()) {
        return false;
    }
    auto& furn = g_furnitureStore[g_currFurnitureId++];

    furn.m_nModelId = modelId;
    furn.m_nId      = id;

    const auto& bbox = CModelInfo::GetModelInfo(modelId)->GetColModel()->m_boundBox;
    const auto  extent = m_bCanSteal
        ? bbox.m_vecMax.y - bbox.m_vecMin.y
        : bbox.m_vecMax.y + 0.5f;

    // Round to nearest, but bump up by 1 more if there's still a non-trivial (>=0.02) remainder
    // left over - a safety margin so a furniture's footprint is never underestimated.
    const auto RoundUpFootprint = [](float v) -> int8 {
        auto rounded = std::lround(v);
        if (v - (float)rounded >= 0.02f) {
            rounded++;
        }
        return (int8)rounded;
    };
    furn.m_nWidthX = RoundUpFootprint(extent);
    furn.m_nWidthY = RoundUpFootprint(extent);

    furn.m_nWealthMin = (int8)wealthMin;
    furn.m_nWealthMax = (int8)wealthMax;
    furn.m_nMaxAng    = (int8)maxAng;
    furn.m_bCanPlaceInFrontOfWindow = m_bCanPlaceInFrontOfWindow;
    furn.m_bIsTall                  = m_bIsTall;
    furn.m_bCanSteal                = m_bCanSteal;

    m_Furnitures.AddItem(&furn);
    return true;
}

// 0x590EE0
Furniture_c* FurnitureSubGroup_c::GetFurniture(int16 id, uint8 wealth) {
    if (id >= 0) {
        for (auto* furn = m_Furnitures.GetHead(); furn; furn = m_Furnitures.GetNext(furn)) {
            if (furn->m_nId == id) {
                return furn;
            }
        }
        return nullptr;
    }

    if (wealth == 0xFF) { // NOTSA: sentinel for "any wealth" - pick a uniformly random furniture from the whole subgroup
        const auto index = CGeneral::GetRandomNumberInRange<int32>(0, (int32)m_Furnitures.GetNumItems(), true);
        return m_Furnitures.GetItemOffset(true, index);
    }

    int32 matchCount = 0;
    for (auto* furn = m_Furnitures.GetHead(); furn; furn = m_Furnitures.GetNext(furn)) {
        if (furn->m_nWealthMin <= wealth && wealth <= furn->m_nWealthMax) {
            matchCount++;
        }
    }

    const auto targetIdx = CGeneral::GetRandomNumberInRange<int32>(0, matchCount, true);
    auto idx = 0;
    for (auto* furn = m_Furnitures.GetHead(); furn; furn = m_Furnitures.GetNext(furn)) {
        if (furn->m_nWealthMin <= wealth && wealth <= furn->m_nWealthMax) {
            if (idx == targetIdx) {
                return furn;
            }
            idx++;
        }
    }
    return nullptr;
}

// 0x590FD0
int32 FurnitureSubGroup_c::GetRandomId(uint8 wealth) {
    if (wealth == 0xFF) { // NOTSA: sentinel for "any wealth" - pick a uniformly random furniture id from the whole subgroup
        const auto index = CGeneral::GetRandomNumberInRange<int32>(0, (int32)m_Furnitures.GetNumItems(), true);
        return m_Furnitures.GetItemOffset(true, index)->m_nId;
    }

    int32 matchCount = 0;
    for (auto* furn = m_Furnitures.GetHead(); furn; furn = m_Furnitures.GetNext(furn)) {
        if (furn->m_nWealthMin <= wealth && wealth <= furn->m_nWealthMax) {
            matchCount++;
        }
    }

    const auto targetIdx = CGeneral::GetRandomNumberInRange<int32>(0, matchCount, true);
    auto idx = 0;
    for (auto* furn = m_Furnitures.GetHead(); furn; furn = m_Furnitures.GetNext(furn)) {
        if (furn->m_nWealthMin <= wealth && wealth <= furn->m_nWealthMax) {
            if (idx == targetIdx) {
                return furn->m_nId;
            }
            idx++;
        }
    }
    return -1;
}

void FurnitureSubGroup_c::Exit() {
    m_Furnitures.RemoveAll();
}
