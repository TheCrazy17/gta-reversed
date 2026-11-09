#include "StdInc.h"

#include "EventSource.h"

void CEventSource::InjectHooks() {
    RH_ScopedClass(CEventSource);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(ComputeEventSourceType, 0x4ABAC0);
}

// 0x4ABAC0
int32 CEventSource::ComputeEventSourceType(const CEvent& event, const CPed& ped) {
    const auto* const source = event.GetSourceEntity();
    if (!source || !source->GetIsTypePed()) {
        return 0;
    }

    const auto& sourcePed = *source->AsPed();
    const auto* const intel = ped.GetIntelligence();
    if (intel->IsThreatenedBy(sourcePed)) {
        return 3;
    }
    if (intel->IsFriendlyWith(sourcePed)) {
        return 2;
    }
    if (sourcePed.IsPlayer()) {
        return 1;
    }
    return 0;
}
