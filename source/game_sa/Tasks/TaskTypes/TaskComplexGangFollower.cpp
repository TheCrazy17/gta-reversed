#include "StdInc.h"

#include "TaskComplexGangFollower.h"
#include "Audio/Enums/PedSpeechContexts.h"
#include "TaskSimplePause.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskComplexWanderGang.h"
#include "TaskSimpleCarDrive.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleStandStill.h"
#include "SeekEntity/TaskComplexSeekEntity.h"
#include "TaskSimpleGoToPoint.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexPassObject.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleHoldEntity.h"
#include "Events/EventPassObject.h"
#include "Ragdoll/IKChainManager.h"
#include "Fx/FxManager.h"
#include "Fx/FxSystem.h"

void CTaskComplexGangFollower::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGangFollower, 0x86F938, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65EAA0);
    RH_ScopedInstall(Destructor, 0x65EBB0);
    //RH_ScopedInstall(CalculateOffsetPosition, 0x65ED40, { .Reversed = false }); // not hooked because i want to keep CVector return, but original function took a CVector&
    RH_ScopedInstall(Clone, 0x65ECB0);
    RH_ScopedInstall(MakeAbortable, 0x65EC30);
    RH_ScopedInstall(CreateNextSubTask, 0x665E00);
    RH_ScopedInstall(CreateFirstSubTask, 0x666160);
    RH_ScopedInstall(ControlSubTask, 0x662A10);
}

// 0x65EAA0
CTaskComplexGangFollower::CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6) : CTaskComplex() {
    m_PedGroup = pedGroup;
    m_Leader   = ped;
    byte3C     = a4;
    dword2C    = pos;
    dword20    = pos;
    dword38    = a6;

    m_Flags |= 0x14;

    if (m_Leader) {
        CEntity::RegisterReference(m_Leader);
        m_PedPosn = m_Leader->GetPosition();
    }

    m_Flags &= ~3;
    if (m_Leader == FindPlayerPed(0)) {
        m_Flags |= 8;
    } else {
        m_Flags &= ~8;
    }
}


// 0x65EBB0
CTaskComplexGangFollower::~CTaskComplexGangFollower() {
    CEntity::SafeCleanUpRef(m_Leader);

    if (m_Flags & 1) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_Flags &= ~1;
    }
}

// 0x65ED40
CVector CTaskComplexGangFollower::CalculateOffsetPosition() {
    CVector ret;
    plugin::CallMethod<0x65ED40, CTaskComplexGangFollower*>(this, &ret);
    return ret;
}

// 0x65ECB0
CTask* CTaskComplexGangFollower::Clone() const {
    auto* clone = new CTaskComplexGangFollower(m_PedGroup, m_Leader, byte3C, dword20, dword38);
    clone->m_Flags = (clone->m_Flags & ~4) | (m_Flags & 4);
    return clone;
}

// 0x65EC30
bool CTaskComplexGangFollower::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_pSubTask && !m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }

    ped->bDontAcceptIKLookAts = false;
    ped->bMoveAnimSpeedHasBeenSetByTask = false;
    return true;
}

// Run after the leader, keeping the offset the gang member was given
static CTask* CreateSeekLeaderTask(CPed* leader, const CVector& offset) {
    auto* const seek = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
        leader,
        50'000,
        1'000,
        0.5f,
        6.0f,
        2.0f,
        false,
        true,
        CEntitySeekPosCalculatorXYOffset{ offset }
    };
    seek->SetMoveState(PEDMOVE_SPRINT);
    seek->SetIsTrackingEntity(true);
    return seek;
}

