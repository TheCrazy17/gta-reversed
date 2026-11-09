#pragma once

#include "EventAcquaintancePed.h"

#include "EventEditableResponse.h"
#include "Vector.h"
#include "EventAcquaintancePed.h"


class NOTSA_EXPORT_VTABLE CEventAcquaintancePedDislike : public CEventAcquaintancePed {
public:
    using CEventAcquaintancePed::CEventAcquaintancePed;

    eEventType GetEventType() const override { return EVENT_ACQUAINTANCE_PED_DISLIKE; }
    int32 GetEventPriority() const override { return 24; }
    CEventEditableResponse* CloneEditable() const noexcept override { return new CEventAcquaintancePedDislike(m_AcquaintancePed); }

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CEventAcquaintancePedDislike* Constructor(CPed* ped) { this->CEventAcquaintancePedDislike::CEventAcquaintancePedDislike(ped); return this; }
    CEventAcquaintancePedDislike* Destructor() { this->CEventAcquaintancePedDislike::~CEventAcquaintancePedDislike(); return this; }
};
VALIDATE_SIZE(CEventAcquaintancePedDislike, 0x18);
