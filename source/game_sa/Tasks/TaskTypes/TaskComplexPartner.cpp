#include "StdInc.h"

#include "TaskComplexPartner.h"
#include "TaskComplexPartnerChat.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexWanderStandard.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexPartner::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartner, 0x870664, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x681E70);
    RH_ScopedInstall(Destructor, 0x683A40);
    RH_ScopedInstall(GetPartnerState, 0x6822B0);
    RH_ScopedInstall(CalculateMeetingPoints, 0x681FE0);

    RH_ScopedVMTInstall(GetTaskType, 0x681F10);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x681F20);
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0);
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x682310);
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370);
}

// 0x681E70
CTaskComplexPartner::CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point)
    : CTaskComplex()
{
    m_leadSpeaker              = leadSpeaker;
    m_makePedAlwaysFacePartner = makePedAlwaysFacePartner;
    m_distanceMultiplier       = distanceMultiplier;
    m_updateDirectionCount     = updateDirectionCount;
    m_point                    = point;
    m_partner                  = partner;
    m_partnerState             = PARTNER_STATE_INIT;
    m_taskCompleted            = false;
    m_firstToTargetFlag        = -1;
    m_requiredAnimsStreamedIn  = false;
    m_animBlockName[0]         = '\0';
    CEntity::SafeRegisterRef(m_partner);
}

// 0x683A40
CTaskComplexPartner::~CTaskComplexPartner() {
    CEntity::SafeCleanUpRef(m_partner);

    if (m_requiredAnimsStreamedIn) {
        if (strcmp(m_animBlockName, "") != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
        }
        m_requiredAnimsStreamedIn = false;
    }
}

// 0x6822B0
int32 CTaskComplexPartner::GetPartnerState() {
    // The exe does not check `m_partner`; every caller has one
    if (const auto* const partnerTask = static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(m_taskId))) {
        return partnerTask->m_partnerState;
    }
    return PARTNER_STATE_NONE;
}

// 0x681FE0
void CTaskComplexPartner::CalculateMeetingPoints(CPed* ped, CVector& outPoint, CVector& outPartnerPoint) {
    const CVector pedPos     = ped->GetPosition();
    const CVector partnerPos = m_partner->GetPosition();

    // Start at the spot between the peds that `m_distanceMultiplier` selects
    CVector dir = partnerPos - pedPos;
    CVector pos = dir * m_distanceMultiplier + pedPos;
    dir.Normalise();

    // A chat does not care about the peds standing close together
    if (GetTaskType() != TASK_COMPLEX_PARTNER_CHAT) {
        // Both peds are placed half a unit to either side of `pos`, so keep `pos` far enough from each of them
        bool settled = false;
        for (int32 i = 0; i <= 10 && !settled; i++) {
            bool pushedAwayFromPed = false;

            if (const auto distToPed = (pos - pedPos).Magnitude2D(); distToPed < 0.7f) {
                pos += dir * (0.75f - distToPed);
                pushedAwayFromPed = true;
            }

            if (const auto distToPartner = (pos - partnerPos).Magnitude2D(); distToPartner < 0.7f) {
                pos -= dir * (0.75f - distToPartner);
            } else if (!pushedAwayFromPed) {
                settled = true;
            }
        }

        if (!settled) {
            // No spot found after 11 tries; the outputs stay untouched
            m_taskCompleted = true;
            return;
        }
    }

    outPoint        = pos - dir * 0.5f;
    outPartnerPoint = pos + dir * 0.5f;
}

