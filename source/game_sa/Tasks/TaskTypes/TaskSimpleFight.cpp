#include "StdInc.h"

#include "TaskSimpleFight.h"

// 0x61C470
CTaskSimpleFight::CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) : CTaskSimple() {
    plugin::CallMethod<0x61C470, CTaskSimpleFight*, CEntity*, int32, uint32>(this, entity, nCommand, nIdlePeriod);
}

// 0x61C530
CTaskSimpleFight::~CTaskSimpleFight() {
    plugin::CallMethod<0x61C530, CTaskSimpleFight*>(this);
}

// 0x6240B0
void CTaskSimpleFight::FightStrike(CPed* ped, CVector& posn) {
    plugin::CallMethod<0x6240B0, CTaskSimpleFight*, CPed*, CVector&>(this, ped, posn);
}

// 0x61C5E0
bool CTaskSimpleFight::ControlFight(CEntity* entity, uint8 command) {
    m_bIsInControl = true;
    if (entity != m_pTargetEntity) {
        CEntity::ChangeEntityReference(m_pTargetEntity, entity);
    }
    if (static_cast<int8>(m_nNextCommand) < static_cast<int8>(command)) {
        m_nNextCommand = command;
    }
    return true;
}

// 0x4ABDC0
bool CTaskSimpleFight::IsComboSet() {
    return (CurrentComboData().m_wFlags & (0x10 << m_nCurrentMove)) != 0;
}

// 0x4ABDF0
bool CTaskSimpleFight::IsHitComboSet() {
    return m_nCurrentMove < FIGHT_ATTACK_FIGHT_BLOCK
        && (CurrentComboData().m_wFlags & (0x1000 << m_nCurrentMove)) != 0;
}

// 0x4ABDA0
AssocGroupId CTaskSimpleFight::GetComboAnimGroupID() {
    return CurrentComboData().m_nAnimGroup;
}

// The data of the combo that is set, a negative index (no combo) is treated as the first one
const CMeleeInfo& CTaskSimpleFight::CurrentComboData() const {
    return m_aComboData[std::max(0, m_nComboSet - MELEE_COMBO_UNARMED_1)];
}

