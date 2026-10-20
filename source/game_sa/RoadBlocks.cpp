#include "StdInc.h"

#include "RoadBlocks.h"
#include "PedPlacement.h"
#include "TaskComplexWanderCop.h"
#include "TaskSimpleStandStill.h"
#include <extensions/File.hpp>

void CRoadBlocks::InjectHooks() {
    RH_ScopedClass(CRoadBlocks);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x461100);
    RH_ScopedInstall(ClearScriptRoadBlocks, 0x460EC0);
    RH_ScopedInstall(ClearSpaceForRoadBlockObject, 0x461020);
    RH_ScopedInstall(CreateRoadBlockBetween2Points, 0x4619C0);
    RH_ScopedInstall(GenerateRoadBlockPedsForCar, 0x461170);
    RH_ScopedInstall(GenerateRoadBlocks, 0x4629E0);
    RH_ScopedInstall(GetRoadBlockNodeInfo, 0x460EE0);
    RH_ScopedInstall(RegisterScriptRoadBlock, 0x460DF0);
}

// 0x461100
void CRoadBlocks::Init() {
    rng::fill(InOrOut, true);
    GenerateDynamicRoadBlocks = false;

    if (notsa::File rbx("data\\paths\\roadblox.dat", "rb"); rbx) {
        rbx.Read(&NumRoadBlocks, sizeof(int32));
        assert(NumRoadBlocks <= MAX_ROADBLOCKS);
        rbx.Read(RoadBlockNodes.data(), RoadBlockNodes.size() * sizeof(CNodeAddress));
    } else {
        NOTSA_UNREACHABLE("roadblox.dat couldn't be opened!");
    }
    ClearScriptRoadBlocks();
}

// 0x460EC0
void CRoadBlocks::ClearScriptRoadBlocks() {
    for (auto& srb : aScriptRoadBlocks) {
        srb.IsActive = false;
    }
}

// 0x461020
// Returns true if cleared successfully.
bool CRoadBlocks::ClearSpaceForRoadBlockObject(CVector cornerA, CVector cornerB){
    int16 numEntities{};
    CEntity* entities[2]{};
    CWorld::FindObjectsIntersectingCube(
        cornerA,
        cornerB,
        &numEntities,
        std::size(entities),
        entities,
        false,
        true,
        true,
        true,
        false
    );

    if (numEntities > std::size(entities) || numEntities <= 0) {
        return numEntities <= 0;
    }

    const auto Remove = [](CEntity* e) {
        CWorld::Remove(e);
        delete e;
    };

    for (auto* entity : entities | rngv::take(numEntities)) {
        switch (entity->GetType()) {
        case ENTITY_TYPE_VEHICLE:
            if (auto* v = entity->AsVehicle(); !v->CanBeDeleted()) {
                return false;
            } else if (!v->vehicleFlags.bCreateRoadBlockPeds) {
                Remove(v);
            }
            break;
        case ENTITY_TYPE_PED:
            if (auto* p = entity->AsPed(); p->CanBeDeleted()) {
                Remove(p);
            } else {
                return false;
            }
            break;
        case ENTITY_TYPE_OBJECT:
            if (auto* o = entity->AsObject(); o->CanBeDeleted() && o->m_nObjectType != OBJECT_GAME) {
                Remove(o);
            } else {
                return false;
            }
            break;
        default:
            NOTSA_UNREACHABLE();
        }
    }

    return true;
}

