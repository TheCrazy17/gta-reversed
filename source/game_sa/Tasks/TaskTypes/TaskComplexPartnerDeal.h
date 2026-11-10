#pragma once

#include "TaskComplexPartner.h"

class NOTSA_EXPORT_VTABLE CTaskComplexPartnerDeal : public CTaskComplexPartner {
public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER_DEAL;

    static void InjectHooks();

    CTaskComplexPartnerDeal(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point);
    ~CTaskComplexPartnerDeal() override = default;

    eTaskType GetTaskType() const override { return Type; } // 0x681DF0
    CTask*    Clone() const override { return new CTaskComplexPartnerDeal(m_commandName, m_partner, m_leadSpeaker, m_distanceMultiplier, m_point); } // 0x684D10
    void      StreamRequiredAnims() override;
    CTaskComplexSequence* GetPartnerSequence() override;

private: // Wrappers for hooks
    // 0x684190
    CTaskComplexPartnerDeal* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point) {
        this->CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(commandName, partner, leadSpeaker, distanceMultiplier, point);
        return this;
    }

    // 0x684200
    CTaskComplexPartnerDeal* Destructor() {
        this->CTaskComplexPartnerDeal::~CTaskComplexPartnerDeal();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexPartnerDeal, 0x74);
