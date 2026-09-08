#include "StdInc.h"

#include "TaskSimpleFightingControl.h"
#include "PedStats.h"
#include "TaskSimpleFight.h"

#include "EventObjectCollision.h"
#include "EventBuildingCollision.h"

constexpr auto FIGHT_CTRL_MAX_ATTACK_ANGLE_RAD = DegreesToRadians(15);
constexpr auto FIGHT_CTRL_FIGHT_IDLE_TIME = 60000.f;
constexpr auto FIGHT_CTRL_NEXT_ATTACK_BASE_MS      = 2000;  // DAT_008D2E4C
constexpr auto FIGHT_CTRL_UNARMED_WAIT_CHANCE_MULT = 2.0f;  // DAT_008D2E54
constexpr auto FIGHT_CTRL_UNARMED_WAIT_MIN_MS      = 500;   // DAT_008D2E58
constexpr auto FIGHT_CTRL_UNARMED_WAIT_MAX_MS      = 2000;  // DAT_008D2E5C

void CTaskSimpleFightingControl::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleFightingControl, 0x86d700, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61DC10);
    RH_ScopedInstall(Destructor, 0x61DCA0);

    RH_ScopedInstall(CalcMoveCommand, 0x624B50);

    RH_ScopedVMTInstall(Clone, 0x622EB0);
    RH_ScopedVMTInstall(GetTaskType, 0x61DC90);
    RH_ScopedVMTInstall(MakeAbortable, 0x61DD00);
    RH_ScopedVMTInstall(ProcessPed, 0x62A0A0);
}

// 0x61DC10
CTaskSimpleFightingControl::CTaskSimpleFightingControl(CEntity* entityToFight, float angleRad, float attackRange) :
    m_target{entityToFight},
    m_angleRad{angleRad},
    m_maxAttackRange{ attackRange }
{
    CEntity::SafeRegisterRef(m_target);
}

// NOTSA
CTaskSimpleFightingControl::CTaskSimpleFightingControl(const CTaskSimpleFightingControl& o) :
    CTaskSimpleFightingControl{o.m_target, o.m_angleRad, o.m_maxAttackRange }
{
}

// 0x61DCA0
CTaskSimpleFightingControl::~CTaskSimpleFightingControl() {
    CEntity::SafeCleanUpRef(m_target);
}

// 0x624B50
int16 CTaskSimpleFightingControl::CalcMoveCommand(CPed* ped) {
    if (!m_target) {
        return -1;
    }

    const auto& targetPos = m_target->GetPosition();
    const auto& pedPos    = ped->GetPosition();

    const auto segmentPedToTarget = targetPos - pedPos;

    // Check if target is outside of the attacking angle
    if (std::abs(CGeneral::LimitRadianAngle(segmentPedToTarget.Heading()) - ped->m_fCurrentRotation) >= FIGHT_CTRL_MAX_ATTACK_ANGLE_RAD) {
        m_nextAttackTime += (uint32)CTimer::GetTimeStepInMS();
        return 0;
    }

    switch (m_target->GetType()) {
    case ENTITY_TYPE_PED: {
        const auto range = segmentPedToTarget.Magnitude() - m_maxAttackRange;
        if (range > 0.1f) {
            return 3;
        }

        const auto rnd = rand();

        if (range > -0.1f) { // Inverted
            return (rnd % 16 == 0) ? 7 : -1;
        }

        if (range >= 0.8f) {
            if (rand() % 64 == 0) {
                return 8;
            }
            if (rand() % 64 == 0) {
                return 10;
            }
        } else if (rnd % 16 == 0) {
            return 9;
        }
     
        break;
    }
    case ENTITY_TYPE_VEHICLE: {
        CVector planes[4]{};
        float planesDot[4]{};
        CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanesUncachedAll(targetPos.z, *m_target, &planes, planesDot);

        const auto hitSide = CPedGeometryAnalyser::ComputeEntityHitSide(pedPos, *m_target);
        if ((DotProduct(planes[hitSide], pedPos) + planesDot[hitSide]) >= 0.7f) {
            return 3;
        }

        break;
    }
    default: {
        if (segmentPedToTarget.Magnitude() - m_maxAttackRange > 0.3f) {
            return 3;
        }
        break;
    }
    }

    return -1;
}

