/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "PlayerPed.h"
#include "TagManager.h"
#include "PedClothesDesc.h"
#include "PedStats.h"
#include "TaskSimpleUseGun.h"
#include "TaskSimpleFight.h"
#include "Clothes.h"
#include "VisibilityPlugins.h"
#include "WeaponModelInfo.h"
#include "PedModelInfo.h"
#include "Animation/AnimManager.h"
#include "EntryExitManager.h"
#include "MBlur.h"
#include "GangWars.h"
#include "Radar.h"
#include "Stats.h"
#include "Cheat.h"
#include "TaskComplexBeInGroup.h"
#include "EventScriptCommand.h"
#include "EventGroupEvent.h"
#include "EventNewGangMember.h"
#include "EventDontJoinPlayerGroup.h"
#include "EventPlayerCommandToGroupAttack.h"
#include "EventPlayerCommandToGroupGather.h"
#include "TaskSimplePlayerOnFoot.h"
#include "TaskComplexFacial.h"
#include "Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include "Fire.h"
#include "Darkel.h"
#include "GameLogic.h"
#include "AEPedSpeechAudioEntity.h"
#include "EventGunAimedAt.h"
#include "TaskComplexSmartFleeEntity.h"
#include "CarCtrl.h"

bool CPlayerPed::bDebugPlayerInvincible;
bool CPlayerPed::bDebugTargeting;
bool CPlayerPed::bDebugTapToTarget;

void CPlayerPed::InjectHooks() {
    RH_ScopedVirtualClass(CPlayerPed, 0x86D168, 26);
    RH_ScopedCategory("Entity/Ped");

    RH_ScopedInstall(ResetSprintEnergy, 0x60A530);
    RH_ScopedInstall(ResetPlayerBreath, 0x60A8A0);
    RH_ScopedInstall(RemovePlayerPed, 0x6094A0);
    RH_ScopedInstall(Busted, 0x609EF0);
    RH_ScopedInstall(GetWantedLevel, 0x41BE60);
    RH_ScopedInstall(SetWantedLevel, 0x609F10);
    RH_ScopedInstall(SetWantedLevelNoDrop, 0x609F30);
    RH_ScopedInstall(CheatWantedLevel, 0x609F50);
    RH_ScopedInstall(DoStuffToGoOnFire, 0x60A020);
    RH_ScopedVMTInstall(Load, 0x5D46E0);
    RH_ScopedVMTInstall(Save, 0x5D57E0);
    RH_ScopedInstall(DeactivatePlayerPed, 0x609520);
    RH_ScopedInstall(ReactivatePlayerPed, 0x609540);
    RH_ScopedInstall(GetPadFromPlayer, 0x609560);
    RH_ScopedInstall(CanPlayerStartMission, 0x609590);
    RH_ScopedInstall(IsHidden, 0x609620);
    RH_ScopedInstall(ReApplyMoveAnims, 0x609650);
    RH_ScopedInstall(DoesPlayerWantNewWeapon, 0x609710);
    RH_ScopedInstall(ProcessPlayerWeapon, 0x6097F0);
    RH_ScopedInstall(PickWeaponAllowedFor2Player, 0x609800);
    RH_ScopedInstall(UpdateCameraWeaponModes, 0x609830);
    RH_ScopedInstall(ClearWeaponTarget, 0x609c80);
    RH_ScopedInstall(GetWeaponRadiusOnScreen, 0x609CD0);
    RH_ScopedInstall(PedCanBeTargettedVehicleWise, 0x609D90);
    RH_ScopedInstall(FindTargetPriority, 0x609DE0);
    RH_ScopedInstall(Clear3rdPersonMouseTarget, 0x609ED0);
    RH_ScopedInstall(CanIKReachThisTarget, 0x609F80);
    RH_ScopedInstall(GetPlayerInfoForThisPlayerPed, 0x609FF0);
    RH_ScopedInstall(AnnoyPlayerPed, 0x60A040);
    RH_ScopedInstall(ClearAdrenaline, 0x60A070);
    RH_ScopedInstall(DisbandPlayerGroup, 0x60A0A0);
    RH_ScopedInstall(MakeGroupRespondToPlayerTakingDamage, 0x60A110);
    RH_ScopedInstall(TellGroupToStartFollowingPlayer, 0x60A1D0);
    RH_ScopedInstall(MakePlayerGroupDisappear, 0x60A440);
    RH_ScopedInstall(MakePlayerGroupReappear, 0x60A4B0);
    RH_ScopedInstall(HandleSprintEnergy, 0x60A550);
    RH_ScopedInstall(GetButtonSprintResults, 0x60A820);
    RH_ScopedInstall(HandlePlayerBreath, 0x60A8D0);
    RH_ScopedInstall(SetRealMoveAnim, 0x60A9C0);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "", 0x60B460, void(CPlayerPed::*)(eWeaponType));
    RH_ScopedGlobalInstall(LOSBlockedBetweenPeds, 0x60B550);
    RH_ScopedInstall(DoesTargetHaveToBeBroken, 0x60C0C0);
    RH_ScopedInstall(SetPlayerMoveBlendRatio, 0x60C520);
    RH_ScopedInstall(FindPedToAttack, 0x60C5F0);
    RH_ScopedInstall(ForceGroupToAlwaysFollow, 0x60C7C0);
    RH_ScopedInstall(ForceGroupToNeverFollow, 0x60C800);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "BySlot", 0x60D000, void(CPlayerPed::*)(uint32));
    RH_ScopedInstall(EvaluateTarget, 0x60D020);
    RH_ScopedInstall(PlayerHasJustAttackedSomeone, 0x60D5A0);
    RH_ScopedInstall(SetupPlayerPed, 0x60D790);

    RH_ScopedVMTInstall(ProcessControl, 0x60EA90);
    RH_ScopedVMTInstall(SetMoveAnim, 0x609490);
    RH_ScopedInstall(ProcessAnimGroups, 0x6098F0);
    RH_ScopedInstall(ControlButtonSprint, 0x60A610);
    RH_ScopedInstall(Compute3rdPersonMouseTarget, 0x60B650);
    RH_ScopedInstall(DrawTriangleForMouseRecruitPed, 0x60BA80);
    RH_ScopedInstall(KeepAreaAroundPlayerClear, 0x60C1E0);
    RH_ScopedInstall(MakeThisPedJoinOurGroup, 0x60C840);
    RH_ScopedInstall(PlayerWantsToAttack, 0x60CC50);
    RH_ScopedInstall(SetInitialState, 0x60CD20);
    RH_ScopedInstall(EvaluateNeighbouringTarget, 0x60D1C0);
    RH_ScopedInstall(ProcessGroupBehaviour, 0x60D350);
    RH_ScopedInstall(ProcessWeaponSwitch, 0x60D850);
    RH_ScopedInstall(FindWeaponLockOnTarget, 0x60DC50, { .Reversed = false });
    RH_ScopedInstall(FindNextWeaponLockOnTarget, 0x60E530, { .Reversed = false });
}

// TODO: To class and create 2 function
struct CPlayerPedDataSaveStructure {
    uint32          ChaosLevel{};
    eWantedLevel    WantedLevel{};
    CPedClothesDesc ClothesDesc{};
    uint32          ChosenWeapon{};
    // float          Multiplier{}; // Mobile
};

VALIDATE_SIZE(CPlayerPedDataSaveStructure, 0x84 /* + 0x4 */);

// 0x5D46E0
bool CPlayerPed::Load() {
    CPed::Load();

    CGenericGameStorage::LoadDataFromWorkBuffer<uint32>(); // Discard structure size
    auto sd = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerPedDataSaveStructure>();

    CWanted* wanted = GetPlayerWanted();
    wanted->m_ChaosLevel = sd.ChaosLevel;
    wanted->m_WantedLevel= sd.WantedLevel;

    *GetPlayerData()->m_pPedClothesDesc = sd.ClothesDesc;
    GetPlayerData()->m_nChosenWeapon   = sd.ChosenWeapon;

    return true;
}

// 0x5D57E0
bool CPlayerPed::Save() {
    CPlayerPedDataSaveStructure saveData{};

    CWanted* wanted = GetPlayerWanted();
    saveData.ChaosLevel = wanted->m_ChaosLevel;
    saveData.WantedLevel = wanted->m_WantedLevel;
    saveData.ChosenWeapon = GetPlayerData()->m_nChosenWeapon;
    saveData.ClothesDesc  = *GetPlayerData()->m_pPedClothesDesc;

    CPed::Save();
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerPedDataSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(saveData);

    return true;
}

// 0x60D5B0
CPlayerPed::CPlayerPed(int32 playerId, bool bGroupCreated) : CPed(PED_TYPE_PLAYER1) {
    m_pPlayerData = &CWorld::Players[playerId].m_PlayerData;
    GetPlayerData()->AllocateData();

    CPed::SetModelIndex(MODEL_PLAYER);

    CPlayerPed::SetInitialState(bGroupCreated);

    CEntity::ClearReference(m_pTargetedObject);

    SetPedState(PEDSTATE_IDLE);

    gPlayIdlesAnimBlockIndex = CAnimManager::GetAnimationBlockIndex("playidles");

    if (!bGroupCreated) {
        GetPlayerData()->m_nPlayerGroup = CPedGroups::AddGroup();

        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
        group.m_bIsMissionGroup = true;
        group.m_groupMembership.SetLeader(this);
        group.Process();

        GetPlayerData()->m_bGroupStuffDisabled = false;
        GetPlayerData()->m_bGroupAlwaysFollow  = false;
        GetPlayerData()->m_bGroupNeverFollow   = false;
    }

    m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    m_fHealth    = m_fMaxHealth;

    m_nFightingStyle      = STYLE_GRAB_KICK;
    m_nAllowedAttackMoves = 15;

    m_p3rdPersonMouseTarget = nullptr;
    field_7A0 = 0;
    m_pedSpeech.Initialise(this);
    GetIntelligence()->m_fDmRadius = 30.0f;
    GetIntelligence()->m_nDmNumPedsToScan = 2;

    bUsedForReplay = bGroupCreated;
}

// 0x6094A0
void CPlayerPed::RemovePlayerPed(int32 playerId) {
    CPed* player = FindPlayerPed(playerId);
    CPlayerInfo* playerInfo = &FindPlayerInfo(playerId);
    if (player)
    {
        CVehicle* playerVehicle = player->m_pVehicle;
        if (playerVehicle && playerVehicle->m_pDriver == player)
        {
            playerVehicle->SetStatus(STATUS_PHYSICS);
            playerVehicle->m_GasPedal = 0.0f;
            playerVehicle->m_BrakePedal = 0.1f;
        }
        CWorld::Remove(static_cast<CEntity*>(player));
        delete player;
        playerInfo->m_pPed = nullptr;
    }
}

// 0x609520
void CPlayerPed::DeactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Remove(FindPlayerPed(playerId));
}

// 0x609540
void CPlayerPed::ReactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Add(FindPlayerPed(playerId));
}

// 0x609560
CPad* CPlayerPed::GetPadFromPlayer() const {
    switch (m_nPedType) {
    case PED_TYPE_PLAYER1:
        return CPad::GetPad(0);

    case PED_TYPE_PLAYER2:
        return CPad::GetPad(1);
    }
    assert(0); // shouldn't happen
    return nullptr;
}

