/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Stats.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "TagManager.h"
#include "GangWars.h"
#include "Localisation.h"
#include "World.h"
#include "StuntJumpManager.h"

void CStats::InjectHooks() {
    RH_ScopedClass(CStats);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x55C0C0);
    RH_ScopedOverloadedInstall(GetStatValue, "-OG", 0x558E40, float(*)(eStats));
    RH_ScopedInstall(SetStatValue, 0x55A070);
    RH_ScopedInstall(IsStatFloat, 0x558E30);
    /*RH_ScopedInstall(GetFullFavoriteRadioStationList, 0x558F90); - different return type*/
    RH_ScopedInstall(FindCriminalRatingNumber, 0x559080);
    RH_ScopedInstall(GetPercentageProgress, 0x5591E0);
    RH_ScopedInstall(ConvertToMins, 0x559540);
    RH_ScopedInstall(ConvertToSecs, 0x559560);
    RH_ScopedInstall(SafeToShowThisStat, 0x559590);
    RH_ScopedInstall(CheckForThreshold, 0x5595F0);
    RH_ScopedInstall(IsStatCapped, 0x559630);
    RH_ScopedInstall(LoadActionReactionStats, 0x5599B0);
    RH_ScopedInstall(FindMaxNumberOfGroupMembers, 0x559A50);
    RH_ScopedInstall(ProcessReactionStatsOnDecrement, 0x559730);
    RH_ScopedInstall(DecrementStat, 0x559FA0);
    RH_ScopedInstall(SetNewRecordStat, 0x55C410);
    RH_ScopedInstall(RegisterFastestTime, 0x55A0B0);
    RH_ScopedInstall(RegisterBestPosition, 0x55A160);
    RH_ScopedInstall(ProcessReactionStatsOnIncrement, 0x55B900);
    RH_ScopedInstall(DisplayScriptStatUpdateMessage, 0x55B980);
    RH_ScopedInstall(IncrementStat, 0x55C180);
    RH_ScopedInstall(UpdateStatsWhenSprinting, 0x55C660);
    RH_ScopedInstall(UpdateStatsWhenRunning, 0x55C6F0);
    RH_ScopedInstall(UpdateStatsWhenOnMotorBike, 0x55CD60);
    RH_ScopedInstall(UpdateStatsWhenFighting, 0x55CFA0);
    RH_ScopedInstall(ModifyStat, 0x55D090);

    // unused
    RH_ScopedInstall(GetStatID, 0x558DE0);
    RH_ScopedInstall(GetTimesMissionAttempted, 0x558E70);
    RH_ScopedInstall(RegisterMissionAttempted, 0x558E80);
    RH_ScopedInstall(RegisterMissionPassed, 0x558EA0);

    RH_ScopedInstall(Load, 0x5D3BF0);
    RH_ScopedInstall(Save, 0x5D3B40);
    RH_ScopedInstall(PopulateFavoriteRadioStationList, 0x558EC0);
    RH_ScopedInstall(FindMostFavoriteRadioStation, 0x558FA0);
    RH_ScopedInstall(FindLeastFavoriteRadioStation, 0x559010);
    RH_ScopedInstall(BuildStatLine, 0x559230);
    RH_ScopedInstall(CheckForStatsMessage, 0x559760);
    RH_ScopedInstall(LoadStatUpdateConditions, 0x559860);
    RH_ScopedInstall(GetFatAndMuscleModifier, 0x559AF0);
    RH_ScopedInstall(FindCriminalRatingString, 0x55A210);
    RH_ScopedInstall(ConstructStatLine, 0x55A780);
    RH_ScopedInstall(UpdateRespectStat, 0x55BC50);
    RH_ScopedInstall(UpdateSexAppealStat, 0x55BF20);
    RH_ScopedInstall(UpdateFatAndMuscleStats, 0x55C470);
    RH_ScopedInstall(UpdateStatsWhenCycling, 0x55C780);
    RH_ScopedInstall(UpdateStatsWhenSwimming, 0x55C990);
    RH_ScopedInstall(UpdateStatsWhenDriving, 0x55CAC0);
    RH_ScopedInstall(UpdateStatsWhenFlying, 0x55CC00);
    RH_ScopedInstall(UpdateStatsWhenWeaponHit, 0x55CEB0);
    RH_ScopedInstall(UpdateStatsOnRespawn, 0x55CFC0);
    RH_ScopedInstall(UpdateStatsAddToHealth, 0x55D030);
}

// 0x55C0C0
void CStats::Init() {
    std::ranges::fill(StatTypesFloat, 0.0f);
    std::ranges::fill(StatTypesInt, 0);
    std::ranges::fill(PedsKilledOfThisType, 0);
    std::ranges::fill(TimesMissionAttempted, 0);

    bStatUpdateMessageDisplayed = false;
    CTimer::SetTimeInMS(0);
    std::ranges::fill(LastMissionPassedName, 0);
    m_SprintStaminaCounter = 0;
    m_CycleStaminaCounter = 0;
    m_SwimStaminaCounter = 0;
    m_DrivingCounter = 0;
    m_FlyingCounter = 0;
    m_BoatCounter = 0;
    m_BikeCounter = 0;
    m_FatCounter = 0;
    m_RunningCounter = 0;

    StatTypesFloat[STAT_MAX_HEALTH] = 569.0f;
    CheckForStatsMessage();
    StatTypesFloat[STAT_STAMINA] = 100.0f;
    CheckForStatsMessage();

    PopulateFavoriteRadioStationList();
    LoadActionReactionStats();
    LoadStatUpdateConditions();
}

// 0x558E40
float CStats::GetStatValue(eStats stat) {
    if (!IsStatFloat(stat)) { // int32
        assert(stat >= FIRST_INT_STAT);

        return static_cast<float>(StatTypesInt[stat - FIRST_INT_STAT]);
    }

    return StatTypesFloat[stat];
}

// 0x55A070
void CStats::SetStatValue(eStats stat, float value) {
    if (IsStatFloat(stat)) {
        StatTypesFloat[stat] = value;
    } else { // int32
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] = static_cast<int32>(value);
    }
    CheckForStatsMessage();
}

// 0x558E30
bool CStats::IsStatFloat(eStats stat) {
    return stat < FIRST_UNUSED_STAT;
}

// 0x558EC0
bool CStats::PopulateFavoriteRadioStationList() {
    const auto* const listenTimes = AudioEngine.GetRadioStationListenTimes();

    // Returns whether nothing has been listened to yet
    bool nothingListenedTo = true;
    for (size_t i = 0; i < FavoriteRadioStationList.size(); i++) {
        FavoriteRadioStationList[i] = listenTimes[i];
        if (listenTimes[i] != 0) {
            nothingListenedTo = false;
        }
    }
    return nothingListenedTo;
}

// 0x558FA0
eRadioID CStats::FindMostFavoriteRadioStation() {
    // The first and the last slot (the emergency station and the radio being off) are not looked at
    auto best     = static_cast<int32>(RADIO_CLASSIC_HIP_HOP);
    auto bestTime = 0;
    for (int32 i = RADIO_CLASSIC_HIP_HOP; i <= RADIO_USER_TRACKS; i++) {
        if (bestTime < FavoriteRadioStationList[i]) {
            best     = i;
            bestTime = FavoriteRadioStationList[i];
        }
    }
    return static_cast<eRadioID>(best);
}

// 0x559010
int32 CStats::FindLeastFavoriteRadioStation() {
    auto best     = static_cast<int32>(RADIO_CLASSIC_HIP_HOP);
    auto bestTime = FavoriteRadioStationList[RADIO_CLASSIC_HIP_HOP];
    for (int32 i = RADIO_CLASSIC_HIP_HOP; i <= RADIO_USER_TRACKS; i++) {
        if (FavoriteRadioStationList[i] < bestTime) {
            best     = i;
            bestTime = FavoriteRadioStationList[i];
        }
    }
    return best;
}

