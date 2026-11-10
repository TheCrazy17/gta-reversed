#include "StdInc.h"

#include "TaskSimpleGangDriveBy.h"
#include "EventDamage.h"
#include "WeaponInfo.h"
#include "PlayerPed.h"

namespace {
// Time the animation is rewound to when it gets replaced (a tuning value of the exe, kept as a reference in case something changes it)
auto& s_StartAnimTime = StaticRef<float>(0x8D2E7C);
} // namespace

void CTaskSimpleGangDriveBy::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleGangDriveBy, 0x86D944, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6217D0);
    RH_ScopedInstall(Destructor, 0x6218C0);
    RH_ScopedInstall(StartDriveByAnim, 0x627B20);
    RH_ScopedInstall(FinishGangDriveByAnimCB, 0x621BE0);
    RH_ScopedVMTInstall(Clone, 0x6236D0);
    RH_ScopedVMTInstall(MakeAbortable, 0x62D290);
}

// 0x6217D0
CTaskSimpleGangDriveBy::CTaskSimpleGangDriveBy(CEntity* target, const CVector* targetPos, float abortRange, int8 frequencyPercentage, eDrivebyStyle drivebyStyle, bool seatRHS) {
    m_bSeatRHS             = seatRHS;
    m_nDrivebyStyle        = drivebyStyle;
    m_fAbortRange          = abortRange;
    m_pTargetEntity        = target;
    m_nFrequencyPercentage = frequencyPercentage;
    m_bIsFinished          = false;
    m_bAnimsReferenced     = false;
    m_bInRangeToShoot      = false;
    m_bInWeaponRange       = false;
    m_bReachedAbortRange   = false;
    m_bFromScriptCommand   = false;
    m_nBurstShots          = -1;
    m_nFakeShootDirn       = -1;
    m_nAttackTimer         = -1;
    m_nLastCommand         = 0;
    m_nNextCommand         = 1;
    m_nLOSCheckTime        = 0;
    m_nLOSBlocked          = true;
    m_pAnimAssoc           = nullptr;
    m_nRequiredAnimID      = ANIM_ID_NO_ANIMATION_SET;
    m_nRequiredAnimGroup   = ANIM_GROUP_DEFAULT;
    m_pWeaponInfo          = nullptr;
    CEntity::SafeRegisterRef(m_pTargetEntity);
    if (targetPos) {
        m_vecCoords = *targetPos;
    }
}

// 0x6218C0
CTaskSimpleGangDriveBy::~CTaskSimpleGangDriveBy()
{
    if (m_bAnimsReferenced)
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));

    if (m_pAnimAssoc)
        m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);

    CEntity::SafeCleanUpRef(m_pTargetEntity);
}

// 0x6236D0
CTask* CTaskSimpleGangDriveBy::Clone() const {
    auto* const clone = new CTaskSimpleGangDriveBy(m_pTargetEntity, &m_vecCoords, m_fAbortRange, m_nFrequencyPercentage, m_nDrivebyStyle, m_bSeatRHS);
    clone->m_bFromScriptCommand = m_bFromScriptCommand; // The only member the constructor does not take
    return clone;
}

// 0x62D290
bool CTaskSimpleGangDriveBy::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event)
{
    if (priority != ABORT_PRIORITY_URGENT && priority != ABORT_PRIORITY_IMMEDIATE) {
        m_nNextCommand = 6;
        return false;
    }

    // Damage that did not kill the ped must not end the drive by
    if (event && event->GetEventType() == EVENT_DAMAGE && !static_cast<const CEventDamage*>(event)->HasKilledPed()) {
        return false;
    }

    m_nNextCommand = 7;
    if (StartDriveByAnim(ped)) {
        m_nLastCommand = m_nNextCommand;
        if (priority == ABORT_PRIORITY_IMMEDIATE) {
            m_pAnimAssoc->m_BlendDelta = -1000.0f; // Gone this frame
        } else {
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
        }
    }

    if (const auto playerData = ped->GetPlayerData()) {
        playerData->m_bFreeAiming = true; // Bit 3 of the flags, the exe ors in 8
    }
    if (ped->IsPlayer()) {
        ped->AsPlayer()->GetPlayerInfoForThisPlayerPed()->m_bCanDoDriveBy = true;
    }

    if (m_pAnimAssoc) {
        // An animation that is not playing yet and is not already fading out gets faded out
        if (!m_pAnimAssoc->IsPlaying()
            && (m_pAnimAssoc->m_BlendDelta > 0.0f || (m_pAnimAssoc->m_BlendAmount > 0.0f && m_pAnimAssoc->m_BlendDelta >= 0.0f))) {
            m_pAnimAssoc->m_BlendDelta = -8.0f;
        }
        m_pAnimAssoc->SetDefaultDeleteCallback();
        m_pAnimAssoc = nullptr;
    }

    m_bIsFinished = true;
    return true;
}

