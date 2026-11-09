#pragma once

#include "EventEditableResponse.h"


class NOTSA_EXPORT_VTABLE CEventVehicleThreat : public CEventEditableResponse {
public:
    CVehicle* m_Vehicle;

public:
    CEventVehicleThreat(CVehicle *vehicle);
    ~CEventVehicleThreat() override;

    eEventType GetEventType() const override { return EVENT_VEHICLE_THREAT; }
    int32 GetEventPriority() const override { return 19; }
    int32 GetLifeTime() override { return 0; }
    bool AffectsPed(CPed* ped) override;
    CEventEditableResponse* CloneEditable() const noexcept override { return new CEventVehicleThreat(m_Vehicle); }

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CEventVehicleThreat* Constructor(CVehicle* vehicle) { this->CEventVehicleThreat::CEventVehicleThreat(vehicle); return this; }
    CEventVehicleThreat* Destructor() { this->CEventVehicleThreat::~CEventVehicleThreat(); return this; }
};
VALIDATE_SIZE(CEventVehicleThreat, 0x18);