// 0x559080
int32 CStats::FindCriminalRatingNumber() {
    CPlayerInfo* playerInfo = FindPlayerPed()->GetPlayerInfoForThisPlayerPed();

    auto value = (int32)(
        GetStatValue(STAT_TOTAL_LEGITIMATE_KILLS)
        - (GetStatValue(STAT_TIMES_BUSTED) - GetStatValue(STAT_NUMBER_OF_HOSPITAL_VISITS)) * 3.0f
        + (GetStatValue(STAT_HIGHEST_FIREFIGHTER_MISSION_LEVEL) + GetStatValue(STAT_HIGHEST_PARAMEDIC_MISSION_LEVEL)) * 10.0f
        + int32((float)playerInfo->m_nMoney / 5000.0f)
        + GetStatValue(STAT_PLANES_HELICOPTERS_DESTROYED) * 30.0f
        + GetStatValue(STAT_TOTAL_FIRES_EXTINGUISHED)
        + GetStatValue(STAT_CRIMINALS_KILLED_ON_VIGILANTE_MISSION)
        + GetStatValue(STAT_PEOPLE_SAVED_IN_AN_AMBULANCE)
    );

    if (CCheat::m_bHasPlayerCheated || GetStatValue(STAT_TIMES_CHEATED) > 0.0f) {
        value -= 10 * (int32)GetStatValue(STAT_TIMES_CHEATED);

        value = std::max(value, -10000);
    } else {
        value = std::max(value, 0);
    }

    float bulletsFired = GetStatValue(STAT_BULLETS_FIRED);

    if (bulletsFired >= 100.0f) {
        value += (int32)(500 * (GetStatValue(STAT_BULLETS_THAT_HIT) / bulletsFired));
    }

    return value + (int32)(10 * GetPercentageProgress());
}

// 0x5591E0
float CStats::GetPercentageProgress() {
    // The total is a stat of its own (187 in the game)
    const auto total = StatTypesFloat[STAT_TOTAL_PROGRESS];
    if (total == 0.0f) {
        return 0.0f;
    }
    return std::min(StatTypesFloat[STAT_PROGRESS_MADE] / total * 100.0f, 100.0f);
}

namespace {
// An entry of the lists of stats of the stats page
struct tStatLine {
    int16 stat;
    uint8 valueType;   // How the value is shown
    bool  alwaysShown; // Otherwise it's hidden while the value is 0
    bool  isSpecial;   // Shown in its own way, depending on the stat
    uint8 _pad;
};
VALIDATE_SIZE(tStatLine, 0x6);

// The lists end with this stat
constexpr int16 STAT_LINES_END = -99;

auto GetStatLines(uint8 listId) {
    constexpr uintptr_t LISTS[] = { 0x8CD8A0, 0x8CD938, 0x8CD9B0, 0x8CD9D8, 0x8CDA70, 0x8CDAC8, 0x8CDBF8 };
    const auto* const first = reinterpret_cast<const tStatLine*>(listId < std::size(LISTS) ? LISTS[listId] : 0x8CDE08);
    const auto*       last  = first;
    while (last->stat != STAT_LINES_END) {
        last++;
    }
    return std::span{ first, last };
}

// The text of a GXT entry as ASCII (0x69F7E0)
const char* TextAsAscii(const char* key) {
    return plugin::CallAndReturn<const char*, 0x69F7E0, const GxtChar*, uint32>(TheText.Get(key), 0);
}
} // namespace

// 0x559230
void CStats::BuildStatLine(const char* labelKey, void* pValue1, int32 metrics, void* pValue2, int32 type) {
    if (!labelKey) {
        return;
    }

    gString2[0] = '\0';

    if (type == 1) { // Time, "minutes:seconds"
        const auto minutes = *(int32*)pValue1;
        const auto seconds = *(int32*)pValue2;
        sprintf_s(gString2, seconds < 10 ? "%d:0%d" : "%d:%d", minutes, seconds);
    } else if (pValue2) { // "x of y"
        switch (metrics) {
        case 0:
            sprintf_s(gString2, " %d %s %d", *(int32*)pValue1, TextAsAscii("FEST_OO"), *(int32*)pValue2);
            break;
        case 1:
            sprintf_s(gString2, "%.2f %s %.2f", (double)*(float*)pValue1, TextAsAscii("FEST_OO"), (double)*(float*)pValue2);
            break;
        case 3:
            sprintf_s(gString2, "$%.2f %s $%.2f", (double)*(float*)pValue1, TextAsAscii("FEST_OO"), (double)*(float*)pValue2);
            break;
        default:
            break;
        }
    } else if (pValue1) {
        switch (metrics) {
        case 0:
            sprintf_s(gString2, "%d", *(int32*)pValue1);
            break;
        case 1:
            sprintf_s(gString2, "%.2f", (double)*(float*)pValue1);
            break;
        case 2:
            sprintf_s(gString2, "%0.2f%%", (double)*(float*)pValue1);
            break;
        case 3:
            sprintf_s(gString2, "$%.2f", (double)*(float*)pValue1);
            break;
        case 4:
            sprintf_s(gString2, "%d|", *(int32*)pValue1);
            break;
        case 5: { // Weight
            const auto lbs = *(int32*)pValue1;
            if (CLocalisation::Metric()) {
                // The format wants an integer, but a double is passed (Like the original does)
#pragma warning(suppress: 4477)
                sprintf_s(gString2, "%dkgs", (double)((float)lbs * 0.4536f));
            } else {
                sprintf_s(gString2, "%dlbs", lbs);
            }
            break;
        }
        case 6:
            sprintf_s(gString2, "%.2f %s", (double)*(float*)pValue1, TextAsAscii("ST_MILE"));
            break;
        case 7:
            sprintf_s(gString2, "%.2fm", (double)*(float*)pValue1);
            break;
        case 8:
            sprintf_s(gString2, "%.2fft", (double)(*(float*)pValue1 * 3.3333333f));
            break;
        case 9:
            sprintf_s(gString2, "%d %s", *(int32*)pValue1, TextAsAscii("ST_SECS"));
            break;
        default:
            break;
        }
    }

    // Prepare the label (Without any of the `~x~` control codes) and the value
    GxtCharStrcpy(gGxtString, TheText.Get(labelKey));
    plugin::Call<0x719240, GxtChar*>(gGxtString); // Removes the control codes
    AsciiToGxtChar(gString2, gGxtString2);
}

// 0x559540
int32 CStats::ConvertToMins(int32 statValue) {
    if (statValue > 59)
        return (statValue - 60) / 60 + 1;
    return 0;
}

// 0x559560
int32 CStats::ConvertToSecs(int32 statValue) {
    int32 seconds = statValue;
    if (statValue > 59)
        seconds = -60 - 60 * ((statValue - 60) / 60) + statValue;
    if (seconds < 0)
        seconds = -seconds;
    return seconds;
}

// 0x559590
bool CStats::SafeToShowThisStat(eStats stat) {
    if (!CLocalisation::GermanGame()) {
        return true;
    }

    switch (stat) {
    case STAT_RAMPAGES_ATTEMPTED:
    case STAT_RAMPAGES_PASSED:
    case STAT_TOTAL_LEGITIMATE_KILLS:
    case STAT_HIGHEST_CIVILIAN_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_CIVILIAN_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_NUMBER_OF_TANKS_DESTROYED_ON_RAMPAGE:
        return false;
    default:
        return true;
    }
}

// 0x5595F0
bool CStats::CheckForThreshold(float* pValue, float range) {
    if (*pValue + 40.0f >= range && *pValue - 40.0f <= range) {
        return false;
    }
    *pValue = range;
    return true;
}

// 0x559630
bool CStats::IsStatCapped(eStats stat) {
    switch (stat) {
    case STAT_GIRLFRIEND_RESPECT:
    case STAT_CLOTHES_RESPECT:
    case STAT_FITNESS_RESPECT:
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_APPEARANCE:
    case STAT_ARMOR:
    case STAT_ENERGY:
    case STAT_DRIVING_SKILL:
    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_LUCK:
    case STAT_HORSESHOES_COLLECTED:
    case STAT_TOTAL_HORSESHOES:
    case STAT_OYSTERS_COLLECTED:
    case STAT_TOTAL_OYSTERS:
    case STAT_CYCLING_SKILL:
        return true;
    }
    return false;
}

