#include "StdInc.h"
#include "Interior_c.h"

void Interior_c::InjectHooks() {
    RH_ScopedClass(Interior_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x5921D0, { .reversed = false });
    //RH_ScopedInstall(Destructor, 0x591360, { .reversed = false });

    RH_ScopedInstall(Bedroom_AddTableItem, 0x593F10, { .reversed = false });
    RH_ScopedInstall(FurnishBedroom, 0x593FC0, { .reversed = false });
    RH_ScopedInstall(Kitchen_FurnishEdges, 0x596930, { .reversed = false });
    RH_ScopedInstall(FurnishKitchen, 0x5970B0, { .reversed = false });
    RH_ScopedInstall(Lounge_AddTV, 0x597240, { .reversed = false });
    RH_ScopedInstall(Lounge_AddHifi, 0x597430, { .reversed = false });
    RH_ScopedInstall(Lounge_AddChairInfo, 0x5974E0, { .reversed = false });
    RH_ScopedInstall(Lounge_AddSofaInfo, 0x5975C0, { .reversed = false });
    RH_ScopedInstall(FurnishLounge, 0x597740, { .reversed = false });
    RH_ScopedInstall(Office_PlaceEdgeFillers, 0x599210, { .reversed = false });
    RH_ScopedInstall(Office_PlaceDesk, 0x5993E0, { .reversed = false });
    RH_ScopedInstall(Office_PlaceEdgeDesks, 0x5995B0, { .reversed = false });
    RH_ScopedInstall(Office_FurnishEdges, 0x599770, { .reversed = false });
    RH_ScopedInstall(Office_PlaceDeskQuad, 0x599960, { .reversed = false });
    RH_ScopedInstall(Office_FurnishCenter, 0x599A30, { .reversed = false });
    RH_ScopedInstall(FurnishOffice, 0x599AF0, { .reversed = false });
    RH_ScopedInstall(Shop_Place3PieceUnit, 0x599BB0, { .reversed = false });
    RH_ScopedInstall(Shop_PlaceEdgeUnits, 0x599DC0, { .reversed = false });
    RH_ScopedInstall(Shop_PlaceCounter, 0x599EF0, { .reversed = false });
    RH_ScopedInstall(Shop_PlaceFixedUnits, 0x59A030, { .reversed = false });
    RH_ScopedInstall(Shop_FurnishCeiling, 0x59A130, { .reversed = false });
    RH_ScopedInstall(Shop_AddShelfInfo, 0x59A140, { .reversed = false });
    RH_ScopedInstall(Shop_FurnishEdges, 0x59A1B0, { .reversed = false });
    RH_ScopedInstall(GetBoundingBox, 0x593DB0, { .reversed = false });
    RH_ScopedInstall(Init, 0x593BF0, { .reversed = false });
    RH_ScopedInstall(ResetTiles, 0x593910);
    RH_ScopedInstall(PlaceObject, 0x5934E0, { .reversed = false });
    RH_ScopedInstall(GetFurnitureEntity, 0x5913B0, { .reversed = false });
    RH_ScopedInstall(IsPtInside, 0x5913E0, { .reversed = false });
    RH_ScopedInstall(CalcMatrix, 0x5914D0, { .reversed = false });
    RH_ScopedInstall(Furnish, 0x591590, { .reversed = false });
    RH_ScopedInstall(Unfurnish, 0x5915D0, { .reversed = false });
    RH_ScopedInstall(CheckTilesEmpty, 0x591680);
    RH_ScopedInstall(SetTilesStatus, 0x591700);
    RH_ScopedInstall(SetCornerTiles, 0x5917C0);
    RH_ScopedInstall(GetTileStatus, 0x5918E0);
    RH_ScopedInstall(GetNumEmptyTiles, 0x591920);
    RH_ScopedInstall(GetRandomTile, 0x591B20, { .reversed = false });
    RH_ScopedInstall(Shop_FurnishAisles, 0x59A590, { .reversed = false });
    RH_ScopedInstall(GetTileCentre, 0x591BD0, { .reversed = false });
    RH_ScopedInstall(AddGotoPt, 0x591D20, { .reversed = false });
    RH_ScopedInstall(AddInteriorInfo, 0x591E40, { .reversed = false });
    RH_ScopedInstall(AddPickups, 0x591F90, { .reversed = false });
    RH_ScopedInstall(Exit, 0x592230, { .reversed = false });
    RH_ScopedInstall(FindBoundingBox, 0x5922C0, { .reversed = false });
    RH_ScopedInstall(CalcExitPts, 0x5924A0, { .reversed = false });
    RH_ScopedInstall(IsVisible, 0x5929F0, { .reversed = false });
    RH_ScopedInstall(PlaceFurniture, 0x592AA0, { .reversed = false });
    RH_ScopedInstall(PlaceFurnitureOnWall, 0x593120, { .reversed = false });
    RH_ScopedInstall(PlaceFurnitureInCorner, 0x593340, { .reversed = false });
    RH_ScopedInstall(FindEmptyTiles, 0x591C50, { .reversed = false });
    RH_ScopedInstall(FurnishShop, 0x59A790, { .reversed = false });
}

