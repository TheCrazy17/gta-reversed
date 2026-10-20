#include "StdInc.h"

#include "ProjectileInfo.h"

#include "Entity/Object/Projectile.h"
#include "Entity/Ped/Ped.h"
#include "Entity/Vehicle/Vehicle.h"
#include "Radar.h"
#include "World.h"
#include "Pools/Pools.h"
#include "Collision/Box.h"
#include "Fx/Fx.h"
#include "Fx/FxManager.h"
#include "Fx/FxSystem.h"
#include "Fx/FxPrtMult.h"
#include "Explosion.h"
#include "Audio/AudioEngine.h"
#include "Weapon.h"
#include "General.h"
#include "Timer.h"

void CProjectileInfo::InjectHooks() {
    RH_ScopedClass(CProjectileInfo);
    RH_ScopedCategoryGlobal();

    // Install("CProjectileInfo", "", , &CProjectileInfo::);
    RH_ScopedInstall(Initialise, 0x737B40);
    RH_ScopedInstall(Shutdown, 0x737BC0);
    RH_ScopedInstall(GetProjectileInfo, 0x737BF0);
    RH_ScopedInstall(RemoveNotAdd, 0x737C00);
    RH_ScopedInstall(AddProjectile, 0x737C80);
    RH_ScopedInstall(RemoveDetonatorProjectiles, 0x738860);
    RH_ScopedInstall(RemoveProjectile, 0x7388F0);
    RH_ScopedInstall(Update, 0x738B20);
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
    static_assert(WEAPON_FLARE == 58 && WEAPON_REMOTE_SATCHEL_CHARGE == 39 && WEAPON_FREEFALL_BOMB == 21);

    int32   destroyTime{};
    float   elasticity{ 0.75f };
    bool    applyGravity{ true };
    bool    isThrown{};
    CVector velocity{};
    CMatrix mat;

    // Direction the creator is looking at, the thrown things go that way
    const auto SetupThrownMatrix = [&](bool turnIfVehicle) {
        auto heading = creator->GetHeading();
        if (turnIfVehicle && creator->GetIsTypeVehicle()) {
            heading = CGeneral::LimitRadianAngle(heading + PI);
        }
        mat.SetTranslate({});
        mat.RotateZ(heading);
        mat.GetPosition() += origin;
        return heading;
    };

    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE: {
        destroyTime = CTimer::GetTimeInMS() + 2000;

        auto speed = force == 0.0f ? 0.0f : force * 0.22f + 0.15f;
        if (projectileType == WEAPON_REMOTE_SATCHEL_CHARGE) {
            speed *= 0.5f;
        }
        const auto heading = SetupThrownMatrix(true);
        velocity           = CVector{ -std::sin(heading) * speed, std::cos(heading) * speed, (force + 1.0f) * 0.4f * speed };
        if (creator->m_nModelIndex == MODEL_SENTINEL) {
            velocity += creator->AsPhysical()->GetMoveSpeed();
        }
        isThrown   = true;
        elasticity = projectileType == WEAPON_REMOTE_SATCHEL_CHARGE ? 0.03f : 0.5f;
        break;
    }
    case WEAPON_TEARGAS: {
        destroyTime = CTimer::GetTimeInMS() + 20000;

        const auto speed   = force == 0.0f ? 0.0f : force * 0.22f + 0.15f;
        const auto heading = SetupThrownMatrix(false);
        velocity           = CVector{ -std::sin(heading) * speed, std::cos(heading) * speed, (force + 1.0f) * 0.4f * speed };
        isThrown   = true;
        elasticity = 0.5f;
        break;
    }
    case WEAPON_MOLOTOV: {
        destroyTime = CTimer::GetTimeInMS() + 2000;

        const auto speed   = std::max(force * 0.22f + 0.15f, 0.2f);
        const auto heading = SetupThrownMatrix(false);
        velocity           = CVector{ -std::sin(heading) * speed, std::cos(heading) * speed, (force * 0.2f + 0.4f) * speed };
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS: {
        auto speed = 0.4f;
        if (projectileType == WEAPON_ROCKET) {
            destroyTime = CTimer::GetTimeInMS() + 3000;
        } else {
            destroyTime = CTimer::GetTimeInMS() + 10000;
            speed       = 0.2f;
        }

        if (creator->GetIsTypeVehicle()) {
            mat                 = creator->GetMatrix();
            mat.GetPosition()   = origin;
            speed              += creator->AsPhysical()->GetMoveSpeed().Magnitude();
        } else if (creator->GetIsTypePed() && creator->AsPed()->IsPlayer()) {
            // Shot from the player, goes where the camera looks
            const auto& cam = TheCamera.GetActiveCam();
            mat.GetForward() = cam.m_vecFront;
            mat.GetUp()      = cam.m_vecUp;
            mat.GetRight()   = CrossProduct(cam.m_vecUp, cam.m_vecFront);
            mat.GetPosition() = origin;
        } else if (!dir) {
            mat = creator->GetMatrix();
        } else {
            mat.GetForward()  = *dir;
            mat.GetRight()    = creator->GetMatrix().GetRight();
            mat.GetUp()       = CrossProduct(mat.GetRight(), mat.GetForward());
            mat.GetPosition() = origin;
        }
        velocity     = mat.TransformVector({ 0.0f, speed, 0.0f });
        applyGravity = false;
        break;
    }
    case WEAPON_FREEFALL_BOMB:
    case WEAPON_FLARE: {
        if (projectileType == WEAPON_FREEFALL_BOMB) {
            destroyTime = CTimer::GetTimeInMS() + 2000000;
        } else {
            CStreaming::RequestModel(ModelIndices::MI_FLARE, 0);
            destroyTime = CTimer::GetTimeInMS() + 10000;
        }

        // Falls with the speed of what dropped it
        if (const auto type = creator->GetType(); type == ENTITY_TYPE_VEHICLE || type == ENTITY_TYPE_PED || type == ENTITY_TYPE_OBJECT) {
            velocity = creator->AsPhysical()->GetMoveSpeed();
        }
        mat               = creator->GetMatrix();
        mat.GetPosition() = origin;
        break;
    }
    default:
        break;
    }

    // Free slot for the projectile
    auto slot = 0u;
    while (slot < gaProjectileInfo.size() && gaProjectileInfo[slot].m_bActive) {
        slot++;
    }
    if (slot == gaProjectileInfo.size()) {
        return false;
    }

    CProjectile* proj{};
    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_TEARGAS:
    case WEAPON_MOLOTOV:
    case WEAPON_REMOTE_SATCHEL_CHARGE: {
        proj = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);

        // The projectile needs a sphere to collide with
        auto* const colModel = CModelInfo::GetModelInfo(proj->m_nModelIndex)->GetColModel();
        if (!colModel->m_pColData) {
            colModel->AllocateData(1, 0, 0, 0, 0, false);
            colModel->m_pColData->m_pSpheres[0].Set(colModel->GetBoundRadius() * 0.75f, colModel->GetBoundCenter(), (eSurfaceType)0x38, 0, tColLighting{ 0xFF });
        } else if (colModel->m_pColData->m_nNumSpheres == 0 && !colModel->m_pColData->m_pSpheres) {
            colModel->m_pColData->m_nNumSpheres = 1;
            colModel->m_pColData->m_pSpheres    = (CColSphere*)CMemoryMgr::Malloc(sizeof(CColSphere), 0);
            colModel->m_pColData->m_pSpheres[0].Set(colModel->GetBoundRadius() * 0.75f, colModel->GetBoundCenter(), (eSurfaceType)0x38, 0, tColLighting{ 0xFF });
        }
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
    case WEAPON_FREEFALL_BOMB:
        proj = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);
        break;
    case WEAPON_FLARE:
        proj = new CProjectile(ModelIndices::MI_FLARE);
        proj->m_fAirResistance = 0.9f;
        break;
    default:
        break;
    }
    ms_apProjectile[slot] = proj;
    if (!proj) {
        return false;
    }

    auto& info         = gaProjectileInfo[slot];
    info.m_nWeaponType = projectileType;
    info.m_pCreator    = creator;
    creator->RegisterReference(&info.m_pCreator);

    proj->SetMatrix(mat);
    proj->m_vecMoveSpeed                = velocity;
    proj->physicalFlags.bApplyGravity   = applyGravity;
    info.m_nDestroyTime                 = destroyTime;
    proj->m_fElasticity                 = elasticity;
    if (isThrown) {
        proj->m_pObjectInfo = &CObjectData::GetAtIndex(4);
    }

    info.m_pVictim = target;
    if (target) {
        target->RegisterReference(&info.m_pVictim);
    }
    info.m_bActive = true;

    CWorld::Add(proj);
    proj->RegisterReference(reinterpret_cast<CEntity**>(&ms_apProjectile[slot]));
    info.m_vecLastPosn = proj->GetPosition();

    if (projectileType == WEAPON_TEARGAS) {
        if (const auto matrix = proj->GetModellingMatrix()) {
            info.m_pFxSystem = g_fxMan.CreateFxSystem("teargasAD", CVector{}, matrix, false);
            if (info.m_pFxSystem) {
                info.m_pFxSystem->Play();
            }
        }
    }

    proj->m_pEntityIgnoredCollision          = creator;
    proj->physicalFlags.bCanBeCollidedWith   = true;
    if (const auto type = creator->GetType(); type == ENTITY_TYPE_VEHICLE || type == ENTITY_TYPE_PED || type == ENTITY_TYPE_OBJECT) {
        if (!creator->AsPhysical()->m_pEntityIgnoredCollision) {
            creator->AsPhysical()->m_pEntityIgnoredCollision = creator;
        }
    }

    // The heat seeking rockets can be seen on the radar
    if (projectileType == WEAPON_ROCKET_HS) {
        const auto blip = CRadar::SetEntityBlip(BLIP_OBJECT, GetObjectPool()->GetRef(proj), 0xFF0000FF, BLIP_DISPLAY_BLIPONLY);
        CRadar::ChangeBlipScale(blip, 1);
        CRadar::ChangeBlipColour(blip, (creator == FindPlayerPed(-1) || creator == FindPlayerVehicle(-1, false)) ? (eBlipColour)0xFFFFFFFF : (eBlipColour)0xFF0000FF);
    }

    AudioEngine.ReportWeaponEvent(0x94, projectileType, proj);
    return true;
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
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        // If we've gone underwater kill our trail/burn effect immediately
        if (proj->physicalFlags.bSubmergedInWater && info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }

        // Clear a dangling creator ped ref (Eg.: Ped got deleted/reused this frame)
        if (info.m_pCreator && info.m_pCreator->GetIsTypePed() && !info.m_pCreator->AsPed()->IsPointerValid()) {
            info.m_pCreator = nullptr;
        }

        if (info.m_nWeaponType == WEAPON_REMOTE_SATCHEL_CHARGE || info.m_nWeaponType == WEAPON_GRENADE || info.m_nWeaponType == WEAPON_TEARGAS) {
            // Once it's basically come to rest reduce elasticity so it stops residually bouncing
            if (proj->m_fElasticity > 0.1f
                && std::abs(proj->m_vecMoveSpeed.x) < 0.05f
                && std::abs(proj->m_vecMoveSpeed.y) < 0.05f
                && std::abs(proj->m_vecMoveSpeed.z) < 0.05f) {
                proj->m_fElasticity = 0.03f;
            }

            // Tear gas periodically makes nearby peds choke, starting a bit after it lands
            if (info.m_nWeaponType == WEAPON_TEARGAS
                && info.m_nDestroyTime - 0x445Cu < CTimer::GetTimeInMS()
                && CGeneral::GetRandomNumberInRange(0, 100) < 10) {
                const auto pos = proj->GetPosition();
                CWorld::SetPedsChoking(pos.x, pos.y, pos.z, 6.0f, info.m_pCreator);
            }
        }

        // Rocket exhaust trail particles, scattered perpendicular to the direction of travel
        if (info.m_nWeaponType == WEAPON_ROCKET || info.m_nWeaponType == WEAPON_ROCKET_HS) {
            static const FxPrtMult_c mult{ 0.3f, 0.3f, 0.3f, 0.3f, 0.5f, 1.0f, 0.08f };

            const auto moveDelta = proj->m_vecMoveSpeed * CTimer::GetTimeStep();
            const auto numParticles = std::max(1L, std::lround(moveDelta.Magnitude()));
            for (auto j = 0L; j < numParticles; j++) {
                const auto t = 1.0f - (float)j / (float)numParticles;
                const auto trailPos = proj->GetPosition() - moveDelta * t;

                CVector randDir{
                    2.0f * rand() / RAND_MAX - 1.0f,
                    2.0f * rand() / RAND_MAX - 1.0f,
                    2.0f * rand() / RAND_MAX - 1.0f
                };
                randDir.Normalise();

                auto velDir = proj->m_vecMoveSpeed;
                velDir.Normalise();

                CVector scatterVel;
                CrossProduct(&scatterVel, &velDir, &randDir);
                scatterVel *= 1.5f;

                g_fx.m_SmokeHuge->AddParticle(trailPos, scatterVel, 0.0f, mult);
            }
        }

        // Checks LOS between last and current position (Eg.: Did we just fly into something?),
        // and removes the projectile unless what we hit is our own creator or another missile.
        const auto CheckForImpact = [&] {
            const auto curPos = proj->GetPosition();
            if (!proj->physicalFlags.bOnSolidSurface) {
                CWorld::pIgnoreEntity = info.m_pCreator;
                proj->SetUsesCollision(false);
                const auto losClear = CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, curPos, true, true, true, true, false, false, false);
                CWorld::pIgnoreEntity = nullptr;
                proj->SetUsesCollision(true);
                proj->m_pEntityIgnoredCollision = info.m_pCreator;
                if (losClear) {
                    return false;
                }
            }
            const auto* hit = proj->m_nNumEntitiesCollided ? proj->m_apCollidedEntities[0] : nullptr;
            if (hit && (hit == info.m_pCreator || hit->m_nModelIndex == MODEL_MISSILE)) {
                return false;
            }
            RemoveProjectile(&info, proj);
            return true;
        };

        if (info.m_nDestroyTime != 0 && (uint32)info.m_nDestroyTime < CTimer::GetTimeInMS()) {
            // Expired - remove it, unless it's a detonator charge (those don't expire on their own)
            if (info.m_nWeaponType == WEAPON_REMOTE_SATCHEL_CHARGE) {
                if (info.m_pCreator && info.m_pCreator->GetIsTypePed() && info.m_pCreator->AsPed()->IsPlayer()) {
                    const auto& detonator = info.m_pCreator->AsPed()->GetWeapon(WEAPON_DETONATOR);
                    if (detonator.m_Type != WEAPON_DETONATOR || detonator.m_TotalAmmo == 0) {
                        info.m_nDestroyTime = 0; // Player no longer has a detonator on them - let it live forever (harmless)
                    }
                }
            } else {
                RemoveProjectile(&info, proj);
                continue;
            }
        } else {
            switch (info.m_nWeaponType) {
            case WEAPON_ROCKET: {
                proj->m_vecMoveSpeed += proj->m_matrix->GetForward() * (CTimer::GetTimeStep() * GAME_GRAVITY);
                if (const auto speed = proj->m_vecMoveSpeed.Magnitude(); speed > 9.9f) {
                    proj->m_vecMoveSpeed *= 9.9f / speed;
                }
                if (CheckForImpact()) {
                    continue;
                }
                break;
            }
            case WEAPON_FLARE: {
                const auto curPos = proj->GetPosition();
                CWorld::pIgnoreEntity = info.m_pCreator;
                proj->SetUsesCollision(false);
                const auto losClear = CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, curPos, true, true, true, true, false, false, false);
                proj->SetUsesCollision(true);
                CWorld::pIgnoreEntity = nullptr;
                if (!losClear) {
                    proj->m_vecMoveSpeed = CVector{};
                    proj->GetPosition() = info.m_vecLastPosn;
                }
                break;
            }
            case WEAPON_MOLOTOV:
            case WEAPON_FREEFALL_BOMB: {
                const auto curPos = proj->GetPosition();
                CWorld::pIgnoreEntity = info.m_pCreator;
                proj->SetUsesCollision(false);

                const auto checkImpact = !info.m_pCreator
                    || (info.m_vecLastPosn - info.m_pCreator->GetPosition()).SquaredMagnitude() >= 2.0f;
                const auto shouldRemove = checkImpact
                    && (proj->physicalFlags.bOnSolidSurface || !CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, curPos, true, true, true, true, false, false, false));

                CWorld::pIgnoreEntity = nullptr;
                proj->SetUsesCollision(true);

                if (shouldRemove) {
                    RemoveProjectile(&info, proj);
                    continue;
                }
                break;
            }
            case WEAPON_ROCKET_HS: {
                // Warn the player if this heat-seeking rocket is locked onto them
                if (info.m_pVictim && info.m_pVictim == FindPlayerVehicle()) {
                    AudioEngine.ReportFrontendAudioEvent(AE_MISSILE_LOCK, 0.0f, 1.0f);
                }

                const auto& fwd = proj->m_matrix->GetForward();
                const auto projPos = proj->GetPosition();
                const auto aimOrigin = projPos + fwd;

                const auto origRating = CWeapon::EvaluateTargetForHeatSeekingMissile(info.m_pVictim, aimOrigin, fwd, 1.2f, true, nullptr);

                // Flares dropped as countermeasures can steal the lock away from the original target
                CEntity* bestFlare  = nullptr;
                auto     bestRating = 0.0f;
                for (auto&& [flareInfo, flareObj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
                    if (flareInfo.m_nWeaponType != WEAPON_FLARE || !flareInfo.m_bActive) {
                        continue;
                    }
                    if (const auto rating = CWeapon::EvaluateTargetForHeatSeekingMissile(flareObj, aimOrigin, fwd, 1.2f, true, nullptr); bestRating <= rating) {
                        bestRating = rating;
                        bestFlare  = flareObj;
                    }
                }

                auto* target = (bestFlare && bestRating > origRating) ? bestFlare : info.m_pVictim;

                // Player-fired missiles get a much tighter, faster-locking behaviour against planes specifically
                const auto isPlayerVsPlane = target->GetIsTypeVehicle()
                    && target->AsVehicle()->m_nVehicleSubType == VEHICLE_TYPE_PLANE
                    && (info.m_pCreator == FindPlayerPed() || info.m_pCreator == FindPlayerVehicle());

                auto  leadPos     = projPos + proj->m_vecMoveSpeed * 100.0f;
                if (isPlayerVsPlane) {
                    leadPos = projPos;
                }

                const auto targetDist        = (projPos - target->GetPosition()).Magnitude();
                const auto leadTime          = std::min(targetDist, isPlayerVsPlane ? 1.5f : 50.0f);
                const auto targetLeadPos     = target->GetPosition() + target->AsPhysical()->m_vecMoveSpeed * leadTime;

                auto desiredDir = targetLeadPos - leadPos;
                auto curVelDir  = proj->m_vecMoveSpeed;
                curVelDir.Normalise();

                if (const auto dot = DotProduct(&desiredDir, &curVelDir); dot < 0.0f) {
                    desiredDir -= curVelDir * dot;
                }
                desiredDir.Normalise();

                auto  velDamping = 1.0f;
                auto  turnRate   = (info.m_pCreator == FindPlayerPed() || info.m_pCreator == FindPlayerVehicle()) ? 0.0117f : 0.009f;
                if (target->AsPhysical()->m_vecMoveSpeed.Magnitude() > 0.8f) {
                    turnRate *= 1.2f;
                }
                if (isPlayerVsPlane) {
                    velDamping = std::pow(0.95f, CTimer::GetTimeStep());
                    turnRate   = 0.15f;
                }

                const auto oldVel  = proj->m_vecMoveSpeed;
                proj->m_vecMoveSpeed *= velDamping;
                proj->m_vecMoveSpeed += desiredDir * (turnRate * CTimer::GetTimeStep());
                if (const auto speed = proj->m_vecMoveSpeed.Magnitude(); speed > 9.9f) {
                    proj->m_vecMoveSpeed *= 9.9f / speed;
                }
                proj->m_matrix->GetForward() = oldVel;

                if (CheckForImpact()) {
                    continue;
                }
                break;
            }
            default: {
                // Remaining weapon types (WEAPON_GRENADE, WEAPON_TEARGAS, WEAPON_REMOTE_SATCHEL_CHARGE):
                // A satchel charge sticks to whatever last damaged it (Eg.: the ped/vehicle it hit)
                if (info.m_nWeaponType == WEAPON_REMOTE_SATCHEL_CHARGE
                    && proj->m_fDamageIntensity > 0.0f
                    && proj->m_pDamageEntity
                    && !proj->m_pAttachedTo) {
                    proj->AttachEntityToEntity(proj->m_pDamageEntity->AsPhysical(), nullptr, nullptr);
                    proj->SetUsesCollision(false);
                }
                break;
            }
            }
        }

        info.m_vecLastPosn = proj->GetPosition();
    }
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
