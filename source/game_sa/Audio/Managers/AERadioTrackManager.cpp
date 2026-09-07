#include "StdInc.h"

#include "AERadioTrackManager.h"
#include "RadioStreamsPC.h"

#include "AEAudioHardware.h"
#include "AEUserRadioTrackManager.h"
#include "AEAudioUtility.h"
#include "AEAudioEnvironment.h"

auto& AERadioTrackManager = StaticRef<CAERadioTrackManager>(0x8CB6F8);

// Per-talk-show segment duration (ms), indexed by `tRadioSettings::TrackIndices[0]`. Only used for
// `RADIO_TALK` (WCTR, 31 shows) - not yet extracted into `RadioStreamsPC.h`, so referenced directly.
static auto& gRadioTalkShowDurationsMs = StaticRef<std::array<int32, 31>>(0x8CAD50);

void CAERadioTrackManager::InjectHooks() {
    RH_ScopedClass(CAERadioTrackManager);
    RH_ScopedCategory("Audio/Managers");

    RH_ScopedInstall(Load, 0x5D40E0);
    RH_ScopedInstall(Save, 0x5D3EE0);
    RH_ScopedInstall(Initialise, 0x5B9390);
    RH_ScopedInstall(Service, 0x4EB9A0);
    RH_ScopedInstall(DisplayRadioStationName, 0x4E9E50);
    RH_ScopedInstall(CheckForStationRetune, 0x4EB660);
    RH_ScopedInstall(CheckForPause, 0x4EA590);
    RH_ScopedInstall(IsVehicleRadioActive, 0x4E9800);
    RH_ScopedInstall(AddDJBanterIndexToHistory, 0x4E97B0);
    RH_ScopedInstall(AddAdvertIndexToHistory, 0x4E9760);
    RH_ScopedInstall(AddIdentIndexToHistory, 0x4E9720);
    RH_ScopedInstall(AddMusicTrackIndexToHistory, 0x4E96C0);
    RH_ScopedOverloadedInstall(StartRadio, "manual", 0x4EB3C0, void (CAERadioTrackManager::*)(eRadioID, eBassSetting, float, bool));
    RH_ScopedOverloadedInstall(StartRadio, "with-settings", 0x4EB550, void (CAERadioTrackManager::*)(const tVehicleAudioSettings&));
    RH_ScopedInstall(CheckForStationRetuneDuringPause, 0x4EB890);
    RH_ScopedInstall(TrackRadioStation, 0x4EAC30);
    RH_ScopedInstall(ChooseTracksForStation, 0x4EB180);
    RH_ScopedInstall(CheckForTrackConcatenation, 0x4EA930);
    RH_ScopedInstall(QueueUpTracksForStation, 0x4EA670);
    RH_ScopedInstall(ChooseDJBanterIndex, 0x4EA2D0);
    RH_ScopedInstall(ChooseDJBanterIndexFromList, 0x4E95E0);
    RH_ScopedInstall(ChooseAdvertIndex, 0x4E9570);
    RH_ScopedInstall(ChooseIdentIndex, 0x4E94C0);
    RH_ScopedInstall(ChooseMusicTrackIndex, 0x4EA270);
    RH_ScopedInstall(ChooseTalkRadioShow, 0x4E8E40);
    RH_ScopedInstall(CheckForMissionStatsChanges, 0x4E8410);
    RH_ScopedInstall(StartTrackPlayback, 0x4EA640);
    RH_ScopedInstall(UpdateRadioVolumes, 0x4EA010, { .reversed = false });
    RH_ScopedInstall(PlayRadioAnnouncement, 0x4E8400);
    RH_ScopedInstall(GetCurrentRadioStationID, 0x4E83F0);
    RH_ScopedInstall(GetRadioStationListenTimes, 0x4E83E0);
    RH_ScopedInstall(GetRadioStationName, 0x4E9E10);
    RH_ScopedInstall(GetRadioStationNameKey, 0x4E8380);
    RH_ScopedInstall(HasRadioRetuneJustStarted, 0x4E8370);
    RH_ScopedInstall(StopRadio, 0x4E9820);
    RH_ScopedInstall(IsRadioOn, 0x4E8350, { .reversed = true });
    RH_ScopedInstall(InitialiseRadioStationID, 0x4E8330);
    RH_ScopedInstall(SetBassEnhanceOnOff, 0x4E9DB0);
    RH_ScopedInstall(SetBassSetting, 0x4E82F0);
    RH_ScopedInstall(SetRadioAutoRetuneOnOff, 0x4E82E0);
    RH_ScopedInstall(RetuneRadio, 0x4E8290, { .reversed = true });
    RH_ScopedInstall(ResetStatistics, 0x4E8200);
    RH_ScopedInstall(Reset, 0x4E7F80, { .reversed = true });
}

// Code from 0x5B9390
CAERadioTrackManager::CAERadioTrackManager(int32 hwClientHandle) :
    m_HwClientHandle{ hwClientHandle },
    m_nUserTrackPlayMode{ AEUserRadioTrackManager.GetUserTrackPlayMode() }
{
    // All constant value inits are done using member init lists

    rng::copy(CStats::GetFullFavoriteRadioStationList(), m_aListenTimes.begin());

    for (auto i = 0u; i < RADIO_COUNT; i++) {
        m_nMusicTrackIndexHistory[i].Reset();
        m_nDJBanterIndexHistory[i].Reset();
        m_nAdvertIndexHistory[i].Reset();
        m_nIdentIndexHistory[i].Reset();
    }

    // [1st radio, off]
    m_RequestedSettings = m_ActiveSettings = tRadioSettings{CAEAudioUtility::GetRandomRadioStation()};
}

// 0x5B9390
bool CAERadioTrackManager::Initialise(int32 channelId) {
    *this = CAERadioTrackManager{};
    return true;
}

// 0x4E8330
void CAERadioTrackManager::InitialiseRadioStationID(eRadioID id) {
    m_RequestedSettings.StationID = m_ActiveSettings.StationID = id;
}

// 0x4E7F80
void CAERadioTrackManager::Reset() {
    m_bInitialised = false;
    m_bDisplayStationName = false;
    rng::copy(CStats::GetFullFavoriteRadioStationList(), m_aListenTimes.begin());

    rng::for_each(m_nDJBanterIndexHistory, &DJBanterIndexHistory::Reset);
    rng::for_each(m_nAdvertIndexHistory, &AdvertIndexHistory::Reset);
    rng::for_each(m_nIdentIndexHistory, &IdentIndexHistory::Reset);
    rng::for_each(m_nMusicTrackIndexHistory, &MusicTrackHistory::Reset);
    rng::for_each(m_aRadioState, [](auto& s) { s.Reset(); });

    m_RequestedSettings = m_ActiveSettings = tRadioSettings{CAEAudioUtility::GetRandomRadioStation()};
    m_nStationsListed = m_nStationsListDown = 0;
    m_nTimeRadioStationRetuned = m_nTimeToDisplayRadioName = 0;
    m_prev = field_60 = 0;
    m_nRetuneStartedTime = 0;
    m_bEnabledInPauseMode = false;
    m_nSavedGameClockDays = m_nSavedGameClockHours = -1;
    m_bRadioAutoSelect = m_bBassEnhance = true;
    m_nSavedRadioStationId = m_iRadioStationMenuRequest = m_iRadioStationScriptRequest = RADIO_INVALID;
    m_nSpecialDJBanterPending = 3; // todo: enum
    m_nSpecialDJBanterIndex = -1;
    m_bPauseMode = m_bRetuneJustStarted = false;
    m_f80 = m_f84 = 0.0f;
    ResetStatistics();
}

// 0x4E8200
void CAERadioTrackManager::ResetStatistics() {
    m_nStatsCitiesPassed = 0;
    m_nStatsLastHitGameClockDays = -1;
    m_nStatsLastHitGameClockHours = -1;
    m_nStatsLastHitTimeOutHours = -1;
    m_nStatsPassedCasino3 = false;
    m_nStatsPassedCasino6 = false;
    m_nStatsPassedCasino10 = false;
    m_nStatsPassedCat1 = false;
    m_nStatsPassedDesert1 = false;
    m_nStatsPassedDesert3 = false;
    m_nStatsPassedDesert5 = false;
    m_nStatsPassedDesert8 = false;
    m_nStatsPassedDesert10 = false;
    m_nStatsPassedFarlie3 = false;
    m_nStatsPassedLAFin2 = false;
    m_nStatsPassedMansion2 = false;
    m_nStatsPassedRyder2 = false;
    m_nStatsPassedRiot1 = false;
    m_nStatsPassedSCrash1 = false;
    m_nStatsPassedStrap4 = false;
    m_nStatsPassedSweet2 = false;
    m_nStatsPassedTruth2 = false;
    m_nStatsPassedVCrash2 = false;
    m_nStatsStartedBadlands = false;
    m_nStatsStartedCat2 = false;
    m_nStatsStartedCrash1 = false;
}

// 0x4E8350
bool CAERadioTrackManager::IsRadioOn() const {
    return m_nMode != eRadioTrackMode::RADIO_STOPPED || m_bInitialised || m_nStationsListed || m_nStationsListDown;
}

// 0x4E8370
bool CAERadioTrackManager::HasRadioRetuneJustStarted() const {
    return m_bRetuneJustStarted;
}

// 0x4E83E0
int32* CAERadioTrackManager::GetRadioStationListenTimes() {
    return m_aListenTimes.data();
}