// 0x559760
void CStats::CheckForStatsMessage() {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_cutsceneProcessing
        || CHud::HelpMessageDisplayed()
        || CMenuSystem::num_menus_in_use
    ) {
        return;
    }

    for (size_t i = 0; i < std::min<size_t>(TotalNumStatMessages, StatMessage.size()); i++) {
        auto& message = StatMessage[i];
        if (message.displayed) {
            continue;
        }

        const auto value = GetStatValue(static_cast<eStats>(message.stat_num));
        const auto reached = message.condition == STATMESSAGE_LESSTHAN
            ? value <= message.value
            : value >= message.value;
        if (!reached) {
            continue;
        }

        message.displayed = true;
        CHud::SetHelpMessage(TheText.Get(message.text_id), false, false, false);
        bStatUpdateMessageDisplayed = true;
    }
}

// 0x559860
void CStats::LoadStatUpdateConditions() {
    CFileMgr::SetDir("");

    auto* file = CFileMgr::OpenFile("DATA\\STATDISP.DAT", "rb");

    TotalNumStatMessages = 0;

    uint32 numMessages = 0;
    for (char* line = CFileLoader::LoadLine(file); line; line = CFileLoader::LoadLine(file)) {
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        if (numMessages >= StatMessage.size()) {
            break;
        }

        int32 statId{};
        float value{};
        char  statName[84]{};
        char  condition[12]{};
        char  textId[8]{};
        RET_IGNORED(sscanf_s(line, "%d %s %s %f %s", &statId, SCANF_S_STR(statName), SCANF_S_STR(condition), &value, SCANF_S_STR(textId)));

        auto& message     = StatMessage[numMessages];
        message.stat_num  = static_cast<int16>(statId);
        message.displayed = false;
        if (std::strcmp(condition, "lessthan") == 0) {
            message.condition = STATMESSAGE_LESSTHAN;
        } else if (std::strcmp(condition, "morethan") == 0) {
            message.condition = STATMESSAGE_MORETHAN;
        } // Anything else keeps the condition the message already had
        message.value = value;
        std::strcpy(message.text_id, textId);

        numMessages++;
    }
    TotalNumStatMessages = numMessages;

    CFileMgr::CloseFile(file);
}

// 0x5599B0
void CStats::LoadActionReactionStats() {
    CFileMgr::SetDir("");

    auto* file = CFileMgr::OpenFile("DATA\\AR_STATS.DAT", "rb");

    for (char* line = CFileLoader::LoadLine(file); line != nullptr; line = CFileLoader::LoadLine(file)) {
        int32 reactId;
        float reactValue;

        if (line[0] != '#' && line[0] != NULL) {
            VERIFY(sscanf_s(line, "%d %*s %f", &reactId, &reactValue) == 2);

            StatReactionValue[reactId] = reactValue;
        }
    }

    CFileMgr::CloseFile(file);
}

// 0x559A50
int32 CStats::FindMaxNumberOfGroupMembers() {
    float respect = StatTypesFloat[STAT_TOTAL_RESPECT];

    if (respect < 10.0f)
        return 0;
    if (respect < 60.0f)
        return 2;
    if (respect < 160.0f)
        return 3;
    if (respect < 330.0f)
        return 4;
    if (respect < 540.0f)
        return 5;
    if (respect < 800.0f)
        return 6;

    return 7;
}

// 0x559AF0
float CStats::GetFatAndMuscleModifier(eStatModAbilities statMod) {
    // Tweakable values the game keeps in its data section, each one scales the share of the stat in an ability
    static auto& fat200Min       = StaticRef<float>(0x8CDEC0); // 200, below this much fat there is no fat penalty
    static auto& muscle50Min     = StaticRef<float>(0x8CDEB8); // 50, below this much muscle there is no muscle bonus
    static auto& mod1Muscle      = StaticRef<float>(0x8CDEB4);
    static auto& mod1Fat         = StaticRef<float>(0x8CDEBC);
    static auto& mod2Fat         = StaticRef<float>(0x8CDEB0);
    static auto& mod2Muscle      = StaticRef<float>(0x8CDEAC);
    static auto& mod3Fat         = StaticRef<float>(0x8CDEA8);
    static auto& mod3Muscle      = StaticRef<float>(0x8CDEA4);
    static auto& mod4Fat         = StaticRef<float>(0x8CDEA0);
    static auto& mod4Muscle      = StaticRef<float>(0x8CDE9C);
    static auto& mod5Fat         = StaticRef<float>(0x8CDE98);
    static auto& mod5Muscle      = StaticRef<float>(0x8CDE94);
    static auto& mod5Stamina     = StaticRef<float>(0x8CDE90);
    static auto& mod5Cycling     = StaticRef<float>(0x8CDE8C);
    static auto& mod6Fat         = StaticRef<float>(0x8CDE88);
    static auto& mod6Cycling     = StaticRef<float>(0x8CDE84);
    static auto& modRunTime      = StaticRef<float>(0x8CDE80);
    static auto& modLungAir      = StaticRef<float>(0x8CDE7C);
    static auto& modMaxHealth    = StaticRef<float>(0x8CDE78);
    static auto& mod11Bike       = StaticRef<float>(0x8CDE74);
    static auto& mod12Bike       = StaticRef<float>(0x8CDE70);
    static auto& mod13Bike       = StaticRef<float>(0x8CDE6C);
    static auto& mod14Driving    = StaticRef<float>(0x8CDE64);
    static auto& mod14Fat        = StaticRef<float>(0x8CDE68);
    static auto& mod15Muscle     = StaticRef<float>(0x8CDE5C);
    static auto& mod15Fat        = StaticRef<float>(0x8CDE60);
    static auto& mod15Min        = StaticRef<float>(0x8CDE58);

    const auto fat        = StatTypesFloat[STAT_FAT];
    const auto stamina    = StatTypesFloat[STAT_STAMINA];
    const auto muscle     = StatTypesFloat[STAT_MUSCLE];
    const auto maxHealth  = StatTypesFloat[STAT_MAX_HEALTH];
    const auto Int        = [](eStats stat) { return (float)StatTypesInt[stat - FIRST_INT_STAT]; };

    // How far the stat is above its neutral value, from 0 to 1
    const auto fatShare    = (fat - fat200Min) / (1000.0f - fat200Min);
    const auto muscleShare = (muscle - muscle50Min) / (1000.0f - muscle50Min);

    switch (statMod) {
    case STAT_MOD_0:
        if (fat <= 800.0f) {
            return fat <= 400.0f ? 0.0f : 1.0f;
        }
        return 2.0f;
    case STAT_MOD_1: {
        const auto value = muscleShare * mod1Muscle + std::max(0.0f, fatShare) * mod1Fat + 1.0f;
        return std::max(value, 0.7f);
    }
    case STAT_MOD_2: {
        const auto value = muscleShare * mod2Muscle + (std::max(0.0f, fatShare) * mod2Fat + 1.0f);
        return std::max(value, 0.8f);
    }
    case STAT_MOD_3: {
        const auto value = muscleShare * mod3Muscle + (fatShare * mod3Fat + 1.0f);
        return std::max(value, 0.8f);
    }
    case STAT_MOD_4: {
        const auto value = muscleShare * mod4Muscle + fatShare * mod4Fat + 1.0f;
        return std::min(value, 2.0f);
    }
    case STAT_MOD_5: {
        const auto value = Int(STAT_CYCLING_SKILL) * 0.001f * mod5Cycling + stamina * 0.001f * mod5Stamina + muscleShare * mod5Muscle + fatShare * mod5Fat + 1.0f;
        return std::clamp(value, 0.25f, 2.0f);
    }
    case STAT_MOD_6: {
        const auto value = Int(STAT_CYCLING_SKILL) * 0.001f * mod6Cycling + fatShare * mod6Fat + 1.0f;
        return std::clamp(value, 0.5f, 2.0f);
    }
    case STAT_MOD_TIME_CAN_RUN:
        return stamina * 0.001f * modRunTime + 150.0f;
    case STAT_MOD_AIR_IN_LUNG:
        return (Int(STAT_LUNG_CAPACITY) + stamina) * 0.0005f * modLungAir + 1000.0f;
    case STAT_MOD_MAX_HEALTH:
        return maxHealth * 0.001f * modMaxHealth;
    case STAT_MOD_10:
        return modMaxHealth;
    case STAT_MOD_11:
        return std::min(1.0f, Int(STAT_BIKE_SKILL) * 0.001f) * mod11Bike + 1.0f;
    case STAT_MOD_12:
        return std::min(1.0f, Int(STAT_BIKE_SKILL) * 0.001f) * mod12Bike + 1.0f;
    case STAT_MOD_13:
        return std::min(1.0f, Int(STAT_BIKE_SKILL) * 0.001f) * mod13Bike + 1.0f;
    case STAT_MOD_DRIVING_SKILL: {
        const auto value = Int(STAT_DRIVING_SKILL) * 0.001f * mod14Driving + 1.0f + fatShare * mod14Fat;
        return std::min(value, 1.0f);
    }
    case STAT_MOD_15: {
        const auto value = muscleShare * mod15Muscle + fatShare * mod15Fat + 1.0f;
        return std::max(value, mod15Min);
    }
    default:
        return 1.0f;
    }
}

