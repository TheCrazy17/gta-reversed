#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#include "3dMarkers.h"
#include "AudioEngine.h"
#include "Clock.h"
#include "Credits.h"
#include "CutsceneMgr.h"
#include "Darkel.h"
#include "EntryExitManager.h"
#include "FireManager.h"
#include "GameLogic.h"
#include "GangWars.h"
#include "Garages.h"
#include "Hud.h"
#include "Localisation.h"
#include "MenuManager.h"
#include "PathFind.h"
#include "Population.h"
#include "PostEffects.h"
#include "Restart.h"
#include "RoadBlocks.h"
#include "Shopping.h"
#include "Stats.h"
#include "Streaming.h"
#include "TagManager.h"
#include "Train.h"
#include "VehicleRecording.h"
#include "Wanted.h"
#include "Weather.h"
#include "World.h"

/*!
* Small commands that only call into the game or flip a flag. Those that store a flag keep the exe's
* sense: some of the stored booleans are the negation of the command's argument.
*/

namespace {
// STORE_CLOTHES_STATE(0793)
void StoreClothesState() { CShopping::StoreClothesState(); }

// RESTORE_CLOTHES_STATE(0794)
void RestoreClothesState() { CShopping::RestoreClothesState(); }

// CLEAR_LOADED_SHOP(087C)
void ClearLoadedShop() { CShopping::RemoveLoadedShop(); }

// STORE_CAR_MOD_STATE(096B)
void StoreCarModState() { CShopping::StoreVehicleMods(); }

// RESTORE_CAR_MOD_STATE(096C)
void RestoreCarModState() { CShopping::RestoreVehicleMods(); }

// GET_NUMBER_OF_ITEMS_IN_SHOP(075F)
int32 GetNumberOfItemsInShop() { return CShopping::GetNumberOfItemsInShop(); }

// GET_ITEM_IN_SHOP(0760)
int32 GetItemInShop(int32 index) { return (int32)CShopping::GetItemInShop(index); }

// FORCE_DEATH_RESTART(0970)
void ForceDeathRestart() { CGameLogic::ForceDeathRestart(); }

// RESET_STUFF_UPON_RESURRECTION(0974)
void ResetStuffUponResurrection() { CGameLogic::ResetStuffUponResurrection(); }

// CLEAR_SKIP(0951)
void ClearSkip() { CGameLogic::ClearSkip(false); }

// IS_SKIP_WAITING_FOR_SCRIPT_TO_FADE_IN(0A36)
bool IsSkipWaitingForScriptToFadeIn() { return CGameLogic::IsSkipWaitingForScriptToFadeIn(); }

// SWITCH_ARREST_PENALTIES(08DE)
void SwitchArrestPenalties(bool enabled) { CGameLogic::bPenaltyForArrestApplies = enabled; }

// FAIL_KILL_FRENZY(09C2)
void FailKillFrenzy() { CDarkel::FailKillFrenzy(); }

// GET_TOTAL_NUMBER_OF_PEDS_KILLED_BY_PLAYER(0806)
int32 GetTotalNumberOfPedsKilledByPlayer(int32 playerId) { return CDarkel::FindTotalPedsKilledByPlayer(playerId); }

// MANAGE_ALL_POPULATION(0A13)
void ManageAllPopulation() { CPopulation::ManageAllPopulation(); }

// SET_CREATE_RANDOM_GANG_MEMBERS(08EA)
void SetCreateRandomGangMembers(bool create) { CPopulation::m_bDontCreateRandomGangMembers = !create; }

// SET_ONLY_CREATE_GANG_MEMBERS(0983)
void SetOnlyCreateGangMembers(bool v) { CPopulation::m_bOnlyCreateRandomGangMembers = !v; }

// START_CREDITS(0434)
void StartCredits() { CCredits::Start(); }

// STOP_CREDITS(0435)
void StopCredits() { CCredits::Stop(); }

// CLEAR_ALL_SCRIPT_ROADBLOCKS(04C1)
void ClearAllScriptRoadblocks() { CRoadBlocks::ClearScriptRoadBlocks(); }

// DELETE_MISSION_TRAINS(06D9)
void DeleteMissionTrains() { CTrain::RemoveMissionTrains(); }

// MARK_MISSION_TRAINS_AS_NO_LONGER_NEEDED(06DA)
void MarkMissionTrainsAsNoLongerNeeded() { CTrain::ReleaseMissionTrains(); }

// DELETE_ALL_TRAINS(06DB)
void DeleteAllTrains() { CTrain::RemoveAllTrains(); }

// SET_TIME_ONE_DAY_FORWARD(088E)
void SetTimeOneDayForward() { CClock::OffsetClockByADay(1); }

// GET_CURRENT_DAY_OF_WEEK(07D0)
int32 GetCurrentDayOfWeek() { return CClock::CurrentDay; }

// CLEAR_ALL_SCRIPT_FIRE_FLAGS(0986)
void ClearAllScriptFireFlags() { gFireManager.ClearAllScriptFireFlags(); }

// UNMARK_ALL_ROAD_NODES_AS_DONT_WANDER(0995)
void UnmarkAllRoadNodesAsDontWander() { ThePaths.UnMarkAllRoadNodesAsDontWander(); }

// RELEASE_PATH_NODES(0607)
void ReleasePathNodes() { ThePaths.ReleaseRequestedNodes(); }

// GET_BEAT_TRACK_STATUS(0953)
int32 GetBeatTrackStatus() { return AudioEngine.GetBeatTrackStatus(); }

// PLAY_BEAT_TRACK(0954); the exe always passes false
void PlayBeatTrack() { AudioEngine.PlayPreloadedBeatTrack(false); }

// STOP_BEAT_TRACK(0955)
void StopBeatTrack() { AudioEngine.StopBeatTrack(); }

// GET_RADIO_CHANNEL(051E)
int32 GetRadioChannel() { return (int32)AudioEngine.GetCurrentRadioStationID() - 1; }

// SET_WEATHER_TO_APPROPRIATE_TYPE_NOW(0915)
void SetWeatherToAppropriateTypeNow() { CWeather::SetWeatherToAppropriateTypeNow(); }

// RELEASE_WEATHER(01B7)
void ReleaseWeather() { CWeather::ReleaseWeather(); }

// START_CUTSCENE(02E7)
void StartCutscene() { CCutsceneMgr::StartCutscene(); }

// CLEAR_CUTSCENE(02EA)
void ClearCutscene() { CCutsceneMgr::DeleteCutsceneData(); }

// WAS_CUTSCENE_SKIPPED(056A)
bool WasCutsceneSkipped() { return CCutsceneMgr::ms_wasCutsceneSkipped; }

// SKIP_CUTSCENE_START_INTERNAL(0707)
void SkipCutsceneStartInternal(CRunningScript& S, int32 ip) { S.m_SceneSkipIP = ip; }

// SKIP_CUTSCENE_END(0701)
void SkipCutsceneEnd(CRunningScript& S) { S.m_SceneSkipIP = 0; }

// SET_DEATHARREST_STATE(0111)
void SetDeathArrestState(CRunningScript& S, int32 state) { S.m_IsDeathArrestCheckEnabled = state == 1; }

// HAS_DEATHARREST_BEEN_EXECUTED(0112)
bool HasDeathArrestBeenExecuted(CRunningScript& S) { return S.m_DoneDeathArrest; }

// USE_DETONATOR(09D9)
void UseDetonator() { CWorld::UseDetonator(FindPlayerPed()); }

// MAKE_PLAYER_GANG_DISAPPEAR(08F5)
void MakePlayerGangDisappear() { FindPlayerPed()->MakePlayerGroupDisappear(); }

// MAKE_PLAYER_GANG_REAPPEAR(08F6)
void MakePlayerGangReappear() { FindPlayerPed()->MakePlayerGroupReappear(); }

// CLEAR_WANTED_LEVEL_IN_GARAGE(09D4)
void ClearWantedLevelInGarage() { FindPlayerWanted()->ClearWantedLevelAndGoOnParole(); }

// GET_MAX_WANTED_LEVEL(050F)
int32 GetMaxWantedLevel() { return (int32)CWanted::MaximumWantedLevel; }

// CANCEL_OVERRIDE_RESTART(01F6)
void CancelOverrideRestart() { CRestart::CancelOverrideRestart(); }

// GET_PROGRESS_PERCENTAGE(058C)
float GetProgressPercentage() { return CStats::GetPercentageProgress(); }

// SHOW_UPDATE_STATS(08F8)
void ShowUpdateStats(bool show) { CStats::bShowUpdateStats = show; }

// FIND_MAX_NUMBER_OF_GROUP_MEMBERS(0956)
int32 FindMaxNumberOfGroupMembers() { return CStats::FindMaxNumberOfGroupMembers(); }

// IS_GANG_WAR_GOING_ON(087A)
bool IsGangWarGoingOn() { return CGangWars::GangWarGoingOn(); }

// FIND_NUMBER_TAGS_TAGGED(08E1)
int32 FindNumberTagsTagged() { return CTagManager::ms_numTagged; }

// REQUEST_CAR_RECORDING(07C0)
void RequestCarRecording(int32 fileNumber) { CVehicleRecording::RequestRecordingFile(fileNumber); }

// REMOVE_CAR_RECORDING(0873)
void RemoveCarRecording(int32 fileNumber) { CVehicleRecording::RemoveRecordingFile(fileNumber); }

// REMOVE_SPHERE(03BD)
void RemoveSphere(int32 index) { CTheScripts::RemoveScriptSphere(index); }

// REMOVE_USER_3D_MARKER(0A41)
void RemoveUser3dMarker(int32 slot) { C3dMarkers::User3dMarkerDelete(slot); }

// IS_AUSTRALIAN_GAME(059A)
bool IsAustralianGame() { return false; }

// IS_GERMAN_GAME(040C)
bool IsGermanGame() { return CLocalisation::GermanGame(); }

// IS_PC_VERSION(0485)
bool IsPcVersion() { return true; }

// GET_CONTROLLER_MODE(0293)
int32 GetControllerMode() { return 0; }

// IS_NIGHT_VISION_ACTIVE(099D)
bool IsNightVisionActive() { return CPostEffects::m_bNightVision; }

// IS_MINIGAME_IN_PROGRESS(09BE)
bool IsMinigameInProgress() { return CTheScripts::bMiniGameInProgress; }

// ARE_SUBTITLES_SWITCHED_ON(09C8)
bool AreSubtitlesSwitchedOn() { return FrontEndMenuManager.m_bShowSubtitles; }

// GET_AREA_VISIBLE(077E)
int32 GetAreaVisible() { return (int32)CGame::currArea; }

// SET_RADAR_ZOOM(075B)
void SetRadarZoom(uint8 zoom) { CTheScripts::RadarZoomValue = zoom; }

// SHOW_BLIPS_ON_ALL_LEVELS(09A6)
void ShowBlipsOnAllLevels(bool show) { CTheScripts::RadarShowBlipOnAllLevels = show; }

// SET_FORCE_RANDOM_CAR_MODEL(09BF)
void SetForceRandomCarModel(int32 model) { CTheScripts::ForceRandomCarModel = model; }

// FORCE_ALL_VEHICLE_LIGHTS_OFF(0A37)
void ForceAllVehicleLightsOff(bool off) { CVehicle::ms_forceVehicleLightsOff = off; }

// SET_HELP_MESSAGE_BOX_SIZE(0989); the script passes an integer width
void SetHelpMessageBoxSize(int32 width) { CHud::m_fHelpMessageBoxWidth = (float)width; }

// FORCE_BIG_MESSAGE_AND_COUNTER(09EE)
void ForceBigMessageAndCounter(bool force) { CHud::bScriptForceDisplayWithCounters = force; }

// SET_TIME_SCALE(015D)
void SetTimeScale(float scale) { CTimer::SetTimeScale(scale); }

// SET_GUNSHOT_SENSE_RANGE_FOR_RIOT2(098A)
void SetGunshotSenseRangeForRiot2(float range) { CEventGunShot::ms_fGunShotSenseRangeForRiot2 = range; }

// DISABLE_ALL_ENTRY_EXITS(08E7)
void DisableAllEntryExits(bool disable) { CEntryExitManager::ms_bDisabled = disable; }

// SET_AIRCRAFT_CARRIER_SAM_SITE(09E4); the stored flag is "disabled"
void SetAircraftCarrierSamSite(bool enabled) { CObject::bAircraftCarrierSamSiteDisabled = !enabled; }

// SWITCH_STREAMING(03AF); the stored flag is "disabled"
void SwitchStreaming(bool enabled) { CStreaming::ms_disableStreaming = !enabled; }

// SWITCH_WORLD_PROCESSING(03B7); the stored flag is "only process the cutscene"
void SwitchWorldProcessing(bool enabled) { CWorld::bProcessCutsceneOnly = !enabled; }
}

