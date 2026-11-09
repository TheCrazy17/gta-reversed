#include "StdInc.h"

#include "PlayerRelationshipRecorder.h"
#include "TaskCategories.h"

void CPlayerRelationshipRecorder::InjectHooks() {
    RH_ScopedClass(CPlayerRelationshipRecorder);
    RH_ScopedCategory("Core");

    RH_ScopedInstall(Constructor, 0x61A130);
    RH_ScopedInstall(Destructor, 0x61A2C0);
    RH_ScopedInstall(Flush, 0x61A2A0);
    RH_ScopedInstall(AddRelationship, 0x61A180);
    RH_ScopedInstall(ClearRelationshipWithPlayer, 0x61A150);
    RH_ScopedInstall(GetRelationshipWithPlayer, 0x61A1A0);
    RH_ScopedInstall(RecordRelationshipWithPlayer, 0x61A1D0);
    RH_ScopedGlobalInstall(GetPlayerRelationshipRecorder, 0x61A2E0);
}

// 0x61A130
CPlayerRelationshipRecorder::CPlayerRelationshipRecorder() {
    Flush();
}

// 0x61A2C0
CPlayerRelationshipRecorder::~CPlayerRelationshipRecorder() {
    Flush();
}

// 0x61A2A0
void CPlayerRelationshipRecorder::Flush() {
    for (auto& relationship : m_Relationships) {
        relationship.Flush();
    }
}

// 0x61A180
void CPlayerRelationshipRecorder::AddRelationship(const CPed* ped, int32 value) {
    // Only the first slot is ever written, and only when it is already taken; the exe tests it that way
    auto& rel = m_Relationships[0];
    if (rel.Ped) {
        rel.Ped = ped;
        rel.Relationship = value;
    }
}

// 0x61A1D0
void CPlayerRelationshipRecorder::RecordRelationshipWithPlayer(const CPed* ped) {
    ClearRelationshipWithPlayer(ped);

    const auto* const task = ped->GetTaskManager().GetActiveTask();
    if (!task) {
        return;
    }

    // Each query below is only judged by its SECOND flag, which the exe versions of IsKillPedTask and
    // IsFollowPedTask never set, so nothing ends up being recorded; the structure is kept as the exe has it
    bool isKill{}, killFlag{};
    CTaskCategories::IsKillPedTask(task, isKill, killFlag);
    if (killFlag) {
        AddRelationship(ped, 3);
        return;
    }

    bool isFollow{}, followFlag{};
    CTaskCategories::IsFollowPedTask(task, isFollow, followFlag);
    if (followFlag) {
        AddRelationship(ped, 3);
        return;
    }

    bool isKill2{}, killFlag2{};
    CTaskCategories::IsKillPedTask(task, isKill2, killFlag2);
    if (killFlag2) {
        AddRelationship(ped, 7);
    }
}

// 0x61A1A0
int32 CPlayerRelationshipRecorder::GetRelationshipWithPlayer(const CPed* ped) {
    for (auto& relationship : m_Relationships) {
        if (relationship.Ped != ped)
            continue;

        return relationship.Relationship;
    }
    return 0;
}

// 0x61A150
void CPlayerRelationshipRecorder::ClearRelationshipWithPlayer(const CPed* ped) {
    for (auto& relationship : m_Relationships) {
        if (relationship.Ped != ped)
            continue;

        return relationship.Flush();
    }
}

// 0x61A2E0
CPlayerRelationshipRecorder& GetPlayerRelationshipRecorder() {
    static auto& g_sPlayerRelationshipRecorder = StaticRef<CPlayerRelationshipRecorder*>(0xC17084);

    if (!g_sPlayerRelationshipRecorder) {
        g_sPlayerRelationshipRecorder = new CPlayerRelationshipRecorder();
    }
    return *g_sPlayerRelationshipRecorder;
}
