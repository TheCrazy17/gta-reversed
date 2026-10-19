/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Train.h"

#include "Buoyancy.h"
#include "CarCtrl.h"
#include "ModelIndices.h"
#include "Population.h"
#include "CullZones.h"
#include "Weather.h"
#include "FileMgr.h"
#include "CustomBuildingDNPipeline.h"
#include "Replay.h"
#include "TaskComplexEnterCar.h"
#include "TaskComplexLeaveCarAndWander.h"
#include "TaskComplexEnterCarAsDriver.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskComplexSequence.h"
#include "SeekEntity/TaskComplexSeekEntity.h"
#include "CarEnterExit.h"
#include "PedGroups.h"


// Models of the carriages of every kind of train, the list of each one ends with a zero
auto& TrainConfigs = StaticRef<int32[16][16]>(0x8D44F8);
auto& pTrackNodes = StaticRef<CTrainNode*[4]>(0xC38024);
auto& NumTrackNodes = StaticRef<std::array<int32, 4>>(0xC38014);
auto& arrTotalTrackLength = StaticRef<std::array<float, 4>>(0xC37FEC);
auto& StationDist = StaticRef<std::array<float, 6>>(0xC38034);

void CTrain::InjectHooks() {
    RH_ScopedVirtualClass(CTrain, 0x872370, 66);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6F6030);
    RH_ScopedInstall(InitTrains, 0x6F7440);
    RH_ScopedInstall(ReadAndInterpretTrackFile, 0x6F55D0);
    RH_ScopedInstall(Shutdown, 0x6F58D0);
    RH_ScopedInstall(UpdateTrains, 0x6F5900);
    RH_ScopedInstall(FindCoorsFromPositionOnTrack, 0x6F59E0);
    RH_ScopedInstall(FindMaximumSpeedToStopAtStations, 0x6F5BA0);
    RH_ScopedInstall(FindNumCarriagesPulled, 0x6F5CD0);
    RH_ScopedInstall(OpenTrainDoor, 0x6F5D80);
    RH_ScopedInstall(AddPassenger, 0x6F5D90);
    RH_ScopedInstall(RemovePassenger, 0x6F5DA0);
    RH_ScopedInstall(DisableRandomTrains, 0x6F5DB0);
    RH_ScopedInstall(RemoveOneMissionTrain, 0x6F5DC0);
    RH_ScopedInstall(ReleaseOneMissionTrain, 0x6F5DF0);
    RH_ScopedInstall(SetTrainSpeed, 0x6F5E20);
    RH_ScopedInstall(SetTrainCruiseSpeed, 0x6F5E50);
    RH_ScopedInstall(FindCaboose, 0x6F5E70);
    RH_ScopedInstall(FindEngine, 0x6F5E90);
    RH_ScopedInstall(FindCarriage, 0x6F5EB0);
    RH_ScopedInstall(FindSideStationIsOn, 0x6F5EF0);
    RH_ScopedInstall(FindNextStationPositionInDirection, 0x6F5F00);
    RH_ScopedInstall(IsInTunnel, 0x6F6320);
    RH_ScopedInstall(RemoveRandomPassenger, 0x6F6850);
    RH_ScopedInstall(RemoveMissionTrains, 0x6F6A20);
    RH_ScopedInstall(RemoveAllTrains, 0x6F6AA0);
    RH_ScopedInstall(ReleaseMissionTrains, 0x6F6B60);
    RH_ScopedInstall(FindClosestTrackNode, 0x6F6BD0);
    RH_ScopedInstall(FindPositionOnTrackFromCoors, 0x6F6CC0);
    RH_ScopedInstall(FindNearestTrain, 0x6F7090);
    RH_ScopedInstall(SetNewTrainPosition, 0x6F7140);
    RH_ScopedInstall(IsNextStationAllowed, 0x6F7260);
    RH_ScopedInstall(SkipToNextAllowedStation, 0x6F72F0);
    RH_ScopedInstall(CreateMissionTrain, 0x6F7550);
    RH_ScopedInstall(DoTrainGenerationAndRemoval, 0x6F7900);
    RH_ScopedInstall(AddNearbyPedAsRandomPassenger, 0x6F8170);
    RH_ScopedVMTInstall(SetModelIndex, 0x6F5530);
    RH_ScopedVMTInstall(ProcessControl, 0x6F86A0);

    RH_ScopedGlobalInstall(ProcessTrainAnnouncements, 0x6F5910);
    RH_ScopedGlobalInstall(PlayAnnouncement, 0x6F5920);
    RH_ScopedGlobalInstall(MarkSurroundingEntitiesForCollisionWithTrain, 0x6F6640);
    RH_ScopedGlobalInstall(TrainHitStuff<CPtrListSingleLink<CPhysical*>>, 0x6F5CF0);
}

// 0x6F6030
CTrain::CTrain(int32 modelIndex, eVehicleCreatedBy createdBy) : CVehicle(createdBy) {
    m_nVehicleType    = VEHICLE_TYPE_TRAIN;
    m_nVehicleSubType = VEHICLE_TYPE_TRAIN;

    // The handling is needed before the model is set
    auto* const mi           = CModelInfo::GetModelInfo(modelIndex)->AsVehicleModelInfoPtr();
    m_pHandlingData          = gHandlingDataMgr.GetVehiclePointer(mi->m_nHandlingId);
    m_nHandlingFlagsIntValue = m_pHandlingData->m_nHandlingFlags;

    CVehicle::SetModelIndex(modelIndex);

    SetupModelNodes();

    m_aDoors[2].m_dirn = 0x14;
    m_aDoors[2].m_axis = DOOR_AXIS_Z;
    m_aDoors[3].m_dirn = 0x14;
    m_aDoors[3].m_axis = DOOR_AXIS_Z;
    if (m_nModelIndex == MODEL_STREAKC) {
        m_aDoors[2].m_openAngle = 1.25f;
        m_aDoors[2].m_closedAngle = 0.25f;
        m_aDoors[3].m_openAngle = 1.25f;
        m_aDoors[3].m_closedAngle = 0.25f;
    } else {
        m_aDoors[2].m_openAngle = -0.4f * PI;
        m_aDoors[2].m_closedAngle = 0.0f;
        m_aDoors[3].m_openAngle = 0.4f * PI;
        m_aDoors[3].m_closedAngle = 0.0f;
    }

    // Only the flags below are set, the direction flags are left as they were
    trainFlags.b01                         = true;
    trainFlags.bStoppedAtStation           = false;
    trainFlags.bPassengersCanEnterAndLeave = false;
    trainFlags.bMissionTrain               = false;
    trainFlags.bStopsAtStations            = true;
    trainFlags.bNotOnARailRoad             = false;
    trainFlags.bForceSlowDown              = true;
    trainFlags.bIsStreakModel              = false;

    m_nPassengersGenerationState = 0;
    m_nNumPassengersToLeave      = CGeneral::GetRandomNumber() & 3;
    m_nNumPassengersToEnter      = (CGeneral::GetRandomNumber() & 3) + 1;

    physicalFlags.bDisableSimpleCollision = true;
    m_pTemporaryPassenger                 = nullptr;
    m_nMaxPassengers                      = 5;
    m_nTimeWhenCreated                    = CTimer::GetTimeInMS();
    SetUsesCollision(true);
    field_5C8                    = 0;
    m_nTrackId                   = 0;
    m_fCurrentRailDistance       = 0.0f;
    m_fTrainSpeed                = 0.0f;
    m_nTimeWhenStoppedAtStation  = 0;

    mi->ChooseVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor, 1);

    m_fMass             = m_pHandlingData->m_fMass;
    m_fTurnMass         = m_pHandlingData->m_fTurnMass;
    m_vecCentreOfMass   = m_pHandlingData->m_vecCentreOfMass;
    m_fElasticity       = 0.05f;
    m_fBuoyancyConstant = m_pHandlingData->m_fBuoyancyConstant;
    m_fAirResistance    = GetDefaultAirResistance();

    physicalFlags.bCollidable             = false;
    physicalFlags.bDisableCollisionForce  = true;
    m_bTunnelTransition                   = true;
    m_pPrevCarriage                       = nullptr;
    m_pNextCarriage                       = nullptr;
    SetStatus(STATUS_TRAIN_MOVING);
    m_autoPilot.m_speed       = 0.0f;
    m_autoPilot.m_nCruiseSpeed = 0;
    m_vehicleAudio.Initialise(this);
}