// 0x559730
void CStats::ProcessReactionStatsOnDecrement(eStats stat) {
    if (stat == STAT_ENERGY && GetStatValue(STAT_ENERGY) < 0.0f)
        DecrementStat(STAT_FAT, 23.0f);
}

// 0x559FA0
void CStats::DecrementStat(eStats stat, float value) {
    if (value <= 0.0f)
        return;

    float oldValue = GetStatValue(stat);

    SetStatValue(stat, std::max(oldValue - value, 0.0f));

    ProcessReactionStatsOnDecrement(stat);
    CheckForStatsMessage();
}

// 0x55C410
void CStats::SetNewRecordStat(eStats stat, float value) {
    float currentValue = GetStatValue(stat);

    if (currentValue < value)
        SetStatValue(stat, value);

    CheckForStatsMessage();
}

// 0x55A0B0
void CStats::RegisterFastestTime(eStats stat, int32 fastestTime) {
    SetNewRecordStat(stat, (float)fastestTime);
}

// 0x55A160
void CStats::RegisterBestPosition(eStats stat, int32 position) {
    SetNewRecordStat(stat, (float)position);
}

// 0x55A210
const GxtChar* CStats::FindCriminalRatingString() {
    const auto rating = FindCriminalRatingNumber();

    if (rating < 0) {
        if (rating > -500)  return TheText.Get("RATNG53");
        if (rating > -2000) return TheText.Get("RATNG54");
        if (rating > -4000) return TheText.Get("RATNG55");
        if (rating > -6000) return TheText.Get("RATNG56");
        return TheText.Get("RATNG57");
    }

    // A rating below the limit gets the text next to it, the `nullptr` one depends on the player's record
    static constexpr struct { int32 limit; const char* key; } LADDER[] = {
        { 20, "RATNG1" }, { 50, "RATNG2" }, { 75, "RATNG3" }, { 100, "RATNG4" }, { 120, "RATNG5" }, { 150, "RATNG6" },
        { 200, "RATNG7" }, { 240, "RATNG8" }, { 270, "RATNG9" }, { 300, "RATNG10" }, { 335, "RATNG11" },
        { 370, "RATNG12" }, { 400, "RATNG13" }, { 450, "RATNG14" }, { 500, "RATNG15" }, { 550, "RATNG16" },
        { 600, "RATNG17" }, { 610, "RATNG18" }, { 650, "RATNG19" }, { 700, "RATNG20" }, { 850, "RATNG21" },
        { 1000, "RATNG22" }, { 1005, "RATNG23" }, { 1150, "RATNG24" }, { 1300, nullptr }, { 1500, "RATNG26" },
        { 1700, "RATNG27" }, { 2000, "RATNG28" }, { 2100, "RATNG29" }, { 2300, "RATNG30" }, { 2500, "RATNG31" },
        { 2750, "RATNG32" }, { 3000, "RATNG33" }, { 3500, "RATNG34" }, { 4000, "RATNG35" }, { 5000, "RATNG36" },
        { 7500, "RATNG37" }, { 10000, "RATNG38" }, { 20000, "RATNG39" }, { 30000, "RATNG40" }, { 40000, "RATNG41" },
        { 50000, "RATNG42" }, { 65000, "RATNG43" }, { 80000, "RATNG44" }, { 100000, "RATNG45" },
        { 150000, "RATNG46" }, { 200000, "RATNG47" }, { 300000, "RATNG48" }, { 375000, "RATNG49" },
    };
    for (const auto& [limit, key] : LADDER) {
        if (rating < limit) {
            return TheText.Get(key ? key : StatTypesInt[STAT_TIMES_BUSTED - FIRST_INT_STAT] > 0 ? "RATNG25" : "RATNG24");
        }
    }

    if (rating < 500000) {
        // Hours spent flying
        const auto flightHours = static_cast<int32>(static_cast<float>(StatTypesInt[STAT_FLIGHT_TIME - FIRST_INT_STAT]) * (1.0f / 60000.0f) * (1.0f / 60.0f));
        return TheText.Get(flightHours > 10 ? "RATNG50" : "RATNG49");
    }
    if (rating >= 1000000 && CWorld::Players[CWorld::PlayerInFocus].m_nDisplayMoney > 10000000) {
        return TheText.Get("RATNG52");
    }
    return TheText.Get("RATNG51");
}