// 0x4E83F0
eRadioID CAERadioTrackManager::GetCurrentRadioStationID() const {
    return m_RequestedSettings.StationID == RADIO_INVALID ? RADIO_OFF : m_RequestedSettings.StationID;
}

// 0x4E82E0
void CAERadioTrackManager::SetRadioAutoRetuneOnOff(bool enable) {
    m_bRadioAutoSelect = enable;
}

// 0x4E82F0
void CAERadioTrackManager::SetBassSetting(eBassSetting bassSetting, float bassGrain) {
    m_RequestedSettings.BassGain = m_ActiveSettings.BassGain = bassGrain;
    m_RequestedSettings.BassSetting = m_ActiveSettings.BassSetting = bassSetting;
    AEAudioHardware.SetBassSetting(m_bBassEnhance ? bassSetting : eBassSetting::NORMAL, bassGrain);
}

// 0x4E9DB0
void CAERadioTrackManager::SetBassEnhanceOnOff(bool enable) {
    m_bBassEnhance = enable;
    if (m_nMode == eRadioTrackMode::RADIO_PLAYING) {
        m_RequestedSettings.BassSetting = m_ActiveSettings.BassSetting;
        m_RequestedSettings.BassGain = m_ActiveSettings.BassGain;
        if (enable) {
            AEAudioHardware.SetBassSetting(m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain);
        } else {
            AEAudioHardware.SetBassSetting(eBassSetting::NORMAL, m_ActiveSettings.BassGain);
        }
    }
}

// 0x4E8290
void CAERadioTrackManager::RetuneRadio(eRadioID id) {
    const auto retunedStation = [id] {
        if (id == RADIO_USER_TRACKS && !AEUserRadioTrackManager.m_nUserTracksCount) {
            return RADIO_OFF;
        } else {
            return id;
        }
    }();

    if (CTimer::GetIsPaused()) {
        m_iRadioStationMenuRequest = retunedStation;
        m_nRetuneStartedTime = CTimer::GetTimeInMSPauseMode();
    } else {
        m_iRadioStationScriptRequest = retunedStation;
    }
}

// 0x4E9E50
void CAERadioTrackManager::DisplayRadioStationName() {
    if (CTimer::GetIsPaused())
        return;

    if (TheCamera.m_bWideScreenOn)
        return;

    if (!FindPlayerVehicle())
        return;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    if (m_bDisplayStationName && IsVehicleRadioActive()) {
        m_nTimeToDisplayRadioName = CTimer::GetTimeInMS() + 2500;
        m_bDisplayStationName = false;
    }

    if (CTimer::GetTimeInMS() < m_nTimeToDisplayRadioName) {
        int station = m_nStationsListed + m_RequestedSettings.StationID;
        if (station) {
            if (station >= RADIO_COUNT) {
                station -= RADIO_COUNT - 1;
            } else if (station <= 0) {
                station += RADIO_COUNT - 1;
            }

            CFont::SetFontStyle(eFontStyle::FONT_MENU);
            CFont::SetJustify(false);
            CFont::SetBackground(false, false);
            CFont::SetScale(SCREEN_SCALE_X(0.6f), SCREEN_SCALE_Y(0.9f));
            CFont::SetProportional(true);
            CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
            CFont::SetRightJustifyWrap(0.0f);
            CFont::SetEdge(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetColor(HudColour.GetRGB(m_nStationsListed || m_nStationsListDown ? HUD_COLOUR_DARK_GRAY : HUD_COLOUR_GOLD));
            CFont::PrintString(SCREEN_WIDTH / 2, SCREEN_SCALE_Y(22.0f), GetRadioStationName((eRadioID)station));
            CFont::DrawFonts();
        }
    }
}

// 0x4E9E10
const GxtChar* CAERadioTrackManager::GetRadioStationName(eRadioID id) {
    if (id <= 0) {
        NOTSA_UNREACHABLE();
        return nullptr;
    }

    char key[8];
    GetRadioStationNameKey(id, key);
    return TheText.Get(key);
}

// 0x4E8380
void CAERadioTrackManager::GetRadioStationNameKey(eRadioID id, char* outStr) {
    switch (id) {
    case RADIO_OFF:
        *std::format_to_n(outStr, 7u, "FEA_NON").out = '\0';
        break;
    case RADIO_USER_TRACKS:
        *std::format_to_n(outStr, 7u, "FEA_MP3").out = '\0';
        break;
    default:
        assert(0 <= id && id < RADIO_USER_TRACKS);
        *std::format_to_n(outStr, 7u, "FEA_R{:d}", (int32)id - 1).out = '\0';
        break;
    }
}

// 0x4E9800
bool CAERadioTrackManager::IsVehicleRadioActive() {
    if (const auto opts = CAEVehicleAudioEntity::StaticGetPlayerVehicleAudioSettingsForRadio()) {
        switch (opts->RadioType) {
        case AE_RT_CIVILIAN:
        case AE_RT_EMERGENCY:
        case AE_RT_UNKNOWN:
            return true;
        default:
            break;
        }
    }
    return false;
}

// 0x4E8410
void CAERadioTrackManager::CheckForMissionStatsChanges() {
    if (m_nSpecialDJBanterPending != 3) {
        int32 days = CClock::GetGameClockDays() - m_nStatsLastHitGameClockDays;
        if (days < 0) {
            auto month = CClock::GetGameClockMonth() - 1;
            if (month < 0) {
                month += 12;
            }
            days += CClock::daysInMonth[month];
        }
        if (CClock::GetGameClockHours() + 24 * days - m_nStatsLastHitGameClockHours >= m_nStatsLastHitTimeOutHours) {
            m_nSpecialDJBanterPending = 3;
        }
    }

    const auto statsCitiesPassed = CStats::GetStatValue<uint8>(STAT_CITY_UNLOCKED);
    if (m_nStatsCitiesPassed < statsCitiesPassed) {
        m_nStatsCitiesPassed = statsCitiesPassed;
        if (statsCitiesPassed == 1 || statsCitiesPassed == 2) {
            m_nStatsLastHitGameClockDays = CClock::GetGameClockDays();
            m_nStatsLastHitGameClockHours = CClock::GetGameClockHours();
            m_nStatsLastHitTimeOutHours = 24;
            m_nSpecialDJBanterPending = 1;
            m_nSpecialDJBanterIndex = m_nStatsCitiesPassed - 1;
        }
    }

    const auto Update = [](uint8& inputStat, const eStats stat, const auto specialDJBanterIndex) {
        const auto statValue = CStats::GetStatValue<uint8>(stat);
        if (inputStat < statValue) {
            inputStat = statValue;
            if (inputStat == 1) {
                m_nStatsLastHitGameClockDays = CClock::GetGameClockDays();
                m_nStatsLastHitGameClockHours = CClock::GetGameClockHours();
                m_nStatsLastHitTimeOutHours = 24 * 7;
                m_nSpecialDJBanterPending = 2;
                m_nSpecialDJBanterIndex = specialDJBanterIndex;
            }
        }
    };

    Update(m_nStatsPassedCasino3, STAT_LEAST_FAVORITE_RADIO_STATION, 0);
    Update(m_nStatsPassedCasino6, STAT_CURRENT_WEAPON_SKILL, 1);
    Update(m_nStatsPassedCasino10, STAT_WEAPON_SKILL_LEVELS, 2);
    Update(m_nStatsPassedCat1, STAT_LOCAL_LIQUOR_STORE_MISSION_ACCOMPLISHED, 3);
    Update(m_nStatsPassedDesert1, STAT_PLAYING_TIME, 4);
    Update(m_nStatsPassedDesert3, STAT_PILOT_RANKING, 5);
    Update(m_nStatsPassedDesert5, STAT_STRONGEST_GANG, 6);
    Update(m_nStatsPassedDesert8, STAT_2ND_STRONGEST_GANG, 7);
    Update(m_nStatsPassedDesert10, STAT_3RD_STRONGEST_GANG, 8);
    Update(m_nStatsPassedFarlie3, STAT_MIKE_TORENO_MISSION_ACCOMPLISHED, 9);
    Update(m_nStatsPassedLAFin2, STAT_LEAST_FAVORITE_GANG, 10);
    Update(m_nStatsPassedMansion2, STAT_A_HOME_IN_THE_HILLS_MISSION_ACCOMPLISHED, 11);
    Update(m_nStatsPassedRyder2, STAT_RYDERS_MISSION_ROBBING_UNCLE_SAM_ACCOMPLISHED, 12);
    Update(m_nStatsPassedRiot1, STAT_RIOT_MISSION_ACCOMPLISHED, 13);
    Update(m_nStatsPassedSCrash1, STAT_GANG_STRENGTH, 14);
    Update(m_nStatsPassedStrap4, STAT_TERRITORY_UNDER_CONTROL, 15);
    Update(m_nStatsPassedSweet2, STAT_DRIVE_THRU_MISSION_ACCOMPLISHED, 16);
    Update(m_nStatsPassedTruth2, STAT_ARE_YOU_GOING_TO_SAN_FIERRO_MISSION_ACCOMPLISHED, 17);
    Update(m_nStatsPassedVCrash2, STAT_HIGH_NOON_MISSION_ACCOMPLISHED, 18);
    Update(m_nStatsStartedBadlands, STAT_THE_GREEN_SABRE_MISSION_ACCOMPLISHED, 19);
    Update(m_nStatsStartedCat2, STAT_MAYBE_CATALINA_MEETING, 20);
    Update(m_nStatsStartedCrash1, STAT_MAYBE_WU_ZI_MEETING, 21);
}

// 0x4EA930
void CAERadioTrackManager::CheckForTrackConcatenation() {
    int8 count = 1;

    if (m_ActiveSettings.StationID == RADIO_USER_TRACKS) {
        const auto oldMode = m_nUserTrackPlayMode;
        const auto newMode = AEUserRadioTrackManager.GetUserTrackPlayMode();
        if (oldMode != newMode) {
            if ((oldMode == eRadioMode::RADIO_MODE_SEQUENTIAL || AEUserRadioTrackManager.GetUserTrackPlayMode() == eRadioMode::RADIO_MODE_SEQUENTIAL)
                && m_ActiveSettings.PlayTime != -4)
            {
                AEUserRadioTrackManager.SetUserTrackIndex(m_ActiveSettings.TrackQueue[0]);

                m_ActiveSettings.TrackQueue[1] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[1] = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[1] = (int8)m_ActiveSettings.TrackQueue[1];
                count = 2;

                AEAudioHardware.PlayTrack(
                    m_ActiveSettings.TrackQueue[0],
                    m_ActiveSettings.TrackQueue[1],
                    0u,
                    m_ActiveSettings.TrackFlags,
                    m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK,
                    m_ActiveSettings.TrackTypes[1] == TYPE_USER_TRACK
                );
            }
            m_nUserTrackPlayMode = AEUserRadioTrackManager.GetUserTrackPlayMode();
        }
    }

    const auto nextTrack = m_ActiveSettings.TrackQueue[1];
    if (AEAudioHardware.GetActiveTrackID() != nextTrack || nextTrack < 0) {
        return;
    }

    m_ActiveSettings.SwitchToNextTrack();

    if (m_ActiveSettings.TrackQueue[1] == -1) {
        const auto id = m_ActiveSettings.StationID;
        if (id == RADIO_USER_TRACKS) {
            if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
                m_ActiveSettings.TrackQueue[count] = ChooseAdvertIndex(id);
                m_ActiveSettings.TrackTypes[count] = TYPE_ADVERT;
                count++;
            }

            m_ActiveSettings.TrackQueue[count] = AEUserRadioTrackManager.SelectUserTrackIndex();
            m_ActiveSettings.TrackTypes[count] = TYPE_USER_TRACK;
            m_ActiveSettings.TrackIndices[count] = (int8)m_ActiveSettings.TrackQueue[count];
            count++;

            m_ActiveSettings.TrackQueue[count] = AEUserRadioTrackManager.SelectUserTrackIndex();
            m_ActiveSettings.TrackTypes[count] = TYPE_USER_TRACK;
            m_ActiveSettings.TrackIndices[count] = (int8)m_ActiveSettings.TrackQueue[count];
            count++;
        } else {
            switch (m_ActiveSettings.TrackTypes[0]) {
            case TYPE_INTRO:
            case TYPE_TRACK:
            case TYPE_OUTRO:
                if (id == RADIO_EMERGENCY_AA) {
                    QueueUpTracksForStation(RADIO_EMERGENCY_AA, &count, TYPE_DJ_BANTER, m_ActiveSettings);
                } else if (m_nTracksInARow[id] < 2 && CAEAudioUtility::ResolveProbability(0.5f)) {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(id, &count, TYPE_INDENT, m_ActiveSettings);
                    }
                    QueueUpTracksForStation(id, &count, TYPE_INTRO, m_ActiveSettings);
                } else {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(id, &count, TYPE_INDENT, m_ActiveSettings);
                    }
                    if (!QueueUpTracksForStation(id, &count, TYPE_DJ_BANTER, m_ActiveSettings)) {
                        QueueUpTracksForStation(id, &count, TYPE_ADVERT, m_ActiveSettings);
                    }
                }
                break;
            default:
                QueueUpTracksForStation(id, &count, TYPE_INTRO, m_ActiveSettings);
                break;
            }
        }
    }

    AEAudioHardware.PlayTrack(
        m_ActiveSettings.TrackQueue[0],
        m_ActiveSettings.TrackQueue[1],
        0u,
        m_ActiveSettings.TrackFlags,
        m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK,
        m_ActiveSettings.TrackTypes[1] == TYPE_USER_TRACK
    );
}

