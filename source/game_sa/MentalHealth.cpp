#include "StdInc.h"
#include "MentalHealth.h"

#include "Events/EventHealthLow.h"
#include "Events/EventHealthReallyLow.h"
#include "Events/EventHighAngerAtPlayer.h"
#include "Events/EventLowAngerAtPlayer.h"

void CMentalState::InjectHooks() {
    RH_ScopedClass(CMentalState);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(IncrementAnger, 0x421050);
    RH_ScopedInstall(Process, 0x6008A0);
}

// 0x421050
void CMentalState::IncrementAnger(int32 anger) {
    if (!m_AngerTimer.IsOutOfTime()) {
        return;
    }
    m_AngerTimer.Start(3000);
    m_AngerAtPlayer += anger;
}

// 0x6008A0
void CMentalState::Process(CPed& ped) {
    m_pedHealth = (uint8)ped.m_fHealth;
    if (ped.bInVehicle && ped.m_pVehicle) {
        m_vehicleHealth = (uint8)ped.m_pVehicle->m_fHealth;
    }

    auto& events = ped.GetEventGroup();

    if (!ped.bInVehicle) {
        if (m_oldPedHealth >= 50) {
            if (m_pedHealth < 10) {
                events.Add(CEventHealthReallyLow{});
            } else if (m_pedHealth < 50) {
                events.Add(CEventHealthLow{});
            }
        } else if (m_oldPedHealth >= 10 && m_pedHealth < 10) {
            events.Add(CEventHealthReallyLow{});
        }
    } else {
        // The vehicle health is a byte, so these never hold (kept like the original)
        const int32 oldVehicleHealth = m_oldVehicleHealth;
        const int32 vehicleHealth    = m_vehicleHealth;
        if (oldVehicleHealth >= 600) {
            if (vehicleHealth < 300) {
                events.Add(CEventHealthReallyLow{});
            } else if (vehicleHealth < 600) {
                events.Add(CEventHealthLow{});
            }
        } else if (oldVehicleHealth >= 300 && vehicleHealth < 300) {
            events.Add(CEventHealthReallyLow{});
        }
    }

    if (m_LastAngerAtPlayer <= 3) {
        if (m_AngerAtPlayer > 6) {
            events.Add(CEventHighAngerAtPlayer{});
        } else if (m_AngerAtPlayer > 3) {
            events.Add(CEventLowAngerAtPlayer{});
        }
    } else if (m_LastAngerAtPlayer <= 6 && m_AngerAtPlayer > 6) {
        events.Add(CEventHighAngerAtPlayer{});
    }

    m_LastAngerAtPlayer = m_AngerAtPlayer;
    m_oldPedHealth      = m_pedHealth;
    m_oldVehicleHealth  = m_vehicleHealth;
}
