#include "StdInc.h"

#include "TaskComplexPartnerShove.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleRunAnim.h"

void CTaskComplexPartnerShove::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerShove, 0x870800, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6846F0);
    RH_ScopedInstall(Destructor, 0x684750);

    RH_ScopedVMTInstall(Clone, 0x684ED0);
    RH_ScopedVMTInstall(GetTaskType, 0x681E20);
    // The remaining slots are the base class's functions, which CTaskComplexPartner::InjectHooks already hooks
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0, { .Overrides = false });
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x681F20, { .Overrides = false });
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0, { .Overrides = false });
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x682310, { .Overrides = false });
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370, { .Overrides = false });
    RH_ScopedVMTInstall(GetPartnerSequence, 0x683120);
}

// 0x6846F0
CTaskComplexPartnerShove::CTaskComplexPartnerShove(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, false, updateDirectionCount, point)
{
    m_updateDirectionCount = updateDirectionCount;
    m_taskId = TASK_COMPLEX_PARTNER_SHOVE;
}

// 0x683120
CTaskComplexSequence* CTaskComplexPartnerShove::GetPartnerSequence() {
    const auto seq = new CTaskComplexSequence();
    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord(m_partner, 0.5f, 0.001f));

    // The lead speaker shoves, the other ped throws its hands up
    seq->AddTask(new CTaskSimpleRunAnim(ANIM_GROUP_DEFAULT, m_leadSpeaker ? ANIM_ID_SHOVE_PARTIAL : ANIM_ID_HANDSUP, 4.0f, false));
    return seq;
}
