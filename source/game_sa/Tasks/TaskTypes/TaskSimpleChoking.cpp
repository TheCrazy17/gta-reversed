#include "StdInc.h"

#include "TaskSimpleChoking.h"

// 0x6202C0
CTaskSimpleChoking::CTaskSimpleChoking(CPed* attacker, bool bIsTeargas) :
    m_pAttacker{attacker},
    m_bIsTeargas{bIsTeargas}
{
    CEntity::SafeRegisterRef(m_pAttacker);
    m_pAnim          = nullptr;
    m_bIsFinished    = false;
    m_nTimeRemaining = CGeneral::GetRandomNumberInRange(0u, 1000u);
    m_nTimeStarted   = CTimer::GetTimeInMS();
}

// 0x623220
CTaskSimpleChoking::CTaskSimpleChoking(const CTaskSimpleChoking& o) :
    CTaskSimpleChoking{o.m_pAttacker, o.m_bIsTeargas}
{
}

// 0x6203F0
bool CTaskSimpleChoking::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    switch (priority) {
    case ABORT_PRIORITY_URGENT:
    case ABORT_PRIORITY_IMMEDIATE: {
        if (event && event->GetEventPriority() < 57) {
            return false;
        }
        m_bIsFinished = true;
        break;
    }
    case ABORT_PRIORITY_LEISURE: {
        if (m_pAnim) {
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        }
        break;
    }
    default: {
        NOTSA_UNREACHABLE_CASE(priority);
    }
    }

    if (auto* const anim = std::exchange(m_pAnim, nullptr)) {
        anim->SetBlendDelta(-4.f);
        anim->SetDefaultFinishCallback();
    }

    return true;
}

// 0x620490
bool CTaskSimpleChoking::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        return true;
    }

    // Look towards the attacker if they are in front of us
    if (m_pAttacker && !ped->IsPlayer() && !m_bIsTeargas) {
        const auto toAttacker = m_pAttacker->GetPosition() - ped->GetPosition();
        if (toAttacker.Dot(ped->GetForward()) > 0.0f) {
            ped->m_fAimingRotation = std::atan2(-toAttacker.x, toAttacker.y);
        }
    }

    const auto elapsed = (uint32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);
    m_nTimeRemaining   = m_nTimeRemaining > elapsed ? m_nTimeRemaining - elapsed : 0;

    const auto PlayAnim = [&](AnimationId animId) {
        m_pAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, animId, 4.0f);
        m_pAnim->SetFinishCallback([](CAnimBlendAssociation*, void* data) {
            const auto self = static_cast<CTaskSimpleChoking*>(data);
            self->m_pAnim       = nullptr;
            self->m_bIsFinished = true;
        }, this);
    };

    if (!m_pAnim) {
        PlayAnim(ANIM_ID_GAS_CWR);
        m_pAnim->m_Speed = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 0.25f + 0.75f;
    } else if (m_nTimeRemaining == 0) {
        if (m_pAnim->m_AnimId == ANIM_ID_GAS_CWR) {
            // Done coughing, so get tired for a while
            m_pAnim->SetDefaultFinishCallback();
            PlayAnim(ANIM_ID_IDLE_TIRED);
            const auto sinceStart = CTimer::GetTimeInMS() - m_nTimeStarted;
            m_nTimeRemaining      = sinceStart < CGeneral::GetRandomNumberInRange(8000u, 12000u)
                ? sinceStart
                : CGeneral::GetRandomNumberInRange(8000u, 12000u);
        } else {
            m_pAnim->SetBlendDelta(-4.0f);
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        }
    }

    ped->Say(CTX_GLOBAL_PAIN_COUGH);
    return false;
}

// 0x620660
void CTaskSimpleChoking::UpdateChoke(CPed* victim, CPed* attacker, bool bIsTeargas) {
    m_bIsTeargas = bIsTeargas;
    if (m_pAttacker != attacker) {
        CEntity::ChangeEntityReference(m_pAttacker, attacker);
    }
    m_bIsFinished = false;
    if (m_pAnim && m_pAnim->m_AnimId != ANIM_ID_GAS_CWR) {
        m_pAnim->SetDefaultFinishCallback();
        m_pAnim = CAnimManager::BlendAnimation(victim->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_GAS_CWR, 4.f);
        m_pAnim->SetFinishCallback([](CAnimBlendAssociation* a, void* data) {
            const auto self = static_cast<CTaskSimpleChoking*>(data);
            self->m_bIsFinished = true;
            self->m_pAnim = nullptr;
        });
        m_pAnim->SetSpeed(CGeneral::GetRandomNumberInRange(0.8f, 1.1f));
        m_nTimeStarted = CTimer::GetTimeInMS() - m_nTimeRemaining;
    }
    m_nTimeRemaining = CGeneral::GetRandomNumberInRange<uint32>(1'000, 2'000);
}

void CTaskSimpleChoking::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleChoking, 0x86d820, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6202C0);
    RH_ScopedInstall(Destructor, 0x620370);

    RH_ScopedInstall(UpdateChoke, 0x620660);

    RH_ScopedVMTInstall(Clone, 0x623220);
    RH_ScopedVMTInstall(GetTaskType, 0x620360);
    RH_ScopedVMTInstall(MakeAbortable, 0x6203F0);
    RH_ScopedVMTInstall(ProcessPed, 0x620490);
}
