#include "StdInc.h"
#include "TaskInteriorLieInBed.h"

#include "Interior/InteriorInfo_t.h"
#include "Interior/Interior_c.h"
#include "Interior/InteriorManager_c.h"

void CTaskInteriorLieInBed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorLieInBed, 0x870338, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x675E20);
    RH_ScopedInstall(Destructor, 0x675E90);

    RH_ScopedInstall(FinishAnimCB, 0x675FC0);

    RH_ScopedVMTInstall(Clone, 0x675EF0);
    RH_ScopedVMTInstall(GetTaskType, 0x675E80);
    RH_ScopedVMTInstall(MakeAbortable, 0x675F60);
    RH_ScopedVMTInstall(ProcessPed, 0x6772E0);
}

// 0x675E20
CTaskInteriorLieInBed::CTaskInteriorLieInBed(
    int32 duration,
    InteriorInfo_t* intInfo,
    bool rightHandSide,
    bool doInstantly
) :
    m_IntInfo{intInfo},
    m_GetOutAfterInterval{duration},
    m_bDoInstantly{doInstantly},
    m_BaseAnimId{ rightHandSide ? ANIM_ID_BED_IN_R : ANIM_ID_BED_IN_L },

#ifdef FIX_BUGS
    m_bRghtHandSide{rightHandSide}
#endif
{
}

// For `0x675EF0`
CTaskInteriorLieInBed::CTaskInteriorLieInBed(const CTaskInteriorLieInBed& o) :
    CTaskInteriorLieInBed{
        o.m_GetOutAfterInterval,
        o.m_IntInfo,
        o.m_BaseAnimId == ANIM_ID_BED_IN_R, // The exe reads a byte its constructor never wrote, so the side is taken from the base animation
        o.m_bDoInstantly
    }
{
}

// 0x675E90
CTaskInteriorLieInBed::~CTaskInteriorLieInBed() {
    if (m_Anim) {
        m_Anim->SetDefaultDeleteCallback(); // The exe sets the delete callback (type 2), not the finish callback
    }
}

// 0x675FC0
void CTaskInteriorLieInBed::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorLieInBed>(static_cast<CTask*>(data));

    self->m_PrevAnimId = anim->GetAnimId();

    if (self->m_PrevAnimId == self->GetAnimIdInSeq(AnimSeqIdx::GET_OUT) // Last animation in the sequence
     || self->m_TaskAborting && self->m_PrevAnimId == (AnimationId)self->m_BaseAnimId
    ) {
        anim->SetBlendDelta(-1000.f);
        self->m_LastAnimFinished = true;
    }

    self->m_Anim = nullptr;
}

// 0x675F60
bool CTaskInteriorLieInBed::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        if (m_Anim) {
            m_Anim->SetBlendDelta(-1000.f);
            m_Anim->SetDefaultDeleteCallback();
            m_Anim = nullptr;
        }
        ped->GetIntelligence()->GetEventScanner().GetAcquaintanceScanner().SetOnlyScriptPedAllowed();
        return true;
    } else {
        m_TaskAborting = true;
        return false;
    }
}