void notsa::script::commands::misc::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Misc");

    REGISTER_COMMAND_HANDLER(COMMAND_STORE_CLOTHES_STATE, StoreClothesState);
    REGISTER_COMMAND_HANDLER(COMMAND_RESTORE_CLOTHES_STATE, RestoreClothesState);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_LOADED_SHOP, ClearLoadedShop);
    REGISTER_COMMAND_HANDLER(COMMAND_STORE_CAR_MOD_STATE, StoreCarModState);
    REGISTER_COMMAND_HANDLER(COMMAND_RESTORE_CAR_MOD_STATE, RestoreCarModState);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_NUMBER_OF_ITEMS_IN_SHOP, GetNumberOfItemsInShop);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_ITEM_IN_SHOP, GetItemInShop);
    REGISTER_COMMAND_HANDLER(COMMAND_FORCE_DEATH_RESTART, ForceDeathRestart);
    REGISTER_COMMAND_HANDLER(COMMAND_RESET_STUFF_UPON_RESURRECTION, ResetStuffUponResurrection);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_SKIP, ClearSkip);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_SKIP_WAITING_FOR_SCRIPT_TO_FADE_IN, IsSkipWaitingForScriptToFadeIn);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_ARREST_PENALTIES, SwitchArrestPenalties);
    REGISTER_COMMAND_HANDLER(COMMAND_FAIL_KILL_FRENZY, FailKillFrenzy);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_TOTAL_NUMBER_OF_PEDS_KILLED_BY_PLAYER, GetTotalNumberOfPedsKilledByPlayer);
    REGISTER_COMMAND_HANDLER(COMMAND_MANAGE_ALL_POPULATION, ManageAllPopulation);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CREATE_RANDOM_GANG_MEMBERS, SetCreateRandomGangMembers);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_ONLY_CREATE_GANG_MEMBERS, SetOnlyCreateGangMembers);
    REGISTER_COMMAND_HANDLER(COMMAND_START_CREDITS, StartCredits);
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_CREDITS, StopCredits);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_ALL_SCRIPT_ROADBLOCKS, ClearAllScriptRoadblocks);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_MISSION_TRAINS, DeleteMissionTrains);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_MISSION_TRAINS_AS_NO_LONGER_NEEDED, MarkMissionTrainsAsNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_ALL_TRAINS, DeleteAllTrains);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TIME_ONE_DAY_FORWARD, SetTimeOneDayForward);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CURRENT_DAY_OF_WEEK, GetCurrentDayOfWeek);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_ALL_SCRIPT_FIRE_FLAGS, ClearAllScriptFireFlags);
    REGISTER_COMMAND_HANDLER(COMMAND_UNMARK_ALL_ROAD_NODES_AS_DONT_WANDER, UnmarkAllRoadNodesAsDontWander);
    REGISTER_COMMAND_HANDLER(COMMAND_RELEASE_PATH_NODES, ReleasePathNodes);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_BEAT_TRACK_STATUS, GetBeatTrackStatus);
    REGISTER_COMMAND_HANDLER(COMMAND_PLAY_BEAT_TRACK, PlayBeatTrack);
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_BEAT_TRACK, StopBeatTrack);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_RADIO_CHANNEL, GetRadioChannel);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_WEATHER_TO_APPROPRIATE_TYPE_NOW, SetWeatherToAppropriateTypeNow);
    REGISTER_COMMAND_HANDLER(COMMAND_RELEASE_WEATHER, ReleaseWeather);
    REGISTER_COMMAND_HANDLER(COMMAND_START_CUTSCENE, StartCutscene);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_CUTSCENE, ClearCutscene);
    REGISTER_COMMAND_HANDLER(COMMAND_WAS_CUTSCENE_SKIPPED, WasCutsceneSkipped);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_CUTSCENE_START_INTERNAL, SkipCutsceneStartInternal);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_CUTSCENE_END, SkipCutsceneEnd);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_DEATHARREST_STATE, SetDeathArrestState);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_DEATHARREST_BEEN_EXECUTED, HasDeathArrestBeenExecuted);
    REGISTER_COMMAND_HANDLER(COMMAND_USE_DETONATOR, UseDetonator);
    REGISTER_COMMAND_HANDLER(COMMAND_MAKE_PLAYER_GANG_DISAPPEAR, MakePlayerGangDisappear);
    REGISTER_COMMAND_HANDLER(COMMAND_MAKE_PLAYER_GANG_REAPPEAR, MakePlayerGangReappear);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_WANTED_LEVEL_IN_GARAGE, ClearWantedLevelInGarage);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_MAX_WANTED_LEVEL, GetMaxWantedLevel);
    REGISTER_COMMAND_HANDLER(COMMAND_CANCEL_OVERRIDE_RESTART, CancelOverrideRestart);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_PROGRESS_PERCENTAGE, GetProgressPercentage);
    REGISTER_COMMAND_HANDLER(COMMAND_SHOW_UPDATE_STATS, ShowUpdateStats);
    REGISTER_COMMAND_HANDLER(COMMAND_FIND_MAX_NUMBER_OF_GROUP_MEMBERS, FindMaxNumberOfGroupMembers);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GANG_WAR_GOING_ON, IsGangWarGoingOn);
    REGISTER_COMMAND_HANDLER(COMMAND_FIND_NUMBER_TAGS_TAGGED, FindNumberTagsTagged);
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_CAR_RECORDING, RequestCarRecording);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_CAR_RECORDING, RemoveCarRecording);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_SPHERE, RemoveSphere);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_USER_3D_MARKER, RemoveUser3dMarker);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_AUSTRALIAN_GAME, IsAustralianGame);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GERMAN_GAME, IsGermanGame);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_PC_VERSION, IsPcVersion);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CONTROLLER_MODE, GetControllerMode);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_NIGHT_VISION_ACTIVE, IsNightVisionActive);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_MINIGAME_IN_PROGRESS, IsMinigameInProgress);
    REGISTER_COMMAND_HANDLER(COMMAND_ARE_SUBTITLES_SWITCHED_ON, AreSubtitlesSwitchedOn);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_AREA_VISIBLE, GetAreaVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RADAR_ZOOM, SetRadarZoom);
    REGISTER_COMMAND_HANDLER(COMMAND_SHOW_BLIPS_ON_ALL_LEVELS, ShowBlipsOnAllLevels);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_FORCE_RANDOM_CAR_MODEL, SetForceRandomCarModel);
    REGISTER_COMMAND_HANDLER(COMMAND_FORCE_ALL_VEHICLE_LIGHTS_OFF, ForceAllVehicleLightsOff);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_HELP_MESSAGE_BOX_SIZE, SetHelpMessageBoxSize);
    REGISTER_COMMAND_HANDLER(COMMAND_FORCE_BIG_MESSAGE_AND_COUNTER, ForceBigMessageAndCounter);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TIME_SCALE, SetTimeScale);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GUNSHOT_SENSE_RANGE_FOR_RIOT2, SetGunshotSenseRangeForRiot2);
    REGISTER_COMMAND_HANDLER(COMMAND_DISABLE_ALL_ENTRY_EXITS, DisableAllEntryExits);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_AIRCRAFT_CARRIER_SAM_SITE, SetAircraftCarrierSamSite);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_STREAMING, SwitchStreaming);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_WORLD_PROCESSING, SwitchWorldProcessing);
}