// 0x609590
bool CPlayerPed::CanPlayerStartMission() {
    if (CGameLogic::GameState != GAMELOGIC_STATE_PLAYING || CGameLogic::IsCoopGameGoingOn())
        return false;

    if (!IsPedInControl() && !IsStateDriving())
        return false;

    auto& taskMgr = GetTaskManager();
    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_PHYSICAL_RESPONSE))
        return false;

    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_NONTEMP))
        return false;

    auto primaryTask = taskMgr.GetTaskPrimary(TASK_PRIMARY_PRIMARY);
    if (primaryTask != nullptr && primaryTask->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK))
        return false;

    if (!IsAlive())
        return false;

    return !GetEventGroup().GetEventOfType(EVENT_SCRIPT_COMMAND);
}

// 0x609620
bool CPlayerPed::IsHidden() {
    return bInVehicle && GetLightingTotal() <= 0.05f;
}

// 0x609650
void CPlayerPed::ReApplyMoveAnims() {
    constexpr AnimationId anims[]{
        ANIM_ID_WALK,
        ANIM_ID_RUN,
        ANIM_ID_SPRINT,
        ANIM_ID_IDLE,
        ANIM_ID_WALK_START
    };
    for (const AnimationId& id : anims) {
        if (CAnimBlendAssociation* anim = RpAnimBlendClumpGetAssociation(GetRpClump(), id)) {
            if (anim->GetHashKey() != CAnimManager::GetAnimAssociation(m_nAnimGroup, id)->GetHashKey()) {
                CAnimBlendAssociation* addedAnim = CAnimManager::AddAnimation(GetRpClump(), m_nAnimGroup, id);
                addedAnim->m_BlendDelta = anim->m_BlendDelta;
                addedAnim->m_BlendAmount = anim->m_BlendAmount;

                anim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                anim->m_BlendDelta = -1000.0f;
            }
        }
    }
}

// 0x609710
bool CPlayerPed::DoesPlayerWantNewWeapon(eWeaponType weaponType, bool arg1) {
    // GetPadFromPlayer(); // Called, but not used

    auto weaponSlot = GetWeaponSlot(weaponType);
    auto weaponInSlotType = GetWeaponInSlot(weaponSlot).m_Type;
    if (weaponInSlotType == weaponType)
        return true;

    if (weaponInSlotType == eWeaponType::WEAPON_UNARMED)
        return true;

    if (arg1)
        return false;

    if (GetIntelligence()->GetTaskJetPack())
        return false;

    /* !See comment!
    if (m_nActiveWeaponSlot == weaponSlot
        && CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm   \ One of these two is always false, so
        && !CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm) / the whole expression is always false
        return false;
    */

    if (m_nActiveWeaponSlot == weaponSlot) {
        switch (m_nPedState) {
        case PEDSTATE_ATTACK:
        case PEDSTATE_AIMGUN:
            return false;
        }
    }

    return true;
}

// 0x6097F0
void CPlayerPed::ProcessPlayerWeapon(CPad* pad) {
    /* empty */
}

// 0x609800
void CPlayerPed::PickWeaponAllowedFor2Player() {
    if (!GetWeaponInSlot(GetPlayerData()->m_nChosenWeapon).CanBeUsedFor2Player()) {
        GetPlayerData()->m_nChosenWeapon = eWeaponType::WEAPON_UNARMED;
    }
}

// 0x609830
void CPlayerPed::UpdateCameraWeaponModes(CPad* pad) {
    switch (GetActiveWeapon().m_Type) {
    case eWeaponType::WEAPON_M4:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_M16_1STPERSON, 0, 0);
        break;

    case eWeaponType::WEAPON_SNIPERRIFLE:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_SNIPER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER_HS:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER_HS, 0, 0);
        break;

    case eWeaponType::WEAPON_CAMERA:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_CAMERA, 0, 0);
        break;

    default:
        TheCamera.ClearPlayerWeaponMode();
        break;
    }
}

// 0x6098F0
void CPlayerPed::ProcessAnimGroups() {
    auto* const playerData = GetPlayerData();

    // The player specific groups come in sets; the clothes pick the member of the set
    const auto GetGroupForBase = [](int32 base) -> AssocGroupId {
        return static_cast<AssocGroupId>(base - ANIM_GROUP_PLAYER + CClothes::GetDefaultPlayerMotionGroup());
    };
    const auto GetPlainPlayerGroup = [&]() -> AssocGroupId {
        if (m_nPedType == PED_TYPE_PLAYER2) {
            const auto modelGroup = static_cast<AssocGroupId>(CModelInfo::GetModelInfo(m_nModelIndex)->AsPedModelInfoPtr()->m_nAnimType);
            if (modelGroup != ANIM_GROUP_PLAYER) {
                return modelGroup;
            }
        }
        return GetGroupForBase(ANIM_GROUP_PLAYER);
    };

    const auto heading  = playerData->m_fFPSMoveHeading;
    const auto strafing = !(heading > -0.87266463f && heading < 0.87266463f)
        && TheCamera.GetActiveCam().Using3rdPersonMouseCam()
        && CanStrafeOrMouseControl();

    AssocGroupId newGroup;
    if (strafing) {
        newGroup = m_nAnimGroup == ANIM_GROUP_PLAYER ? GetPlainPlayerGroup() : GetGroupForBase(m_nAnimGroup);
    } else {
        // Type of the weapon whose model is held
        auto weaponType = WEAPON_UNARMED;
        if (m_pWeaponObject) {
            if (auto* const mi = CVisibilityPlugins::GetClumpModelInfo(m_pWeaponObject); mi && mi->GetModelType() == MODEL_INFO_WEAPON) {
                weaponType = reinterpret_cast<CWeaponModelInfo*>(mi)->GetWeaponInfo();
            }
        }

        if (weaponType == WEAPON_RLAUNCHER || weaponType == WEAPON_RLAUNCHER_HS) {
            newGroup = GetGroupForBase(ANIM_GROUP_PLAYERROCKET);
        } else if (GetIntelligence()->GetTaskJetPack()) {
            newGroup = ANIM_GROUP_PLAYERJETPACK;
        } else {
            switch (weaponType) {
            case WEAPON_BASEBALLBAT:
            case WEAPON_SHOVEL:
            case WEAPON_POOL_CUE:
                newGroup = GetGroupForBase(ANIM_GROUP_PLAYERBBBAT);
                break;
            case WEAPON_CHAINSAW:
            case WEAPON_FLAMETHROWER:
            case WEAPON_MINIGUN:
                newGroup = GetGroupForBase(ANIM_GROUP_PLAYERCSAW);
                break;
            case WEAPON_M4:
            case WEAPON_AK47:
            case WEAPON_SPAS12_SHOTGUN:
            case WEAPON_SHOTGUN:
            case WEAPON_SNIPERRIFLE:
            case WEAPON_COUNTRYRIFLE:
                newGroup = GetGroupForBase(ANIM_GROUP_PLAYER2ARMED);
                break;
            default:
                newGroup = playerData->m_pPedClothesDesc->GetIsWearingBalaclava() ? ANIM_GROUP_PLAYERSNEAK : GetPlainPlayerGroup();
                break;
            }
        }
    }
    if (m_nAnimGroup != newGroup) {
        m_nAnimGroup = newGroup;
        ReApplyMoveAnims();
    }

    // Keep the animation blocks of the melee styles in use loaded and referenced
    const auto& weaponInfo = *CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());

    bool releaseFirst{};
    bool releaseSecond{};
    if (weaponInfo.m_nWeaponFire != WEAPON_FIRE_MELEE || bInVehicle) {
        releaseFirst  = true;
        releaseSecond = true;
    } else {
        static_assert(MELEE_COMBO_UNARMED_1 == STYLE_STANDARD);

        // Requests the animation block of a melee style and references it once it is loaded
        const auto UpdateReference = [](int32 style, uint32& referencedGroup, bool& release) {
            if (style == STYLE_STANDARD) {
                release = referencedGroup != 0;
                return;
            }
            const AssocGroupId group = CTaskSimpleFight::m_aComboData[style - MELEE_COMBO_UNARMED_1].m_nAnimGroup;
            if (group == referencedGroup) {
                return;
            }
            release = referencedGroup != 0;

            auto* block = CAnimManager::GetAnimationBlock(group);
            if (!block) {
                block = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(group));
            }
            const auto blockIdx = CAnimManager::GetAnimationBlockIndex(block);
            if (block->IsLoaded) {
                if (referencedGroup == 0) {
                    CAnimManager::AddAnimBlockRef(blockIdx);
                    referencedGroup = group;
                }
            } else {
                CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
            }
        };
        UpdateReference(weaponInfo.m_nBaseCombo, playerData->m_nMeleeWeaponAnimReferenced, releaseFirst);
        UpdateReference(m_nFightingStyle, playerData->m_nMeleeWeaponAnimReferencedExtra, releaseSecond);
    }

    const auto ReleaseReference = [](auto& referencedGroup) {
        if (referencedGroup != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(static_cast<AssocGroupId>(referencedGroup)));
            referencedGroup = 0;
        }
    };
    if (releaseFirst) {
        ReleaseReference(playerData->m_nMeleeWeaponAnimReferenced);
    }
    if (releaseSecond) {
        ReleaseReference(playerData->m_nMeleeWeaponAnimReferencedExtra);
    }
}

// 0x609C80
void CPlayerPed::ClearWeaponTarget() {
    if (IsPlayer()) {
        CEntity::ClearReference(m_pTargetedObject);
        TheCamera.ClearPlayerWeaponMode();
        CWeaponEffects::ClearCrossHair(m_nPedType);
    }
}

// 0x609CD0
float CPlayerPed::GetWeaponRadiusOnScreen() {
    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    if (wep.IsTypeMelee())
        return 0.0f;

    const float accuracyProg = 0.5f / wepInfo.m_fAccuracy;
    switch (wep.m_Type) {
    case eWeaponType::WEAPON_SHOTGUN:
    case eWeaponType::WEAPON_SPAS12_SHOTGUN:
    case eWeaponType::WEAPON_SAWNOFF_SHOTGUN:
        return std::max(0.2f, accuracyProg); // here they multiply *accuracyProg * 1.0f* :thinking

    default: {
        const float rangeProg = std::min(1.0f, 15.0f / wepInfo.m_fWeaponRange);
        const float radius = (GetPlayerData()->m_fAttackButtonCounter * 0.5f + 1.0f) * rangeProg * accuracyProg;
        if (bIsDucking)
            return std::max(0.2f, radius / 2.0f);
        return std::max(0.2f, radius);
    }
    }
}

// 0x609D90
bool CPlayerPed::PedCanBeTargettedVehicleWise(CPed* ped) {
    if (ped->bInVehicle) {
        CVehicle* veh = ped->m_pVehicle;
        return veh && (veh->IsBike() || veh->vehicleFlags.bVehicleCanBeTargetted);
    }
    return true;
}

// 0x609DE0
float CPlayerPed::FindTargetPriority(CEntity* entity) {
    switch (entity->GetType()) {
    case ENTITY_TYPE_VEHICLE:
        return 0.1f;

    case ENTITY_TYPE_PED: {
        auto ped = entity->AsPed();

        if (ped->bThisPedIsATargetPriority)
            return 1.0f;

        if (ped->GetTaskManager().HasAnyOf<TASK_COMPLEX_KILL_PED_ON_FOOT, TASK_COMPLEX_ARREST_PED>()) {
            return 0.8f;
        }

        if (CPedGroups::AreInSameGroup(this, ped))
            return 0.05f;

        if (ped->m_nPedType == PED_TYPE_GANG2)
            return 0.06f;

        if (ped->IsCreatedByMission())
            return 0.25f;

        return 0.1f;
    }

    case ENTITY_TYPE_OBJECT: {
        switch (entity->AsObject()->m_nObjectType) {
        case eObjectType::OBJECT_MISSION:
        case eObjectType::OBJECT_MISSION2:
            return 0.1f;

        default:
            return 0.0f;
        }
    }
    default: {
        return 0.1f;
    }
    }
}

