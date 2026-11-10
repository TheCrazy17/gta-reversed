#include "StdInc.h"

#include "TaskComplexBeInCouple.h"
#include "TaskComplexWanderStandard.h"
#include "TaskComplexWalkAlongsidePed.h"
#include "TaskComplexWaitForPed.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexBeInCouple::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexBeInCouple, 0x870724, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6836F0);
    RH_ScopedInstall(Destructor, 0x683780);
    RH_ScopedInstall(AbortArmIK, 0x6837F0);
    RH_ScopedInstall(GetWalkSide, 0x683840);

    RH_ScopedVMTInstall(Clone, 0x6839C0);
    RH_ScopedVMTInstall(GetTaskType, 0x683770);
    RH_ScopedVMTInstall(MakeAbortable, 0x6847C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6837E0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x684840);
    RH_ScopedVMTInstall(ControlSubTask, 0x684930);
}

// 0x6836F0
CTaskComplexBeInCouple::CTaskComplexBeInCouple(
    CPed* ped,
    bool  isLeader,
    bool  holdHands,
    bool  lookAtEachOther,
    float giveUpDist
) :
    CTaskComplex(),
    m_partner{ped},
    m_isLeader{isLeader},
    m_holdHands{holdHands},
    m_lookAtEachOther{lookAtEachOther},
    m_giveUpDist{giveUpDist}
{
    CEntity::SafeRegisterRef(m_partner);
}

// 0x683780
CTaskComplexBeInCouple::~CTaskComplexBeInCouple() {
    CEntity::SafeCleanUpRef(m_partner);
}

// 0x6837E0
CTask* CTaskComplexBeInCouple::CreateFirstSubTask(CPed* ped) {
    return CreateNextSubTask(ped);
}

// 0x6837F0
void CTaskComplexBeInCouple::AbortArmIK(CPed* ped) {
    const auto DoAbortArmIK = [ped](eIKArm arm) {
        if (IKChainManager_c::IsArmPointing(arm, ped)) {
            IKChainManager_c::AbortPointArm(arm, ped, 250);
        }
    };
    DoAbortArmIK(eIKArm::IK_ARM_RIGHT);
    DoAbortArmIK(eIKArm::IK_ARM_LEFT);
}

// 0x683840
CTaskComplexBeInCouple::WalkSide CTaskComplexBeInCouple::GetWalkSide(CPed* ped) {
    // The exe reads the partner's matrix without a null check
    const auto& partnerRight = m_partner->GetRight();

    // Dot products with the partner's right vector, summed in the exe's order (y, z, x)
    const auto DotRight = [&](const CVector& pos) {
        return partnerRight.y * pos.y + partnerRight.z * pos.z + partnerRight.x * pos.x;
    };
    const auto partnerDot = DotRight(m_partner->GetPosition());
    const auto pedDot     = DotRight(ped->GetPosition());

    return pedDot + -partnerDot < 0.0f ? LEFT : RIGHT;
}

// 0x6847C0
bool CTaskComplexBeInCouple::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_partner && event && event->HasEditableResponse()) {
        auto editable = static_cast<CEventEditableResponse*>(const_cast<CEvent*>(event)); // Okay let's go!

        bool bAddToEventGroup = editable->m_bAddToEventGroup;
        editable->m_bAddToEventGroup = false;
        switch (event->GetEventType()) {
        case EVENT_DAMAGE:
        case EVENT_GUN_AIMED_AT:
        case EVENT_SHOT_FIRED:
            m_partner->GetEventGroup().Add(editable, false);
            break;
        }
        editable->m_bAddToEventGroup = bAddToEventGroup;
    }
    AbortArmIK(ped);
    return true;
}

// 0x684840
CTask* CTaskComplexBeInCouple::CreateNextSubTask(CPed* ped) {
    AbortArmIK(ped);

    if (!m_partner) {
        return nullptr;
    }

    // The leader wanders around, the other ped follows it
    if (m_isLeader) {
        return new CTaskComplexWanderStandard(PEDMOVE_WALK, CGeneral::RandomNodeHeading(), true);
    }
    return new CTaskComplexWalkAlongsidePed(m_partner, m_giveUpDist);
}

