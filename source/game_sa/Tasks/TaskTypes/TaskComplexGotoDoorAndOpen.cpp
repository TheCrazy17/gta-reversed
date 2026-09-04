#include "StdInc.h"

#include "TaskComplexGotoDoorAndOpen.h"

#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleRunAnim.h"
#include "Events/EventAreaCodes.h"
#include "Events/EventGroup.h"
#include "RwHelper.h"

void CTaskComplexGotoDoorAndOpen::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGotoDoorAndOpen, 0x86FFC0, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedOverloadedInstall(Constructor, "0", 0x66BB20, CTaskComplexGotoDoorAndOpen*(CTaskComplexGotoDoorAndOpen::*)(CObject *));
    RH_ScopedOverloadedInstall(Constructor, "1", 0x66BBA0, CTaskComplexGotoDoorAndOpen*(CTaskComplexGotoDoorAndOpen::*)(CVector const&, CVector const&));
    RH_ScopedInstall(Destructor, 0x66BC00);
    RH_ScopedInstall(Clone, 0x66BCA0);
    RH_ScopedInstall(CreateNextSubTask, 0x66C0D0);
    RH_ScopedInstall(CreateFirstSubTask, 0x66BD40);
    RH_ScopedInstall(ControlSubTask, 0x66C1F0);
}

// 0x66BB20
CTaskComplexGotoDoorAndOpen::CTaskComplexGotoDoorAndOpen(CObject* door) : CTaskComplex() {
    m_Object = door;
    m_nStartTime = 0;
    m_nOffsetTime = 0;
    byte30 = 0;
    m_bRefreshTime = false;
    m_nFlags = m_nFlags & 0xF0 | 1; // todo: flags
    CEntity::SafeRegisterRef(m_Object);
}

// 0x66BBA0
CTaskComplexGotoDoorAndOpen::CTaskComplexGotoDoorAndOpen(const CVector& start, const CVector& end) : CTaskComplex() {
    m_Object = nullptr;
    m_Start = start;
    m_End = end;
    m_nStartTime = 0;
    m_nOffsetTime = 0;
    byte30 = 0;
    m_bRefreshTime = false;
    m_nFlags &= 244u; // todo: flags
}

// 0x66BC00
CTaskComplexGotoDoorAndOpen::~CTaskComplexGotoDoorAndOpen() {
    CEntity::SafeCleanUpRef(m_Object);
    if ((m_nFlags & 8) != 0) { // transition finished
        CPad::GetPad()->bPlayerOnInteriorTransition = false;
    }
}

// 0x66BCA0
CTask* CTaskComplexGotoDoorAndOpen::Clone() const {
    if ((m_nFlags & 1) != 0) { // todo: flags
        return new CTaskComplexGotoDoorAndOpen(m_Object);
    } else {
        return new CTaskComplexGotoDoorAndOpen(m_Start, m_End);
    }
}

// 0x66BC80
bool CTaskComplexGotoDoorAndOpen::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return priority == ABORT_PRIORITY_IMMEDIATE && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);
}

// 0x66C0D0
CTask* CTaskComplexGotoDoorAndOpen::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() != TASK_SIMPLE_GO_TO_POINT) {
        return nullptr;
    }

    if ((m_nFlags & 2) != 0) {
        if (ped->IsPlayer()) {
            CPad::GetPad(0)->bPlayerOnInteriorTransition = true;
        }
        return nullptr;
    }

    if (!ped->GetIntelligence()->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        ped->GetIntelligence()->GetTaskManager().SetTaskSecondary(
            new CTaskSimpleRunAnim(ANIM_GROUP_DEFAULT, ANIM_ID_WALK_DOORPARTIAL, 8.0f, false),
            TASK_SECONDARY_PARTIAL_ANIM
        );
    }

    auto* const nextTask = new CTaskSimpleGoToPoint(PEDMOVE_WALK, m_End, 0.2f, false, false);
    m_nFlags |= 2;
    return nextTask;
}

// 0x66BD40
CTask* CTaskComplexGotoDoorAndOpen::CreateFirstSubTask(CPed* ped) {
    if (ped->IsPlayer()) {
        CPad::GetPad(0)->bPlayerOnInteriorTransition = true;
        m_nFlags |= 8;
    }

    CEventAreaCodes eventAreaCodes{ ped };
    GetEventGlobalGroup()->Add(&eventAreaCodes, false);

    m_nStartTime  = CTimer::m_snTimeInMilliseconds;
    m_nOffsetTime = 1000;
    byte30        = 1;

    if (m_Object) {
        const auto up  = m_Object->GetUp();
        const auto mid = m_Object->GetPosition() + m_Object->GetRight() * 0.75f;

        if (DotProduct(m_Object->GetPosition() - ped->GetPosition(), up) <= 0.0f) {
            m_Start = mid + up * 0.5f;
            m_End   = mid - up * 2.0f;
        } else {
            m_Start = mid - up * 0.5f;
            m_End   = mid + up * 2.0f;
        }

        if (m_Object->physicalFlags.bCollidable) {
            m_nFlags |= 4;
            m_Object->physicalFlags.bCollidable = false;
            m_Object->physicalFlags.bDisableCollisionForce = false;
        }

        if (RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SPRINT)) {
            CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_WALK, 1000.0f);
        }

        return new CTaskSimpleGoToPoint(PEDMOVE_WALK, m_Start, 0.35f, false, false);
    }

    if ((m_nFlags & 1) != 0) {
        return nullptr;
    }

    m_nFlags |= 2;
    return new CTaskSimpleGoToPoint(PEDMOVE_WALK, m_End, 0.5f, false, false);
}

// 0x66C1F0
CTask* CTaskComplexGotoDoorAndOpen::ControlSubTask(CPed* ped) {
    if (byte30) {
        if (m_bRefreshTime) {
            m_nStartTime   = CTimer::m_snTimeInMilliseconds;
            m_bRefreshTime = false;
        }
        if (CTimer::m_snTimeInMilliseconds < m_nStartTime + m_nOffsetTime) {
            if (m_Object || (m_nFlags & 1) == 0) {
                return m_pSubTask;
            }
        }
    } else if (m_Object || (m_nFlags & 1) == 0) {
        return m_pSubTask;
    }

    if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        if (ped->IsPlayer()) {
            CPad::GetPad(0)->bPlayerOnInteriorTransition = true;
        }
        return nullptr;
    }
    return m_pSubTask;
}
