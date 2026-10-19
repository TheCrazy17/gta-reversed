#include "StdInc.h"

#include "DecisionMakerTypes.h"
#include "PedGroups.h"
#include "PedStats.h"

// The kind of decision maker (ped or group) that each slot holds
static auto& DecisionMakerTypeOfSlot = StaticRef<std::array<uint8, CDecisionMakerTypes::NUM_TYPES>>(0xC0AFE0);

void CDecisionMakerTypes::InjectHooks() {
    RH_ScopedClass(CDecisionMakerTypes);
    RH_ScopedCategory("DecisionMakers");
    RH_ScopedInstall(AddDecisionMaker, 0x607050);
    RH_ScopedInstall(GetInstance, 0x4684F0);
    RH_ScopedOverloadedInstall(MakeDecision, "ped-ptr", 0x606E70, void(CDecisionMakerTypes::*)(CPed*, eEventType, int32, bool, eTaskType, eTaskType, eTaskType, eTaskType, bool, int16&, int16&));
    RH_ScopedInstall(RemoveDecisionMaker, 0x6043A0);
    RH_ScopedOverloadedInstall(MakeDecision, "pedgroup-ptr", 0x606F80, eTaskType(CDecisionMakerTypes::*)(CPedGroup*, eEventType, int32, bool, eTaskType, eTaskType, eTaskType, eTaskType));
    RH_ScopedInstall(AddEventResponse, 0x6044C0);
    RH_ScopedInstall(FlushDecisionMakerEventResponse, 0x604490);
    RH_ScopedInstall(LoadEventIndices, 0x600840);
}

// 0x607050
int32 CDecisionMakerTypes::AddDecisionMaker(CDecisionMaker* decisionMaker, eDecisionTypes decisionMakerType, bool bDecisionMakerForMission) {
    // The first slots are for the game, the last ones for the missions
    const auto begin = bDecisionMakerForMission ? 15 : 0;
    const auto end   = bDecisionMakerForMission ? 20 : 15;
    for (auto slot = begin; slot < end; slot++) {
        if (m_IsActive[slot]) {
            continue;
        }

        m_IsActive[slot]               = true;
        DecisionMakerTypeOfSlot[slot]  = static_cast<uint8>(decisionMakerType);
        for (auto i = 0; i < CDecision::MAX_NUM_DECISIONS; i++) {
            m_DecisionMakers[slot].m_aDecisions[i].From(decisionMaker->m_aDecisions[i]);
        }
        m_NoOfDecisionMakers++;
        return slot;
    }
    return -1;
}

// 0x4684F0
CDecisionMakerTypes* CDecisionMakerTypes::GetInstance() {
    static auto& instance = StaticRef<CDecisionMakerTypes*>(0xC0B030);
    if (!instance) {
        instance = new CDecisionMakerTypes();
    }
    return instance;
}

// 0x606E70
void CDecisionMakerTypes::MakeDecision(CPed* ped, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskTypeToAvoid1, eTaskType taskTypeToAvoid2, eTaskType taskTypeToAvoid3, eTaskType taskTypeToSeek, bool bUseInGroupDecisionMaker, int16& taskType, int16& facialTaskType) {
    auto* const intel = ped->GetIntelligence();
    const auto  dm    = bUseInGroupDecisionMaker ? intel->m_nDecisionMakerTypeInGroup : intel->m_nDecisionMakerType;
    const auto  idx   = m_EventIndices[eventType];

    taskType       = TASK_NONE;
    facialTaskType = -1;

    // -2 is the player's, -1 means the default one for the kind of ped
    auto& decision = [&]() -> CDecision& {
        switch (dm) {
        case -2: return m_DefaultPlayerPedDecisionMaker.m_aDecisions[idx];
        case -1: return (ped->GetCreatedBy() == PED_MISSION ? m_DefaultMissionPedDecisionMaker : m_DefaultRandomPedDecisionMaker).m_aDecisions[idx];
        default: return m_DecisionMakers[dm].m_aDecisions[idx];
        }
    }();
    decision.MakeDecision(eventSourceType, bIsPedInVehicle, taskTypeToAvoid1, taskTypeToAvoid2, taskTypeToAvoid3, taskTypeToSeek, taskType, facialTaskType);
}

