#include "StdInc.h"
#include "Interior_c.h"
#include "FurnitureManager_c.h"

// One-shot flag: has the rare/special office filler item already been placed this game session?
// Sits right next to InteriorGroup_c.cpp's bInteriorPedsEnabled (0xBB3DC2) in the same small block of flags.
static auto& s_bRareOfficeFillerPlaced = StaticRef<bool>(0xBB3DC8);

void Interior_c::InjectHooks() {
    RH_ScopedClass(Interior_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x5921D0, { .reversed = false });
    //RH_ScopedInstall(Destructor, 0x591360, { .reversed = false });

    RH_ScopedInstall(Bedroom_AddTableItem, 0x593F10);
    RH_ScopedInstall(FurnishBedroom, 0x593FC0, { .reversed = false });
    RH_ScopedInstall(Kitchen_FurnishEdges, 0x596930, { .reversed = false });
    RH_ScopedInstall(FurnishKitchen, 0x5970B0, { .reversed = false });
    RH_ScopedInstall(Lounge_AddTV, 0x597240);
    RH_ScopedInstall(Lounge_AddHifi, 0x597430);
    RH_ScopedInstall(Lounge_AddChairInfo, 0x5974E0);
    RH_ScopedInstall(Lounge_AddSofaInfo, 0x5975C0);
    RH_ScopedInstall(FurnishLounge, 0x597740, { .reversed = false });
    RH_ScopedInstall(Office_PlaceEdgeFillers, 0x599210);
    RH_ScopedInstall(Office_PlaceDesk, 0x5993E0);
    RH_ScopedInstall(Office_PlaceEdgeDesks, 0x5995B0);
    RH_ScopedInstall(Office_FurnishEdges, 0x599770);
    RH_ScopedInstall(Office_PlaceDeskQuad, 0x599960);
    RH_ScopedInstall(Office_FurnishCenter, 0x599A30);
    RH_ScopedInstall(FurnishOffice, 0x599AF0);
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
    RH_ScopedInstall(GetFurnitureEntity, 0x5913B0);
    RH_ScopedInstall(IsPtInside, 0x5913E0);
    RH_ScopedInstall(CalcMatrix, 0x5914D0, { .reversed = false });
    RH_ScopedInstall(Furnish, 0x591590, { .reversed = false });
    RH_ScopedInstall(Unfurnish, 0x5915D0);
    RH_ScopedInstall(CheckTilesEmpty, 0x591680);
    RH_ScopedInstall(SetTilesStatus, 0x591700);
    RH_ScopedInstall(SetCornerTiles, 0x5917C0);
    RH_ScopedInstall(GetTileStatus, 0x5918E0);
    RH_ScopedInstall(GetNumEmptyTiles, 0x591920);
    RH_ScopedInstall(GetRandomTile, 0x591B20);
    RH_ScopedInstall(Shop_FurnishAisles, 0x59A590, { .reversed = false });
    RH_ScopedInstall(GetTileCentre, 0x591BD0);
    RH_ScopedInstall(AddGotoPt, 0x591D20);
    RH_ScopedInstall(AddInteriorInfo, 0x591E40);
    RH_ScopedInstall(AddPickups, 0x591F90, { .reversed = false });
    RH_ScopedInstall(Exit, 0x592230, { .reversed = false });
    RH_ScopedInstall(FindBoundingBox, 0x5922C0, { .reversed = false });
    RH_ScopedInstall(CalcExitPts, 0x5924A0, { .reversed = false });
    RH_ScopedInstall(IsVisible, 0x5929F0);
    RH_ScopedInstall(PlaceFurniture, 0x592AA0, { .reversed = false });
    RH_ScopedInstall(PlaceFurnitureOnWall, 0x593120, { .reversed = false });
    RH_ScopedInstall(PlaceFurnitureInCorner, 0x593340, { .reversed = false });
    RH_ScopedInstall(FindEmptyTiles, 0x591C50);
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
CObject* Interior_c::Bedroom_AddTableItem(int32 groupId, int32 subGroupId, int32 side, int32 x, int32 y, int32 angleIdx) {
    auto fx = (float)x;
    auto fy = (float)y;
    if (side == 0 || side == 2) {
        fx += TILE_SIZE;
    } else if (side == 1 || side == 3) {
        fy += TILE_SIZE;
    }

    const auto furniture = g_furnitureMan.GetFurniture(groupId, subGroupId, -1, m_box->m_status);
    return PlaceObject(true, furniture, fx + TILE_SIZE, fy + TILE_SIZE, TILE_SIZE, (float)angleIdx * 90.0f);
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
CObject* Interior_c::Lounge_AddTV(int32 side, int32, int32, int32) {
    // NOTSA: `a3`/`a4`/`a5` are genuinely unused by the original (confirmed via raw disasm - only `side`
    // is ever read from the stack), kept only so this matches the other `Lounge_AddX`-family signatures.
    float pos1X, pos1Y, pos2X, pos2Y, blockX, blockY;
    switch (side) {
    case 0:
        pos1X = TILE_SIZE;
        pos2X = 1.5f;
        pos1Y = pos2Y = (float)m_box->m_depth - TILE_SIZE;
        blockX = 1.0f;
        blockY = (float)m_box->m_depth - 2.0f;
        break;
    case 2:
        pos1Y = pos2Y = TILE_SIZE;
        pos1X = (float)m_box->m_width - TILE_SIZE;
        pos2X = pos1X - 1.0f;
        blockX = (float)m_box->m_width - 2.0f;
        blockY = 1.0f;
        break;
    case 1:
        pos1X = pos1Y = TILE_SIZE;
        pos2X = TILE_SIZE;
        pos2Y = 1.5f;
        blockX = 1.0f;
        blockY = 1.0f;
        break;
    default: // 3 (and, per the original's raw disasm, any other value too - but that reads uninitialised locals in the original, so this NOTSA default only covers the intended `side==3` case)
        pos1X = pos2X = (float)m_box->m_width - TILE_SIZE;
        pos1Y = (float)m_box->m_depth - TILE_SIZE;
        pos2Y = pos1Y - 1.0f;
        blockX = (float)m_box->m_width - 2.0f;
        blockY = (float)m_box->m_depth - 2.0f;
        break;
    }

    AddInteriorInfo(0, blockX, blockY, -1, nullptr);

    const auto angle = (float)(side & 3) * 90.0f;

    const auto tv = g_furnitureMan.GetFurniture(2, 3, -1, m_box->m_status);
    PlaceObject(true, tv, pos1X, pos1Y, TILE_SIZE, angle + 45.0f);

    const auto accessorySubGroup = (rand() < 0x3FFF) ? 7 : 9;
    const auto accessory = g_furnitureMan.GetFurniture(2, accessorySubGroup, -1, m_box->m_status);
    return PlaceObject(true, accessory, pos2X, pos2Y, TILE_SIZE, angle);
}

// 0x597430
CObject* Interior_c::Lounge_AddHifi(int32 side, int32 x, int32 y, int32 angleIdx) {
    auto fx = (float)x;
    auto fy = (float)y;
    if (side == 0 || side == 2) {
        fx += TILE_SIZE;
    } else if (side == 1 || side == 3) {
        fy += TILE_SIZE;
    }

    const auto furniture = g_furnitureMan.GetFurniture(2, 8, -1, m_box->m_status);
    return PlaceObject(true, furniture, fx + TILE_SIZE, fy + TILE_SIZE, TILE_SIZE, (float)angleIdx * 90.0f);
}

// 0x5974E0
void Interior_c::Lounge_AddChairInfo(int32 side, int32 offset, CEntity* entityIgnoredCollision) {
    switch (side) {
    case 0: AddInteriorInfo(1, (float)offset + TILE_SIZE, (float)(m_box->m_depth - 1) - 1.0f, 2, entityIgnoredCollision); break;
    case 2: AddInteriorInfo(1, (float)offset + TILE_SIZE, 1.0f, 0, entityIgnoredCollision); break;
    case 1: AddInteriorInfo(1, 1.0f, (float)offset + TILE_SIZE, 3, entityIgnoredCollision); break;
    case 3: AddInteriorInfo(1, (float)(m_box->m_width - 1) - 1.0f, (float)offset + TILE_SIZE, 1, entityIgnoredCollision); break;
    default: break;
    }
}

// 0x5975C0
void Interior_c::Lounge_AddSofaInfo(int32 sitType, int32 offset, CEntity* entityIgnoredCollision) {
    switch (sitType) {
    case 0: {
        const auto x = (float)offset + TILE_SIZE;
        const auto y = (float)(m_box->m_depth - 1) - 1.0f;
        AddInteriorInfo(1, x, y, 2, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.0f, y, 2, entityIgnoredCollision);
        break;
    }
    case 2: {
        const auto x = (float)offset + TILE_SIZE;
        AddInteriorInfo(1, x, 1.0f, 0, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.0f, 1.0f, 0, entityIgnoredCollision);
        break;
    }
    case 1: {
        const auto y = (float)offset + TILE_SIZE;
        AddInteriorInfo(1, 1.0f, y, 3, entityIgnoredCollision);
        AddInteriorInfo(1, 1.0f, y + 1.0f, 3, entityIgnoredCollision);
        break;
    }
    case 3: {
        const auto x = (float)(m_box->m_width - 1) - 1.0f;
        const auto y = (float)offset + TILE_SIZE;
        AddInteriorInfo(1, x, y, 1, entityIgnoredCollision);
        AddInteriorInfo(1, x, y + 1.0f, 1, entityIgnoredCollision);
        break;
    }
    default: break;
    }
}

// 0x597740
void Interior_c::FurnishLounge() {
    plugin::CallMethod<0x597740, Interior_c*>(this);
}

// 0x599210 (prologue tail-jumps through a linker-shared `rand() & 0xFFFF` thunk at 0x405BEF into the
// body at 0x599223 - not a separate function, just code folded by the linker)
int32 Interior_c::Office_PlaceEdgeFillers(int32 furnitureSubGroupOverride, int32 x, int32 y, int32 side, int32) {
    // rand() & 0xFFFF scaled to a 0-99 "roll". rand() only ever returns 0-0x7FFF so the mask is a
    // no-op here, but the original does it anyway (this arithmetic is the linker-shared fragment).
    const auto roll = (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 100.0f);

    // GetNumEmptyTiles scans along X for wall sides 0/2 and along Y for sides 1/3.
    const auto scanAxis = (side == 0 || side == 2) ? 1 : 2;
    if (GetNumEmptyTiles(x, y, scanAxis, 1) < 1) {
        return 1;
    }

    // One tile further along the wall in `side`'s direction - used only for the AddInteriorInfo
    // goto-point below, NOT for the PlaceFurniture call (which always uses the original x/y).
    auto nextX = x;
    auto nextY = y;
    switch (side) {
    case 2: nextY = y + 1; break;
    case 0: nextY = y - 1; break;
    case 3: nextX = x - 1; break;
    case 1: nextX = x + 1; break;
    default: break;
    }

    Furniture_c* furniture;
    auto placeArg = side; // passed to PlaceFurniture's `a6` below; overwritten with a random count in the "assorted" branch
    if (furnitureSubGroupOverride == -1) {
        if (roll > 90 && !s_bRareOfficeFillerPlaced) {
            // Rare/special filler - placed at most once per game session.
            furniture = g_furnitureMan.GetFurniture(1, 2, -1, m_box->m_status);
            if (furniture) {
                AddInteriorInfo(7, (float)nextX, (float)nextY, (side - 2) & 3, nullptr);
            }
            s_bRareOfficeFillerPlaced = true;
        } else if (roll <= 75) {
            if (roll <= 25) {
                return 1; // nothing placed
            }
            furniture = g_furnitureMan.GetFurniture(1, 3, -1, m_box->m_status);
            if (furniture) {
                AddInteriorInfo(7, (float)nextX, (float)nextY, (side - 2) & 3, nullptr);
            }
        } else {
            // 75 < roll <= 90 (or roll > 90 but the rare item was already placed this session): assorted filler set.
            placeArg = CGeneral::GetRandomNumberInRange<int32>(0, 5);
            furniture = g_furnitureMan.GetFurniture(8, 0, -1, m_box->m_status);
        }
    } else {
        furniture = g_furnitureMan.GetFurniture(1, furnitureSubGroupOverride, -1, m_box->m_status);
    }

    int32 tilesConsumed, unused;
    PlaceFurniture(furniture, x, y, 0.0f, 1, placeArg, &tilesConsumed, &unused, 0);
    return tilesConsumed;
}

// 0x5993E0
int32 Interior_c::Office_PlaceDesk(int32 x, int32 y, int32 side, int32, int32, int32 deskFurnitureId) {
    const auto rotBase = (side - 2) & 3;

    // First furniture piece (the desk itself). Its grid cell is offset by +1 along one axis depending on `side`.
    auto deskX = x;
    auto deskY = y;
    if (side == 2) {
        deskY = y + 1;
    } else if (side == 1) {
        deskX = x + 1;
    }

    const auto deskFurniture = g_furnitureMan.GetFurniture(1, 0, deskFurnitureId, m_box->m_status); // group 1 = office, subgroup 0 = desk
    int32 placedX, placedY; // NOTSA: PlaceFurniture's snapped-position out params - written by PlaceFurniture but never read back by the original
    const auto placedDesk = PlaceFurniture(deskFurniture, deskX, deskY, 0.0f, 1, rotBase, &placedX, &placedY, 0);
    if (!placedDesk) {
        return 1;
    }

    // Second furniture piece (the chair). Its grid cell is offset by `side` independently of the desk's cell,
    // and the tile-status coordinates used at the very end are independent again - the three don't all agree.
    auto chairX = x;
    auto chairY = y;
    auto tilesX = x;
    auto tilesY = y;
    switch (side) {
    case 2:
        chairX = x + 1;
        break;
    case 0:
        chairY = y + 1;
        tilesX = x + 1;
        tilesY = y + 1;
        break;
    case 3:
        chairX = x + 1;
        chairY = y + 1;
        tilesX = x + 1;
        break;
    case 1:
        tilesY = y + 1;
        break;
    default:
        break;
    }

    const auto chairFurniture = g_furnitureMan.GetFurniture(1, 1, m_chairFurnitureId, m_box->m_status); // group 1 = office, subgroup 1 = chair
    const auto placedChair = PlaceFurniture(chairFurniture, chairX, chairY, 0.0f, 1, rotBase, &placedX, &placedY, 1);

    // Chair's own placement offset, snapped half a tile towards the desk depending on facing.
    auto offsetX = (float)chairX;
    auto offsetY = (float)chairY;
    switch (rotBase) {
    case 2: offsetY += TILE_SIZE; break;
    case 0: offsetY -= TILE_SIZE; break;
    case 3: offsetX -= TILE_SIZE; break;
    case 1: offsetX += TILE_SIZE; break;
    default: break;
    }
    AddInteriorInfo(6, offsetX, offsetY, (rotBase - 2) & 3, placedChair);

    SetTilesStatus(tilesX, tilesY, 1, 1, 2, true);

    return 2;
}

// 0x5995B0
int32 Interior_c::Office_PlaceEdgeDesks(int32, int32 x, int32 y, int32 direction, int32 edge) {
    // FUN_00821b40 (raw disasm) is a generic compiler-emitted "round the float on the FPU stack to the
    // nearest int, ties away from zero" runtime helper - not interior/desk specific (30+ unrelated call
    // sites all over the binary), so there's no existing named wrapper for it in this codebase.
    // `rand() * (1/32768)` (NOT CGeneral's `RAND_MAX_FLOAT_RECIPROCAL` = 1/32767) is rounded rather than
    // truncated, so `CGeneral::GetRandomNumberInRange` doesn't reproduce it - reimplemented inline.
    const auto RandRound = [](float scale) {
        return (int32)std::lround((float)rand() * (1.0f / 32768.0f) * scale);
    };

    const auto randPercent = RandRound(100.0f); // dice roll, ~[0, 100]
    const auto randOffset  = RandRound(-40.0f); // ~[-40, 0]

    // GetNumEmptyTiles scans along the x-axis for a horizontal wall (direction 0/2) or the y-axis for a
    // vertical wall (direction 1/3, or anything else) - `direction` is the wall/side this run is placed on.
    const auto scanDirection = (direction == 0 || direction == 2) ? 1 : 2;
    const auto emptyTiles    = GetNumEmptyTiles(x, y, scanDirection, 1);
    if (emptyTiles <= 1) {
        return 1;
    }

    auto deskCount = emptyTiles / 2;
    if (deskCount >= 2 - RandRound(-2.0f)) {
        deskCount = 2 - RandRound(-2.0f); // re-rolled with a fresh draw, discarding the value used for the check above
    }

    if (randPercent > 30 - randOffset) {
        return 1;
    }

    auto totalConsumed = 0;
    if (deskCount > 0) {
        const auto side = (direction - 2) & 3; // same rotation idiom Office_PlaceDesk computes internally
        for (auto i = deskCount; i != 0; i--) {
            int32 deskX, deskY;
            switch (edge) {
            case 0: deskX = totalConsumed + x; deskY = y - 1;             break;
            case 1: deskX = x;                 deskY = totalConsumed + y; break;
            case 2: deskX = totalConsumed + x; deskY = y;                 break;
            case 3: deskX = x - 1;             deskY = totalConsumed + y; break;
            default: continue; // no matching edge -> place nothing this iteration (matches original)
            }
            totalConsumed += Office_PlaceDesk(deskX, deskY, side, 0x46, 0, m_furnitureId);
        }
    }
    return totalConsumed + 1;
}

// 0x599770
void Interior_c::Office_FurnishEdges() {
    const auto width = m_box->m_width;
    const auto depth = m_box->m_depth;

    // Block the inner border ring (inset 2 tiles from each wall) so the desks/fillers placed below
    // don't end up on the room's perimeter walkway.
    const auto ringRight  = width - 3;
    const auto ringBottom = depth - 3;
    for (auto x = 2; x <= ringRight; ++x) {
        SetTilesStatus(x, ringBottom, 1, 1, 3, false);
        SetTilesStatus(x, 2,          1, 1, 3, false);
    }
    for (auto y = 2; y <= ringBottom; ++y) {
        SetTilesStatus(2,         y, 1, 1, 3, false);
        SetTilesStatus(ringRight, y, 1, 1, 3, false);
    }

    // 4 AI goto-points at the inner corners of that ring.
    AddGotoPt(2,         2,          TILE_SIZE,  TILE_SIZE);
    AddGotoPt(2,         ringBottom, TILE_SIZE, -TILE_SIZE);
    AddGotoPt(ringRight, 2,         -TILE_SIZE,  TILE_SIZE);
    AddGotoPt(ringRight, ringBottom,-TILE_SIZE, -TILE_SIZE);

    // Mark a 4x2 door-notch area along the top wall, offset from the door position.
    const auto doorX = m_box->m_door - 2;
    SetTilesStatus(doorX, 0, 4, 2, 7, false);

    const auto rightX    = width - 1;
    const auto bottomY   = depth - 1;
    const auto innerMaxY = depth - 2;

    // Edge desks: top, bottom, left, right walls.
    for (auto x = 1; x < rightX; ) {
        x += Office_PlaceEdgeDesks(-1, x, 0, 2, 2);
    }
    for (auto x = 1; x < rightX; ) {
        x += Office_PlaceEdgeDesks(-1, x, bottomY, 0, 0);
    }
    for (auto y = 1; y <= innerMaxY; ) {
        y += Office_PlaceEdgeDesks(-1, 0, y, 1, 1);
    }
    for (auto y = 1; y <= innerMaxY; ) {
        y += Office_PlaceEdgeDesks(-1, rightX, y, 3, 3);
    }

    // Edge fillers: same 4-wall walk, filling gaps left by the desks.
    for (auto x = 1; x < rightX; ) {
        x += Office_PlaceEdgeFillers(-1, x, 0, 2, 2);
    }
    for (auto x = 1; x < rightX; ) {
        x += Office_PlaceEdgeFillers(-1, x, bottomY, 0, 0);
    }
    for (auto y = 1; y <= innerMaxY; ) {
        y += Office_PlaceEdgeFillers(-1, 0, y, 1, 1);
    }
    for (auto y = 1; y <= innerMaxY; ) {
        y += Office_PlaceEdgeFillers(-1, rightX, y, 3, 3);
    }
}

// 0x599960
int32 Interior_c::Office_PlaceDeskQuad(int32 unused, int32 centerX, int32 centerY, int32 deskFurnitureId) {
    const auto y = centerY - 2;
    Office_PlaceDesk(centerX, y, 2, 0x46, 0, deskFurnitureId);
    Office_PlaceDesk(centerX, centerY, 0, 0x46, 0, deskFurnitureId);
    Office_PlaceDesk(centerX - 2, centerY, 0, 0x46, 0, deskFurnitureId);
    Office_PlaceDesk(centerX - 2, y, 2, 0x46, 0, deskFurnitureId);
    SetTilesStatus(centerX - 3, centerY - 3, 6, 1, 3, false);
    SetTilesStatus(centerX - 3, centerY + 2, 6, 1, 3, false);
    SetTilesStatus(centerX - 3, y, 1, 4, 3, false);
    SetTilesStatus(centerX + 2, y, 1, 4, 3, false);
    return 6;
}

// 0x599A30
void Interior_c::Office_FurnishCenter() {
    const auto cols = (m_box->m_width - 6) / 6;
    const auto rows = (m_box->m_depth - 6) / 6;
    if (m_box->m_width - 6 <= 0 || m_box->m_depth - 6 <= 0 || cols <= 0) {
        return;
    }

    auto centerX = ((m_box->m_width - 6) % 6) / 2;
    for (auto col = cols; col > 0; col--) {
        centerX += 6;
        auto centerY = ((m_box->m_depth - 6) % 6) / 2 + 6;
        for (auto row = rows; row > 0; row--) {
            Office_PlaceDeskQuad(-1, centerX, centerY, m_furnitureId);
            centerY += 6;
        }
    }
}

// 0x599AF0
void Interior_c::FurnishOffice() {
    SetTilesStatus(0, 0, 2, 2, 2, false);
    SetTilesStatus(0, m_box->m_depth - 2, 2, 2, 2, false);
    SetTilesStatus(m_box->m_width - 2, 0, 2, 2, 2, false);
    SetTilesStatus(m_box->m_width - 2, m_box->m_depth - 2, 2, 2, 2, false);

    m_furnitureId      = (int8)g_furnitureMan.GetRandomId(1, 0, m_box->m_status);
    m_chairFurnitureId = (int8)g_furnitureMan.GetRandomId(1, 1, m_box->m_status);

    Office_FurnishEdges();
    Office_FurnishCenter();
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
    for (auto* furn = m_list.GetHead(); furn; furn = m_list.GetNext(furn)) {
        if (furn->m_entity == entity) {
            return furn;
        }
    }
    return nullptr;
}

// 0x5913E0
bool Interior_c::IsPtInside(const CVector& pt, CVector bias) {
    const auto rel = pt - m_matrix.pos;
    if (std::abs(DotProduct(m_matrix.right, rel)) > (float)m_box->m_width * TILE_SIZE + bias.x) {
        return false;
    }
    if (std::abs(DotProduct(m_matrix.up, rel)) > (float)m_box->m_depth * TILE_SIZE + bias.y) {
        return false;
    }
    if (std::abs(DotProduct(m_matrix.at, rel)) > (float)m_box->m_height * TILE_SIZE + bias.z) {
        return false;
    }
    return true;
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
    for (auto* furn = m_list.GetHead(); furn;) {
        auto* const next = m_list.GetNext(furn);

        auto* const player = FindPlayerPed();
        auto* const held   = player ? player->GetEntityThatThisPedIsHolding() : nullptr;
        if (!held || held != furn->m_entity || !held->GetIsTypeObject() || !((CObject*)held)->objectFlags.bIsLiftable) {
            CWorld::Remove(furn->m_entity);
            delete furn->m_entity;
        } else {
            CObject::nNoTempObjects++;
            auto* const obj    = (CObject*)held;
            obj->m_nObjectType = 3;
            obj->m_nRemovalTime = CTimer::GetTimeInMS() + 99999999;
        }

        furn->m_entity = nullptr;
        m_list.RemoveItem(furn);
        g_furnitureEntityFreeList.AddItem(furn);

        furn = next;
    }
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
int32 Interior_c::GetRandomTile(int32 targetStatus, int32* outX, int32* outY) {
    int32 x, y;
    for (;;) {
        x = CGeneral::GetRandomNumberInRange<int32>(0, m_box->m_width);
        y = CGeneral::GetRandomNumberInRange<int32>(0, m_box->m_depth);
        const auto status = (x >= 0 && y >= 0 && x < m_box->m_width && y < m_box->m_depth) ? m_tiles[x][y] : (int8)1;
        if (status == targetStatus) {
            break;
        }
    }
    *outX = x;
    *outY = y;
    return targetStatus;
}

// 0x59A590
void Interior_c::Shop_FurnishAisles() {
    plugin::CallMethod<0x59A590, Interior_c*>(this);
}

// 0x591BD0
CVector* Interior_c::GetTileCentre(float offsetX, float offsetY, CVector* pointsIn) {
    pointsIn->x = -(float)(int)m_box->m_width * TILE_SIZE + offsetX + TILE_SIZE;
    pointsIn->y = -(float)(int)m_box->m_depth * TILE_SIZE + offsetY + TILE_SIZE;
    pointsIn->z = -(float)(int)m_box->m_height * TILE_SIZE;
    RwV3dTransformPoints(pointsIn, pointsIn, 1, &m_matrix);
    return pointsIn;
}

// 0x591D20
void Interior_c::AddGotoPt(int32 x, int32 y, float offsetX, float offsetY) {
    if (m_gotoPointCount >= 16) {
        return;
    }

    const auto inBoundsTile3 = x >= 0 && y >= 0 && x < m_box->m_width && y < m_box->m_depth && m_tiles[x][y] == 3;
    if (!inBoundsTile3 && GetTileStatus(x, y) != 7) {
        return;
    }

    auto& pt = m_gotoPoints[m_gotoPointCount];
    GetTileCentre((float)x + offsetX, (float)y + offsetY, &pt.Pos);
    pt.X = (int8)x;
    pt.Y = (int8)y;

    if (x >= 0 && y >= 0 && x + 1 <= m_box->m_width && y + 1 <= m_box->m_depth) {
        auto& tile = m_tiles[x][y];
        if (tile == 3 || tile == 0) {
            tile = 4;
        }
    }

    m_gotoPointCount++;
}

// 0x591E40
bool Interior_c::AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision) {
    if (m_interiorInfosCount >= 16) {
        return false;
    }

    CVector pos;
    GetTileCentre(offsetX, offsetY, &pos);
    pos.z += 0.8f;

    CVector dir{};
    if (direction != -1) {
        switch (direction) {
        case 3: dir.x = -1.0f; break;
        case 1: dir.x =  1.0f; break;
        case 2: dir.y =  1.0f; break;
        case 0: dir.y = -1.0f; break;
        default: break;
        }
        RwV3dTransformVectors(&dir, &dir, 1, &m_matrix);
    }

    auto& info = m_interiorInfos[m_interiorInfosCount];
    info.Type                   = static_cast<eInteriorInfoType>(actionType);
    info.Pos                    = pos;
    info.Dir                    = dir;
    info.IsInUse                = false;
    info.EntityIgnoredCollision = entityIgnoredCollision;
    m_interiorInfosCount++;
    return true;
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
    const auto camPos = TheCamera.GetPosition();
    if (IsPtInside(camPos, CVector{ 5.0f, 5.0f, 0.0f })) {
        return true;
    }
    if (m_box->m_door > 0) {
        const auto dx = camPos.x - m_position.x;
        const auto dy = camPos.y - m_position.y;
        if (dx * dx + dy * dy < 100.0f) {
            return true;
        }
    }
    return false;
}

// 0x592AA0
// NOTSA: Header signature only - return type corrected from `void` to `CObject*` (raw disasm shows
// callers TEST/JZ on EAX and pass the result straight into AddInteriorInfo's CEntity* param).
CObject* Interior_c::PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9) {
    return plugin::CallMethodAndReturn<CObject*, 0x592AA0, Interior_c*, Furniture_c*, int32, int32, float, int32, int32, int32*, int32*, uint8>(this, a1, a2, a3, a4, a5, a6, a7, a8, a9);
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
bool Interior_c::FindEmptyTiles(int32 xSpan, int32 ySpan, int32* outX, int32* outY) {
    for (auto i = 0; i < 100; i++) {
        const auto x = CGeneral::GetRandomNumberInRange<int32>(0, m_box->m_width);
        const auto y = CGeneral::GetRandomNumberInRange<int32>(0, m_box->m_depth);
        if (CheckTilesEmpty(x, y, xSpan, ySpan, true)) {
            *outX = x;
            *outY = y;
            return true;
        }
    }
    return false;
}

// 0x59A790
void Interior_c::FurnishShop(int32 a2) {
    plugin::CallMethod<0x59A790, Interior_c*, int32>(this, a2);
}
