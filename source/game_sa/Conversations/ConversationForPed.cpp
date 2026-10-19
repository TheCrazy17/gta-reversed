#include "ConversationForPed.h"
#include "Conversations.h"
#include "PedGroups.h"

void CConversationForPed::InjectHooks() {
    RH_ScopedClass(CConversationForPed);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Update, 0x43C190);
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
    const auto now = CTimer::GetTimeInMS();

    if (!m_pPed || !m_Enabled || !IsPlayerInPositionForConversation(false)) {
        m_Status = INACTIVE;
        return;
    }

    // The ped speaks (or says something awkward if the player isn't answering)
    const auto PedSpeaks = [&] {
        const auto& node = CConversations::m_Nodes[m_CurrentNode];
        if (!m_SuppressSubtitles) {
            CMessages::ClearSmallMessagesOnly();
            CMessages::AddMessageJump(TheText.Get(node.m_Name), 4000, 1, true);
        }
        if (node.m_Speech > 0) {
            m_pPed->Say((eGlobalSpeechContext)(int16)node.m_Speech);
        }
        if (node.m_Speech < 0) {
            CConversations::AwkwardSay(-node.m_Speech, m_pPed);
        }
        m_Status     = PED_SPEAKING;
        m_LastChange = now;
    };

    // The player picked an answer, and the next node starts
    const auto PlayerAnswers = [&](bool yes) {
        const auto& node = CConversations::m_Nodes[m_CurrentNode];
        if (!m_SuppressSubtitles) {
            CMessages::ClearSmallMessagesOnly();
            const auto key = std::string{ node.m_Name, strnlen(node.m_Name, sizeof(node.m_Name)) } + (yes ? 'Y' : 'N');
            CMessages::AddMessageJump(TheText.Get(key.c_str()), 4000, 3, true);
        }
        const auto speech = yes ? node.m_SpeechY : node.m_SpeechN;
        if (speech > 0) {
            FindPlayerPed()->Say((eGlobalSpeechContext)(int16)speech);
        }
        if (speech < 0) {
            CConversations::AwkwardSay(-speech, FindPlayerPed());
        }
        m_CurrentNode = yes ? node.m_NodeYes : node.m_NodeNo;
        m_LastChange  = now;
        if (!m_SuppressSubtitles) {
            CMessages::AddMessageQ(TheText.Get(CConversations::m_Nodes[m_CurrentNode].m_Name), 4000, 1, true);
        }
        m_Status = PLAYER_SPEAKING;
    };

    // Time to wait for the player's answer ran out, or the player is already answering
    bool pedSpeaks = true;
    if (now <= m_LastTimeWeWereCloseEnough + 10000) {
        pedSpeaks = false;
        if (m_Status == PLAYER_SPEAKING) {
            pedSpeaks = now > m_LastChange + 4000;
            if (!pedSpeaks) {
                auto* const pad = CPad::GetPad(0);
                pedSpeaks = pad->ConversationNoJustDown() || pad->ConversationYesJustDown();
            }
        }
    }

    if (pedSpeaks) {
        PedSpeaks();
    } else if (m_LastChange + 400 < now && (m_Status == PED_SPEAKING || m_Status == WAITINGFORINPUT || m_Status == INACTIVE)) {
        const auto& node = CConversations::m_Nodes[m_CurrentNode];
        if (CPad::GetPad(0)->ConversationNoJustDown() && node.m_NodeNo >= 0) {
            PlayerAnswers(false);
        } else if (CPad::GetPad(0)->ConversationYesJustDown() && node.m_NodeYes >= 0) {
            PlayerAnswers(true);
        }
    }

    m_LastTimeWeWereCloseEnough = now;
    if (m_Status == PED_SPEAKING && m_LastChange + 4000 < now) {
        m_Status = WAITINGFORINPUT;
    }
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
