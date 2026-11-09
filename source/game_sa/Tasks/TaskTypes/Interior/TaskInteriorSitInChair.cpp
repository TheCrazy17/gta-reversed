#include "StdInc.h"
#include "TaskInteriorSitInChair.h"

#include "Interior/InteriorInfo_t.h"
#include "Interior/InteriorGroup_c.h"
#include "Interior/Interior_c.h"
#include "Interior/InteriorManager_c.h"
#include "Ragdoll/IKChainManager.h"

void CTaskInteriorSitInChair::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorSitInChair, 0x870314, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x675C30);
    RH_ScopedInstall(Destructor, 0x675C90);

    RH_ScopedInstall(FinishAnimCB, 0x675DD0);

    RH_ScopedVMTInstall(Clone, 0x675CF0);
    RH_ScopedVMTInstall(GetTaskType, 0x675C80);
    RH_ScopedVMTInstall(MakeAbortable, 0x675D60);
    RH_ScopedVMTInstall(ProcessPed, 0x676D30);
}

// 0x675C30
CTaskInteriorSitInChair::CTaskInteriorSitInChair(int32 duration, InteriorInfo_t* intInfo, bool doInstantly) :
    m_SitDuration{duration},
    m_IntInfo{intInfo},
    m_bDoInstantly{doInstantly}
{
}

// For `0x675CF0`
CTaskInteriorSitInChair::CTaskInteriorSitInChair(const CTaskInteriorSitInChair& o) :
    CTaskInteriorSitInChair{o.m_SitDuration, o.m_IntInfo, o.m_bDoInstantly}
{
}

// 0x675C90
CTaskInteriorSitInChair::~CTaskInteriorSitInChair() {
    if (m_Anim) {
        m_Anim->SetDefaultDeleteCallback();
    }
}

// 0x675DD0
void CTaskInteriorSitInChair::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorSitInChair>(static_cast<CTask*>(data));

    self->m_PrevAnimId = anim->GetAnimId();

    if (self->m_PrevAnimId == ANIM_ID_LOU_OUT) {
        anim->SetBlendDelta(-1000.f);
        self->m_LastAnimFinished = true;
    }

    if (self->m_TaskAborting && self->m_PrevAnimId == ANIM_ID_LOU_IN) {
        anim->SetBlendDelta(-1000.f);
        self->m_LastAnimFinished = true;
    }

    self->m_Anim = nullptr;
}

// 0x675D60
bool CTaskInteriorSitInChair::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250u);
        }
        if (m_Anim) {
            m_Anim->SetBlendDelta(-1000.f);
            m_Anim->SetDefaultDeleteCallback();
            m_Anim = nullptr;
        }
        return true;
    }
    m_TaskAborting = true;
    return false;
}

