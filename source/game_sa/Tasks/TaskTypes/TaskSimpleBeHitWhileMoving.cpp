#include "StdInc.h"

#include "TaskSimpleBeHitWhileMoving.h"

#include "Animation/AnimBlendHierarchy.h"

void CTaskSimpleBeHitWhileMoving::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleBeHitWhileMoving, 0x86D7FC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61FE30);
    RH_ScopedInstall(Destructor, 0x61FE70);
    RH_ScopedInstall(StartAnim, 0x61FF40);
    RH_ScopedInstall(FinishAnimCB, 0x61FF30);

    RH_ScopedVMTInstall(Clone, 0x623190);
    RH_ScopedVMTInstall(GetTaskType, 0x61FE60);
    RH_ScopedVMTInstall(MakeAbortable, 0x61FEE0);
    RH_ScopedVMTInstall(ProcessPed, 0x620290);
}

// 0x61FE30
CTaskSimpleBeHitWhileMoving::CTaskSimpleBeHitWhileMoving(int32 hitType, int32 direction) :
    CTaskSimple{},
    m_bIsFinished{false},
    m_HitType{hitType},
    m_Direction{direction},
    m_Anim{nullptr}
{
}

// 0x61FE70
CTaskSimpleBeHitWhileMoving::~CTaskSimpleBeHitWhileMoving() {
    if (m_Anim) {
        m_Anim->m_BlendDelta = -4.f;
        m_Anim->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr); // The task is gone, so the callback must not touch it
    }
}

// 0x61FEE0
bool CTaskSimpleBeHitWhileMoving::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
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

// 0x620290
bool CTaskSimpleBeHitWhileMoving::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        return true;
    }

    if (!m_Anim) {
        StartAnim(ped);
    }

    return false;
}

// 0x61FF40
void CTaskSimpleBeHitWhileMoving::StartAnim(CPed* ped) {
    const auto clump = ped->GetRpClump();

    // Short reaction. A still playing animation of that id is restarted, one that is fading out is replaced
    const auto PlayReaction = [&](AnimationId animId) {
        m_Anim = RpAnimBlendClumpGetAssociation(clump, animId);
        if (!m_Anim || m_Anim->m_BlendDelta < 0.f) {
            m_Anim = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, animId, 8.f);
        }
        m_Anim->SetCurrentTime(0.f);
        m_Anim->SetFlag(ANIMATION_IS_PLAYING);
        m_Anim->SetFlag(ANIMATION_IS_FINISH_AUTO_REMOVE);
    };

    // Knock down animation, which also decides for how long the ped stays unconscious.
    // The exe has more branches here for `ANIM_ID_NO_ANIMATION_SET` (and a timer variant for the two car roll out
    // animations of the player), but every caller passes a constant id, so they can't be reached.
    const auto PlayKnockDown = [&](AnimationId animId) {
        if (ped->m_pAttachedTo) { // Nothing is started
            return;
        }

        ped->ClearLookFlag();
        ped->ClearAimFlag();

        const auto existing = RpAnimBlendClumpGetAssociation(clump, animId);
        if (existing) {
            existing->SetCurrentTime(0.f);
            existing->SetFlag(ANIMATION_IS_PLAYING);
            existing->m_BlendAmount = 0.f;
            existing->m_BlendDelta  = 8.f;
            if (notsa::IsFixBugs()) { // The exe leaves `m_Anim` unset here, and crashes setting the finish callback below
                m_Anim = existing;
            }
        } else {
            m_Anim = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, animId, 8.f);
        }

        const auto timeMS        = CTimer::GetTimeInMS();
        const auto seedAndFrame  = (uint32)ped->m_nRandomSeed + CTimer::GetFrameCounter();
        const auto spreadMS      = seedAndFrame % 1000u; // Stops peds hit in the same frame from getting up at the same time

        // Based on the animation that was already playing (the freshly blended one is not used)
        ped->m_nUnconsciousTimer = [&]() -> uint32 {
            if (!existing) {
                return spreadMS + timeMS + 1000u;
            }
            const auto animTime = existing->m_BlendHier->m_fTotalTime;
            return ped->IsPlayer()
                ? (uint32)(animTime * 1000.f + (float)timeMS + 500.f)
                : (uint32)((float)spreadMS + animTime * 1000.f + (float)timeMS + 500.f);
        }();
    };

    const auto StartReaction = [&] {
        const auto fullReaction = (m_HitType == 4 || m_HitType == 2)
            && ((!ped->IsPlayer() && (CGeneral::GetRandomNumber() & 1)) || (CGeneral::GetRandomNumber() & 7) == 0);

        if (!fullReaction) {
            if (!ped->IsPlayer() && !(CGeneral::GetRandomNumber() & 1)) {
                CGeneral::GetRandomNumberInRange(1000, 3000); // The result is unused in the exe
                return;
            }
            PlayReaction([&] {
                switch (m_Direction) {
                case 1:  return ANIM_ID_SHOT_LEFTP;
                case 2:  return ANIM_ID_SHOT_PARTIAL_B;
                case 3:  return ANIM_ID_SHOT_RIGHTP;
                default: return ANIM_ID_SHOT_PARTIAL;
                }
            }());
            return;
        }

        switch (m_Direction) {
        case 1:
            PlayKnockDown(ANIM_ID_KO_SPIN_R);
            return;
        case 2:
            if (CGeneral::GetRandomNumber() & 1) {
                PlayReaction(ANIM_ID_HIT_BACK);
            } else {
                PlayKnockDown(ANIM_ID_KO_SKID_BACK);
            }
            return;
        case 3:
            PlayKnockDown(ANIM_ID_KO_SPIN_L);
            return;
        default:
            if (m_HitType == 2) {
                PlayKnockDown(ANIM_ID_KO_SHOT_STOM);
            } else if (CGeneral::GetRandomNumber() & 1) {
                PlayReaction(ANIM_ID_HIT_WALK);
            } else if (CGeneral::GetRandomNumber() & 1) {
                PlayReaction(ANIM_ID_HIT_FRONT);
            } else {
                PlayKnockDown(ANIM_ID_KO_SHOT_FACE);
            }
            return;
        }
    };
    StartReaction();

    // The exe also gets here when no animation was started (attached ped, or the random "do nothing" case)
    // and then crashes on the null animation
    if (!notsa::IsFixBugs() || m_Anim) {
        m_Anim->SetFinishCallback(FinishAnimCB, this);
    }
}

// 0x61FF30
void CTaskSimpleBeHitWhileMoving::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = static_cast<CTaskSimpleBeHitWhileMoving*>(data);
    self->m_Anim        = nullptr;
    self->m_bIsFinished = true;
}
