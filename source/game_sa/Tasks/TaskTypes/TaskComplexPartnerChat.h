#pragma once

#include "TaskComplexPartner.h"

class NOTSA_EXPORT_VTABLE CTaskComplexPartnerChat : public CTaskComplexPartner {
public:
    bool m_conversationEnabled;
    bool m_conversationOptional; // Carry on with a plain chat if the conversation could not be loaded, otherwise keep retrying
    bool m_pedConversationLoaded;

public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER_CHAT;

    static void InjectHooks();

    CTaskComplexPartnerChat(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool conversationOptional, CVector point);
    ~CTaskComplexPartnerChat() override;

    eTaskType GetTaskType() const override { return Type; } // 0x681E10
    CTask*    Clone() const override { return new CTaskComplexPartnerChat(m_commandName, m_partner, m_leadSpeaker, m_distanceMultiplier, m_updateDirectionCount, m_conversationEnabled, m_conversationOptional, m_point); } // 0x684E30
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTaskComplexSequence* GetPartnerSequence() override;

private: // Wrappers for hooks
    // 0x684290
    CTaskComplexPartnerChat* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool conversationOptional, CVector point) {
        this->CTaskComplexPartnerChat::CTaskComplexPartnerChat(commandName, partner, leadSpeaker, distanceMultiplier, updateDirectionCount, conversationEnabled, conversationOptional, point);
        return this;
    }

    // 0x684320
    CTaskComplexPartnerChat* Destructor() {
        this->CTaskComplexPartnerChat::~CTaskComplexPartnerChat();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexPartnerChat, 0x78);