// 0x676D30
bool CTaskInteriorSitInChair::ProcessPed(CPed* ped) {
    const auto currAnimId = m_Anim
        ? (AnimationId)m_Anim->GetAnimId()
        : ANIM_ID_UNDEFINED;

    ped->SetMoveState(PEDMOVE_STILL);

    if (m_LastAnimFinished) {
        if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_LOU_OUT)) { // Check if the get up animation has really finished
            return true;
        }
    }

    const auto StartAnim = [&, this](AnimationId animId, float blendDelta) {
        m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_HOUSE, animId, blendDelta);
        m_Anim->SetFinishCallback(FinishAnimCB, this);
    };

    if (m_TaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(ANIM_GROUP_DEFAULT)) {
            return true;
        }

        if (currAnimId == ANIM_ID_LOU_IN) {
            m_Anim->SetBlendDelta(-8.f);
        } else if (currAnimId == ANIM_ID_LOU_LOOP) {
            if (!m_UpdatePedPos) {
                m_Anim->SetDefaultDeleteCallback();
                StartAnim(ANIM_ID_LOU_OUT, 1000.f);
                m_UpdatePedPos = true;
                return false;
            }
        } else if (currAnimId == ANIM_ID_LOU_OUT) {
            m_Anim->m_Speed = 3.f;
        }
    }

    if (!m_Anim) {
        if (InteriorManager_c::AreAnimsLoaded(ANIM_GROUP_DEFAULT)) {
            if (m_PrevAnimId == ANIM_ID_UNDEFINED) { // Nothing played yet
                if (m_bDoInstantly) {
                    m_SitTimer.Start(m_SitDuration);
                    StartAnim(ANIM_ID_LOU_LOOP, 1000.f);
                    m_UpdatePedPos = true;
                } else {
                    StartAnim(ANIM_ID_LOU_IN, 4.f);
                }
            } else if (m_PrevAnimId == ANIM_ID_LOU_IN) {
                m_SitTimer.Start(m_SitDuration);
                StartAnim(ANIM_ID_LOU_LOOP, 1000.f);
                m_UpdatePedPos = true;
            }
        }
    } else {
        if (m_UpdatePedPos) {
            auto       pos    = ped->GetPosition();
            const auto savedZ = pos.z;
            if (currAnimId == ANIM_ID_LOU_LOOP) {
                const auto chairPosWS = ped->m_matrix->TransformPoint(CCarEnterExit::ms_vecPedChairAnimOffset);
                pos.x = chairPosWS.x;
                pos.y = chairPosWS.y;

                // Turn around to face away from the chair's back
                ped->m_fAimingRotation = ped->m_fCurrentRotation = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + PI);
                ped->SetHeading(ped->m_fCurrentRotation);
            } else if (currAnimId == ANIM_ID_LOU_OUT) {
                const auto chairPosWS = ped->m_matrix->TransformPoint(CCarEnterExit::ms_vecPedChairAnimOffset);
                pos.x = chairPosWS.x;
                pos.y = chairPosWS.y;
            }
            ped->SetPosn({ pos.x, pos.y, savedZ });
            m_UpdatePedPos = false;
        }

        if (m_SitTimer.IsStarted() && m_SitTimer.IsOutOfTime() && m_Anim->GetAnimId() != ANIM_ID_LOU_OUT) { // Time to get up
            m_Anim->SetDefaultDeleteCallback();
            StartAnim(ANIM_ID_LOU_OUT, 1000.f);
            m_UpdatePedPos = true;
        }

        if (m_Anim->GetAnimId() == ANIM_ID_LOU_IN) { // Walk the ped to the chair while sitting down
            auto pedToIntDir           = m_IntInfo->Pos - ped->GetPosition();
            const auto pedToIntMag     = pedToIntDir.NormaliseAndMag();
            const auto pedToIntShiftWS = pedToIntDir * std::min(pedToIntMag, 0.02f);

            // Transform the shift into ped's object space
            ped->m_vecAnimMovingShiftLocal = {
                pedToIntShiftWS.Dot(ped->GetRight()),
                pedToIntShiftWS.Dot(ped->GetForward()),
            };

            ped->m_fAimingRotation = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(m_IntInfo->Dir.x, m_IntInfo->Dir.y, 0.f, 0.f));
        }
    }

    // While sitting around the ped looks at things in the interior now and then
    if (m_Anim && m_Anim->GetAnimId() == ANIM_ID_LOU_LOOP) {
        if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 1000) > 980) {
            if (const auto group = g_interiorMan.GetPedsInteriorGroup(ped)) {
                InteriorInfo_t* info{};
                Interior_c*     interior{};
                float           distSq{};
                group->FindClosestInteriorInfo(0, ped->GetPosition(), 10.f, &info, &interior, &distSq);
                if (info) {
                    g_ikChainMan.LookAt(
                        "TaskSitInChair",
                        ped,
                        nullptr,
                        CGeneral::GetRandomNumberInRange(10'000, 20'000),
                        BONE_UNKNOWN,
                        &info->Pos,
                        false,
                        0.25f,
                        500,
                        3,
                        false
                    );
                }
            }
        }
    } else if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250u);
    }

    return false;
}