// 0x609ED0
void CPlayerPed::Clear3rdPersonMouseTarget() {
    CEntity::ClearReference(m_p3rdPersonMouseTarget);
}

// 0x609EF0
void CPlayerPed::Busted() {
    CWanted* wanted = GetWanted();
    if (wanted) {
        wanted->m_ChaosLevel = 0;
    }
}

// 0x41BE60
eWantedLevel CPlayerPed::GetWantedLevel() const {
    if (const auto* wanted = GetWanted()) {
        return wanted->GetWantedLevel();
    }

    return eWantedLevel::WANTED_CLEAN;
}

// 0x609F10
void CPlayerPed::SetWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevel(level);
}

// 0x609F30
void CPlayerPed::SetWantedLevelNoDrop(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevelNoDrop(level);
}

// 0x609F50
void CPlayerPed::CheatWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->CheatWantedLevel(level);
}

// 0x609F80
bool CPlayerPed::CanIKReachThisTarget(CVector posn, CWeapon* weapon, bool arg2) {
    if (!weapon->GetWeaponInfo(this).flags.bAimWithArm) {
        const CVector thisPos = GetPosition();
        return (posn - thisPos).Magnitude2D() >= thisPos.z - posn.z;
    }
    return true;
}

// 0x609FF0
CPlayerInfo* CPlayerPed::GetPlayerInfoForThisPlayerPed() {
    // TODO: Use range for here
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (FindPlayerPed(i) == this)
            return &FindPlayerInfo(i);
    }
    return nullptr;
}

// 0x60A020
void CPlayerPed::DoStuffToGoOnFire() {
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();
}

// 0x60A040
void CPlayerPed::AnnoyPlayerPed(bool arg0) {
    auto& temper = m_pStats->m_nTemper;

    if (temper < 52) {
        temper++;
    } else if (arg0) {
        if (temper < 55)
            temper++;
        else
            temper = 46;
    }
}

// 0x60A070
void CPlayerPed::ClearAdrenaline() {
    if (GetPlayerData()->m_bAdrenaline && GetPlayerData()->m_nAdrenalineEndTime != 0) {
        GetPlayerData()->m_nAdrenalineEndTime = 0;
        CTimer::ResetTimeScale();
    }
}

// 0x60A0A0
void CPlayerPed::DisbandPlayerGroup() {
    auto& ms = GetPlayerGroup().GetMembership();
    if (const auto numMembers = ms.CountMembersExcludingLeader()) {
        Say(numMembers > 1 ? CTX_GLOBAL_ORDER_DISBAND_MANY : CTX_GLOBAL_ORDER_ATTACK_SINGLE);
    } else {
        ms.RemoveAllFollowers(true);
    }
}

// 0x60A110
void CPlayerPed::MakeGroupRespondToPlayerTakingDamage(CEventDamage& damageEvent) {
    auto& group = GetPlayerGroup();
    if (!damageEvent.m_pSourceEntity)
        return;
    if (group.GetMembership().CountMembersExcludingLeader() < 1)
        return;
    if (!group.m_bMembersEnterLeadersVehicle)
        return;

    CEventGroupEvent groupEvent(this, damageEvent.Clone());
    group.GetIntelligence().AddEvent(&groupEvent);
}

// 0x60A1D0
void CPlayerPed::TellGroupToStartFollowingPlayer(bool arg0, bool arg1, bool arg2) {
    if (GetPlayerData()->m_bGroupAlwaysFollow && !arg0)
        return;
    if (GetPlayerData()->m_bGroupNeverFollow && arg0)
        return;

    CPedGroup& group = GetPlayerGroup();
    CPedGroupIntelligence& groupIntel = group.GetIntelligence();
    CPedGroupMembership& membership = group.GetMembership();
    if (!arg2 && !membership.CountMembersExcludingLeader())
        return;

    group.m_bMembersEnterLeadersVehicle = arg0;
    groupIntel.SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
    if (arg0) {
        CEventPlayerCommandToGroup playerCmdEvent;
        playerCmdEvent.ComputeResponseTaskType(&group);
        if (playerCmdEvent.WillRespond()) {
            auto gatherCmdEvent = new CEventPlayerCommandToGroup(ePlayerGroupCommand::PLAYER_GROUP_COMMAND_GATHER);
            gatherCmdEvent->m_TaskId = playerCmdEvent.m_TaskId;

            CEventGroupEvent groupEvent(this, gatherCmdEvent);
            groupIntel.AddEvent(&groupEvent);
        }
    }

    if (arg1) {
        const uint32 nMembers = membership.CountMembersExcludingLeader();
        if (!nMembers)
            return;

        if (arg0) {
            const float distToFurthest = group.FindDistanceToFurthestMember();
            if (nMembers > 1) {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_MANY : CTX_GLOBAL_ORDER_FOLLOW_NEAR_MANY);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_MANY);
            } else {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_ONE : CTX_GLOBAL_ORDER_FOLLOW_NEAR_ONE);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_ONE);
            }
        } else if (nMembers > 1) {
            Say(CTX_GLOBAL_ORDER_WAIT_MANY);
        } else {
            Say(CTX_GLOBAL_ORDER_WAIT_ONE);
        }
    }
}

// 0x60A440
void CPlayerPed::MakePlayerGroupDisappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetCollisionProcessed(false);
                member->SetIsVisible(false);
                abTempNeverLeavesGroup[i] = member->bNeverLeavesGroup;
                member->bNeverLeavesGroup = true;
            }
        }
    }
}

// 0x60A4B0
void CPlayerPed::MakePlayerGroupReappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetIsVisible(true);
                if (!member->bInVehicle)
                    member->SetUsesCollision(true);
                member->bNeverLeavesGroup = abTempNeverLeavesGroup[i];
            }
        }
    }
}

// 0x60A530
void CPlayerPed::ResetSprintEnergy()
{
    GetPlayerData()->m_fTimeCanRun = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
}

// 0x60A550
bool CPlayerPed::HandleSprintEnergy(bool sprint, float adrenalineConsumedPerTimeStep) {
    float& timeCanRun = GetPlayerData()->m_fTimeCanRun;
    if (sprint) {
        if (FindPlayerInfo().m_bDoesNotGetTired)
            return true;
        if (GetPlayerData()->m_bAdrenaline || adrenalineConsumedPerTimeStep == 0.0f)
            return true;

        if (timeCanRun > -150.0f) { // TODO: Find out what this magic number is
            timeCanRun = std::max(-150.0f, timeCanRun - CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep);
            return true;
        }
    } else {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > timeCanRun) {
            timeCanRun += CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep / 2.0f;
        }
    }
    return false;
}

constexpr auto PLAYER_SPRINT_THRESHOLD{ 5.0f }; // 0x8D2458
constexpr struct tPlayerSprintSet { // From 0x8D2460
    float field_0;
    float field_4;
    float field_8;
    float field_C;
    float field_10;
    float field_14;
    float field_18;
    float field_1C;
} PLAYER_SPRINT_SET[] = {
    // 0x0, 0x4,  0x8,  0xC,  0x10,  0x14, 0x18, 0x1C
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.5f, 0.3f }, // GROUND
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.4f, 1.0f }, // BMX
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.3f, 0.3f }, // WATER
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.0f, 1.0f }  // UNDERWATER
};

// 0x60A610
float CPlayerPed::ControlButtonSprint(eSprintType sprintType) {
    auto* const playerData = GetPlayerData();
    if (!playerData) {
        return 0.0f;
    }

    auto* const pad = GetPadFromPlayer();
    const auto& set = PLAYER_SPRINT_SET[sprintType];

    // The player can sprint while there is speed left or energy to keep going
    const auto canSprint = !playerData->m_bPlayerSprintDisabled && (playerData->m_fMoveSpeed > 0.0f || playerData->m_fTimeCanRun > 0.0f);

    auto& moveSpeed = playerData->m_fMoveSpeed;
    if (pad->SprintJustDown() && canSprint) {
        moveSpeed = std::min(set.field_0 + moveSpeed, set.field_10);
    } else if (pad->GetSprint() && canSprint) {
        moveSpeed = std::max(moveSpeed - CTimer::GetTimeStep() * set.field_4, 1.0f);
    } else if (moveSpeed > 0.0f) {
        moveSpeed = std::max(moveSpeed - CTimer::GetTimeStep() * set.field_8, 0.0f);
    }

    float sprintShare;
    float energyUse;
    if (moveSpeed > set.field_C) {
        sprintShare = moveSpeed / set.field_C;
        energyUse   = set.field_18;
    } else if (moveSpeed > 0.0f && canSprint) {
        sprintShare = 1.0f;
        energyUse   = set.field_14;
    } else {
        return 0.0f;
    }

    if (!HandleSprintEnergy(true, energyUse)) {
        playerData->m_fMoveSpeed = 0.0f;
        return 0.0f;
    }
    return std::max(sprintShare - 1.0f, 0.0f) * set.field_1C + 1.0f;
}

// 0x60A820
float CPlayerPed::GetButtonSprintResults(eSprintType sprintType) {
    // The original 0x60A820 leaves `edx` alone; this keeps that guarantee for callers of the hook.
    __asm { and edx, edx };

    const float moveSpeed = GetPlayerData()->m_fMoveSpeed;
    if (moveSpeed <= PLAYER_SPRINT_THRESHOLD) {
        return moveSpeed <= 0.0f ? 0.0f : 1.0f;
    }

    const float progress = std::max(0.0f, moveSpeed / PLAYER_SPRINT_THRESHOLD - 1.0f);
    return PLAYER_SPRINT_SET[sprintType].field_1C * progress + 1.0f;
}

