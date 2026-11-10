#include "StdInc.h"

#include "TaskComplexGoPickUpEntity.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleHoldEntity.h"
#include "TaskSimplePickUpEntity.h"
#include "Entity/Ped/PlayerPed.h"
#include "Camera.h"
#include "Pad.h"

#include <numbers>

void CTaskComplexGoPickUpEntity::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGoPickUpEntity, 0x870B98, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6919C0);
    RH_ScopedInstall(Destructor, 0x691A50);

    RH_ScopedVMTInstall(Clone, 0x692C80);
    RH_ScopedVMTInstall(GetTaskType, 0x691A40);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x691AE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x693610);
    RH_ScopedVMTInstall(ControlSubTask, 0x691D50);
}

// 0x6919C0
CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(CEntity* entity, AssocGroupId animGroup) :
    CTaskComplex{},
    m_pEntity{entity},
    m_vecPosition{-999.0f, 0.0f, 0.0f},        // 0xC479C000, not set yet
    m_vecPickupPosition{-999.0f, 0.0f, 0.0f},  // 0xC479C000, not set yet
    m_nSubTaskCreatedTimeMs{0},
    m_nAnimGroupId{static_cast<int32>(animGroup)},
    m_bAnimBlockReferenced{false}
{
    CEntity::SafeRegisterRef(m_pEntity);
}

CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(const CTaskComplexGoPickUpEntity& o) :
    CTaskComplexGoPickUpEntity{o.m_pEntity, (AssocGroupId)o.m_nAnimGroupId}
{
}

// 0x691A50
CTaskComplexGoPickUpEntity::~CTaskComplexGoPickUpEntity() {
    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bAnimBlockReferenced) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex((AssocGroupId)m_nAnimGroupId));
        m_bAnimBlockReferenced = false;
    }
}

// 0x691AE0
CTask* CTaskComplexGoPickUpEntity::CreateNextSubTask(CPed* ped) {
    if (!m_pEntity) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        // Arrived next to the entity, so face it
        const auto toEntity = m_pEntity->GetPosition() - ped->GetPosition();
        return new CTaskSimpleAchieveHeading{ std::atan2(-toEntity.x, toEntity.y), 1.0f, 0.001f };
    }
    case TASK_SIMPLE_ACHIEVE_HEADING: {
        // The carry animation is picked by how high the entity is compared to the ped
        float movePedUntilAnimProgress = 0.6f;
        if (m_nAnimGroupId == ANIM_GROUP_CARRY) {
            const auto entityBottomZ = m_pEntity->GetColModel()->m_boundBox.m_vecMin.z + m_pEntity->GetPosition().z;
            if (entityBottomZ > ped->GetPosition().z) {
                m_nAnimGroupId           = ANIM_GROUP_CARRY105;
                movePedUntilAnimProgress = 0.2f;
            } else if (ped->GetPosition().z < 0.55f + entityBottomZ) {
                m_nAnimGroupId           = ANIM_GROUP_CARRY05;
                movePedUntilAnimProgress = 0.26666668f;
            }
        }

        auto* const task = new CTaskSimplePickUpEntity(m_pEntity, &m_vecPosition, PED_NODE_RIGHT_HAND, HOLD_ENTITY_FLAG_1, ANIM_ID_LIFTUP, (AssocGroupId)m_nAnimGroupId, movePedUntilAnimProgress);
        task->m_vecPickuposn = m_vecPickupPosition;
        return task;
    }
    case TASK_SIMPLE_PICKUP_ENTITY: {
        // The entity is picked up, so the ped carries it with a secondary task and this task is done
        m_nSubTaskCreatedTimeMs = CTimer::GetTimeInMS();

        auto& taskMgr = ped->GetTaskManager();
        if (!taskMgr.GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
            taskMgr.SetTaskSecondary(
                new CTaskSimpleHoldEntity(m_pEntity, &m_vecPosition, PED_NODE_RIGHT_HAND, HOLD_ENTITY_FLAG_1, ANIM_ID_CRRY_PRTIAL, ANIM_GROUP_CARRY, false),
                TASK_SECONDARY_PARTIAL_ANIM
            );
        }
        return nullptr;
    }
    default:
        return nullptr;
    }
}

