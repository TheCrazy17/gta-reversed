#pragma once

#include "Base.h"

#include "rwplcore.h" // RwMatrix

#include "Vector.h"
#include "NodeAddress.h"
#include "InteriorInfo_t.h"
#include "InteriorGotoPoint_t.h"
#include "List_c.h"
#include "ListItem_c.h"
#include "FurnitureEntity_c.h"

class CEntity;
class CObject;
class Furniture_c;
class InteriorGroup_c;

class Interior_c : public ListItem_c<Interior_c> {
public:
    int32             m_interiorId;         // 0x8
    InteriorGroup_c*  m_pGroup;             // 0xC
    int32             m_areaCode;           // 0x10
    tEffectInterior*  m_box;                // 0x14
    RwMatrix          m_matrix;             // 0x18
    int32             field_58;             // 0x58
    TList_c<FurnitureEntity_c> m_list;       // 0x5C
    char              m_tiles[30][30];      // 0x68 - tile status grid, indexed [x][y] (see tEffectInterior::m_width/m_depth)
    int16             field_3EC;            // 0x3EC
    int16             field_3EE;            // 0x3EE
    CNodeAddress      m_nodeAddress;        // 0x3F0
    int16             field_3F4;            // 0x3F4
    int16             field_3F6;            // 0x3F6
    int32             field_3F8;            // 0x3F8
    int32             field_3FC;            // 0x3FC
    CVector           m_position;           // 0x400
    int8              m_gotoPointCount;     // 0x40C - count of valid entries in m_gotoPoints, capped at 16 (see AddGotoPt)
    int8              m_interiorInfosCount; // 0x40D
    char              gap40E[2];            // 0x40E - alignment padding before m_gotoPoints
    InteriorGotoPoint_t m_gotoPoints[16];   // 0x410
    char              gap510[128];          // 0x510 - exit-point candidate data, written by CalcExitPts (unreversed)
    InteriorInfo_t    m_interiorInfos[16];  // 0x590
    int8              m_furnitureGroupId;   // 0x790
    int8              m_furnitureId;        // 0x791
    int8              m_chairFurnitureId;   // 0x792
    int8              field_793;            // 0x793

public:
    static constexpr auto TILE_SIZE = 0.5f;

    static void InjectHooks();

    Interior_c() = default;
    ~Interior_c() = default; // 0x591360

    int32 Init(const CVector& pos);
    void Exit();

    CObject* Bedroom_AddTableItem(int32 groupId, int32 subGroupId, int32 side, int32 x, int32 y, int32 angleIdx);
    void FurnishBedroom();
    CObject* Kitchen_FurnishEdges();
    void FurnishKitchen();
    CObject* Lounge_AddTV(int32 side, int32 a3, int32 a4, int32 a5); // NOTSA: a3/a4/a5 unused (confirmed via raw disasm), kept for signature parity with the other Lounge_AddX functions
    CObject* Lounge_AddHifi(int32 side, int32 x, int32 y, int32 angleIdx);
    void Lounge_AddChairInfo(int32 a2, int32 a3, CEntity* entityIgnoredCollision);
    void Lounge_AddSofaInfo(int32 sitType, int32 offsetX, CEntity* entityIgnoredCollision);
    void FurnishLounge();
    int32 Office_PlaceEdgeFillers(int32 furnitureSubGroupOverride, int32 x, int32 y, int32 side, int32); // NOTSA: 5th param genuinely unused (confirmed via raw disasm), kept for signature parity with Office_PlaceEdgeDesks
    int32 Office_PlaceDesk(int32 x, int32 y, int32 side, int32 a4, int32 a5, int32 deskFurnitureId); // NOTSA: a4/a5 unused (confirmed via raw disasm), always 0x46/0 at the only call site
    int32 Office_PlaceEdgeDesks(int32 unused, int32 x, int32 y, int32 direction, int32 edge); // NOTSA: `unused` (1st stack param) is genuinely dead - never read in the body (confirmed via raw disasm)
    void Office_FurnishEdges();
    int32 Office_PlaceDeskQuad(int32 unused, int32 centerX, int32 centerY, int32 deskFurnitureId);
    void Office_FurnishCenter();
    void FurnishOffice();
    int8 Shop_Place3PieceUnit(int32 furnitureSubgroupBase, int32 x, int32 y, int32 side, int32 count);
    int32 Shop_PlaceEdgeUnits(int32 unitTypeOverride, int32 x, int32 y, int32 side);
    int32 Shop_PlaceCounter(uint8 doorSide);
    void Shop_PlaceFixedUnits();
    void Shop_FurnishCeiling();
    void Shop_AddShelfInfo(int32 x, int32 y, int32 direction);
    void Shop_FurnishEdges();
    bool GetBoundingBox(FurnitureEntity_c* entity, CVector* a3);
    void ResetTiles();
    CObject* PlaceObject(uint8 isStealable, Furniture_c* furniture, float offsetX, float offsetY, float offsetZ, float rotationZ);
    FurnitureEntity_c* GetFurnitureEntity(CEntity*);
    bool IsPtInside(const CVector& pt, CVector bias = {});
    void CalcMatrix(CVector* translation);
    void Furnish();
    void Unfurnish();
    bool  CheckTilesEmpty(int32 x, int32 y, int32 xSpan, int32 ySpan, bool bAllowWindowTiles);
    void  SetTilesStatus(int32 x, int32 y, int32 xSpan, int32 ySpan, int8 status, bool bOverwriteSpecial);
    void  SetCornerTiles(int32 corner, int32 size, int8 status, bool bOverwriteSpecial);
    int32 GetTileStatus(int32 x, int32 y);
    int32 GetNumEmptyTiles(int32 x, int32 y, int32 direction, int32 span);
    int32 GetRandomTile(int32 a2, int32* a3, int32* a4);
    void Shop_FurnishAisles();
    CVector* GetTileCentre(float offsetX, float offsetY, CVector* pointsIn);
    void AddGotoPt(int32 x, int32 y, float offsetX, float offsetY);
    bool AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision);
    void AddPickups();
    void FindBoundingBox(int32, int32, int32*, int32*, int32*, int32*, int32*);
    void CalcExitPts();
    bool IsVisible();
    CObject* PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9);
    CObject* PlaceFurnitureOnWall(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 furnitureId, float a5, int32 a6, int32 a7, int32 a8, int32 a9, int32* a10, int32* a11,
                              int32* a12, int32* a13, int32* a14, int32* a15);
    CObject* PlaceFurnitureInCorner(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 id, float a4, int32 a5, int32 a6, int32 a2, int32* a9, int32* a10, int32* a11, int32* a12,
                                int32* a13);
    bool FindEmptyTiles(int32 xSpan, int32 ySpan, int32* outX, int32* outY);
    void FurnishShop(int8 furnitureGroupId); // NOTSA: header previously declared `int32 a2` - raw disasm shows only the low byte is ever read (stored straight into m_furnitureGroupId, itself int8)

    auto GetNodeAddress() const { return m_nodeAddress; }
};
VALIDATE_SIZE(Interior_c, 0x794);
