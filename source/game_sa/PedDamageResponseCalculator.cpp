#include "StdInc.h"
#include "PedStats.h"
#include "Tasks/TaskTypes/TaskSimpleFight.h"
#include "Localisation.h"
#include "Events/EventDamage.h"
#include "PedDamageResponse.h"

#include <reversiblebugfixes/Bugs.hpp>
#include "PedDamageResponseCalculator.h"

void CPedDamageResponseCalculator::InjectHooks() {
    RH_ScopedClass(CPedDamageResponseCalculator);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x4AD3F0);
    RH_ScopedInstall(AccountForPedDamageStats, 0x4AD430);
    RH_ScopedInstall(AccountForPedArmour, 0x4AD550);
    RH_ScopedInstall(ComputeWillForceDeath, 0x4AD610);
    RH_ScopedInstall(ComputeWillKillPed, 0x4B3210);
    RH_ScopedInstall(IsBleedingWeapon, 0x4B5C2A);
    RH_ScopedInstall(ComputeDamageResponse, 0x4B5AC0);
}

// 0x4AD3F0
CPedDamageResponseCalculator::CPedDamageResponseCalculator(const CEntity* entity, float fDamage, eWeaponType weaponType, ePedPieceTypes bodyPart, bool bSpeak) {
    m_pDamager      = entity;
    m_fDamageFactor = fDamage;
    m_bodyPart      = bodyPart;
    m_weaponType    = weaponType;
    m_bSpeak        = bSpeak;
}

CPedDamageResponseCalculator* CPedDamageResponseCalculator::Constructor(CEntity* entity, float fDamage, eWeaponType weaponType, ePedPieceTypes bodyPart, bool bSpeak) {
    this->CPedDamageResponseCalculator::CPedDamageResponseCalculator(entity, fDamage, weaponType, bodyPart, bSpeak);
    return this;
}

/*!
 *
 * @param ped
 * @param response
 * @addr 0x4AD430
 */
void CPedDamageResponseCalculator::AccountForPedDamageStats(CPed* ped, CPedDamageResponse& response) {
    const bool damageFactorUntouched = m_fDamageFactor == ms_damageFactor;

    m_fDamageFactor = ped->IsPlayer()
        ? m_fDamageFactor * 0.33f
        : ped->m_pStats->m_fDefendWeakness * m_fDamageFactor;

    const auto damager = static_cast<const CPed*>(m_pDamager);
    if (m_pDamager && m_pDamager->GetIsTypePed() && damager->IsPlayer()) {
        if (!CPedGroups::AreInSameGroup(ped, damager))
            return;
        if (m_weaponType == WEAPON_EXPLOSION || m_weaponType == WEAPON_FLAMETHROWER)
            return;
        if (damageFactorUntouched)
            return;
    } else {
        if (ped && ped->IsPlayer()) {
            if (!m_pDamager)
                return;
            if (m_pDamager->GetIsTypePed() && CPedIntelligence::AreFriends(*ped, *damager)) {
                goto applyDamageScale;
            }
        }
        if (!m_pDamager || !m_pDamager->GetIsTypePed())
            return;
        if (ped->GetCreatedBy() != PED_MISSION)
            return;
        if (!CPedGroups::AreInSameGroup(ped, damager))
            return;
        if (m_weaponType == WEAPON_EXPLOSION || m_weaponType == WEAPON_FLAMETHROWER)
            return;
    }
applyDamageScale:
    m_fDamageFactor = std::min(12.5f, m_fDamageFactor * 0.1f);
}

/*!
 *
 * @param ped
 * @param response
 * @addr 0x4AD550
 */
void CPedDamageResponseCalculator::AccountForPedArmour(CPed* ped, CPedDamageResponse& response) {
    if (ped->m_fArmour == 0.0f)
        return;

    if (m_weaponType == WEAPON_DROWNING || m_weaponType == WEAPON_FALL)
        return;

    if (FindPlayerPed() == ped)
        CWorld::Players[CWorld::PlayerInFocus].m_nLastTimeArmourLost = CTimer::GetTimeInMS();

    if (m_fDamageFactor > ped->m_fArmour) {
        m_fDamageFactor = m_fDamageFactor - ped->m_fArmour;
        response.m_fDamageArmor = ped->m_fArmour;
        ped->m_fArmour = 0.0f;
    } else {
        response.m_fDamageArmor = m_fDamageFactor;
        ped->m_fArmour = ped->m_fArmour - m_fDamageFactor;
        m_fDamageFactor = 0.0f;
    }
}

/*!
 *
 * @param ped
 * @addr Added in Android
 */