// 0x4EB660
void CAERadioTrackManager::CheckForStationRetune() {
    if (m_ActiveSettings.StationID == RADIO_EMERGENCY_AA) {
        return;
    }

    m_bRetuneJustStarted = false;

    if (m_nMode == eRadioTrackMode::RADIO_STARTING || m_nMode == eRadioTrackMode::RADIO_WAITING_TO_PLAY ||
        m_nMode == eRadioTrackMode::RADIO_PLAYING || m_bInitialised || m_nStationsListed != 0 ||
        m_nStationsListDown != 0 || m_ActiveSettings.StationID == RADIO_OFF)
    {
        if (!AudioEngine.GetCutsceneTrackStatus()) {
            // Opaque, obfuscated pointer-decrypt helper (same one used in Service()); raw-called.
            if (const auto ptr = plugin::CallAndReturn<int32, 0x4F4ED0>()) {
                const auto b = *reinterpret_cast<int8*>(ptr + 0x1b);
                if ((b == 0 || b == 3 || b == 2) && CReplay::Mode != MODE_PLAYBACK) {
                    const auto ptr2 = plugin::CallAndReturn<int32, 0x4F4ED0>();
                    if (*reinterpret_cast<int8*>(ptr2 + 0x1b) != 0) {
                        return;
                    }

                    if (m_iRadioStationScriptRequest >= 0) {
                        m_nStationsListDown = m_nStationsListed;
                        m_nStationsListed = m_iRadioStationScriptRequest - m_RequestedSettings.StationID;
                        m_iRadioStationScriptRequest = -1;
                        m_nTimeRadioStationRetuned = CTimer::GetTimeInMS();
                    } else if (CPad::GetPad()->NextStationJustUp()) {
                        m_nStationsListDown = m_nStationsListed;
                        m_nStationsListed++;
                        m_nTimeRadioStationRetuned = CTimer::GetTimeInMS();
                    } else if (CPad::GetPad()->LastStationJustUp()) {
                        m_nStationsListDown = m_nStationsListed;
                        m_nStationsListed--;
                        m_nTimeRadioStationRetuned = CTimer::GetTimeInMS();
                    }
                    m_bDisplayStationName = true;
                    m_bRetuneJustStarted = true;
                }
            }
        }
    }

    if (m_nStationsListed == 0 && m_nStationsListDown == 0) {
        return;
    }

    auto wrapped = static_cast<int8>(m_RequestedSettings.StationID + m_nStationsListed);
    if (wrapped < 1) {
        wrapped += 13;
    } else if (wrapped > 13) {
        wrapped -= 13;
    }

    if (wrapped == RADIO_OFF || (wrapped == RADIO_USER_TRACKS && AEUserRadioTrackManager.m_nUserTracksCount == 0)) {
        StopRadio(nullptr, false);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_OFF);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
    } else {
        if (m_ActiveSettings.StationID == RADIO_OFF) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON);
            m_ActiveSettings.StationID = RADIO_INVALID;
        } else {
            StopRadio(nullptr, false);
        }
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);

        // Unidentified global (no other xrefs in the binary) - some kind of quality/detail setting.
        const auto timeoutMs = StaticRef<float>(0xB6F14C) <= 0.9f ? 2000u : 4000u;
        if (CTimer::GetTimeInMS() <= m_nTimeRadioStationRetuned + 1500u) {
            return;
        }
        if (CTimer::GetTimeInMS() <= field_60 + timeoutMs) {
            return;
        }
    }

    StartRadio(static_cast<eRadioID>(wrapped), m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, false);
    m_nStationsListed = 0;
    m_nStationsListDown = 0;
}

// 0x4EB890
void CAERadioTrackManager::CheckForStationRetuneDuringPause() {
    if (m_ActiveSettings.StationID == RADIO_EMERGENCY_AA && IsRadioOn() || m_iRadioStationMenuRequest <= RADIO_INVALID)
        return;

    if (m_iRadioStationMenuRequest != RADIO_OFF) {
        if (m_ActiveSettings.StationID == RADIO_OFF) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON);
            m_ActiveSettings.StationID = RADIO_INVALID;
        } else {
            AudioEngine.StopRadio(nullptr, true);
        }

        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);
        if (CTimer::GetTimeInMSPauseMode() > m_nRetuneStartedTime + 700u) {
            StartRadio((eRadioID)m_iRadioStationMenuRequest, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, 0);
            m_iRadioStationMenuRequest = RADIO_INVALID;
        }
    } else {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_OFF);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
        StartRadio(RADIO_OFF, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, 0);
        m_iRadioStationMenuRequest = RADIO_INVALID;
    }
}

// 0x4EA640
void CAERadioTrackManager::StartTrackPlayback() {
    AEAudioHardware.SetChannelFlags(m_HwClientHandle, 0, 55);
    AEAudioHardware.StartTrackPlayback();
    UpdateRadioVolumes();
}