// 0x60A8A0
void CPlayerPed::ResetPlayerBreath() {
    GetPlayerData()->m_fBreath = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// 0x60A8D0
void CPlayerPed::HandlePlayerBreath(bool bDecreaseAir, float fMultiplier) {
    float& breath = GetPlayerData()->m_fBreath;
    float  decreaseAmount = CTimer::GetTimeStep() * fMultiplier;
    if (!bDecreaseAir || CCheat::IsActive(CHEAT_INFINITE_OXYGEN)) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > breath)
            breath += decreaseAmount * 2.0f;
    } else {
        if (breath > 0.0f && bDrownsInWater)
            breath = std::max(0.0f, breath - decreaseAmount);
        else
            CWeapon::GenerateDamageEvent(this, this, eWeaponType::WEAPON_DROWNING, (int32)(decreaseAmount * 3.0f), PED_PIECE_TORSO, 0);
    }
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// 0x60A9C0
void CPlayerPed::SetRealMoveAnim() {
    RpClump* const clump = GetRpClump();
    CPlayerPedData* const playerData = GetPlayerData();

    auto* walk      = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_WALK);
    auto* run       = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN);
    auto* sprint    = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_SPRINT);
    auto* walkStart = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_WALK_START);
    auto* idle      = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_IDLE);
    auto* runStop   = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN_STOP);
    auto* runStopR  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN_STOPR);
    auto* turnLeft  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_TURN_L);
    auto* turnRight = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_TURN_R);
    auto* idleTired = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_IDLE_TIRED);

    if (bResetWalkAnims) {
        if (walk) walk->SetCurrentTime(0.0f);
        if (run) run->SetCurrentTime(0.0f);
        if (sprint) sprint->SetCurrentTime(0.0f);
        bResetWalkAnims = false;
    }

    if ((runStop && runStop->IsPlaying()) || (runStopR && runStopR->IsPlaying())) {
        m_nMoveState = PEDMOVE_RUN;
        if (runStop && !runStop->IsPlaying() && runStop->m_CurrentTime < runStop->m_BlendHier->m_fTotalTime) {
            runStop->m_Flags |= ANIMATION_IS_PLAYING;
        }
    } else if ((runStop && runStop->m_BlendDelta >= 0.0f) || (runStopR && runStopR->m_BlendDelta >= 0.0f)) {
        // Fade whatever run-stop animation is still up, right into the idle anim
        if (CAnimBlendAssociation* stopAnim = runStop ? runStop : runStopR) {
            stopAnim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
            stopAnim->m_BlendAmount = 1.0f;
            stopAnim->m_BlendDelta = -8.0f;
        }
        RestoreHeadingRate();
        if (!idle) {
            idle = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 8.0f);
        }
        idle->m_BlendAmount = 0.0f;
        idle->m_BlendDelta = 8.0f;
    } else if (playerData->m_fMoveBlendRatio != 0.0f || sprint) {
        if (idle) {
            // Standing up out of idle: `walk start` takes over unless it's already running
            if (!walkStart) {
                walkStart = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK_START);
            } else {
                walkStart->m_BlendAmount = 1.0f;
                walkStart->m_BlendDelta = 0.0f;
            }
            if (walk) walk->SetCurrentTime(0.0f);
            if (run) run->SetCurrentTime(0.0f);
            delete idle;
            if (idleTired) idleTired->m_BlendDelta = -4.0f;
            if (sprint) delete sprint;
            sprint = nullptr;
            m_nMoveState = PEDMOVE_WALK;
        }
        if (runStop) {
            delete runStop;
            RestoreHeadingRate();
        }
        if (runStopR) {
            delete runStopR;
            RestoreHeadingRate();
        }
        if (turnLeft) delete turnLeft;
        if (turnRight) delete turnRight;
        if (!walk) {
            walk = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK);
            walk->m_BlendAmount = 0.0f;
        }
        if (!run) {
            run = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_RUN);
            run->m_BlendAmount = 0.0f;
        }
        if (walkStart && (!walkStart->IsPlaying() || walkStart->m_BlendHier->m_fTotalTime < walkStart->m_CurrentTime + walkStart->m_TimeStep)) {
            // Not walked up yet, so skip the rest of it and blend walk/run straight in
            delete walkStart;
            walk->m_Flags |= ANIMATION_IS_PLAYING;
            run->m_Flags |= ANIMATION_IS_PLAYING;
            walkStart = nullptr;
        }
        if (m_nMoveState == PEDMOVE_SPRINT && walkStart) {
            m_nMoveState = PEDMOVE_STILL;
        }

        if (!sprint || (m_nMoveState == PEDMOVE_SPRINT && playerData->m_fMoveBlendRatio >= 0.4f)) {
            if (walkStart) {
                walk->m_Flags &= ~ANIMATION_IS_PLAYING;
                run->m_Flags &= ~ANIMATION_IS_PLAYING;
                walk->m_BlendAmount = 0.0f;
                run->m_BlendAmount = 0.0f;
                goto rtn;
            }
            if (m_nMoveState == PEDMOVE_SPRINT) {
                if (!sprint) {
                    if (run->m_BlendAmount < 1.0f) {
                        if (walk->m_BlendAmount == 0.0f && run->m_BlendAmount == 0.0f) {
                            walk->m_BlendAmount = 1.0f;
                        }
                        if (run->m_BlendDelta < 0.0f) {
                            run = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_RUN, 4.0f);
                        }
                        playerData->m_fMoveBlendRatio = run->m_BlendDelta + 1.0f;
                        goto rtn;
                    }
                    sprint = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_SPRINT, 2.0f);
                } else if (sprint->m_BlendDelta < 0.0f) {
                    sprint->m_BlendDelta = 2.0f;
                    run->m_BlendDelta = -2.0f;
                }
                if (sprint) CStats::UpdateStatsWhenSprinting();
                goto rtn;
            }
            if (playerData->m_fMoveBlendRatio >= 1.0f) {
                if (playerData->m_fMoveBlendRatio >= 2.0f) {
                    walk->m_BlendAmount = 0.0f;
                    run->m_BlendAmount = 1.0f;
                    walk->m_BlendDelta = 0.0f;
                    run->m_BlendDelta = 0.0f;
                    m_nMoveState = PEDMOVE_RUN;
                    CStats::UpdateStatsWhenRunning();
                } else {
                    walk->m_BlendAmount = 2.0f - playerData->m_fMoveBlendRatio;
                    run->m_BlendAmount = playerData->m_fMoveBlendRatio - 1.0f;
                    walk->m_BlendDelta = 0.0f;
                    run->m_BlendDelta = 0.0f;
                    m_nMoveState = PEDMOVE_RUN;
                }
                goto rtn;
            }
            walk->m_BlendAmount = 1.0f;
            run->m_BlendAmount = 0.0f;
            walk->m_BlendDelta = 0.0f;
            run->m_BlendDelta = 0.0f;
        } else {
            if (sprint->m_BlendAmount == 0.0f) {
                sprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                sprint->m_BlendDelta = -1000.0f;
            } else if (sprint->m_BlendDelta >= 0.0f || sprint->m_BlendAmount >= 0.8f) {
                if (playerData->m_fMoveBlendRatio >= 0.4f) {
                    if (sprint->m_BlendDelta >= 0.0f) {
                        sprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                        sprint->m_BlendDelta = -1.0f;
                        run->m_BlendDelta = 1.0f;
                    }
                } else {
                    // Slow down: half way through the sprint means the right-foot stop, otherwise the left one
                    CAnimBlendAssociation* stopAnim;
                    if (sprint->m_CurrentTime / sprint->m_BlendHier->m_fTotalTime >= 0.5) {
                        stopAnim = CAnimManager::AddAnimation(clump, (AssocGroupId)0, ANIM_ID_RUN_STOPR);
                    } else {
                        stopAnim = CAnimManager::AddAnimation(clump, (AssocGroupId)0, ANIM_ID_RUN_STOP);
                    }
                    stopAnim->m_BlendAmount = 1.0f;
                    stopAnim->SetDeleteCallback(CPed::RestoreHeadingRateCB, this);
                    m_fHeadingChangeRate = 0.0f;
                    sprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                    sprint->m_BlendDelta = -1000.0f;
                    walk->m_Flags &= ~ANIMATION_IS_PLAYING;
                    run->m_Flags &= ~ANIMATION_IS_PLAYING;
                    walk->m_BlendAmount = 0.0f;
                    run->m_BlendAmount = 0.0f;
                    walk->m_BlendDelta = 0.0f;
                    run->m_BlendDelta = 0.0f;
                }
            } else if (playerData->m_fMoveBlendRatio < 1.0f) {
                sprint->m_BlendDelta = -8.0f;
                run->m_BlendDelta = 8.0f;
            }
            if (playerData->m_fMoveBlendRatio > 1.0f) {
                m_nMoveState = PEDMOVE_RUN;
                goto rtn;
            }
        }
        m_nMoveState = PEDMOVE_WALK;
    } else {
        CPad* const pad = GetPadFromPlayer();
        bool turnAnimSet = false;
        if (pad->GetForceCameraBehindPlayer()) {
            if (const int16 aim = pad->AimWeaponLeftRight(nullptr); aim != 0 && TheCamera.GetActiveCam().m_nMode == MODE_FOLLOWPED) {
                const bool turningRight = aim >= 0;
                if (turningRight) {
                    if (!turnRight || turnRight->m_BlendDelta < 0.0f) {
                        turnRight = CAnimManager::BlendAnimation(clump, (AssocGroupId)0, ANIM_ID_TURN_R, 16.0f);
                    }
                } else {
                    if (!turnLeft || turnLeft->m_BlendDelta < 0.0f) {
                        turnLeft = CAnimManager::BlendAnimation(clump, (AssocGroupId)0, ANIM_ID_TURN_L, 16.0f);
                    }
                }
                // Full stick deflection (= 128) is a 1.0 blend speed
                (turningRight ? turnRight : turnLeft)->m_Speed = std::fabs(static_cast<float>(pad->AimWeaponLeftRight(nullptr))) * 0.0078125f;
                if (idle && idle->m_BlendAmount < 0.01f) delete idle;
                turnAnimSet = true;
            }
        }
        if (!turnAnimSet && !idle) {
            CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 4.0f);
        }

        if (playerData->m_fTimeCanRun >= 0.0f || GetIntelligence()->GetTaskFighting() || GetIntelligence()->GetTaskUseGun()
            || GetIntelligence()->GetTaskDuck(true) || GetIntelligence()->GetTaskThrow() || GetIntelligence()->GetTaskJetPack()
            || CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f, nullptr, true, false, false, false, false, false)) {
            if (idleTired && idleTired->m_BlendAmount > 0.0f && idleTired->m_BlendDelta >= 0.0f) {
                idleTired->m_Flags &= ~ANIMATION_IS_PLAYING;
                idleTired->m_BlendDelta = -2.0f;
            }
        } else if (!idleTired) {
            // Out of breath: fat players get their own tired anim
            idleTired = CAnimManager::BlendAnimation(clump,
                CClothes::GetDefaultPlayerMotionGroup() == ANIM_GROUP_FAT ? ANIM_GROUP_FAT_TIRED : (AssocGroupId)0,
                ANIM_ID_IDLE_TIRED, 4.0f);
            idleTired->m_Flags |= ANIMATION_IS_PLAYING;
            m_nMoveState = PEDMOVE_STILL;
            goto rtn;
        }
        m_nMoveState = PEDMOVE_STILL;
    }

rtn:
    if (playerData->m_bAdrenaline) {
        float speed;
        if (playerData->m_nAdrenalineEndTime < CTimer::GetTimeInMS() && !CCheat::IsActive(CHEAT_ADRENALINE_MODE)) {
            playerData->m_bAdrenaline = false;
            CTimer::SetTimeScale(1.0f);
            speed = 1.0f;
        } else {
            CTimer::SetTimeScale(1.0f / 3.0f);
            speed = 2.0f;
        }
        if (walkStart) walkStart->m_Speed = speed;
        if (walk) walk->m_Speed = speed;
        if (run) run->m_Speed = speed;
        if (!sprint) return;
        sprint->m_Speed = speed;
    }
    if (sprint) {
        if (TheCamera.GetActiveCam().m_nMode == MODE_FIXED) {
            sprint->m_Speed = 0.7f;
            return;
        }
        sprint->m_Speed = std::max(1.0f, GetButtonSprintResults(SPRINT_GROUND));
    }
}

// 0x60B460
void CPlayerPed::MakeChangesForNewWeapon(eWeaponType weaponType) {
    GetActiveWeapon().StopWeaponEffect();
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();

    SetCurrentWeapon(weaponType);

    GetPlayerData()->m_nChosenWeapon = m_nActiveWeaponSlot;
    GetPlayerData()->m_fAttackButtonCounter= 0.0f;

    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    wep.m_AmmoInClip = std::min<uint32>(wep.m_TotalAmmo, (uint32)wepInfo.m_nAmmoClip);

    if (!wepInfo.flags.bCanAim)
        ClearWeaponTarget();

    if (!wepInfo.flags.bOnlyFreeAim)
        GetPlayerData()->m_bFreeAiming = false;


    if (auto anim = RpAnimBlendClumpGetAssociation(GetRpClump(), ANIM_ID_FIRE))
        anim->m_Flags |= ANIMATION_IS_PLAYING & ANIMATION_IS_FINISH_AUTO_REMOVE;

    TheCamera.ClearPlayerWeaponMode();
}

