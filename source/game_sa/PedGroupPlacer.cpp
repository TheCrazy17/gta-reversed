#include "StdInc.h"

#include "PedGroupPlacer.h"
#include "PedPlacement.h"
#include "Population.h"
#include "VisibilityPlugins.h"
#include "Tasks/Allocators/PedGroup/PedGroupDefaultTaskAllocators.h"
#include "Tasks/TaskTypes/TaskComplexBeInGroup.h"
#include "Tasks/TaskTypes/TaskComplexWanderGang.h"
#include "Tasks/TaskTypes/TaskComplexFollowLeaderInFormation.h"

void CPedGroupPlacer::InjectHooks() {
    RH_ScopedClass(CPedGroupPlacer);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(PlaceFormationGroup, 0x5FC9B0);
    RH_ScopedInstall(PlaceChatGroup, 0x5FCE80);
    RH_ScopedInstall(PlaceRandomGroup, 0x5FD330);
    RH_ScopedInstall(PlaceGroup, 0x5FD810);
}

namespace {
//! The models of the peds are the ones of the gang, and they have to be loaded
std::optional<eModelID> ChooseLoadedGangModel(ePedType type) {
    const auto model = CPopulation::ChooseGangOccupation((eGangID)(type - PED_TYPE_GANG1));
    if (!CModelInfo::GetModelInfo(model)->GetRwObject()) {
        return std::nullopt;
    }
    return model;
}

//! Nobody can see the group appear next to the player
bool IsPlacementSeenTooClose(const CVector& origin, float radius) {
    if (!TheCamera.IsSphereVisible(origin, radius)) {
        return false;
    }
    const auto& playerPos = FindPlayerPed(-1)->GetPosition();
    const auto  dist      = std::sqrt((origin.y - playerPos.y) * (origin.y - playerPos.y) + (origin.x - playerPos.x) * (origin.x - playerPos.x));
    return CPopulation::PedCreationDistMultiplier() * 42.5f > dist;
}

//! The ground that is under the position, if there is any
std::optional<float> FindGroundZ(const CVector& pos) {
    bool       found{};
    const auto z = CWorld::FindGroundZFor3DCoord({ pos.x, pos.y, pos.z + 1.0f }, &found) + 1.0f;
    return found ? std::optional{ z } : std::nullopt;
}

//! Sets what the group is made of, and what all of the peds do
void SetupGroup(int32 groupId, const CPedGroupDefaultTaskAllocator& allocator, std::span<CPed* const> peds) {
    auto& group = CPedGroups::ms_groups[groupId];
    group.GetIntelligence().SetDefaultTaskAllocator(allocator);
    group.GetMembership().SetLeader(peds.front());
    group.GetMembership().Process();
    group.GetIntelligence().Process();
    peds.front()->GetTaskManager().SetTask(new CTaskComplexBeInGroup{ groupId, true }, TASK_PRIMARY_PRIMARY);

    for (auto* const follower : peds.subspan(1)) {
        group.GetMembership().AddFollower(follower);
        group.GetMembership().Process();
        group.GetIntelligence().Process();
        follower->GetTaskManager().SetTask(new CTaskComplexBeInGroup{ groupId, false }, TASK_PRIMARY_PRIMARY);
    }
}

//! The peds of the circle are around the origin, and each one is a bit away from where it should be
float CircleRadius(uint32 numOfPeds, float& outAngleStep) {
    outAngleStep = TWO_PI / (float)(int32)numOfPeds;
    return std::sqrt(0.5f / (1.0f - std::cos(outAngleStep)));
}
CVector CircleOffset(uint32 idx, float angleStep, float radius) {
    const auto angle = (float)(int32)idx * angleStep + CGeneral::GetRandomNumberInRange(-0.2f, 0.2f) * angleStep;
    const auto dist  = radius + CGeneral::GetRandomNumberInRange(-0.2f, 0.2f) * radius;
    return { dist * std::cos(angle), dist * std::sin(angle), 0.0f };
}

//! Puts the peds in a circle, `Setup` is what is done with the ones that were placed
template<typename T_Setup>
bool PlaceCircleGroup(ePedType type, uint32 numOfPeds, const CVector& origin, T_Setup&& Setup) {
    if ((int32)numOfPeds < 2) {
        return false;
    }

    const auto groupId = CPedGroups::AddGroup();
    if (groupId < 0) {
        return false;
    }

    float      angleStep;
    const auto radius = CircleRadius(numOfPeds, angleStep);
    if (IsPlacementSeenTooClose(origin, radius)) {
        return false;
    }
    if (!CPedPlacement::IsPositionClearForPed(origin, radius, -1, nullptr, true, true, true)) {
        return false;
    }

    std::vector<CPed*> peds;
    CVector            firstPos{};
    for (auto i = 0u; i < (uint32)(int32)numOfPeds; i++) {
        const auto offset = CircleOffset(i, angleStep, radius);

        const auto groundZ = FindGroundZ({ origin.x + offset.x, origin.y + offset.y, origin.z });
        if (!groundZ) {
            continue;
        }
        const CVector pos{ origin.x + offset.x, origin.y + offset.y, *groundZ < origin.z ? origin.z : *groundZ };
        if (i == 0) {
            firstPos = pos;
        }

        const auto model = ChooseLoadedGangModel(type);
        if (!model) {
            continue;
        }

        // The ped has to fit where it is, it can only be in touch with the ones that were placed
        std::array<CEntity*, 9> hitEntities{};
        CPedPlacement::IsPositionClearForPed(pos, CModelInfo::GetModelInfo(*model)->GetColModel()->GetBoundRadius(), 9, hitEntities.data(), true, true, true);
        const auto isBlocked = rng::any_of(hitEntities, [&](CEntity* hit) {
            return hit && rng::find(peds, hit) == peds.end();
        });
        if (isBlocked) {
            continue;
        }

        // The rest can be seen from the first one, and are not too far from its height
        if (i != 0) {
            const auto canSeeFirst = CWorld::GetIsLineOfSightClear(pos, firstPos, true, false, false, false, false, false, false);
            if (!canSeeFirst || !(std::abs(pos.z - firstPos.z) < 1.0f)) {
                continue;
            }
        }

        auto* const ped = CPopulation::AddPed(type, *model, pos, false);
        if (!ped) {
            continue;
        }
        peds.push_back(ped);
        ped->m_fCurrentRotation = ped->m_fAimingRotation = CGeneral::GetRadianAngleBetweenPoints(origin.x, origin.y, pos.x, pos.y);
        CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);
    }

