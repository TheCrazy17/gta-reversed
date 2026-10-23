/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Fx.h"
#include "Blueprints/FxInfoAttractLine.h"
#include "Shadows.h"

static auto& TempVertexBuffer = StaticRef<std::array<RxObjSpace3DVertex, 4>>(0xC4D958);

auto& g_fx = StaticRef<Fx_c>(0xA9AE00);

void Fx_c::InjectHooks() {
    RH_ScopedClass(Fx_c);
    RH_ScopedCategory("Fx");

    // + RH_ScopedInstall(Constructor, 0x49E620);
    // + RH_ScopedInstall(Destructor, 0x49E630);
    // + RH_ScopedInstall(InitStaticSystems, 0x49E660);
    // + RH_ScopedInstall(ExitStaticSystems, 0x49E850);
    // + RH_ScopedInstall(InitEntitySystems, 0x49EA60);
    // + RH_ScopedInstall(ExitEntitySystems, 0x4A12D0);
    // + RH_ScopedInstall(Init, 0x49EA90);
    // + RH_ScopedInstall(Exit, 0x4A1320);
    // + RH_ScopedInstall(Reset, 0x49EAE0);
    // + RH_ScopedInstall(CreateEntityFx, 0x4A11E0);
    // + RH_ScopedInstall(DestroyEntityFx, 0x4A1280);
    // + RH_ScopedInstall(Update, 0x49E640);
    // + RH_ScopedInstall(Render, 0x49E650);
    RH_ScopedInstall(CreateMatFromVec, 0x49E950);
    RH_ScopedGlobalInstall(FindClosestPtOnLine, 0x4A4880);
    // + RH_ScopedInstall(SetFxQuality, 0x49EA40);
    // + RH_ScopedInstall(GetFxQuality, 0x49EA50);
    RH_ScopedInstall(AddBlood, 0x49EB00);
    RH_ScopedInstall(AddWood, 0x49EE10);
    RH_ScopedInstall(AddSparks, 0x49F040);
    RH_ScopedInstall(AddTyreBurst, 0x49F300);
    RH_ScopedInstall(AddBulletImpact, 0x49F3D0);
    RH_ScopedInstall(AddPunchImpact, 0x49F670);
    RH_ScopedInstall(AddDebris, 0x49F750);
    RH_ScopedInstall(AddGlass, 0x49F970);
    RH_ScopedInstall(AddWheelSpray, 0x49FB30);
    RH_ScopedInstall(AddWheelGrass, 0x49FF20);
    RH_ScopedInstall(AddWheelGravel, 0x4A0170);
    RH_ScopedInstall(AddWheelMud, 0x4A03C0);
    RH_ScopedInstall(AddWheelSand, 0x4A0610);
    RH_ScopedInstall(AddWheelDust, 0x4A09C0);
    RH_ScopedInstall(TriggerWaterHydrant, 0x4A0D70);
    RH_ScopedInstall(TriggerGunshot, 0x4A0DE0);
    RH_ScopedInstall(TriggerTankFire, 0x4A0FA0);
    RH_ScopedInstall(TriggerWaterSplash, 0x4A1070);
    RH_ScopedInstall(TriggerBulletSplash, 0x4A10E0);
    RH_ScopedInstall(TriggerFootSplash, 0x4A1150);

    RH_ScopedGlobalInstall(RenderAddTri_, 0x4A1410);
    RH_ScopedGlobalInstall(RenderEnd, 0x4A1600);
    RH_ScopedGlobalInstall(RenderBegin, 0x4A13B0);
    RH_ScopedGlobalInstall(RotateVecIntoVec, 0x4A1660);
    RH_ScopedGlobalInstall(RotateVecAboutVec, 0x4A1780);
}

Fx_c* Fx_c::Constructor() { this->Fx_c::Fx_c(); return this; }
Fx_c* Fx_c::Destructor() { this->Fx_c::~Fx_c(); return this; }

