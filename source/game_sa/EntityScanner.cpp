#include "StdInc.h"

#include "EntityScanner.h"
#include "World.h"

void CEntityScanner::InjectHooks() {
    RH_ScopedClass(CEntityScanner);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x5FF990);
    RH_ScopedInstall(Destructor, 0x603480);

    RH_ScopedInstall(Clear, 0x5FF9D0);
    RH_ScopedInstall(ScanForEntitiesInRange, 0x5FFA20);
}

// 0x5FF990
CEntityScanner::CEntityScanner() {
    m_timer = {};

    rng::fill(m_apEntities, nullptr);
    m_pClosestEntityInRange = nullptr;

    m_timer.SetPeriod(MAX_NUM_ENTITIES);
}

// 0x603480
CEntityScanner::~CEntityScanner() {
    Clear();
}

// 0x5FF9D0
void CEntityScanner::Clear() {
    for (auto& entity : m_apEntities) {
        CEntity::ClearReference(entity);
    }

    CEntity::ClearReference(m_pClosestEntityInRange);
}

// 0x5FFA20
void CEntityScanner::ScanForEntitiesInRange(const eRepeatSectorList sectorList, const CPed& ped) {
    if (!m_timer.Tick()) {
        return;
    }

    Clear();

    auto* const player = FindPlayerPed();
    const auto  radius  = std::max(ped.GetIntelligence()->m_fHearingRange, ped.GetIntelligence()->m_fSeeingRange);
    const auto& pos     = ped.GetPosition();

    std::array<float, MAX_NUM_ENTITIES> distances;
    distances.fill(std::numeric_limits<float>::max());

    const auto minX = std::max(0, CWorld::GetSectorX(pos.x - radius));
    const auto minY = std::max(0, CWorld::GetSectorY(pos.y - radius));
    const auto maxX = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(pos.x + radius));
    const auto maxY = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(pos.y + radius));

    CWorld::AdvanceCurrentScanCode();
    const_cast<CPed&>(ped).SetCurrentScanCode(); // NOTSA: original mutates the (const-in-this-header) searching ped's own scan code so it never matches itself below

    const auto processCandidate = [&](CEntity* candidate) {
        if (candidate->IsScanCodeCurrent()) {
            return;
        }
        candidate->SetCurrentScanCode();

        if (sectorList == REPEATSECTOR_PEDS && player != &ped && candidate->AsPed()->m_nPedState == PEDSTATE_DEAD) {
            return;
        }

        const auto distSq = (candidate->GetPosition() - pos).SquaredMagnitude();
        if (distSq >= radius * radius) {
            return;
        }

        auto insertAt = 0u;
        while (insertAt < MAX_NUM_ENTITIES && distSq >= distances[insertAt]) {
            insertAt++;
        }
        if (insertAt >= MAX_NUM_ENTITIES) {
            return;
        }
        for (auto i = MAX_NUM_ENTITIES - 1; i > insertAt; i--) {
            m_apEntities[i] = m_apEntities[i - 1];
            distances[i]    = distances[i - 1];
        }
        m_apEntities[insertAt] = candidate;
        distances[insertAt]    = distSq;
    };

    for (auto y = minY; y <= maxY; y++) {
        for (auto x = minX; x <= maxX; x++) {
            auto& sector = CWorld::GetRepeatSector(x, y);
            switch (sectorList) {
            case REPEATSECTOR_VEHICLES:
                for (auto* v : sector.Vehicles) { processCandidate(v); }
                break;
            case REPEATSECTOR_PEDS:
                for (auto* p : sector.Peds) { processCandidate(p); }
                break;
            case REPEATSECTOR_OBJECTS:
                for (auto* o : sector.Objects) { processCandidate(o); }
                break;
            default:
                break;
            }
        }
    }

    for (auto& entity : m_apEntities) {
        if (entity) {
            CEntity::RegisterReference(entity);
        }
    }
    if (m_apEntities[0]) {
        CEntity::SetEntityReference(m_pClosestEntityInRange, m_apEntities[0]);
    }
}