// 0x593BF0
int32 Interior_c::Init(const CVector& pos) {
    return plugin::CallMethodAndReturn<int32, 0x593BF0>(this, &pos);
}

// 0x592230
void Interior_c::Exit() {
    plugin::CallMethod<0x592230, Interior_c*>(this);
}

// 0x593F10
CObject* Interior_c::Bedroom_AddTableItem(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7) {
    return plugin::CallMethodAndReturn<CObject*, 0x593F10, Interior_c*, int32, int32, int32, int32, int32, int32>(this, a2, a3, a4, a5, a6, a7);
}

// 0x593FC0
void Interior_c::FurnishBedroom() {
    plugin::CallMethod<0x593FC0, Interior_c*>(this);
}

// 0x596930
CObject* Interior_c::Kitchen_FurnishEdges() {
    return plugin::CallMethodAndReturn<CObject*, 0x596930, Interior_c*>(this);
}

// 0x5970B0
void Interior_c::FurnishKitchen() {
    plugin::CallMethod<0x5970B0, Interior_c*>(this);
}

// 0x597240
CObject* Interior_c::Lounge_AddTV(int32 a2, int32 a3, int32 a4, int32 a5) {
    return plugin::CallMethodAndReturn<CObject*, 0x597240, Interior_c*, int32, int32, int32, int32>(this, a2, a3, a4, a5);
}

// 0x597430
CObject* Interior_c::Lounge_AddHifi(int32 a2, int32 a3, int32 a4, int32 a5) {
    return plugin::CallMethodAndReturn<CObject*, 0x597430, Interior_c*, int32, int32, int32, int32>(this, a2, a3, a4, a5);
}

// 0x5974E0
void Interior_c::Lounge_AddChairInfo(int32 a2, int32 a3, CEntity* entityIgnoredCollision) {
    plugin::CallMethod<0x5974E0, Interior_c*, int32, int32, CEntity*>(this, a2, a3, entityIgnoredCollision);
}

// 0x5975C0
void Interior_c::Lounge_AddSofaInfo(int32 sitType, int32 offsetX, CEntity* entityIgnoredCollision) {
    plugin::CallMethod<0x5975C0, Interior_c*, int32, int32, CEntity*>(this, sitType, offsetX, entityIgnoredCollision);
}

// 0x597740
void Interior_c::FurnishLounge() {
    plugin::CallMethod<0x597740, Interior_c*>(this);
}

// 0x599210
bool Interior_c::Office_PlaceEdgeFillers(int32 arg0, int32 a2, int32 a3, int32 a6, int32 a7) {
    return plugin::CallMethodAndReturn<bool, 0x599210, Interior_c*, int32, int32, int32, int32, int32>(this, arg0, a2, a3, a6, a7);
}