// 0x55A780
int32 CStats::ConstructStatLine(int32 lineIdx, uint8 listId) {
    // What showing a line returns, when it was the one asked for
    constexpr int32 LINE_SHOWN = -1;

    int32 line = 0; // The line being looked at

    // The label is the text of the stat, the value is what's in `gString2`
    const auto ShowLine = [] {
        GxtCharStrcpy(gGxtString, TheText.Get(gString));
        plugin::Call<0x719240, GxtChar*>(gGxtString); // Removes the control codes
        AsciiToGxtChar(gString2, gGxtString2);
    };
    const auto ShowFormatted = [&](const char* format, auto... args) {
        gString2[0] = '\0';
        sprintf_s(gString2, format, args...);
        ShowLine();
        return LINE_SHOWN;
    };
    const auto ShowLabelOnly = [&] {
        gString2[0] = '\0';
        ShowLine();
        return LINE_SHOWN;
    };
    // No label, the value is the text of the key
    const auto ShowText = [](const char* key) {
        gGxtString[0] = '\0';
        GxtCharStrcpy(gGxtString2, TheText.Get(key));
        return LINE_SHOWN;
    };
    const auto ShowXofY = [&](int32 value, int32 total) {
        gString2[0]            = '\0';
        const auto* const text = TextAsAscii("FEST_OO");
        sprintf_s(gString2, " %d %s %d", value, text, total);
        ShowLine();
        return LINE_SHOWN;
    };
    const auto ShowTime = [&](int32 hours, int32 minutes) {
        BuildStatLine(gString, &hours, 0, &minutes, 1);
        return LINE_SHOWN;
    };
    // The int stats are read straight from the array, which is also how the ones before it are read
    const auto IntStat = [](eStats stat) {
        return *(StatTypesInt.data() + (ptrdiff_t)stat - (ptrdiff_t)FIRST_INT_STAT);
    };
    const auto RawStatAsFloat = [&](uint16 stat) {
        return stat < FIRST_UNUSED_STAT ? StatTypesFloat[stat] : (float)IntStat((eStats)stat);
    };
    // Time in minutes, as given by the stats that count it in milliseconds
    const auto MinutesFromStat = [&](eStats stat) {
        return (int32)((double)IntStat(stat) * (double)1.6666667e-05f);
    };

    // How many lines the stat takes, or `LINE_SHOWN` if one of them is the line that was asked for
    const auto ShowStat = [&](const tStatLine& entry) -> int32 {
        const auto stat   = (eStats)entry.stat;
        const bool isLine = line == lineIdx;

        if (!entry.isSpecial) {
            switch (entry.valueType) {
            case 1:
                return isLine ? ShowFormatted("%.2f", (double)GetStatValue(stat)) : 1;
            case 3:
                return isLine ? ShowFormatted("$%.2f", (double)GetStatValue(stat)) : 1;
            case 4:
                return isLine ? ShowFormatted("%d|", (int32)GetStatValue(stat)) : 1;
            case 5: { // Weight
                if (!isLine) {
                    return 1;
                }
                const auto pounds = (int32)GetStatValue(stat);
                if (!CLocalisation::Metric()) {
                    return ShowFormatted("%dlbs", pounds);
                }
                // The format wants an integer, but a double is passed (Like the original does)
                return ShowFormatted("%dkgs", (double)pounds * (double)0.4536f);
            }
            case 6: {
                if (!isLine) {
                    return 1;
                }
                const auto value       = GetStatValue(stat);
                const auto* const unit = TextAsAscii("ST_MILE");
                return ShowFormatted("%.2f %s", (double)value, unit);
            }
            case 7: // Distance
                if (!isLine) {
                    return 1;
                }
                return CLocalisation::Metric()
                    ? ShowFormatted("%.2fm", (double)GetStatValue(stat))
                    : ShowFormatted("%.2fft", (double)GetStatValue(stat) * (double)3.3333333f);
            case 9: { // Time, in seconds
                if (!isLine) {
                    return 1;
                }
                const auto minutes = ConvertToMins((int32)GetStatValue(stat));
                const auto seconds = ConvertToSecs((int32)GetStatValue(stat));
                return ShowTime(minutes, seconds);
            }
            case 10: // Bar chart
                m_ThisStatIsABarChart = (int16)stat;
                return isLine ? ShowFormatted("%d", (int32)GetStatValue(stat)) : 1;
            default:
                return isLine ? ShowFormatted("%d", entry.stat < FIRST_UNUSED_STAT ? (int32)StatTypesFloat[entry.stat] : IntStat(stat)) : 1;
            }
        }

        // The stats that are shown in their own way
        if (entry.stat >= STAT_PROGRESS_WITH_DENISE && entry.stat <= STAT_PROGRESS_WITH_MILLIE) {
            if (!(0.0f < RawStatAsFloat(entry.stat))) {
                return 0;
            }
            return isLine ? ShowFormatted("%0.2f%%", (double)GetStatValue(stat)) : 1;
        }

        switch (stat) {
        case STAT_PROGRESS_MADE:
            return isLine ? ShowFormatted("%0.2f%%", (double)GetPercentageProgress()) : 1;
        case STAT_BEST_INSANE_STUNT_AWARDED: { // A line of its own for the stunt
            if (isLine) {
                return ShowLabelOnly();
            }
            static constexpr const char* STUNTS[] = { "INSTUN", "PRINST", "DBINST", "DBPINS", "TRINST", "PRTRST", "QUINST", "PQUINS" };
            const auto stunt = (uint32)(IntStat(stat) - 1);
            if (line + 1 != lineIdx) {
                return 2;
            }
            return ShowText(stunt < std::size(STUNTS) ? STUNTS[stunt] : "NOSTUC");
        }
        case STAT_UNIQUE_JUMPS_FOUND:
        case STAT_UNIQUE_JUMPS_DONE:
            return isLine ? ShowXofY(IntStat(stat), CStuntJumpManager::m_iNumJumps) : 1;
        case STAT_ARMOR:
            return isLine ? ShowFormatted("%d", (int32)CWorld::Players[0].m_pPed->m_fArmour) : 1;
        case STAT_FLIGHT_TIME: {
            const auto minutes = MinutesFromStat(stat);
            return isLine ? ShowTime(minutes / 60, minutes % 60) : 1;
        }
        case STAT_TIME_ON_JETPACK: {
            const auto minutes = MinutesFromStat(stat);
            if (minutes / 60 <= 0 && minutes % 60 <= 0) {
                return 0;
            }
            return isLine ? ShowTime(minutes / 60, minutes % 60) : 1;
        }
        case STAT_SHOOTING_RANGE_LEVELS_PASSED:
            return isLine ? ShowXofY(IntStat(stat), 12) : 1;
        case STAT_MOST_CARS_PARKED_ON_VALET_PARKING:
            return isLine ? ShowXofY(IntStat(stat), 25) : 1;
        case STAT_NUMBER_OF_VEHICLES_EXPORTED:
            return isLine ? ShowXofY(IntStat(stat), 30) : 1;
        case STAT_SNAPSHOTS_TAKEN:
            return isLine ? ShowXofY(IntStat(stat), IntStat(STAT_TOTAL_SNAPSHOTS)) : 1;
        case STAT_HORSESHOES_COLLECTED:
            return isLine ? ShowXofY(IntStat(stat), IntStat(STAT_TOTAL_HORSESHOES)) : 1;
        case STAT_OYSTERS_COLLECTED:
            return isLine ? ShowXofY(IntStat(stat), IntStat(STAT_TOTAL_OYSTERS)) : 1;
        case STAT_PLAYING_TIME: {
            const auto minutes = CTimer::GetTimeInMS() / 60000;
            return isLine ? ShowTime((int32)minutes / 60, (int32)minutes % 60) : 1;
        }
        case STAT_TAGS_SPRAYED:
            return isLine ? ShowXofY(CTagManager::ms_numTagged, CTagManager::ms_numTags) : 1;
        case STAT_LEAST_FAVORITE_GANG: { // The gang that was killed the most
            uint32 mostKilled = 0;
            int32  gang       = 0;
            for (auto i = 7; i < 15; i++) {
                if ((uint32)PedsKilledOfThisType[i] > mostKilled) {
                    mostKilled = PedsKilledOfThisType[i];
                    gang       = i;
                }
            }
            if (gang == 0) {
                return 0;
            }
            if (isLine) {
                return ShowLabelOnly();
            }
            static constexpr const char* GANGS[] = { "ST_GNG0", "ST_GNG1", "ST_GNG2", "ST_GNG3", "ST_GNG4", "ST_GNG5", "ST_GNG6", "ST_GNG7" };
            if (line + 1 != lineIdx) {
                return 2;
            }
            return ShowText(GANGS[gang - 7]);
        }
        case STAT_GANG_MEMBERS_WASTED: {
            if (!isLine) {
                return 1;
            }
            int32 killed = 0;
            for (auto i = 7; i < 16; i++) {
                killed += PedsKilledOfThisType[i];
            }
            return ShowFormatted("%d", killed);
        }
        case STAT_CRIMINALS_WASTED:
            return isLine ? ShowFormatted("%d", PedsKilledOfThisType[20]) : 1;
        case STAT_MOST_FAVORITE_RADIO_STATION:
        case STAT_LEAST_FAVORITE_RADIO_STATION: {
            if (PopulateFavoriteRadioStationList()) {
                return 0;
            }
            if (isLine) {
                return ShowLabelOnly();
            }
            char key[16];
            AudioEngine.GetRadioStationNameKey(
                stat == STAT_MOST_FAVORITE_RADIO_STATION ? FindMostFavoriteRadioStation() : (eRadioID)FindLeastFavoriteRadioStation(),
                key
            );
            return line + 1 == lineIdx ? ShowText(key) : 2;
        }
        case STAT_CURRENT_WEAPON_SKILL: {
            const auto& activeWeapon = FindPlayerPed(-1)->GetActiveWeapon();
            const auto  type         = (uint16)activeWeapon.m_Type;
            if (type != WEAPON_TEC9 && (type < WEAPON_PISTOL || type > WEAPON_TEC9)) { // Only the ones that have a skill
                return 0;
            }
            auto skill = (uint8)((type == WEAPON_TEC9 ? 0x1C : type) - 0x15);
            if (isLine) {
                return ShowFormatted("%d", (int32)GetStatValue((eStats)(STAT_PISTOL_SKILL - 1 + skill)));
            }

            // The name of the skill
            if ((int32)FindPlayerPed(-1)->GetActiveWeapon().m_Type == WEAPON_TEC9) {
                skill = 11;
                sprintf_s(gString, "STWE0%d", (int32)skill);
            } else {
                sprintf_s(gString, skill < 10 ? "STWE00%d" : skill < 100 ? "STWE0%d" : "STWE%d", (int32)skill);
            }
            return line + 1 == lineIdx ? ShowText(gString) : 2;
        }
        case STAT_WEAPON_SKILL_LEVELS: { // The skill of each weapon
            if (isLine) {
                return ShowLabelOnly();
            }
            for (uint16 i = 1; i < 11; i++) {
                sprintf_s(gString, i < 10 ? "STWE00%d" : i < 100 ? "STWE0%d" : "STWE%d", (int32)i);
                const auto skill = FindPlayerPed(-1)->GetWeaponSkill((eWeaponType)(i + 0x15));
                sprintf_s(gString2, skill == eWeaponSkill::STD ? "WS_STD" : skill == eWeaponSkill::PRO ? "WS_PRO" : "WS_POOR");
                if (line + i == lineIdx) {
                    GxtCharStrcpy(gGxtString, TheText.Get(gString));
                    GxtCharStrcpy(gGxtString2, TheText.Get(gString2));
                    return LINE_SHOWN;
                }
            }
            return 11;
        }
        case STAT_PILOT_RANKING: {
            const auto minutes = MinutesFromStat(STAT_FLIGHT_TIME);
            const auto hours   = minutes / 60;
            const auto mins    = minutes % 60;
            if (hours <= 0 && mins < 5) {
                return 0;
            }
            if (isLine) {
                return ShowLabelOnly();
            }

            // The rank, up to the hours and minutes it needs
            struct tRank {
                int32       maxHours;
                int32       lessMinutes;
                const char* key;
            };
            static constexpr tRank RANKS[] = {
                { 0, 10, "ST_PR01" }, { 0, 20, "ST_PR02" }, { 0, 30, "ST_PR03" },
                { 1, 0, "ST_PR04" }, { 1, 30, "ST_PR05" }, { 2, 0, "ST_PR06" }, { 2, 30, "ST_PR07" },
                { 3, 0, "ST_PR08" }, { 3, 30, "ST_PR09" }, { 4, 0, "ST_PR10" }, { 5, 0, "ST_PR11" },
                { 10, 0, "ST_PR12" }, { 20, 0, "ST_PR13" }, { 25, 0, "ST_PR14" }, { 30, 0, "ST_PR15" },
                { 49, 2, "ST_PR16" }, { 50, 0, "ST_PR17" }, { 100, 0, "ST_PR18" },
            };
            const char* rank = "ST_PR19";
            for (const auto& r : RANKS) {
                if (hours <= r.maxHours && mins < r.lessMinutes) {
                    rank = r.key;
                    break;
                }
            }
            return line + 1 == lineIdx ? ShowText(rank) : 2;
        }
        case STAT_STRONGEST_GANG:
        case STAT_2ND_STRONGEST_GANG:
        case STAT_3RD_STRONGEST_GANG: {
            const auto rating = stat - STAT_STRONGEST_GANG;
            const auto gang   = CGangWars::GangRatings[rating];
            if (gang < 0) {
                return 0;
            }
            sprintf_s(gString2, "ST_GNG%d", gang);
            sprintf_s(gString, "ST_LAB%d", (int32)rating);
            const auto* const label = TheText.Get(gString);
            if (isLine) {
                GxtCharStrcpy(gGxtString, label);
                GxtCharStrcpy(gGxtString2, TheText.Get(gString2));
                return LINE_SHOWN;
            }

            // The strength of the gang, without a label
            sprintf_s(gString, "%d", CGangWars::GangRatingStrength[rating]);
            AsciiToGxtChar(gString, gGxtString2);
            if (line + 1 == lineIdx) {
                gGxtString[0] = '\0'; // The original also copied the value over itself
                return LINE_SHOWN;
            }
            return 2;
        }
        case STAT_MONEY_LOST_GAMBLING: {
            auto lost = (double)StatTypesFloat[STAT_MONEY_SPENT_GAMBLING] - (double)StatTypesFloat[STAT_MONEY_WON_GAMBLING];
            if (lost < 0.0) {
                lost = 0.0;
            }
            return isLine ? ShowFormatted("$%.2f", lost) : 1;
        }
        case STAT_TERRITORY_UNDER_CONTROL:
            return isLine ? ShowFormatted("%0.2f%%", (double)CGangWars::TerritoryUnderControlPercentage * 100.0) : 1;
        default:
            return 0;
        }
    };

    for (const auto& entry : GetStatLines(listId)) {
        if (!entry.alwaysShown && !(0.0f < RawStatAsFloat(entry.stat))) {
            continue;
        }

        sprintf_s(gString, entry.stat < 10 ? "STAT00%d" : entry.stat < 100 ? "STAT0%d" : "STAT%d", (int32)entry.stat);

        if (CLocalisation::GermanGame()) {
            switch ((uint8)entry.stat) {
            case 0xA7: case 0xA8: case 0xB1: case 0xCD: case 0xCE: case 0xCF: case 0xD0: case 0xD1:
                continue;
            }
        }

        m_ThisStatIsABarChart = 0;

        const auto numLines = ShowStat(entry);
        if (numLines == LINE_SHOWN) {
            return 0;
        }
        line += numLines;
    }
    return line;
}