// 0x60B550
bool LOSBlockedBetweenPeds(CEntity* entity1, CEntity* entity2) {
    CVector origin{};
    if (entity1->GetIsTypePed()) {
        origin = entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false);
        if (entity1->AsPed()->bIsDucking)
            origin.z += 0.35f;
    } else {
        origin = entity1->GetPosition();
    }

    const auto target = entity2->GetIsTypePed()
        ? entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false)
        : entity1->GetPosition();

    CColPoint colPoint;
    CEntity* hitEntity;
    if (CWorld::ProcessLineOfSight(origin, target, colPoint, hitEntity, true, false, false, true, false, false, false, true))
        return hitEntity != entity2;
    return false;
}

// 0x60B650
void CPlayerPed::Compute3rdPersonMouseTarget(bool weaponIsNotMelee) {
    CPed* target{};
    if (CCamera::m_bUseMouse3rdPerson) {
        const auto& weaponInfo = *CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
        const auto  range      = weaponInfo.m_fTargetRange;
        const auto  origin     = GetPosition();

        // Line from the camera, or from the crosshair for ranged weapons, through the aimed point
        CVector start, end;
        if (weaponIsNotMelee) {
            TheCamera.Find3rdPersonCamTargetVector(range, origin, start, end);
        } else {
            auto& cam = TheCamera.GetActiveCam();
            start     = cam.m_vecSource;

            // Keep the line from starting behind the player
            const auto behind = DotProduct(start - origin, cam.m_vecFront);
            if (behind < 0.0f) {
                start -= cam.m_vecFront * behind;
            }
            end = start + cam.m_vecFront * range;
        }

        CWorld::pIgnoreEntity  = this;
        CWorld::bIncludeBikers = true;
        CColPoint colPoint;
        CEntity*  hitEntity{};
        if (CWorld::ProcessLineOfSight(start, end, colPoint, hitEntity, false, false, true, false, false, false, false, false)
            && hitEntity != this
            && hitEntity->AsPed()->IsAlive()
        ) {
            target = hitEntity->AsPed();
        }
        CWorld::ResetLineTestOptions();

        if (target) {
            if (target != m_p3rdPersonMouseTarget) {
                if (m_p3rdPersonMouseTarget) {
                    m_p3rdPersonMouseTarget->CleanUpOldReference(reinterpret_cast<CEntity**>(&m_p3rdPersonMouseTarget));
                }
                m_p3rdPersonMouseTarget = target;
                target->RegisterReference(reinterpret_cast<CEntity**>(&m_p3rdPersonMouseTarget));
            }
            field_7A0 = CTimer::GetTimeInMS() + 1000;

            // Aiming at someone tells them (and their group) about it
            if (CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire == WEAPON_FIRE_MELEE) {
                return;
            }
            if (!CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->flags.bCanAim) {
                return;
            }
            if (!target->GetIntelligence()->IsInSeeingRange(GetPosition())) {
                return;
            }

            // Already reacting to being aimed at
            auto* const targetTasks = &target->GetIntelligence()->GetTaskManager();
            for (const auto slot : { TASK_PRIMARY_PHYSICAL_RESPONSE, TASK_PRIMARY_EVENT_RESPONSE_TEMP, TASK_PRIMARY_EVENT_RESPONSE_NONTEMP }) {
                if (auto* const task = targetTasks->GetPrimaryTasks()[slot]) {
                    if (task->GetTaskType() == TASK_COMPLEX_REACT_TO_GUN_AIMED_AT) {
                        return;
                    }
                    break;
                }
            }

            if (GetActiveWeapon().m_Type != WEAPON_PISTOL_SILENCED) {
                Say(CTX_GLOBAL_PULL_GUN);
            }

            if (auto* const group = CPedGroups::GetPedsGroup(target)) {
                if (CPedGroups::AreInSameGroup(target, this)) {
                    return;
                }
                CEventGroupEvent groupEvent{ target, new CEventGunAimedAt(this) };
                group->GetIntelligence().AddEvent(&groupEvent);
            } else {
                CEventGunAimedAt aimedAtEvent{ this };
                target->GetIntelligence()->m_eventGroup.Add(&aimedAtEvent, false);
            }
            return;
        }
    }

    // The mouse target is dropped once it has not been aimed at for a while
    if (m_p3rdPersonMouseTarget && static_cast<uint32>(field_7A0) < CTimer::GetTimeInMS()) {
        m_p3rdPersonMouseTarget = nullptr;
    }
}

// 0x60BA80
void CPlayerPed::DrawTriangleForMouseRecruitPed() {
    auto* const target = m_p3rdPersonMouseTarget;
    if (!target) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,           RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,          RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,  RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,      RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,  RWRSTATE(rwALPHATESTFUNCTIONALWAYS));

    // The marker goes from red to green with the health of the target
    uint8 red{}, green{}, blue{};
    const auto health = std::min(target->m_fHealth / target->m_fMaxHealth, 1.0f);
    if (health > 0.0f) {
        red   = static_cast<uint8>((1.0f - health) * 255.0f);
        green = static_cast<uint8>(health * 255.0f);
    }

    // The marker grows with the distance to the target
    const auto distance = std::max((target->GetPosition() - GetPosition()).Magnitude() - 10.0f, 0.0f);
    const auto size     = std::min(distance * 0.02f, 1.0f) * 0.825f + 0.175f;

    const auto side = TheCamera.GetRightVector() * size;
    const auto up   = CVector{ 0.0f, 0.0f, size };

    auto  base = target->GetPosition();
    base.z += 1.0f;

    // Gang members of the Ballas get the triangle pointing down
    CVector points[3];
    if (target->m_nPedType == PED_TYPE_GANG1) {
        points[0] = base;
        points[1] = base - side + up;
        points[2] = base + side + up;
    } else {
        points[0] = base + up;
        points[1] = base + side;
        points[2] = base - side;
    }

    // Pull the vertices one unit towards the camera so they do not sink into the ped
    const auto& camPos = TheCamera.GetPosition();
    for (auto& point : points) {
        auto towardsCam = camPos - point;
        towardsCam.Normalise();
        point += towardsCam;
    }

    auto* const verts = TempBufferVertices.m_3d;
    for (auto i = 0; i < 3; i++) {
        RwIm3DVertexSetPos(&verts[i], points[i].x, points[i].y, points[i].z);
    }
    RwIm3DVertexSetRGBA(&verts[0], red, green, blue, 255);
    RwIm3DVertexSetRGBA(&verts[1], red, green, blue, 0);
    RwIm3DVertexSetRGBA(&verts[2], red, green, blue, 0);

    aTempBufferIndices[0] = 0;
    aTempBufferIndices[1] = 1;
    aTempBufferIndices[2] = 2;
    if (RwIm3DTransform(verts, 3, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, 3);
        RwIm3DEnd();
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,      RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,           RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,          RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,  RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,  RWRSTATE(rwALPHATESTFUNCTIONGREATER));
}

// 0x60C0C0
bool CPlayerPed::DoesTargetHaveToBeBroken(CEntity* target, CWeapon* weapon) {
    if (!target->GetIsVisible()) {
        return true;
    }

    if (weapon->GetWeaponRange(this, target) < (target->GetPosition() - GetPosition()).Magnitude()) {
        return true;
    }

    if (weapon->m_Type == eWeaponType::WEAPON_SPRAYCAN) {
        if (target->GetIsTypeBuilding()) {
            if (CTagManager::IsTag(*target)) {
                if (CTagManager::GetAlpha(*target) == 255) { // they probably used -1
                    return true;
                }
            }
        }
    }

    return !CanIKReachThisTarget(target->GetPosition(), weapon, false);
}

// 0x60C1E0
void CPlayerPed::KeepAreaAroundPlayerClear() {
    // Game created pedestrians nearby either walk away from the player or are removed when off screen
    static auto& fleePosCheckPeriod    = StaticRef<int32>(0x86F678);
    static auto& fleePosChangeTolerance = StaticRef<float>(0xC18CF0);

    for (auto* const entity : GetIntelligence()->GetPedScanner().m_apEntities) {
        auto* const ped = entity ? entity->AsPed() : nullptr;
        if (!ped || !ped->IsCreatedBy(PED_GAME) || ped->bInVehicle || !ped->IsAlive()) {
            continue;
        }
        if (CPedGroups::GetGroup(0).GetMembership().IsMember(ped)) {
            continue;
        }

        if (!ped->GetIsOnScreen() || ped->bKeepTasksAfterCleanUp) {
            ped->FlagToDestroyWhenNextProcessed();
            continue;
        }

        // Skip peds that are already fleeing from the player
        if (auto* const fleeTask = static_cast<CTaskComplexSmartFleeEntity*>(ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_SMART_FLEE_ENTITY));
            fleeTask && fleeTask->m_fleeFrom == this
        ) {
            continue;
        }
        if (auto* const scriptCommand = static_cast<CEventScriptCommand*>(ped->GetIntelligence()->m_eventGroup.GetEventOfType(EVENT_SCRIPT_COMMAND));
            scriptCommand && scriptCommand->m_task && scriptCommand->m_task->GetTaskType() == TASK_COMPLEX_SMART_FLEE_ENTITY
        ) {
            continue;
        }

        auto* const task = new CTaskComplexSmartFleeEntity{ this, false, 1000.0f, 100'000, fleePosCheckPeriod, fleePosChangeTolerance, PEDMOVE_WALK };
        CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, task, false };
        ped->GetIntelligence()->m_eventGroup.Add(&event, false);
    }

    // Vehicles close to the player (or the vehicle the player is in) are told to wait or back off
    const auto center = bInVehicle && m_pVehicle ? m_pVehicle->GetPosition() : GetPosition();

    int16     numFound{};
    CEntity*  found[6]{};
    CWorld::FindObjectsInRange(GetPosition(), 15.0f, true, &numFound, 6, found, false, true, false, false, false);
    for (auto i = 0; i < numFound; i++) {
        auto* const vehicle = found[i]->AsVehicle();
        if (vehicle->m_nCreatedBy == MISSION_VEHICLE || vehicle->GetStatus() == STATUS_PLAYER || vehicle->GetStatus() == STATUS_FORCED_STOP) {
            continue;
        }

        auto& autoPilot = vehicle->m_autoPilot;
        const auto& vehPos = vehicle->GetPosition();
        if ((vehPos - center).SquaredMagnitude() > 25.0f) {
            autoPilot.m_nTempAction     = TEMPACT_WAIT;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 5000;
        } else {
            // Back off when the player is in front of the vehicle, otherwise keep going
            const auto& forward = vehicle->GetForward();
            const auto  inFront = (center.y - vehPos.y) * forward.y + (center.x - vehPos.x) * forward.x > 0.0f;
            autoPilot.m_nTempAction     = inFront ? TEMPACT_REVERSE : TEMPACT_GOFORWARD;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 2000;
        }
        CCarCtrl::PossiblyRemoveVehicle(vehicle);
    }
}

