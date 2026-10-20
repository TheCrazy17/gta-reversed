#include "StdInc.h"
#include "TaskInteriorSitAtDesk.h"
#include "Interior/InteriorManager_c.h"
#include "Interior/InteriorInfo_t.h"

void CTaskInteriorSitAtDesk::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorSitAtDesk, 0x87035c, 9);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x676010);
    RH_ScopedInstall(Destructor, 0x676080);

    RH_ScopedInstall(FinishAnimCB, 0x676190);
    RH_ScopedInstall(StartRandomLoopAnim, 0x677780);
    RH_ScopedInstall(StartRandomOneOffAnim, 0x677880);
    RH_ScopedVMTInstall(Clone, 0x6760E0);
    RH_ScopedVMTInstall(GetTaskType, 0x676070);
    RH_ScopedVMTInstall(MakeAbortable, 0x676150);
    RH_ScopedVMTInstall(ProcessPed, 0x677920);
}

// 0x676010
CTaskInteriorSitAtDesk::CTaskInteriorSitAtDesk(int32 duration, InteriorInfo_t* interiorInfo, bool bDoInstantly) :
    m_bDoInstantly{bDoInstantly},
    m_InteriorInfo{interiorInfo},
    m_Duration{duration}
{
}

// 0x6760E0
CTaskInteriorSitAtDesk::CTaskInteriorSitAtDesk(const CTaskInteriorSitAtDesk& o) :
    CTaskInteriorSitAtDesk{o.m_Duration, o.m_InteriorInfo, o.m_bDoInstantly}
{
}

// 0x676080
CTaskInteriorSitAtDesk::~CTaskInteriorSitAtDesk() {
    if (m_Anim) {
        m_Anim->SetDefaultFinishCallback();
    }
}

// 0x676190
void CTaskInteriorSitAtDesk::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorSitAtDesk>(static_cast<CTask*>(data));

    assert(self && anim);
    assert(anim == self->m_Anim);

    self->m_PrevAnimId = (AnimationId)anim->m_AnimId;
    if (anim->m_AnimId == ANIM_ID_OFF_SIT_2IDLE_180 || self->m_bTaskAborting && self->m_PrevAnimId == ANIM_ID_OFF_SIT_IN) {
        anim->SetBlendDelta(-1000.f);
        self->m_bTaskFinished = true;
    }

    self->m_Anim = nullptr;
}

// 0x677780
void CTaskInteriorSitAtDesk::StartRandomLoopAnim(CPed* ped, float blendDelta) {
    using CGeneral::GetRandomNumberInRange;

    const auto chance = GetRandomNumberInRange(100u);
    StartAnim(
        ped,
        chance > 40
            ? ANIM_ID_OFF_SIT_TYPE_LOOP
            : chance > 10
                ? ANIM_ID_OFF_SIT_BORED_LOOP
                : ANIM_ID_OFF_SIT_IDLE_LOOP,
        blendDelta
    );
    m_AnimTimer.Start(chance > 40 ? GetRandomNumberInRange(2000, 5000) : GetRandomNumberInRange(7000, 12000));
}

// 0x677880
void CTaskInteriorSitAtDesk::StartRandomOneOffAnim(CPed* ped) {
    const auto chance = CGeneral::GetRandomNumberInRange(100u);
    StartAnim(
        ped,
        chance > 60
            ? ANIM_ID_OFF_SIT_READ
            : chance > 35
                ? ANIM_ID_OFF_SIT_DRINK
                : chance > 10
                    ? ANIM_ID_OFF_SIT_WATCH
                    : ANIM_ID_OFF_SIT_CRASH,
        4.f
    );
}

// 0x676150
bool CTaskInteriorSitAtDesk::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        if (m_Anim) {
            m_Anim->SetBlendDelta(-1000.f);
            m_Anim->SetDefaultFinishCallback();
            m_Anim = nullptr;
        }
        return true;
    }
    m_bTaskAborting = true;
    return false;
}