// 0x6043A0
void CDecisionMakerTypes::RemoveDecisionMaker(eDecisionTypes dm) {
    if (!m_IsActive[dm]) {
        return;
    }

    if (DecisionMakerTypeOfSlot[dm] == 0) {
        // Peds that use it go back to the one their stats say
        for (auto& ped : GetPedPool()->GetAllValid()) {
            auto* const intel = ped.GetIntelligence();
            if (intel->GetPedDecisionMakerType() == dm) {
                intel->SetPedDecisionMakerType(ped.m_pStats->m_nDefaultDecisionMaker);
            }
        }
    } else {
        for (auto&& [i, group] : CPedGroups::GetActiveGroupsWithIDs()) {
            if (static_cast<int32>(group.GetIntelligence().GetGroupDecisionMakerType()) == dm) {
                group.GetIntelligence().SetGroupDecisionMakerType(eDecisionMakerType::UNKNOWN);
            }
        }
    }

    m_IsActive[dm]              = false;
    DecisionMakerTypeOfSlot[dm] = 0;
    m_NoOfDecisionMakers--;
}

// 0x606F80
eTaskType CDecisionMakerTypes::MakeDecision(CPedGroup* pedGroup, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskId1, eTaskType taskId2, eTaskType taskId3, eTaskType taskId4) {
    const auto dm  = static_cast<int32>(pedGroup->GetIntelligence().GetGroupDecisionMakerType());
    const auto idx = m_EventIndices[eventType];

    auto& decision = dm == -1
        ? (pedGroup->m_bIsMissionGroup ? m_DefaultMissionPedGroupDecisionMaker : m_DefaultRandomPedGroupDecisionMaker).m_aDecisions[idx]
        : m_DecisionMakers[dm].m_aDecisions[idx];

    int16 taskType = TASK_NONE;
    int16 facialTaskType{};
    decision.MakeDecision(eventSourceType, bIsPedInVehicle, taskId1, taskId2, taskId3, taskId4, taskType, facialTaskType);
    return (eTaskType)taskType;
}

// 0x6044C0
void CDecisionMakerTypes::AddEventResponse(int32 decisionMakerIndex, eEventType eventType, eTaskType taskId, float* responseChances, int32* flags) {
    m_DecisionMakers[decisionMakerIndex].m_aDecisions[m_EventIndices[eventType]].Add(taskId, responseChances, flags);
}

// 0x604490
void CDecisionMakerTypes::FlushDecisionMakerEventResponse(int32 decisionMakerIndex, eEventType eventId) {
    m_DecisionMakers[decisionMakerIndex].m_aDecisions[m_EventIndices[eventId]].SetDefault();
}

void CDecisionMakerTypes::LoadEventIndices() {
    m_EventIndices.fill(0);

    CFileMgr::SetDir("");
    CFileMgr::SetDir("data\\decision\\");
    auto* const file = CFileMgr::OpenFile("PedEvent.txt", "r");
    CFileMgr::SetDir("");

    // Every line has the name of an event and its id, the position of the line is the index the event uses
    int32 index = 0;
    char  line[256];
    while (CFileMgr::ReadLine(file, line, sizeof(line))) {
        if (line[0] == '\0' || line[0] == '\n') {
            continue;
        }

        char  eventName[256];
        int32 eventId{};
        RET_IGNORED(sscanf_s(line, "%s %d", SCANF_S_STR(eventName), &eventId));
        if (eventId >= 0 && eventId < static_cast<int32>(m_EventIndices.size())) {
            m_EventIndices[eventId] = index;
        }
        index++;
    }

    CFileMgr::CloseFile(file);
}
