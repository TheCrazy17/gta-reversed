#include "StdInc.h"

#include "TaskSimpleRunNamedAnim.h"

void CTaskSimpleRunNamedAnim::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleRunNamedAnim, 0x86D54C, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedOverloadedInstall(Constructor, "Default", 0x6674B0, CTaskSimpleRunNamedAnim*(CTaskSimpleRunNamedAnim::*)());
    RH_ScopedOverloadedInstall(Constructor, "Anim", 0x61A990, CTaskSimpleRunNamedAnim*(CTaskSimpleRunNamedAnim::*)(char const*, char const*, int32, float, int32, bool, bool, bool, bool));
    RH_ScopedInstall(Destructor, 0x61BF10);

    //RH_ScopedInstall(FinishRunAnimMovePedCB, 0x61AAA0, { .Reversed = false });
    //RH_ScopedInstall(StartAnim, 0x61BB10, { .Reversed = false });
    RH_ScopedVMTInstall(Clone, 0x61B770);
    RH_ScopedVMTInstall(GetTaskType, 0x61AA90);
    RH_ScopedVMTInstall(ProcessPed, 0x61BF20);
    RH_ScopedInstall(OffsetPedPosition, 0x61AB00);
}

// 0x6674B0
CTaskSimpleRunNamedAnim::CTaskSimpleRunNamedAnim() :
    CTaskSimpleAnim{ false }
{
    // Rest done in header
}

// 0x61A990
CTaskSimpleRunNamedAnim::CTaskSimpleRunNamedAnim(
    const char* animName,
    const char* animGroupName,
    uint32 animFlags,
    float blendDelta,
    uint32 endTime,
    bool bDontInterrupt,
    bool bRunInSequence,
    bool bOffsetPed,
    bool bHoldLastFrame
) :
    CTaskSimpleAnim(bHoldLastFrame),
    m_Time{ endTime },
    m_animFlags{ animFlags }
{
    m_bDontInterrupt = bDontInterrupt;
    m_bRunInSequence = bRunInSequence;
    m_bOffsetAtEnd = bOffsetPed;
    m_fBlendDelta = blendDelta;
    strcpy_s(m_animName, animName);
    strcpy_s(m_animGroupName, animGroupName);
    if (const auto block = CAnimManager::GetAnimationBlock(m_animGroupName)) {
        m_pAnimHierarchy = CAnimManager::GetAnimation(m_animName, block);
    }
}

// 0x61BB10 (not reversed yet)
void CTaskSimpleRunNamedAnim::StartAnim(CPed* ped) {
    plugin::CallMethod<0x61BB10, CTaskSimpleRunNamedAnim*, CPed*>(this, ped);
}

// 0x61BF20
bool CTaskSimpleRunNamedAnim::ProcessPed(CPed* ped) {
    if (m_bOffsetAvailable) {
        OffsetPedPosition(ped);
    }

    if (m_bIsFinished) {
        if (m_bOffsetAtEnd) {
            return true;
        }

        // Fade out whatever is left of the anim if the ped ended up far away from where it started
        auto* const anim = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), true, m_pAnimHierarchy);
        CVector rootPos{};
        ped->GetBonePosition(&rootPos, BONE_ROOT, false);
        if (m_fBlendDelta > 100.0f && anim && (ped->GetPosition() - rootPos).Magnitude() > 1.0f) {
            anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
            anim->m_BlendDelta = m_fBlendDelta * -1.0f;
        }
        return true;
    }

    if (!m_pAnimHierarchy || !m_pAnimHierarchy->m_pSequences) {
        return true;
    }

    if (m_bDontInterrupt && (!(m_animFlags & ANIMATION_IS_FINISH_AUTO_REMOVE) || (m_animFlags & ANIMATION_IS_LOOPED))) {
        ped->GetEventGroup().RemoveInvalidEvents(true);
        ped->GetEventGroup().Reorganise();
    }

    if (m_Timer.m_bStarted && m_Timer.IsOutOfTime()) {
        if (!(m_animFlags & ANIMATION_IS_FINISH_AUTO_REMOVE)) {
            if (m_pAnim) {
                m_pAnim->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
            }
            m_bIsFinished = true;
            m_pAnim       = nullptr;
            return true;
        }
        MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
    }

    if (!m_pAnim) {
        // Somebody else is already using the anim
        if (const auto anim = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), true, m_pAnimHierarchy); anim && anim->m_pCallbackData) {
            return true;
        }
        StartAnim(ped);
    }
    return m_bIsFinished;
}

// 0x61AB00
void CTaskSimpleRunNamedAnim::OffsetPedPosition(CPed* ped) {
    ped->UpdateRpHAnim();
    ped->m_bDontUpdateHierarchy = true;
    auto& pos = ped->GetPosition();
    pos += ped->m_matrix->TransformVector(m_vecOffsetAtEnd);
    m_bOffsetAvailable = false;
}