// 0x6F5530
void CTrain::SetModelIndex(uint32 index) {
    CVehicle::SetModelIndex(index);
    SetupModelNodes();
}

void CTrain::SetupModelNodes() {
    std::ranges::fill(m_aTrainNodes, nullptr);
    CClumpModelInfo::FillFrameArray(GetRpClump(), m_aTrainNodes.data());
}

// 0x6F7440
void CTrain::InitTrains() {
    ZoneScoped;

    bDisableRandomTrains = false;
    GenTrain_Status = 0;

    constexpr const char* filenames[] {
        "data\\paths\\tracks.dat",
        "data\\paths\\tracks3.dat",
        "data\\paths\\tracks2.dat",
        "data\\paths\\tracks4.dat",
    };
    for (auto i = 0u; i < std::size(pTrackNodes); ++i) {
        if (!pTrackNodes[i]) {
            CTrain::ReadAndInterpretTrackFile(filenames[i], &pTrackNodes[i], &NumTrackNodes[i], &arrTotalTrackLength[i], i);
        }
    }

    for (auto i = 0u; i < std::size(aStationCoors); ++i) {
        int32 trackId;
        const auto nodeIndex = CTrain::FindClosestTrackNode(aStationCoors[i], &trackId);

        // FIX_BUGS candidate: original always reads from pTrackNodes[0] here, regardless of the
        // track actually returned by FindClosestTrackNode - stations whose closest node isn't on
        // track 0 get their StationDist computed from the wrong track's node array.
        StationDist[i] = pTrackNodes[0][nodeIndex].GetDistanceFromStart();
    }
}

// 0x6F55D0
void CTrain::ReadAndInterpretTrackFile(const char* filename, CTrainNode** nodes, int32* lineCount, float* totalDist, int32 skipStations) {
    if (!*nodes) {
        const std::unique_ptr<char[]> buffer{ new char[0xB530] };
        CFileMgr::LoadFile(filename, reinterpret_cast<uint8*>(buffer.get()), 0xB530, "rb");

        // Copies the next line of the file to gString, the terminator is written only when asked for
        size_t pos = 0;
        const auto ReadLine = [&](bool terminate) {
            size_t len = 0;
            while (buffer[pos] != '\n') {
                gString[len++] = buffer[pos++];
            }
            if (terminate) {
                gString[len] = '\0';
            }
            pos++;
        };

        // A first line that reads "processed" is followed by the real header
        ReadLine(true);
        if (!std::strcmp(gString, "processed")) {
            ReadLine(true);
        }
        std::sscanf(gString, "%d", lineCount);

        *nodes = new CTrainNode[*lineCount];

        // Stations are flagged in the nodes of the first track
        auto* station = &aStationCoors[0];
        for (auto i = 0; i < *lineCount; i++) {
            // The line is left unterminated as in the original, so the parsing can see the rest of the previous line
            ReadLine(false);

            float x, y, z;
            int32 isStation;
            std::sscanf(gString, "%f %f %f %d", &x, &y, &z, &isStation);

            (*nodes)[i].SetX(x);
            (*nodes)[i].SetY(y);
            (*nodes)[i].SetZ(z);

            if (skipStations == 0 && isStation != 0 && station < std::end(aStationCoors)) {
                *station++ = CVector{ x, y, z };
            }
        }
    }

    // Distance of every node from the start of the track
    float total = 0.0f;
    for (auto i = 0; i < *lineCount; i++) {
        auto&       node     = (*nodes)[i];
        const auto& nextNode = (*nodes)[(i + 1) % *lineCount];
        node.m_nDistanceFromStart = static_cast<uint16>(static_cast<int32>(3.0f * total));

        const auto dx = node.GetX() - nextNode.GetX();
        const auto dy = node.GetY() - nextNode.GetY();
        total += std::sqrt(dx * dx + dy * dy);
    }
    *totalDist = total;
}

// 0x6F58D0
void CTrain::Shutdown() {
    for (auto node : pTrackNodes) {
        delete node;
        node = nullptr;
    }
}

// 0x6F5900
void CTrain::UpdateTrains() {
    ZoneScoped;

    // NOP
}

// 0x6F5910
void ProcessTrainAnnouncements() {
    // NOP
}

// 0x6F5920
void PlayAnnouncement(uint8 arg0, uint8 arg1) {
    // NOP
}

// 0x6F59E0
void CTrain::FindCoorsFromPositionOnTrack(float railDistance, int32 trackId, CVector* outCoors) {
    const auto numNodes = NumTrackNodes[trackId];
    if (numNodes <= 0) {
        return;
    }

    const auto* nodes = pTrackNodes[trackId];

    auto i = 0;
    float distFromA, distToB;
    for (;;) {
        distFromA = railDistance - nodes[i].GetDistanceFromStart();
        distToB = nodes[(i + 1) % numNodes].GetDistanceFromStart() - railDistance;
        if (distFromA >= 0.0f && distToB >= 0.0f) {
            break;
        }
        if (++i >= numNodes) {
            return;
        }
    }

    const auto& nodeA = nodes[i];
    const auto& nodeB = nodes[(i + 1) % numNodes];
    const auto  t = 1.0f / (distFromA + distToB);

    *outCoors = (nodeA.GetPosn() * distToB + nodeB.GetPosn() * distFromA) * t;
}

// 0x6F5BA0
bool CTrain::FindMaximumSpeedToStopAtStations(float* speed) {
    *speed = 50.0f;

    if (m_nTrackId != 0) { // Stations only exist on track 0
        return false;
    }

    const auto& totalLength = arrTotalTrackLength[0];

    auto minDistAhead = 10000.0f;
    for (const auto& stationDistOnTrack : StationDist) {
        auto dist = stationDistOnTrack - m_fCurrentRailDistance;
        dist += trainFlags.bClockwiseDirection ? 40.0f : -40.0f; // Look-ahead buffer (train length)

        while (dist > totalLength * 0.5f) {
            dist -= totalLength;
        }
        while (dist < totalLength * -0.5f) {
            dist += totalLength;
        }

        if (trainFlags.bClockwiseDirection) {
            if (dist > 0.0f) {
                minDistAhead = std::min(minDistAhead, dist);
            }
        } else if (dist < 0.0f) {
            minDistAhead = std::min(minDistAhead, -dist);
        }
    }

    if (minDistAhead >= 500.0f) {
        *speed = 100000.0f;
    } else {
        *speed = (1.0f - (500.0f - minDistAhead) * 0.002f) * 50.0f;
    }

    return minDistAhead < 5.0f;
}

// 0x6F5CD0
uint32 CTrain::FindNumCarriagesPulled() {
    uint32 num;
    CTrain* carriage = m_pNextCarriage;
    for (num = 0; carriage; ++num) {
        carriage = carriage->m_pNextCarriage;
    }
    return num;
}

// 0x6F5D80
void CTrain::OpenTrainDoor(float state) {
    // NOP
}

// 0x6F5D90
void CTrain::AddPassenger(CPed* ped) {
    // NOP
}

// 0x6F5DA0
void CTrain::RemovePassenger(CPed* ped) {
    // NOP
}

// 0x6F5DB0
void CTrain::DisableRandomTrains(bool disable) {
    bDisableRandomTrains = disable;
}

// 0x6F5DC0
void CTrain::RemoveOneMissionTrain(CTrain* train) {
    if (!train)
        return;

    CTrain* next;
    CTrain* _train = train;
    do {
        next = _train->m_pNextCarriage;
        CWorld::Remove(_train);
        delete _train;
        _train = next;
    } while (next);
}

// 0x6F5DF0
void CTrain::ReleaseOneMissionTrain(CTrain* train) {
    for (auto* head = train; head; head = head->m_pNextCarriage) {
        head->trainFlags.bMissionTrain = false;
    }
}

// 0x6F5E20
void CTrain::SetTrainSpeed(CTrain* train, float speed) {
    train->m_fTrainSpeed = speed / 50.0f;
    if (!train->trainFlags.bClockwiseDirection) {
        train->m_fTrainSpeed = -train->m_fTrainSpeed;
    }
}

// 0x6F5E50
void CTrain::SetTrainCruiseSpeed(CTrain* train, float speed) {
    train->m_autoPilot.SetCruiseSpeed((uint8)speed);
}

// 0x6F5E70
CTrain* CTrain::FindCaboose(CTrain* train) {
    while (train->m_pNextCarriage) {
        train = train->m_pNextCarriage;
    }
    return train;
}

// 0x6F5E90
CTrain* CTrain::FindEngine(CTrain* train) {
    while (train->m_pPrevCarriage) {
        train = train->m_pPrevCarriage;
    }
    return train;
}