// 0x4EA010
void CAERadioTrackManager::UpdateRadioVolumes() {
    plugin::CallMethod<0x4EA010, CAERadioTrackManager*>(this);

    /*
    const auto fEffectsScalingFactor = AEAudioHardware.GetEffectsMasterScalingFactor();
    const auto fMusicScalingFactor = AEAudioHardware.GetMusicMasterScalingFactor();

    auto volume = -4.0f;
    if (!CTimer::GetIsPaused() || !m_bEnabledInPauseMode) {
        if (CTimer::GetIsSlowMotionActive()) {
            volume = -100.0f;
        }
        else if (TheCamera.m_bWideScreenOn) {
            volume = -16.0f;
        }
        else if (fEffectsScalingFactor > 0.0f && fMusicScalingFactor > 0.0f) {
            if (!CAEPedSpeechAudioEntity::s_bForceAudible) {
                auto audioEvent = 0;
                while (true) {
                    if (!AudioEngine.IsMissionAudioSampleFinished(audioEvent) && AudioEngine.GetMissionAudioEvent(audioEvent) != 0xFFFF) {
                        auto missionAudioPosition = AudioEngine.GetMissionAudioPosition(audioEvent);
                        if (!missionAudioPosition)
                            break;

                        CAEAudioEnvironment::GetPositionRelativeToCamera(v9, missionAudioPosition);
                        if (CVector::Magnitude(&v9) <= 15.0f)
                            break;
                    }
                    if (++audioEvent >= 2) {
                        if (m_f80 >= 0.0f)
                            goto LABEL_22;

                        auto v4 = m_f84 + m_f80;
                        if (v4 >= 0.0f)
                            v4 = 0.0f;
                        m_f80 = v4;
                        goto LABEL_21;
                    }
                }
            }

            volumea = fEffectsScalingFactor;
            if (__FYL2X__(volumea / fMusicScalingFactor, 0.30102999566398119802) * 20.0f - 9.0f >= 0.0f) {
                v4 = 0.0f;
            } else {
                volumeb = fEffectsScalingFactor;
                v4 = __FYL2X__(volumeb / fMusicScalingFactor, 0.30102999566398119802) * 20.0f - 9.0f;
            }
            m_f80 = v4;
            m_f84 = -0.02 * v4;
        LABEL_21:
            volume = v4 - 4.0f;
        }
    LABEL_22:
        if (AudioEngine.IsAmbienceRadioActive())
            volume = volume - 20.0f;
    }

    if (m_bBassEnhance && m_ActiveSettings.m_BassSetting) {
        switch (m_ActiveSettings.m_BassSetting) {
        case 1:
            volume -= 2.0f;
            break;
        case 2:
            volume += 1.5f;
            break;
        }
    }

    AEAudioHardware.SetChannelVolume(m_nChannel, 0, volume, 0);
    */
}

// 0x4E8400
void CAERadioTrackManager::PlayRadioAnnouncement(uint32) {
    // NOP
}

// 0x4EB550
void CAERadioTrackManager::StartRadio(const tVehicleAudioSettings& settings) {
    // plugin::CallMethod<0x4EB550, CAERadioTrackManager*, tVehicleAudioSettings*>(this, settings);

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    if (settings.RadioType == AE_RT_EMERGENCY) {
        StartRadio(RADIO_EMERGENCY_AA, settings.BassSetting, settings.BassFactor, 0);
        return;
    }

    if (settings.RadioType != AE_RT_CIVILIAN)
        return;

    const bool needsRetune = [&] {
       if (!m_bRadioAutoSelect)
           return false;

       const auto savedId = m_nSavedRadioStationId;
       if (savedId < 0 || savedId == settings.RadioStation || savedId == RADIO_OFF || savedId == RADIO_EMERGENCY_AA)
           return false;

       if (CTimer::GetTimeInMS() > m_nSavedTimeMs + 60'000)
           return false;

       const auto savedHours = m_nSavedGameClockHours;
       auto savedDays = m_nSavedGameClockDays;
       if (savedHours < 0 || savedDays < 0)
           return false;

       if (savedDays > CClock::GetGameClockDays()) {
           const auto month = CClock::GetGameClockMonth();
           savedDays += CClock::daysInMonth[month == 0 ? 11 : month - 1]; // prev month
       }

       if (CClock::GetGameClockHours() + 24 * savedDays - savedHours > 5)
           return false;

       return true;
    }();

    if (needsRetune) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);
        StartRadio((eRadioID)m_nSavedRadioStationId, settings.BassSetting, settings.BassFactor, 0);
    } else {
        StartRadio(settings.RadioStation, settings.BassSetting, settings.BassFactor, 0);
    }
}

// 0x4EB3C0
void CAERadioTrackManager::StartRadio(eRadioID id, eBassSetting bassSetting, float bassGain, bool skipTrack) {
    id = std::min(id, RADIO_OFF);

    if (CTimer::GetIsPaused()) {
        m_bEnabledInPauseMode = true;

        if (IsRadioOn() && id == m_ActiveSettings.StationID) {
            m_aRadioState[id].m_iTimeInPauseModeInMs = CTimer::GetTimeInMSPauseMode();
            return;
        }
    }

    if (id != RADIO_OFF && CAudioEngine::IsAmbienceTrackActive()) {
        if (!CTimer::GetIsPaused() && CAudioEngine::DoesAmbienceTrackOverrideRadio()) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
            return;
        }
        AudioEngine.StopAmbienceTrack(false);
    }

    auto* const rs = &m_RequestedSettings;
    rs->StationID = id;
    rs->BassSetting             = bassSetting;
    rs->BassGain            = bassGain;
    if (id == RADIO_OFF) {
        rs->Reset();
    } else if (m_aRadioState[id].m_iTimeInMs < 0 || !TrackRadioStation(id, skipTrack)) {
        ChooseTracksForStation(rs->StationID);
        rs->PlayTime = CAEAudioUtility::GetRandomNumberInRange(0, 300'000);
    }
    switch (m_nMode) {
    case eRadioTrackMode::RADIO_STARTING:
    case eRadioTrackMode::RADIO_WAITING_TO_PLAY:
    case eRadioTrackMode::RADIO_PLAYING:
        m_nMode = eRadioTrackMode::RADIO_STOPPING;
    }
    m_aRadioState[rs->StationID].m_iTimeInPauseModeInMs = -1;

    m_bInitialised = true;
}

// 0x4EAC30
bool CAERadioTrackManager::TrackRadioStation(eRadioID id, bool skipTrack) {
    auto& state = m_aRadioState[id];

    if (state.m_nGameClockHours >= 0 && state.m_nGameClockDays >= 0) {
        auto dayDiff = (int32)CClock::GetGameClockDays() - state.m_nGameClockDays;
        if (dayDiff < 0) {
            auto month = CClock::GetGameClockMonth() - 1;
            if (month < 0) {
                month += 12;
            }
            dayDiff += CClock::daysInMonth[month];
        }
        if (dayDiff * 24 - state.m_nGameClockHours + CClock::GetGameClockHours() > 5) {
            return false;
        }
    }

    auto elapsedMs = (int32)CTimer::GetTimeInMS() - state.m_iTimeInMs;
    if (elapsedMs < 7001) {
        elapsedMs = 7000;
    }
    if (skipTrack) {
        const auto minElapsed = state.m_aElapsed[0] + 1;
        if (elapsedMs <= minElapsed) {
            elapsedMs = minElapsed;
        }
    }

    rng::fill(m_RequestedSettings.TrackQueue, -1);
    rng::fill(m_RequestedSettings.TrackTypes, TYPE_NONE);
    rng::fill(m_RequestedSettings.TrackIndices, -1);

    int8 trackCount = 0;
    int32 cumulative = 0;
    for (auto i = 0; i < 3; i++) {
        cumulative += state.m_aElapsed[i];
        if (elapsedMs > cumulative) {
            continue;
        }

        m_RequestedSettings.PlayTime = i == 0
            ? state.m_iTrackPlayTime + elapsedMs
            : (state.m_aElapsed[i] - cumulative) + elapsedMs;

        switch (state.m_aTrackTypes[i]) {
        case TYPE_INDENT:
        case TYPE_ADVERT:
        case TYPE_DJ_BANTER:
            m_RequestedSettings.TrackQueue[0] = state.m_aTrackQueue[i];
            m_RequestedSettings.TrackTypes[0] = state.m_aTrackTypes[i];
            trackCount = 1;
            if (id == RADIO_USER_TRACKS) {
                QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
            } else {
                QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
            }
            return true;
        case TYPE_INTRO:
            for (auto j = 0; j < 3; j++) {
                m_RequestedSettings.TrackQueue[j] = state.m_aTrackQueue[i + j];
                m_RequestedSettings.TrackTypes[j] = state.m_aTrackTypes[i + j];
            }
            return true;
        case TYPE_TRACK:
            for (auto j = 0; j < 2; j++) {
                m_RequestedSettings.TrackQueue[j] = state.m_aTrackQueue[i + j];
                m_RequestedSettings.TrackTypes[j] = state.m_aTrackTypes[i + j];
            }
            return true;
        case TYPE_OUTRO:
            m_RequestedSettings.TrackQueue[0] = state.m_aTrackQueue[i];
            m_RequestedSettings.TrackTypes[0] = state.m_aTrackTypes[i];
            trackCount = 1;
            if (id == RADIO_EMERGENCY_AA) {
                QueueUpTracksForStation(RADIO_EMERGENCY_AA, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
            } else {
                if (CAEAudioUtility::ResolveProbability(0.5f)) {
                    QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
                }
                if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings)) {
                    QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
                }
            }
            return true;
        case TYPE_USER_TRACK:
            for (auto j = 0; j < 2; j++) {
                m_RequestedSettings.TrackQueue[j] = state.m_aTrackQueue[i + j];
                m_RequestedSettings.TrackTypes[j] = state.m_aTrackTypes[i + j];
            }
            if (m_RequestedSettings.TrackQueue[1] == -1) {
                m_RequestedSettings.TrackQueue[1] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_RequestedSettings.TrackTypes[1] = TYPE_USER_TRACK;
                m_RequestedSettings.TrackIndices[1] = (int8)m_RequestedSettings.TrackQueue[1];
            }
            return true;
        default:
            return false;
        }
    }

    // Elapsed real time runs past all 3 saved queue segments - resume with fresh, freshly-queued tracks.
    if (elapsedMs <= cumulative + 7000) {
        if (id == RADIO_USER_TRACKS) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        } else {
            QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        }
        m_RequestedSettings.PlayTime = std::min(elapsedMs - cumulative, 5000);
        return true;
    }

    if (elapsedMs <= cumulative + 155'000) {
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        m_RequestedSettings.PlayTime = (elapsedMs - cumulative) - 5000;
        return true;
    }

    if (elapsedMs > cumulative + 160'000) {
        return false;
    }

    if (id == RADIO_USER_TRACKS) {
        QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_ADVERT, m_RequestedSettings);
        }
    } else {
        QueueUpTracksForStation(id, &trackCount, TYPE_OUTRO, m_RequestedSettings);
        AddMusicTrackIndexToHistory(id, m_RequestedSettings.TrackIndices[trackCount - 1]);

        if (id == RADIO_EMERGENCY_AA) {
            QueueUpTracksForStation(RADIO_EMERGENCY_AA, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        } else if (CAEAudioUtility::ResolveProbability(0.5f)) {
            if (CAEAudioUtility::ResolveProbability(0.5f)) {
                QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
            }
            QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        } else {
            if (CAEAudioUtility::ResolveProbability(0.5f)) {
                QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
            }
            if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings)) {
                QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
            }
        }
    }

    m_RequestedSettings.PlayTime = (elapsedMs - cumulative) - 155'000;
    return true;
}

