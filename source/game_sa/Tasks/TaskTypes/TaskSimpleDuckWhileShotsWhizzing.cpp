#include "StdInc.h"

#include "TaskSimpleDuckWhileShotsWhizzing.h"
#include "AnimBlendAssociation.h"
#include "Events/Event.h"

void CTaskSimpleDuckWhileShotsWhizzing::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleDuckWhileShotsWhizzing, 0x870BEC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x692680);
    RH_ScopedVMTInstall(Clone, 0x692D60);
    RH_ScopedVMTInstall(GetTaskType, 0x6926E0);
    RH_ScopedVMTInstall(MakeAbortable, 0x692700);
    RH_ScopedVMTInstall(ProcessPed, 0x694640);
}

// 0x692680
CTaskSimpleDuckWhileShotsWhizzing::CTaskSimpleDuckWhileShotsWhizzing(uint16 lengthOfDuck) :
    CTaskSimpleDuck{DUCK_STANDALONE, lengthOfDuck, -1} // The exe inlines the base constructor with exactly these values
{
}

// 0x692700
bool CTaskSimpleDuckWhileShotsWhizzing::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority != ABORT_PRIORITY_IMMEDIATE && event && event->GetEventType() == EVENT_SHOT_FIRED_WHIZZED_BY) {
        // Only detaches the callbacks; unlike the base class the animations are not blended out here
        if (!m_DuckAnim) {
            return false;
        }

        m_DuckAnim->SetDefaultFinishCallback();
        m_DuckAnim = nullptr;

        if (m_MoveAnim) {
            m_MoveAnim->SetDefaultFinishCallback();
            m_MoveAnim = nullptr;
        }

        ped->bIsDucking = false;
        return true;
    }
    return CTaskSimpleDuck::MakeAbortable(ped, priority, event);
}

// 0x694640
bool CTaskSimpleDuckWhileShotsWhizzing::ProcessPed(CPed* ped) {
    if (!m_DuckAnim) {
        // Created here so the animation gets a delete callback; the base class would attach a finish callback instead
        m_DuckAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_DUCK_COWER, 4.0f);
        m_DuckAnim->SetDeleteCallback(DeleteDuckAnimCB, this);
    }
    return CTaskSimpleDuck::ProcessPed(ped);
}