// 0x6772E0
bool CTaskInteriorLieInBed::ProcessPed(CPed* ped) {
    const auto currAnimId = m_Anim
        ? (AnimationId)m_Anim->GetAnimId()
        : ANIM_ID_UNDEFINED;

    ped->SetMoveState(PEDMOVE_STILL);

    if (m_LastAnimFinished) {
        if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), GetAnimIdInSeq(AnimSeqIdx::GET_OUT))) { // Check if last anim has really finished
            ped->GetIntelligence()->GetEventScanner().GetAcquaintanceScanner().SetOnlyScriptPedAllowed();
            return true;
        }
    }

    const auto StartAnim = [&, this](AnimSeqIdx seqIdx, float blendDelta) {
        m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_HOUSE, GetAnimIdInSeq(seqIdx), blendDelta);
        m_Anim->SetFinishCallback(FinishAnimCB, this);
    };

    // Only the paths that already have an animation replace it; the ones that create the first animation have nothing to detach
    const auto SwitchToAnim = [&, this](AnimSeqIdx seqIdx) {
        m_Anim->SetDefaultDeleteCallback();
        StartAnim(seqIdx, 1000.f);
        m_UpdatePedPos = true;
    };

    if (m_TaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(ANIM_GROUP_DEFAULT)) {
            ped->GetIntelligence()->GetEventScanner().GetAcquaintanceScanner().SetOnlyScriptPedAllowed();
            return true;
        }

        if (currAnimId == GetAnimIdInSeq(AnimSeqIdx::GET_IN)) {
            m_Anim->SetBlendDelta(-8.f);
        } else if (currAnimId == GetAnimIdInSeq(AnimSeqIdx::LOOP)) {
            if (!m_UpdatePedPos) {
                SwitchToAnim(AnimSeqIdx::GET_OUT);
                return false;
            }
        } else if (currAnimId == GetAnimIdInSeq(AnimSeqIdx::GET_OUT)) {
            m_Anim->m_Speed = 3.f; // Gets out faster; this is the speed, not the blend delta
        }
    }

    if (!m_Anim) { // Create animation
        if (!InteriorManager_c::AreAnimsLoaded(ANIM_GROUP_DEFAULT)) {
            return false;
        }

        if (m_PrevAnimId == ANIM_ID_UNDEFINED) { // Nothing played yet
            ped->GetIntelligence()->GetEventScanner().GetAcquaintanceScanner().TurnOffAllScanners();
            if (m_bDoInstantly) {
                m_GetOutTimer.Start(m_GetOutAfterInterval);
                StartAnim(AnimSeqIdx::LOOP, 1000.f);
                m_UpdatePedPos = true;
            } else {
                StartAnim(AnimSeqIdx::GET_IN, 4.f); // The timer starts and the position is placed once the ped is in the bed
            }
        } else if (m_PrevAnimId == GetAnimIdInSeq(AnimSeqIdx::GET_IN)) {
            m_GetOutTimer.Start(m_GetOutAfterInterval);
            StartAnim(AnimSeqIdx::LOOP, 1000.f);
            m_UpdatePedPos = true;
        }
        return false;
    }

    if (m_UpdatePedPos) {
        // Anims without an offset (the ones that get in) keep the current position, but the exe still sets it
        auto       pos    = ped->GetPosition();
        const auto savedZ = pos.z;
        switch (currAnimId) {
        case ANIM_ID_BED_LOOP_L:
        case ANIM_ID_BED_OUT_L:
            pos = ped->m_matrix->TransformPoint(CCarEnterExit::ms_vecPedBedLAnimOffset);
            break;
        case ANIM_ID_BED_LOOP_R:
        case ANIM_ID_BED_OUT_R:
            pos = ped->m_matrix->TransformPoint(CCarEnterExit::ms_vecPedBedRAnimOffset);
            break;
        default:
            break;
        }
        pos.z = savedZ;
        ped->SetPosn(pos);
        m_UpdatePedPos = false;

        if (currAnimId == GetAnimIdInSeq(AnimSeqIdx::LOOP)) {
            ped->m_fAimingRotation = ped->m_fCurrentRotation = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + PI);
            ped->SetHeading(ped->m_fCurrentRotation);
        }
    }

    if (m_GetOutTimer.IsStarted() && m_GetOutTimer.IsOutOfTime() && m_Anim->GetAnimId() != GetAnimIdInSeq(AnimSeqIdx::GET_OUT)) {
        SwitchToAnim(AnimSeqIdx::GET_OUT);
    }

    // The animation can have been replaced just above, so its id is read again; only getting in walks the ped to the bed
    if (m_Anim->GetAnimId() == GetAnimIdInSeq(AnimSeqIdx::GET_IN)) {
        auto pedToIntDir           = m_IntInfo->Pos - ped->GetPosition();
        const auto pedToIntMag     = pedToIntDir.NormaliseAndMag();
        const auto pedToIntShiftWS = pedToIntDir * std::min(pedToIntMag, 0.02f);

        // Now transform the point into ped's object space from world space
        // in quite a convoluted way.
        ped->m_vecAnimMovingShiftLocal = {
            pedToIntShiftWS.Dot(ped->GetRight()),
            pedToIntShiftWS.Dot(ped->GetForward()),
        };

        ped->m_fAimingRotation = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(m_IntInfo->Dir.x, m_IntInfo->Dir.y, 0.f, 0.f));
    }

    return false;
}

// NOTSA
AnimationId CTaskInteriorLieInBed::GetAnimIdInSeq(AnimSeqIdx sequenceIdx) {
    using enum AnimSeqIdx;

    switch (m_BaseAnimId) {
    case ANIM_ID_BED_IN_R:
        switch (sequenceIdx) {
        case GET_IN:  return ANIM_ID_BED_IN_R;
        case LOOP:    return ANIM_ID_BED_LOOP_R;
        case GET_OUT: return ANIM_ID_BED_OUT_R;
        default: NOTSA_UNREACHABLE();
        }
    case ANIM_ID_BED_IN_L:
        switch (sequenceIdx) {
        case GET_IN:  return ANIM_ID_BED_IN_L;
        case LOOP:    return ANIM_ID_BED_LOOP_L;
        case GET_OUT: return ANIM_ID_BED_OUT_L;
        default: NOTSA_UNREACHABLE();
        }
    default:
        NOTSA_UNREACHABLE();
    }
}
