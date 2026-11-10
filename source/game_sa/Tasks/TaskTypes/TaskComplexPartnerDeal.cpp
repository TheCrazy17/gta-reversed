#include "StdInc.h"

#include "TaskComplexPartnerDeal.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleRunAnim.h"

void CTaskComplexPartnerDeal::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerDeal, 0x870754, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x684190);
    RH_ScopedInstall(Destructor, 0x684200);

    RH_ScopedVMTInstall(Clone, 0x684D10);
    RH_ScopedVMTInstall(GetTaskType, 0x681DF0);
    // The remaining slots are the base class's functions, which CTaskComplexPartner::InjectHooks already hooks.
    // CreateFirstSubTask has its own address here (0x6823B0 is a jump to the base class's function).
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0, { .Overrides = false });
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6823B0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0, { .Overrides = false });
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x6823C0);
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370, { .Overrides = false });
    RH_ScopedVMTInstall(GetPartnerSequence, 0x682440);
}

// 0x684190
CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, true, 1, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_DEAL;
    strcpy_s(m_animBlockName, "gangs");
}

// 0x6823C0
void CTaskComplexPartnerDeal::StreamRequiredAnims() {
    CAnimManager::StreamAnimBlock(m_animBlockName, CTaskComplexGangLeader::ShouldLoadGangAnims(), m_requiredAnimsStreamedIn);
}

// 0x682440
CTaskComplexSequence* CTaskComplexPartnerDeal::GetPartnerSequence() {
    const auto seq = new CTaskComplexSequence();
    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord(m_partner, 0.5f, 0.02f));

    // The lead speaker hands over the goods, the other ped takes them
    seq->AddTask(new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, m_leadSpeaker ? ANIM_ID_DEALER_DEAL : ANIM_ID_DRUGS_BUY, 4.0f, false));
    return seq;
}