// 0x4EA670
bool CAERadioTrackManager::QueueUpTracksForStation(eRadioID id, int8* iTrackCount, int8 radioState, tRadioSettings& settings) {
    auto& count = *iTrackCount;

    switch (radioState) {
    case TYPE_INDENT:
        if (id != RADIO_USER_TRACKS) {
            const auto identId = ChooseIdentIndex(id);
            settings.TrackQueue[count] = identId;
            if (identId != -1) {
                settings.TrackTypes[count] = TYPE_INDENT;
                count++;
                return true;
            }
        }
        break;
    case TYPE_ADVERT: {
        settings.TrackQueue[count] = ChooseAdvertIndex(id);
        settings.TrackTypes[count] = TYPE_ADVERT;
        count++;
        return true;
    }
    case TYPE_DJ_BANTER: {
        if (id != RADIO_USER_TRACKS) {
            const auto banterId = ChooseDJBanterIndex(id);
            settings.TrackQueue[count] = banterId;
            if (banterId != -1) {
                settings.TrackTypes[count] = TYPE_DJ_BANTER;
                count++;
                return true;
            }
        }
        break;
    }
    case TYPE_INTRO: {
        if (id != RADIO_USER_TRACKS) {
            const auto trackIdx = ChooseMusicTrackIndex(id);
            settings.TrackIndices[count] = trackIdx;
            settings.TrackQueue[count] = CAEAudioUtility::GetRandomNumberInRange(gRadioMusicIntros[id][trackIdx][0], gRadioMusicIntros[id][trackIdx][1]);
            settings.TrackTypes[count] = TYPE_INTRO;
            count++;

            settings.TrackIndices[count] = settings.TrackIndices[count - 1];
            settings.TrackQueue[count] = gRadioMusicTracks[id][settings.TrackIndices[count]];
            settings.TrackTypes[count] = TYPE_TRACK;
            count++;

            settings.TrackIndices[count] = settings.TrackIndices[count - 1];
            settings.TrackQueue[count] = CAEAudioUtility::GetRandomNumberInRange(gRadioMusicOutros[id][settings.TrackIndices[count]][0], gRadioMusicOutros[id][settings.TrackIndices[count]][1]);
            settings.TrackTypes[count] = TYPE_OUTRO;
            count++;
            return true;
        }
        break;
    }
    case TYPE_TRACK: {
        if (id == RADIO_USER_TRACKS) {
            const auto userTrackId = AEUserRadioTrackManager.SelectUserTrackIndex();
            settings.TrackQueue[count] = userTrackId;
            settings.TrackTypes[count] = TYPE_USER_TRACK;
            settings.TrackIndices[count] = static_cast<int8>(userTrackId);
            count++;
            return true;
        }

        const auto trackIdx = ChooseMusicTrackIndex(id);
        settings.TrackIndices[count] = trackIdx;
        settings.TrackQueue[count] = gRadioMusicTracks[id][trackIdx];
        settings.TrackTypes[count] = TYPE_TRACK;
        count++;

        settings.TrackIndices[count] = settings.TrackIndices[count - 1];
        settings.TrackQueue[count] = CAEAudioUtility::GetRandomNumberInRange(gRadioMusicOutros[id][settings.TrackIndices[count]][0], gRadioMusicOutros[id][settings.TrackIndices[count]][1]);
        settings.TrackTypes[count] = TYPE_OUTRO;
        count++;
        return true;
    }
    case TYPE_OUTRO: {
        if (id != RADIO_USER_TRACKS) {
            const auto trackIdx = ChooseMusicTrackIndex(id);
            settings.TrackIndices[count] = trackIdx;
            settings.TrackQueue[count] = CAEAudioUtility::GetRandomNumberInRange(gRadioMusicOutros[id][trackIdx][0], gRadioMusicOutros[id][trackIdx][1]);
            settings.TrackTypes[count] = TYPE_OUTRO;
            count++;
            return true;
        }
        break;
    }
    default:
        return true;
    }
    return false;
}