// 0x5993E0
int32 Interior_c::Office_PlaceDesk(int32 a3, int32 arg4, int32 offsetY, int32 a5, uint8 a6, int32 b) {
    return plugin::CallMethodAndReturn<int32, 0x5993E0, Interior_c*, int32, int32, int32, int32, uint8, int32>(this, a3, arg4, offsetY, a5, a6, b);
}

// 0x5995B0
int32 Interior_c::Office_PlaceEdgeDesks(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    return plugin::CallMethodAndReturn<int32, 0x5995B0, Interior_c*, int32, int32, int32, int32, int32>(this, a2, a3, a4, a5, a6);
}

// 0x599770
void Interior_c::Office_FurnishEdges() {
    plugin::CallMethod<0x599770, Interior_c*>(this);
}

// 0x599960
int32 Interior_c::Office_PlaceDeskQuad(int32 a2, int32 a3, int32 a4, int32 a5) {
    return plugin::CallMethodAndReturn<int32, 0x599960, Interior_c*, int32, int32, int32, int32>(this, a2, a3, a4, a5);
}

// 0x599A30
int32 Interior_c::Office_FurnishCenter() {
    return plugin::CallMethodAndReturn<int32, 0x599A30, Interior_c*>(this);
}

// 0x599AF0
void Interior_c::FurnishOffice() {
    plugin::CallMethod<0x599AF0, Interior_c*>(this);
}

// 0x599BB0
int8 Interior_c::Shop_Place3PieceUnit(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    return plugin::CallMethodAndReturn<int8, 0x599BB0, Interior_c*, int32, int32, int32, int32, int32>(this, a2, a3, a4, a5, a6);
}

// 0x599DC0
int32 Interior_c::Shop_PlaceEdgeUnits(int32 a2, int32 a3, int32 a4, int32 a5) {
    return plugin::CallMethodAndReturn<int32, 0x599DC0, Interior_c*, int32, int32, int32, int32>(this, a2, a3, a4, a5);
}

// 0x599EF0
int32 Interior_c::Shop_PlaceCounter(uint8 a2) {
    return plugin::CallMethodAndReturn<int32, 0x599EF0, Interior_c*, uint8>(this, a2);
}

// 0x59A030
void Interior_c::Shop_PlaceFixedUnits() {
    return plugin::CallMethod<0x59A030, Interior_c*>(this);
}

// 0x59A130
void Interior_c::Shop_FurnishCeiling() {
    plugin::CallMethod<0x59A130, Interior_c*>(this);
}

// 0x59A140
void Interior_c::Shop_AddShelfInfo(int32 a2, int32 a3, int32 a5) {
    plugin::CallMethod<0x59A140, Interior_c*, int32, int32, int32>(this, a2, a3, a5);
}

// 0x59A1B0
void Interior_c::Shop_FurnishEdges() {
    plugin::CallMethod<0x59A1B0, Interior_c*>(this);
}

// 0x593DB0
bool Interior_c::GetBoundingBox(FurnitureEntity_c* entity, CVector* a3) {
    return plugin::CallMethodAndReturn<bool, 0x593DB0, Interior_c*, FurnitureEntity_c*, CVector*>(this, entity, a3);
}