// 0x49E660
void Fx_c::InitStaticSystems() {
    m_Blood          = g_fxMan.CreateFxSystem("prt_blood",            CVector{}, nullptr, true);
    m_BoatSplash     = g_fxMan.CreateFxSystem("prt_boatsplash",       CVector{}, nullptr, true);
    m_Bubble         = g_fxMan.CreateFxSystem("prt_bubble",           CVector{}, nullptr, true);
    m_Cardebris      = g_fxMan.CreateFxSystem("prt_cardebris",        CVector{}, nullptr, true);
    m_CollisionSmoke = g_fxMan.CreateFxSystem("prt_collisionsmoke",   CVector{}, nullptr, true);
    m_GunShell       = g_fxMan.CreateFxSystem("prt_gunshell",         CVector{}, nullptr, true);
    m_Sand           = g_fxMan.CreateFxSystem("prt_sand",             CVector{}, nullptr, true);
    m_Sand2          = g_fxMan.CreateFxSystem("prt_sand2",            CVector{}, nullptr, true);
    m_SmokeHuge      = g_fxMan.CreateFxSystem("prt_smoke_huge",       CVector{}, nullptr, true);
    m_SmokeII3expand = g_fxMan.CreateFxSystem("prt_smokeII_3_expand", CVector{}, nullptr, true);
    m_Spark          = g_fxMan.CreateFxSystem("prt_spark",            CVector{}, nullptr, true);
    m_Spark2         = g_fxMan.CreateFxSystem("prt_spark_2",          CVector{}, nullptr, true);
    m_Splash         = g_fxMan.CreateFxSystem("prt_splash",           CVector{}, nullptr, true);
    m_Wake           = g_fxMan.CreateFxSystem("prt_wake",             CVector{}, nullptr, true);
    m_WaterSplash    = g_fxMan.CreateFxSystem("prt_watersplash",      CVector{}, nullptr, true);
    m_WheelDirt      = g_fxMan.CreateFxSystem("prt_wheeldirt",        CVector{}, nullptr, true);
    m_Glass          = g_fxMan.CreateFxSystem("prt_glass",            CVector{}, nullptr, true);
}

// 0x49E850
void Fx_c::ExitStaticSystems() {
    g_fxMan.DestroyFxSystem(m_Blood);
    g_fxMan.DestroyFxSystem(m_BoatSplash);
    g_fxMan.DestroyFxSystem(m_Bubble);
    g_fxMan.DestroyFxSystem(m_Cardebris);
    g_fxMan.DestroyFxSystem(m_CollisionSmoke);
    g_fxMan.DestroyFxSystem(m_GunShell);
    g_fxMan.DestroyFxSystem(m_Sand);
    g_fxMan.DestroyFxSystem(m_Sand2);
    g_fxMan.DestroyFxSystem(m_SmokeHuge);
    g_fxMan.DestroyFxSystem(m_SmokeII3expand);
    g_fxMan.DestroyFxSystem(m_Spark);
    g_fxMan.DestroyFxSystem(m_Spark2);
    g_fxMan.DestroyFxSystem(m_Splash);
    g_fxMan.DestroyFxSystem(m_Wake);
    g_fxMan.DestroyFxSystem(m_WaterSplash);
    g_fxMan.DestroyFxSystem(m_WheelDirt);
    g_fxMan.DestroyFxSystem(m_Glass);
}

// 0x49EA60
void Fx_c::InitEntitySystems() {
    // NOP
}

// NOTSA
static void CreateFxWithinCameraRange(const char* name, const CVector& pos, float range) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) <= range) {
        if (auto* fxSystem = g_fxMan.CreateFxSystem(name, pos, nullptr, false)) {
            fxSystem->PlayAndKill();
        }
    }
}

// 0x4A12D0
void Fx_c::ExitEntitySystems() {
    for (auto it = m_FxEntities.GetHead(); it; it = m_FxEntities.GetNext(it)) {
        m_FxEntities.RemoveItem(it);
        g_fxMan.DestroyFxSystem(it->m_System);
        delete it;
    }
}

// 0x49EA90
void Fx_c::Init() {
    ZoneScoped;

    g_fxMan.Init();
    g_fxMan.LoadFxProject("models\\effects.fxp");
    g_fxMan.SetWindData(&CWeather::WindDir, &CWeather::Wind);
    InitStaticSystems();
    InitEntitySystems();
    m_Randomizer = 0;
}

// 0x4A1320
void Fx_c::Exit() {
    ExitEntitySystems();
    ExitStaticSystems();
    g_fxMan.Exit();
}

// 0x49EAE0
void Fx_c::Reset() {
    g_fxMan.DestroyAllFxSystems();
    InitStaticSystems();
    InitEntitySystems(); // NOTSA
}

// 0x4A11E0
void Fx_c::CreateEntityFx(CEntity* entity, const char* fxName, const CVector& pos, RwMatrix* transform) {
    // ((void(__thiscall*)(Fx_c*, CEntity*, char*, RwV3d*, RwMatrix*))0x4A11E0)(this, entity, fxName, pos transform);

    auto* particle = g_fxMan.CreateFxSystem(fxName, pos, transform, true);
    if (particle) {
        auto it = new FxEntitySystem();
        it->m_System = particle;
        it->m_Entity = entity;
        m_FxEntities.AddItem(it);
        it->m_System->Play();
    }
}

// 0x4A1280
void Fx_c::DestroyEntityFx(CEntity* entity) {
    // ((void(__thiscall*)(Fx_c*, CEntity*))0x4A1280)(this, entity);

    for (auto it = m_FxEntities.GetHead(); it; it = m_FxEntities.GetNext(it)) {
        if (it->m_Entity == entity) {
            m_FxEntities.RemoveItem(it);
            it->m_System->Kill();
            operator delete(it);
        }
    }
}

