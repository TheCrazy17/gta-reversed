#include "StdInc.h"

#include "TaskSimpleDead.h"

#include "CarEnterExit.h"
#include "WeaponInfo.h"
#include "AccidentManager.h"
#include "Localisation.h"
#include "WaterLevel.h"
#include "Shadows.h"
#include "Events/EventDeadPed.h"
#include "Events/EventGroup.h"

void CTaskSimpleDead::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleDead, 0x86DEA4, 9);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(ProcessPed, 0x630600);
}

// NOTSA: *deathTime* originally int32
// 0x630590
CTaskSimpleDead::CTaskSimpleDead(uint32 deathTime, bool hasDrowned) :
    m_nDeathTimeMS{deathTime},
    m_bHasDrowned{hasDrowned}
{
}

// 0x636100
CTaskSimpleDead::CTaskSimpleDead(const CTaskSimpleDead& o) :
    CTaskSimpleDead{o.m_nDeathTimeMS, o.m_bHasDrowned}
{
}

// 0x630600
bool CTaskSimpleDead::ProcessPed(CPed* ped) {
    bool bIsPlayerTarget      = false; // Is `ped` currently the look/lock-on target of a local player
    bool bWasStandingOnGround = false;

    if (m_bFirstTime) {
        if (ped->bInVehicle) {
            const auto targetDoor = CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped); // NOTSA: door-id enum not identified; raw int
            const auto animId     = (targetDoor == 10 || targetDoor == 11) ? ANIM_ID_CAR_DEAD_LHS : ANIM_ID_CAR_DEAD_RHS;
            CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_NONE, animId, 4.0f);
        }

        ped->SetPedState(PEDSTATE_DEAD);
        m_bFirstTime = false;

        if (ped->bIsStanding || ped->bWasStanding) {
            bWasStandingOnGround = true;
        }

        if (FindPlayerPed(0)->m_pTargetedObject == ped ||
            (FindPlayerPed(1) && FindPlayerPed(1)->m_pTargetedObject == ped)) {
            bIsPlayerTarget = true;
        }

        if (!m_bHasDrowned && !ped->m_standingOnEntity && !bIsPlayerTarget) {
            ped->m_bUsesCollision = false;
        }

        ped->m_fHealth = 0.0f;

        const auto weaponInfo = CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().GetType(), eWeaponSkill::STD);
        ped->RemoveWeaponModel(weaponInfo->m_nModelId1);
        ped->m_nActiveWeaponSlot = 0;

        if (!ped->IsPlayer()) {
            ped->RemoveWeaponAnims(0, -1000.0f);
            ped->CreateDeadPedWeaponPickups();
            ped->CreateDeadPedMoney();
        }

        {
            CEventDeadPed deathEvent{ped, m_bHasDrowned, m_nDeathTimeMS};
            GetEventGlobalGroup()->Add(&deathEvent, false);
        }
        CAccidentManager::GetInstance()->ReportAccident(ped);
    }

    bool bDoSlopeCalc = true;
    if (!m_bFirstTime && m_bHasDrowned && ped->bIsStanding) {
        ped->m_bUsesCollision = false;
        m_bHasDrowned = false;
        const auto assoc = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_NONE, ANIM_ID_FLOOR_HIT_F, 8.0f);
        assoc->SetFlag(ANIMATION_IS_FINISH_AUTO_REMOVE, false);
    } else if (!ped->m_bUsesCollision || m_bHasDrowned || !ped->bIsStanding || ped->m_standingOnEntity ||
               FindPlayerPed(0)->m_pTargetedObject == ped ||
               (FindPlayerPed(1) && FindPlayerPed(1)->m_pTargetedObject == ped) || bIsPlayerTarget) {
        bDoSlopeCalc = bWasStandingOnGround;
    } else {
        ped->m_bUsesCollision = false;
    }

    if (bDoSlopeCalc) {
        // NOTSA: `field_578` semantics unclear (unnamed CVector on CPed) -- likely a death-impulse/impact direction.
        const auto dot     = DotProduct(ped->field_578, ped->GetUp());
        const auto clamped = std::clamp(dot, -1.0f, 1.0f);
        ped->m_pedIK.m_fSlopeRoll = ped->m_pedIK.m_fSlopePitch = std::acos(clamped); // disasm computes this identical expr twice
    }

    ped->DeadPedMakesTyresBloody();

    if (CLocalisation::Blood() && !m_bHasDrowned) {
        const uint32 timeSinceDeath = CTimer::GetTimeInMS() - m_nDeathTimeMS;

        float radius = 0.0f;
        if (timeSinceDeath > 2000) {
            radius = (timeSinceDeath < 7001) ? float(timeSinceDeath - 2000) * 0.00015f : 0.75f;
        }

        for (const auto candidate : std::span{ped->GetIntelligence()->GetPedEntities(), MAX_NUM_ENTITIES}) {
            if (!candidate) {
                continue;
            }
            if ((ped->GetPosition() - candidate->GetPosition()).SquaredMagnitude() < radius * radius) {
                const auto candidatePed = static_cast<CPed*>(candidate); // NOTSA: assumed safe, array is filled by CPedScanner
                candidatePed->bDoBloodyFootprints = true;
                candidatePed->m_nDeathTimeMS = 200; // NOTSA: verified offset (CPed::m_nDeathTimeMS) but semantically odd for a LIVING nearby ped
            }
        }

        if (timeSinceDeath > 2000 && !m_bBloodPuddleCreated) {
            auto pedPos = ped->GetPosition();

            // One-shot check gated to fire exactly on the frame `timeSinceDeath` crosses 2000ms (frame-delta-window based, not an FPS bug)
            if (timeSinceDeath - 2000 <= CTimer::GetTimeInMS() - CTimer::GetPreviousTimeInMS()) {
                float waterLevel;
                if (CWaterLevel::GetWaterLevelNoWaves(pedPos, &waterLevel) && pedPos.z < waterLevel) {
                    m_bBloodPuddleCreated = true; // suppress blood spawn: ped is underwater
                }
            }

            if (!m_bBloodPuddleCreated && CLocalisation::Blood()) {
                const auto age = timeSinceDeath - 2000;
                if (age < 5000) {
                    const auto t = float(age);
                    // NOTSA: LOW CONFIDENCE. `id` = reinterpret_cast<uint32>(this)+0x11, i.e. one byte past
                    // CTaskSimpleDead's own 16-byte size -- verified positionally against StoreStaticShadow's
                    // real signature, but the *purpose* of this "id" value is not understood (StoreStaticShadow
                    // itself is still unreversed, so there's no trusted implementation to cross-check semantics
                    // against). Treated as an opaque id, not a memory read.
                    CShadows::StoreStaticShadow(
                        reinterpret_cast<uint32>(reinterpret_cast<const uint8*>(this) + 0x11), SHADOW_DEFAULT, gpBloodPoolTex, pedPos,
                        -0.00015f * t, 0.0f, 0.0f, 0.00015f * t,
                        255, 200, 0, 0,
                        4.0f, 1.0f, 40.0f, false, 0.0f
                    );
                } else {
                    CShadows::AddPermanentShadow(
                        SHADOW_DEFAULT, gpBloodPoolTex, &pedPos,
                        0.75f, 0.0f, 0.0f, -0.75f,
                        255, 200, 0, 0,
                        4.0f, 40000, 1.0f
                    );
                    m_bBloodPuddleCreated = true;
                }
            }
        }
    }

    if (!m_bHasDrowned) {
        ped->m_pedIK.bSlopePitch = true;
        ped->m_vecMoveSpeed = CVector{};
    } else {
        ped->bIsStanding = false;
        ped->bWasStanding = false;
    }

    return false;
}