// 0x4E9820
void CAERadioTrackManager::StopRadio(tVehicleAudioSettings* settings, bool duringPause) {
    switch (m_nMode) {
    case eRadioTrackMode::RADIO_STARTING:
    case eRadioTrackMode::RADIO_WAITING_TO_PLAY:
    case eRadioTrackMode::RADIO_PLAYING: {
        if (!CTimer::GetIsPaused() || duringPause) {
            m_nMode = eRadioTrackMode::RADIO_STOPPING;
        }

        const auto id = m_ActiveSettings.StationID;
        auto& state = m_aRadioState[id];

        rng::fill(state.m_aElapsed, 0);
        state.m_iTrackPlayTime = -1;
        rng::fill(state.m_aTrackQueue, -1);
        rng::fill(state.m_aTrackTypes, TYPE_NONE);
        state.m_iTimeInMs = CTimer::GetTimeInMS();
        state.m_nGameClockDays = CClock::ms_nGameClockDays;
        state.m_nGameClockHours = CClock::ms_nGameClockHours;

        if (state.m_iTimeInPauseModeInMs >= 0 && id != RADIO_EMERGENCY_AA && id != RADIO_OFF) {
            m_aListenTimes[id] += CTimer::GetTimeInMSPauseMode() - state.m_iTimeInPauseModeInMs;
        }

        if (id == RADIO_OFF) {
            m_aRadioState[RADIO_OFF].m_aElapsed[0] = 0;
            break;
        }

        state.m_aElapsed[0] = (m_ActiveSettings.TrackLengthMs - m_ActiveSettings.PlayTime) - 100;

        switch (m_ActiveSettings.CurrTrackType) {
        case TYPE_INDENT:
        case TYPE_ADVERT:
        case TYPE_DJ_BANTER:
        case TYPE_OUTRO:
            state.m_iTrackPlayTime = m_ActiveSettings.PlayTime;
            state.m_aTrackQueue[0] = m_ActiveSettings.CurrTrackID;
            state.m_aTrackTypes[0] = m_ActiveSettings.CurrTrackType;
            break;
        case TYPE_INTRO:
            state.m_aElapsed[1] = id == RADIO_TALK ? gRadioTalkShowDurationsMs[m_ActiveSettings.TrackIndices[0]] : 150'000;
            state.m_aElapsed[2] = 5000;
            state.m_iTrackPlayTime = m_ActiveSettings.PlayTime;
            if (m_ActiveSettings.TrackQueue[0] == m_ActiveSettings.CurrTrackID) {
                state.m_aTrackQueue[0] = m_ActiveSettings.TrackQueue[0];
                state.m_aTrackTypes[0] = m_ActiveSettings.TrackTypes[0];
                state.m_aTrackQueue[1] = m_ActiveSettings.TrackQueue[1];
                state.m_aTrackTypes[1] = m_ActiveSettings.TrackTypes[1];
                state.m_aTrackQueue[2] = m_ActiveSettings.TrackQueue[2];
                state.m_aTrackTypes[2] = m_ActiveSettings.TrackTypes[2];
            } else {
                state.m_aTrackQueue[0] = m_ActiveSettings.CurrTrackID;
                state.m_aTrackTypes[0] = m_ActiveSettings.CurrTrackType;
                state.m_aTrackQueue[1] = m_ActiveSettings.TrackQueue[0];
                state.m_aTrackTypes[1] = m_ActiveSettings.TrackTypes[0];
                state.m_aTrackQueue[2] = m_ActiveSettings.TrackQueue[1];
                state.m_aTrackTypes[2] = m_ActiveSettings.TrackTypes[1];
            }
            break;
        case TYPE_TRACK:
        case TYPE_USER_TRACK:
            state.m_aElapsed[1] = 5000;
            state.m_iTrackPlayTime = m_ActiveSettings.PlayTime;
            if (m_ActiveSettings.TrackQueue[0] == m_ActiveSettings.CurrTrackID) {
                state.m_aTrackQueue[0] = m_ActiveSettings.TrackQueue[0];
                state.m_aTrackTypes[0] = m_ActiveSettings.TrackTypes[0];
                state.m_aTrackQueue[1] = m_ActiveSettings.TrackQueue[1];
                state.m_aTrackTypes[1] = m_ActiveSettings.TrackTypes[1];
            } else {
                state.m_aTrackQueue[0] = m_ActiveSettings.CurrTrackID;
                state.m_aTrackTypes[0] = m_ActiveSettings.CurrTrackType;
                state.m_aTrackQueue[1] = m_ActiveSettings.TrackQueue[0];
                state.m_aTrackTypes[1] = m_ActiveSettings.TrackTypes[0];
            }
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    m_bInitialised = false;
    m_bEnabledInPauseMode = false;

    if (CTimer::GetIsPaused() && !duringPause) {
        m_iRadioStationMenuRequest = -1;
        m_nRetuneStartedTime = 0;
    }

    if (settings) {
        m_nStationsListed = 0;
        m_nStationsListDown = 0;
        m_iRadioStationScriptRequest = -1;
        m_bDisplayStationName = false;
        m_bRetuneJustStarted = false;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);

        if (m_ActiveSettings.StationID == RADIO_INVALID) {
            m_ActiveSettings.StationID = RADIO_OFF;
        }

        settings->RadioStation = m_ActiveSettings.StationID;
        settings->BassSetting = m_ActiveSettings.BassSetting;

        if (m_nMode != eRadioTrackMode::RADIO_STOPPED) {
            m_nSavedTimeMs = CTimer::GetTimeInMS();
            m_nSavedGameClockDays = CClock::ms_nGameClockDays;
            m_nSavedGameClockHours = CClock::ms_nGameClockHours;
            m_nSavedRadioStationId = m_ActiveSettings.StationID;
        }

        if (m_ActiveSettings.StationID == RADIO_EMERGENCY_AA) {
            m_ActiveSettings.StationID = static_cast<eRadioID>(CAEAudioUtility::GetRandomNumberInRange(1, 13));
        }
    } else if (duringPause) {
        m_nStationsListed = 0;
        m_nStationsListDown = 0;
        m_bRetuneJustStarted = false;
    }
}

// 0x4E94C0
int32 CAERadioTrackManager::ChooseIdentIndex(eRadioID id) {
    if (gRadioIdents[id][0] == NOTRACK) {
        return -1;
    }

    for (;;) {
        const auto identId = CAEAudioUtility::GetRandomNumberInRange(gRadioIdents[id][0], gRadioIdents[id][1]);

        // NOTSA: the first Radio Los Santos ident references the "Are You Going To San Fierro?"
        // mission - don't play it until that mission's been completed.
        if (id == RADIO_MODERN_HIP_HOP && identId == gRadioIdents[RADIO_MODERN_HIP_HOP][0]) {
            if (CStats::GetStatValue(STAT_ARE_YOU_GOING_TO_SAN_FIERRO_MISSION_ACCOMPLISHED) == 0.0f) {
                continue;
            }
        }

        const auto historyLimit = std::min<int32>(IDENT_INDEX_HISTORY_COUNT, (gRadioIdents[id][1] - gRadioIdents[id][0]) - 1);
        bool alreadyPlayed = false;
        for (auto i = 0; i < historyLimit; i++) {
            if (identId == m_nIdentIndexHistory[id].indices[i]) {
                alreadyPlayed = true;
                break;
            }
        }
        if (!alreadyPlayed) {
            return identId;
        }
    }
}

// 0x4E9570
int32 CAERadioTrackManager::ChooseAdvertIndex(eRadioID id) {
    for (;;) {
        const auto advertId = CAEAudioUtility::GetRandomNumberInRange(gRadioAdverts[0], gRadioAdverts[1]);

        bool excluded = false;
        for (const auto restricted : gnRadioStationRestrictedAdverts[id]) {
            if (advertId == restricted) {
                excluded = true;
                break;
            }
        }
        if (excluded) {
            continue;
        }

        bool alreadyPlayed = false;
        for (const auto historyId : m_nAdvertIndexHistory[id].indices) {
            if (advertId == historyId) {
                alreadyPlayed = true;
                break;
            }
        }
        if (!alreadyPlayed) {
            return advertId;
        }
    }
}

// 0x4EA270
int8 CAERadioTrackManager::ChooseMusicTrackIndex(eRadioID id) {
    if (id == RADIO_TALK) {
        return ChooseTalkRadioShow();
    }

    for (;;) {
        const auto trackIdx = CAEAudioUtility::GetRandomNumberInRange<int32>(0, gRadioNumMusicTracksPerStation[id] - 1);

        const auto historyLimit = std::min<int32>(MUSIC_TRACK_HISTORY_COUNT, gRadioNumMusicTracksPerStation[id] - 2);
        bool alreadyPlayed = false;
        for (auto i = 0; i < historyLimit; i++) {
            if (trackIdx == m_nMusicTrackIndexHistory[id].indices[i]) {
                alreadyPlayed = true;
                break;
            }
        }
        if (!alreadyPlayed) {
            return static_cast<int8>(trackIdx);
        }
    }
}

// 0x4EA2D0
int32 CAERadioTrackManager::ChooseDJBanterIndex(eRadioID id) {
    const auto TryCandidate = [&](int32 candidate) -> int32 {
        if (candidate == NOTRACK) {
            return -1;
        }
        if (candidate >= 0) {
            for (const auto historyId : m_nDJBanterIndexHistory[id].indices) {
                if (candidate == historyId) {
                    return -1;
                }
            }
        }
        return candidate;
    };

    if (m_nSpecialDJBanterPending == 0) {
        if (const auto result = TryCandidate(gRadioDJBanterBC[id][0]); result != -1) {
            return result;
        }
    } else if (m_nSpecialDJBanterPending == 1) {
        if (m_nSpecialDJBanterIndex == 0 ||
            (m_nSpecialDJBanterIndex == 1 && gRadioDJBanterSpecialCity[id][0] != gRadioDJBanterSpecialCity[id][1]))
        {
            if (const auto result = TryCandidate(gRadioDJBanterSpecialCity[id][m_nSpecialDJBanterIndex]); result != -1) {
                return result;
            }
        }
    } else if (m_nSpecialDJBanterPending == 2) {
        if (const auto result = TryCandidate(gRadioDJBanterSpecialMission[id][m_nSpecialDJBanterIndex]); result != -1) {
            return result;
        }
    }

    if (id == RADIO_EMERGENCY_AA) {
        return CGameLogic::LaRiotsActiveHere()
            ? ChooseDJBanterIndexFromList(RADIO_EMERGENCY_AA, gRadioDJBanterTN)
            : ChooseDJBanterIndexFromList(RADIO_EMERGENCY_AA, gRadioDJBanterGN);
    }

    if (!CAEAudioUtility::ResolveProbability(0.6f) || CGame::currArea != AREA_CODE_NORMAL_WORLD) {
        return -1;
    }

    const auto hour = CClock::GetGameClockHours();
    if (!CWeather::ForecastWeather(WEATHER_RAINY_COUNTRYSIDE, 3) && !CWeather::ForecastWeather(WEATHER_RAINY_SF, 3)) {
        if (CWeather::ForecastWeather(WEATHER_FOGGY_SF, 3) && CAEAudioUtility::ResolveProbability(0.5f)) {
            if (const auto result = ChooseDJBanterIndexFromList(id, gRadioDJBanterFoggy); result != -1) {
                return result;
            }
        }
    } else if (CAEAudioUtility::ResolveProbability(0.5f)) {
        if (const auto result = ChooseDJBanterIndexFromList(id, gRadioDJBanterRainy); result != -1) {
            return result;
        }
    }

    if (hour >= 6 && hour < 9) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto result = ChooseDJBanterIndexFromList(id, gRadioDJBanterMorning); result != -1) {
                return result;
            }
        }
    } else if (hour >= 18 && hour < 21) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto result = ChooseDJBanterIndexFromList(id, gRadioDJBanterEvening); result != -1) {
                return result;
            }
        }
    } else if (hour >= 22 || hour < 3) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto result = ChooseDJBanterIndexFromList(id, gRadioDJBanterTN); result != -1) {
                return result;
            }
        }
    }

    return ChooseDJBanterIndexFromList(id, gRadioDJBanterGN);
}