// 0x49E640
void Fx_c::Update(RwCamera* camera, float timeDelta) {
    ZoneScoped;

    g_fxMan.Update(camera, timeDelta);
}

// 0x49E650
void Fx_c::Render(RwCamera* camera, bool heatHaze) {
    ZoneScoped;

    g_fxMan.Render(camera, heatHaze);
}

// 0x49E950
void Fx_c::CreateMatFromVec(RwMatrix* out, const CVector* origin, const CVector* direction) {
    out->right = { 1.0f, 0.0f, 0.0f };
    out->up    = { 0.0f, 1.0f, 0.0f };
    out->at    = { 0.0f, 0.0f, 1.0f };
    out->pos   = { 0.0f, 0.0f, 0.0f };
    out->flags |= 0x20003;

    RwV3dAssign(&out->pos, origin);
    RwV3dAssign(&out->up, direction);
    RwV3dNormalize(&out->up, &out->up);

    const auto& up = out->up;
    out->right.x = up.z * 0.0f - up.y * -1.0f;
    out->right.y = up.x * -1.0f - up.z * 0.0f;
    out->right.z = up.y * 0.0f - up.x * 0.0f;

    out->at.x = out->right.y * up.z - out->right.z * up.y;
    out->at.y = out->right.z * up.x - out->right.x * up.z;
    out->at.z = out->right.x * up.y - out->right.y * up.x;

    RwMatrixUpdate(out);
}

// 0x49EA40
void Fx_c::SetFxQuality(FxQuality_e quality) {
    m_FxQuality = quality;
}

// 0x49EA50
FxQuality_e Fx_c::GetFxQuality() const {
    return m_FxQuality;
}

// NOTSA - Shared by `AddDebris` and `AddGlass`
static void SetDebrisOrGlassPrtMult(FxPrtMult_c& prtMult, const RwRGBA& color, float scale) {
    prtMult.m_Color.red   = (float)color.red * 0.003921569f;
    prtMult.m_fSize       = scale;
    prtMult.m_Color.green = (float)color.green * 0.003921569f;
    prtMult.m_Color.blue  = (float)color.blue * 0.003921569f;
    prtMult.m_Color.alpha = (float)color.alpha * 0.003921569f;
    prtMult.m_Rot         = ((float)(CGeneral::GetRandomNumber() % 10000) * 0.0001f + 1.0f) * 0.5f;
    prtMult.m_fLife       = 0.2f;
}

// NOTSA - Shared by `AddDebris` and `AddGlass`
static CVector RandomDebrisOrGlassVelocity() {
    const auto RandNorm = [] { return (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL; };
    CVector vel;
    vel.x = RandNorm() * 0.5f * 20.0f - 5.0f;
    vel.y = RandNorm() * 0.5f * 20.0f - 5.0f;
    vel.z = RandNorm() * 0.15f * 20.0f + 2.0f;
    return vel;
}

// 0x49EB00
void Fx_c::AddBlood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (!CLocalisation::Blood()) {
        return;
    }
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }

    FxPrtMult_c prtMult{ 0.5f, 0.0f, 0.0f, 1.0f, 0.8f, 0.0f, 0.8f };
    const auto Rand10000 = [] { return (float)(CGeneral::GetRandomNumber() % 10000); };
    for (auto i = 0; i < amount; i++) {
        prtMult.m_fSize = Rand10000() * 0.0001f * 0.3f + 0.7f;

        CVector vel = direction * 1.5f;
        const auto RandSpread = [&](float base) {
            const auto r = Rand10000() * 0.0001f;
            return ((r + r) - 1.0f) + base;
        };
        vel.x = RandSpread(vel.x);
        vel.y = RandSpread(vel.y);
        vel.z = RandSpread(vel.z);
        m_Blood->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, lightMult, 0.6f);
    }

    // Every 8th call (3rd and 6th) leaves a blood pool on the ground
    CVector shadowPos = direction * 0.5f + pos;
    shadowPos.x = (Rand10000() * 0.0001f * 0.2f - 0.1f) + shadowPos.x;
    const auto randY = Rand10000();
    m_Randomizer++;
    shadowPos.y = (randY * 0.0001f * 0.2f - 0.1f) + shadowPos.y;
    shadowPos.z += 1.0f;
    switch (m_Randomizer & 7) {
    case 5:
        CShadows::AddPermanentShadow(1, gpBloodPoolTex, &shadowPos, 0.1f, 0.0f, 0.0f, -0.1f, 255, 200, 0, 0, 4.0f, (CGeneral::GetRandomNumber() & 0xFFF) + 2000, 1.0f);
        break;
    case 2:
        CShadows::AddPermanentShadow(1, gpBloodPoolTex, &shadowPos, 0.2f, 0.0f, 0.0f, -0.2f, 255, 200, 0, 0, 4.0f, (CGeneral::GetRandomNumber() & 0xFFF) + 8000, 1.0f);
        break;
    }
}