// 0x60C520
void CPlayerPed::SetPlayerMoveBlendRatio(CVector* point) {
    float& moveBlendRatio = GetPlayerData()->m_fMoveBlendRatio;
    if (point) {
        moveBlendRatio = std::min(2.0f, (*point - GetPosition()).Magnitude2D() * 2.0f);
    } else {
        switch (m_nMoveState)
        {
        case PEDMOVE_WALK:
            moveBlendRatio = 1.0f;
            break;

        case PEDMOVE_RUN:
            moveBlendRatio = 1.8f;
            break;

        case PEDMOVE_SPRINT:
            moveBlendRatio = 2.5f;
            break;

        default:
            moveBlendRatio = 0.0f;
            break;
        }
    }
    SetRealMoveAnim();
}

// 0x60C5F0
CPed* CPlayerPed::FindPedToAttack() {
    CVector origin = FindPlayerCoors();
    origin.z = 0.0f;

    CVector end = origin + TheCamera.GetForward() * 100.0f;
    end.z = 0.0f;

    CPed* closestPed{};
    float closestDistance = std::numeric_limits<float>::max();

    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; GetPedPool()->GetSize(); i++) {
        CPed* ped = GetPedPool()->GetAt(i);
        if (!ped)
            continue;
        if (ped->IsPlayer())
            continue;
        if (!ped->IsAlive())
            continue;
        if (membership.IsMember(ped))
            continue;
        if (ped->m_nPedType == PED_TYPE_GANG2)
            continue;

        CVector point = ped->GetPosition();
        point.z = 0.0f;

        float dist = CCollision::DistToLine(origin, end, point);
        float pointDist = (point - origin).Magnitude2D();
        if (pointDist > 20.0f)
            dist += (pointDist - 20.0f) / 5.0f;

        if (IsPedTypeGang(ped->m_nPedType))
            dist = std::max(0.0f, dist / 2.0f - 2.0f);

        if (dist < closestDistance) {
            closestDistance = dist;
            closestPed = ped;
        }
    }
    return closestPed;
}

// 0x60C7C0
void CPlayerPed::ForceGroupToAlwaysFollow(bool enable) {
    GetPlayerData()->m_bGroupAlwaysFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(true, false, true);
}

// 0x60C800
void CPlayerPed::ForceGroupToNeverFollow(bool enable) {
    GetPlayerData()->m_bGroupNeverFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(false, false, true);
}

// 0x609380
static bool IsRecruitCheatActive() {
    return CCheat::IsActive(CHEAT_WANNA_BE_IN_MY_GANG) || CCheat::IsActive(CHEAT_NO_ONE_CAN_STOP_US) || CCheat::IsActive(CHEAT_ROCKET_MAYHEM);
}

// Time the group buttons have to be held to disband the group
static constexpr uint16 HOLD_TO_DISBAND_MS = 0x4B0;

// 0x60C840
void CPlayerPed::MakeThisPedJoinOurGroup(CPed* ped) {
    if (ped->bDruggedUp) {
        Say(CTX_GLOBAL_DRUGGED_IGNORE);
        return;
    }
    if (ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_KILL_PED_ON_FOOT)) {
        return;
    }

    // Only Families members can be recruited, unless one of the recruit cheats is on
    const auto recruitCheat = IsRecruitCheatActive();
    if (ped->m_nPedType != PED_TYPE_GANG2 && !recruitCheat) {
        return;
    }

    auto& group = GetPlayerGroup();
    if (group.GetMembership().IsMember(ped)) {
        return;
    }

    CAEPedSpeechAudioEntity::SetCJMood(static_cast<eCJMood>(-1), 10'000, 1, -1, -1);
    Say(CTX_GLOBAL_JOIN_ME_ASK, 0, 1.0f, true);

    auto maxMembers = CStats::FindMaxNumberOfGroupMembers();
    maxMembers      = std::min<int32>(maxMembers, GetPlayerData()->m_nScriptLimitToGangSize);
    if (const auto cityUnlocked = CStats::GetStatValue(STAT_CITY_UNLOCKED); cityUnlocked == 1.0f || cityUnlocked == 2.0f) {
        maxMembers = 0;
    }

    const auto numMembers = group.GetMembership().CountMembersExcludingLeader();
    if (!(recruitCheat && numMembers < 7) && numMembers >= maxMembers) {
        ped->Say(CTX_GLOBAL_JOIN_GANG_NO, 2500, 1.0f, true);

        CEventDontJoinPlayerGroup event{ this };
        ped->GetIntelligence()->m_eventGroup.Add(&event, false);
        return;
    }

    if (auto* const oldGroup = CPedGroups::GetPedsGroup(ped)) {
        oldGroup->GetMembership().RemoveMember(ped);
    }

    // The task belongs to the script command event, which is destroyed at the end of the scope
    const auto playerGroupId = FindPlayerPed(-1)->GetPlayerData()->m_nPlayerGroup;
    CEventScriptCommand scriptCommand{ TASK_PRIMARY_PRIMARY, new CTaskComplexBeInGroup(playerGroupId, false), false };
    ped->GetIntelligence()->m_eventGroup.Add(&scriptCommand, false);

    group.GetMembership().AddFollower(ped);
    group.Process();
    ped->GiveWeaponWhenJoiningGang();

    CEventGroupEvent newMemberEvent{ this, new CEventNewGangMember(ped) };
    group.GetIntelligence().AddEvent(&newMemberEvent);

    ped->bDrownsInWater = false;

    CStats::IncrementStat(STAT_GANG_MEMBERS_RECRUITED, 1.0f);
    CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_GANG_STRENGTH, 1.0f);

    const auto blipColor = CGangWars::GetGangColor(GANG_GROVE);
    const auto blip      = CRadar::SetEntityBlip(BLIP_CHAR, CPools::GetPedRef(ped), blipColor, BLIP_DISPLAY_BLIPONLY);
    CRadar::ChangeBlipScale(blip, 2);
    CRadar::ChangeBlipColour(blip, blipColor);
    CRadar::SetBlipFriendly(blip, true);

    ped->bClearRadarBlipOnDeath = true;

    static auto& playerGroupMaxSeparation = StaticRef<float>(0x86C6C0);
    group.GetMembership().SetSeparationRange(playerGroupMaxSeparation);

    ped->Say(CTX_GLOBAL_JOIN_GANG_YES, 2500, 1.0f, true);
}

// 0x60CC50
bool CPlayerPed::PlayerWantsToAttack() {
    auto& group = GetPlayerGroup();
    if (group.GetMembership().CountMembersExcludingLeader() < 1) {
        return false;
    }
    if (!group.m_bMembersEnterLeadersVehicle) {
        return false;
    }
    group.GetIntelligence().ReportAllBarScriptTasksFinished();

    // Lock-on target first, then the mouse target when the mouse aims in third person
    auto* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }

    CPed* ped{};
    if (target) {
        if (target->GetIsTypePed()) {
            ped = target->AsPed();
        } else {
            if (CTagManager::IsTag(*target)) {
                return false;
            }
            if (target->GetIsTypeObject() && target->AsObject()->CanBeTargetted()) {
                return false;
            }
            ped = FindPedToAttack();
        }
    } else {
        ped = FindPedToAttack();
    }

    if (ped) {
        group.PlayerGaveCommand_Attack(this, ped);
    }
    return false;
}

// 0x60CD20
void CPlayerPed::SetInitialState(bool bGroupCreated) {
    CMBlur::ClearDrunkBlur();
    CTimer::ResetTimeScale();

    m_bUsesCollision            = true;
    physicalFlags.bApplyGravity = true;

    ClearAimFlag();
    ClearLookFlag();
    bRenderPedInCar = true;
    if (m_pFire) {
        m_pFire->Extinguish();
    }

    SetPedState(PEDSTATE_IDLE);
    SetMoveState(PEDMOVE_STILL);

    bDontRender      = false;
    bIsBeingArrested = false;
    bCanExitCar      = true;
    bIsDucking       = false;

    GetIntelligence()->FlushIntelligence();
    RpAnimBlendClumpRemoveAllAssociations(GetRpClump());
    GetTaskManager().SetTask(new CTaskSimplePlayerOnFoot{}, TASK_PRIMARY_DEFAULT, false);

    m_nAnimGroup         = ANIM_GROUP_PLAYER;
    bIsPedDieAnimPlaying = false;
    if (m_pPlayerData) {
        m_pPlayerData->m_bAdrenaline = false;
    }
    SetRealMoveAnim();

    m_pStats->m_nTemper = 50;

    if (m_pAttachedTo && !m_bUsesCollision) {
        m_bUsesCollision = true;
    }
    m_pAttachedTo = nullptr;
    m_nTurretAmmo = 0;

    GetTaskManager().SetTaskSecondary(new CTaskComplexFacial{}, TASK_SECONDARY_FACIAL_COMPLEX);

    if (!bGroupCreated && !GetPlayerData()->m_bGroupNeverFollow) {
        auto& group = GetPlayerGroup();
        group.m_bMembersEnterLeadersVehicle = true;
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);

        CEventPlayerCommandToGroupAttack attackEvent{ nullptr };
        attackEvent.ComputeResponseTaskType(&group);
        if (attackEvent.WillRespond()) {
            auto* const gatherEvent = new CEventPlayerCommandToGroupGather{ nullptr };
            gatherEvent->m_TaskId   = attackEvent.m_TaskId;

            CEventGroupEvent groupEvent{ this, gatherEvent };
            group.GetIntelligence().AddEvent(&groupEvent);
        }
    }

    if (m_pPlayerData) {
        m_pPlayerData->SetInitialState();
    }
}

// 0x60D000
void CPlayerPed::MakeChangesForNewWeapon(uint32 weaponSlot) {
    if (weaponSlot != -1)
        MakeChangesForNewWeapon(GetWeaponInSlot(weaponSlot).m_Type);
}

static auto& PLAYER_MAX_TARGET_VIEW_ANGLE = StaticRef<float>(0x8D243C); // 140.0f

// 0x60D020
void CPlayerPed::EvaluateTarget(CEntity* target, CEntity *& outTarget, float & outTargetPriority, float maxDistance, float compensationRotRad, bool arg5) {
    const CVector dir = target->GetPosition() - GetPosition();
    const float dist = dir.Magnitude();

    if (dist > maxDistance)
        return;

    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon()))
        return;

    const float targetAngleDeg = std::fabs(RadiansToDegrees(CGeneral::LimitRadianAngle(CGeneral::GetATanOf(dir) - compensationRotRad)));

    float viewAngleMultiplier = 1.0f - targetAngleDeg / PLAYER_MAX_TARGET_VIEW_ANGLE;
    if (dist > 1.0f)
        viewAngleMultiplier /= sqrt(sqrt(dist)); // Take quad root of dist

    const float targetPriority = FindTargetPriority(target) * viewAngleMultiplier;
    if (targetPriority > outTargetPriority && !LOSBlockedBetweenPeds(this, target)) {
        outTarget = target;
        outTargetPriority = targetPriority;
    }
}

// 0x60D1C0
void CPlayerPed::EvaluateNeighbouringTarget(CEntity* target, CEntity** outTarget, float* outTargetPriority, float maxDistance, float compensationRotRad, bool searchPositiveSide) {
    if ((target->GetPosition() - GetPosition()).Magnitude() > maxDistance) {
        return;
    }
    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon())) {
        return;
    }

    // Angle between the camera to target direction and the reference rotation
    const auto angle = CGeneral::LimitRadianAngle(CGeneral::GetATanOf(CVector2D{ target->GetPosition() - TheCamera.GetPosition() }) - compensationRotRad);
    if (std::fabs(angle) >= 0.87266463f) { // 50 degrees
        return;
    }

    // Only targets on the requested side are candidates; the closest to the reference wins
    float priority;
    if (searchPositiveSide) {
        priority = angle > 0.0f ? -angle : -100'000.0f;
    } else {
        priority = angle < 0.0f ? angle : -100'000.0f;
    }

    if (priority > *outTargetPriority) {
        *outTarget         = target;
        *outTargetPriority = priority;
    }
}