// 0x55B900
void CStats::ProcessReactionStatsOnIncrement(eStats stat) {
    if (stat != STAT_STAMINA && stat != STAT_ENERGY && stat != STAT_LUNG_CAPACITY)
        return;

    float energy = GetStatValue(STAT_ENERGY);

    if (stat == STAT_STAMINA || stat == STAT_LUNG_CAPACITY) {
        if (energy < 0.0f) {
            StatTypesFloat[STAT_FAT] = std::max(StatTypesFloat[STAT_FAT] - 23.0f, 0.0f);
            CheckForStatsMessage();
        }
        return;
    }

    if (energy > 1000.0f)
        IncrementStat(STAT_FAT, energy - 1000.0f);
}

// 0x55B980
void CStats::DisplayScriptStatUpdateMessage(eStatUpdateState state, eStats stat, float value) {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CHud::HelpMessageDisplayed()
        || bStatUpdateMessageDisplayed
        || CMenuSystem::num_menus_in_use)
    {
        bStatUpdateMessageDisplayed = false;

        return;
    }

    if (IsStatCapped(stat) && GetStatValue(stat) >= 1000.0f)
        return;

    switch (stat) {
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_GAMBLING:
    case STAT_DRIVING_SKILL:
    case STAT_ARMOR:
    case STAT_ENERGY:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_TOTAL_RESPECT:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_CYCLING_SKILL:
    case STAT_LUCK:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_PROGRESS_WITH_DENISE:
    case STAT_PROGRESS_WITH_MICHELLE:
    case STAT_PROGRESS_WITH_HELENA:
    case STAT_PROGRESS_WITH_BARBARA:
    case STAT_PROGRESS_WITH_KATIE:
    case STAT_PROGRESS_WITH_MILLIE:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 100.0f);
        break;

    case STAT_PIMPING_LEVEL:
        CHud::SetHelpMessageStatUpdate(state, STAT_PIMPING_LEVEL, value, 10.0f);
        break;

    case STAT_GANG_STRENGTH: {
        if (auto player = FindPlayerPed())
        {
            auto maxGroup = std::min<uint8>(FindMaxNumberOfGroupMembers(), player->GetPlayerData()->m_nScriptLimitToGangSize);
            CHud::SetHelpMessageStatUpdate(state, stat, value, maxGroup);
        }
        break;
    }
    default:
        return;
    }
}

// 0x55BC50
void CStats::UpdateRespectStat(uint8 reset) {
    // The respect the player had on the previous update, and the one a message was last shown about
    static auto& lastRespect      = StaticRef<float>(0x8CDEC8);
    static auto& lastShownRespect = StaticRef<float>(0x8CDEC4);

    constexpr float NO_RESPECT = -99.0f;

    if (reset) {
        lastRespect      = NO_RESPECT;
        lastShownRespect = NO_RESPECT;
        return;
    }

    const auto missionRespect      = static_cast<float>(StatTypesInt[STAT_RESPECT_MISSION - FIRST_INT_STAT]);
    const auto missionRespectTotal = static_cast<float>(StatTypesInt[STAT_RESPECT_MISSION_TOTAL - FIRST_INT_STAT]);
    const auto territory           = std::max(0.0f, CGangWars::TerritoryUnderControlPercentage - 0.2f);
    const auto money               = std::min(1.0f, static_cast<float>(CWorld::Players[CWorld::PlayerInFocus].m_nMoney) * 1.0e-7f);
    const auto tagged              = static_cast<float>(CTagManager::GetPercentageTagged() * 10);

    auto respect = StatTypesFloat[STAT_RESPECT] * 0.4f
                 + (missionRespect * 1000.0f / std::max(1.0f, missionRespectTotal)) * 0.36f
                 + StatTypesFloat[STAT_GIRLFRIEND_RESPECT] * 0.03f
                 + territory * 1250.0f * 0.05f
                 + money * 1000.0f * 0.05f
                 + StatTypesFloat[STAT_MUSCLE] * 0.03f
                 + StatTypesFloat[STAT_CLOTHES_RESPECT] * 0.03f
                 + tagged * 0.05f;
    if (respect < 0.0f) {
        respect = 0.0f;
    }
    if (CCheat::IsActive(CHEAT_MAX_RESPECT)) {
        respect = 1000.0f;
    }

    // Nothing to do while the whole number stays the same
    if (static_cast<int32>(lastRespect) == static_cast<int32>(respect)) {
        return;
    }

    StatTypesFloat[STAT_TOTAL_RESPECT] = respect;
    CheckForStatsMessage();

    const auto CanShowMessage = [&] {
        return lastRespect != NO_RESPECT && lastShownRespect != NO_RESPECT;
    };

    if (respect <= lastShownRespect && lastRespect >= 2.0f) {
        // Dropped
        if (CanShowMessage() && (CheckForThreshold(&lastShownRespect, respect) || respect < 2.0f)) {
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_TOTAL_RESPECT, respect);
        }
        if (lastRespect < respect) {
            lastShownRespect = respect;
        }
    } else {
        // Went up
        if (CanShowMessage() && (CheckForThreshold(&lastShownRespect, respect) || lastRespect < 2.0f)) {
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_TOTAL_RESPECT, respect);
        }
        if (respect < lastRespect) {
            lastShownRespect = respect;
        }
    }
    lastRespect = respect;
}

