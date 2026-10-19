#include "StdInc.h"

#include "EntryExit.h"
#include "EntryExitManager.h"
#include "TaskComplexGotoDoorAndOpen.h"
#include "TaskSimpleUninterruptable.h"
#include "TaskComplexFollowLeaderInFormation.h"
#include "TaskComplexFacial.h"
#include "Garages.h"
#include "PlayerPed.h"
#include "Object.h"
#include "Interior/InteriorManager_c.h"
#include "Events/EventLeaderEntryExit.h"
#include "Events/EventGroupEvent.h"
#include "Audio/AudioZones.h"
#include "Clothes.h"
#include "Shopping.h"
#include "Rubbish.h"
#include "WaterLevel.h"

void CEntryExit::InjectHooks() {
    RH_ScopedClass(CEntryExit);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(GenerateAmbientPeds, 0x43E8B0);
    RH_ScopedInstall(GetEntryExitToDisplayNameOf, 0x43E650);
    RH_ScopedInstall(FindValidTeleportPoint, 0x43EAF0);
    RH_ScopedInstall(IsInArea, 0x43E460);
    RH_ScopedInstall(GetPositionRelativeToOutsideWorld, 0x43EA00, {.Locked = true});
    RH_ScopedInstall(TransitionStarted, 0x43FFD0);
    RH_ScopedInstall(TransitionFinished, 0x4404A0);
    RH_ScopedInstall(RequestObjectsInFrustum, 0x43E690);
    RH_ScopedInstall(RequestAmbientPeds, 0x43E6D0);
    RH_ScopedInstall(WarpGangWithPlayer, 0x43F1F0);
    RH_ScopedInstall(ProcessStealableObjects, 0x43E990);
}

// Code based on 0x43FA00
CEntryExit::CEntryExit(
    CVector center,
    CVector entranceRange,
    float entranceAngleDeg,
    CVector exit,
    float exitAngle,
    eAreaCodes area,
    CEntryExit::eFlags flags,
    int32 skyColor,
    int32 timeOn, int32 timeOff,
    int32 numberOfPeds,
    const char* name
) :
    m_nFlags{(uint16)flags}, // Set here, this way we can directly use the flag fields instead of using the enum
    m_recEntrance{
        center.x - entranceRange.x / 2.f,
        center.y - entranceRange.y / 2.f,
        center.x + entranceRange.x / 2.f,
        center.y + entranceRange.y / 2.f,
    },
    m_vecExitPos{ exit + CVector{ 0.f, 0.f, 1.f } },
    m_nArea{ area },
    m_nNumberOfPeds{ (uint8)numberOfPeds },
    m_fExitAngle{ exitAngle },
    m_nSkyColor{ (uint8)skyColor },
    m_fEntranceZ{ center.z + 1.f },
    m_fEntranceAngleRad{ DegreesToRadians(entranceAngleDeg) }
{
    std::tie(m_nTimeOn, m_nTimeOff) = [&]() -> std::pair<uint8, uint8> {
        if (bUnknownBurglary && CGeneral::RandomBool(50.f)) {
            return { 0, 0 };
        } else if (bBurglaryAccess) {
            return { 0, 24 };
        } else {
            return { timeOn, timeOff };
        }
    }();

    if (CEntryExitManager::ms_bBurglaryHousesEnabled || !bBurglaryAccess) {
        bEnableAccess = true;
    }

    if (name){
        strcpy_s(m_szName, name);
    } else {
        m_szName[0] = 0;
    }
}

// 0x43EA90
bool IsTeleportPointValid(const CVector& origin, const CVector& target) {
    return !CWorld::TestSphereAgainstWorld(target, 0.35f, nullptr, true, true, true, true, true, false)
        && CWorld::GetIsLineOfSightClear(origin, target, true, true, false, true, true, false, false);
}