// 0x665E00
CTask* CTaskComplexGangFollower::CreateNextSubTask(CPed* ped) {
    static_assert(offsetof(CPed, m_nMoveState) == 0x534);

    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    static auto& s_MaxStaticCounter = StaticRef<int32>(0x86C938);
    const auto   tooManyFollowers   = ped->GetIntelligence()->m_AnotherStaticCounter > s_MaxStaticCounter;
    const auto   subTaskType        = m_pSubTask->GetTaskType();

    // Done signalling, leave the group and go wandering
    if ((m_Flags & 2) && subTaskType == TASK_COMPLEX_SIGNAL_AT_PED) {
        m_PedGroup->GetMembership().RemoveMember(ped);
        ped->GetTaskManager().SetTask(
            new CTaskComplexWanderGang{ PEDMOVE_WALK, (uint8)CGeneral::GetRandomNumberInRange(0, 8), 30'000, true, 0.5f },
            TASK_PRIMARY_DEFAULT,
            false
        );
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    const auto stayPut = tooManyFollowers || (subTaskType == TASK_COMPLEX_SEEK_ENTITY && (int32)m_Leader->m_nMoveState < PEDMOVE_WALK);
    if (!stayPut) {
        switch (subTaskType) {
        case TASK_SIMPLE_STAND_STILL:
        case TASK_COMPLEX_HANDSIGNAL_ANIM: {
            if (m_Flags & 2) {
                return new CTaskComplexSignalAtPed{ m_Leader, -1, false };
            }
            // Now and then look at somebody else of the gang
            if (CGeneral::GetRandomNumberInRange(0, 30) == 20) {
                auto& membership = m_PedGroup->GetMembership();
                auto* member     = membership.GetMember(CGeneral::GetRandomNumberInRange(0, (int32)membership.CountMembers()));
                if (member == ped) {
                    member = membership.GetLeader();
                }
                if (member) {
                    return new CTaskComplexTurnToFaceEntityOrCoord{ member, 0.5f, 0.2f };
                }
            }
            return new CTaskSimplePause{ 50 };
        }
        case TASK_SIMPLE_CAR_DRIVE:
            return CreateFirstSubTask(ped);
        default:
            if (m_Flags & 4) {
                return CreateSeekLeaderTask(m_Leader, dword20);
            }
            return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
        }
    }

    if (m_Leader->IsPlayer()) {
        ped->Say(CTX_GLOBAL_FOLLOW_ARRIVE, 0, 0.3f);
    }
    return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
}

// 0x666160
CTask* CTaskComplexGangFollower::CreateFirstSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    // Ride along when the leader is in the same vehicle, otherwise get out of the one the ped is in
    if (ped->bInVehicle && ped->m_pVehicle) {
        if (m_Leader->m_pVehicle == ped->m_pVehicle) {
            return new CTaskSimpleCarDrive{ ped->m_pVehicle, nullptr, false };
        }
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    }

    static auto& s_MaxStaticCounter = StaticRef<int32>(0x86C938);
    if ((m_Flags & 4) && !(ped->GetIntelligence()->m_AnotherStaticCounter > s_MaxStaticCounter)) {
        return CreateSeekLeaderTask(m_Leader, dword20);
    }
    return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
}

// 0x662A10
CTask* CTaskComplexGangFollower::ControlSubTask(CPed* ped) {
    ZoneScoped;

    ped->bDontAcceptIKLookAts = false;

    if (!m_Leader) {
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            ped->bMoveAnimSpeedHasBeenSetByTask = false;
            return nullptr;
        }
        return m_pSubTask;
    }
    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE) {
        return m_pSubTask;
    }

    auto* const walkAnim = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_ID_WALK);

    const auto leaderMoveState = m_Leader->GetIntelligence()->GetMoveStateFromGoToTask();
    const auto leaderIsMoving  = leaderMoveState == PEDMOVE_WALK || leaderMoveState == PEDMOVE_RUN || leaderMoveState == PEDMOVE_SPRINT;

    dword20 = CalculateOffsetPosition();

    auto pedToLeader = m_Leader->GetPosition() - ped->GetPosition();
    pedToLeader.z    = 0.0f;
    const auto pedToLeaderDistSq = pedToLeader.SquaredMagnitude();

    // Keep up with the leader, the pace of the walk is changed for it
    auto speedAdjusted = false;
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY) {
        const auto tooManyFollowers = ped->GetIntelligence()->m_AnotherStaticCounter > 8;
        if (m_Flags & 4) {
            auto* const seekTask = static_cast<CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>*>(m_pSubTask);
            seekTask->GetSeekPosCalculator().SetOffset(dword20);
            seekTask->SetEntityMinDist2D(2.0f);

            auto* const goToPoint   = static_cast<CTaskSimpleGoToPoint*>(ped->GetIntelligence()->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_GO_TO_POINT));
            auto* const followRoute = ped->GetIntelligence()->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_FOLLOW_NODE_ROUTE);
            if (goToPoint && !followRoute) {
                auto target   = m_Leader->GetPosition() + dword20;
                auto toTarget = target - ped->GetPosition();
                toTarget.z    = 0.0f;

                if (leaderIsMoving) {
                    // The ped is behind where it should be, the pace is matched to get there
                    const auto& leaderFwd = m_Leader->GetForward();
                    if (DotProduct(ped->GetPosition(), leaderFwd) + -DotProduct(target, leaderFwd) < 0.0f) {
                        const auto radius = goToPoint->m_fRadius;
                        if (toTarget.SquaredMagnitude() <= (radius + 1.0f) * (radius + 1.0f)) {
                            target += leaderFwd * 2.0f;
                            if (walkAnim) {
                                walkAnim->m_Speed = std::max(0.85f, walkAnim->m_Speed - 0.0125f);
                                speedAdjusted     = true;
                            }
                        } else if (walkAnim) {
                            const auto oldSpeed = walkAnim->m_Speed;
                            ped->SetMoveAnimSpeed(walkAnim);
                            const auto newSpeed = walkAnim->m_Speed;
                            if (std::abs(oldSpeed - newSpeed) < 0.0130f /* 0x3C54FDF4 */) {
                                walkAnim->m_Speed = oldSpeed;
                            } else if (oldSpeed <= newSpeed) {
                                walkAnim->m_Speed = oldSpeed + 0.0125f;
                            } else {
                                walkAnim->m_Speed = oldSpeed - 0.0125f;
                            }
                            speedAdjusted = true;
                        }
                    }
                }
                goToPoint->UpdatePoint(target, 0.5f, false);
            }

            // Nobody else needs to follow when there are too many, so just stand around
            if (tooManyFollowers && pedToLeaderDistSq < 64.0f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                dword20 = pedToLeader;
                return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
            }
        }
    }
    ped->bMoveAnimSpeedHasBeenSetByTask = speedAdjusted;

    // The animations of the gang are kept in memory while they are used
    if (m_Flags & 1) {
        if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
            m_Flags &= ~1;
        }
    } else if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        const auto blockIdx = CAnimManager::GetAnimationBlockIndex("gangs");
        if (CAnimManager::ms_aAnimBlocks[blockIdx].IsLoaded) {
            CAnimManager::AddAnimBlockRef(blockIdx);
            m_Flags |= 1;
        } else {
            CStreaming::RequestModel(RESOURCE_ID_IFP + blockIdx, STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (!m_PedGroup->m_bIsMissionGroup && !(m_Flags & 8) && m_PedGroup->GetMembership().CountMembers() > 3 && CGeneral::GetRandomNumberInRange(0, 2000) == 500) {
        m_Flags |= 2;
    }

    if (m_ExhaleTimer.m_bStarted && m_ExhaleTimer.IsOutOfTime()) {
        if (const auto clump = ped->m_pRwClump; clump && RpClumpGetFrame(clump)) {
            if (auto* const fx = g_fxMan.CreateFxSystem("exhale", CVector{ 0.0f, 0.1f, 0.0f }, RwFrameGetMatrix(RpClumpGetFrame(clump)), false)) {
                fx->AttachToBone(ped, BONE_HEAD);
                fx->PlayAndKill();
            }
            m_ExhaleTimer.m_bStarted = false;
        }
    }

    if (!ped->IsVisible()) {
        return m_pSubTask;
    }

    // Now and then look at someone from the gang
    if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 95) {
        const auto lookTime   = CGeneral::GetRandomNumberInRange(3000, 5000);
        auto*      lookTarget = m_PedGroup->GetMembership().GetMember(CGeneral::GetRandomNumberInRange(0, 8));
        if (lookTarget == ped) {
            lookTarget = m_Leader;
        }
        if (lookTarget) {
            g_ikChainMan.LookAt("Task", ped, lookTarget, lookTime, BONE_HEAD, nullptr, true, 0.15f, 500, 3, false);
        }
    }

    if (!(m_Flags & 1) || ped->m_nMoveState >= PEDMOVE_RUN) {
        return m_pSubTask;
    }

    if (auto* const held = ped->GetEntityThatThisPedIsHolding()) {
        const auto* const animA = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, 0x12B);
        const auto* const animB = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, 0x12C);
        const auto* const animC = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, 0x12D);
        const auto* const animD = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, 0x12E);
        const auto        usingHeldObject = animA || animB || animC || animD;

        if (usingHeldObject) {
            if (g_ikChainMan.IsLooking(ped)) {
                g_ikChainMan.AbortLookAt(ped, 250);
            }
            ped->bDontAcceptIKLookAts = true;
            if ((animB && animB->m_CurrentTime < 0.5f) || (animD && animD->m_CurrentTime < 0.5f)) {
                m_ExhaleTimer.StartIfNotAlready(2700);
            }
        }

        if (ped->GetIntelligence()->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_PASS_OBJECT)) {
            return m_pSubTask;
        }

        if (CGeneral::GetRandomNumberInRange(0, 500) == 200) {
            // Offer what is held to a member of the gang that has nothing
            if (usingHeldObject) {
                return m_pSubTask;
            }
            auto* const friendPed = CTaskComplexGangLeader::TryToPassObject(ped, m_PedGroup);
            if (!friendPed || friendPed->GetEntityThatThisPedIsHolding() || friendPed->GetActiveWeapon().m_Type != WEAPON_UNARMED) {
                return m_pSubTask;
            }

            if (held->m_nModelIndex == MI_GANG_DRINK) {
                if (CGeneral::GetRandomNumberInRange(0, 500) < 250) {
                    friendPed->Say(CTX_GLOBAL_BOOZE_REQUEST);
                } else {
                    ped->Say(CTX_GLOBAL_BOOZE_RECEIVE, 1500);
                }
            } else if (held->m_nModelIndex == MI_GANG_SMOKE) {
                if (CGeneral::GetRandomNumberInRange(0, 500) < 250) {
                    friendPed->Say(CTX_GLOBAL_SPLIFF_REQUEST);
                } else {
                    ped->Say(CTX_GLOBAL_SPLIFF_RECEIVE, 1500);
                }
            }

            CEventPassObject event{ ped, false };
            friendPed->GetIntelligence()->GetEventGroup().Add(&event, false);
            return new CTaskComplexPassObject{ friendPed, true };
        }

        if (CGeneral::GetRandomNumberInRange(0, 100) == 50) {
            if (auto* const holdTask = static_cast<CTaskSimpleHoldEntity*>(ped->GetIntelligence()->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY))) {
                holdTask->PlayAnim(CTaskComplexGangLeader::GetRandomGangAmbientAnim(ped, held), ANIM_GROUP_GANGS);
            }
        }
        return m_pSubTask;
    }

    // Nothing is held, so gesture or say something
    if (ped->IsPlayingHandSignal() || ped->GetIntelligence()->m_TaskMgr.GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        return m_pSubTask;
    }

    const auto SaySomething = [ped] {
        if (ped->m_nMoveState != PEDMOVE_STILL) {
            return;
        }
        const auto roll = CGeneral::GetRandomNumberInRange(0, 10);
        if (roll <= 2) {
            ped->Say(CTX_GLOBAL_CHAT);
        } else if (roll <= 7) {
            ped->Say(CTX_GLOBAL_PCONV_GREET_MALE);
        } else if (roll == 8) {
            ped->Say(CTX_GLOBAL_BOOZE_REQUEST);
        } else {
            ped->Say(CTX_GLOBAL_SPLIFF_REQUEST);
        }
    };

    const auto roll = CGeneral::GetRandomNumberInRange(0, 500);
    if (roll > 50 && roll < 56) {
        const auto animId = (AnimationId)(0x117 + (int32)((double)(rand() & 0xFFFF) * (double)(1.0f / 32768.0f) * 8.0));
        ped->GetIntelligence()->m_TaskMgr.SetTaskSecondary(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, animId, 4.0f, false }, TASK_SECONDARY_PARTIAL_ANIM);
        SaySomething();
    } else if (roll == 100) {
        if (ped->m_nMoveState == PEDMOVE_STILL) {
            ped->GetIntelligence()->m_TaskMgr.SetTaskSecondary(new CTaskComplexPlayHandSignalAnim{ ANIM_ID_UNDEFINED, 4.0f }, TASK_SECONDARY_PARTIAL_ANIM);
            SaySomething();
        }
    }
    return m_pSubTask;
}