// 0x693610
CTask* CTaskComplexGoPickUpEntity::CreateFirstSubTask(CPed* ped) {
    if (!m_pEntity) {
        return nullptr;
    }

    m_nSubTaskCreatedTimeMs = CTimer::GetTimeInMS();

    const auto  pedToEntity = ped->GetPosition() - m_pEntity->GetPosition();
    const auto& bounds      = CModelInfo::GetModelInfo(m_pEntity->m_nModelIndex)->GetColModel()->m_boundBox;

    m_vecPosition = CVector{ -0.2f, -bounds.m_vecMin.y - 0.2f, -bounds.m_vecMin.z };

    // Turns the entity so its short side faces the ped, or stands it up when it is not upright.
    // The pi of the exe is the exact float, the PI of common.h is shorter.
    constexpr float EXE_HALF_PI = std::numbers::pi_v<float> / 2.0f;
    if (bounds.m_vecMax.y * 2.0f < bounds.m_vecMax.x) {
        m_pEntity->SetHeading(std::atan2(-pedToEntity.x, pedToEntity.y));
    } else if (bounds.m_vecMax.x * 2.0f < bounds.m_vecMax.y) {
        m_pEntity->SetHeading(std::atan2(-pedToEntity.x, pedToEntity.y) - EXE_HALF_PI);
    } else if (m_pEntity->GetMatrix().GetUp().z < 0.9f) {
        const auto& forward = m_pEntity->GetMatrix().GetForward();
        m_pEntity->SetHeading(std::atan2(-forward.x, forward.y));
    }

    // Objects must not fall or slide away while the ped walks to them
    if (m_pEntity->GetIsTypeObject()) {
        auto* const physical = m_pEntity->AsPhysical();
        physical->m_vecMoveSpeed = CVector{};
        physical->m_vecTurnSpeed = CVector{};
        m_pEntity->SetIsStatic(true);
    }
    m_pEntity->UpdateRwMatrix();

    // Which side of the entity the ped is on
    const auto& entityRight   = m_pEntity->GetMatrix().GetRight();
    const auto  localX        = pedToEntity.z * entityRight.z + pedToEntity.y * entityRight.y + pedToEntity.x * entityRight.x;
    const auto& entityForward = m_pEntity->GetMatrix().GetForward();
    const auto  localY        = pedToEntity.z * entityForward.z + pedToEntity.y * entityForward.y + pedToEntity.x * entityForward.x;

    constexpr float PICKUP_DIST = 0.4f; // From the side of the bounding box
    if (localX > std::abs(localY)) {
        m_vecPickupPosition.x = PICKUP_DIST + bounds.m_vecMax.x;
        m_vecPickupPosition.y = 0.0f;
    } else if (-std::abs(localY) > localX) {
        m_vecPickupPosition.x = bounds.m_vecMin.x - PICKUP_DIST;
        m_vecPickupPosition.y = 0.0f;
    } else {
        m_vecPickupPosition.x = 0.0f;
        m_vecPickupPosition.y = localY > 0.0f ? PICKUP_DIST + bounds.m_vecMax.y : bounds.m_vecMin.y - PICKUP_DIST;
    }
    m_vecPickupPosition.z = bounds.m_vecMin.z + 1.0f;

    const auto pickupWorldPos = m_pEntity->GetMatrix().TransformPoint(m_vecPickupPosition);
    return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, pickupWorldPos, 0.2f, 0.0f, false, true };
}

// 0x691D50
CTask* CTaskComplexGoPickUpEntity::ControlSubTask(CPed* ped) {
    // The animation block of the pick up animation is kept in memory while the task runs
    if (m_nAnimGroupId != ANIM_GROUP_DEFAULT && !m_bAnimBlockReferenced) {
        auto* animBlock = CAnimManager::GetAnimationBlock((AssocGroupId)m_nAnimGroupId);
        if (!animBlock) {
            animBlock = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName((AssocGroupId)m_nAnimGroupId));
        }

        const auto animBlockIdx = CAnimManager::GetAnimationBlockIndex(animBlock);
        if (animBlock->IsLoaded) {
            CAnimManager::AddAnimBlockRef(animBlockIdx);
            m_bAnimBlockReferenced = true;
        } else {
            CStreaming::RequestModel(RESOURCE_ID_IFP + animBlockIdx, STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (!m_pSubTask) {
        return m_pSubTask;
    }

    // Gives up on the sub task when it takes too long
    const auto GiveUp = [&] {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        return nullptr;
    };

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PICKUP_ENTITY: {
        const auto* const pickUp = static_cast<CTaskSimplePickUpEntity*>(m_pSubTask);
        if (!pickUp->m_pAnimBlendAssociation && !pickUp->m_bEntityDropped && CTimer::GetTimeInMS() > m_nSubTaskCreatedTimeMs + 8000) {
            return GiveUp();
        }
        return m_pSubTask;
    }
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        if (CTimer::GetTimeInMS() > m_nSubTaskCreatedTimeMs + 5000) {
            return GiveUp();
        }

        // The player can cancel by jumping, sprinting or moving the stick away from the entity
        if (!ped->GetPlayerData()) {
            return m_pSubTask;
        }
        auto* const pad = static_cast<CPlayerPed*>(ped)->GetPadFromPlayer();
        if (!pad) {
            return m_pSubTask;
        }

        const auto stickUpDown    = (float)pad->GetPedWalkUpDown();
        const auto stickLeftRight = (float)pad->GetPedWalkLeftRight();
        const auto stickStrength  = std::sqrt(stickUpDown * stickUpDown + stickLeftRight * stickLeftRight) * (1.0f / 128.0f);

        const auto& goToTask   = *static_cast<CTaskComplexGoToPointAndStandStill*>(m_pSubTask);
        const auto& pedPos     = ped->GetPosition();
        const auto  targetAngle = std::atan2(-(goToTask.m_vecTargetPoint.x - pedPos.x), goToTask.m_vecTargetPoint.y - pedPos.y);

        // Brought to within half a turn of the angle to the target
        constexpr float EXE_PI = std::numbers::pi_v<float>;
        auto stickAngle = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -stickLeftRight, stickUpDown) - TheCamera.m_fOrientation;
        if (targetAngle + EXE_PI < stickAngle) {
            stickAngle -= EXE_PI * 2.0f;
        } else if (targetAngle - EXE_PI > stickAngle) {
            stickAngle += EXE_PI * 2.0f;
        }

        if (pad->JumpJustDown() || pad->SprintJustDown()) {
            return GiveUp();
        }
        if (stickStrength <= 0.75f) {
            return m_pSubTask;
        }
        // Only one side is checked, like in the exe
        if (stickAngle - targetAngle > EXE_PI / 4.0f) {
            return GiveUp();
        }
        return m_pSubTask;
    }
    default:
        return m_pSubTask;
    }
}