// 0x5BEDC0
void CTaskSimpleFight::LoadMeleeData() {
    ZoneScoped;

    static_assert(offsetof(CMeleeInfo, m_nHitLevel) == 0x50 && offsetof(CMeleeInfo, m_Hit) == 0x5C && offsetof(CMeleeInfo, m_wFlags) == 0x84);

    // The three values for every level of the fight, read from the first section of the file
    static auto& s_Levels = StaticRef<std::array<CVector, 7>>(0xC177D0);
    // Not hooked
    const auto GetHitLevel = [](const char* str) { return plugin::CallAndReturn<uint8, 0x5BD360, const char*>(str); };
    const auto GetHitSound = [](int32 id) { return plugin::CallAndReturn<int16, 0x5BD3B0, int32>(id); };

    // Everything has a default in case the file is missing something
    for (auto& combo : m_aComboData) {
        combo.m_nAnimGroup  = (AssocGroupId)0x21;
        combo.m_fRanges     = 1.5f;
        combo.m_fHit.fill(100.0f);
        combo.m_fChain.fill(100.0f);
        combo.m_fRadius.fill(1.0f);
        combo.m_nHitLevel.fill(7);
        combo.m_nDamage.fill(0);
        combo.m_Hit.fill(0);
        combo.m_AltHit.fill(0);
        combo.m_fGroundLoop = 0.0f;
        combo.ABlockHit     = 100.0f;
        combo.ABlockChain   = 100.0f;
        combo.m_wFlags      = 0;
    }
    s_Levels.fill(CVector{ 0.0f, 0.75f, 0.0f });

    constexpr auto TICKS = 1.0f / 30.0f; // 0.033333335

    auto* const file = fopen("DATA\\melee.dat", "rb");

    bool    inCombo{}, inLevels{};
    size_t  comboIdx{};
    int32   lineIdx{};
    CVector level{ 0.0f, 0.75f, 0.0f }; // Keeps its value when a line can't be read
    char    keyword[32];
    char    text[32];

    for (auto* line = CFileLoader::LoadLine(file); line; line = CFileLoader::LoadLine(file)) {
        if (*line == '#' || *line == '\0') {
            continue;
        }
        if (!strncmp(line, "END_MELEE_DATA", 14)) {
            break;
        }

        if (!inCombo && !inLevels) {
            if (!strncmp(line, "START_COMBO", 11)) {
                inCombo = true;
            } else if (!strncmp(line, "START_LEVELS", 12)) {
                inLevels = true;
            }
            continue;
        }

        if (!strncmp(line, "END_COMBO", 9)) {
            if (inCombo) {
                comboIdx++;
            }
            lineIdx  = 0;
            inCombo  = false;
            inLevels = false;
            continue;
        }

        if (inLevels) {
            sscanf(line, "%s %f %f %f", keyword, &level.x, &level.y, &level.z);
            s_Levels[lineIdx++] = level;
            continue;
        }

        // One line of a combo, which one it is depends on how far into the section we are
        auto& combo = m_aComboData[comboIdx];
        switch (lineIdx++) {
        case 0: { // Animation group
            sscanf(line, "%s %s", keyword, text);
            for (auto i = 0u; i < CAnimManager::GetAssocGroups().size(); i++) {
                if (!strcmp(text, CAnimManager::GetAnimGroupName((AssocGroupId)i))) {
                    combo.m_nAnimGroup = (AssocGroupId)i;
                    break;
                }
            }
            break;
        }
        case 1: // Range
            sscanf(line, "%s %f", keyword, &combo.m_fRanges);
            break;
        case 2: case 3: case 4: case 5: case 6: { // One of the moves
            const auto move = lineIdx - 1 - 2;
            float  hit{}, chain{}, radius{}, groundLoop{};
            int32  damage{}, hitSound{}, altHitSound{};
            sscanf(line, "%s %f %f %f %s %d %d %d %f", keyword, &hit, &chain, &radius, text, &damage, &hitSound, &altHitSound, &groundLoop);

            combo.m_fHit[move]      = hit * TICKS;
            combo.m_fRadius[move]   = radius;
            combo.m_fChain[move]    = chain * TICKS;
            combo.m_nHitLevel[move] = GetHitLevel(text);
            combo.m_nDamage[move]   = (uint8)damage;
            combo.m_Hit[move]       = (int32)GetHitSound(hitSound);
            combo.m_AltHit[move]    = (int32)GetHitSound(altHitSound);
            if (groundLoop > 0.0f) {
                combo.m_fGroundLoop = groundLoop * TICKS;
            }
            break;
        }
        case 7: { // Blocking
            float hit{}, chain{};
            sscanf(line, "%s %f %f", keyword, &hit, &chain);
            combo.ABlockHit   = hit * TICKS;
            combo.ABlockChain = chain * TICKS;
            break;
        }
        case 8: { // Flags
            uint32 flags{};
            sscanf(line, "%s %x", keyword, &flags);
            combo.m_wFlags = (uint16)flags;
            break;
        }
        }
    }

    fclose(file);
}

// 0x6239F0
bool CTaskSimpleFight::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return plugin::CallMethodAndReturn<bool, 0x6239F0, CTaskSimpleFight*, CPed*, eAbortPriority, const CEvent*>(this, ped, priority, event);
}

// 0x629920
bool CTaskSimpleFight::ProcessPed(CPed* ped) {
    return plugin::CallMethodAndReturn<bool, 0x629920, CTaskSimpleFight*, CPed*>(this, ped);
}

void CTaskSimpleFight::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleFight, 0x86D684, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(ControlFight, 0x61C5E0);
    RH_ScopedInstall(IsComboSet, 0x4ABDC0);
    RH_ScopedInstall(IsHitComboSet, 0x4ABDF0);
    RH_ScopedInstall(GetComboAnimGroupID, 0x4ABDA0);
    RH_ScopedInstall(LoadMeleeData, 0x5BEDC0);
}

CTaskSimpleFight* CTaskSimpleFight::Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) {
    this->CTaskSimpleFight::CTaskSimpleFight(entity, nCommand, nIdlePeriod);
    return this;
}

CTaskSimpleFight* CTaskSimpleFight::Destructor() {
    this->CTaskSimpleFight::~CTaskSimpleFight();
    return this;
}