// 0x4619C0
void CRoadBlocks::CreateRoadBlockBetween2Points(CVector a, CVector b, bool isGangRoadBlock) {
    const auto delta  = b - a;
    const auto length = delta.Magnitude();
    const auto mid    = (a + b) * 0.5f;

    auto dir = delta;
    dir.Normalise();

    // Perpendicular to the line, towards the player
    auto perp = CVector{ dir.y, -dir.x, 0.0f };
    perp.Normalise();
    if (DotProduct(FindPlayerCoors(-1) - mid, perp) < 0.0f) {
        perp *= -1.0f;
    }

    // Model of the vehicles of the road block
    int32 model;
    if (!isGangRoadBlock) {
        if (FindPlayerWanted(-1)->AreArmyRequired()) {
            model = MODEL_BARRACKS;
        } else if (FindPlayerWanted(-1)->AreFbiRequired()) {
            model = MODEL_FBIRANCH;
        } else if (FindPlayerWanted(-1)->AreSwatRequired()) {
            model = MODEL_ENFORCER;
        } else {
            model = CStreaming::GetDefaultCopCarModel(false);
        }
        if (!CStreaming::GetInfo(model).IsLoaded()) {
            model = CStreaming::GetDefaultCopCarModel(false);
        }
        if (model == MODEL_COPBIKE) {
            return;
        }
    } else {
        model = CPopulation::PickRiotRoadBlockCar();
        if (model == MODEL_INVALID) {
            return;
        }
    }

    // Space the vehicles need on the line, depends on the direction they are placed in
    const auto& bounds           = CModelInfo::GetModelInfo(model)->GetColModel()->GetBoundingBox();
    const auto  boundsMin        = bounds.m_vecMin;
    const auto  boundsMax        = bounds.m_vecMax;
    const auto  widthRotated     = (boundsMax.x - boundsMin.x) + 2.0f;
    const auto  lengthAlongLine  = (boundsMax.y - boundsMin.y) + 0.2f + (isGangRoadBlock ? 0.5f : 0.0f);

    constexpr auto MAX_VEHICLES = 5;
    bool           rotated[MAX_VEHICLES]{};
    auto           remaining   = length;
    auto           numVehicles = 0;
    for (auto i = 0; i < MAX_VEHICLES; i++) {
        rotated[i] = (model == MODEL_BARRACKS || isGangRoadBlock) ? false : (CGeneral::GetRandomNumber() & 1) != 0;

        const auto needed = rotated[i] ? widthRotated : lengthAlongLine;
        if (remaining < needed) {
            break;
        }
        remaining -= needed;
        if (++numVehicles == MAX_VEHICLES) {
            break;
        }
    }

    // Corners of the box that has to be clear to place something with the matrix
    const auto IsSpaceClear = [&](const CMatrix& mat, const CVector& pos, const CVector& min, const CVector& max) {
        return ClearSpaceForRoadBlockObject(
            mat.InverseTransformVector(min) + pos,
            mat.InverseTransformVector(max) + pos
        );
    };
    const auto AddTempObject = [&](CObject* obj, const CMatrix& mat) {
        obj->GetMatrix() = mat;
        obj->SetPosn(mat.GetPosition());
        obj->m_nObjectType = OBJECT_TEMPORARY;
        obj->m_nRemovalTime = CTimer::GetTimeInMS() + 600000;
        CObject::nNoTempObjects++;
    };

    CMatrix mat;

    const auto spacing = remaining / (float)(numVehicles + 1);
    if (numVehicles > 0) {
        const auto advance = lengthAlongLine * 0.5f + spacing;
        auto       along   = spacing;

        for (auto k = 0; k < numVehicles; k++) {
            auto rolled = false;
            float angleJitter;
            if (rotated[k]) {
                along += widthRotated * 0.5f;

                mat.GetForward() = perp;
                mat.GetRight()   = CVector{ perp.y, -perp.x, 0.0f };
                mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };

                angleJitter = ((float)(CGeneral::GetRandomNumber() & 0xFF) - 128.0f) * (isGangRoadBlock ? 0.004f : 0.002f);
            } else {
                along += lengthAlongLine * 0.5f;

                if (isGangRoadBlock) {
                    rolled = (CGeneral::GetRandomNumber() & 0xFF) < 0x40;
                }

                mat.GetForward() = dir;
                if (rolled) {
                    mat.GetRight() = CVector{ 0.0f, 0.0f, 1.0f };
                    mat.GetUp()    = CVector{ -dir.y, dir.x, 0.0f };
                } else {
                    mat.GetRight() = CVector{ dir.y, -dir.x, 0.0f };
                    mat.GetUp()    = CVector{ 0.0f, 0.0f, 1.0f };
                }
                if (CGeneral::GetRandomNumber() & 1) {
                    if (rolled) {
                        mat.RotateY(PI);
                    } else {
                        mat.RotateZ(PI);
                    }
                }

                angleJitter = ((float)(CGeneral::GetRandomNumber() & 0xFF) - 128.0f) * (isGangRoadBlock ? 0.006f : 0.003f);
            }
            mat.RotateZ(angleJitter);

            mat.SetTranslateOnly(a + dir * along);
            mat.GetPosition().z = (mat.GetPosition().z - (rolled ? boundsMin.x : boundsMin.z)) + 0.3f;
            along += advance;

            if (!IsSpaceClear(mat, mat.GetPosition(), boundsMin, boundsMax)) {
                continue;
            }

            if (!isGangRoadBlock || (!rolled && (CGeneral::GetRandomNumber() & 0xFF) >= 0x40)) {
                // A vehicle
                auto* const veh = new CAutomobile(model, RANDOM_VEHICLE, true);
                veh->SetStatus(STATUS_ABANDONED);

                mat.GetPosition().z = (veh->GetHeightAboveRoad() - 0.6f) + mat.GetPosition().z;
                veh->GetMatrix() = mat;
                veh->PlaceOnRoadProperly();
                veh->SetIsStatic(false);
                veh->UpdateRwMatrix();

                veh->m_nDoorLock = CARLOCK_UNLOCKED;
                CCarCtrl::JoinCarWithRoadSystem(veh);

                veh->vehicleFlags.bNeverUseSmallerRemovalRange = true;
                veh->vehicleFlags.bIsLocked                    = false;
                veh->vehicleFlags.bEngineOn                    = false;

                auto& autoPilot          = veh->m_autoPilot;
                autoPilot.m_nCarMission  = MISSION_NONE;
                autoPilot.m_nTempAction  = TEMPACT_NONE;
                autoPilot.m_nCurrentLane = 0;
                autoPilot.m_nNextLane    = 0;
                autoPilot.m_speed        = 0.0f;
                autoPilot.m_nCruiseSpeed = 0;

                // Riot vehicles are damaged or burning
                auto burning = false;
                if (isGangRoadBlock) {
                    if ((int32)((float)(CGeneral::GetRandomNumber() & 0xFFFF) * 3.0517578125e-05f * 4.0f) == 0) {
                        veh->SetTotalDamage(true);
                    } else {
                        veh->BlowUpCarCutSceneNoExtras(true, true, true, true);
                        veh->m_nTimeWhenBlowedUp += 1000000;
                        burning = true;
                    }
                } else if (veh->UsesSiren() && (CGeneral::GetRandomNumber() & 1)) {
                    veh->vehicleFlags.bSirenOrAlarm = true;
                }

                // The ones that are on their roof are not wanted
                if (veh->GetMatrix().GetUp().z <= 0.94f) {
                    delete veh;
                    continue;
                }

                CVisibilityPlugins::SetClumpAlpha(veh->GetRpClump(), 0);
                CWorld::Add(veh);
                veh->vehicleFlags.bCreateRoadBlockPeds = true;
                veh->m_nTimeTillWeNeedThisCar          = CTimer::GetTimeInMS() + 7000;
                veh->m_nNumPedsForRoadBlock            = numVehicles > 3 ? 1 : 2;

                if (rotated[k]) {
                    // The doors that face the player are opened
                    veh->m_nPedsPositionForRoadBlock = 2;
                    for (const auto door : { DOOR_LEFT_FRONT, DOOR_RIGHT_FRONT }) {
                        if (door == DOOR_RIGHT_FRONT && veh->m_nNumPedsForRoadBlock <= 1) {
                            break;
                        }
                        const auto node = CDamageManager::GetCarNodeIndexFromDoor(door);
                        if (veh->m_aCarNodes[node]) {
                            veh->OpenDoor(nullptr, node, door, 1.0f, true);
                        }
                    }
                } else {
                    const auto toVeh = veh->GetPosition() - FindPlayerCoors(-1);
                    veh->m_nPedsPositionForRoadBlock = DotProduct(toVeh, veh->GetMatrix().GetRight()) >= 0.0f ? 1 : 0;
                }

                if (burning) {
                    gFireManager.StartFire(veh, nullptr, 2.8f, true, 60000, 2);
                    if (veh->m_pFire) {
                        veh->m_pFire->SetRemovalDist(0x5C);
                    }
                }
            } else {
                // Wreck of a vehicle that burns
                auto* const obj = new CObject(ModelIndices::MI_ROADBLOCKFUCKEDCAR1, true);
                AddTempObject(obj, mat);
                obj->SetIsStatic(false);
                CWorld::Add(obj);
                gFireManager.StartFire(obj, nullptr, 2.8f, true, 60000, 2);
                if (obj->m_pFire) {
                    obj->m_pFire->SetRemovalDist(0x5C);
                }
            }
        }
    }

    // Barriers in front of the vehicles
    if (isGangRoadBlock) {
        return;
    }

    const auto& barrierBounds = CModelInfo::GetModelInfo(ModelIndices::MI_ROADWORKBARRIER1)->GetColModel()->GetBoundingBox();
    const auto  barrierMin    = barrierBounds.m_vecMin;
    const auto  barrierMax    = barrierBounds.m_vecMax;
    const auto  barrierSize   = (barrierMax.x - barrierMin.x) + 0.5f;

    const auto numBarriers    = std::min((int32)(length / barrierSize), 8);
    const auto barrierSpacing = (length - (float)numBarriers * barrierSize) / (float)(numBarriers + 1);

    CObject::DeleteAllTempObjectsInArea(mid, length * 0.5f);

    for (auto k = 0; k < numBarriers; k++) {
        const auto along = ((float)k + 0.5f) * barrierSize + (float)(k + 1) * barrierSpacing;

        mat.SetUnity();
        mat.SetTranslate({});
        mat.GetRight()   = dir;
        mat.GetForward() = CVector{ dir.y, -dir.x, 0.0f };
        mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
        mat.RotateZ(((float)(CGeneral::GetRandomNumber() & 0xFF) - 128.0f) * 0.003f);

        // Barriers are placed a bit in front of the line, towards the player
        mat.SetTranslateOnly(a + dir * along + perp * 5.0f);
        mat.GetPosition().x += (float)(CGeneral::GetRandomNumber() & 0xF) * 0.05f;
        mat.GetPosition().y += (float)(CGeneral::GetRandomNumber() & 0xF) * 0.05f;

        auto found = false;
        const auto groundZ = CWorld::FindGroundZFor3DCoord({ mat.GetPosition().x, mat.GetPosition().y, mat.GetPosition().z + 2.0f }, &found, nullptr);
        if (!found) {
            continue;
        }
        mat.GetPosition().z = groundZ - barrierMin.z;

        if (!IsSpaceClear(mat, mat.GetPosition(), barrierMin, barrierMax)) {
            continue;
        }

        auto* const obj = new CObject(ModelIndices::MI_ROADWORKBARRIER1, true);
        AddTempObject(obj, mat);
        CWorld::Add(obj);
    }
}

