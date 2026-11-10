#include "StdInc.h"

#include "TaskComplexReactToAttack.h"

#include "TaskSimpleBeHit.h"
#include "TaskSimpleNone.h"
#include "TaskComplexFallAndGetUp.h"

void CTaskComplexReactToAttack::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexReactToAttack, 0x86D868, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x620B00);
    RH_ScopedInstall(Destructor, 0x620B90);
    RH_ScopedInstall(CreateSubTask, 0x620C30);

    RH_ScopedVMTInstall(Clone, 0x623300);
    RH_ScopedVMTInstall(GetTaskType, 0x620B80);
    RH_ScopedVMTInstall(MakeAbortable, 0x620BF0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x625C20);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x625C70);
    RH_ScopedVMTInstall(ControlSubTask, 0x620E00);
}

// 0x620B00
CTaskComplexReactToAttack::CTaskComplexReactToAttack(eWeaponType weaponType, CEntity* attacker, float damage, int32 direction, ePedPieceTypes pieceType) :
    CTaskComplex{},
    m_bRestart{false},
    m_bAborted{false},
    m_WeaponType{weaponType},
    m_Attacker{attacker},
    m_Damage{damage},
    m_Direction{direction},
    m_PieceType{pieceType}
{
    CEntity::SafeRegisterRef(m_Attacker);
}

// 0x620B90
CTaskComplexReactToAttack::~CTaskComplexReactToAttack() {
    CEntity::SafeCleanUpRef(m_Attacker);
}

// 0x620BF0
bool CTaskComplexReactToAttack::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_bAborted) {
        return true;
    }

    if (priority != ABORT_PRIORITY_URGENT && priority != ABORT_PRIORITY_IMMEDIATE) {
        return false;
    }

    m_bAborted = m_pSubTask->MakeAbortable(ped, priority, event);
    return m_bAborted;
}

// 0x625C20
CTask* CTaskComplexReactToAttack::CreateNextSubTask(CPed* ped) {
    if (m_bAborted) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_NONE:
    case TASK_COMPLEX_FALL_AND_GET_UP:
    case TASK_SIMPLE_BE_HIT:
        return CreateSubTask(TASK_FINISHED);
    default:
        return nullptr;
    }
}

// 0x625C70
CTask* CTaskComplexReactToAttack::CreateFirstSubTask(CPed* ped) {
    m_bRestart = false;

    // The player stays in control while holding a firearm and giving input
    if (ped->IsPlayer() && m_WeaponType > WEAPON_CHAINSAW) {
        const auto pad = CPad::GetPad(0);
        if (!pad->DisablePlayerControls) {
            const auto& input = pad->NewState;
            if (pad->GetTarget() || input.LeftStickX || input.LeftStickY || input.DPadUp || input.DPadDown || input.DPadLeft || input.DPadRight) {
                return CreateSubTask(TASK_NONE);
            }
        }
    }

    if (m_PieceType == PED_PIECE_TORSO) {
        // Shotguns and (against NPCs only) the desert eagle throw the ped back and down
        if (ped->bCanBeShotInVehicle) {
            if (m_WeaponType == WEAPON_SPAS12_SHOTGUN || m_WeaponType == WEAPON_SHOTGUN || (m_WeaponType == WEAPON_DESERT_EAGLE && !ped->IsPlayer())) {
                // Pushes away from the attacker, 2D direction only
                CVector2D dir = CVector2D{m_Attacker->GetPosition()} - CVector2D{ped->GetPosition()};
                dir.Normalise();

                ped->bIsStanding = false; // Lifts off the ground
                ped->ApplyMoveForce(CVector{dir.x * -5.f, dir.y * -5.f, 5.f});
                return CreateSubTask(TASK_COMPLEX_FALL_AND_GET_UP);
            }
        }

        // Only melee weapons (ids below the chainsaw) knock a badly hurt ped down
        if (m_WeaponType < WEAPON_CHAINSAW && ped->m_fHealth < 30.f) {
            return CreateSubTask(TASK_COMPLEX_FALL_AND_GET_UP);
        }
    }

    return CreateSubTask(TASK_SIMPLE_BE_HIT);
}

// 0x620E00
CTask* CTaskComplexReactToAttack::ControlSubTask(CPed* ped) {
    if (m_bRestart) {
        return CreateFirstSubTask(ped);
    }

    // The exe also asks the subtask for its type here and ignores the answer
    return m_pSubTask;
}

// 0x620C30
CTask* CTaskComplexReactToAttack::CreateSubTask(eTaskType taskType) {
    switch (taskType) {
    case TASK_SIMPLE_BE_HIT: {
        const auto attackerPed = m_Attacker && m_Attacker->GetIsTypePed()
            ? m_Attacker->AsPed()
            : nullptr;
        return new CTaskSimpleBeHit{attackerPed, m_PieceType, m_Direction, (int32)m_Damage};
    }
    case TASK_COMPLEX_FALL_AND_GET_UP: {
        if (m_Direction == 2) { // Hit from behind
            return new CTaskComplexFallAndGetUp{ANIM_ID_KO_SKID_BACK, ANIM_GROUP_DEFAULT, 1000};
        }
        return new CTaskComplexFallAndGetUp{ANIM_ID_KO_SHOT_STOM, ANIM_GROUP_DEFAULT, 1000};
    }
    case TASK_NONE:
        return new CTaskSimpleNone{};
    default: // TASK_FINISHED and everything else
        return nullptr;
    }
}
