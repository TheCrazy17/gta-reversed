#include "StdInc.h"

#include "PedShelterAttractor.h"
#include "Tasks/TaskTypes/TaskComplexGoToAttractor.h"

// 0x5EF420
CVector CPedShelterAttractor::GetDisplacement(int32 pedId) {
    if (ms_displacements.empty()) {
        for (auto i = 0; i < 5; i++) {
            CVector candidate;
            do {
                const auto angle  = CGeneral::GetRandomNumberInRange(0.0f, TWO_PI);
                const auto radius = CGeneral::GetRandomNumberInRange(0.0f, 2.0f);
                candidate = CVector{ radius * std::cos(angle), radius * std::sin(angle), 0.0f };
            } while (rng::any_of(ms_displacements, [&](const auto& existing) {
                return (existing - candidate).SquaredMagnitude() < 1.0f;
            }));
            ms_displacements.emplace_back(candidate);
        }
    }
    return ms_displacements[pedId];
}

// 0x5EFC40
void CPedShelterAttractor::ComputeAttractPos(int32 pedId, CVector& outPos) {
    if (m_Fx) {
        CVector displacement = GetDisplacement(pedId);
        outPos = displacement + m_Pos;
    }
}

// 0x5E9690
void CPedShelterAttractor::ComputeAttractHeading(int32 bQueue, float& heading) {
    heading = CGeneral::GetRandomNumberInRange(0.0f, TWO_PI);
}

// 0x5EF570
bool CPedShelterAttractor::BroadcastDeparture(CPed* ped) {
    const auto it = rng::find(m_ArrivedPeds, ped);
    if (it == m_ArrivedPeds.end()) {
        return false;
    }

    if (const auto taskIt = rng::find(m_PedTaskPairs, ped, &CPedTaskPair::Ped); taskIt != m_PedTaskPairs.end()) {
        m_PedTaskPairs.erase(taskIt);
    }
    m_ArrivedPeds.erase(it);

    for (auto* const attractedPed : m_AttractPeds) {
        const auto idx = (int32)(m_ArrivedPeds.size());
        SetTaskForPed(attractedPed, new CTaskComplexGoToAttractor{
            this,
            CPedAttractor::ComputeAttractPos(idx),
            CPedAttractor::ComputeAttractHeading(idx),
            m_AchieveQueueTime,
            idx,
            PEDMOVE_WALK
        });
    }
    return true;
}

void CPedShelterAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CPedShelterAttractor, 0x86C5B4, 6);
    RH_ScopedCategory("Attractors");

    RH_ScopedInstall(GetDisplacement, 0x5EF420);
    RH_ScopedVMTInstall(ComputeAttractPos, 0x5EFC40);
    RH_ScopedVMTInstall(ComputeAttractHeading, 0x5E9690);
    RH_ScopedVMTInstall(BroadcastDeparture, 0x5EF570);
}