// 0x55BF20
void CStats::UpdateSexAppealStat() {
    // The vehicle the player was last seen in, so the stat reacts to a car that was just left too
    static auto& lastPlayerVehicle = StaticRef<CVehicle*>(0xB79530);

    const auto appearanceShare = StatTypesFloat[STAT_APPEARANCE] * 0.5f;
    auto       vehicleShare    = 0.0f;

    if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
        lastPlayerVehicle = vehicle;
        CEntity::RegisterReference(lastPlayerVehicle);
    }

    if (lastPlayerVehicle) {
        const auto& vehiclePos = lastPlayerVehicle->GetPosition();
        const auto  playerPos  = FindPlayerCoors(-1);
        if (CVector2D{ vehiclePos - playerPos }.Magnitude() < 35.0f) {
            vehicleShare = std::clamp(lastPlayerVehicle->m_fHealth * 2.0f - 1000.0f, 0.0f, 1000.0f);

            // Flashy vehicles count the most
            switch (lastPlayerVehicle->GetVehicleModelInfo()->m_nVehicleClass) {
            case VEHICLE_CLASS_POORFAMILY:
            case VEHICLE_CLASS_BIG:
            case VEHICLE_CLASS_MOPED:
            case VEHICLE_CLASS_BICYCLE:
                vehicleShare *= 0.1f;
                break;
            case VEHICLE_CLASS_RICHFAMILY:
            case VEHICLE_CLASS_EXECUTIVE:
            case VEHICLE_CLASS_LEISUREBOAT:
                break;
            case VEHICLE_CLASS_WORKER:
            case VEHICLE_CLASS_TAXI:
            case VEHICLE_CLASS_WORKERBOAT:
                vehicleShare *= 0.3f;
                break;
            default:
                vehicleShare *= 0.5f;
                break;
            }
        }
    }

    auto& sexAppeal = StatTypesFloat[STAT_SEX_APPEAL];
    sexAppeal       = std::clamp(vehicleShare * 0.5f + appearanceShare, 0.0f, 1000.0f);
    if (CCheat::IsActive(CHEAT_MAX_SEX_APPEAL)) {
        sexAppeal = 1000.0f;
    }
    CheckForStatsMessage();
}

// 0x55C180
void CStats::IncrementStat(eStats stat, float value)
{
    if (value <= 0.0f)
        return;

    if (IsStatFloat(stat)) { // float
        StatTypesFloat[stat] += value;

        if (IsStatCapped(stat))
            StatTypesFloat[stat] = std::min(StatTypesFloat[stat], 1000.0f);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    CPlayerPed* player = FindPlayerPed();
    CPlayerInfo* playerInfo = player->GetPlayerInfoForThisPlayerPed();

    if (stat == STAT_CALORIES) {
        float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

        IncrementStat(STAT_RIOT_MISSION_ACCOMPLISHED, value);

        if (value > healthDiff) {
            float avg = (value - healthDiff) / 2.0f;

            IncrementStat(STAT_FAT, avg);
        }

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    if (stat != STAT_RIOT_MISSION_ACCOMPLISHED) {
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] += (int32)value;

        if (IsStatCapped(stat))
            StatTypesInt[stat - FIRST_INT_STAT] = std::min(StatTypesInt[stat - FIRST_INT_STAT], 1000);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    // STAT_RIOT_MISSION_ACCOMPLISHED increment, enum name incorrect?

    float kcals = playerInfo->m_nNumHoursDidntEat - value / 2.0f;
    kcals = std::clamp(kcals, 0.0f, 36.0f);

    float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

    if (value >= healthDiff) {
        playerInfo->m_nNumHoursDidntEat = 0;
    }

    player->m_fHealth += value;
    UpdateStatsAddToHealth((uint32)value);
    ProcessReactionStatsOnIncrement(stat);
    CheckForStatsMessage();
}

// 0x55C470
void CStats::UpdateFatAndMuscleStats(uint32 value) {
    // What the last change of fat and muscle was, for the message that is shown: 1 muscle gain, 2 fat loss, 3 muscle loss
    static auto& lastChange = StaticRef<uint8>(0xB79534);

    if (StatReactionValue[STAT_TIMELIMIT_FAT_ADJUST] * 1000.0f >= static_cast<float>(m_FatCounter)) {
        m_FatCounter += (static_cast<uint32>(CTimer::GetTimeStepInMS()) * value) / 10;
    } else {
        m_FatCounter = 0;
        if (StatTypesFloat[STAT_FAT] <= 0.0f) {
            // No fat left to burn, the muscle goes instead
            if (StatReactionValue[STAT_DEC_BODY_MUSCLE] > 0.0f) {
                StatTypesFloat[STAT_MUSCLE] = std::max(StatTypesFloat[STAT_MUSCLE] - StatReactionValue[STAT_DEC_BODY_MUSCLE], 0.0f);
                CheckForStatsMessage();
            }
            lastChange = 3;
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, StatReactionValue[STAT_DEC_BODY_MUSCLE]);
        } else {
            if (StatReactionValue[STAT_DEC_FAT] > 0.0f) {
                StatTypesFloat[STAT_FAT] = std::max(StatTypesFloat[STAT_FAT] - StatReactionValue[STAT_DEC_FAT], 0.0f);
                CheckForStatsMessage();
            }
            IncrementStat(STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);
            if (lastChange == 1) {
                lastChange = 2;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);
            } else {
                lastChange = 1;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, StatReactionValue[STAT_DEC_FAT]);
            }
        }
    }

    if (StatReactionValue[STAT_TIMELIMIT_MAX_HEALTH] * 1000.0f < static_cast<float>(m_MaxHealthCounter)) {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_MaxHealthCounter = 0;
    } else {
        m_MaxHealthCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    }
}

// 0x55C660
void CStats::UpdateStatsWhenSprinting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_SPRINT]));
    if (StatReactionValue[STAT_TIMELIMIT_SPRINT_STAMINA] * 1000.0f >= static_cast<float>(m_SprintStaminaCounter)) {
        m_SprintStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_SprintStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
    }
}

// 0x55C6F0
void CStats::UpdateStatsWhenRunning() {
    UpdateFatAndMuscleStats((uint32)StatReactionValue[STAT_EXERCISE_RATE_RUN]);
    if (StatReactionValue[STAT_TIMELIMIT_RUNNING] * 1000.0f >= static_cast<float>(m_RunningCounter)) {
        m_RunningCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_RunningCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
    }
}

