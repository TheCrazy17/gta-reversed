#include "StdInc.h"

#include "TaskGangHassleVehicle.h"
#include "TaskComplexTrackEntity.h"
#include "TaskComplexSmartFleeEntity.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskGangHasslePed.h"
#include "TaskSimpleShakeFist.h"
#include "TaskSimpleRunAnim.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskSimpleFight.h"
#include "Ragdoll/IKChainManager.h"
#include "Collision/Collision.h"
#include "Game.h"

void CTaskGangHassleVehicle::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHassleVehicle, 0x86F9D4, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FAC0);
    RH_ScopedInstall(Destructor, 0x65FB60);
    RH_ScopedInstall(GetTargetHeading, 0x65FDD0);
    RH_ScopedInstall(CalcTargetOffset, 0x6641A0);
    RH_ScopedInstall(Clone, 0x65FC00);
    RH_ScopedInstall(CreateNextSubTask, 0x65FC80);
    RH_ScopedInstall(CreateFirstSubTask, 0x664BA0);
    RH_ScopedInstall(ControlSubTask, 0x6637C0);
}

// 0x65FAC0
CTaskGangHassleVehicle::CTaskGangHassleVehicle(CVehicle* vehicle, int32 a3, uint8 a4, float a5, float a6) : CTaskComplex() {
    m_nTime = 0;
    dword3C = 0;
    byte40 = 0;
    byte41 = 0;
    byte18 = a4;
    dword1C = a5;
    m_Vehicle = vehicle;
    m_nHasslePosId = -1;
    m_fOffsetX = a6;
    m_bRemoveAnim = 0;
    m_pEntity = nullptr;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x65FB60
CTaskGangHassleVehicle::~CTaskGangHassleVehicle() {
    if (m_Vehicle) {
        if (m_nHasslePosId > -1) {
            m_Vehicle->SetHasslePosId(m_nHasslePosId, false);
        }
        CEntity::SafeCleanUpRef(m_Vehicle);
    }

    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bRemoveAnim) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }
}

// 0x65FDD0
float CTaskGangHassleVehicle::GetTargetHeading(CPed* ped) {
    const auto& matrix = m_Vehicle->GetMatrix();

    float dx, dy;
    switch (m_nHasslePosId) {
    case 1:
    case 3:
        dx = -matrix.GetRight().x;
        dy = -matrix.GetRight().y;
        break;
    case 4:
        dx = matrix.GetUp().x;
        dy = matrix.GetUp().y;
        break;
    case 5:
        dx = -matrix.GetUp().x;
        dy = -matrix.GetUp().y;
        break;
    default: // 0, 2
        dx = matrix.GetRight().x;
        dy = matrix.GetRight().y;
        break;
    }

    return CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(dx, dy, 0.0f, 0.0f));
}

// 0x6641A0
void CTaskGangHassleVehicle::CalcTargetOffset() {
    m_vecPosn = CVector{};

    const auto& box = m_Vehicle->GetModelInfo()->GetColModel()->GetBoundingBox();
    switch (m_nHasslePosId) {
    case 0:
        m_vecPosn.x = box.m_vecMin.x - m_fOffsetX;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 1:
        m_vecPosn.x = box.m_vecMax.x + m_fOffsetX;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 2:
        m_vecPosn.x = box.m_vecMin.x - m_fOffsetX;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 3:
        m_vecPosn.x = box.m_vecMax.x + m_fOffsetX;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 4:
        m_vecPosn.y = box.m_vecMin.y - m_fOffsetX;
        break;
    case 5:
        m_vecPosn.y = box.m_vecMax.y + m_fOffsetX;
        break;
    }
}

