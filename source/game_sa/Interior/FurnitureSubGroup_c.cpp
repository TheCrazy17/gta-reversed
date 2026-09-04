#include "StdInc.h"
#include "FurnitureSubGroup_c.h"
#include "Furniture_c.h"

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0);
    RH_ScopedInstall(GetRandomId, 0x590FD0);
    RH_ScopedInstall(AddFurniture, 0x5C00C0, { .reversed = false });
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
    return plugin::CallMethodAndReturn<bool, 0x5C00C0, FurnitureSubGroup_c*, uint16, int16, uint8, uint8, uint8>(this, modelId, id, wealthMin, wealthMax, maxAng);
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