// 0x683AD0
CTask* CTaskComplexPartner::CreateNextSubTask(CPed* ped) {
    if (!m_partner) {
        return nullptr;
    }

    // The partner has to run the same task and be paired with this ped
    auto* const partnerTask = static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(m_taskId));
    if (!partnerTask || partnerTask->m_partner != ped) {
        return nullptr;
    }

    const auto NewStandStill = [] { return new CTaskSimpleStandStill(50, false, false, 8.0f); };

    switch (m_partnerState) {
    case PARTNER_STATE_PERFORM: {
        if (!m_requiredAnimsStreamedIn || m_updateDirectionCount == 0) {
            return nullptr;
        }
        m_updateDirectionCount--;
        return GetPartnerSequence();
    }
    case PARTNER_STATE_ALIGN: {
        if (!m_makePedAlwaysFacePartner) {
            if (GetPartnerState() >= PARTNER_STATE_ALIGN) {
                // Give the ped a fresh standard wander task that does not scan for the next partner task for a
                // while, otherwise the peds would pair up again right away. Its direction comes from the ped's
                // negated forward vector.
                const auto defaultTask = ped->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT);
                if (defaultTask && defaultTask->GetTaskType() == TASK_COMPLEX_WANDER && static_cast<CTaskComplexWander*>(defaultTask)->GetWanderType() == WANDER_TYPE_STANDARD) {
                    const auto& fwd    = ped->GetForward();
                    const auto  wander = new CTaskComplexWanderStandard(PEDMOVE_WALK, (uint8)CGeneral::GetNodeHeadingFromVector(-fwd.x, -fwd.y), true);
                    wander->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100'000;
                    ped->GetTaskManager().SetTask(wander, TASK_PRIMARY_DEFAULT, false);
                }
                m_partnerState = PARTNER_STATE_PERFORM;
            }
            return NewStandStill();
        }

        if (m_firstToTargetFlag == 1) {
            // The ped that arrived first waits for the other one to get into position
            if (GetPartnerState() == PARTNER_STATE_PERFORM) {
                m_partnerState = PARTNER_STATE_PERFORM;
            }
            return NewStandStill();
        }

        // The ped that arrived second closes in until it is exactly one unit away from the partner
        const CVector2D pedPos     = ped->GetPosition();
        const CVector2D partnerPos = m_partner->GetPosition();

        CVector2D awayFromPartner = pedPos - partnerPos;
        const auto dist           = awayFromPartner.Magnitude();
        if (dist > 0.99f && dist < 1.01f) {
            m_partnerState = PARTNER_STATE_PERFORM;
            return NewStandStill();
        }

        awayFromPartner.Normalise();
        CVector2D shift = partnerPos + awayFromPartner - pedPos;
        if (shift.Magnitude() > 0.02f) {
            shift.Normalise();
            shift *= 0.02f;
        }

        // The animation moves the ped by this much (in its own space) each frame
        ped->m_vecAnimMovingShiftLocal = {
            shift.Dot(CVector2D{ped->GetRight()}),
            shift.Dot(CVector2D{ped->GetForward()}),
        };
        return NewStandStill();
    }
    case PARTNER_STATE_WAIT_FOR_PARTNER: {
        const auto partnerState = GetPartnerState();
        if (partnerState == PARTNER_STATE_WAIT_FOR_PARTNER || partnerState == PARTNER_STATE_ALIGN) {
            // The partner is here too; this ped arrived second
            if (m_firstToTargetFlag == -1) {
                m_firstToTargetFlag = 0;
            }
            if (partnerTask->m_firstToTargetFlag == -1) {
                partnerTask->m_firstToTargetFlag = 1;
            }
            m_partnerState = PARTNER_STATE_ALIGN;
            return NewStandStill();
        }

        // This ped arrived first
        if (m_firstToTargetFlag == -1) {
            m_firstToTargetFlag = 1;
        }
        if (partnerTask->m_firstToTargetFlag == -1) {
            partnerTask->m_firstToTargetFlag = 0;
        }
        partnerTask->m_point = m_targetPoint;
        return NewStandStill();
    }
    case PARTNER_STATE_GO_TO_POINT: {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
            return new CTaskComplexTurnToFaceEntityOrCoord(m_partner, 0.5f, 0.2f);
        case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
            m_partnerState = PARTNER_STATE_WAIT_FOR_PARTNER;
            return NewStandStill();
        default:
            return nullptr;
        }
    }
    case PARTNER_STATE_WAIT_FOR_POINT: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }

        // A zero point means the lead speaker has not handed out the meeting point yet
        if (m_point.x == 0.0f && m_point.y == 0.0f) {
            return NewStandStill();
        }

        m_partnerState   = PARTNER_STATE_GO_TO_POINT;
        m_goToPointTicks = 0;
        return new CTaskComplexGoToPointAndStandStill(PEDMOVE_WALK, m_point, 0.1f, 0.0f, false, true);
    }
    case PARTNER_STATE_INIT: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }

        if (m_leadSpeaker) {
            CalculateMeetingPoints(ped, m_point, m_targetPoint);
            partnerTask->m_point = m_targetPoint;
        }
        m_partnerState = PARTNER_STATE_WAIT_FOR_POINT;
        return NewStandStill();
    }
    default:
        return nullptr;
    }
}