namespace {
auto& s_HassleVehFleeTime            = StaticRef<int32>(0x86F674);
auto& s_HassleVehFleePosCheckPeriod  = StaticRef<int32>(0x86F678);
auto& s_HassleVehFleePosChangeTolerance = StaticRef<float>(0xC18CF0);

// NOTSA: same list as CTaskComplexGangLeader's file-local s_gangTalkAnims (that one has internal linkage)
constexpr AnimationId s_gangTalkAnims[]{
    ANIM_ID_PRTIAL_GNGTLKA, ANIM_ID_PRTIAL_GNGTLKB, ANIM_ID_PRTIAL_GNGTLKC, ANIM_ID_PRTIAL_GNGTLKD,
    ANIM_ID_PRTIAL_GNGTLKE, ANIM_ID_PRTIAL_GNGTLKF, ANIM_ID_PRTIAL_GNGTLKG, ANIM_ID_PRTIAL_GNGTLKH,
};
}; // namespace

// 0x65FC80
CTask* CTaskGangHassleVehicle::CreateNextSubTask(CPed* ped) {
    if (!m_Vehicle) {
        return nullptr;
    }

    if (m_pSubTask && m_pSubTask->GetTaskType() == TASK_COMPLEX_SMART_FLEE_ENTITY) {
        return nullptr;
    }

    if (m_Vehicle->GetHealth() < 250.0f) {
        return new CTaskComplexSmartFleeEntity(m_Vehicle, false, 30.0f, s_HassleVehFleeTime, s_HassleVehFleePosCheckPeriod, s_HassleVehFleePosChangeTolerance);
    }

    if (m_pSubTask && (m_pSubTask->GetTaskType() == TASK_COMPLEX_GANG_HASSLE_PED || m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY)) {
        return nullptr;
    }

    return new CTaskComplexTrackEntity(m_Vehicle, m_vecPosn, 1, -1, 10.0f, 40.0f, 1);
}

// 0x664BA0
CTask* CTaskGangHassleVehicle::CreateFirstSubTask(CPed* ped) {
    if (!m_Vehicle) {
        return nullptr;
    }

    m_pEntity = m_Vehicle->GetDriver();
    CEntity::SafeRegisterRef(m_pEntity);

    const auto& box = m_Vehicle->GetModelInfo()->GetColModel()->GetBoundingBox();
    if (box.m_vecMax.x - box.m_vecMin.x > 4.0f || box.m_vecMax.y - box.m_vecMin.y > 8.0f) {
        return nullptr;
    }

    m_nHasslePosId = m_Vehicle->GetSpareHasslePosId();
    if (m_nHasslePosId == -1) {
        return nullptr;
    }

    m_Vehicle->SetHasslePosId(m_nHasslePosId, true);
    CalcTargetOffset();
    m_b31 = false;
    ped->DropEntityThatThisPedIsHolding(true);

    m_nTime = CTimer::GetTimeInMS();
    dword3C = CGeneral::GetRandomNumberInRange(150000, 250000);
    byte40 = true;

    if (ped->IsInVehicle()) {
        return new CTaskComplexLeaveCar(ped->m_pVehicle, 0, 0, true, false);
    }
    return CreateNextSubTask(ped);
}

