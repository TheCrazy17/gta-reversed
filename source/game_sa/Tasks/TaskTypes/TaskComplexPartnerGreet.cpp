#include "StdInc.h"

#include "TaskComplexPartnerGreet.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleDoHandSignal.h"
#include "TaskSimpleChat.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexPartnerGreet::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerGreet, 0x87078c, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x684210);
    RH_ScopedInstall(Destructor, 0x684280);

    RH_ScopedVMTInstall(Clone, 0x684DA0);
    RH_ScopedVMTInstall(GetTaskType, 0x681E00);
    // The remaining slots are the base class's functions, which CTaskComplexPartner::InjectHooks already hooks.
    // CreateFirstSubTask has its own address here (0x6825A0 is a jump to the base class's function).
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0, { .Overrides = false });
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6825A0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0, { .Overrides = false });
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x6825B0);
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370, { .Overrides = false });
    RH_ScopedVMTInstall(GetPartnerSequence, 0x682630);
}

// 0x684210
CTaskComplexPartnerGreet::CTaskComplexPartnerGreet(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int32 handShakeType, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, true, 1, point)
{
    m_handShakeType = handShakeType;
    m_taskId = TASK_COMPLEX_PARTNER_GREET;
    strcpy_s(m_animBlockName, "gangs");
}

// For `0x684DA0` (Clone)
CTaskComplexPartnerGreet::CTaskComplexPartnerGreet(const CTaskComplexPartnerGreet& o) :
    CTaskComplexPartnerGreet{
        o.m_commandName,
        o.m_partner,
        o.m_leadSpeaker,
        o.m_distanceMultiplier,
        o.m_handShakeType,
        o.m_point
    }
{
}

// 0x6825B0
void CTaskComplexPartnerGreet::StreamRequiredAnims() {
    CAnimManager::StreamAnimBlock(m_animBlockName, CTaskComplexGangLeader::ShouldLoadGangAnims(), m_requiredAnimsStreamedIn);
}

// 0x682630
CTaskComplexSequence* CTaskComplexPartnerGreet::GetPartnerSequence() {
    const auto seq = new CTaskComplexSequence();
    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord(m_partner, 0.5f, 0.001f));

    // The handshake itself; an unknown type adds nothing
    const auto AddHandShake = [&](AnimationId animId) {
        seq->AddTask(new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, animId, 4.0f, false));
    };
    switch (m_handShakeType) {
    case 0: AddHandShake(ANIM_ID_HNDSHKAA); break;
    case 1: AddHandShake(ANIM_ID_HNDSHKBA); break;
    case 2: AddHandShake(m_leadSpeaker ? ANIM_ID_HNDSHKCA : ANIM_ID_HNDSHKCB); break;
    case 3: AddHandShake(ANIM_ID_HNDSHKDA); break;
    case 4: AddHandShake(ANIM_ID_HNDSHKEA); break;
    case 5: AddHandShake(ANIM_ID_HNDSHKFA); break;
    default: break;
    }

    // Durations of the small talk that follows are drawn up front, in this order, then a coin flip decides about a second round
    const auto time1     = CGeneral::GetRandomNumberInRange(3000, 7000);
    const auto time2     = CGeneral::GetRandomNumberInRange(3000, 7000);
    const auto time3     = CGeneral::GetRandomNumberInRange(3000, 7000);
    const auto time4     = CGeneral::GetRandomNumberInRange(3000, 7000);
    const auto extraTalk = CGeneral::GetRandomNumber() < RAND_MAX / 2;

    // The lead speaker does its hand signal right away, the other ped after a second; then they take turns chatting and standing still
    if (m_leadSpeaker) {
        seq->AddTask(new CTaskSimpleDoHandSignal());
        seq->AddTask(new CTaskSimpleStandStill(1000, false, false, 8.0f));
        seq->AddTask(new CTaskSimpleStandStill(time1, false, false, 8.0f));
        seq->AddTask(new CTaskSimpleChat(time2));
    } else {
        seq->AddTask(new CTaskSimpleStandStill(1000, false, false, 8.0f));
        seq->AddTask(new CTaskSimpleDoHandSignal());
        seq->AddTask(new CTaskSimpleChat(time1));
        seq->AddTask(new CTaskSimpleStandStill(time2, false, false, 8.0f));
    }

    if (extraTalk) {
        seq->AddTask(new CTaskSimpleStandStill(time3, false, false, 8.0f));
        seq->AddTask(new CTaskSimpleChat(time4));
    }
    return seq;
}
