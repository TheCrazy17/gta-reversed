#pragma once

#include "TaskSimple.h"

class CAnimBlendAssociation;
class CEvent;
class CPed;

/// Ped lying on the ground gets kicked; plays the "floor hit" animation once
class NOTSA_EXPORT_VTABLE CTaskSimpleBeKickedOnGround : public CTaskSimple {
public:
    static constexpr auto Type = TASK_SIMPLE_BE_KICKED_ON_GROUND;

    static void InjectHooks();

    CTaskSimpleBeKickedOnGround();
    ~CTaskSimpleBeKickedOnGround() override;

    CTask*    Clone() const override { return new CTaskSimpleBeKickedOnGround{}; } // 0x623120
    eTaskType GetTaskType() const override { return Type; }                        // 0x61FC40
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool      ProcessPed(CPed* ped) override;

    void StartAnim(CPed* ped);

    static void FinishAnimCB(CAnimBlendAssociation* anim, void* data);

private: // Wrappers for hooks
    // 0x61FC20
    CTaskSimpleBeKickedOnGround* Constructor() {
        this->CTaskSimpleBeKickedOnGround::CTaskSimpleBeKickedOnGround();
        return this;
    }

    // 0x61FC50
    CTaskSimpleBeKickedOnGround* Destructor() {
        this->CTaskSimpleBeKickedOnGround::~CTaskSimpleBeKickedOnGround();
        return this;
    }

private:
    bool                   m_bIsFinished{}; //< Set by `FinishAnimCB`, or by `StartAnim` when kicking a downed ped is disabled
    CAnimBlendAssociation* m_Anim{};        //< The playing animation (null while none)
};
VALIDATE_SIZE(CTaskSimpleBeKickedOnGround, 0x10);
