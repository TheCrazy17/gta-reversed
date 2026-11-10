#pragma once

#include "TaskComplex.h"

class CEntity;
class CEvent;
class CPed;

/// Ped reacts to being shot: either keeps going (player aiming or moving), gets knocked back by shotguns,
/// falls down when badly hurt, or plays the hit animation
class NOTSA_EXPORT_VTABLE CTaskComplexReactToAttack : public CTaskComplex {
public:
    static constexpr auto Type = TASK_COMPLEX_REACT_TO_ATTACK;

    static void InjectHooks();

    CTaskComplexReactToAttack(eWeaponType weaponType, CEntity* attacker, float damage, int32 direction, ePedPieceTypes pieceType);
    ~CTaskComplexReactToAttack() override;

    CTask*    Clone() const override { return new CTaskComplexReactToAttack{m_WeaponType, m_Attacker, m_Damage, m_Direction, m_PieceType}; } // 0x623300
    eTaskType GetTaskType() const override { return Type; }                                                                                  // 0x620B80
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

    CTask* CreateSubTask(eTaskType taskType);

private: // Wrappers for hooks
    // 0x620B00
    CTaskComplexReactToAttack* Constructor(eWeaponType weaponType, CEntity* attacker, float damage, int32 direction, ePedPieceTypes pieceType) {
        this->CTaskComplexReactToAttack::CTaskComplexReactToAttack(weaponType, attacker, damage, direction, pieceType);
        return this;
    }

    // 0x620B90
    CTaskComplexReactToAttack* Destructor() {
        this->CTaskComplexReactToAttack::~CTaskComplexReactToAttack();
        return this;
    }

private:
    bool           m_bRestart{};    //< `ControlSubTask` starts over while set; the exe only ever clears it
    bool           m_bAborted{};    //< The subtask was made abortable, so no next subtask is created
    eWeaponType    m_WeaponType{};
    CEntity*       m_Attacker{};
    float          m_Damage{};      //< Damage to the health, truncated for `CTaskSimpleBeHit`
    int32          m_Direction{};   //< As in `CTaskSimpleBeHit`; 2 is a hit from behind
    ePedPieceTypes m_PieceType{};
};
VALIDATE_SIZE(CTaskComplexReactToAttack, 0x24);
