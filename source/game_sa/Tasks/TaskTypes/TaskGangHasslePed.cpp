#include "SeekEntity/TaskComplexSeekEntityStandard.h"
#include "StdInc.h"

#include "TaskGangHasslePed.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskSimpleRunAnim.h"

void CTaskGangHasslePed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHasslePed, 0x86FA00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FED0);
    RH_ScopedInstall(Destructor, 0x65FF60);

    RH_ScopedVMTInstall(CreateNextSubTask, 0x6642C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x664380);
    RH_ScopedVMTInstall(ControlSubTask, 0x65FFE0);
}

// 0x65FED0
CTaskGangHasslePed::CTaskGangHasslePed(CPed* ped, int32 a3, int32 a4, int32 a5) : CTaskComplex() {
    m_nTime = 0;
    m_nSomeRandomShit = 0;
    m_bFirstSubTaskInitialised = 0;
    m_bRefreshTime = 0;
    dword10 = a3;
    m_RndMin = a4;
    m_Ped = ped;
    m_RndMax = a5;
    m_bAnimBlockRefAdded = false;
    CEntity::SafeRegisterRef(m_Ped);
}

// 0x65FF60
CTaskGangHasslePed::~CTaskGangHasslePed() {
    CEntity::SafeCleanUpRef(m_Ped);

    if (m_bAnimBlockRefAdded) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bAnimBlockRefAdded = false;
    }
}

namespace {
auto& s_HassleMaxEntityDistMult = StaticRef<float>(0x86FC2C);
auto& s_HassleMoveStateRadius   = StaticRef<float>(0x86FC28);
auto& s_HassleFollowNodeThresholdHeightChange = StaticRef<float>(0x86FC30);

// NOTSA: same list as CTaskComplexGangLeader's file-local s_gangTalkAnims (that one has internal linkage)
constexpr AnimationId s_gangTalkAnims[]{
    ANIM_ID_PRTIAL_GNGTLKA, ANIM_ID_PRTIAL_GNGTLKB, ANIM_ID_PRTIAL_GNGTLKC, ANIM_ID_PRTIAL_GNGTLKD,
    ANIM_ID_PRTIAL_GNGTLKE, ANIM_ID_PRTIAL_GNGTLKF, ANIM_ID_PRTIAL_GNGTLKG, ANIM_ID_PRTIAL_GNGTLKH,
};
}; // namespace

// 0x6642C0
CTask* CTaskGangHasslePed::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_KILL_PED_ON_FOOT) {
        return nullptr;
    }

    const auto radius = CGeneral::GetRandomNumberInRange(3.0f, 5.0f); // strange math here
    return new CTaskComplexSeekEntityStandard(m_Ped, 999'999, 1000, s_HassleMaxEntityDistMult * radius, s_HassleMoveStateRadius, s_HassleFollowNodeThresholdHeightChange, false, true);
}

// 0x664380
CTask* CTaskGangHasslePed::CreateFirstSubTask(CPed* ped) {
    if (!m_Ped) {
        return nullptr;
    }

    const auto radius = CGeneral::GetRandomNumberInRange(3.0f, 5.0f); // strange math here
    m_nTime = CTimer::GetTimeInMS();
    m_nSomeRandomShit = CGeneral::GetRandomNumberInRange(m_RndMin, m_RndMax);
    m_bFirstSubTaskInitialised = true;

    return new CTaskComplexSeekEntityStandard(m_Ped, 999'999, 1000, s_HassleMaxEntityDistMult * radius, s_HassleMoveStateRadius, s_HassleFollowNodeThresholdHeightChange, false, true);
}

// 0x65FFE0
CTask* CTaskGangHasslePed::ControlSubTask(CPed* ped) {
    if (!m_Ped) {
        return nullptr;
    }

    if (m_bFirstSubTaskInitialised) {
        if (m_bRefreshTime) {
            m_nTime = CTimer::GetTimeInMS();
            m_bRefreshTime = false;
        }

        if (m_nSomeRandomShit + m_nTime <= (int32)CTimer::m_snTimeInMilliseconds) {
            if (dword10 == 2) {
                if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) {
                    return new CTaskComplexKillPedOnFoot(m_Ped);
                }
            } else if (dword10 != 1) {
                return nullptr;
            }
        }
    }

    // Keep the "gangs" (talk/hand-signal) animation block referenced only while it's actually needed
    if (!m_bAnimBlockRefAdded) {
        if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
            if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
                CAnimManager::AddAnimBlockRef(blk);
                m_bAnimBlockRefAdded = true;
            } else {
                CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
            }
        }
    } else if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bAnimBlockRefAdded = false;
    }

    // Occasionally kick off a "partial anim" secondary task (talk anim or hand signal)
    if (m_bAnimBlockRefAdded && !ped->IsPlayingHandSignal() && !ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        const auto rnd = CGeneral::GetRandomNumberInRange(0, 200);
        if (51 <= rnd && rnd <= 55) { // 2.5% chance
            ped->GetTaskManager().SetTaskSecondary(
                new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, CGeneral::RandomChoice(s_gangTalkAnims), 4.f, false),
                TASK_SECONDARY_PARTIAL_ANIM
            );
        } else if (rnd == 100) { // 0.5% chance
            ped->GetTaskManager().SetTaskSecondary(new CTaskComplexPlayHandSignalAnim(), TASK_SECONDARY_PARTIAL_ANIM);
        }
    }

    if (dword10 == 0) {
        ped->Say(CTX_GLOBAL_EYEING_PED, 0, 1.f, false, false, false);
    } else if (dword10 == 1 || dword10 == 2) {
        CTaskComplexGangLeader::DoGangAbuseSpeech(ped, m_Ped);
    }

    return m_pSubTask;
}