void CPedDamageResponseCalculator::AdjustPedDamage(CPed* ped) {
    // TODO: Reverted function from GTA SA 2.10 version ABI : arm64-v8a (x64)
    // offset: 0x444370

    if (!ped) {
        return;
    }

    const auto plyr = FindPlayerPed(-1);
    if (!plyr) {
        return;
    }

    const bool isUnarmedBrassOrParachute = m_weaponType == WEAPON_UNARMED
        || m_weaponType == WEAPON_BRASSKNUCKLE
        || m_weaponType == WEAPON_PARACHUTE;

    if (CCheat::m_aCheatsActive[CHEAT_COUNTRY_TRAFFIC] && m_weaponType <= eWeaponType::WEAPON_PARACHUTE && isUnarmedBrassOrParachute) {
        m_fDamageFactor = ped->m_fHealth;
    }

    if (plyr == ped && CTheScripts::pActiveScripts && !strcmp(CTheScripts::pActiveScripts->m_szName, "intro1")) {
        m_fDamageFactor = m_fDamageFactor * 0.77f;
    }
}

/*!
 *
 * @param ped
 * @param response
 * @return
 * @addr 0x4AD610
 */
bool CPedDamageResponseCalculator::ComputeWillForceDeath(CPed* ped, CPedDamageResponse& response) {
    if (ped->bNoCriticalHits) {
        return false;
    }

    const auto damagerPed = m_pDamager && m_pDamager->GetIsTypePed() ? static_cast<const CPed*>(m_pDamager) : nullptr;

    switch (m_weaponType) {
    case WEAPON_KATANA: {
        // The player's special katana moves have a chance to kill outright
        if (!damagerPed || !damagerPed->IsPlayer()) {
            return false;
        }
        const auto fight = const_cast<CPed*>(damagerPed)->GetIntelligence()->GetTaskFighting();
        if (!fight || fight->m_nComboSet != 11) {
            return false;
        }
        if (fight->m_nCurrentMove == 2 && (CGeneral::GetRandomNumber() & 1)) {
            return true;
        }
        return (CGeneral::GetRandomNumber() & 7) == 0;
    }
    case WEAPON_GRENADE:
    case WEAPON_RLAUNCHER:
    case WEAPON_RLAUNCHER_HS:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
    case WEAPON_EXPLOSION:
        if (!CLocalisation::KillPeds() || ped->IsPlayer() || ped->bInVehicle) {
            return false;
        }
        return ped->m_fHealth < m_fDamageFactor + 1.0f;
    case WEAPON_PISTOL:
    case WEAPON_PISTOL_SILENCED:
    case WEAPON_DESERT_EAGLE:
    case WEAPON_SHOTGUN:
    case WEAPON_SAWNOFF_SHOTGUN:
    case WEAPON_SPAS12_SHOTGUN:
    case WEAPON_MICRO_UZI:
    case WEAPON_MP5:
    case WEAPON_AK47:
    case WEAPON_M4:
    case WEAPON_TEC9:
    case WEAPON_COUNTRYRIFLE:
    case WEAPON_SNIPERRIFLE:
    case WEAPON_MINIGUN:
    case WEAPON_UZI_DRIVEBY: {
        // Headshots kill, but only now and then unless the ped is hit with a rifle
        uint32 chance = 1;
        if (!ped->IsPlayer() && !ped->bNoCriticalHits) {
            chance = m_weaponType == WEAPON_SNIPERRIFLE || m_weaponType == WEAPON_COUNTRYRIFLE
                ? 0
                : (uint32)(CGeneral::GetRandomNumber() & 7);
        }

        // Headshots always kill peds that are driving, and when the player aims on purpose
        const auto isDrivingNpc = ped->bInVehicle && ped->m_pVehicle && !ped->IsPlayer() && !ped->bNoCriticalHits;
        const auto isAimedAt    = !ped->IsPlayer()
            && damagerPed
            && damagerPed->IsPlayer()
            && (damagerPed->GetPlayerData()->m_nTargetBone == 5 || damagerPed->GetPlayerData()->m_bFreeAiming);
        if (!isDrivingNpc && !isAimedAt && chance != 0) {
            return false;
        }
        return m_bodyPart == PED_PIECE_HEAD;
    }
    default:
        return false;
    }
}

/*!
 *
 * @param ped
 * @param response
 * @param bSpeak
 * @addr 0x4B3210
 */