// 0x43E8B0
void CEntryExit::GenerateAmbientPeds(const CVector& posn) {
    CPopulation::bInPoliceStation = false;
    if (CGame::currArea == eAreaCodes::AREA_CODE_NORMAL_WORLD) {
        CPopulation::NumberOfPedsInUseInterior = 0;
        float exitAngle = DegreesToRadians(ms_spawnPoint->m_fExitAngle);
        CPopulation::CreateWaitingCoppers(posn, exitAngle);
    } else {
        constexpr const char* PoliceStationEnExNames[]{ "POLICE1", "POLICE2", "POLICE3", "POLICE4" };
        CPopulation::bInPoliceStation = rng::any_of(PoliceStationEnExNames, [this](auto&& n) { return _stricmp(m_szName, n) == 0; });
        auto numPedsToSpawn = m_pLink ? m_pLink->m_nNumberOfPeds : m_nNumberOfPeds;
        CPopulation::NumberOfPedsInUseInterior = numPedsToSpawn;
        CPopulation::PopulateInterior(numPedsToSpawn, posn);
    }
}

// 0x43E650
CEntryExit* CEntryExit::GetEntryExitToDisplayNameOf() {
    if (ms_spawnPoint->m_nArea == eAreaCodes::AREA_CODE_NORMAL_WORLD || !HasNameSet()) {
        return nullptr;
    }
    if (m_nArea == eAreaCodes::AREA_CODE_NORMAL_WORLD) {
        return nullptr;
    }
    // TODO: Probably inlined from `CEntryExitManager`
    if (   CEntryExitManager::ms_entryExitStackPosn > 1
        && CEntryExitManager::ms_entryExitStack[CEntryExitManager::ms_entryExitStackPosn - 1] == ms_spawnPoint
    ) {
        return CEntryExitManager::ms_entryExitStack[CEntryExitManager::ms_entryExitStackPosn];
    }
    return this;
}

// 0x43EA00
// TODO: `_asm` shouldn't be removed, because the caller `EntryExitManager::GetPositionRelativeToOutsideWorld`
// uses it as well (and without reserving `edx` it is going to crash)
void CEntryExit::GetPositionRelativeToOutsideWorld(CVector& outPos) {
    _asm { push edx };

    const auto enex = GetLinkedOrThis(); // this = GetLinkedOrThis();
    if (enex->m_nArea != eAreaCodes::AREA_CODE_NORMAL_WORLD) {
        outPos += GetPosition() - enex->m_vecExitPos;
    }

    _asm { pop edx };
}

// Return center of entrance rect
CVector CEntryExit::GetPosition() const {
    return CVector{ m_recEntrance.GetCenter(), m_fEntranceZ };
}

CVector2D CEntryExit::GetPosition2D() const {
    return CVector2D{ m_recEntrance.GetCenter() };
}

// Returns the matrix with which `rectEntrance`'s points were calculated.
CMatrix CEntryExit::GetRectEntranceMatrix() const {
    CMatrix mat;
    mat.SetRotateZ(m_fEntranceAngleRad);
    mat.GetPosition() = GetPosition();
    return mat;
}

bool CEntryExit::IsVisibleByTime() const {
    return CClock::GetIsTimeInRange(m_nTimeOn, m_nTimeOff);
}

// Transforms a point into the entrance rect
CVector CEntryExit::TransformEntrancePoint(const CVector& point) const {
    return GetRectEntranceMatrix().TransformPoint(point - GetPosition());
}

// 0x43EAF0
void CEntryExit::FindValidTeleportPoint(CVector& outTeleportPoint) {
    outTeleportPoint = ms_spawnPoint->m_vecExitPos;

    if (!CWorld::TestSphereAgainstWorld(outTeleportPoint, 0.35f, nullptr, true, true, true, true, true, false)) {
        return;
    }
    
    // Test 2 * 8 spheres around the spawn point, and return whichever
    // doesn't collide with the world and the line of sight between it and `outPoint` is clear
    for (auto r : { 1.25f, 2.f }) { // Test with 2 ranges
        constexpr auto NumTestPoints{ 8 };
        for (auto i = 0; i < NumTestPoints; i++) {
            const auto rot{ (float)i * TWO_PI / (float)NumTestPoints };
            const auto point = outTeleportPoint + CVector{
                std::cos(rot) * r,
                std::sin(rot) * r,
                0.f
            };
            if (IsTeleportPointValid(outTeleportPoint, point)) {
                outTeleportPoint = point;
                return;
            }
        }
    }
}