// 0x4E95E0
int32 CAERadioTrackManager::ChooseDJBanterIndexFromList(eRadioID id, const int32 (*list)[2]) {
    if (list[id][0] == NOTRACK) {
        return -1;
    }

    const auto rangeSize = list[id][1] - list[id][0] + 1;
    if (rangeSize < 1) {
        return -1;
    }

    const auto randomOffset = CAEAudioUtility::GetRandomNumberInRange(0, list[id][1] - list[id][0]);
    // NOTSA: the history cap is computed from `gRadioDJBanterGN` specifically, regardless of which
    // `list` was actually passed in - matches the original disassembly exactly (not a mistake).
    const auto historyLimit = std::min<int32>(DJBANTER_INDEX_HISTORY_COUNT, (gRadioDJBanterGN[id][1] - gRadioDJBanterGN[id][0]) - 1);

    for (auto counter = 0; counter < rangeSize; counter++) {
        const auto candidate = (counter + randomOffset) % rangeSize + list[id][0];

        bool alreadyPlayed = false;
        for (auto i = 0; i < historyLimit; i++) {
            if (candidate == m_nDJBanterIndexHistory[id].indices[i]) {
                alreadyPlayed = true;
                break;
            }
        }
        if (!alreadyPlayed) {
            return candidate;
        }
    }
    return -1;
}

// 0x4EB180
void CAERadioTrackManager::ChooseTracksForStation(eRadioID id) {
    int8 trackCount = 0;

    for (auto i = 0u; i < tRadioSettings::NUM_TRACKS; i++) {
        m_RequestedSettings.TrackTypes[i] = TYPE_NONE;
        m_RequestedSettings.TrackQueue[i] = -1;
        m_RequestedSettings.TrackIndices[i] = -1;
    }

    if (!CAEAudioUtility::ResolveProbability(0.95f)) {
        if (id) {
            if (CAEAudioUtility::ResolveProbability(0.5f))
                QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);

            if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings))
                QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);

            if (id == RADIO_USER_TRACKS) {
                QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
                return;
            }
        } else {
            QueueUpTracksForStation(RADIO_EMERGENCY_AA, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    if (id == RADIO_USER_TRACKS) {
        QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_ADVERT, m_RequestedSettings);
        }
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.9f)) {
        QueueUpTracksForStation(id, &trackCount, TYPE_TRACK, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    QueueUpTracksForStation(id, &trackCount, TYPE_OUTRO, m_RequestedSettings);
    AddMusicTrackIndexToHistory(id, m_RequestedSettings.TrackIndices[trackCount - 1]);

    if (id == RADIO_EMERGENCY_AA) {
        QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f))
        QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);

    if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings))
        QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
}