void CPedDamageResponseCalculator::ComputeWillKillPed(CPed* ped, CPedDamageResponse& rsp, bool bSpeak) {
    if (ped->IsPlayer()) {
        if (CCheat::IsActive(CHEAT_NO_ONE_CAN_STOP_US) && m_weaponType < WEAPON_LAST_WEAPON) {
            return;
        }
    }

    rsp.m_bForceDeath = ComputeWillForceDeath(ped, rsp);

    if (CCheat::IsActive(CHEAT_MEGA_PUNCH)) {
        if (notsa::contains({ WEAPON_UNARMED, WEAPON_BRASSKNUCKLE, WEAPON_PARACHUTE }, m_weaponType)) {
            m_fDamageFactor = ped->m_fHealth;
        }
    }

    if (!rsp.m_bForceDeath && m_weaponType == WEAPON_FALL && m_bSpeak) {
        const auto prevHP   = ped->m_fHealth;
        ped->m_fHealth      = std::max(prevHP - m_fDamageFactor, 5.0f);
        rsp.m_bHealthZero   = false;
        rsp.m_fDamageHealth = prevHP - ped->m_fHealth;
        if (bSpeak) {
            ped->Say(CTX_GLOBAL_PAIN_LOW, 0, 1.0, 0, 0, 0);
        }
        return;
    }

    if (rsp.m_bForceDeath || ped->m_fHealth - m_fDamageFactor < 1.0f) {
        rsp.m_fDamageHealth = ped->m_fHealth;
        rsp.m_bHealthZero   = true;
        ped->m_fHealth      = 0.0f;
        return;
    }

    rsp.m_bHealthZero   = false;
    rsp.m_fDamageHealth = m_fDamageFactor;
    ped->m_fHealth      = ped->m_fHealth - m_fDamageFactor;

    if (bSpeak) {
        ped->Say(
            m_weaponType == WEAPON_DROWNING
                ? CTX_GLOBAL_PAIN_CJ_DROWNING
                : m_fDamageFactor < 5.0f && ped->m_fHealth > 10.0f
                    ? CTX_GLOBAL_PAIN_LOW
                    : CTX_GLOBAL_PAIN_HIGH
        );
    }
}

/*!
 * NOTSA CAUTION: 0x4B5C2A is code inlined inside `ComputeDamageResponse` (0x4B5AC0), not a
 * standalone function - raw disasm shows it doesn't just return a bool, it also has a side effect
 * (writes `ped->field_0x72f = 0xC8` when true) and falls straight through into the rest of
 * `ComputeDamageResponse`'s body rather than returning. Do NOT flip this hook's `reversed` flag to
 * enable it until `ComputeDamageResponse` itself is reversed and its own call site replicates that
 * side effect/fallthrough - enabling this hook as-is would detour into the middle of the original
 * `ComputeDamageResponse` and silently drop both the field write and everything after it.
 *
 * @param ped
 * @return
 * @addr 0x4B5C2A inlined
 */
bool CPedDamageResponseCalculator::IsBleedingWeapon(CPed* ped) const {
    if (ped->IsPlayer())
        return false;

    if (m_weaponType == WEAPON_KNIFE || m_weaponType == WEAPON_KATANA || m_weaponType == WEAPON_CHAINSAW)
        return true;

    return false;
}

/*!
 *
 * @param ped
 * @param response
 * @param bSpeak
 * @addr 0x4B5AC0
 */
void CPedDamageResponseCalculator::ComputeDamageResponse(CPed* ped, CPedDamageResponse& response, bool bSpeak) {
    if (response.m_bDamageCalculated) {
        return;
    }
    response = CPedDamageResponse{};
    response.m_bDamageCalculated = true;
    response.m_bCheckIfAffectsPed = true;

    AccountForPedDamageStats(ped, response);
    AccountForPedArmour(ped, response);
    ComputeWillKillPed(ped, response, bSpeak);

    auto* const player = FindPlayerPed();

    // Hurting others as (or with the vehicle of) the player causes havoc
    if (m_pDamager) {
        if ((m_pDamager == player || m_pDamager == FindPlayerVehicle()) && m_pDamager != ped && response.m_fDamageHealth + response.m_fDamageArmor > 3.0f) {
            CWorld::Players[CWorld::PlayerInFocus].m_nHavocCaused++;
        }
    }

    if (ped == player) {
        player->AnnoyPlayerPed(false);
    }

    // The player's group reacts to the player getting hurt
    if (ped->IsPlayer() && m_pDamager && m_pDamager->GetIsTypePed()) {
        CEventDamage damageEvent{ const_cast<CEntity*>(m_pDamager), 0, m_weaponType, m_bodyPart, 0, false, ped->bInVehicle };
        ped->AsPlayer()->MakeGroupRespondToPlayerTakingDamage(damageEvent);
    }

    if (response.m_fDamageHealth + response.m_fDamageArmor > 0.0f) {
        if (!ped->IsPlayer() && (m_weaponType == WEAPON_KNIFE || m_weaponType == WEAPON_KATANA || m_weaponType == WEAPON_CHAINSAW)) {
            ped->field_72F = 200;
        }
        if (ped == player) {
            CWorld::Players[CWorld::PlayerInFocus].m_nLastTimeEnergyLost = CTimer::GetTimeInMS();
        }
        ped->m_nLastWeaponDamage = (char)m_weaponType;
        if (m_pDamager) {
            ped->m_pLastEntityDamage = const_cast<CEntity*>(m_pDamager);
            ped->m_pLastEntityDamage->RegisterReference(&ped->m_pLastEntityDamage);
            ped->field_768 = CTimer::GetTimeInMS();
        }
    }
}