bool CEntryExit::HasNameSet() const {
    return m_szName[0] != 0;
}

const CRect& CEntryExit::GetEntranceRect() const {
    return m_recEntrance;
}

uint8 CEntryExit::GetMyOrLinkedArea() const {
    return m_pLink ? m_pLink->m_nArea : m_nArea;
}

// 0x43E460
bool CEntryExit::IsInArea(const CVector& position) {

    const auto CheckPointInEntranceRect = [this](const CVector& point) {
        if (m_recEntrance.IsPointInside(point)) {
            if (std::abs(point.z - m_fEntranceZ) < 1.0f) {
                return true;
            }
        }
        return false;
    };

    // It's quite common `m_fEntranceAngle` is 0, so I guess this is some kind of an optimization?
    return CheckPointInEntranceRect(m_fEntranceAngleRad == 0.f ? position : TransformEntrancePoint(position));
}

// 0x43FFD0
bool CEntryExit::TransitionStarted(CPed* ped) {
    if (   !bEnableAccess
        || CEntryExitManager::ms_exitEnterState != 0
        || !IsVisibleByTime()
    ) {
        return false;
    }

    // If ped in vehicle check if enex accepts it
    if (ped->bInVehicle) {
        if (ped->m_pVehicle->m_pHandlingData->m_bIsBig) { // Move before the IsAuto or Bike check
            return false; // No big vehicles allowed :(
        }

        switch (ped->m_pVehicle->m_nVehicleType) {
        case eVehicleType::VEHICLE_TYPE_AUTOMOBILE: {
            if (!bCarsAndAircraft) {
                return false;
            }
            break;
        }
        case eVehicleType::VEHICLE_TYPE_BIKE: {
            if (!bBikesAndMotorcycles) {
                return false;
            }
            break;
        }
        default: // Nothing else is disallowed - TODO: Uncomment this, I want my Rhino in CJ's house
            return false;
        }
    } else if (bDisableExit) {
        return false;
    }

    if (m_pLink) {
        ms_spawnPoint = m_pLink;
        m_pLink->m_pLink = this;
    } else {
        ms_spawnPoint = this;
    }

    const auto spawnPointExitToUs{ ms_spawnPoint->m_vecExitPos - GetPosition() };

    if (!ped->AsPlayer()->CanPlayerStartMission()) {
        return false;
    }

    bEnteredWithoutExit = true;
    ms_pDoor            = nullptr;

    ped->bCanExitCar    = false;

    const auto AddPedScriptCommand = [ped](auto* task) {
        ped->GetEventGroup().Add(CEventScriptCommand{
            TASK_PRIMARY_PRIMARY,
            task,
            false
        });
    };

    if (bUnknownPairing || bFoodDateFlag || ped->bInVehicle) {
        if (spawnPointExitToUs.SquaredMagnitude() > sq(10.f)) {
            ms_bWarping = true;
        }
    } else {
        ms_bWarping = spawnPointExitToUs.SquaredMagnitude() > sq(10.f);

        const auto spawnPointExitToUsDir = Normalized(spawnPointExitToUs);

        const auto SetupFixedCamera = [this](CVector lookAtDir) {
            // 0x44031A
            auto fixedModePos = GetPosition() - lookAtDir * 3.f;
            fixedModePos.z += 1.f;
            TheCamera.SetCamPositionForFixedMode(fixedModePos, {});
            TheCamera.TakeControlNoEntity(GetPosition() + lookAtDir, eSwitchType::JUMPCUT, 1);
        };


        if (const auto door = CEntryExitManager::FindNearestDoor(*this, 10.f)) {
            // 0x4401AB

            AddPedScriptCommand(new CTaskComplexGotoDoorAndOpen(door));
            ms_pDoor = door;

            SetupFixedCamera(Normalized(CVector{ CVector2D{door->GetPosition()} - GetPosition2D(), 0.f }));
        } else {
            // 0x440246
            if (ms_bWarping) {
                bUnknownPairing = true;
                return true;
            }
            AddPedScriptCommand(new CTaskComplexGotoDoorAndOpen(GetPosition(), GetPosition() + spawnPointExitToUsDir * 4.f));
        }
    }

    CPad::GetPad()->bPlayerOnInteriorTransition = true;
    if (!ped->bInVehicle) {
        AddPedScriptCommand(new CTaskSimpleUninterruptable);
    }

    return true;
}