// 0x593910
void Interior_c::ResetTiles() {
    memset(m_tiles, 0, sizeof(m_tiles));

    const auto* box = m_box;

    // Left/right/top walls: mark door and window spans over currently-empty tiles only.
    if (box->m_lDoorStart != -1) {
        const auto len = box->m_lDoorEnd - box->m_lDoorStart;
        if (len > 0 && box->m_lDoorStart >= 0 && box->m_width != 0 && box->m_lDoorStart + len <= box->m_depth) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[0][box->m_lDoorStart + i]; tile == 0) {
                    tile = 8;
                }
            }
        }
    }
    if (box->m_rDoorStart != -1) {
        const auto len = box->m_rDoorEnd - box->m_rDoorStart;
        const auto row = box->m_width - 1;
        if (len > 0 && row >= 0 && box->m_rDoorStart >= 0 && box->m_rDoorStart + len <= box->m_depth) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[row][box->m_rDoorStart + i]; tile == 0) {
                    tile = 8;
                }
            }
        }
    }
    if (box->m_tDoorStart != -1) {
        const auto len = box->m_tDoorEnd - box->m_tDoorStart;
        const auto col = box->m_depth - 1;
        if (len > 0 && box->m_tDoorStart >= 0 && col >= 0 && box->m_tDoorStart + len <= box->m_width) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[box->m_tDoorStart + i][col]; tile == 0) {
                    tile = 8;
                }
            }
        }
    }
    if (box->m_lWindowStart != -1) {
        const auto len = box->m_lWindowEnd - box->m_lWindowStart;
        if (len > 0 && box->m_lWindowStart >= 0 && box->m_width != 0 && box->m_lWindowStart + len <= box->m_depth) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[0][box->m_lWindowStart + i]; tile == 0) {
                    tile = 9;
                }
            }
        }
    }
    if (box->m_rWindowStart != -1) {
        const auto len = box->m_rWindowEnd - box->m_rWindowStart;
        const auto row = box->m_width - 1;
        if (len > 0 && row >= 0 && box->m_rWindowStart >= 0 && box->m_rWindowStart + len <= box->m_depth) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[row][box->m_rWindowStart + i]; tile == 0) {
                    tile = 9;
                }
            }
        }
    }
    if (box->m_tWindowStart != -1) {
        const auto len = box->m_tWindowEnd - box->m_tWindowStart;
        const auto col = box->m_depth - 1;
        if (len > 0 && box->m_tWindowStart >= 0 && col >= 0 && box->m_tWindowStart + len <= box->m_width) {
            for (auto i = 0; i < len; i++) {
                if (auto& tile = m_tiles[box->m_tWindowStart + i][col]; tile == 0) {
                    tile = 9;
                }
            }
        }
    }

    // Up to 3 rectangular "no-go" zones, marked with status 0xB over currently-empty tiles only.
    for (auto i = 0; i < 3; i++) {
        const auto left   = box->m_noGoLeft[i];
        const auto bottom = box->m_noGoBottom[i];
        const auto width  = box->m_noGoWidth[i];
        const auto depth  = box->m_noGoDepth[i];
        if (left == -1 || bottom == -1) {
            continue;
        }
        if (left < 0 || bottom < 0 || left + width > box->m_width || bottom + depth > box->m_depth || width <= 0) {
            continue;
        }
        for (auto x = 0; x < width; x++) {
            for (auto y = 0; y < depth; y++) {
                if (auto& tile = m_tiles[left + x][bottom + y]; tile == 0) {
                    tile = 0xB;
                }
            }
        }
    }
}

// 0x5934E0
CObject* Interior_c::PlaceObject(uint8 isStealable, Furniture_c* furniture, float offsetX, float offsetY, float offsetZ, float rotationZ) {
    return plugin::CallMethodAndReturn<CObject*, 0x5934E0, Interior_c*, uint8, Furniture_c*, float, float, float, float>(this, isStealable, furniture, offsetX, offsetY, offsetZ,
                                                                                                                         rotationZ);
}

// 0x5913B0
FurnitureEntity_c* Interior_c::GetFurnitureEntity(CEntity* entity) {
    return plugin::CallMethodAndReturn<FurnitureEntity_c*, 0x5913B0, Interior_c*, CEntity*>(this, entity);
}

// 0x5913E0
bool Interior_c::IsPtInside(const CVector& pt, CVector bias) {
    return plugin::CallMethodAndReturn<bool, 0x5913E0, Interior_c*, const CVector&, CVector&>(this, pt, bias);
}

// 0x5914D0
void Interior_c::CalcMatrix(CVector* translation) {
    plugin::CallMethod<0x5914D0, Interior_c*, CVector*>(this, translation);
}