// 0x4E8E40
int8 CAERadioTrackManager::ChooseTalkRadioShow() {
    const auto Passed = [](eStats stat) { return CStats::GetStatValue(stat) != 0.0f; };

    std::array<int8, 31> candidates;
    rng::fill(candidates, -1);
    int8 count = 0;

    // Story-progress "most advanced heist" candidate.
    if (Passed(STAT_RYDERS_MISSION_ROBBING_UNCLE_SAM_ACCOMPLISHED) && !Passed(STAT_MIKE_TORENO_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 14;
    } else if (Passed(STAT_ARCHITECTURAL_ESPIONAGE_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 15;
    }

    if (!Passed(STAT_JIZZY_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 12;
    } else if (!Passed(STAT_ARCHITECTURAL_ESPIONAGE_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 13;
    }

    if (Passed(STAT_SMALL_TOWN_BANK_MISSION_ACCOMPLISHED) && !Passed(STAT_PHOTO_OPPORTUNITY_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 6;
    }

    if (Passed(STAT_DRIVE_THRU_MISSION_ACCOMPLISHED) && Passed(STAT_REUNITING_THE_FAMILIES_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 3;
    } else if (Passed(STAT_PHOTO_OPPORTUNITY_MISSION_ACCOMPLISHED) && !Passed(STAT_DON_PEYOTE_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 4;
    } else if (Passed(STAT_DON_PEYOTE_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 5;
    }

    if (!Passed(STAT_LOCAL_LIQUOR_STORE_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 7;
    } else {
        candidates[count++] = 8;
    }

    if (!Passed(STAT_BADLANDS_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 9;
    } else if (!Passed(STAT_555_WE_TIP_MISSION_ACCOMPLISHED) && !Passed(STAT_PLAYING_TIME)) {
        candidates[count++] = 10;
    } else if (Passed(STAT_PLAYING_TIME)) {
        candidates[count++] = 11;
    }

    if (!Passed(STAT_HIDDEN_PACKAGES_FOUND)) {
        candidates[count++] = 27;
    } else {
        candidates[count++] = 28;
    }

    if (!Passed(STAT_TAGS_SPRAYED)) {
        candidates[count++] = 29;
    } else {
        candidates[count++] = 30;
    }

    if (!Passed(STAT_LEAST_FAVORITE_GANG)) {
        candidates[count++] = 0;
    } else if (Passed(STAT_GANG_MEMBERS_WASTED) && !Passed(STAT_CRIMINALS_WASTED)) {
        candidates[count++] = 1;
    } else if (Passed(STAT_MOST_FAVORITE_RADIO_STATION)) {
        candidates[count++] = 2;
    }

    if (!Passed(STAT_DRIVE_THRU_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 16;
    } else if (!Passed(STAT_MANAGEMENT_ISSUES_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 17;
    } else if (!Passed(STAT_LEAST_FAVORITE_GANG)) {
        candidates[count++] = 18;
    } else if (!Passed(STAT_555_WE_TIP_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 19;
    } else if (!Passed(STAT_YAY_KA_BOOM_BOOM_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 20;
    } else if (!Passed(STAT_FISH_IN_A_BARREL_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 21;
    } else if (!Passed(STAT_BREAKING_THE_BANK_AT_CALIGULAS_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 22;
    } else if (!Passed(STAT_A_HOME_IN_THE_HILLS_MISSION_ACCOMPLISHED)) {
        candidates[count++] = 23;
    } else if (!Passed(STAT_MAYBE_SET_RIOT_MODE)) {
        candidates[count++] = 24;
    } else if (CStats::GetStatValue(STAT_CITY_UNLOCKED) != 4.0f) { // 4.0f = every area unlocked
        candidates[count++] = 25;
    } else {
        candidates[count++] = 26;
    }

    const auto pick = candidates[CAEAudioUtility::GetRandomNumberInRange(0, count - 1)];
    if (count <= 1) {
        return pick;
    }

    // NOTSA: `StaticRef<int8>(0xB62C1C)` is a static, zero-initialized, otherwise-unreferenced byte
    // in the original binary - reading it directly here reproduces the original comparison exactly
    // without guessing at its intended meaning.
    if (pick != StaticRef<int8>(0xB62C1C)) {
        return pick;
    }
    for (;;) {
        const auto candidate = candidates[CAEAudioUtility::GetRandomNumberInRange(0, count - 1)];
        if (candidate != StaticRef<int8>(0xB62C1C)) {
            return candidate;
        }
    }
}

// 0x4E96C0
void CAERadioTrackManager::AddMusicTrackIndexToHistory(eRadioID id, int8 trackIndex) {
    if (trackIndex >= 0 && m_nMusicTrackIndexHistory[id].indices[0] != trackIndex) {
        m_nMusicTrackIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id]++;
    }
}

// 0x4E9720
void CAERadioTrackManager::AddIdentIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nIdentIndexHistory[id].indices[0] != trackIndex)
        m_nIdentIndexHistory[id].PutAtFirst(trackIndex);
}

// 0x4E9760
void CAERadioTrackManager::AddAdvertIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nAdvertIndexHistory[id].indices[0] != trackIndex) {
        m_nAdvertIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id] = 0;
    }
}

// 0x4E97B0
void CAERadioTrackManager::AddDJBanterIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nDJBanterIndexHistory[id].indices[0] != trackIndex) {
        m_nDJBanterIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id] = 0;
    }
}

// 0x4EA590
void CAERadioTrackManager::CheckForPause() {
    if (CTimer::GetIsPaused()) {
        m_bPauseMode = true;
        AEAudioHardware.SetChannelFrequencyScalingFactor(m_HwClientHandle, 0, m_bEnabledInPauseMode ? 1.0f : 0.0f);
    } else {
        const auto* settings = CAEVehicleAudioEntity::StaticGetPlayerVehicleAudioSettingsForRadio();
        
        if (settings && notsa::contains({
                AE_RT_CIVILIAN,
                AE_RT_EMERGENCY,
                AE_RT_UNKNOWN
            }, settings->RadioType)
            || CAudioEngine::IsAmbienceRadioActive()
        ) {
            m_bPauseMode = false;
            AEAudioHardware.SetChannelFrequencyScalingFactor(m_HwClientHandle, 0, 1.0f);
        } else {
            StopRadio(nullptr, false);
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
            m_bPauseMode = false;
        }
    }
}

// 0x4EB9A0
void CAERadioTrackManager::Service(int32 playTime) {
    // Debug-only snapshot of the previous PlayTime - never read back anywhere in the codebase.
    StaticRef<int32>(0x8CBA68) = m_ActiveSettings.PlayTime;

    m_ActiveSettings.PlayTime = playTime;
    m_ActiveSettings.TrackLengthMs = AEAudioHardware.GetTrackLengthMs();
    m_ActiveSettings.CurrTrackID = AEAudioHardware.GetPlayingTrackID();

    if (!CTimer::GetIsPaused()) {
        CheckForMissionStatsChanges();
        CheckForStationRetune();
    } else {
        CheckForStationRetuneDuringPause();
    }

    if (m_bInitialised && m_nMode == eRadioTrackMode::RADIO_STOPPED) {
        bool skipReset = false;
        if (m_RequestedSettings.StationID == RADIO_OFF) {
            m_ActiveSettings = m_RequestedSettings;

            // Opaque, obfuscated pointer-decrypt helper (raw-called; not worth hand-decoding).
            if (const auto ptr = plugin::CallAndReturn<int32, 0x4F4ED0>()) {
                const auto b = *reinterpret_cast<int8*>(ptr + 0x1b);
                if (b == 0 || b == 3 || b == 2) {
                    m_bDisplayStationName = true;
                }
            }
        } else if (AudioEngine.IsAmbienceTrackActive()) {
            skipReset = true;
        } else {
            m_ActiveSettings = m_RequestedSettings;
            m_nMode = eRadioTrackMode::RADIO_STARTING;
            if (CPad::GetPad()->IsRadioTrackSkipPressed()) {
                m_bDisplayStationName = true;
            }
        }
        if (!skipReset) {
            m_bInitialised = false;
        }
    }

    switch (m_nMode) {
    case eRadioTrackMode::RADIO_STARTING: {
        if (m_ActiveSettings.PlayTime < 0) {
            m_ActiveSettings.PlayTime = 0;
        }
        AEAudioHardware.SetBassSetting(m_bBassEnhance ? m_ActiveSettings.BassSetting : eBassSetting::NORMAL, m_ActiveSettings.BassGain);
        AEAudioHardware.PlayTrack(
            m_ActiveSettings.TrackQueue[0],
            m_ActiveSettings.TrackQueue[1],
            m_ActiveSettings.PlayTime,
            m_ActiveSettings.TrackFlags,
            m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK,
            m_ActiveSettings.TrackTypes[1] == TYPE_USER_TRACK
        );
        m_nMode = eRadioTrackMode::RADIO_WAITING_TO_PLAY;
        return;
    }
    case eRadioTrackMode::RADIO_WAITING_TO_PLAY:
        if (m_ActiveSettings.PlayTime == -2) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
            StartTrackPlayback();
            field_60 = CTimer::GetTimeInMS();
            m_aRadioState[m_ActiveSettings.StationID].m_iTimeInPauseModeInMs = CTimer::GetTimeInMSPauseMode();
            m_nMode = eRadioTrackMode::RADIO_PLAYING;
            return;
        }
        if (m_ActiveSettings.PlayTime == -8) {
            if (m_ActiveSettings.CurrTrackID == m_ActiveSettings.TrackQueue[1] ||
                (m_ActiveSettings.CurrTrackID == m_ActiveSettings.TrackQueue[0] && m_ActiveSettings.TrackQueue[1] == -1))
            {
                m_ActiveSettings.TrackQueue[0] = ChooseAdvertIndex(RADIO_USER_TRACKS);
                m_ActiveSettings.TrackTypes[0] = TYPE_ADVERT;
                m_ActiveSettings.TrackQueue[1] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[1] = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[1] = static_cast<int8>(m_ActiveSettings.TrackQueue[1]);
                m_nMode = eRadioTrackMode::RADIO_STARTING;
                return;
            }
        }
        break;
    case eRadioTrackMode::RADIO_PLAYING: {
        if (m_ActiveSettings.StationID == RADIO_USER_TRACKS && m_ActiveSettings.PlayTime == -6) {
            if (AEAudioHardware.GetActiveTrackID() == m_ActiveSettings.TrackQueue[0] && m_ActiveSettings.TrackQueue[1] != -1) {
                m_ActiveSettings.TrackQueue[0] = m_ActiveSettings.TrackQueue[1];
                m_ActiveSettings.TrackTypes[0] = m_ActiveSettings.TrackTypes[1];
                m_ActiveSettings.TrackIndices[0] = m_ActiveSettings.TrackIndices[1];
                m_ActiveSettings.TrackQueue[1] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[1] = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[1] = static_cast<int8>(m_ActiveSettings.TrackQueue[1]);
            } else {
                m_ActiveSettings.TrackQueue[0] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[0] = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[0] = static_cast<int8>(m_ActiveSettings.TrackQueue[0]);
                m_ActiveSettings.TrackQueue[1] = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[1] = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[1] = static_cast<int8>(m_ActiveSettings.TrackQueue[1]);
            }
            m_nMode = eRadioTrackMode::RADIO_STARTING;
        }

        const auto activeId = m_ActiveSettings.CurrTrackID;
        const auto RecordHistory = [&](int8 type, int8 index) {
            switch (type) {
            case TYPE_INTRO:
            case TYPE_TRACK:
            case TYPE_OUTRO:
            case TYPE_USER_TRACK:
                AddMusicTrackIndexToHistory(m_ActiveSettings.StationID, index);
                break;
            case TYPE_INDENT:
                AddIdentIndexToHistory(m_ActiveSettings.StationID, activeId);
                break;
            case TYPE_ADVERT:
                AddAdvertIndexToHistory(m_ActiveSettings.StationID, activeId);
                break;
            case TYPE_DJ_BANTER:
                AddDJBanterIndexToHistory(m_ActiveSettings.StationID, activeId);
                break;
            }
        };

        if (activeId == m_ActiveSettings.TrackQueue[0]) {
            RecordHistory(m_ActiveSettings.TrackTypes[0], m_ActiveSettings.TrackIndices[0]);
            m_ActiveSettings.CurrTrackType = m_ActiveSettings.TrackTypes[0];
            m_ActiveSettings.CurrTrackIdx = m_ActiveSettings.TrackIndices[0];
        } else if (activeId == m_ActiveSettings.PrevTrackID) {
            RecordHistory(m_ActiveSettings.PrevTrackType, m_ActiveSettings.PrevTrackIdx);
            m_ActiveSettings.CurrTrackType = m_ActiveSettings.PrevTrackType;
            m_ActiveSettings.CurrTrackIdx = m_ActiveSettings.PrevTrackIdx;
        }

        if (m_ActiveSettings.StationID == RADIO_USER_TRACKS &&
            (m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK || !AEUserRadioTrackManager.GetUserTrackPlayMode()))
        {
            if (CPad::GetPad()->IsRadioTrackSkipPressed()) {
                StopRadio(nullptr, true);
                while (m_nMode != eRadioTrackMode::RADIO_STOPPED || m_bInitialised || m_nStationsListed != 0 || m_nStationsListDown != 0) {
                    Service(AEAudioHardware.GetTrackPlayTime());
                    AEAudioHardware.Service();
                }
                StartRadio(m_ActiveSettings.StationID, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, true);
            }
        }

        CheckForPause();
        UpdateRadioVolumes();
        CheckForTrackConcatenation();
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
        return;
    }
    case eRadioTrackMode::RADIO_STOPPING:
    case eRadioTrackMode::RADIO_STOPPING_CHANNELS_STOPPED:
        AEAudioHardware.StopTrack();
        m_nMode = eRadioTrackMode::RADIO_WAITING_TO_STOP;
        return;
    case eRadioTrackMode::RADIO_STOPPING_SILENCED:
        m_nMode = eRadioTrackMode::RADIO_STOPPING_CHANNELS_STOPPED;
        return;
    case eRadioTrackMode::RADIO_WAITING_TO_STOP:
        if (m_ActiveSettings.PlayTime == -6 || m_ActiveSettings.PlayTime == -8) {
            m_nMode = eRadioTrackMode::RADIO_STOPPED;
        } else if (m_ActiveSettings.PlayTime == -7 || m_ActiveSettings.PlayTime == -2) {
            AEAudioHardware.StopTrack();
        }
        break;
    default:
        break;
    }
}

// 0x5D40E0
void CAERadioTrackManager::Load() {
    for (auto r = 0; r < RADIO_COUNT; r++) {
        for (auto& historyIndex : m_nMusicTrackIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(historyIndex);
        }

        for (auto& identIndex : m_nIdentIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(identIndex);
        }

        for (auto& advertIndex : m_nAdvertIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(advertIndex);
        }

        for (auto& banterIndex : m_nDJBanterIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(banterIndex);
        }
    }

    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsCitiesPassed);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino6);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino10);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCat1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert5);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert8);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert10);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedFarlie3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedLAFin2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedMansion2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedRyder2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedRiot1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedSCrash1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedStrap4);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedSweet2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedTruth2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedVCrash2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedBadlands);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedCat2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedCrash1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitGameClockDays);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitGameClockHours);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitTimeOutHours);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nSpecialDJBanterPending);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nSpecialDJBanterIndex);
}

// 0x5D3EE0
void CAERadioTrackManager::Save() {
    for (auto r = 0; r < RADIO_COUNT; r++) {
        for (auto& historyIndex : m_nMusicTrackIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(historyIndex);
        }

        for (auto& identIndex : m_nIdentIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(identIndex);
        }

        for (auto& advertIndex : m_nAdvertIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(advertIndex);
        }

        for (auto& banterIndex : m_nDJBanterIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(banterIndex);
        }
    }

    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsCitiesPassed);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino6);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino10);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCat1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert5);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert8);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert10);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedFarlie3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedLAFin2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedMansion2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedRyder2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedRiot1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedSCrash1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedStrap4);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedSweet2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedTruth2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedVCrash2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedBadlands);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedCat2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedCrash1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitGameClockDays);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitGameClockHours);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitTimeOutHours);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nSpecialDJBanterPending);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nSpecialDJBanterIndex);
}
