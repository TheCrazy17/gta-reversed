#include "ConversationForPed.h"
#include "Conversations.h"
#include "PedGroups.h"

void CConversationForPed::InjectHooks() {
    RH_ScopedClass(CConversationForPed);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Update, 0x43C190, {.Reversed = false});
    RH_ScopedInstall(IsPlayerInPositionForConversation, 0x43AC40);
}

void CConversationForPed::Clear(bool dontClearNodes) {
    if (!dontClearNodes) {
        CConversations::m_Nodes[m_FirstNode].ClearRecursively();
    }
    m_FirstNode                 = -1;
    m_CurrentNode               = -1;
    m_pPed                      = nullptr;
    m_LastChange                = 0;
    m_LastTimeWeWereCloseEnough = 0;
}

// 0x43C190
void CConversationForPed::Update() {
    plugin::CallMethod<0x43C190, CConversationForPed*>(this);
}

// 0x43AC40
bool CConversationForPed::IsPlayerInPositionForConversation(bool randomConversation) {
    auto* const player = FindPlayerPed();

    // Close enough, and facing each other
    if ((FindPlayerCoors() - m_pPed->GetPosition()).Magnitude() > 4.0f) {
        return false;
    }
    if (player->GetForward().Dot(m_pPed->GetForward()) > 0.0f) {
        return false;
    }

    // The player has to stand still
    if (player->m_vecMoveSpeed.Magnitude2D() > 0.01f) {
        return false;
    }

    // And the ped can't have been hurt by the player recently
    if (m_pPed->m_pLastEntityDamage == player && CTimer::GetTimeInMS() < (uint32)m_pPed->field_768 + 6000) {
        return false;
    }

    if (randomConversation && CPedGroups::GetGroup(player->GetPlayerData()->m_nPlayerGroup).GetMembership().CountMembersExcludingLeader() > 0) {
        return false;
    }

    return player->PedIsReadyForConversation(randomConversation) && m_pPed->PedIsReadyForConversation(randomConversation);
}