// 0x55C780
void CStats::UpdateStatsWhenCycling(bool isSprinting, CBmx* bmx) {
    if (std::abs(bmx->m_GasPedal) > 0.0f || isSprinting) {
        UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[isSprinting ? STAT_EXERCISE_RATE_CYCLE_SPRINT : STAT_EXERCISE_RATE_CYCLE]));
        if (StatReactionValue[STAT_TIMELIMIT_CYCLE_STAMINA] * 1000.0f >= static_cast<float>(m_CycleStaminaCounter)) {
            m_CycleStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
        } else {
            m_CycleStaminaCounter = 0;
            IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
        }
    }

    if (StatReactionValue[STAT_TIMELIMIT_CYCLE_SKILL] * 1000.0f < static_cast<float>(m_CycleSkillCounter)) {
        m_CycleSkillCounter = 0;
        IncrementStat(STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        return;
    }

    // The faster the bike goes the more the skill grows, in the air it grows the most
    const auto speedSq = bmx->m_vecMoveSpeed.SquaredMagnitude();
    float      skillRate;
    if (bmx->m_nNoOfContactWheels < 2 && speedSq > sq(0.05f)) {
        skillRate = 3.0f;
    } else if (speedSq > sq(0.2f)) {
        skillRate = isSprinting ? 1.5f : 1.0f;
    } else {
        return;
    }

    const auto timeStepMs = static_cast<uint32>(CTimer::GetTimeStepInMS());
    m_CycleSkillCounter += static_cast<int32>(std::ceil(static_cast<double>(static_cast<float>(timeStepMs) * skillRate)));
}

// 0x55C990
void CStats::UpdateStatsWhenSwimming(bool isUnderwater, bool isSprinting) {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[isUnderwater || isSprinting ? STAT_EXERCISE_RATE_SWIM_SPRINT : STAT_EXERCISE_RATE_SWIM]));

    if (StatReactionValue[STAT_TIMELIMIT_SWIM_STAMINA] * 1000.0f >= static_cast<float>(m_SwimStaminaCounter)) {
        m_SwimStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_SwimStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
    }

    if (isUnderwater) {
        if (StatReactionValue[STAT_TIMELIMIT_BREATH_UNDERWATER] * 1000.0f >= static_cast<float>(m_SwimUnderWaterCounter)) {
            m_SwimUnderWaterCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
        } else {
            m_SwimUnderWaterCounter = 0;
            IncrementStat(STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
        }
    }
}

// 0x55CAC0
void CStats::UpdateStatsWhenDriving(CVehicle* vehicle) {
    const auto* const automobile = vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE ? vehicle->AsAutomobile() : nullptr;
    const auto        counter    = static_cast<float>(m_DrivingCounter);

    if (StatReactionValue[STAT_TIMELIMIT_DRIVING_SKILL] * 1000.0f < counter) {
        m_DrivingCounter = 0;
        IncrementStat(STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        return;
    }

    // Driving fast or with no wheel on the ground counts more
    const auto timeStepMs = static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()));
    const auto speed      = vehicle->m_vecMoveSpeed.Magnitude();
    if (speed > 0.8f || (automobile && automobile->m_nNumContactWheels == 0)) {
        m_DrivingCounter = static_cast<int32>(timeStepMs * 1.5f + counter);
    } else if (speed > 0.2f) {
        m_DrivingCounter = static_cast<int32>(timeStepMs * 0.5f + counter);
    }
}

// 0x55CC00
void CStats::UpdateStatsWhenFlying(CVehicle* vehicle) {
    if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE) {
        return;
    }

    const auto counter = static_cast<float>(m_FlyingCounter);
    if (StatReactionValue[STAT_TIMELIMIT_FLYING_SKILL] * 1000.0f < counter) {
        m_FlyingCounter = 0;
        IncrementStat(STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        return;
    }

    // Only counts while no wheel touches the ground
    if (vehicle->AsAutomobile()->m_nNumContactWheels != 0) {
        return;
    }

    const auto timeStepMs = static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()));
    const auto speed      = vehicle->m_vecMoveSpeed.Magnitude();
    if (speed > 1.3f || vehicle->GetUp().z < 0.0f) {
        m_FlyingCounter = static_cast<int32>(timeStepMs * 1.5f + counter);
    } else if (speed > 0.5f) {
        m_FlyingCounter = static_cast<int32>(timeStepMs * 0.5f + counter);
    }
}

// 0x55CD60
void CStats::UpdateStatsWhenOnMotorBike(CBike* bike) {
    auto bikeCounter = static_cast<float>(m_BikeCounter);
    if (StatReactionValue[STAT_TIMELIMIT_MOTORBIKE_SKILL] * 1000.0f >= bikeCounter) {
        const float bikeMoveSpeed = bike->m_vecMoveSpeed.Magnitude();
        const auto  fTimeStep = CTimer::GetTimeStepInMS();

        if (bikeMoveSpeed > 0.6f || bike->m_nNoOfContactWheels < 3u && bikeMoveSpeed > 0.1f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 1.5f + bikeCounter);
        else if (bikeMoveSpeed > 0.2f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 0.5f + bikeCounter);
    } else {
        m_BikeCounter = 0;
        IncrementStat(STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
    }
}

// 0x55CEB0
void CStats::UpdateStatsWhenWeaponHit(eWeaponType weaponType) {
    const auto skill = CWeaponInfo::GetSkillStatIndex(weaponType);
    const auto index = skill - STAT_PISTOL_SKILL;

    if (CGameLogic::IsCoopGameGoingOn() || GetStatValue(skill) >= 1000.0f) {
        return;
    }

    IncrementStat(skill, StatReactionValue[STAT_INC_PISTOL_SKILL + index]);

    // Hits with another weapon than the one before start the counting again
    if (m_LastWeaponTypeFired != static_cast<uint32>(index)) {
        m_LastWeaponTypeFired = index;
        m_WeaponCounter       = 0;
        return;
    }

    const auto counter = static_cast<float>(m_WeaponCounter);
    if (StatReactionValue[STAT_TIMELIMIT_PISTOL_SKILL + index] < counter) {
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, skill, counter * StatReactionValue[STAT_INC_PISTOL_SKILL + index]);
        m_WeaponCounter = 0;
    } else {
        m_WeaponCounter++;
    }
}

// 0x55CFA0
void CStats::UpdateStatsWhenFighting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_FIGHT]));
}

// 0x55CFC0
void CStats::UpdateStatsOnRespawn() {
    if (StatReactionValue[STAT_TIMELIMIT_DEATH_HEALTH] < static_cast<float>(m_DeathCounter)) {
        if (StatTypesFloat[STAT_MAX_HEALTH] > 400.0f) {
            IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
        }
        m_DeathCounter = 0;
        return;
    }
    m_DeathCounter++;
}

// 0x55D030
void CStats::UpdateStatsAddToHealth(uint32 addToHealth) {
    m_AddToHealthCounter += addToHealth;
    if (StatReactionValue[STAT_TIMELIMIT_ADD_TO_HEALTH] < static_cast<float>(m_AddToHealthCounter)) {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_AddToHealthCounter = 0;
    }
}

// 0x55D090
void CStats::ModifyStat(eStats stat, float value) {
    if (value < 0.0f) {
        CStats::DecrementStat(stat, -value);
    } else {
        CStats::IncrementStat(stat, value);
    }
}

// 0x5D3B40
bool CStats::Save() {
    IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
    SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);

    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesFloat);
    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesInt);
    CGenericGameStorage::SaveDataToWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::SaveDataToWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::SaveDataToWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::SaveDataToWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::SaveDataToWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::SaveDataToWorkBuffer(statMessage.displayed);
    }
    return true;
}

// 0x5D3BF0
bool CStats::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesFloat);
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesInt);
    CGenericGameStorage::LoadDataFromWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::LoadDataFromWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::LoadDataFromWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::LoadDataFromWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::LoadDataFromWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::LoadDataFromWorkBuffer(statMessage.displayed);
    }
    return true;
}

// Unused
// 0x558DE0
char* CStats::GetStatID(eStats stat) {
    if (!IsStatFloat(stat)) // int32
        sprintf_s(gString, "stat_i_%d", stat);
    else
        sprintf_s(gString, "stat_f_%d", stat);

    return gString;
}

// Unused
// 0x558E70
int8 CStats::GetTimesMissionAttempted(uint8 missionId) {
    return TimesMissionAttempted[missionId];
}

// Unused
// 0x558E80
void CStats::RegisterMissionAttempted(uint8 missionId) {
    if (TimesMissionAttempted[missionId] != -1) {
        TimesMissionAttempted[missionId]++;
    }
}

// Unused
// 0x558EA0
void CStats::RegisterMissionPassed(uint8 missionId) {
    TimesMissionAttempted[missionId] = -1;
}
