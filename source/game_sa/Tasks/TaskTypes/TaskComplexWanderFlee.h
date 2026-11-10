#pragma once

#include "TaskComplexWander.h"
#include "eMoveState.h"

class CPed;
class CTaskComplexWanderFlee;

class NOTSA_EXPORT_VTABLE CTaskComplexWanderFlee final : public CTaskComplexWander {
public:
    static void InjectHooks();

    CTaskComplexWanderFlee(eMoveState moveState, uint8 dir);
    CTaskComplexWanderFlee(const CTaskComplexWanderFlee&);
    ~CTaskComplexWanderFlee() = default;

    CTask*      Clone() const override { return new CTaskComplexWanderFlee{ *this }; } // 0x65CD70
    eWanderType GetWanderType() override { return WANDER_TYPE_FLEE; } // 0x65B350
    void        ScanForStuff(CPed* ped) override { /*nop*/ } // 0x65B360

private: // Wrappers for hooks
    // 0x65B320
    CTaskComplexWanderFlee* Constructor(eMoveState moveState, uint8 dir) {
        this->CTaskComplexWanderFlee::CTaskComplexWanderFlee(moveState, dir);
        return this;
    }

    // 0x65B370
    CTaskComplexWanderFlee* Destructor() {
        this->CTaskComplexWanderFlee::~CTaskComplexWanderFlee();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexWanderFlee, 0x28);
