#pragma once

#include "EventAcquaintancePed.h"

#include "EventEditableResponse.h"
#include "Vector.h"
#include "EventAcquaintancePed.h"


class NOTSA_EXPORT_VTABLE CEventAcquaintancePedLike : public CEventAcquaintancePed {
public:
    using CEventAcquaintancePed::CEventAcquaintancePed;

    eEventType GetEventType() const override { return EVENT_ACQUAINTANCE_PED_LIKE; }
    int32 GetEventPriority() const override { return 22; }
    CEventEditableResponse* CloneEditable() const noexcept override { return new CEventAcquaintancePedLike(m_AcquaintancePed); }

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CEventAcquaintancePedLike* Constructor(CPed* ped) { this->CEventAcquaintancePedLike::CEventAcquaintancePedLike(ped); return this; }
    CEventAcquaintancePedLike* Destructor() { this->CEventAcquaintancePedLike::~CEventAcquaintancePedLike(); return this; }
};
VALIDATE_SIZE(CEventAcquaintancePedLike, 0x18);