// 0x627B20
bool CTaskSimpleGangDriveBy::StartDriveByAnim(CPed* ped) {
    // The group has no block yet when it was never loaded, then it is found by name
    auto* animBlock = CAnimManager::GetAnimationBlock(m_nRequiredAnimGroup);
    if (!animBlock) {
        animBlock = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(m_nRequiredAnimGroup));
    }
    const auto animBlockIdx = CAnimManager::GetAnimationBlockIndex(animBlock);

    if (!animBlock->IsLoaded) {
        CStreaming::RequestModel(IFPToModelId(animBlockIdx), STREAMING_KEEP_IN_MEMORY);
        return false;
    }

    if (!m_bAnimsReferenced) {
        CAnimManager::AddAnimBlockRef(animBlockIdx);
        m_bAnimsReferenced = true;
    }

    if (m_nRequiredAnimID == ANIM_ID_NO_ANIMATION_SET && !m_pAnimAssoc) {
        return false;
    }

    const auto needsNewAnim = [&] {
        if (!m_pAnimAssoc) {
            return m_nAttackTimer <= 0 && m_bInWeaponRange;
        }
        if ((int32)m_pAnimAssoc->GetAnimId() == m_nRequiredAnimID) {
            return false;
        }
        // Only commands 1 to 3 may replace the animation, and not while a later command is the last one
        return (m_nNextCommand == 1 || m_nNextCommand == 2 || m_nNextCommand == 3) && m_nLastCommand < 4;
    }();

    if (needsNewAnim) {
        bool replacedAnim = false;
        if (m_pAnimAssoc) {
            m_pAnimAssoc->SetDefaultDeleteCallback();
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
            replacedAnim = true;
        }

        m_pAnimAssoc = CAnimManager::BlendAnimation(ped->GetRpClump(), m_nRequiredAnimGroup, (AnimationId)m_nRequiredAnimID, 4.0f);
        m_pAnimAssoc->SetDeleteCallback(FinishGangDriveByAnimCB, this);

        if (m_nNextCommand == 3 && m_nBurstShots <= 1) {
            m_nBurstShots = (int8)CGeneral::GetRandomNumberInRange<int32>(2, (int16)m_pWeaponInfo->m_nAmmoClip);
        }

        if (replacedAnim) {
            m_pAnimAssoc->SetCurrentTime(s_StartAnimTime);
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
            m_nLastCommand = 1;
            m_nNextCommand = 0;
            if (!ped->IsPlayer()) {
                m_nAttackTimer = 100;
            }
        } else {
            m_nLastCommand = m_nNextCommand;
            m_nNextCommand = 0;
        }
    }

    if (!m_pAnimAssoc) {
        return false;
    }
    return m_pAnimAssoc->m_BlendAmount > 0.9f && m_pAnimAssoc->m_BlendDelta >= 0.0f;
}

// 0x621BE0
void CTaskSimpleGangDriveBy::FinishGangDriveByAnimCB(CAnimBlendAssociation* anim, void* data) {
    auto* const task = static_cast<CTaskSimpleGangDriveBy*>(data);
    if (task->m_pAnimAssoc == anim) {
        task->m_pAnimAssoc = nullptr;
    }
}

bool CTaskSimpleGangDriveBy::ProcessPed(CPed* ped)
{
    return plugin::CallMethodAndReturn<bool, 0x62D3B0, CTask*, CPed*>(this, ped);
}