// 0x60D350
void CPlayerPed::ProcessGroupBehaviour(CPad* pad) {
    auto* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }

    auto* const playerData = GetPlayerData();
    const auto  timeMS     = static_cast<uint16>(static_cast<int32>(CTimer::GetTimeStepNonClipped() * 0.02f * 1000.0f));

    if (!FindPlayerVehicle(-1, false)) {
        // Holding the forward button disbands the group, a short press recruits or calls the group
        auto& pressedMS = playerData->m_nPadUpPressedInMilliseconds;
        if (pad->GetGroupControlForward()) {
            pressedMS += timeMS;
            if (pressedMS == HOLD_TO_DISBAND_MS && !playerData->m_bGroupStuffDisabled) {
                DisbandPlayerGroup();
            }
        } else {
            if (pressedMS != 0 && pressedMS < HOLD_TO_DISBAND_MS) {
                const auto isRecruitable = target && target->GetIsTypePed() && (target->AsPed()->m_nPedType == PED_TYPE_GANG2 || IsRecruitCheatActive());
                if (!isRecruitable) {
                    TellGroupToStartFollowingPlayer(true, true, false);
                } else if (!playerData->m_bGroupStuffDisabled) {
                    MakeThisPedJoinOurGroup(target->AsPed());
                }
            }
            pressedMS = 0;
        }
    }

    if (playerData->m_bGroupStuffDisabled) {
        return;
    }

    if (!FindPlayerVehicle(-1, false)) {
        auto& pressedMS = playerData->m_nPadDownPressedInMilliseconds;
        if (pad->GetGroupControlBack()) {
            pressedMS += timeMS;
            if (pressedMS >= HOLD_TO_DISBAND_MS) {
                DisbandPlayerGroup();
            }
        } else {
            if (pressedMS != 0 && pressedMS < HOLD_TO_DISBAND_MS) {
                if (target && target->GetIsTypePed() && target->AsPed()->m_nPedType == PED_TYPE_GANG2) {
                    MakeThisPedJoinOurGroup(target->AsPed());
                } else {
                    TellGroupToStartFollowingPlayer(false, true, false);
                }
            }
            pressedMS = 0;
        }
    }

    // Families members hate the police only while the player is wanted
    if ((CTimer::GetFrameCounter() & 0x1F) == 6) {
        for (auto& ped : GetPedPool()->GetAllValid()) {
            if (ped.m_nPedType != PED_TYPE_GANG2) {
                continue;
            }
            const auto copMask = CPedType::GetPedFlag(PED_TYPE_COP);
            if (FindPlayerWanted(-1)->m_WantedLevel != eWantedLevel::WANTED_CLEAN) {
                ped.m_acquaintance.SetAsAcquaintance(ACQUAINTANCE_HATE, copMask);
            } else {
                ped.m_acquaintance.ClearAsAcquaintance(ACQUAINTANCE_HATE, copMask);
            }
        }
    }
}

// 0x60D5A0
bool CPlayerPed::PlayerHasJustAttackedSomeone() {
    return PlayerWantsToAttack();
}

// 0x60D790
void CPlayerPed::SetupPlayerPed(int32 playerId) {
    auto ped = new CPlayerPed(playerId, false);
    auto& playerInfo = FindPlayerInfo(playerId);
    playerInfo.m_pPed = ped;

    if (playerId == 1)
        ped->m_nPedType = PED_TYPE_PLAYER2;

    ped->SetOrientation(0.0f, 0.0f, 0.0f);
    CWorld::Add(ped);
    ped->m_nWeaponAccuracy = 100;
    playerInfo.m_nPlayerState = ePlayerState::PLAYERSTATE_PLAYING;
}

// 0x60D850
void CPlayerPed::ProcessWeaponSwitch(CPad* pad) {
    auto* const playerData = GetPlayerData();

    // Checks whether the slot holds a weapon that can be selected
    const auto IsSlotSelectable = [this](int8 slot) {
        auto& weapon = m_aWeapons[slot];
        return weapon.m_Type != WEAPON_UNARMED
            && weapon.HasWeaponAmmoToBeUsed()
            && (!CGameLogic::IsCoopGameGoingOn() || weapon.CanBeUsedFor2Player());
    };

    // Camera modes in which the weapon cannot be changed with the cycle buttons
    const auto camMode = TheCamera.m_PlayerWeaponMode.m_nMode;

    if (!CDarkel::FrenzyOnGoing() && !m_pAttachedTo && !GetIntelligence()->GetTaskJetPack()) {
        if (!m_pTargetedObject && !playerData->m_bFreeAiming && !playerData->m_bDontAllowWeaponChange && !playerData->m_bInVehicleDontAllowWeaponChange) {
            if (pad->CycleWeaponRightJustDown()) {
                switch (camMode) {
                case MODE_M16_1STPERSON:
                case MODE_M16_1STPERSON_RUNABOUT:
                case MODE_SNIPER:
                case MODE_SNIPER_RUNABOUT:
                case MODE_ROCKETLAUNCHER:
                case MODE_ROCKETLAUNCHER_RUNABOUT:
                case MODE_ROCKETLAUNCHER_HS:
                case MODE_ROCKETLAUNCHER_RUNABOUT_HS:
                case MODE_CAMERA:
                    break;
                default: {
                    auto slot = static_cast<int8>(m_nActiveWeaponSlot + 1);
                    while (slot < NUM_WEAPON_SLOTS && !IsSlotSelectable(slot)) {
                        slot++;
                    }
                    playerData->m_nChosenWeapon = slot < NUM_WEAPON_SLOTS ? slot : 0;
                    break;
                }
                }
            } else if (pad->CycleWeaponLeftJustDown()) {
                switch (camMode) {
                case MODE_M16_1STPERSON:
                case MODE_SNIPER:
                case MODE_ROCKETLAUNCHER:
                case MODE_ROCKETLAUNCHER_HS:
                case MODE_CAMERA:
                    break;
                default: {
                    auto slot = static_cast<int8>(m_nActiveWeaponSlot - 1);
                    while (true) {
                        if (slot < 0) {
                            slot = NUM_WEAPON_SLOTS - 1;
                        }
                        if (slot == 0 || IsSlotSelectable(slot)) {
                            break;
                        }
                        slot--;
                    }
                    playerData->m_nChosenWeapon = slot;
                    break;
                }
                }
            }
        }

        // Out of ammo: fall back to the closest lower slot that still has some
        const auto& activeWeapon = GetWeaponInSlot(m_nActiveWeaponSlot);
        if (CWeaponInfo::GetWeaponInfo(activeWeapon.m_Type, eWeaponSkill::STD)->m_nWeaponFire != WEAPON_FIRE_MELEE
            && !(pad->GetWeapon(this) && activeWeapon.m_Type == WEAPON_MINIGUN)
            && static_cast<int32>(activeWeapon.m_TotalAmmo) <= 0
        ) {
            switch (camMode) {
            case MODE_M16_1STPERSON:
            case MODE_SNIPER:
            case MODE_ROCKETLAUNCHER:
            case MODE_ROCKETLAUNCHER_HS:
                return;
            default:
                break;
            }

            // The detonator is paired with the remote charges in slot 8
            auto slot = (activeWeapon.m_Type == WEAPON_DETONATOR && GetWeaponInSlot(8).m_Type == WEAPON_REMOTE_SATCHEL_CHARGE)
                ? int8{ 8 }
                : static_cast<int8>(m_nActiveWeaponSlot - 1);
            while (slot >= 0) {
                if (slot == 5 && GetWeaponInSlot(5).m_Type == WEAPON_BASEBALLBAT) {
                    break;
                }
                if (static_cast<int32>(GetWeaponInSlot(slot).m_TotalAmmo) > 0 && slot != 0x12 && slot != 0x11 && slot != 0x10) {
                    break;
                }
                slot--;
            }
            playerData->m_nChosenWeapon = slot >= 0 ? slot : 0;
        }
    }

    if (playerData->m_nChosenWeapon == m_nActiveWeaponSlot) {
        return;
    }

    // Do not interrupt a shot or a reload that is in progress
    if (auto* const gunTask = GetIntelligence()->GetTaskUseGun()) {
        const auto lastCmd = gunTask->GetLastGunCommand();
        if (lastCmd == eGunCommand::FIRE || lastCmd == eGunCommand::FIREBURST) {
            return;
        }
        if (lastCmd == eGunCommand::RELOAD && gunTask->m_Anim) {
            return;
        }
    }

    RemoveWeaponAnims(m_nActiveWeaponSlot, -4000.0f);
    if (static_cast<int8>(playerData->m_nChosenWeapon) != -1) {
        MakeChangesForNewWeapon(GetWeaponInSlot(static_cast<int8>(playerData->m_nChosenWeapon)).m_Type);
    }
}

// 0x60DC50
bool CPlayerPed::FindWeaponLockOnTarget() {
    return plugin::CallMethodAndReturn<bool, 0x60DC50, CPlayerPed *>(this);
}

// 0x60E530
bool CPlayerPed::FindNextWeaponLockOnTarget(CEntity* arg0, bool arg1) {
    return plugin::CallMethodAndReturn<bool, 0x60E530, CPlayerPed *, CEntity*, bool>(this, arg0, arg1);
}