// 0x684930
CTask* CTaskComplexBeInCouple::ControlSubTask(CPed* ped) {
    if (!m_partner || !m_partner->IsAlive()) {
        AbortArmIK(ped);
        return nullptr;
    }

    // The partner has to be in a couple task too, with the opposite role, and has to be paired with this ped
    const auto partnerTask = static_cast<CTaskComplexBeInCouple*>(m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE));
    if (!partnerTask || partnerTask->m_isLeader == m_isLeader || partnerTask->m_partner != ped) {
        AbortArmIK(ped);
        return nullptr;
    }

    const auto pedTask        = ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE);
    const auto partnerTaskAct = m_partner->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE);
    const auto walkSide       = GetWalkSide(ped);

    if (!m_isLeader) {
        // Make the follower keep to the side it is on
        if (const auto walkTask = ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WALK_ALONGSIDE_PED)) {
            // CTaskComplexWalkAlongsidePed::m_Offset is private and has no setter; it is at +0x14
            auto& walkOffset = *reinterpret_cast<CVector*>(reinterpret_cast<uint8*>(walkTask) + 0x14);
            walkOffset       = CVector{walkSide == LEFT ? -1.05f : 1.05f, 0.0f, 0.0f};
        }
    }

    const auto& pedPos     = ped->GetPosition();
    const auto& partnerPos = m_partner->GetPosition();
    const auto  toPartnerX = partnerPos.x - pedPos.x;
    const auto  toPartnerY = partnerPos.y - pedPos.y;
    const auto  distSq     = toPartnerY * toPartnerY + toPartnerX * toPartnerX;

    // Too far apart, the couple is broken up
    if (distSq > m_giveUpDist * m_giveUpDist || !pedTask || !partnerTaskAct) {
        AbortArmIK(ped);
        return nullptr;
    }

    // The leader waits for a partner that fell behind
    if (distSq > 4.0f && m_isLeader) {
        AbortArmIK(ped);
        if (!ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WAIT_FOR_PED)) {
            return new CTaskComplexWaitForPed(m_partner, 0.75f, 20'000, false);
        }
    }

    if (m_holdHands) {
        if (distSq < 2.25f) {
            // Hold hands: point the arm facing the partner at the middle between the peds
            CVector middle = pedPos + CVector{toPartnerX * 0.5f, toPartnerY * 0.5f, 0.0f};

            if (walkSide != (uint8)m_prevSide) {
                AbortArmIK(ped);
            }
            if (walkSide == LEFT) {
                g_ikChainMan.PointArm("CTaskComplexBeInCouple", IK_ARM_RIGHT, ped, nullptr, BONE_UNKNOWN, &middle, 0.5f, 250, 30.0f);
                m_prevSide = (int8)walkSide;
            } else if (walkSide == RIGHT) {
                g_ikChainMan.PointArm("CTaskComplexBeInCouple", IK_ARM_LEFT, ped, nullptr, BONE_UNKNOWN, &middle, 0.5f, 250, 30.0f);
                m_prevSide = (int8)walkSide;
            }
        } else {
            AbortArmIK(ped);
        }
    }

    if (m_lookAtEachOther) {
        // Now and then each ped glances at the other one for a few seconds
        if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 80) {
            g_ikChainMan.LookAt("TaskBeInCouple", ped, m_partner, CGeneral::GetRandomNumberInRange(2000, 4000), BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        }
        if (!g_ikChainMan.IsLooking(m_partner) && CGeneral::GetRandomNumberInRange(0, 100) > 80) {
            g_ikChainMan.LookAt("TaskBeInCouple", m_partner, ped, CGeneral::GetRandomNumberInRange(2000, 4000), BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        }
    }

    return m_pSubTask;
}