// 0x4404A0
bool CEntryExit::TransitionFinished(CPed* ped) {
    auto spawnPos = ms_spawnPoint->m_vecExitPos;

    if (const auto entity = ped->GetEntityThatThisPedIsHolding()) {
        entity->SetAreaCode((eAreaCodes)ms_spawnPoint->m_nArea);
    }

    const auto DisplayEnExName = [this] {
        if (const auto enex = GetEntryExitToDisplayNameOf()) {
            CHud::SetZoneName(TheText.Get(enex->m_szName), true);
        }
    };

    // Tells the group of the ped that its leader is going through the door
    const auto NotifyGroup = [ped] {
        if (const auto pedsGroup = CPedGroups::GetPedsGroup(ped)) {
            CEventGroupEvent groupEvent{ ped, new CEventLeaderEntryExit(ped) };
            pedsGroup->GetIntelligence().AddEvent(&groupEvent);
        }
    };

    auto& state = CEntryExitManager::ms_exitEnterState;

    // The states are: 0 = not started, 1 = walking to the door, 2 = fading, 3 = done
    if (bFoodDateFlag || bUnknownPairing || ped->bInVehicle) {
        switch (state) {
        case 0: {
            TheCamera.SetFadeColour(0, 0, 0);
            TheCamera.Fade(0.5f, eFadeFlag::FADE_IN);
            state = 2;
            NotifyGroup();
            DisplayEnExName();
            return false;
        }
        case 2: {
            if (!TheCamera.GetFading()) {
                state = 3;
            }
            return false;
        }
        default: {
            CGame::currArea = ms_spawnPoint->m_nArea;
            CEntryExitManager::ms_numVisibleEntities = 0;
            break;
        }
        }
    } else {
        CColStore::AddCollisionNeededAtPosn(spawnPos);
        CIplStore::AddIplsNeededAtPosn(spawnPos);

        if (ms_bWarping) {
            CRenderer::m_loadingPriority = false;
            CStreaming::AddModelsToRequestList(spawnPos, STREAMING_LOADING_SCENE);
            ms_spawnPoint->RequestObjectsInFrustum();
            ThePaths.SetPathsNeededAtPosition(spawnPos);
        }

        switch (state) {
        case 0: {
            CEntryExitManager::SetAreaCodeForVisibleObjects();
            state = 1;
            CGame::currArea = ms_spawnPoint->m_nArea;
            NotifyGroup();
            return false;
        }
        case 1: {
            // The fade starts once the ped is no longer busy opening the door
            const auto primaryTask = ped->GetTaskManager().GetPrimaryTasks()[TASK_PRIMARY_PRIMARY];
            if (!primaryTask || primaryTask->GetTaskType() != TASK_COMPLEX_GOTO_DOOR_AND_OPEN) {
                TheCamera.SetFadeColour(0, 0, 0);
                TheCamera.Fade(1.0f, eFadeFlag::FADE_IN);
                state = 2;
            }
            DisplayEnExName();
            return false;
        }
        case 2: {
            if (TheCamera.GetFading()) {
                return false;
            }
            state = 3;
            break;
        }
        default:
            break;
        }
    }

    // The transition is over
    ped->SetAreaCode((eAreaCodes)CGame::currArea);
    if (ped->m_pVehicle && ped->bInVehicle) {
        ped->m_pVehicle->SetAreaCode((eAreaCodes)CGame::currArea);
    }
    ped->m_pEnex = CGame::CanSeeOutSideFromCurrArea() ? nullptr : this;

    CEntryExitManager::AddEntryExitToStack(this);

    ped->bCanExitCar = true;
    CPad::GetPad(0)->bDisablePlayerEnterCar = false;

    CClothes::RebuildPlayerIfNeeded(ped->AsPlayer());

    if (bFoodDateFlag) {
        ms_spawnPoint->bEnteredWithoutExit = false;
        return true;
    }

    TheCamera.SetPosn(spawnPos);
    TheCamera.RestoreWithJumpCut();

    CAudioZones::Update(true, spawnPos);
    CWaterLevel::FindNearestWaterAndItsFlow();
    CGarages::CloseHideOutGaragesBeforeSave();
    CEntryExitManager::ResetAreaCodeForVisibleObjects();
    g_interiorMan.SetEntryExitPtr(this);
    CPopulation::RemoveAllRandomPeds();
    RequestAmbientPeds();
    CStreaming::LoadAllRequestedModels(false);
    CTimer::Suspend();

    if (!CGame::currArea) {
        RwCameraSetFarClipPlane(TheCamera.m_pRwCamera, CTimeCycle::FindFarClipForCoors(spawnPos));
    }

    ms_spawnPoint->RequestObjectsInFrustum();
    CStreaming::LoadScene(spawnPos);
    CStreaming::LoadAllRequestedModels(false);
    GenerateAmbientPeds(spawnPos);

    if (g_interiorMan.Update()) {
        CStreaming::SetLoadVehiclesInLoadScene(false);
        CStreaming::LoadScene(spawnPos);
        CStreaming::SetLoadVehiclesInLoadScene(true);
    }

    CTimer::Resume();
    CStreaming::ClearFlagForAll(STREAMING_LOADING_SCENE);

    if (ms_spawnPoint->m_nSkyColor) {
        CTimeCycle::StartExtraColour(ms_spawnPoint->m_nSkyColor - 1, false);
    } else {
        CTimeCycle::StopExtraColour(false);
    }

    CRubbish::SetVisibility(ms_spawnPoint->bUnknownInterior);

    if (ped->bInVehicle) {
        auto* const vehicle = ped->m_pVehicle;
        spawnPos.z = (spawnPos.z - 1.0f) + vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
        vehicle->Teleport(spawnPos, false);

        const auto heading = DegreesToRadians(ms_spawnPoint->m_fExitAngle);
        if (vehicle->m_matrix) {
            vehicle->m_matrix->SetRotateZOnly(heading);
        } else {
            vehicle->m_placement.m_fHeading = heading;
        }
    } else {
        FindValidTeleportPoint(spawnPos);
        ped->Teleport(spawnPos, false);

        ped->m_fCurrentRotation = DegreesToRadians(ms_spawnPoint->m_fExitAngle);
        ped->m_fAimingRotation  = ped->m_fCurrentRotation;
        if (ped->m_matrix) {
            ped->m_matrix->SetRotateZOnly(ped->m_fCurrentRotation);
        } else {
            ped->m_placement.m_fHeading = ped->m_fCurrentRotation;
        }
    }

    TheCamera.Fade(1.0f, eFadeFlag::FADE_OUT);

    CTheScripts::ClearSpaceForMissionEntity(ped->GetPosition(), ped);

    if (ms_spawnPoint->bUsedRewardEntrance) {
        CShopping::RemoveLoadedShop();
    } else if (ms_spawnPoint->bRewardInterior) {
        CShopping::LoadShop(m_szName);
        if (m_pLink) {
            bUsedRewardEntrance = true;
        }
    }

    CPopulation::ManageAllPopulation();
    CTheScripts::Process();

    if (ms_spawnPoint->bAcceptNpcGroup) {
        WarpGangWithPlayer(ped->AsPlayer());
    }

    ProcessStealableObjects(ped);

    ms_spawnPoint->bEnteredWithoutExit = false;
    if (ms_spawnPoint->bDeleteEnex) {
        CEntryExitManager::DeleteOne(CEntryExitManager::GetPool()->GetIndex(ms_spawnPoint));
    }

    if (auto* const facial = ped->GetTaskManager().GetTaskSecondaryFacial()) {
        facial->StopAll();
    }

    CGame::TidyUpMemory(true, true);
    return true;
}