// 0x6F5EB0
CTrain* CTrain::FindCarriage(CTrain* train, uint8 carriage) {
    if (!carriage) {
        return train;
    }

    uint8 count = 0;
    while ((train = train->m_pNextCarriage)) {
        if (++count >= carriage) {
            return train;
        }
    }
    return nullptr;
}

// 0x6F5EF0
bool CTrain::FindSideStationIsOn() const {
    return trainFlags.bClockwiseDirection; // ?
}

// 0x6F5F00
void CTrain::FindNextStationPositionInDirection(bool clockwiseDirection, float distance, float* distanceToStation, int32* numStations) {
    auto station = 0;
    while (station < 6 && distance >= StationDist[station]) {
        station++;
    }
    if (station >= 6) {
        station = 0;
    }

    if (!clockwiseDirection) {
        station--;
        if (station < 0) {
            station += 6;
        }
    }

    // If we're basically already at this station, snap to the next one in the direction of travel.
    if (std::fabs(distance - StationDist[station]) < 100.0f) {
        station += clockwiseDirection ? 1 : -1;
        if (station < 0) {
            station += 6;
        }
        if (station > 5) {
            station = 0;
        }
    }

    *numStations       = station;
    *distanceToStation = StationDist[station];
}

// 0x6F6320
bool CTrain::IsInTunnel() const {
    const auto& pos = GetPosition();
    CColPoint colPoint{};
    CEntity* colEntity{};
    return CWorld::ProcessVerticalLine(pos, pos.z + 100.0f, colPoint, colEntity, true);
}

// 0x6F6640
void MarkSurroundingEntitiesForCollisionWithTrain(CVector pos, float radius, CEntity* entity, bool bOnlyVehicles) {
    int32 startSectorX = std::max(CWorld::GetSectorX(pos.x - radius), 0);
    int32 startSectorY = std::max(CWorld::GetSectorY(pos.y - radius), 0);
    int32 endSectorX   = std::min(CWorld::GetSectorX(pos.x + radius), MAX_SECTORS_X - 1);
    int32 endSectorY   = std::min(CWorld::GetSectorY(pos.y + radius), MAX_SECTORS_Y - 1);

    CWorld::AdvanceCurrentScanCode();

    for (int32 sectorY = startSectorY; sectorY <= endSectorY; ++sectorY) {
        for (int32 sectorX = startSectorX; sectorX <= endSectorX; ++sectorX) {
            auto& repeatSector = CWorld::GetRepeatSector(sectorX, sectorY);
            TrainHitStuff(repeatSector.Vehicles, entity);
            if (!bOnlyVehicles) {
                TrainHitStuff(repeatSector.Peds, entity);
                TrainHitStuff(repeatSector.Objects, entity);
            }
        }
    }
}

// 0x6F5CF0
template<typename PtrListType>
void TrainHitStuff(PtrListType& ptrList, CEntity* entity) {
    for (auto* item : ptrList) {
        if (item == entity) {
            continue;
        }

        item->physicalFlags.bProcessCollisionEvenIfStationary = true;

        if (item->GetType() != ENTITY_TYPE_OBJECT || !(item->m_bIsStatic || item->m_bIsStaticWaitingForCollision)) {
            continue;
        }

        // Knock the magno-crane's hook payload off when a train hits it.
        if (item->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE1 &&
            item->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE2 &&
            item->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE3
        ) {
            continue;
        }

        item->SetModelIndexNoCreate(0);
        item->AddToMovingList();
        item->m_nFakePhysics = 0;
    }
}

// 0x6F6850
void CTrain::RemoveRandomPassenger() {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    auto* const player = FindPlayerPed();
    if (player->m_pVehicle == this) {
        return;
    }

    // Leave the train alone while the player is already on the way to get in
    if (auto* const task = player->GetIntelligence()->GetTaskManager().GetPrimaryTasks()[TASK_PRIMARY_PRIMARY]) {
        if (task->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER || task->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
            if (static_cast<CTaskComplexEnterCar*>(task)->GetTargetCar() == this) {
                return;
            }
        }
    }

    if (m_nNumPassengersToLeave == 0) {
        return;
    }

    // The previous passenger has to be out of the train before the next one is created
    if (m_pTemporaryPassenger) {
        if (m_pTemporaryPassenger->bInVehicle) {
            return;
        }
        CEntity::ClearReference(m_pTemporaryPassenger);
    }

    auto carRating = CGeneral::GetRandomNumberInRange(0, 12);
    if (carRating == 12) {
        carRating = 11;
    }
    if (CGeneral::GetRandomNumber() < 100) {
        carRating = 25;
    }

    // The direction of the train decides between the driver seat and a passenger seat
    auto* const ped = CPopulation::AddPedInCar(this, !trainFlags.bClockwiseDirection, carRating, 0, false, false);
    if (!ped) {
        return;
    }
    ped->bJustGotOffTrain = true;

    m_nNumPassengersToLeave--;
    m_pTemporaryPassenger = ped;
    ped->RegisterReference(reinterpret_cast<CEntity**>(&m_pTemporaryPassenger));

    ped->GetIntelligence()->GetTaskManager().SetTask(new CTaskComplexLeaveCarAndWander{ this, TARGET_DOOR_FRONT_LEFT, 0, true }, TASK_PRIMARY_PRIMARY, false);
}

// 0x6F6A20
void CTrain::RemoveMissionTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsTrain() &&
            &vehicle != FindPlayerVehicle() &&
            vehicle.AsTrain()->trainFlags.bMissionTrain
        ) {
            CWorld::Remove(&vehicle);
            delete &vehicle;
        }
    }
}

// 0x6F6AA0
void CTrain::RemoveAllTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (!vehicle.IsTrain()) {
            continue;
        }

        auto isPlayerTrain = false;
        for (auto* carriage = vehicle.AsTrain(); carriage && !isPlayerTrain; carriage = carriage->m_pPrevCarriage) {
            isPlayerTrain = carriage == FindPlayerVehicle();
        }
        for (auto* carriage = vehicle.AsTrain(); carriage && !isPlayerTrain; carriage = carriage->m_pNextCarriage) {
            isPlayerTrain = carriage == FindPlayerVehicle();
        }

        if (!isPlayerTrain) {
            CWorld::Remove(&vehicle);
            delete &vehicle;
        }
    }
}

// 0x6F6B60
void CTrain::ReleaseMissionTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsTrain() && &vehicle != FindPlayerVehicle()) {
            vehicle.AsTrain()->trainFlags.bMissionTrain = false;
        }
    }
}

// 0x6F6BD0
int32 CTrain::FindClosestTrackNode(CVector posn, int32* outTrackId) {
    auto bestDist = 99999.9f;
    auto bestNode = 0;

    for (auto track = 0; track < 4; track++) {
        for (auto node = 0; node < NumTrackNodes[track]; node++) {
            const auto dist = DistanceBetweenPoints(posn, pTrackNodes[track][node].GetPosn());
            if (dist < bestDist) {
                *outTrackId = track;
                bestNode    = node;
                bestDist    = dist;
            }
        }
    }

    return bestNode;
}