// 0x49EE10
void Fx_c::AddWood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }

    FxPrtMult_c prtMult{ 0.5f, 0.25f, 0.0f, 1.0f, 0.3f, 0.0f, 1.0f };
    const auto Rand10000 = [] { return (float)(CGeneral::GetRandomNumber() % 10000); };
    for (auto i = 0; i < amount; i++) {
        prtMult.m_Color.red   = Rand10000() * 0.0001f * 0.12f + 0.13f;
        prtMult.m_Color.green = Rand10000() * 0.0001f * 0.03f + 0.12f;
        prtMult.m_Color.blue  = Rand10000() * 0.0001f * 0.03f + 0.04f;
        prtMult.m_fSize       = Rand10000() * 0.0001f * 0.3f + 0.7f;

        CVector vel = direction * 4.0f;
        vel.x = (Rand10000() * 0.0001f * 4.0f - 2.0f) + vel.x;
        vel.y = (Rand10000() * 0.0001f * 4.0f - 2.0f) + vel.y;
        vel.z = (Rand10000() * 0.0001f * 4.0f - 2.0f) + vel.z;
        m_Blood->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, lightMult, 0.6f);
    }
}

// 0x49F040 (real body at 0x49F045)
void Fx_c::AddSparks(const CVector& origin, const CVector& direction, float force, int32 amount, CVector across, eSparkType sparksType, float spread, float life) {
    const auto distSq = CVector::DistSqr(TheCamera.GetPosition(), origin);
    if (distSq > 22500.0f || (distSq > 225.0f && (CTimer::GetFrameCounter() & 1) != 0)) {
        return;
    }

    const FxPrtMult_c prtMult{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, life * 0.8f };

    across.x *= CTimer::GetTimeStep();
    across.y *= CTimer::GetTimeStep();
    across.z *= CTimer::GetTimeStep();

    auto* const system = sparksType == SPARK_PARTICLE_SPARK ? m_Spark : m_Spark2;

    for (auto i = 0; i < amount; i++) {
        const auto t  = 1.0f - (float)i / (float)amount;
        const auto rx = CGeneral::GetRandomNumberInRange(0.0f, 1.0f);
        const auto ry = CGeneral::GetRandomNumberInRange(0.0f, 1.0f);
        const auto rz = CGeneral::GetRandomNumberInRange(0.0f, 1.0f);
        const CVector vel{
            (2.0f * spread * rx - spread + direction.x) * force,
            (2.0f * spread * ry - spread + direction.y) * force,
            (2.0f * spread * rz - spread + direction.z) * force,
        };
        const CVector pos{
            origin.x - across.x * t,
            origin.y - across.y * t,
            origin.z - across.z * t,
        };
        system->AddParticle(pos, vel, t * 0.05f, prtMult);
    }
}

// 0x49F300
void Fx_c::AddTyreBurst(const CVector& posn, const CVector& velocity) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), posn) > 625.0f) {
        return;
    }
    const FxPrtMult_c prtMult{ 1.0f, 1.0f, 1.0f, 0.4f, 0.12f, 0.0f, 0.1f };
    for (auto i = 0; i < 4; i++) {
        m_SmokeII3expand->AddParticle(posn, velocity, (float)i * 0.05f, prtMult);
    }
}

// 0x49F3D0
void Fx_c::AddBulletImpact(const CVector& posn, const CVector& direction, int32 bulletFxType, int32 amount, float lightMult) {
    const auto fxType = g_surfaceInfos.GetBulletFx(bulletFxType);
    if (!fxType || DistanceBetweenPointsSquared(TheCamera.GetPosition(), posn) > 22500.0f) {
        return;
    }

    switch (fxType) {
    case 1: { // Sparks and a puff of smoke
        AddSparks(posn, direction, 3.0f, amount, CVector{}, SPARK_PARTICLE_SPARK, 0.4f, 1.0f);

        FxPrtMult_c prtMult{ 1.0f, 1.0f, 1.0f, 0.15f, 0.4f, 0.0f, 0.075f };
        auto numParticles = 2;
        if (amount > 7) {
            numParticles = 1;
            prtMult.m_Color.alpha += prtMult.m_Color.alpha;
        }
        for (auto i = 0; i < numParticles; i++) {
            m_SmokeII3expand->AddParticle(posn, direction, (float)i * 0.05f, prtMult, -1.0f, lightMult, 0.6f);
        }
        break;
    }
    case 2:
    case 4: { // Dust
        FxPrtMult_c prtMult{ 0.81f, 0.67f, 0.57f, 0.15f, 0.4f, 0.0f, 0.3f };
        if (fxType == 4) {
            prtMult.m_Color.red = prtMult.m_Color.green = prtMult.m_Color.blue = 0.6f;
        }
        auto numParticles = 4;
        if (amount > 7) {
            numParticles = 2;
            prtMult.m_Color.alpha += prtMult.m_Color.alpha;
        }
        for (auto i = 0; i < numParticles; i++) {
            m_Sand->AddParticle(posn, direction * 0.3f, (float)i * 0.05f, prtMult, -1.0f, lightMult, 0.6f);
        }
        break;
    }
    case 3: // Wood
        AddWood(posn, direction, (int32)((float)amount * 0.5f), 1.0f);
        break;
    }
}