// 0x591590
void Interior_c::Furnish() {
    plugin::CallMethod<0x591590, Interior_c*>(this);
}

// 0x5915D0
void Interior_c::Unfurnish() {
    plugin::CallMethod<0x5915D0, Interior_c*>(this);
}

// 0x591680
bool Interior_c::CheckTilesEmpty(int32 x, int32 y, int32 xSpan, int32 ySpan, bool bAllowWindowTiles) {
    if (x < 0 || y < 0 || x + xSpan > m_box->m_width || y + ySpan > m_box->m_depth) {
        return false;
    }
    for (auto i = 0; i < xSpan; i++) {
        for (auto j = 0; j < ySpan; j++) {
            if (const auto tile = m_tiles[x + i][y + j]; tile != 0) {
                if (!bAllowWindowTiles || tile != 9) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 0x591700
void Interior_c::SetTilesStatus(int32 x, int32 y, int32 xSpan, int32 ySpan, int8 status, bool bOverwriteSpecial) {
    if (x < 0 || y < 0 || x + xSpan > m_box->m_width || y + ySpan > m_box->m_depth) {
        return;
    }
    for (auto i = 0; i < xSpan; i++) {
        for (auto j = 0; j < ySpan; j++) {
            auto& tile = m_tiles[x + i][y + j];
            if (tile == 9 && status == 5) {
                tile = 10;
            } else if (!bOverwriteSpecial) {
                if (tile == 3) {
                    if (status == 3) {
                        return;
                    }
                    if (status == 4) {
                        tile = 4;
                    }
                } else if (tile == 0) {
                    tile = status;
                }
            } else if (tile != 5 && tile != 7 && tile != 8) {
                tile = status;
            }
        }
    }
}

// 0x5917C0
void Interior_c::SetCornerTiles(int32 corner, int32 size, int8 status, bool bOverwriteSpecial) {
    switch (corner) {
    case 1:
        SetTilesStatus(0, 0, size, 1, status, bOverwriteSpecial);
        SetTilesStatus(0, 0, 1, size, status, bOverwriteSpecial);
        break;
    case 0:
        SetTilesStatus(0, m_box->m_depth - 1, size, 1, status, bOverwriteSpecial);
        SetTilesStatus(0, m_box->m_depth - size, 1, size, status, bOverwriteSpecial);
        break;
    case 2:
        SetTilesStatus(m_box->m_width - size, 0, size, 1, status, bOverwriteSpecial);
        SetTilesStatus(m_box->m_width - 1, 0, 1, size, status, bOverwriteSpecial);
        break;
    case 3:
        SetTilesStatus(m_box->m_width - size, m_box->m_depth - 1, size, 1, status, bOverwriteSpecial);
        SetTilesStatus(m_box->m_width - 1, m_box->m_depth - size, 1, size, status, bOverwriteSpecial);
        break;
    default: break;
    }
}

// 0x5918E0
int32 Interior_c::GetTileStatus(int32 x, int32 y) {
    if (x >= 0 && y >= 0 && x < m_box->m_width && y < m_box->m_depth) {
        return m_tiles[x][y];
    }
    return 1;
}

// 0x591920
int32 Interior_c::GetNumEmptyTiles(int32 x, int32 y, int32 direction, int32 span) {
    auto count = 0;
    auto step  = 1;
    if (direction == 3 || direction == 0) {
        step = -1;
    }
    if (direction == 3 || direction == 1) {
        // Scan along the x (width) axis, over a fixed span of y (depth).
        while (true) {
            for (auto j = y; j < y + span; j++) {
                if (x >= m_box->m_width || j >= m_box->m_depth || x < 0 || j < 0 || m_tiles[x][j] != 0) {
                    return count;
                }
            }
            count++;
            x += step;
        }
    }
    // Scan along the y (depth) axis, over a fixed span of x (width).
    while (true) {
        for (auto i = x; i < x + span; i++) {
            if (i >= m_box->m_width || y >= m_box->m_depth || i < 0 || y < 0 || m_tiles[i][y] != 0) {
                return count;
            }
        }
        count++;
        y += step;
    }
}

// 0x591B20
int32 Interior_c::GetRandomTile(int32 a2, int32* a3, int32* a4) {
    return plugin::CallMethodAndReturn<int32, 0x591B20, Interior_c*, int32, int32*, int32*>(this, a2, a3, a4);
}

// 0x59A590
void Interior_c::Shop_FurnishAisles() {
    plugin::CallMethod<0x59A590, Interior_c*>(this);
}

// 0x591BD0
CVector* Interior_c::GetTileCentre(float offsetX, float offsetY, CVector* pointsIn) {
    return plugin::CallMethodAndReturn<CVector*, 0x591BD0, Interior_c*, float, float, CVector*>(this, offsetX, offsetY, pointsIn);
}

// 0x591D20
void Interior_c::AddGotoPt(int32 a, int32 b, float a3, float a4) {
    plugin::CallMethod<0x591D20, Interior_c*, int32, int32, float, float>(this, a, b, a3, a4);
}

// 0x591E40
bool Interior_c::AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision) {
    return plugin::CallMethodAndReturn<bool, 0x591E40, Interior_c*, int32, float, float, int32, CEntity*>(this, actionType, offsetX, offsetY, direction, entityIgnoredCollision);
}

// 0x591F90
void Interior_c::AddPickups() {
    plugin::CallMethod<0x591F90, Interior_c*>(this);
}

// 0x5922C0
void Interior_c::FindBoundingBox(int32 a1, int32 a2, int32* a3, int32* a4, int32* a5, int32* a6, int32* a7) {
    plugin::CallMethod<0x5922C0, Interior_c*, int32, int32, int32*, int32*, int32*, int32*, int32*>(this, a1, a2, a3, a4, a5, a6, a7);
}

// 0x5924A0
void Interior_c::CalcExitPts() {
    plugin::CallMethod<0x5924A0, Interior_c*>(this);
}

// 0x5929F0
bool Interior_c::IsVisible() {
    return plugin::CallMethodAndReturn<bool, 0x5929F0, Interior_c*>(this);
}

// 0x592AA0
void Interior_c::PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9) {
    plugin::CallMethod<0x592AA0, Interior_c*, Furniture_c*, int32, int32, float, int32, int32, int32*, int32*, uint8>(this, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

// 0x593120
void Interior_c::PlaceFurnitureOnWall(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 furnitureId, float a5, int32 a6, int32 a7, int32 a8, int32 a9, int32* a10,
                                      int32* a11, int32* a12, int32* a13, int32* a14, int32* a15) {
    plugin::CallMethod<0x593120, Interior_c*, int32, int32, int32, float, int32, int32, int32, int32, int32*, int32*, int32*, int32*, int32*, int32*>(
        this, furnitureGroupId, furnitureSubgroupId, furnitureId, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
}

// 0x593340
void Interior_c::PlaceFurnitureInCorner(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 id, float a4, int32 a5, int32 a6, int32 a2, int32* a9, int32* a10, int32* a11,
                                        int32* a12, int32* a13) {
    plugin::CallMethod<0x593340, Interior_c*, int32, int32, int32, float, int32, int32, int32, int32*, int32*, int32*, int32*, int32*>(this, furnitureGroupId, furnitureSubgroupId,
                                                                                                                                       id, a4, a5, a6, a2, a9, a10, a11, a12, a13);
}

// 0x591C50
bool Interior_c::FindEmptyTiles(int32 a3, int32 a4, int32* arg8, int32* a5) {
    return plugin::CallMethodAndReturn<bool, 0x591C50, Interior_c*, int32, int32, int32*, int32*>(this, a3, a4, arg8, a5);
}

// 0x59A790
void Interior_c::FurnishShop(int32 a2) {
    plugin::CallMethod<0x59A790, Interior_c*, int32>(this, a2);
}