// 0x6F6CC0
void CTrain::FindPositionOnTrackFromCoors() {
    const auto numNodes = NumTrackNodes[m_nTrackId];
    auto* const nodes   = pTrackNodes[m_nTrackId];

    // Brightness of a node, blending between the day and night values
    const auto GetNodeBrightness = [](CTrainNode& node) {
        const auto lighting = node.GetLightingFromCollision();
        const auto balance  = CCustomBuildingDNPipeline::m_fDNBalanceParam;
        return static_cast<float>(lighting.night) * 0.033333335f * balance + (1.0f - balance) * (static_cast<float>(lighting.day) * 0.033333335f);
    };

    for (auto i = 0; i < numNodes; i++) {
        const auto  nextIndex = (i + 1) % NumTrackNodes[m_nTrackId];
        auto&       node      = nodes[i];
        auto&       nextNode  = nodes[nextIndex];

        const CVector2D from{ node.GetX(), node.GetY() };
        const CVector2D to{ nextNode.GetX(), nextNode.GetY() };
        const auto      pos = GetPosition();

        // Where along this piece of the track the train is
        const auto segment   = to - from;
        const auto along     = ((pos.x - from.x) * segment.x + (pos.y - from.y) * segment.y) / segment.SquaredMagnitude();
        if (!(along > 0.001f && along < 1.001f)) {
            continue;
        }

        const CVector2D onTrack{ segment.x * along + from.x, segment.y * along + from.y };
        if ((onTrack - CVector2D{ pos }).Magnitude() < 3.0f) {
            const auto distFrom = static_cast<float>(node.m_nDistanceFromStart) * 0.33333334f;
            const auto distTo   = static_cast<float>(nextNode.m_nDistanceFromStart) * 0.33333334f;

            // The rail distance is measured at the front of the train
            const auto& bbox = CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel()->GetBoundingBox();
            m_fCurrentRailDistance = (distTo - distFrom) * along + distFrom - (bbox.m_vecMax.y - bbox.m_vecMin.y) * 0.5f;
            if (m_fCurrentRailDistance < 0.0f) {
                m_fCurrentRailDistance += arrTotalTrackLength[m_nTrackId];
            }

            m_fTrainSpeed = m_vecMoveSpeed.Magnitude();
            const auto movingAlongSegment = segment.x * m_vecMoveSpeed.x + segment.y * m_vecMoveSpeed.y > 0.0f;
            if (trainFlags.bClockwiseDirection == movingAlongSegment) {
                m_fTrainSpeed = -m_fTrainSpeed;
            }
            return;
        }

        const auto brightness     = GetNodeBrightness(node);
        const auto nextBrightness = GetNodeBrightness(nextNode);
        m_fContactSurfaceBrightness = (nextBrightness - brightness) * along + brightness;
    }
}

// 0x6F7090
CTrain* CTrain::FindNearestTrain(CVector posn, bool mustBeMainTrain) {
    CTrain* nearest  = nullptr;
    auto    bestDist = 1e+07f;

    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (!vehicle.IsTrain()) {
            continue;
        }

        auto*      train    = vehicle.AsTrain();
        const auto trainPosn = train->GetPosition();
        const auto dist      = std::sqrt(sq(trainPosn.x - posn.x) + sq(trainPosn.y - posn.y));

        if (dist < bestDist && (!mustBeMainTrain || train->trainFlags.bIsFrontCarriage)) {
            nearest  = train;
            bestDist = dist;
        }
    }

    return nearest;
}

// 0x6F7140
void CTrain::SetNewTrainPosition(CTrain* train, CVector posn) {
    train->SetPosn(posn);
    train->FindPositionOnTrackFromCoors();
}

// 0x6F7260
bool CTrain::IsNextStationAllowed(CTrain* train) {
    train = FindEngine(train);

    float distanceToStation;
    int32 stationIndex;
    FindNextStationPositionInDirection(train->trainFlags.bClockwiseDirection, train->m_fCurrentRailDistance, &distanceToStation, &stationIndex);

    const eLevelName stationLevel = CTheZones::GetLevelFromPosition(aStationCoors[stationIndex]);
    return CStats::GetStatValue(STAT_CITY_UNLOCKED) + 1.0f >= (float)stationLevel;
}

// 0x6F72F0
void CTrain::SkipToNextAllowedStation(CTrain* train) {
    train = FindEngine(train);

    float      distanceToStation;
    int32      stationIndex;
    eLevelName stationLevel;
    do {
        FindNextStationPositionInDirection(train->trainFlags.bClockwiseDirection, train->m_fCurrentRailDistance, &distanceToStation, &stationIndex);
        stationLevel = CTheZones::GetLevelFromPosition(aStationCoors[stationIndex]);
    } while (CStats::GetStatValue(STAT_CITY_UNLOCKED) + 1.0f < (float)stationLevel);

    if (train->trainFlags.bClockwiseDirection) {
        train->m_fTrainSpeed          = 0.1f;
        train->m_fCurrentRailDistance = distanceToStation - 20.0f; // _DAT_00858ba4
    } else {
        train->m_fTrainSpeed          = -0.1f;
        train->m_fCurrentRailDistance = distanceToStation + 20.0f; // _DAT_00858ba4
    }

    CStreaming::LoadScene(aStationCoors[stationIndex]);
    // NOTSA: the original computes a nonzero minutes value here (via an unidentified call chain
    // starting at 0x40EA15) instead of passing 0 - not resolved this session.
    CGameLogic::PassTime(0);
}

// 0x6F7550
void CTrain::CreateMissionTrain(CVector posn, bool clockwiseDirection, uint32 trainType, CTrain** outFirstCarriage, CTrain** outLastCarriage, int32 nodeIndex, int32 trackId, bool isMissionTrain) {
    if (nodeIndex < 0) {
        nodeIndex = FindClosestTrackNode(posn, &trackId);
    }

    float railDistance = static_cast<float>(pTrackNodes[trackId][nodeIndex].m_nDistanceFromStart) * 0.33333334f;

    std::array<CTrain*, 16> created{};
    size_t                  numCreated{};
    CTrain*                 first{};
    CTrain*                 previous{};

    for (const auto modelIndex : TrainConfigs[trainType]) {
        if (!modelIndex) {
            break;
        }

        auto* const train = new CTrain(modelIndex, PERMANENT_VEHICLE);
        train->GetMatrix().SetTranslate(CVector{});
        train->m_nNodeIndex = static_cast<int16>(nodeIndex);
        train->SetStatus(STATUS_ABANDONED);
        train->vehicleFlags.bIsLocked          = true;
        train->m_fCurrentRailDistance          = railDistance;
        train->trainFlags.bMissionTrain        = isMissionTrain;
        train->trainFlags.bClockwiseDirection  = clockwiseDirection;
        train->m_nTrackId                      = static_cast<int8>(trackId);

        if (isMissionTrain) {
            train->trainFlags.bStopsAtStations = false;
        }

        if (!previous) {
            if (isMissionTrain) {
                train->GetPosition() = posn;
                train->FindPositionOnTrackFromCoors();
            }
            train->m_fLength = 0.0f;
        } else {
            // Each carriage is placed one carriage length behind or ahead of the previous rail position
            if (clockwiseDirection) {
                const auto& bbox = CModelInfo::GetModelInfo(train->m_nModelIndex)->GetColModel()->GetBoundingBox();
                const auto  length = bbox.m_vecMax.y - bbox.m_vecMin.y;
                train->m_fLength = -length;
                railDistance -= length;
            } else {
                const auto& bbox = CModelInfo::GetModelInfo(previous->m_nModelIndex)->GetColModel()->GetBoundingBox();
                const auto  length = bbox.m_vecMax.y - bbox.m_vecMin.y;
                train->m_fLength = length;
                railDistance += length;
            }
        }

        created[numCreated++] = train;

        if (!previous) {
            train->trainFlags.bIsFrontCarriage = true;
            first = train;
            if (outFirstCarriage) {
                *outFirstCarriage = train;
            }
        } else {
            train->trainFlags.bIsFrontCarriage = false;
            train->vehicleFlags.bHasBeenOwnedByPlayer = true;
            previous->trainFlags.bIsLastCarriage = false;
        }
        train->trainFlags.bIsLastCarriage = true;
        if (outLastCarriage) {
            *outLastCarriage = train;
        }

        train->m_pPrevCarriage = previous;
        if (previous) {
            previous->RegisterReference(reinterpret_cast<CEntity**>(&train->m_pPrevCarriage));
        }
        train->m_pNextCarriage = nullptr;
        if (previous) {
            previous->m_pNextCarriage = train;
            train->RegisterReference(reinterpret_cast<CEntity**>(&previous->m_pNextCarriage));
        }

        train->ProcessControl();
        previous = train;
    }

    for (auto i = numCreated; i-- > 0;) {
        CWorld::Remove(created[i]);
        CWorld::Add(created[i]);
    }

    if (first->m_nModelIndex != MODEL_TRAM) {
        CPopulation::AddPedInCar(first, true, -1, 0, false, false);
        if (first->m_pDriver) {
            first->m_pDriver->GetIntelligence()->SetPedDecisionMakerType(6);
        }
    }

    // A train that has a streak model carriage is a streak train
    bool isStreak = false;
    for (auto* carriage = first; carriage; carriage = carriage->m_pNextCarriage) {
        if (carriage->m_nModelIndex == MODEL_STREAK) {
            isStreak = true;
            break;
        }
    }
    for (auto* carriage = first; carriage; carriage = carriage->m_pNextCarriage) {
        carriage->trainFlags.bIsStreakModel = isStreak;
    }
}