// 0x6637C0
CTask* CTaskGangHassleVehicle::ControlSubTask(CPed* ped) {
    // Target vehicle vanished -> abort/keep current subtask (unless already fighting on foot)
    if (m_pSubTask && m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT && !m_Vehicle) {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr) ? nullptr : m_pSubTask;
    }

    // "gangs" anim block ref-counting (load while wanted, unload once not)
    if (!m_bRemoveAnim) {
        if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            const auto blockIdx = CAnimManager::GetAnimationBlockIndex("gangs");
            if (CAnimManager::GetAnimBlocks()[blockIdx].IsLoaded) {
                CAnimManager::AddAnimBlockRef(blockIdx);
                m_bRemoveAnim = true;
            } else {
                CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
            }
        }
    } else if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }

    // Escalation timer: once armed (byte40, set in CreateFirstSubTask) and byte18 is set,
    // after `dword3C` ms fully attack the driver instead of just hassling.
    if (byte40) {
        if (byte41) {
            m_nTime = CTimer::GetTimeInMS();
            byte41 = false;
        }
        if (CTimer::GetTimeInMS() >= m_nTime + dword3C && byte18) {
            if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) {
                return new CTaskComplexKillPedOnFoot(static_cast<CPed*>(m_pEntity), -1, 0, 0, 0, 1);
            }
            return m_pSubTask;
        }
    }

    // Driver bailed out of the vehicle -> switch to hassling them directly on foot
    if (m_Vehicle && !m_Vehicle->GetDriver() && m_pEntity
        && m_pSubTask->GetTaskType() != TASK_COMPLEX_GANG_HASSLE_PED
        && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        m_b31 = 3;
        return new CTaskGangHasslePed(static_cast<CPed*>(m_pEntity), byte18 ? 2 : 1, 12000, 20000);
    }

    // Squared distance to the target vehicle, from the tracking subtask if present
    float distSq = 100.0f;
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY) {
        distSq = static_cast<CTaskComplexTrackEntity*>(m_pSubTask)->m_distToTargetSq;
    }

    // Cosmetic: when close, visible and not already looking, occasionally IK-LookAt the driver
    if (ped->IsVisible() && distSq < 4.0f && !g_ikChainMan.IsLooking(ped)
        && CGeneral::GetRandomNumberInRange(0, 100) > 60 && m_Vehicle->HasDriver()) {
        g_ikChainMan.LookAt(
            "TaskHassleVehicle", ped, m_Vehicle->GetDriver(),
            CGeneral::GetRandomNumberInRange(1000, 3000), BONE_HEAD, nullptr, true, 0.15f, 500, 3, false
        );
    }

    if (!m_pSubTask
        || (m_pSubTask->GetTaskType() != TASK_COMPLEX_TRACK_ENTITY
            && m_pSubTask->GetTaskType() != TASK_COMPLEX_FOLLOW_NODE_ROUTE)) {
        return m_pSubTask;
    }

    if (m_Vehicle->GetHealth() < 250.0f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateNextSubTask(ped);
    }

    switch (m_b31) {
    case 0: {
        if (!ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)
            && CGeneral::GetRandomNumberInRange(0, 100) > 60) {
            ped->GetTaskManager().SetTaskSecondary(new CTaskSimpleShakeFist(), TASK_SECONDARY_PARTIAL_ANIM);
        }
        if (distSq < dword1C * dword1C) {
            m_b31 = 1;
        }
        ped->Say(CTX_GLOBAL_CHASE_CAR, 0, 1.0f, false, false, false);
        return m_pSubTask;
    }
    case 1: {
        const auto targetHeading = GetTargetHeading(ped);
        ped->m_fAimingRotation = targetHeading;
        const auto curHeading = ped->GetHeading();
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_TRACK_ENTITY) {
            return m_pSubTask;
        }
        if (distSq <= dword1C * dword1C) {
            if (std::abs(curHeading - targetHeading) < 0.05f) {
                m_b31 = 2;
            }
            return m_pSubTask;
        }
        break; // Too far again -> falls through to the shared `m_b31 = 0` tail below
    }
    case 2: {
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY
            && distSq > dword1C * dword1C + 0.05f) {
            m_b31 = 0;
        }
        const auto targetHeading = GetTargetHeading(ped);
        const auto curHeading = ped->GetHeading();
        if (std::abs(curHeading - targetHeading) >= 0.1f) {
            m_b31 = 1;
        }
        if (const auto roll = CGeneral::GetRandomNumberInRange(0, 3); roll < 3) {
            static constexpr eGlobalSpeechContext taunts[3] = {
                CTX_GLOBAL_DRIVE_THROUGH_TAUNT, CTX_GLOBAL_ATTACK_CAR, CTX_GLOBAL_TIP_CAR
            };
            ped->Say(taunts[roll], 0, 1.0f, false, false, false);
        }

        if (m_bRemoveAnim && *reinterpret_cast<int32*>(reinterpret_cast<char*>(ped) + 0x534) < 6) { // NOTSA: CPed field, not identified
            if (const auto gestureTask = ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
                if (sq(dword1C) < distSq) {
                    m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr); // result discarded (fire-and-forget)
                }

                auto* assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARA);
                float progressThreshold = 0.5f;
                if (!assoc) {
                    progressThreshold = 0.7f;
                    assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARSH);
                    if (!assoc) {
                        progressThreshold = 0.5f;
                        assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARK);
                        if (!assoc) {
                            return m_pSubTask;
                        }
                    }
                }

                // Only once playback is solidly past the impact point (this frame AND ~last frame)
                if (assoc->m_CurrentTime <= progressThreshold || (assoc->m_CurrentTime - assoc->m_TimeStep) < progressThreshold) {
                    return m_pSubTask;
                }

                // "Rock the car": a vertical off-centre push, torqued toward whichever side the ped is hassling from
                const float fMag = *reinterpret_cast<const float*>(reinterpret_cast<const char*>(m_Vehicle) + 0x8C) * 0.02f; // NOTSA: CVehicle field, not identified
                CVector pushPoint{};
                switch (m_nHasslePosId) {
                case 0: case 2: pushPoint = -m_Vehicle->GetMatrix().GetRight();   break;
                case 1: case 3: pushPoint =  m_Vehicle->GetMatrix().GetRight();   break;
                case 4:         pushPoint = -m_Vehicle->GetMatrix().GetForward(); break;
                case 5:         pushPoint =  m_Vehicle->GetMatrix().GetForward(); break;
                default:        pushPoint = CVector{ 0.f, 0.f, fMag };            break;
                }
                m_Vehicle->ApplyTurnForce({ 0.f, 0.f, fMag }, pushPoint);

                {
                    const auto oldHealth = m_Vehicle->m_fHealth; // saved/restored: this probe must stay cosmetic-only

                    CTaskSimpleFight fightReaction{ m_Vehicle, 0xB /* nCommand, not identified */, 20000u }; // stack-local; work happens in ctor

                    CMatrix probe{ ped->GetMatrix() };
                    probe.GetPosition() += ped->GetMatrix().GetForward(); // roughly arm's-reach in front of the ped

                    plugin::Call<0x61D5F0, float>(0.5f); // resizes the shared probe CColModel (`col1[0]`) to a +-0.5 cube/sphere

                    if (CCollision::ProcessColModels(probe, col1[0], m_Vehicle->GetMatrix(), *m_Vehicle->GetColModel(), CWorld::m_aTempColPts, nullptr, nullptr, false) > 0) {
                        // NOTSA: the original also spawns a hit FX/sound/rattle here (0x61D0B0) - not translated,
                        // its calling convention (an unidentified stack-scratch `this`) wasn't confidently resolved.
                        // Skipping it is purely cosmetic; the health save/restore below already makes this probe non-damaging.
                    }

                    m_Vehicle->m_fHealth = oldHealth;
                    m_Vehicle->m_vehicleAudio.AddAudioEvent(eAudioEvents(0x6C), 0.0f);
                }
                return m_pSubTask;
            } else {
                // Not gesturing yet: roll a random secondary anim/gesture to start
                const auto roll = CGeneral::GetRandomNumberInRange(0, 200);
                CTask* newSecondary = nullptr;
                if (roll > 166) {
                    newSecondary = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARA, 4.0f, false);
                } else if (roll > 133) {
                    newSecondary = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARSH, 4.0f, false);
                } else if (roll > 100) {
                    newSecondary = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARK, 4.0f, false);
                } else if (roll > 70) {
                    newSecondary = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, CGeneral::RandomChoice(s_gangTalkAnims), 4.0f, false);
                } else if (roll > 60 && !ped->IsPlayingHandSignal()) {
                    newSecondary = new CTaskComplexPlayHandSignalAnim();
                }
                if (newSecondary) {
                    ped->GetTaskManager().SetTaskSecondary(newSecondary, TASK_SECONDARY_PARTIAL_ANIM);
                }
                return newSecondary; // matches the original's non-standard early-return path
            }
        }
        break;
    }
    default:
        break;
    }

    m_b31 = 0;
    return m_pSubTask;
}