// 0x681F20
CTask* CTaskComplexPartner::CreateFirstSubTask(CPed* ped) {
    // The chat is the only partner task with a conversation. Its extra fields live past this class's size, which
    // is why the exe checks the task id here and reads them from the derived task directly (this is also the
    // function CTaskComplexPartnerDeal::CreateFirstSubTask, 0x6823B0, jumps to).
    if (m_leadSpeaker && m_taskId == TASK_COMPLEX_PARTNER_CHAT) {
        auto* const chat = static_cast<CTaskComplexPartnerChat*>(this);
        if (chat->m_conversationEnabled) {
            if (CAEPedSpeechAudioEntity::RequestPedConversation(ped, m_partner)) {
                chat->m_pedConversationLoaded = true;
            } else if (chat->m_conversationOptional) {
                // No conversation could be loaded, carry on without one
                chat->m_conversationEnabled = false;
            } else {
                return nullptr;
            }
        }
    }

    ped->StopPlayingHandSignal();
    return new CTaskSimpleStandStill(50, false, false, 8.0f);
}

// 0x6840D0
CTask* CTaskComplexPartner::ControlSubTask(CPed* ped) {
    // While the task starts up (state 1) the partner may not have its task yet, so the pairing is only enforced
    // once the state machine has advanced
    bool shouldAbort = false;
    if (m_partnerState > PARTNER_STATE_INIT || !m_partner || m_taskCompleted) {
        const auto* const partnerTask = m_partner
            ? static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(m_taskId))
            : nullptr;
        shouldAbort = !partnerTask || partnerTask->m_partner != ped || m_taskCompleted;
    }
    if (shouldAbort && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return nullptr;
    }

    // The go to point sub task can hang forever, so give up on it after 150 calls
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        m_goToPointTicks++;
        if (m_goToPointTicks > 150 && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return nullptr;
        }
    }

    ped->DropEntityThatThisPedIsHolding(true);
    StreamRequiredAnims();
    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 500);
    }
    return m_pSubTask;
}

// 0x682310
void CTaskComplexPartner::StreamRequiredAnims() {
    if (m_requiredAnimsStreamedIn) {
        return;
    }

    // A task without an anim block has nothing to stream; any other waits until its block is loaded
    if (m_animBlockName[0]) {
        const auto blockIdx = CAnimManager::GetAnimationBlockIndex(m_animBlockName);
        if (!CAnimManager::GetAnimBlocks()[blockIdx].IsLoaded) {
            CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
            return;
        }
        CAnimManager::AddAnimBlockRef(blockIdx);
    }
    m_requiredAnimsStreamedIn = true;
}

// 0x682370
void CTaskComplexPartner::RemoveStreamedAnims() {
    if (!m_requiredAnimsStreamedIn) {
        return;
    }

    if (m_animBlockName[0]) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
    }
    m_requiredAnimsStreamedIn = false;
}