// 0x6F7900
void CTrain::DoTrainGenerationAndRemoval() {
    static auto& playerNearStation = StaticRef<bool>(0xC38068);
    static auto& lastTrainConfig   = StaticRef<int32>(0xC38064); // Cycles through the configurations of the main train

    const auto timeMS     = CTimer::GetTimeInMS();
    const auto prevTimeMS = CTimer::GetPreviousTimeInMS();

    // Check once every 3 seconds whether the player is close to a station
    if (timeMS / 3000 != prevTimeMS / 3000) {
        playerNearStation = false;
        for (const auto& station : aStationCoors) {
            if ((CVector2D{ station } - CVector2D{ FindPlayerCoors(-1) }).Magnitude() < 60.0f) {
                playerNearStation = true;
            }
        }
    }

    // Close to a station the trains are handled every frame and from further away
    const uint32 period     = playerNearStation ? 1 : 950;
    const float  maxGenDist = playerNearStation ? 100.0f : 70.0f;
    if (timeMS / period == prevTimeMS / period) {
        return;
    }

    // Remove the trains that are far away from the camera
    bool trainExists = false;
    for (auto i = GetVehiclePool()->GetSize(); i-- > 0;) {
        auto* const vehicle = GetVehiclePool()->GetAt(i);
        if (!vehicle || vehicle->m_nVehicleSubType != VEHICLE_TYPE_TRAIN) {
            continue;
        }
        auto* const front = vehicle->AsTrain();
        if (!front->trainFlags.bIsFrontCarriage || front->trainFlags.bMissionTrain) {
            continue;
        }
        trainExists = true;

        bool canRemove = true;
        for (auto* carriage = front; carriage; carriage = carriage->m_pNextCarriage) {
            const auto toCamera = TheCamera.GetPosition() - carriage->GetPosition();
            if (carriage == FindPlayerVehicle(-1, false) || CVector2D{ toCamera }.Magnitude() < 220.0f) {
                canRemove = false;
            }
        }
        if (!canRemove) {
            continue;
        }
        for (auto* carriage = front; carriage;) {
            auto* const next = carriage->m_pNextCarriage;
            CWorld::Remove(carriage);
            delete carriage;
            carriage = next;
        }
    }

    if (bDisableRandomTrains) {
        return;
    }

    // Distance in the plane from the camera to a node of a track
    const auto DistanceToCamera = [](uint32 track, int32 node) {
        const auto& trackNode = pTrackNodes[track][node];
        return (CVector2D{ trackNode.GetX(), trackNode.GetY() } - CVector2D{ TheCamera.GetPosition() }).Magnitude();
    };

    // Moves the generation node along the track, in the chosen direction, until it is far enough from the camera
    // Returns the node that was left last
    const auto WalkNodeAwayFromCamera = [&](float& distance) {
        auto previousNode = static_cast<int32>(GenTrain_GenerationNode);
        while (distance < 170.0f) {
            previousNode = static_cast<int32>(GenTrain_GenerationNode);
            if (GenTrain_Direction) {
                GenTrain_GenerationNode--;
                if (static_cast<int32>(GenTrain_GenerationNode) < 0) {
                    GenTrain_GenerationNode += NumTrackNodes[GenTrain_Track];
                }
            } else {
                GenTrain_GenerationNode = (GenTrain_GenerationNode + 1) % NumTrackNodes[GenTrain_Track];
            }
            distance = DistanceToCamera(GenTrain_Track, GenTrain_GenerationNode);
        }
        return previousNode;
    };

    switch (GenTrain_Status) {
    case 0: { // Look for a place to put a new train
        if (trainExists) {
            return;
        }

        GenTrain_Track          = 0;
        GenTrain_GenerationNode = CGeneral::GetRandomNumber() % NumTrackNodes[0];
        const auto&   genNode   = pTrackNodes[0][GenTrain_GenerationNode];
        const CVector genNodePos{ genNode.GetX(), genNode.GetY(), genNode.GetZ() };
        float         distance = DistanceToCamera(0, GenTrain_GenerationNode);

        // Too far, or the node is inside a tunnel that the player is not in, use the tram track instead
        bool useTramTrack = distance >= maxGenDist;
        if (!useTramTrack && genNodePos.z + 6.0f <= FindPlayerCoors(-1).z) {
            if (!(CCullZones::FindTunnelAttributesForCoors(FindPlayerCoors(-1)) & (TUNNEL | TUNNEL_TRANSITION))
                && (CCullZones::FindTunnelAttributesForCoors(genNodePos) & TUNNEL)
            ) {
                useTramTrack = true;
            }
        }

        if (!useTramTrack) {
            GenTrain_Direction      = CGeneral::GetRandomNumber() & 1;
            const auto previousNode = WalkNodeAwayFromCamera(distance);

            lastTrainConfig      = (lastTrainConfig + 1) % 8;
            GenTrain_Status      = 1;
            GenTrain_TrainConfig = lastTrainConfig;
            if (distance > 220.0f) {
                GenTrain_GenerationNode = previousNode;
            }
            return;
        }

        // Trams only run in San Fierro
        if (GenTrain_Status != 0 || CWeather::WeatherRegion != WEATHER_REGION_SF) {
            return;
        }
        GenTrain_Track          = 1;
        GenTrain_GenerationNode = CGeneral::GetRandomNumber() % NumTrackNodes[1];
        distance                = DistanceToCamera(1, GenTrain_GenerationNode);
        if (distance >= maxGenDist) {
            return;
        }
        GenTrain_Direction = CGeneral::GetRandomNumber() & 1;
        WalkNodeAwayFromCamera(distance);
        if (distance >= 220.0f) {
            return;
        }
        GenTrain_Status      = 1;
        GenTrain_TrainConfig = (CGeneral::GetRandomNumber() & 1) + 8;
        return;
    }
    case 1: { // Wait for the models and create the train
        bool allLoaded = true;
        for (const auto model : TrainConfigs[GenTrain_TrainConfig]) {
            if (!model) {
                break;
            }
            if (!CStreaming::GetInfo(model).IsLoaded()) {
                CStreaming::RequestModel(model, STREAMING_KEEP_IN_MEMORY);
                allLoaded = false;
            }
        }
        if (!allLoaded) {
            return;
        }

        if (DistanceToCamera(GenTrain_Track, GenTrain_GenerationNode) > 60.0f) {
            CTrain* firstCarriage{};
            CreateMissionTrain(CVector{}, GenTrain_Direction, GenTrain_TrainConfig, &firstCarriage, nullptr, GenTrain_GenerationNode, GenTrain_Track, false);

            int32 speedRange;
            if (GenTrain_Track == 0) {
                speedRange = CGeneral::GetRandomNumber() % 30 + 15;

                // Slower near the start of the line
                const auto& node = pTrackNodes[GenTrain_Track][GenTrain_GenerationNode];
                if ((CVector2D{ node.GetX(), node.GetY() } - CVector2D{ 2222.0f, -1750.0f }).Magnitude() < 300.0f) {
                    speedRange /= 2;
                }
            } else {
                speedRange = CGeneral::GetRandomNumber() % 7 + 7;
            }

            float maxSpeed;
            firstCarriage->FindMaximumSpeedToStopAtStations(&maxSpeed);
            SetTrainSpeed(firstCarriage, std::min(static_cast<float>(speedRange), maxSpeed));
            firstCarriage->m_autoPilot.m_nCruiseSpeed = static_cast<uint8>(speedRange);
        }

        for (const auto model : TrainConfigs[GenTrain_TrainConfig]) {
            if (!model) {
                break;
            }
            CStreaming::SetModelIsDeletable(model);
            CStreaming::SetModelTxdIsDeletable(model);
        }
        GenTrain_Status = 0;
        return;
    }
    default:
        return;
    }
}

