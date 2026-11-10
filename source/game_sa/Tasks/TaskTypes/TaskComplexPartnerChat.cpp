#include "StdInc.h"

#include "TaskComplexPartnerChat.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexChat.h"
#include "TaskSimpleChat.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexPartnerChat::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerChat, 0x8707C4, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x684290);
    RH_ScopedInstall(Destructor, 0x684320);

    RH_ScopedVMTInstall(Clone, 0x684E30);
    RH_ScopedVMTInstall(GetTaskType, 0x681E10);
    RH_ScopedVMTInstall(MakeAbortable, 0x682C60);
    // The remaining slots are the base class's functions, which CTaskComplexPartner::InjectHooks already hooks
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0, { .Overrides = false });
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x681F20, { .Overrides = false });
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0, { .Overrides = false });
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x682310, { .Overrides = false });
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370, { .Overrides = false });
    RH_ScopedVMTInstall(GetPartnerSequence, 0x684380);
}

// 0x684290
CTaskComplexPartnerChat::CTaskComplexPartnerChat(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool conversationOptional, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, false, updateDirectionCount, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_CHAT;
    m_pedConversationLoaded = false;
    m_conversationEnabled = conversationEnabled;
    if (conversationEnabled) {
        // A conversation runs the partner sequence four times, once for each pair of lines
        m_updateDirectionCount = 4;
    }
    m_conversationOptional = conversationOptional;
    strcpy_s(m_commandName, commandName);
}

// 0x684320
CTaskComplexPartnerChat::~CTaskComplexPartnerChat() {
    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
    }
}

// 0x682C60
bool CTaskComplexPartnerChat::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    // The exe forwards without a null check; the task always has a sub task while it runs.
    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }

    // Aborted, so the conversation this task started has to be released for the next one
    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
        m_pedConversationLoaded = false;
    }
    return true;
}

// 0x684380
CTaskComplexSequence* CTaskComplexPartnerChat::GetPartnerSequence() {
    const auto seq = new CTaskComplexSequence();
    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord(m_partner, 0.5f, 0.001f));

    // Both halves of a plain chat last between 3500 and 6500 ms and add up to a fixed total; the split is seeded
    // by the meeting point so every pair gets a different one
    const auto seed       = (int32)((float)(m_updateDirectionCount * 16) + m_point.z + m_point.y + m_point.x) & 0x7F;
    const auto split      = (float)seed * 0.0078125f;
    const auto firstTime  = (int32)(split * 3000.0f + 3500.0f);
    const auto secondTime = (int32)((1.0f - split) * 3000.0f + 3500.0f);

    if (!m_leadSpeaker) {
        // The lead speaker decided whether this chat is a conversation
        m_conversationEnabled = static_cast<CTaskComplexPartnerChat*>(m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT))->m_conversationEnabled;
    }

    if (m_conversationEnabled) {
        // The count is already decremented by CreateNextSubTask (3 down to 0), so each run takes the next pair of lines
        const auto lineIdx    = 6 - 2 * m_updateDirectionCount;
        const auto leadCtx    = CAEPedSpeechAudioEntity::s_Conversation[lineIdx].get_underlying();
        const auto partnerCtx = CAEPedSpeechAudioEntity::s_Conversation[lineIdx + 1].get_underlying();

        // Once the index of this run reaches the conversation length there are no further runs
        if (3 - m_updateDirectionCount == CAEPedSpeechAudioEntity::s_ConversationLength) {
            m_updateDirectionCount = 0;
        }

        // The lead speaker talks and then listens, the other ped listens and then talks
        if (m_leadSpeaker) {
            seq->AddTask(new CTaskComplexChat(true, m_partner, m_updateDirectionCount, leadCtx));
            seq->AddTask(new CTaskComplexChat(false, m_partner, m_updateDirectionCount, leadCtx));
        } else {
            seq->AddTask(new CTaskComplexChat(false, m_partner, m_updateDirectionCount, partnerCtx));
            seq->AddTask(new CTaskComplexChat(true, m_partner, m_updateDirectionCount, partnerCtx));
        }
    } else if (m_leadSpeaker) {
        seq->AddTask(new CTaskSimpleChat(firstTime));
        seq->AddTask(new CTaskSimpleStandStill(secondTime, false, false, 8.0f));
    } else {
        seq->AddTask(new CTaskSimpleStandStill(firstTime, false, false, 8.0f));
        seq->AddTask(new CTaskSimpleChat(secondTime));
    }
    return seq;
}