// 0x61DD00
bool CTaskSimpleFightingControl::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    switch (priority) {
    case ABORT_PRIORITY_LEISURE:
        return false;
    case ABORT_PRIORITY_IMMEDIATE:
        return true;
    case ABORT_PRIORITY_URGENT: {
        if (event) {
            if (event->GetEventPriority() < 22) {
                return false;
            }

            switch (event->GetEventType()) {
            case EVENT_PED_COLLISION_WITH_PED:
            case EVENT_PED_COLLISION_WITH_PLAYER:
            case EVENT_PLAYER_COLLISION_WITH_PED: {
                return false;
            }
            case EVENT_VEHICLE_COLLISION:
            case EVENT_OBJECT_COLLISION:
            case EVENT_BUILDING_COLLISION: {
                if (!m_target) {
                    return true;
                }

                // This is not how they did it, but I'd like to avoid using blackmagic.
                // Once we reverse all stuff related to these events we should make a common base class
                // like `CEventEntityCollision` or something similar.
                const auto impactNormal = [event] {
                    switch (event->GetEventType()) {
                    case EVENT_VEHICLE_COLLISION:
                        return static_cast<const CEventVehicleCollision*>(event)->m_impactNormal;
                    case EVENT_OBJECT_COLLISION:
                        return static_cast<const CEventObjectCollision*>(event)->m_impactNormal;
                    case EVENT_BUILDING_COLLISION:
                        return static_cast<const CEventBuildingCollision*>(event)->m_impactNormal;
                    default:
                        NOTSA_UNREACHABLE();
                    }
                }();

                // Check if vector are opposing each other with 45deg tolerance
                if (-DotProduct(Normalized(m_target->GetPosition() - ped->GetPosition()), impactNormal) < SQRT_2 / 2) {
                    return false; // They aren't
                }
            }
            }
        }

        if (ped->GetIntelligence()->GetTaskFighting()) {
            return ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK)->MakeAbortable(ped, priority, event);
        }

        return true;
    }
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x62A0A0
bool CTaskSimpleFightingControl::ProcessPed(CPed* ped) {
    if (!m_target || m_bool) {
        return true;
    }

    // NOTSA: `m_unk3` is never written by any currently-reversed code path (default-constructed to
    // 0.f), so `rateFactor` below always collapses to a constant 0.3f in practice today - likely a
    // missing constructor parameter/setter elsewhere, out of scope for this function specifically.
    auto shootingRate = static_cast<float>(ped->m_nWeaponShootingRate);
    if (!ped->IsCreatedByMission() && ped->m_nPedType != PED_TYPE_COP && ped->m_nWeaponShootingRate == 40) {
        shootingRate = static_cast<float>(ped->m_pStats->m_flags);
    }

    ped->GiveWeaponAtStartOfFight();

    auto& taskMgr = ped->GetTaskManager();

    CTaskSimpleFight* fightTask{};
    int16 fallbackCmd = 0;
    if (const auto existing = taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK)) {
        if (existing->GetTaskType() != TASK_SIMPLE_FIGHT) {
            existing->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
            return false;
        }
        fightTask = static_cast<CTaskSimpleFight*>(existing);

        if (CTimer::GetTimeInMS() < m_nextAttackTime) { // Still on cooldown
            const auto giveUp = m_target->GetType() != ENTITY_TYPE_PED
                              || m_someTime != 0
                              || FIGHT_CTRL_UNARMED_WAIT_CHANCE_MULT * shootingRate * 0.025f <= CGeneral::GetRandomNumberInRange(0.f, 100.f);
            if (giveUp) {
                if (m_someTime != 0) {
                    const auto elapsed = static_cast<uint32>(CTimer::GetTimeStepInMS());
                    m_someTime = elapsed < m_someTime ? m_someTime - elapsed : 0;
                    fallbackCmd = 2;
                }
                // else: m_someTime already 0 -> leave fallbackCmd at 0
            } else {
                m_someTime = static_cast<uint32>(CGeneral::GetRandomNumberInRange(FIGHT_CTRL_UNARMED_WAIT_MIN_MS, FIGHT_CTRL_UNARMED_WAIT_MAX_MS));
                fallbackCmd = 2;
            }
        } else { // Cooldown expired
            m_nextAttackTime = 0;
            fallbackCmd = 0xb;
            if (ped->m_nFightingStyle != STYLE_STANDARD && ped->IsCurrentlyUnarmed()) {
                fallbackCmd = 0xc;
            }
        }
    } else {
        fightTask = new CTaskSimpleFight{ m_target, false, (uint32)FIGHT_CTRL_FIGHT_IDLE_TIME };
        taskMgr.SetTaskSecondary(fightTask, TASK_SECONDARY_ATTACK);
        m_nextAttackTime = 0;
    }

    m_maxAttackRange = CTaskSimpleFight::m_aComboData[std::max(0, fightTask->m_nComboSet - 4)].m_fRanges;

    if (m_nextAttackTime == 0) {
        const auto randomFactor = static_cast<float>(rand()) * (1.f / 32768.f) + 0.25f;
        const auto rateFactor   = shootingRate * m_unk3 * 0.025f * 0.7f + 0.3f;
        m_nextAttackTime = static_cast<uint32>((randomFactor / rateFactor) * static_cast<float>(FIGHT_CTRL_NEXT_ATTACK_BASE_MS)) + CTimer::GetTimeInMS();
    }

    auto cmd = fallbackCmd;
    if (fightTask->m_nComboSet <= 1) {
        if (const auto calculated = CalcMoveCommand(ped); calculated != -1) {
            cmd = calculated;
        }
    }
    fightTask->ControlFight(m_target, static_cast<uint8>(cmd));

    return false;
}
