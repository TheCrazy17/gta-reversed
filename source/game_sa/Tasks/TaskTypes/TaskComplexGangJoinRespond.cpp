#include "StdInc.h"

#include "TaskComplexGangJoinRespond.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleRunAnim.h"
#include "TaskComplexGangLeader.h"

void CTaskComplexGangJoinRespond::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGangJoinRespond, 0x86FB5C, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6616F0);
    RH_ScopedInstall(Destructor, 0x661720);

    RH_ScopedVMTInstall(Clone, 0x662290);
    RH_ScopedVMTInstall(MakeAbortable, 0x661790);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6617A0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6618D0);
    RH_ScopedVMTInstall(ControlSubTask, 0x661950);
}

// 0x6616F0
CTaskComplexGangJoinRespond::CTaskComplexGangJoinRespond(bool response) :
    m_response{response}
{
}

// 0x661720
CTaskComplexGangJoinRespond::~CTaskComplexGangJoinRespond() {
    if (m_animsReferenced) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_animsReferenced = false;
    }
}

// 0x6617A0
CTask* CTaskComplexGangJoinRespond::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_ANIM) {
        return nullptr;
    }
    if (!m_animsReferenced) {
        if (++m_attempts < 11) {
            return new CTaskSimpleStandStill(250, false, false, 8.0f);
        }
        return nullptr;
    }
    return m_response
        ? new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, ANIM_ID_INVITE_YES, 4.0f, false)
        : new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, ANIM_ID_INVITE_NO, 4.0f, false);
}

// 0x6618D0
CTask* CTaskComplexGangJoinRespond::CreateFirstSubTask(CPed* ped) {
    m_attempts = 0;
    return new CTaskComplexTurnToFaceEntityOrCoord(FindPlayerPed(0));
}

// 0x661950
CTask* CTaskComplexGangJoinRespond::ControlSubTask(CPed* ped) {
    // NOTSA: `ped+0x55C` is an unmapped CPed field (also seen written the same way in
    // CTaskSimpleChoking::ProcessPed) - a plain float overwrite, NOT going through
    // CPed::SetLookFlag (which has extra side effects the original disasm doesn't show here).
    *reinterpret_cast<float*>(reinterpret_cast<char*>(ped) + 0x55c) =
        CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(
            FindPlayerPed(-1)->GetPosition().x - ped->GetPosition().x,
            FindPlayerPed(-1)->GetPosition().y - ped->GetPosition().y,
            0.f, 0.f
        ));

    if (!m_animsReferenced) {
        if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
            if (!CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
                CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
                return m_pSubTask;
            }
            CAnimManager::AddAnimBlockRef(blk);
            m_animsReferenced = true;
        }
    } else if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_animsReferenced = false;
    }
    return m_pSubTask;
}