// 0x49F670
void Fx_c::AddPunchImpact(const CVector& pos, const CVector& velocity, int32 num) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }
    const FxPrtMult_c prtMult{ 1.0f, 1.0f, 1.0f, 0.4f, 0.1f, 0.0f, 0.1f };
    m_SmokeII3expand->AddParticle(pos, velocity, 0.0f, prtMult);
    m_SmokeII3expand->AddParticle(pos, velocity, 0.05f, prtMult);
}

// 0x49F750
void Fx_c::AddDebris(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    static auto& s_DebrisPrimIdx = StaticRef<int32>(0xA9ADE4);

    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }

    FxPrtMult_c prtMult{};
    SetDebrisOrGlassPrtMult(prtMult, color, scale);
    for (auto i = 0; i < amount; i++) {
        const auto vel = RandomDebrisOrGlassVelocity();
        for (auto prim = 0; prim < 4; prim++) {
            m_Cardebris->EnablePrim(prim, false);
        }
        m_Cardebris->EnablePrim(s_DebrisPrimIdx, true);
        m_Cardebris->AddParticle(pos, vel, 0.0f, prtMult);
        s_DebrisPrimIdx = (s_DebrisPrimIdx + 1) & 3;
    }
}

// 0x49F970
void Fx_c::AddGlass(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }

    FxPrtMult_c prtMult{};
    SetDebrisOrGlassPrtMult(prtMult, color, scale);
    for (auto i = 0; i < amount; i++) {
        m_Glass->AddParticle(pos, RandomDebrisOrGlassVelocity(), 0.0f, prtMult);
    }
}