// 0x6F8170
void CTrain::AddNearbyPedAsRandomPassenger() {
    auto* const player = FindPlayerPed();
    if (player->m_pVehicle == this) {
        return;
    }

    // Leave the train alone while the player is already on the way to get in
    if (auto* const task = player->GetIntelligence()->GetTaskManager().GetPrimaryTasks()[TASK_PRIMARY_PRIMARY]) {
        if (task->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER || task->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
            if (static_cast<CTaskComplexEnterCar*>(task)->GetTargetCar() == this) {
                return;
            }
        }
    }

    const bool clockwise = trainFlags.bClockwiseDirection;

    // The low nibble counts the peds that were sent to the train, the high nibble is the amount wanted
    if (m_nNumPassengersToLeave == m_nNumPassengersToEnter) {
        return;
    }

    // The ped sent before has to be either on board or gone from the train before sending another
    if (auto* const previous = m_pTemporaryPassenger) {
        const bool justLeft = previous->bJustGotOffTrain;
        if (justLeft == previous->bInVehicle) {
            return;
        }
        if (previous->GetIntelligence()->FindTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_WANDER)) {
            return;
        }
        CEntity::SafeCleanUpRef(m_pTemporaryPassenger);
        if (!m_pTemporaryPassenger->bJustGotOffTrain && m_pTemporaryPassenger->bInVehicle && m_pTemporaryPassenger->m_pVehicle == this) {
            CPopulation::RemovePed(m_pTemporaryPassenger);
        }
        m_pTemporaryPassenger = nullptr;
    }

    // Closest ped on the side of the doors that is free to be sent
    const auto& trainPos   = GetPosition();
    const auto& trainRight = GetRight();
    const auto  planeOffset = -(trainPos.z * trainRight.z + trainPos.y * trainRight.y + trainPos.x * trainRight.x);

    CPed*  closestPed{};
    float  closestDistSq = 999'999.0f;
    for (auto i = GetPedPool()->GetSize(); i-- > 0;) {
        auto* const ped = GetPedPool()->GetAt(i);
        if (!ped || !ped->IsCreatedBy(PED_GAME) || CPedGroups::GetPedsGroup(ped) || ped->GetPlayerData()) {
            continue;
        }
        if (ped->m_nPedType == PED_TYPE_COP || ped->bInVehicle || ped->bJustGotOffTrain) {
            continue;
        }
        if (ped->m_nPedState == PEDSTATE_DIE || ped->m_nPedState == PEDSTATE_DEAD || ped->m_nPedState == PEDSTATE_DIE_BY_STEALTH) {
            continue;
        }
        if (ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER)) {
            continue;
        }

        auto toPed = ped->GetPosition() - trainPos;
        toPed.z    = 0.0f;
        const auto distSq = toPed.SquaredMagnitude();
        if (distSq > 625.0f) {
            continue;
        }

        const auto side = DotProduct(ped->GetPosition(), trainRight) + planeOffset;
        if (clockwise ? side < 0.0f : side > 0.0f) {
            continue;
        }
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestPed    = ped;
        }
    }
    if (!closestPed) {
        return;
    }

    // Door to walk to, in the space of the train
    CVector doorPos;
    int32   doorId{};
    CCarEnterExit::GetNearestCarDoor(closestPed, this, doorPos, doorId);

    auto doorOffset = Invert(*m_matrix).TransformPoint(doorPos);
    if (clockwise) {
        doorOffset += trainRight;
    } else {
        doorOffset -= trainRight;
    }

    static auto& moveStateRadius       = StaticRef<float>(0x86FC28);
    static auto& maxEntityDist         = StaticRef<float>(0x86FC2C);
    static auto& followNodeHeightDelta = StaticRef<float>(0x86FC30);

    auto* const seekTask = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
        this,
        50'000,
        1'000,
        maxEntityDist,
        moveStateRadius,
        followNodeHeightDelta,
        true,
        true,
        CEntitySeekPosCalculatorXYOffset{ doorOffset }
    };
    seekTask->SetMoveState(PEDMOVE_SPRINT);

    auto* const sequence = new CTaskComplexSequence{};
    sequence->AddTask(seekTask);
    if (clockwise) {
        sequence->AddTask(new CTaskComplexEnterCarAsPassenger{ this, 0, false });
    } else {
        sequence->AddTask(new CTaskComplexEnterCarAsDriver{ this });
    }
    closestPed->GetIntelligence()->GetTaskManager().SetTask(sequence, TASK_PRIMARY_PRIMARY, false);

    m_nNumPassengersToLeave++;
    m_pTemporaryPassenger = closestPed;
    closestPed->RegisterReference(reinterpret_cast<CEntity**>(&m_pTemporaryPassenger));
}