// 0x60EA90
void CPlayerPed::ProcessControl() {
    if (GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_nCarDangerCounter--;
    if (!GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_pDangerCar = 0;
    if (GetPlayerData()->m_nFadeDrunkenness) {
        if (GetPlayerData()->m_nDrunkenness - 1 > 0) {
            --GetPlayerData()->m_nDrunkenness;
        } else {
            GetPlayerData()->m_nDrunkenness = 0;
            CMBlur::ClearDrunkBlur();
            GetPlayerData()->m_nFadeDrunkenness = 0;
        }
    }
    if (GetPlayerData()->m_nDrunkenness) 
        CMBlur::SetDrunkBlur(GetPlayerData()->m_nDrunkenness / 255.0f);
    if (GetPlayerData()->m_bRequireHandleBreath) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > GetPlayerData()->m_fBreath)
            GetPlayerData()->m_fBreath += CTimer::GetTimeStep() + CTimer::GetTimeStep();
        GetPlayerData()->m_bRequireHandleBreath = false;
    }
    GetPlayerData()->m_bRequireHandleBreath = true;
    CPed::ProcessControl();
    bCheckColAboveHead = true;
    float markColor = 1.0f;
    bool limitMarkColor = true;
    CVector effectPos;
    CPad* pad = CPad::GetPad(m_nPedType);
    if (!bCanPointGunAtTarget) {
        GetPlayerWanted()->Update();
        PruneReferences();
        if (GetActiveWeapon().m_Type == WEAPON_MINIGUN) {
            auto weaponInfo = CWeaponInfo::GetWeaponInfo(WEAPON_MINIGUN, eWeaponSkill::STD);
            if (GetIntelligence()->GetTaskUseGun()) {
                auto animAssoc = GetIntelligence()->GetTaskUseGun()->m_Anim;
                if (animAssoc && animAssoc->m_CurrentTime - animAssoc->m_TimeStep < weaponInfo->m_fAnimLoopEnd) {
                    if (GetPlayerData()->m_fGunSpinSpeed < 0.45f) {
                        GetPlayerData()->m_fGunSpinSpeed += CTimer::GetTimeStep() * 0.025f;
                        GetPlayerData()->m_fGunSpinSpeed = std::min(GetPlayerData()->m_fGunSpinSpeed, 0.45f);
                    }
                    if (pad->GetWeapon(this) && GetActiveWeapon().m_TotalAmmo > 0 && animAssoc->m_CurrentTime >= weaponInfo->m_fAnimLoopStart) 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_AMMO);
                    else 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_NO_AMMO);
                }
            } else {
                if (GetPlayerData()->m_fGunSpinSpeed > 0.0f) {
                    GetPlayerData()->m_fGunSpinSpeed -= CTimer::GetTimeStep() * 0.003f;
                    GetPlayerData()->m_fGunSpinSpeed = std::max(GetPlayerData()->m_fGunSpinSpeed, 0.0f);
                }
            }
        }
        if (GetActiveWeapon().m_Type == WEAPON_CHAINSAW && m_nPedState != PEDSTATE_ATTACK && !bInVehicle) {
            GetIntelligence()->GetTaskSwim(); // hmmm?
        }
        if (m_pTargetedObject) {
            ClearReference(m_p3rdPersonMouseTarget);
            if (m_pTargetedObject->GetIsTypePed()) {
                CPed* targetPed = m_pTargetedObject->AsPed();
                auto weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
                float targetHeadRange = weaponInfo->GetTargetHeadRange();
                markColor = targetPed->m_fHealth / targetPed->m_fMaxHealth;
                bool instantFireHit = false;
                if (targetPed->IsAlive()) {
                    auto stdWeaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD);
                    if (stdWeaponInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT) {
                        instantFireHit = true;
                        CVector distance = targetPed->GetPosition() - GetPosition();
                        if (targetHeadRange * targetHeadRange > distance.SquaredMagnitude()) {
                            GetPlayerData()->m_nTargetBone = BONE_HEAD;
                            GetPlayerData()->m_vecTargetBoneOffset.x = 0.05f;
                        }
                    }
                }
                if (!instantFireHit) {
                    GetPlayerData()->m_nTargetBone = BONE_SPINE1;
                    GetPlayerData()->m_vecTargetBoneOffset.x = 0.2f;
                }
                effectPos = GetPlayerData()->m_vecTargetBoneOffset;
                targetPed->GetTransformedBonePosition(effectPos, static_cast<eBoneTag>(GetPlayerData()->m_nTargetBone), false);
                bool targetIsInVehicle = false;
                if (markColor > 0.0f) {
                    if (!targetPed->bInVehicle && targetPed->m_nMoveState != PEDMOVE_STILL) {
                        effectPos += targetPed->m_vecMoveSpeed * CTimer::GetTimeStep();
                    }
                }
                if (targetPed->bInVehicle) {
                    auto targetVeh = targetPed->m_pVehicle;
                    if (targetVeh)
                        effectPos += (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                }
            } else if (m_pTargetedObject->GetIsTypeVehicle()) {
                CVehicle* targetVeh = m_pTargetedObject->AsVehicle();
                effectPos = (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                effectPos += targetVeh->GetPosition();
            } else if (m_pTargetedObject->GetIsTypeObject()) {
                CObject* targetObj = m_pTargetedObject->AsObject();
                effectPos = targetObj->m_vecMoveSpeed * CTimer::GetTimeStep();
                effectPos += targetObj->GetPosition();
                markColor = targetObj->m_fHealth * 0.001f;
            } else {
                effectPos = m_pTargetedObject->GetPosition();
                limitMarkColor = false; 
            }
        }
    }
    if (m_pTargetedObject) {
        uint8 r = 0, g = 0, b = 0;
        bool setRGB = true;
        if (limitMarkColor) {
            if (markColor > 0.0f)
                markColor = std::min(markColor, 1.0f);
            else
                setRGB = false;
        }
        if (setRGB) {
            r = static_cast<uint8>((1.0f - markColor) * 255.0f);
            g = static_cast<uint8>(markColor * 255.0f);
            b = static_cast<uint8>(0.0f);
        }
        CVector distance = effectPos - GetPosition();
        float size = 1.0f - distance.Magnitude() * 0.02f;
        CWeaponEffects::MarkTarget(m_nPedType, effectPos, r, g, b, 255u, size, false);
    }
    if (m_nMoveState != PEDMOVE_NONE) {
        if (m_nMoveState != PEDMOVE_RUN) {
            if (m_nMoveState != PEDMOVE_SPRINT)
                HandleSprintEnergy(false, 1.0f);
        } else if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > GetPlayerData()->m_fTimeCanRun)
            GetPlayerData()->m_fTimeCanRun += CTimer::GetTimeStep() * 0.15f;
    } else if (bInVehicle) {
        if (m_pVehicle && !m_pVehicle->IsSubBMX())
            HandleSprintEnergy(false, 1.0f);
    }
    GetActiveWeapon().Update(this);
    if (m_nPedState == PEDSTATE_DEAD || m_nPedState == PEDSTATE_DIE) {
        ClearWeaponTarget();
        return;
    }
    if (pad) {
        if (pad->WeaponJustDown(this)) {
            auto& activeWeapon = GetActiveWeapon();
            auto weaponType = activeWeapon.m_Type;
            if (!TheCamera.Using1stPersonWeaponMode() || activeWeapon.m_State == WEAPONSTATE_OUT_OF_AMMO) {
                if (!GetIntelligence()->GetTaskSwim()) {
                    if (weaponType == WEAPON_SNIPERRIFLE) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_SNIPERRIFFLE, 0.0f, 1.0f);
                    } else if (weaponType == WEAPON_RLAUNCHER || weaponType == WEAPON_RLAUNCHER_HS) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_ROCKET, 0.0f, 1.0f);
                    }
                }
            }
        }
        if (IsPedShootable() && this->m_nPedState != PEDSTATE_ANSWER_MOBILE) {
            int32 slot = CWorld::FindPlayerSlotWithPedPointer(this);
            if (!CWorld::Players[slot].m_pRemoteVehicle)
                ProcessWeaponSwitch(pad);
        }
    }
    ProcessAnimGroups();
    if (pad && TheCamera.GetActiveCamera().m_nMode == MODE_FOLLOWPED && !TheCamera.GetActiveCamera().m_nDirectionWasLooking) {
        auto& activeCam = TheCamera.GetActiveCamera();
        m_nLookTime = 0;
        float lookDir = CGeneral::LimitRadianAngle(atan2(-activeCam.m_vecFront.x, activeCam.m_vecFront.y));
        float angle = fabs(lookDir - m_fCurrentRotation);
        if (m_nPedState != PEDSTATE_ATTACK && angle > DegreesToRadians(30.0f) && angle < DegreesToRadians(330.0f)) {
            if (angle > DegreesToRadians(150.0f) && angle < DegreesToRadians(210.0f)) {
                float dir1 = CGeneral::LimitRadianAngle(m_fCurrentRotation - DegreesToRadians(150.0f));
                float dir2 = CGeneral::LimitRadianAngle(m_fCurrentRotation + DegreesToRadians(150.0f));
                lookDir = dir1;
                if (m_fLookDirection != 999'999.f && !bIsDucking) {
                    if (fabs(dir2 - m_fLookDirection) <= fabs(dir1 - m_fLookDirection))
                        lookDir = dir2;
                }
            }
            SetLookFlag(lookDir, true, false);
            SetLookTimer(static_cast<uint32>(CTimer::GetTimeStepInMS() * 5.0f));
        } else {
            ClearLookFlag();
        }
    }
    if (m_nMoveState == PEDMOVE_SPRINT && bIsLooking) {
        ClearLookFlag();
        SetLookTimer(250);
    }
    if (m_vecMoveSpeed.Magnitude() >= 0.1f) {
        GetPlayerData()->m_nStandStillTimer = 0;
        GetPlayerData()->m_bStoppedMoving = false;
    } else if (!GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_nStandStillTimer = CTimer::GetTimeInMS() + 500;
    } else if (CTimer::GetTimeInMS() > GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_bStoppedMoving = true;
    }
    if (GetPlayerData()->m_bDontAllowWeaponChange) {
        if (IsPlayer()) {
            if (!CPad::GetPad(0)->GetTarget())
                GetPlayerData()->m_bDontAllowWeaponChange = false;
        }
    }
    if (m_nPedState != PEDSTATE_SNIPER_MODE && GetActiveWeapon().m_State == WEAPONSTATE_FIRING)
        GetPlayerData()->m_nLastTimeFiring = CTimer::GetTimeInMS();
    ProcessGroupBehaviour(pad);
    if (bInVehicle)
        CCarCtrl::RegisterVehicleOfInterest(m_pVehicle);
    if (!GetIsVisible())
        UpdateRpHAnim();
    if (bInVehicle) {
        CPad* pad = CPad::GetPad(0);
        if (!pad->IsDPadDownPressed()) {
            if (pad->IsDPadUpPressed())
                GetPlayerData()->m_bPlayersGangActive = true;
        } else {
            GetPlayerData()->m_bPlayersGangActive = false;
        }
    }
    if (physicalFlags.bSubmergedInWater) {
        CVector pos = GetPosition();
        pos.z += 1.5f;
        if (CWaterLevel::GetWaterLevel(pos.x, pos.y, pos.z, GetPlayerData()->m_fWaterHeight, true, nullptr)) {
            auto& box = CEntity::GetColModel()->GetBoundingBox();
            float playerMinZ = pos.z + box.m_vecMin.z;
            float playerMaxZ = pos.z + box.m_vecMax.z;
            if (GetPlayerData()->m_fWaterHeight < playerMaxZ) {
                if (GetPlayerData()->m_fWaterHeight > playerMinZ)
                    GetPlayerData()->m_nWaterCoverPerc = static_cast<uint8>((GetPlayerData()->m_fWaterHeight - playerMinZ) / (playerMaxZ - playerMinZ) * 100.0f);
                else
                    GetPlayerData()->m_nWaterCoverPerc = 0;
            } else {
                GetPlayerData()->m_nWaterCoverPerc = 100;
            }
        } else {
            physicalFlags.bSubmergedInWater = false;
        }
    } else {
        GetPlayerData()->m_nWaterCoverPerc = 0;
    }
    if ((CTimer::GetFrameCounter() & 0x7F) == 0 && !FindPlayerVehicle()) {
        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        if (group.m_bMembersEnterLeadersVehicle) {
            int32 memberCount = group.m_groupMembership.CountMembersExcludingLeader();
            if (memberCount > 0) {
                float distance = group.FindDistanceToNearestMember(nullptr);
                if (distance > 20.0f && distance < 100.0f && CGame::currArea == AREA_CODE_NORMAL_WORLD) {
                    if (memberCount == 1)
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_ONE);
                    else
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_MANY);
                    for (int32 i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; ++i) {
                        CPed* member = group.m_groupMembership.GetMember(i);
                        if (member && CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.5f) {
                            int32 offset = CGeneral::GetRandomNumberInRange(3000, 4500);
                            member->Say(CTX_GLOBAL_FOLLOW_REPLY, offset);
                        }
                    }
                }
            }
        }
    }
    if (!bInVehicle && GetLightingTotal() <= 0.05f && !CEntryExitManager::WeAreInInteriorTransition())
        Say(CTX_GLOBAL_BREATHING);
}

// 0x609490
void CPlayerPed::SetMoveAnim() {
    //nop
}