// 0x461170
void CRoadBlocks::GenerateRoadBlockPedsForCar(CVehicle* vehicle, int32 pedsPositionsType, ePedType pedType) {
    const auto Generate = [&](eModelID pedModel = MODEL_INVALID, eCopType copType = COP_TYPE_CITYCOP, bool isSpecialCop = false) {
        static constexpr auto PLACEMENTS = std::to_array<CVector>({
            { -1.5f, +1.9f, 0.0f },
            { -1.5f, -2.6f, 0.0f },
            { +1.5f, +1.9f, 0.0f },
            { +1.5f, -2.6f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        static constexpr auto SPECIAL_COP_PLACEMENTS = std::to_array<CVector>({
            {  0.0f, +3.2f, 0.0f },
            { +1.5f, -1.8f, 0.0f },
            {  0.0f, +3.2f, 0.0f },
            { -1.5f, -1.8f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        const auto placementIdx = 2 * pedsPositionsType;
        const auto radiusRatio  = vehicle->GetColModel()->GetBoundingSphere().m_fRadius
            / CModelInfo::GetModelInfo(CStreaming::GetDefaultCopCarModel(false))->GetColModel()->GetBoundingSphere().m_fRadius;

        for (auto i = 0u; i < vehicle->m_nNumPedsForRoadBlock; i++) {
            const auto offset = (isSpecialCop ? SPECIAL_COP_PLACEMENTS : PLACEMENTS)[placementIdx + i] * radiusRatio;
            const auto pos = vehicle->GetMatrix().TransformPoint(offset);

            auto* ped = [&]() -> CPed* {
                if (pedType != PED_TYPE_COP) { // 0x461560
                    return new CCivilianPed(pedType, pedModel);
                } else {
                    auto* p = new CCopPed(CStreaming::IsModelLoaded(pedModel) ? copType : COP_TYPE_CITYCOP);
                    if (copType == COP_TYPE_CITYCOP) {
                        p->SetCurrentWeapon(WEAPON_PISTOL);
                    }
                    return p;
                }
            }();

            ped->SetPosn(std::get<CVector>(CPedPlacement::FindZCoorForPed(pos)));
            ped->GetMatrix().SetRotateKeepPos({ 0.0f, 0.0f, -HALF_PI });

            if (pedType == PED_TYPE_COP) {
                auto* t = new CTaskComplexWanderCop(PEDMOVE_STILL, CGeneral::GetRandomNumberInRange(8ui8));
                t->m_nSubTaskCreatedTimer = {};
                t->m_nScanForStuffTimer   = {};
                ped->GetTaskManager().SetTask(t, TASK_PRIMARY_PRIMARY);
            }
            ped->GetTaskManager().SetTask(new CTaskSimpleStandStill(0, true), TASK_PRIMARY_DEFAULT);

            ped->bStayInSamePlace         = true;
            ped->bNotAllowedToDuck        = true;
            ped->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS() + 10'000;
            ped->bCrouchWhenShooting      = !isSpecialCop || pedsPositionsType != 2;
            ped->bCullExtraFarAway        = true;
            CEntity::RegisterReference(ped->m_pVehicle = vehicle);
            CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);

            if (pedType != PED_TYPE_COP) {
                const auto weapon = CGangs::Gang[pedType - PED_TYPE_GANG1].GetRandomWeapon(false);
                if (weapon != WEAPON_UNARMED) {
                    ped->GiveDelayedWeapon(weapon, 25'001);
                    ped->SetCurrentWeapon(weapon);
                }
            }
            CWorld::Add(ped);
            ped->GetEventGroup().Add<CEventScriptCommand>({ TASK_PRIMARY_PRIMARY, new CTaskComplexKillPedOnFoot(FindPlayerPed()) });
        }
    };

    if (pedType == PED_TYPE_COP) {
        switch (vehicle->GetModelId()) {
        case MODEL_ENFORCER: Generate(MODEL_SWAT,    COP_TYPE_SWAT1,   true); break;
        case MODEL_BARRACKS: Generate(MODEL_ARMY,    COP_TYPE_ARMY,    true); break;
        case MODEL_FBIRANCH: Generate(MODEL_FBI,     COP_TYPE_FBI,     true); break;
        case MODEL_COPCARRU: Generate(MODEL_INVALID, COP_TYPE_CITYCOP, true); break;
        default:             Generate(MODEL_INVALID, COP_TYPE_CITYCOP, false); break;
        }
    } else if (IsPedTypeGang(pedType)) {
        for (auto i = 0; i < TOTAL_GANGS; i++) {
            if (!CPopCycle::m_pCurrZoneInfo->GangStrength[i]) {
                continue;
            }
            const auto pedModel = CGangs::ChooseGangPedModel((eGangID)i);
            if (pedModel == MODEL_INVALID) {
                continue;
            }
            Generate(pedModel);
            return;
        }
    } else {
        Generate();
    }
}

// 0x4629E0
void CRoadBlocks::GenerateRoadBlocks() {
    ZoneScoped;

    if (FindPlayerWanted()->m_ChanceOnRoadBlock && FindPlayerVehicle()) {
        if (!GenerateDynamicRoadBlocks) {
            rng::fill(InOrOut, true);
            GenerateDynamicRoadBlocks = true;
        }

        const auto counter1      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16 + 1);
        const auto rbsToGenerate = std::min((uint32)NumRoadBlocks, ((counter1 % 16) + counter1) / 16);
        auto       counter2      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16) / 16;

        for (; counter2 < rbsToGenerate; counter2++) {
            const auto& mrbNode = RoadBlockNodes[counter2];
            if (!ThePaths.IsAreaLoaded(mrbNode)) {
                continue;
            }
            const auto& mainNode = ThePaths.GetPathNode(mrbNode);
            const auto  playerPos = FindPlayerCoors();
            if (std::abs(playerPos.x - mainNode->GetPosition().x) >= 90.0f ||
                std::abs(playerPos.y - mainNode->GetPosition().y) >= 90.0f ||
                DistanceBetweenPoints2D(playerPos, mainNode->GetPosition()) >= 90.0f)
            {
                InOrOut[counter2] = false;
                continue;
            }

            if (InOrOut[counter2]) {
                continue;
            }
            InOrOut[counter2] = true;

            if (CGeneral::GetRandomNumberInRange(128u) >= FindPlayerWanted()->m_ChanceOnRoadBlock) {
                continue;
            }

            float mrbWidth{};
            CVector mrbDir{};
            if (!GetRoadBlockNodeInfo(mrbNode, mrbWidth, mrbDir)) {
                continue;
            }

            if (mainNode->m_nPathWidth) {
                const auto width = mainNode->m_nPathWidth / 16.0f;
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() + mrbDir * (mrbWidth / 2.f + width),
                    mainNode->GetPosition() + mrbDir * width,
                    false
                );
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() - mrbDir * width,
                    mainNode->GetPosition() - mrbDir * (mrbWidth / 2.f + width),
                    false
                );
                continue;
            }

            for (auto&& [i, nodeAddr] : rngv::enumerate(RoadBlockNodes)) {
                if (counter2 == i || InOrOut[i] || !ThePaths.IsAreaLoaded(nodeAddr.m_wAreaId)) {
                    continue;
                }
                const auto& node = ThePaths.GetPathNode(nodeAddr);

                if (std::abs(mainNode->GetPosition().x - node->GetPosition().x) >= 30.0f ||
                    std::abs(mainNode->GetPosition().y - node->GetPosition().y) >= 30.0f)
                {
                    continue;
                }

                float   width{};
                CVector dir{}; 
                if (!GetRoadBlockNodeInfo(nodeAddr, width, dir)) {
                    continue;
                }

                if (mrbWidth != width || dir.Dot(mrbDir) <= 0.7f) {
                    continue;
                }

                [[maybe_unused]] CColPoint col{};
                [[maybe_unused]] CEntity*  colEntity{};
                if (CWorld::ProcessLineOfSight(
                    mainNode->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    node->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    col,
                    colEntity,
                    true,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false
                )) {
                    continue;
                }

                const auto dirFromMain = (node->GetPosition() - mainNode->GetPosition()).Normalized();
                CreateRoadBlockBetween2Points(
                    node->GetPosition()     + dirFromMain * (mrbWidth / 2.0f),
                    mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                    false
                );
                InOrOut[i] = true;

                if (i == NumRoadBlocks) {
                    CreateRoadBlockBetween2Points(
                        mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                        mainNode->GetPosition() + dirFromMain * (mrbWidth / 2.0f),
                        false
                    );
                    break;
                }
            }
        }
    } else {
        GenerateDynamicRoadBlocks = false;
    }

    if (auto& srb = aScriptRoadBlocks[CTimer::GetFrameCounter() % MAX_SCRIPT_ROADBLOCKS]; srb.IsActive) {
        const auto c = CVector::Centroid({ srb.CornerA, srb.CornerB });

        if (DistanceBetweenPoints(FindPlayerCoors(), c) >= 90.0f) {
            srb.IsSafeToCreate = true;
        } else if (srb.IsSafeToCreate) {
            CreateRoadBlockBetween2Points(srb.CornerA, srb.CornerB, srb.IsGangRoadBlock);
            srb.IsSafeToCreate = false;
        }
    }
}

// 0x460EE0
bool CRoadBlocks::GetRoadBlockNodeInfo(CNodeAddress nodeAddress, float& outWidth, CVector& outDir) {
    auto* const node = ThePaths.GetPathNode(nodeAddress);
    assert(node);

    assert(node->m_nNumLinks >= 2);
    const auto naviLinkAddrA = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 0),
               naviLinkAddrB = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 1);
    if (!ThePaths.IsAreaLoaded(naviLinkAddrA.m_wAreaId) || !ThePaths.IsAreaLoaded(naviLinkAddrB.m_wAreaId)) {
        return false;
    }

    const auto &naviLinkA = ThePaths.GetCarPathLink(naviLinkAddrA),
               &naviLinkB = ThePaths.GetCarPathLink(naviLinkAddrB);

    const auto maxNumLanes = std::max(
        naviLinkA.m_numOppositeDirLanes + naviLinkA.m_numSameDirLanes,
        naviLinkB.m_numOppositeDirLanes + naviLinkB.m_numSameDirLanes
    );

    outWidth = ((float)maxNumLanes + 1.f) * 5.f;
    outDir   = CVector{ (naviLinkB.GetNodeCoors() - naviLinkA.GetNodeCoors()).GetPerpRight(), 0.f }.Normalized();

    return true;
}

// 0x460DF0
void CRoadBlocks::RegisterScriptRoadBlock(CVector cornerA, CVector cornerB, bool isGangRoadBlock) {
    auto free = rng::find_if(aScriptRoadBlocks, [](const auto& srb) { return !srb.IsActive; });
    if (free == aScriptRoadBlocks.end()) {
        // No free script roadblock found
        return;
    }

    free->CornerA         = cornerA;
    free->CornerB         = cornerB;
    free->IsActive        = true;
    free->IsSafeToCreate  = true;
    free->IsGangRoadBlock = isGangRoadBlock;
}