// 0x49FB30
void Fx_c::AddWheelSpray(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, bool bInWater, float lightMult) {
    const auto playerVeh = FindPlayerVehicle(-1, false);
    const auto distSq    = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (!(distSq <= 625.0f)) {
        return;
    }

    // Further away vehicles spawn their particles less often
    if (distSq > 400.0f) {
        if ((((uint8)vehicle->m_nModelIndex + CTimer::GetFrameCounter()) & 3) != 0) {
            return;
        }
    } else if (!(distSq <= 64.0f && playerVeh)) {
        if ((((uint8)vehicle->m_nModelIndex + CTimer::GetFrameCounter()) & 1) != 0) {
            return;
        }
    }

    const auto& moveSpeed = vehicle->m_vecMoveSpeed;
    if (!(std::fabs(moveSpeed.Magnitude()) > 0.01f) && !bWheelsSpinning) {
        return;
    }

    FxPrtMult_c prtMult{ 1.0f, 1.0f, 1.0f, 0.05f, 0.0f, 1.0f, 0.0f };

    float speedFactor;
    if (bWheelsSpinning) {
        speedFactor = 1.0f;
    } else {
        const auto doubleSpeed = moveSpeed.Magnitude() + moveSpeed.Magnitude();
        speedFactor = doubleSpeed <= 1.0f ? doubleSpeed : 1.0f;
    }

    prtMult.m_Color.alpha = (speedFactor + 1.0f) * (bInWater ? 0.2f : 0.15f);
    prtMult.m_fSize       = (speedFactor + 1.0f) * 0.2f;
    prtMult.m_fLife       = 0.08f;

    // The particles take a random fraction of the vehicle's speed
    const auto speedA = (speedFactor + 1.0f) * 10.0f;
    const auto speedMin = 30.0f - speedA;
    const auto speedMult = ((speedA + 30.0f) - speedMin) * ((float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL) + speedMin;
    const CVector particleVel = moveSpeed * speedMult;

    const CVector frameMove{
        CTimer::GetTimeStep() * moveSpeed.x,
        CTimer::GetTimeStep() * moveSpeed.y,
        CTimer::GetTimeStep() * moveSpeed.z
    };
    const auto numParticles = std::max(1, (int32)frameMove.Magnitude());
    const auto invNumParticles = 1.0f / (float)numParticles;

    for (auto i = 0; i < numParticles; i++) {
        const CVector offset = ((moveSpeed * invNumParticles) * (float)i) * CTimer::GetTimeStep();
        CVector particlePos = pos - offset;
        particlePos.z += 0.25f;

        CVector vel = particleVel;
        vel.z = ((float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL + 1.0f) * speedFactor + vel.z;
        m_BoatSplash->AddParticle(particlePos, vel, 0.0f, prtMult, -1.0f, lightMult, 0.6f);
    }
}

// NOTSA - The bodies of `AddWheelGrass`, `AddWheelGravel` and `AddWheelMud` only differ in the particle colour
static void AddWheelGroundParticles(Fx_c& fx, CVehicle* vehicle, const CVector& pos, float lightMult, CVector color) {
    // Only for vehicles driven by one of the players
    if (vehicle->m_pDriver != FindPlayerPed(0) && vehicle->m_pDriver != FindPlayerPed(1)) {
        return;
    }

    const auto playerVeh = FindPlayerVehicle(-1, false);
    const auto distSq    = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (!(distSq <= 625.0f)) {
        return;
    }

    // Further away vehicles spawn their particles less often
    const auto forceSpawn = distSq <= 64.0f && playerVeh;
    if (!forceSpawn) {
        const auto mask = distSq <= 400.0f ? 1u : 3u;
        if ((((uint8)vehicle->m_nModelIndex + CTimer::GetFrameCounter()) & mask) != 0) {
            return;
        }
    }

    FxPrtMult_c prtMult{ color.x, color.y, color.z, 1.0f, 0.0f, 0.0f, 0.05f };
    const auto RandNorm = [] { return (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL; };
    for (auto i = 0; i < 3; i++) {
        prtMult.m_fSize = RandNorm() * 0.03f + 0.03f;

        CVector vel;
        vel.x = RandNorm() * vehicle->m_vecMoveSpeed.x * -1.5f;
        vel.y = RandNorm() * vehicle->m_vecMoveSpeed.y * -1.5f;
        vel.z = RandNorm() * 1.5f + 2.0f;

        CVector particlePos = pos;
        particlePos.x = (RandNorm() * 0.4f + particlePos.x) - 0.2f;
        particlePos.y = (RandNorm() * 0.4f + particlePos.y) - 0.2f;
        fx.m_WheelDirt->AddParticle(particlePos, vel, 0.0f, prtMult, -1.0f, lightMult, 0.6f);
    }
}

// 0x49FF20
void Fx_c::AddWheelGrass(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelGroundParticles(*this, vehicle, pos, lightMult, { 0.03f, 0.09f, 0.03f });
}

// 0x4A0170
void Fx_c::AddWheelGravel(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelGroundParticles(*this, vehicle, pos, lightMult, { 0.25f, 0.25f, 0.25f });
}

// 0x4A03C0
void Fx_c::AddWheelMud(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelGroundParticles(*this, vehicle, pos, lightMult, { 0.25f, 0.12f, 0.06f });
}

// NOTSA - `AddWheelSand` and `AddWheelDust` only differ in the particle colour and life
static void AddWheelSandOrDustParticles(Fx_c& fx, CVehicle* vehicle, const CVector& pos, bool bWheelsSpinning, float lightMult, bool isDust) {
    const auto playerVeh = FindPlayerVehicle(-1, false);
    const auto distSq    = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (!(distSq <= 625.0f)) {
        return;
    }

    // Particles are spawned less often on lower fx qualities and further away from the camera
    const auto spawnCounter = CTimer::GetFrameCounter() + (int32)(int16)vehicle->m_nModelIndex;
    const auto forceSpawn   = distSq <= 64.0f && playerVeh;
    if ((int32)fx.m_FxQuality >= 1) {
        if (spawnCounter & 1) {
            return;
        }
        if (!forceSpawn && (spawnCounter & 3)) {
            return;
        }
    } else if ((int32)fx.m_FxQuality == 0) {
        if (spawnCounter & 3) {
            return;
        }
        if (!forceSpawn && (spawnCounter & 7)) {
            return;
        }
    }

    FxPrtMult_c prtMult = isDust
        ? FxPrtMult_c{ 0.51f, 0.44f, 0.31f, 0.5f, 1.0f, 0.0f, 0.0f }
        : FxPrtMult_c{ 0.81f, 0.67f, 0.57f, 0.5f, 1.0f, 0.0f, 0.0f };

    const auto absGasPedal = std::fabs(vehicle->m_GasPedal);

    // How much speed the particles take from the vehicle
    float speedFactor;
    if (bWheelsSpinning) {
        speedFactor = 1.0f;
    } else {
        const auto doubleSpeed = vehicle->m_vecMoveSpeed.Magnitude() + vehicle->m_vecMoveSpeed.Magnitude();
        speedFactor = doubleSpeed <= 1.0f ? doubleSpeed : 1.0f;
    }

    prtMult.m_fLife = isDust
        ? speedFactor * 0.05f + 0.1f
        : (speedFactor + 1.0f) * 0.1f;
    prtMult.m_fSize = speedFactor * 0.9f + 0.1f;

    float particlesPerUnit;
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_BMX:
        particlesPerUnit = 2.0f;
        prtMult.m_fSize *= 0.25f;
        break;
    case VEHICLE_TYPE_BIKE:
    case VEHICLE_TYPE_QUAD:
        particlesPerUnit = 2.0f;
        prtMult.m_fSize *= 0.5f;
        break;
    default:
        particlesPerUnit = 1.5f;
        prtMult.m_fSize *= 0.7f;
        break;
    }

    const CVector frameMove{
        CTimer::GetTimeStep() * vehicle->m_vecMoveSpeed.x,
        CTimer::GetTimeStep() * vehicle->m_vecMoveSpeed.y,
        CTimer::GetTimeStep() * vehicle->m_vecMoveSpeed.z
    };
    const auto numParticles = std::max(1, (int32)(frameMove.Magnitude() * particlesPerUnit));

    const auto RandNorm = [] { return (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL; };
    const auto zSpeedRange = (speedFactor + 0.8f) - 0.2f;
    for (auto i = 0; i < numParticles; i++) {
        CVector vel;
        vel.x = RandNorm() * ((absGasPedal * vehicle->m_vecMoveSpeed.x) * -40.0f);
        vel.y = RandNorm() * ((absGasPedal * vehicle->m_vecMoveSpeed.y) * -40.0f);
        vel.z = RandNorm() * zSpeedRange + 0.2f;

        const auto t = 1.0f - (float)i / (float)numParticles;
        fx.m_Sand->AddParticle(pos - frameMove * t, vel, 0.0f, prtMult, -1.0f, lightMult, 0.7f);
    }
}

// 0x4A0610
void Fx_c::AddWheelSand(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelSandOrDustParticles(*this, vehicle, pos, bWheelsSpinning, lightMult, false);
}

// 0x4A09C0
void Fx_c::AddWheelDust(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelSandOrDustParticles(*this, vehicle, pos, bWheelsSpinning, lightMult, true);
}

// 0x4A0D70
void Fx_c::TriggerWaterHydrant(const CVector& pos) {
    CreateFxWithinCameraRange("water_hydrant", pos, 625.0f);
}

// 0x4A0DE0
void Fx_c::TriggerGunshot(CEntity* entity, const CVector& origin, const CVector& target, bool doGunflash) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), origin) > 625.0f) {
        return;
    }

    RwMatrix* tempMat{};
    RwMatrix* attachTo{};
    CVector   fxPos{};
    if (!entity) {
        tempMat = attachTo = g_fxMan.FxRwMatrixCreate();
        CreateMatFromVec(tempMat, &origin, &target);
    } else {
        const auto originToEntity = origin - entity->GetPosition();
        fxPos = entity->GetMatrix().InverseTransformVector(originToEntity);
        if (!entity->GetRwObject()) {
            return;
        }
        attachTo = RwFrameGetMatrix(RwFrameGetParent(entity->GetRwObject()));
    }

    if (attachTo) {
        const auto CreateAndPlay = [&](const char* name) {
            if (auto* const fx = g_fxMan.CreateFxSystem(name, fxPos, attachTo, false)) {
                if (!entity) {
                    fx->CopyParentMatrix();
                }
                fx->PlayAndKill();
            }
        };
        if (doGunflash) {
            CreateAndPlay("gunflash");
        }
        CreateAndPlay("gunsmoke");
    }

    if (tempMat) {
        g_fxMan.FxRwMatrixDestroy(tempMat);
    }
}

