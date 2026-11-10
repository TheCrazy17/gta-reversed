#include "StdInc.h"

#include "TaskSimpleBeKickedOnGround.h"

void CTaskSimpleBeKickedOnGround::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleBeKickedOnGround, 0x86D7D8, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61FC20);
    RH_ScopedInstall(Destructor, 0x61FC50);
    RH_ScopedInstall(StartAnim, 0x61FD20);
    RH_ScopedInstall(FinishAnimCB, 0x61FD10);

    RH_ScopedVMTInstall(Clone, 0x623120);
    RH_ScopedVMTInstall(GetTaskType, 0x61FC40);
    RH_ScopedVMTInstall(MakeAbortable, 0x61FCC0);
    RH_ScopedVMTInstall(ProcessPed, 0x61FE00);
}

// 0x61FC20
CTaskSimpleBeKickedOnGround::CTaskSimpleBeKickedOnGround() :
    CTaskSimple{},
    m_bIsFinished{false},
    m_Anim{nullptr}
{
}

// 0x61FC50
CTaskSimpleBeKickedOnGround::~CTaskSimpleBeKickedOnGround() {
    if (m_Anim) {
        m_Anim->m_BlendDelta = -4.f;
        m_Anim->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr); // The task is gone, so the callback must not touch it
    }
}

// 0x61FCC0
bool CTaskSimpleBeKickedOnGround::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    switch (priority) {
    case ABORT_PRIORITY_URGENT:
    case ABORT_PRIORITY_IMMEDIATE:
        if (m_Anim) {
            m_Anim->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
            m_Anim = nullptr;
        }
        return true;
    default: // Leisure; the animation fades out, but the task is not abortable yet
        if (m_Anim) {
            m_Anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
            m_Anim->m_BlendDelta = -4.f;
        }
        return false;
    }
}

// 0x61FE00
bool CTaskSimpleBeKickedOnGround::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        return true;
    }

    if (!m_Anim) {
        StartAnim(ped);
    }

    return false;
}

// 0x61FD20
void CTaskSimpleBeKickedOnGround::StartAnim(CPed* ped) {
    if (!CLocalisation::KickingWhenDown()) {
        m_bIsFinished = true;
        return;
    }

    // The exe calls `KickingWhenDown` a second time after the animation setup, to transform the origin by the
    // torso bone matrix. The result is never read, so that dead code is left out.

    // Lying on the front (an animation with the front flag is playing) selects the front variant
    const auto animId = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_IS_FRONT)
        ? ANIM_ID_FLOOR_HIT_F
        : ANIM_ID_FLOOR_HIT;

    m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, animId, 8.f);
    m_Anim->SetCurrentTime(0.f);
    m_Anim->SetFlag(ANIMATION_IS_PLAYING);
    m_Anim->SetFlag(ANIMATION_IS_FINISH_AUTO_REMOVE, false); // Stays on the last frame until the next task takes over
    m_Anim->SetFinishCallback(FinishAnimCB, this);
}

// 0x61FD10
void CTaskSimpleBeKickedOnGround::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = static_cast<CTaskSimpleBeKickedOnGround*>(data);
    self->m_Anim        = nullptr;
    self->m_bIsFinished = true;
}