// 0x43E6D0
void CEntryExit::RequestAmbientPeds() {
    if (!CGame::CanSeeOutSideFromCurrArea()) {
        CStreaming::RemoveDodgyPedsFromRandomSlots();
        return;
    }

    constexpr struct { const char* name; int32 models[TOTAL_LOADED_PEDS]; } mapping[]{
        { 
            "bar1", 
            {MODEL_BFYRI, MODEL_OFYST, MODEL_WFYST, MODEL_WMYST, MODEL_BMYRI, MODEL_BMYST, MODEL_OMOST, MODEL_OMYST} 
        },
        { 
            "strip2", 
            {MODEL_SBFYSTR, MODEL_SWFYSTR, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID} 
        },
        { 
            "LAstrip", 
            {MODEL_VWFYST1, MODEL_VBFYST2, MODEL_VHFYST3, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID, MODEL_INVALID} 
        },
        { 
            "MAFCAS", 
            {MODEL_BFORI, MODEL_HFYRI, MODEL_OMYRI, MODEL_WMYRI, MODEL_OFYST, MODEL_VHMYELV, MODEL_WMOST, MODEL_BMORI} 
        },
        { 
            "TRICAS", 
            {MODEL_BFYRI, MODEL_BMYRI, MODEL_HFORI, MODEL_HMORI, MODEL_WMORI, MODEL_WFYRI, MODEL_OMOST, MODEL_VBMYELV} 
        },
    };

    for (auto&& [name, models] : mapping) {
        if (_stricmp(name, m_szName)) {
            CStreaming::StreamPedsIntoRandomSlots(models);
            return;
        }
    }
}
// 0x43E690
void CEntryExit::RequestObjectsInFrustum() const {
    CRenderer::RequestObjectsInDirection(ms_spawnPoint->m_vecExitPos, m_fExitAngle, 0);
}