// 0x4A0FA0
void Fx_c::TriggerTankFire(const CVector& pos, const CVector& dir) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f) {
        return;
    }
    auto* const mat = g_fxMan.FxRwMatrixCreate();
    CreateMatFromVec(mat, &pos, &dir);
    if (auto* const fx = g_fxMan.CreateFxSystem("tank_fire", CVector{}, mat, false)) {
        fx->CopyParentMatrix();
        fx->PlayAndKill();
    }
    g_fxMan.FxRwMatrixDestroy(mat);
}

// 0x4A1070
void Fx_c::TriggerWaterSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash_big", pos, 625.0f);
}

// 0x4A10E0
void Fx_c::TriggerBulletSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash", pos, 625.0f);
}

// 0x4A1150
void Fx_c::TriggerFootSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splsh_sml", pos, 625.0f);
}

// see RwIm3DTransformFlags
// 0x4A13B0
void RenderBegin(RwRaster* newRaster, RwMatrix* transform, uint32 transformRenderFlags) {
    g_fx.m_pTransformLTM = transform;
    g_fx.m_nVerticesCount = 0;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_pRasterToRender = newRaster;
    g_fx.m_nTransformRenderFlags = transformRenderFlags;
    g_fx.m_pVerts = TempBufferVertices.m_3d;

    // And maybe update raster on RW if the same isnt already set...
    RwRaster* currRaster{};
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &currRaster);
    if (currRaster != newRaster)
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(newRaster));
}

