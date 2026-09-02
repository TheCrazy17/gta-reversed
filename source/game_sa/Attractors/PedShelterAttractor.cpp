#include "StdInc.h"

#include "PedShelterAttractor.h"

// 0x5EF420
CVector CPedShelterAttractor::GetDisplacement(int32 pedId) {
    if (ms_displacements.empty()) {
        for (auto i = 0; i < 5; i++) {
            CVector candidate;
            do {
                const auto angle  = CGeneral::GetRandomNumberInRange(0.0f, TWO_PI);
                const auto radius = CGeneral::GetRandomNumberInRange(0.0f, 2.0f);
                candidate = { radius * std::cos(angle), radius * std::sin(angle), 0.0f };
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
    return plugin::CallMethodAndReturn<bool, 0x5EF570, CPedShelterAttractor*, CPed*>(this, ped);
}

void CPedShelterAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CPedShelterAttractor, 0x86C5B4, 6);
    RH_ScopedCategory("Attractors");

    RH_ScopedInstall(GetDisplacement, 0x5EF420);
    RH_ScopedVMTInstall(ComputeAttractPos, 0x5EFC40);
    RH_ScopedVMTInstall(ComputeAttractHeading, 0x5E9690);
    RH_ScopedVMTInstall(BroadcastDeparture, 0x5EF570, { .reversed = false });
}
