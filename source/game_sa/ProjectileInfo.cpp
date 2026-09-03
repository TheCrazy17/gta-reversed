#include "StdInc.h"

#include "ProjectileInfo.h"

#include "Entity/Object/Projectile.h"
#include "Radar.h"
#include "World.h"
#include "Pools/Pools.h"
#include "Collision/Box.h"
#include "Fx/FxManager.h"
#include "Fx/FxSystem.h"
#include "Explosion.h"
#include "Audio/AudioEngine.h"

void CProjectileInfo::InjectHooks() {
    RH_ScopedClass(CProjectileInfo);
    RH_ScopedCategoryGlobal();

    // Install("CProjectileInfo", "", , &CProjectileInfo::);
    RH_ScopedInstall(Initialise, 0x737B40);
    RH_ScopedInstall(Shutdown, 0x737BC0);
    RH_ScopedInstall(GetProjectileInfo, 0x737BF0);
    RH_ScopedInstall(RemoveNotAdd, 0x737C00);
    RH_ScopedInstall(AddProjectile, 0x737C80, { .reversed = false });
    RH_ScopedInstall(RemoveDetonatorProjectiles, 0x738860);
    RH_ScopedInstall(RemoveProjectile, 0x7388F0);
    RH_ScopedInstall(Update, 0x738B20, { .reversed = false });
    RH_ScopedInstall(IsProjectileInRange, 0x739860);
    RH_ScopedInstall(RemoveAllProjectiles, 0x7399B0);
    RH_ScopedInstall(RemoveIfThisIsAProjectile, 0x739A40);
    RH_ScopedInstall(RemoveFXSystem, 0x737B80);
}

// 0x737B40
void CProjectileInfo::Initialise() {
    ms_apProjectile.fill(nullptr);
    for (auto& info : gaProjectileInfo) {
        info.m_nWeaponType  = WEAPON_GRENADE;
        info.m_pCreator     = nullptr;
        info.m_nDestroyTime = 0;
        info.m_bActive      = false;
        info.m_pFxSystem    = nullptr;
    }
}

// 0x737BC0
void CProjectileInfo::Shutdown() {
    for (auto& info : gaProjectileInfo) {
        if (info.m_pFxSystem) {
            g_fxMan.DestroyFxSystem(info.m_pFxSystem);
            info.m_pFxSystem = nullptr;
        }
    }
}

// 0x737BF0
CProjectileInfo* CProjectileInfo::GetProjectileInfo(int32 infoId) {
    return &gaProjectileInfo[infoId];
}

// 0x737C00
void CProjectileInfo::RemoveNotAdd(CEntity* creator, eWeaponType weaponType, CVector pos) {
    eExplosionType explosionType;
    switch (weaponType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
        explosionType = EXPLOSION_GRENADE;
        break;
    case WEAPON_MOLOTOV:
        explosionType = EXPLOSION_MOLOTOV;
        break;
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
        explosionType = EXPLOSION_ROCKET;
        break;
    default:
        return;
    }

    CExplosion::AddExplosion(nullptr, creator, explosionType, pos, 0, true, -1.0f, false);
}

// 0x737C80
bool CProjectileInfo::AddProjectile(CEntity* creator, eWeaponType projectileType, CVector origin, float force, const CVector* dir, CEntity* target) {
    return plugin::CallAndReturn<bool, 0x737C80>(creator, projectileType, origin, force, dir, target);
}

// 0x738860
void CProjectileInfo::RemoveDetonatorProjectiles() {
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive || info.m_nWeaponType != WEAPON_REMOTE_SATCHEL_CHARGE) {
            continue;
        }

        CExplosion::AddExplosion(nullptr, info.m_pCreator, EXPLOSION_GRENADE, proj->GetPosition(), 0, true, -1.0f, false);

        info.m_bActive = false;
        info.RemoveFXSystem(false);

        proj->m_bRemoveFromWorld = true;
    }
}

// 0x7388F0
void CProjectileInfo::RemoveProjectile(CProjectileInfo* info, CProjectile* object) {
    const auto pos = object->GetPosition();

    switch (info->m_nWeaponType) {
    case WEAPON_GRENADE:
    case WEAPON_FREEFALL_BOMB:
        CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_GRENADE, pos, 0, true, -1.0f, false);
        break;
    case WEAPON_MOLOTOV:
        CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_MOLOTOV, pos, 0, true, -1.0f, false);
        AudioEngine.ReportObjectDestruction(object);
        break;
    case WEAPON_ROCKET: {
        auto* creator = info->m_pCreator;
        if (creator && creator->GetIsTypeVehicle()) {
            creator = creator->AsVehicle()->GetDriver();
        }
        CExplosion::AddExplosion(nullptr, creator, EXPLOSION_ROCKET, pos, 0, true, -1.0f, false);
        break;
    }
    case WEAPON_ROCKET_HS: {
        const auto explosionType = info->m_pCreator == FindPlayerPed() ? EXPLOSION_ROCKET : EXPLOSION_WEAK_ROCKET;
        CExplosion::AddExplosion(nullptr, info->m_pCreator, explosionType, pos, 0, true, -1.0f, false);
        break;
    }
    default:
        break;
    }

    info->m_bActive = false;
    info->RemoveFXSystem(false);

    CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(object));
    CWorld::Remove(object);
    delete object;
}

// 0x738B20
void CProjectileInfo::Update() {
    return plugin::Call<0x738B20>();
}

// 0x739860
bool CProjectileInfo::IsProjectileInRange(float x1, float x2, float y1, float y2, float z1, float z2, bool bDestroy) {
    const CBox bb{
        CVector{ x1, y1, z1 },
        CVector{ x2, y2, z2 }
    };
    bool found = false;
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        if (!IsWeaponTypeProjectile(static_cast<eWeaponType>(info.m_nWeaponType))) {
            continue;
        }

        if (!bb.IsPointInside(proj->GetPosition())) {
            continue;
        }

        found = true;
        if (bDestroy) {
            info.m_bActive = false;
            info.RemoveFXSystem(false);
            CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(proj));
            CWorld::Remove(proj);
            delete proj;
        }
    }
    return found;
}

// 0x7399B0
void CProjectileInfo::RemoveAllProjectiles() {
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        info.m_bActive = false;
        info.RemoveFXSystem(true);

        CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(proj));

        CWorld::Remove(proj);
        delete proj;
    }
}

// 0x739A40
bool CProjectileInfo::RemoveIfThisIsAProjectile(CObject* object) {
    for (auto i = 0u; i < MAX_PROJECTILES; i++) {
        if (ms_apProjectile[i] != object || !gaProjectileInfo[i].m_bActive) {
            continue;
        }

        gaProjectileInfo[i].m_bActive = false;
        gaProjectileInfo[i].RemoveFXSystem(false);

        CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(ms_apProjectile[i]));

        CWorld::Remove(ms_apProjectile[i]);
        delete ms_apProjectile[i];
        ms_apProjectile[i] = nullptr;
        return true;
    }
    return false;
}

// 0x737B80
void CProjectileInfo::RemoveFXSystem(bool bInstantly) {
    if (!m_pFxSystem) {
        return;
    }
    if (bInstantly) {
        g_fxMan.DestroyFxSystem(m_pFxSystem);
    } else {
        m_pFxSystem->Kill();
    }
    m_pFxSystem = nullptr;
}