// Wrapper for original func
// 0x4A1410
void RenderAddTri_(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float u1, float v1,
    float u2, float v2,
    float u3, float v3,
    int32 r1, int32 g1, int32 b1, int32 a1,
    int32 r2, int32 g2, int32 b2, int32 a2,
    int32 r3, int32 g3, int32 b3, int32 a3
) {
    RenderAddTri(
        { x1, y1, z1 },
        { x2, y2, z2 },
        { x3, y3, z3 },
        { u1, v1 },
        { u2, v2 },
        { u3, v3 },
        CRGBA().FromInt32(r1, g1, b1, a1),
        CRGBA().FromInt32(r2, g2, b2, a2),
        CRGBA().FromInt32(r3, g3, b3, a3)
    );
}

// TODO: I honestly think this should be a class method...
// Although originally it wasnt.
// NOTE: Method signature changed to use CVector + RwTexCoords instead of raw values for convenience.
void RenderAddTri(CVector pos1, CVector pos2, CVector pos3, RwTexCoords coord1, RwTexCoords coord2, RwTexCoords coord3, const CRGBA& color1, const CRGBA& color2, const CRGBA& color3) {
    const auto GetVertex = [](unsigned i) {
        return &g_fx.m_pVerts[i];
    };

    const CVector pos[] = { pos1, pos2, pos3 };
    const RwRGBA color[] = {
        { color1.ToRwRGBA() },
        { color2.ToRwRGBA() },
        { color3.ToRwRGBA() },
    };
    for (unsigned i = 0; i < 3; i++) {
        RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color[i]);
    }

    if (g_fx.m_pRasterToRender) {
        const RwTexCoords uvs[] = { coord1, coord2, coord3 };
        for (unsigned i = 0; i < 3; i++) {
            RxObjSpace3DVertexSetU(GetVertex(i), uvs[i].u);
            RxObjSpace3DVertexSetV(GetVertex(i), uvs[i].v);
        }
    }

    g_fx.m_pVerts += 3;
    g_fx.m_nVerticesCount2 += 3;
    g_fx.m_nVerticesCount++;

    if (g_fx.m_nVerticesCount2 >= TOTAL_TEMP_BUFFER_3DVERTICES - 3 || g_fx.m_nVerticesCount >= TOTAL_TEMP_BUFFER_INDICES - 1) {
        RenderEnd(); // Render vertices to free up vertex buffer
    }
}

// 0x4A1600
void RenderEnd() {
    if (!g_fx.m_nVerticesCount)
        return;

    if (RwIm3DTransform(TempBufferVertices.m_3d, 3 * g_fx.m_nVerticesCount, g_fx.m_pTransformLTM, g_fx.m_nTransformRenderFlags)) {
        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
        RwIm3DEnd();
    }

    g_fx.m_pVerts = TempBufferVertices.m_3d;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_nVerticesCount = 0;
}

// 0x4A1660
void RotateVecIntoVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& vecAlign) {
    const CVector up = vecAlign;
    const auto ref = CVector{ 3.f, 4.f, 5.f }.Normalized();

    RwV3d right;
    RwV3dCrossProduct(&right, &up, &ref);
    RwV3dNormalize(&right, &right);

    RwV3d at;
    RwV3dCrossProduct(&at, &up, &right);

    auto* m = g_fxMan.FxRwMatrixCreate();
    m->right = right;
    m->up    = up;
    m->at    = at;
    m->pos   = { 0.0f, 0.0f, 0.0f };

    RwMatrixUpdate(m);
    RwV3dTransformVectors(&vecRes, &vec, 1, m);
    g_fxMan.FxRwMatrixDestroy(m);
}

// 0x4A1780
void RotateVecAboutVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& axis, float angle) {
    const float x = axis.x, y = axis.y, z = axis.z;

    const float s = CMaths::GetSinFast(angle);
    const float c = CMaths::GetCosFast(angle);
    const float t = 1.0f - c;

    vecRes.x = (t*x*x + c)   * vec.x + (t*x*y - s*z) * vec.y + (t*x*z + s*y) * vec.z;
    vecRes.y = (t*x*y + s*z) * vec.x + (t*y*y + c)   * vec.y + (t*y*z - s*x) * vec.z;
    vecRes.z = (t*x*z - s*y) * vec.x + (t*y*z + s*x) * vec.y + (t*z*z + c)   * vec.z;
}