// 0x677920
bool CTaskInteriorSitAtDesk::ProcessPed(CPed* ped) {
    static_assert(offsetof(CPed, m_vecAnimMovingShiftLocal) == 0x4D8);
    static_assert(offsetof(CPed, m_fCurrentRotation) == 0x558 && offsetof(CPed, m_fAimingRotation) == 0x55C);
    // The offset the ped is moved by when it gets up or sits down, filled in by the game
    static auto& s_SitOffset = StaticRef<CVector>(0xC18C6C);

    const auto animId = m_Anim ? (int32)m_Anim->m_AnimId : -1;
    const auto StartTurnAround = [&] {
        if (m_Anim) {
            m_Anim->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        }
        m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_2IDLE_180, 1000.0f);
        m_Anim->SetFinishCallback(FinishAnimCB, this);
        m_bUpdatePedPos = true;
    };

    ped->SetMoveState(PEDMOVE_STILL);

    // Done, turn the ped around once the getting up animation is over
    if (m_bTaskFinished && !RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_OFF_SIT_2IDLE_180)) {
        const auto heading = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + PI);
        ped->m_fAimingRotation  = heading;
        ped->m_fCurrentRotation = heading;
        if (ped->m_matrix) {
            ped->m_matrix->SetRotateZOnly(heading);
        } else {
            ped->m_placement.m_fHeading = heading;
        }
        return true;
    }

    if (m_bTaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) {
            return true;
        }
        if (animId == ANIM_ID_OFF_SIT_IN) {
            m_Anim->m_BlendDelta = -8.0f;
        } else if (animId >= ANIM_ID_OFF_SIT_IDLE_LOOP && animId <= ANIM_ID_OFF_SIT_WATCH) {
            if (!m_bUpdatePedPos) {
                StartTurnAround();
                return false;
            }
        } else if (animId == ANIM_ID_OFF_SIT_2IDLE_180) {
            m_Anim->m_Speed = 2.0f;
        }
    }

    if (!m_Anim) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) {
            return false;
        }

        const auto StartTaskTimer = [&] {
            if (m_Duration >= 0) {
                m_TaskTimer.Start(m_Duration);
            }
        };
        if (m_PrevAnimId == ANIM_ID_UNDEFINED) {
            if (!m_bDoInstantly) {
                // Start by sitting down
                m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_IN, 4.0f);
                m_Anim->SetFinishCallback(FinishAnimCB, this);
                return false;
            }
            StartTaskTimer();
            StartRandomLoopAnim(ped, 1000.0f);
            m_bUpdatePedPos = true;
        } else if (m_PrevAnimId == ANIM_ID_OFF_SIT_IN) {
            StartTaskTimer();
            StartRandomLoopAnim(ped, 1000.0f);
            m_bUpdatePedPos = true;
        } else if (m_PrevAnimId >= ANIM_ID_OFF_SIT_CRASH && m_PrevAnimId <= ANIM_ID_OFF_SIT_WATCH) {
            // A one off animation just ended
            StartRandomLoopAnim(ped, 4.0f);
        }
        return false;
    }

    // Move the ped to where the animation leaves them
    if (m_bUpdatePedPos) {
        auto pos = ped->GetPosition();
        if (animId >= ANIM_ID_OFF_SIT_IDLE_LOOP && animId <= ANIM_ID_OFF_SIT_BORED_LOOP) {
            pos = *ped->m_matrix * s_SitOffset;
            pos.z = ped->GetPosition().z;
        } else if (animId == ANIM_ID_OFF_SIT_2IDLE_180) {
            pos = *ped->m_matrix * -s_SitOffset;
            pos.z = ped->GetPosition().z;
        }
        ped->SetPosn(pos);
        m_bUpdatePedPos = false;
    }

    if (m_TaskTimer.m_bStarted && m_TaskTimer.IsOutOfTime()) {
        // Time to leave, getting up is the animation that ends the task
        if (m_Anim->m_AnimId != ANIM_ID_OFF_SIT_2IDLE_180) {
            StartTurnAround();
        }
    } else if (m_AnimTimer.m_bStarted && m_AnimTimer.IsOutOfTime()) {
        m_AnimTimer.m_bStarted = false;
        if (CGeneral::GetRandomNumber() < 0x3FFF) {
            StartRandomOneOffAnim(ped);
        } else {
            StartRandomLoopAnim(ped, 4.0f);
        }
    }

    // While sitting down the ped slides towards the chair and turns to face the right way
    if (m_Anim->m_AnimId == ANIM_ID_OFF_SIT_IN) {
        const auto& target  = m_InteriorInfo->Pos;
        const auto  diff    = target - ped->GetPosition();
        const auto  dist    = diff.Magnitude();
        const auto  moveBy  = std::min(dist, 0.02f);
        const auto  shift   = diff * (1.0f / dist) * moveBy;
        const auto& matrix  = *ped->m_matrix;
        ped->m_vecAnimMovingShiftLocal.x = DotProduct(matrix.GetRight(), shift);
        ped->m_vecAnimMovingShiftLocal.y = DotProduct(matrix.GetForward(), shift);
        ped->m_fAimingRotation      = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(m_InteriorInfo->Dir.x, m_InteriorInfo->Dir.y, 0.0f, 0.0f));
    }
    return false;
}

void CTaskInteriorSitAtDesk::StartAnim(CPed* ped, AnimationId animId, float blendDelta) {
    if (m_Anim) {
        m_Anim->SetDefaultFinishCallback();
    }
    m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, animId, blendDelta);
    m_Anim->SetFinishCallback(FinishAnimCB, this);
}
