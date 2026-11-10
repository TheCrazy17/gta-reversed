#pragma once

#include "TaskComplexWander.h"
#include "TaskTimer.h"
#include "eMoveState.h"

class NOTSA_EXPORT_VTABLE CTaskComplexWanderStandard : public CTaskComplexWander {
public:
    CTaskTimer m_TaskTimer{};
    uint32     m_nMinNextScanTime{}; // The exe constructor clears it

public:
    static void InjectHooks();

    CTaskComplexWanderStandard(eMoveState MoveState, uint8 Dir, bool bWanderSensibly = true);
    ~CTaskComplexWanderStandard() override = default; // 0x48E600

    // The exe clone passes the wander sensibly flag on as well
    CTask* Clone() const override { return new CTaskComplexWanderStandard(m_nMoveState, m_nDir, m_bWanderSensibly); } // 0x48E530
    eWanderType GetWanderType() override { return WANDER_TYPE_STANDARD; } // 0x48E5D0
    void ScanForStuff(CPed* ped) override; // 0x672600

    bool WillChat(const CPed& first, const CPed& second); // 0x66AED0
    void SetNextMinScanTime(CPed* ped);                   // 0x66AF60
    bool LookForSexyCars(CPed* ped);                      // 0x66AFD0
    bool LookForChatPartners(CPed* ped);                  // 0x66FDA0
    bool LookForGangMembers(CPed* ped);                   // 0x670100

private: // Wrappers for hooks
    // 0x48E4F0
    CTaskComplexWanderStandard* Constructor(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) {
        this->CTaskComplexWanderStandard::CTaskComplexWanderStandard(MoveState, Dir, bWanderSensibly);
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexWanderStandard, 0x38);