// 0x43F1F0
void CEntryExit::WarpGangWithPlayer(CPlayerPed* player) {
    auto& grp = player->GetPlayerGroup();
    
    if (!grp.IsActive()) {
        return;
    }

    auto& ms = grp.GetMembership();

    if (!ms.IsLeader(player)) {
        return;
    }

    const auto& plyrPos = player->GetPosition();
    const auto& offsets = CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets;

    size_t offsetIdx = 0;
    for (auto* const mem : ms.GetMembers()) {
        if (mem == player) {
            continue;
        }
        const auto& memPos = mem->GetPosition();

        // Find position to teleport member to
        // Original code tried only twice, but we'll try all offsets
        // (Probably won't make a lot of difference)
        CVector memTeleportTo;
        do {
            if (offsetIdx >= offsets.size()) {
                return; // No more offsets
            }
            memTeleportTo = memPos + CVector{ offsets[offsetIdx++], 0.f };
        } while (!IsTeleportPointValid(plyrPos, memTeleportTo));

        // Calculate member heading (So that they'll face the player)
        const auto memHeading = (plyrPos - memPos).Heading();

        // Teleport them
        mem->Teleport(memTeleportTo, false);
        mem->GetIntelligence()->FlushImmediately(false);

        // Make the member be heading towards the player
        mem->m_fCurrentRotation = mem->m_fAimingRotation = memHeading;
        mem->SetHeading(memHeading);
        mem->SetAreaCode(player->GetAreaCode());
        mem->m_pEnex = player->m_pEnex;
    }
}

// 0x43E990
void CEntryExit::ProcessStealableObjects(CPed* ped) {
    const auto helde = ped->GetEntityThatThisPedIsHolding();
    if (!helde) {
        return;
    }

    if (!helde->GetIsTypeObject()) {
        return; 
    }

    if (!helde->AsObject()->objectFlags.bIsLiftable) {
        return;
    }

    const auto heldobj = helde->AsObject();
    switch (heldobj->m_nObjectType) {
    case OBJECT_MISSION:
    case OBJECT_MISSION2:
        break;
    default: {
        if (ms_spawnPoint->GetArea() != AREA_CODE_NORMAL_WORLD) {
            if (g_interiorMan.FindStealableObjectId(heldobj) != -1) {
                g_interiorMan.SetStealableObjectStolen(heldobj, false);
            }
        } else {
            g_interiorMan.SetStealableObjectStolen(heldobj, true);
        }
        break;
    }
    }
    heldobj->SetAreaCode(ms_spawnPoint->GetArea());
}
