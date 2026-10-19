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
    RH_ScopedInstall(PopulateFavoriteRadioStationList, 0x558EC0, { .Reversed = false });
    RH_ScopedInstall(FindMostFavoriteRadioStation, 0x558FA0, { .Reversed = false });
    RH_ScopedInstall(FindLeastFavoriteRadioStation, 0x559010, { .Reversed = false });
    RH_ScopedInstall(BuildStatLine, 0x559230, { .Reversed = false });
    RH_ScopedInstall(CheckForStatsMessage, 0x559760, { .Reversed = false });
    RH_ScopedInstall(LoadStatUpdateConditions, 0x559860, { .Reversed = false });
    RH_ScopedInstall(GetFatAndMuscleModifier, 0x559AF0);
    RH_ScopedInstall(FindCriminalRatingString, 0x55A210, { .Reversed = false });
    RH_ScopedInstall(ConstructStatLine, 0x55A780, { .Reversed = false });
    RH_ScopedInstall(UpdateRespectStat, 0x55BC50, { .Reversed = false });
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
    return plugin::CallAndReturn<bool, 0x558EC0>();
}

// 0x558FA0
eRadioID CStats::FindMostFavoriteRadioStation() {
    return plugin::CallAndReturn<eRadioID, 0x558FA0>();
}

// 0x559010
int32 CStats::FindLeastFavoriteRadioStation() {
    return plugin::CallAndReturn<int32, 0x559010>();
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
    return std::min(StatTypesFloat[STAT_PROGRESS_MADE] / 187.0f * 100.0f, 100.0f);
}

// 0x559230
void CStats::BuildStatLine(char* line, void* pValue1, int32 metrics, void* pValue2, int32 type) {
    plugin::Call<0x559230, char*, void*, int32, void*, int32>(line, pValue1, metrics, pValue2, type);
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
    plugin::Call<0x559760>();
}

// 0x559860
void CStats::LoadStatUpdateConditions() {
    plugin::Call<0x559860>();
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
GxtChar* CStats::FindCriminalRatingString() {
    return plugin::CallAndReturn<GxtChar*, 0x55A210>();
}

// 0x55A780
int32 CStats::ConstructStatLine(int32 arg0, uint8 arg1) {
    return plugin::CallAndReturn<int32, 0x55A780, int32, uint8>(arg0, arg1);
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
void CStats::UpdateRespectStat(uint8 arg0) {
    plugin::Call<0x55BC50, uint8>(arg0);
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