    if (peds.empty()) {
        return false;
    }
    Setup(groupId, std::span<CPed* const>{ peds });
    return true;
}
} // namespace

// 0x5FC9B0
bool CPedGroupPlacer::PlaceFormationGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    ZoneScoped;

    const auto groupId = CPedGroups::AddGroup();
    if (groupId < 0) {
        return false;
    }

    if (IsPlacementSeenTooClose(origin, 3.0f)) {
        return false;
    }
    if (!CPedPlacement::IsPositionClearForPed(origin, 3.0f, -1, nullptr, true, true, true)) {
        return false;
    }

    const auto groundZ = FindGroundZ(origin);
    if (!groundZ) {
        return false;
    }
    const auto spawnZ = *groundZ < origin.z ? origin.z : *groundZ;

    // The first one is the leader
    const auto leaderModel = ChooseLoadedGangModel(type);
    if (!leaderModel) {
        return false;
    }
    auto* const leader = CPopulation::AddPed(type, *leaderModel, { origin.x, origin.y, spawnZ }, false);
    if (!leader) {
        return false;
    }

    std::vector<CPed*> peds{ leader };
    for (auto i = 1; i < (int32)numOfPeds; i++) {
        const auto model = ChooseLoadedGangModel(type);
        if (!model) {
            continue;
        }
        auto* const follower = CPopulation::AddPed(type, *model, { origin.x, origin.y, spawnZ }, false);
        if (!follower) {
            continue;
        }

        // Where it stands in the formation, on the ground and close to the height of the leader
        const auto& offset = CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[i];
        follower->SetPosn(leader->GetPosition() + CVector{ offset.x, offset.y, 0.0f });

        const auto followerPos = follower->GetPosition();
        const auto followerGroundZ = FindGroundZ(followerPos);
        const auto z               = followerGroundZ && followerPos.z <= *followerGroundZ ? *followerGroundZ : followerPos.z;
        if (!followerGroundZ
            || std::abs(z - leader->GetPosition().z) > 1.0f
            || !CWorld::GetIsLineOfSightClear({ followerPos.x, followerPos.y, z }, leader->GetPosition(), true, false, false, false, false, false, false)
        ) {
            CPopulation::RemovePed(follower);
            continue;
        }

        follower->SetPosn(followerPos.x, followerPos.y, z);
        peds.push_back(follower);
        CVisibilityPlugins::SetClumpAlpha(follower->GetRpClump(), 0);
    }

    SetupGroup(groupId, CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::FOLLOW_ANY_MEANS), peds);
    return true;
}

// 0x5FCE80
bool CPedGroupPlacer::PlaceChatGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    ZoneScoped;

    return PlaceCircleGroup(type, numOfPeds, origin, [](int32 groupId, std::span<CPed* const> peds) {
        SetupGroup(groupId, CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::STAND_STILL), peds);
    });
}

// 0x5FD330
bool CPedGroupPlacer::PlaceRandomGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    ZoneScoped;

    return PlaceCircleGroup(type, numOfPeds, origin, [](int32 groupId, std::span<CPed* const> peds) {
        auto& group = CPedGroups::ms_groups[groupId];
        group.GetIntelligence().SetDefaultTaskAllocator(CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::RANDOM));
        for (auto&& [i, ped] : rngv::enumerate(peds)) {
            if (i == 0) {
                group.GetMembership().SetLeader(ped);
            } else {
                group.GetMembership().AddFollower(ped);
            }
            group.GetMembership().Process();
            group.GetIntelligence().Process();

            // Each one wanders by its own, but they are still in the group
            const auto dir = (uint8)(int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 8.0f);
            ped->GetTaskManager().SetTask(new CTaskComplexWanderGang{ PEDMOVE_WALK, dir, 5000u, true, 0.5f }, TASK_PRIMARY_DEFAULT);
            ped->GetTaskManager().SetTask(new CTaskComplexBeInGroup{ groupId, false }, TASK_PRIMARY_PRIMARY);
        }
    });
}

// 0x5FD810
bool CPedGroupPlacer::PlaceGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType allocType) {
    using enum ePedGroupDefaultTaskAllocatorType;
    switch (allocType) {
    case FOLLOW_ANY_MEANS:
    case FOLLOW_LIMITED:   return PlaceFormationGroup(type, numOfPeds, origin, allocType);
    case STAND_STILL:
    case CHAT:             return PlaceChatGroup(type, numOfPeds, origin, allocType);
    case RANDOM:           return PlaceRandomGroup(type, numOfPeds, origin, allocType);
    default:               NOTSA_UNREACHABLE();
    }
}
