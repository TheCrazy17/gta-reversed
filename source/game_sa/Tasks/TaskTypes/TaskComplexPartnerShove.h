#pragma once

#include "TaskComplexPartner.h"

class NOTSA_EXPORT_VTABLE CTaskComplexPartnerShove : public CTaskComplexPartner {
public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER_SHOVE;

    static void InjectHooks();

    CTaskComplexPartnerShove(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, CVector point);
    ~CTaskComplexPartnerShove() override = default;

    eTaskType GetTaskType() const override { return Type; } // 0x681E20
    CTask*    Clone() const override { return new CTaskComplexPartnerShove(m_commandName, m_partner, m_leadSpeaker, m_distanceMultiplier, m_updateDirectionCount, m_point); } // 0x684ED0
    CTaskComplexSequence* GetPartnerSequence() override;

private: // Wrappers for hooks
    // 0x6846F0
    CTaskComplexPartnerShove* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, CVector point) {
        this->CTaskComplexPartnerShove::CTaskComplexPartnerShove(commandName, partner, leadSpeaker, distanceMultiplier, updateDirectionCount, point);
        return this;
    }

    // 0x684750
    CTaskComplexPartnerShove* Destructor() {
        this->CTaskComplexPartnerShove::~CTaskComplexPartnerShove();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexPartnerShove, 0x74);
