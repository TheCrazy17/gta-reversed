#pragma once

#include "TaskSimple.h"

class CAnimBlendAssociation;
class CEvent;
class CPed;

/// Ped gets hit while moving; plays a stagger or knock down animation depending on the hit direction.
/// Nothing in the exe constructs it except `Clone`, so the exact meaning of `m_HitType` is unknown.
class NOTSA_EXPORT_VTABLE CTaskSimpleBeHitWhileMoving : public CTaskSimple {
public:
    static constexpr auto Type = TASK_SIMPLE_BE_HIT_WHILE_MOVING;

    static void InjectHooks();

    /*!
    * @param hitType   Only 2 and 4 (which is `PEDMOVE_WALK`) can start the full reactions, everything else plays the partial shot animations
    * @param direction The direction of the hit as in `CTaskSimpleBeHit`: 1 left, 2 back, 3 right, anything else front
    */
    CTaskSimpleBeHitWhileMoving(int32 hitType, int32 direction);
    ~CTaskSimpleBeHitWhileMoving() override;

    CTask*    Clone() const override { return new CTaskSimpleBeHitWhileMoving{m_HitType, m_Direction}; } // 0x623190
    eTaskType GetTaskType() const override { return Type; }                                            // 0x61FE60
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool      ProcessPed(CPed* ped) override;

    void StartAnim(CPed* ped);

    static void FinishAnimCB(CAnimBlendAssociation* anim, void* data);

private: // Wrappers for hooks
    // 0x61FE30
    CTaskSimpleBeHitWhileMoving* Constructor(int32 hitType, int32 direction) {
        this->CTaskSimpleBeHitWhileMoving::CTaskSimpleBeHitWhileMoving(hitType, direction);
        return this;
    }

    // 0x61FE70
    CTaskSimpleBeHitWhileMoving* Destructor() {
        this->CTaskSimpleBeHitWhileMoving::~CTaskSimpleBeHitWhileMoving();
        return this;
    }

private:
    bool                   m_bIsFinished{}; //< Set by `FinishAnimCB`
    int32                  m_HitType{};
    int32                  m_Direction{};
    CAnimBlendAssociation* m_Anim{};        //< The playing animation (null while none)
};
VALIDATE_SIZE(CTaskSimpleBeHitWhileMoving, 0x18);