// 0x6F86A0
void CTrain::ProcessControl() {
    vehicleFlags.bWarnedPeds = 0;
    m_vehicleAudio.Service();
    if (gbModelViewer) {
        return;
    }

    CVector vecOldTrainPosition = GetPosition();
    float fOldTrainHeading = GetHeading();

    const float& fTotalTrackLength = arrTotalTrackLength[m_nTrackId];
    CTrainNode* trainNodes = pTrackNodes[m_nTrackId];
    auto numTrackNodes = NumTrackNodes[m_nTrackId];

    if (trainFlags.bNotOnARailRoad == 0) {
        if (!trainFlags.bIsFrontCarriage) {
            if (m_pPrevCarriage) {
                m_fTrainSpeed = m_pPrevCarriage->m_fTrainSpeed;
                m_fCurrentRailDistance = m_pPrevCarriage->m_fCurrentRailDistance + m_fLength;
            } else {
                m_fTrainSpeed *= std::pow(0.9900000095367432f, CTimer::GetTimeStep());
                m_fCurrentRailDistance += m_fTrainSpeed * CTimer::GetTimeStep();
            }

            if (trainFlags.b01 && trainFlags.bStoppedAtStation && m_nModelIndex == MODEL_STREAKC && !trainFlags.bMissionTrain) {
                CPlayerPed* localPlayer = FindPlayerPed();
                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_LEAVE) {
                    if (localPlayer->m_pVehicle == this) {
                        m_nNumPassengersToLeave = 0;
                    } else {
                        m_nNumPassengersToLeave = (CGeneral::GetRandomNumber() & 3) + 1; // [1, 4]
                    }
                    m_nPassengersGenerationState = TRAIN_PASSENGERS_TELL_PASSENGERS_TO_LEAVE;
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_TELL_PASSENGERS_TO_LEAVE) {
                    RemoveRandomPassenger();
                    if (m_nNumPassengersToLeave == 0) {
                        m_nPassengersGenerationState = TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_ENTER;
                    }
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_ENTER) {
                    if (localPlayer->m_pVehicle == this) {
                        m_nNumPassengersToEnter = 0;
                    } else {
                        m_nNumPassengersToEnter = CGeneral::GetRandomNumber() % 4 + 1; // rand(1, 4)
                    }
                    m_nPassengersGenerationState = TRAIN_PASSENGERS_TELL_PASSENGERS_TO_ENTER;
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_TELL_PASSENGERS_TO_ENTER) {
                    if (trainFlags.bPassengersCanEnterAndLeave) {
                        AddNearbyPedAsRandomPassenger();
                        if (m_nNumPassengersToLeave == m_nNumPassengersToEnter) {
                            m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                        }
                    }
                }
            }
        } else {
            CPad* pad = CPad::GetPad();
            if (m_pDriver && m_pDriver->IsPlayer()) {
                pad = m_pDriver->AsPlayer()->GetPadFromPlayer();
            }

            uint32 numCarriagesPulled = FindNumCarriagesPulled();
            if (!trainFlags.bClockwiseDirection) {
                m_fTrainSpeed = -m_fTrainSpeed;
            }

            if (GetStatus()) {
                bool bIsStreakModel = trainFlags.bIsStreakModel;
                auto fStopAtStationSpeed = static_cast<float>(m_autoPilot.m_nCruiseSpeed);

                uint32 timeAtStation = CTimer::GetTimeInMS() - m_nTimeWhenStoppedAtStation;
                if (timeAtStation >= (bIsStreakModel ? 20'000u : 10'000u)) {
                    if (timeAtStation >= (bIsStreakModel ? 28'000u : 18'000u)) {
                        if (timeAtStation >= (bIsStreakModel ? 32'000u : 22'000u)) {
                            if (trainFlags.bStopsAtStations) {
                                float maxTrainSpeed = 0.0f;
                                if (FindMaximumSpeedToStopAtStations(&maxTrainSpeed)) {
                                    fStopAtStationSpeed = 0.0f;
                                    m_nTimeWhenStoppedAtStation = CTimer::GetTimeInMS();
                                } else {
                                    if (fStopAtStationSpeed >= maxTrainSpeed) {
                                        fStopAtStationSpeed = maxTrainSpeed;
                                    }
                                }
                            }
                        } else if (trainFlags.bStoppedAtStation) {
                            CTrain* trainCarriage = this;
                            do {
                                trainFlags.bStoppedAtStation = false;
                                trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                                trainCarriage = trainCarriage->m_pNextCarriage;
                            } while (trainCarriage);
                        }
                    } else {
                        fStopAtStationSpeed = 0.0f;
                        if (trainFlags.bStoppedAtStation) {
                            CTrain* trainCarriage = this;
                            do {
                                trainFlags.bPassengersCanEnterAndLeave = false;
                                trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                                trainCarriage = trainCarriage->m_pNextCarriage;
                            } while (trainCarriage);
                        }
                    }
                } else {
                    fStopAtStationSpeed = 0.0f;
                    if (!trainFlags.bStoppedAtStation) {
                        CTrain* trainCarriage = this;
                        do {
                            trainFlags.bStoppedAtStation = true;
                            trainFlags.bPassengersCanEnterAndLeave = true;
                            trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_LEAVE;
                            trainCarriage = trainCarriage->m_pNextCarriage;
                        } while (trainCarriage);
                    }
                }

                fStopAtStationSpeed = fStopAtStationSpeed / 50.0f - m_fTrainSpeed;
                if (fStopAtStationSpeed > 0.0f) {
                    m_fTrainGas = fStopAtStationSpeed * 30.0f;
                    if (m_fTrainGas >= 1.0f) {
                        m_fTrainGas = 1.0f;
                    }

                    m_fTrainGas *= 255.0f;
                    m_fTrainBrake = 0.0f;
                } else {
                    float fTrainSpeed = fStopAtStationSpeed * -30.0f;
                    m_fTrainGas = 0.0f;
                    if (fTrainSpeed >= 1.0f) {
                        fTrainSpeed = 1.0f;
                    }
                    m_fTrainBrake = fTrainSpeed * 255.0f;
                }
            } else {
                float fTrainSpeed = m_fTrainSpeed;
                if (fTrainSpeed < 0.0f) {
                    fTrainSpeed = -fTrainSpeed;
                }

                if (fTrainSpeed < 0.001f) {
                    m_fTrainBrake = 0.0f;
                    m_fTrainGas = static_cast<float>(pad->GetAccelerate() - pad->GetBrake());
                } else {
                    if (m_fTrainSpeed > 0.0f) {
                        m_fTrainBrake = static_cast<float>(pad->GetBrake());
                        m_fTrainGas = static_cast<float>(pad->GetAccelerate());
                    } else {
                        m_fTrainGas = static_cast<float>(-pad->GetBrake());
                        m_fTrainBrake = static_cast<float>(pad->GetAccelerate());
                    }
                }
            }

            if (trainFlags.bForceSlowDown) {
                const CVector& vecPoint = GetPosition();
                CVector vecDistance{};
                if (CGameLogic::CalcDistanceToForbiddenTrainCrossing(vecPoint, m_vecMoveSpeed, true, vecDistance) < 230.0f) {
                    if (DotProduct(GetForwardVector(), vecDistance) <= 0.0f) {
                        m_fTrainGas = std::max(0.0f, m_fTrainGas);
                    } else {
                        m_fTrainGas = std::min(0.0f, m_fTrainGas);
                    }

                    if (CGameLogic::CalcDistanceToForbiddenTrainCrossing(vecPoint, m_vecMoveSpeed, false, vecDistance) < 230.0f) {
                        m_fTrainBrake = 512.0f;
                    }
                }
            }

            numCarriagesPulled += 3;

            m_fTrainSpeed += m_fTrainGas / 256.0f * CTimer::GetTimeStep() * 0.002f / float(numCarriagesPulled);

            if (m_fTrainBrake != 0.0f) {
                float fTrainSpeed = m_fTrainSpeed;
                if (m_fTrainSpeed < 0.0f) {
                    fTrainSpeed = -fTrainSpeed;
                }
                float fBreak = m_fTrainBrake / 256.0f * CTimer::GetTimeStep() * 0.006f / float(numCarriagesPulled);
                if (fTrainSpeed >= fBreak) {
                    if (m_fTrainSpeed < 0.0f) {
                        m_fTrainSpeed += fBreak;
                    } else {
                        m_fTrainSpeed -= fBreak;
                    }
                } else {
                    m_fTrainSpeed = 0.0f;
                }
            }

            m_fTrainSpeed *= pow(0.999750018119812f, CTimer::GetTimeStep());
            if (!trainFlags.bClockwiseDirection) {
                m_fTrainSpeed = -m_fTrainSpeed;
            }

            m_fCurrentRailDistance += CTimer::GetTimeStep() * m_fTrainSpeed;

            if (GetStatus() == STATUS_PLAYER) {

                float fTheTrainSpeed = m_fTrainSpeed;
                if (fTheTrainSpeed < 0.0f) {
                    fTheTrainSpeed = -fTheTrainSpeed;
                }
                if (fTheTrainSpeed > 1.0f * 0.95f) {
                    CPad::GetPad()->StartShake(300, 70, 0);
                    TheCamera.CamShake(0.1f, GetPosition());
                }

                fTheTrainSpeed = m_fTrainSpeed;
                if (fTheTrainSpeed < 0.0f) {
                    fTheTrainSpeed = -fTheTrainSpeed;
                }

                if (fTheTrainSpeed > 1.0f) {
                    int32 nNodeIndex = m_nNodeIndex;
                    int32 previousNodeIndex = nNodeIndex - 1;
                    if (previousNodeIndex < 0) {
                        previousNodeIndex = numTrackNodes;
                    }

                    int32 previousNodeIndex2 = previousNodeIndex - 1;
                    if (previousNodeIndex2 < 0) {
                        previousNodeIndex2 = numTrackNodes;
                    }

                    CTrainNode* pCurrentTrainNode = &trainNodes[m_nNodeIndex];
                    CTrainNode* pPreviousTrainNode = &trainNodes[previousNodeIndex];
                    CTrainNode* pPreviousTrainNode2 = &trainNodes[previousNodeIndex2];

                    CVector vecDifference1 = pCurrentTrainNode->GetPosn() - pPreviousTrainNode->GetPosn();
                    CVector vecDifference2 = pPreviousTrainNode->GetPosn() - pPreviousTrainNode2->GetPosn();
                    vecDifference1.Normalise();
                    vecDifference2.Normalise();

                    if (DotProduct(vecDifference1, vecDifference2) < 0.996f) {
                        CTrain* carriage = this;
                        bool bIsInTunnel = false;
                        while (!bIsInTunnel) {
                            bIsInTunnel = carriage->IsInTunnel();
                            carriage = carriage->m_pNextCarriage;
                            if (!carriage) {
                                if (!bIsInTunnel) {
                                    CTrain* theTrainCarriage = this;
                                    do {
                                        trainFlags.bNotOnARailRoad = true;
                                        theTrainCarriage->physicalFlags.bDisableCollisionForce = false;
                                        theTrainCarriage->physicalFlags.bDisableSimpleCollision = false;
                                        theTrainCarriage->SetIsStatic(false);
                                        theTrainCarriage = theTrainCarriage->m_pNextCarriage;
                                    } while (theTrainCarriage);

                                    CPhysical::ProcessControl();
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (m_fCurrentRailDistance < 0.0f) {
            do {
                m_fCurrentRailDistance += fTotalTrackLength;
            } while (m_fCurrentRailDistance < 0.0f);
        }

        if (m_fCurrentRailDistance >= fTotalTrackLength) {
            do {
                m_fCurrentRailDistance -= fTotalTrackLength;
            } while (m_fCurrentRailDistance >= fTotalTrackLength);
        }

        float fNextNodeTrackLength = 0.0f;
        int32 nextNodeIndex = m_nNodeIndex + 1;
        if (nextNodeIndex < numTrackNodes) {
            CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
            fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();
        } else {
            fNextNodeTrackLength = fTotalTrackLength;
            nextNodeIndex = 0;
        }

        CTrainNode* theTrainNode = &trainNodes[m_nNodeIndex];
        float fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();
        while (m_fCurrentRailDistance < fCurrentNodeTrackLength || fNextNodeTrackLength < m_fCurrentRailDistance) {
            int32 newNodeIndex = m_nNodeIndex - 1; // previous node
            if (fCurrentNodeTrackLength <= m_fCurrentRailDistance) {
                newNodeIndex = m_nNodeIndex + 1; // next node
            }
            m_nNodeIndex = newNodeIndex % numTrackNodes;
            m_vehicleAudio.AddAudioEvent(AE_TRAIN_CLACK, 0.0f);

            theTrainNode = &trainNodes[m_nNodeIndex];
            fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();

            nextNodeIndex = m_nNodeIndex + 1;
            if (nextNodeIndex < numTrackNodes) {
                CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
                fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();
            } else {
                fNextNodeTrackLength = fTotalTrackLength;
                nextNodeIndex = 0;
            }
        }

        CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
        fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();

        float fTrackNodeDifference = fNextNodeTrackLength - fCurrentNodeTrackLength;
        if (fTrackNodeDifference < 0.0f) {
            fTrackNodeDifference += fTotalTrackLength;
        }

        float fTheDistance = (m_fCurrentRailDistance - fCurrentNodeTrackLength) / fTrackNodeDifference;
        CVector vecPosition1 = theTrainNode->GetPosn() * (1.0f - fTheDistance) + nextTrainNode->GetPosn() * fTheDistance;

        CColModel* vehicleColModel = CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel();
        const CBoundingBox& bbox = vehicleColModel->GetBoundingBox();
        float fTotalCurrentRailDistance = bbox.GetLength() + m_fCurrentRailDistance;
        if (fTotalCurrentRailDistance > fTotalTrackLength) {
            fTotalCurrentRailDistance -= fTotalTrackLength;
        }

        nextNodeIndex = m_nNodeIndex + 1;
        if (nextNodeIndex < numTrackNodes) {
            fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();
        } else {
            fNextNodeTrackLength = fTotalTrackLength;
            nextNodeIndex = 0;
        }

        int32 trainNodeIndex = m_nNodeIndex;
        while (fTotalCurrentRailDistance < fCurrentNodeTrackLength || fTotalCurrentRailDistance > fNextNodeTrackLength) {
            trainNodeIndex = (trainNodeIndex + 1) % numTrackNodes;

            theTrainNode = &trainNodes[trainNodeIndex];
            fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();

            nextNodeIndex = trainNodeIndex + 1;
            if (nextNodeIndex < numTrackNodes) {
                fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();
            } else {
                fNextNodeTrackLength = fTotalTrackLength;
                nextNodeIndex = 0;
            }
        }

        fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();

        fTrackNodeDifference = fNextNodeTrackLength - fCurrentNodeTrackLength;
        if (fTrackNodeDifference < 0.0f) {
            fTrackNodeDifference += fTotalTrackLength;
        }

        fTheDistance = (fTotalCurrentRailDistance - fCurrentNodeTrackLength) / fTrackNodeDifference;
        CVector vecPosition2 = theTrainNode->GetPosn() * (1.0f - fTheDistance) + trainNodes[nextNodeIndex].GetPosn() * fTheDistance;

        {
            CVector& vecVehiclePosition = GetPosition();
            vecVehiclePosition = (vecPosition1 + vecPosition2) / 2.0f;
            vecVehiclePosition.z += m_pHandlingData->m_fSuspensionLowerLimit - bbox.m_vecMin.z;
        }

        GetForward() = vecPosition2 - vecPosition1;
        GetForward().Normalise();
        if (!trainFlags.bClockwiseDirection) {
            GetForward() *= -1.0f;
        }

        CVector vecTemp(0.0f, 0.0f, 1.0f);
        CrossProduct(&GetRight(), &GetForward(), &vecTemp);
        GetRight().Normalise();
        CrossProduct(&GetUp(), &GetRight(), &GetForward());

        auto fTrainNodeLighting     = theTrainNode->GetLightingFromCollision().GetCurrentLighting();
        auto fTrainNextNodeLighting = trainNodes[nextNodeIndex].GetLightingFromCollision().GetCurrentLighting();

        fTrainNodeLighting += (fTrainNextNodeLighting - fTrainNodeLighting) * fTheDistance;
        m_fContactSurfaceBrightness = fTrainNodeLighting;
        m_vecMoveSpeed = (1.0f / CTimer::GetTimeStep()) * (GetPosition() - vecOldTrainPosition);

        float fNewTrainHeading = GetHeading();
        float fHeading = fNewTrainHeading - fOldTrainHeading;
        if (fHeading <= PI) {
            if (fHeading < -PI) {
                fHeading += TWO_PI;
            }
        } else {
            fHeading -= TWO_PI;
        }

        m_vecTurnSpeed = CVector(0.0f, 0.0f, fHeading / CTimer::GetTimeStep());

        if (trainFlags.bNotOnARailRoad) {
            m_vecMoveSpeed *= -1.0f;
            m_vecTurnSpeed *= -1.0f;

            ApplyMoveSpeed();

            m_vecMoveSpeed *= -1.0f;
            m_vecTurnSpeed *= -1.0f;

            CPhysical::ProcessControl();
        } else {
            m_vecMoveSpeed.x = std::clamp(m_vecMoveSpeed.x, -2.0f, 2.0f);
            m_vecMoveSpeed.y = std::clamp(m_vecMoveSpeed.y, -2.0f, 2.0f);
            m_vecMoveSpeed.z = std::clamp(m_vecMoveSpeed.z, -2.0f, 2.0f);

            m_vecTurnSpeed.x = std::clamp(m_vecTurnSpeed.x, -0.1f, 0.1f);
            m_vecTurnSpeed.y = std::clamp(m_vecTurnSpeed.y, -0.1f, 0.1f);
            m_vecTurnSpeed.z = std::clamp(m_vecTurnSpeed.z, -0.1f, 0.1f);
        }

        UpdateRwMatrix();
        UpdateRwFrame();
        RemoveAndAdd();

        SetIsStuck(false);
        SetWasPostponed(false);
        SetIsInSafePosition(true);

        m_fMovingSpeed = DistanceBetweenPoints(GetPosition(), vecOldTrainPosition);

        if (trainFlags.bIsFrontCarriage || trainFlags.bIsLastCarriage) {
            CVector vecPoint = bbox.m_vecMax.y * GetForward();
            vecPoint += GetPosition();
            vecPoint += CTimer::GetTimeStep() * m_vecMoveSpeed;

            MarkSurroundingEntitiesForCollisionWithTrain(vecPoint, 3.0f, this, false);
        }

        if (!vehicleFlags.bWarnedPeds) {
            CCarCtrl::ScanForPedDanger(this);
        }
        return;
    } else {
        if (!GetIsStuck()) {
            float fMaxForce = 0.003f;
            float fMaxTorque = 0.0009f;
            float fMaxMovingSpeed = 0.005f;

            if (GetStatus() != STATUS_PLAYER) {
                fMaxForce = 0.006f;
                fMaxTorque = 0.0015f;
                fMaxMovingSpeed = 0.015f;
            }

            float fMaxForceTimeStep  = (fMaxForce  * CTimer::GetTimeStep()) * (fMaxForce * CTimer::GetTimeStep());
            float fMaxTorqueTimeStep = (fMaxTorque * CTimer::GetTimeStep()) * (fMaxTorque * CTimer::GetTimeStep());

            m_vecForce  = (m_vecForce  + m_vecMoveSpeed) / 2.0f;
            m_vecTorque = (m_vecTorque + m_vecTurnSpeed) / 2.0f;

            if (m_vecForce.SquaredMagnitude() > fMaxForceTimeStep ||
                m_vecTorque.SquaredMagnitude() > fMaxTorqueTimeStep ||
                m_fMovingSpeed >= fMaxMovingSpeed ||
                m_fDamageIntensity > 0.0f && m_pDamageEntity != nullptr && m_pDamageEntity->GetIsTypePed()
            ) {
                m_nFakePhysics = 0;
            } else {
                m_nFakePhysics += 1;
                if (m_nFakePhysics > 10 /*&& !plugin::Call<0x424100>()*/) {
                    // if (m_nFakePhysics > 10) { // OG redundant check
                        m_nFakePhysics = 10;
                    // }

                    ResetMoveSpeed();
                    ResetTurnSpeed();
                    SkipPhysics();
                    return;
                }
            }
        }

        CPhysical::ProcessControl();

        CVector vecMoveForce{}, vecTurnForce{};
        if (mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancyConstant, &vecMoveForce, &vecTurnForce)) {
            physicalFlags.bTouchingWater = true;

            float fTimeStep = 0.01f;
            if (CTimer::GetTimeStep() >= 0.01f) {
                fTimeStep = CTimer::GetTimeStep();
            }

            float fSpeedFactor = 1.0f - vecOldTrainPosition.z / (fTimeStep * m_fMass * 0.008f) * 0.05f;
            fSpeedFactor = std::pow(fSpeedFactor, CTimer::GetTimeStep());

            m_vecMoveSpeed *= fSpeedFactor;
            m_vecTurnSpeed *= fSpeedFactor;
            ApplyMoveForce(vecOldTrainPosition.x, vecOldTrainPosition.y, vecOldTrainPosition.z);
            ApplyTurnForce(vecTurnForce, vecMoveForce);
        }
    }
}


static_assert(offsetof(CTrain, m_nTrainFlags) == 0x5B8 && offsetof(CTrain, m_pTemporaryPassenger) == 0x5CC);
