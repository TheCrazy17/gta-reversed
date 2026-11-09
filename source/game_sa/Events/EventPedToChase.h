#pragma once

#include "Event.h"

#include "Event.h"
#include "Ped.h"

class NOTSA_EXPORT_VTABLE CEventPedToChase : public CEvent {
public:
    CPed* m_ped;

public:
    CEventPedToChase(CPed* ped);
    ~CEventPedToChase() override;

    eEventType GetEventType() const override { return EVENT_PED_TO_CHASE; }
    int32 GetEventPriority() const override { return 1; }
    int32 GetLifeTime() override { return 0; }
    CEvent* Clone() const noexcept override { return new CEventPedToChase(m_ped); }
    bool AffectsPed(CPed* ped) override { return ped->IsAlive(); }

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CEventPedToChase* Constructor(CPed* ped) { this->CEventPedToChase::CEventPedToChase(ped); return this; }
    CEventPedToChase* Destructor() { this->CEventPedToChase::~CEventPedToChase(); return this; }
};
VALIDATE_SIZE(CEventPedToChase, 0x10);
