#include "StdInc.h"

#include "AEScriptAudioEntity.h"
#include "AESoundManager.h"
#include "AEAudioUtility.h"
#include "AEAudioHardware.h"
#include "AEAmbienceTrackManager.h"

// 0x5074D0
CAEScriptAudioEntity::CAEScriptAudioEntity() : CAEAudioEntity() {
    m_nLastTimeHornPlayed = 0;
    field_7E = 0;
    field_7C = 0;
    m_Volume = 0.0f;
    m_Speed = 1.0f;
    field_7D = 0;
    field_8C = 0.0f;
}

// 0x5B9B60
void CAEScriptAudioEntity::Initialise() {
    for (auto& link : wavLinks) {
        link.Init();
    }
}

// 0x4EC150
void CAEScriptAudioEntity::Reset() {
    for (auto i = 0; i < MISSION_AUDIO_COUNT; i++) {
        ClearMissionAudio(i);
    }
    field_7C = 0;
    m_Entity = nullptr;
    field_7D = 0;
    field_8C = 2.0f;
}

// 0x0
void CAEScriptAudioEntity::AddAudioEvent(int32) {
    /* Android NOP */
}

// 0x4EC100
CVector* CAEScriptAudioEntity::AttachMissionAudioToPhysical(uint8 sampleId, CPhysical* physical) {
    auto& link = wavLinks[sampleId];
    link.m_pEntity   = physical;
    link.m_vPosition = CVector(-1000.0f, -1000.0f, -1000.0f);
    return &link.m_vPosition;
}

// 0x4EC040
void CAEScriptAudioEntity::ClearMissionAudio(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return;
    }

    AESoundManager.CancelSoundsInBankSlot(static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + sampleId), true);

    auto& link = wavLinks[sampleId];
    link.m_pEntity   = nullptr;
    link.m_Sound     = nullptr;
    link.m_vPosition = CVector(-1000.0f, -1000.0f, -1000.0f);
}

// 0x4EBFE0
bool CAEScriptAudioEntity::IsMissionAudioSampleFinished(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return true;
    }
    if (sampleId >= 2) {
        return !AESoundManager.AreSoundsPlayingInBankSlot(SND_BANK_SLOT_MISSION1 + sampleId);
    }
    return wavLinks[sampleId].m_Sound == nullptr;
}

// 0x4EBF60
int8 CAEScriptAudioEntity::GetMissionAudioLoadingStatus(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return true;
    }

    const auto& link = wavLinks[sampleId];
    if (link.m_nBankId < 0) {
        return true;
    }

    const auto slot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + sampleId);
    return link.m_nBankSlotId < 0
        ? AEAudioHardware.GetSoundBankLoadingStatus(static_cast<eSoundBank>(link.m_nBankId), slot)
        : AEAudioHardware.GetSoundLoadingStatus(static_cast<eSoundBank>(link.m_nBankId), static_cast<eSoundID>(link.m_nBankSlotId), slot);
}

// 0x4EC020
int32 CAEScriptAudioEntity::GetMissionAudioEvent(uint8 sampleId) {
    return wavLinks[sampleId].m_nAudioEvent;
}

// 0x4EC0C0
void CAEScriptAudioEntity::SetMissionAudioPosition(uint8 sampleId, CVector& posn) {
    wavLinks[sampleId].m_vPosition = posn;
    wavLinks[sampleId].m_pEntity   = nullptr;
}

// 0x4EC4D0
CVector* CAEScriptAudioEntity::GetMissionAudioPosition(uint8 sampleId) {
    auto& link = wavLinks[sampleId];
    if (link.m_pEntity) {
        return &link.m_pEntity->GetPosition();
    }
    if (link.m_vPosition != CVector{ -1000.0f, -1000.0f, -1000.0f } && link.m_vPosition != 0.0f) {
        return &link.m_vPosition;
    }
    return nullptr;
}

// 0x4EC6D0
void CAEScriptAudioEntity::PlayMissionBankSound(eAudioEvents eventId, CVector& posn, CPhysical* physical, int16 sfxId, uint8 linkId, uint8 a7, float volume, float maxDistance, float speed) {
    if (linkId < 2 || linkId > 3) {
        return;
    }
    if (a7 && AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
        return;
    }

    const auto slot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + linkId);
    if (!AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(wavLinks[linkId].m_nBankId), slot)) {
        return;
    }

    const auto finalVolume = GetDefaultVolume(eventId) + volume;

    bool    frontend = false;
    CVector pos;
    if (physical) {
        pos = physical->GetPosition();
    } else if (posn != CVector{-1000.0f, -1000.0f, -1000.0f} && !posn.IsZero()) {
        pos = posn;
    } else {
        frontend = true;
        pos = CVector{0.0f, 1.0f, 0.0f};
    }

    AESoundManager.PlaySound({
        .BankSlotID         = slot,
        .SoundID            = static_cast<eSoundID>(sfxId),
        .AudioEntity        = this,
        .Pos                = pos,
        .Volume             = finalVolume,
        .RollOffFactor      = maxDistance,
        .Speed              = speed,
        .Doppler            = 1.0f,
        .Flags              = SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | (frontend ? SOUND_FRONT_END : 0u),
        .RegisterWithEntity = physical,
        .EventID            = eventId,
    });
}

// event eAudioEvents
// 0x4EC550
void CAEScriptAudioEntity::PlayResidentSoundEvent(eSoundBankSlot slot, eSoundBank bank, eSoundID sfx, eAudioEvents event, CVector& posn, CPhysical* physical, float vol, float speed, int16 playPosn, float maxDistance) {
    if (!AEAudioHardware.IsSoundBankLoaded(bank, slot)) {
        return;
    }

    bool bFrontend = false;
    const auto volume = GetDefaultVolume(static_cast<eAudioEvents>(event)) + vol;
    CVector pos = [&] {
        if (physical) {
            return physical->GetPosition();
        } else if (posn == -1000.0f || posn.IsZero()) {
            bFrontend = true;
            return CVector{0.0f, 1.0f, 0.0f};
        } else {
            return posn;
        }
    }();

    AESoundManager.PlaySound({
        .BankSlotID         = slot,
        .SoundID            = sfx,
        .AudioEntity        = this,
        .Pos                = pos,
        .Volume             = volume,
        .RollOffFactor      = maxDistance,
        .Speed              = speed,
        .Doppler            = 1.0f,
        .FrameDelay         = 0,
        .Flags              = SOUND_START_PERCENTAGE | SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | (bFrontend ? SOUND_FRONT_END : 0u),
        .FrequencyVariance  = 0.0f,
        .PlayTime           = playPosn,
        .RegisterWithEntity = physical,
        .EventID            = (eAudioEvents)(event),
    });
}

// 0x4EC270
void CAEScriptAudioEntity::PlayLoadedMissionAudio(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return;
    }

    auto& link = wavLinks[sampleId];
    if (link.m_nBankId < 0 || link.m_nBankSlotId < 0) {
        return;
    }
    if (GetMissionAudioLoadingStatus(sampleId) != 1) {
        return;
    }

    bool  lowVolumeBoost = false;
    float volume;
    if (link.m_nAudioEvent == -1) {
        volume = -100.0f;
    } else {
        volume = GetDefaultVolume(static_cast<eAudioEvents>(link.m_nAudioEvent));
        if (sampleId < 2 && volume == -128.0f) {
            lowVolumeBoost = true;
            volume = 6.0f;
        }
    }

    auto* const posPtr  = GetMissionAudioPosition(sampleId);
    const bool  frontend = !posPtr;
    const auto  pos      = posPtr ? *posPtr : CVector{0.0f, 1.0f, 0.0f};

    link.m_Sound = AESoundManager.PlaySound({
        .BankSlotID        = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + sampleId),
        .SoundID           = static_cast<eSoundID>(link.m_nBankSlotId),
        .AudioEntity       = this,
        .Pos               = pos,
        .Volume            = volume,
        .RollOffFactor     = 2.0f,
        .Speed             = 1.0f,
        .Doppler           = 1.0f,
        .FrameDelay        = 0,
        .Flags             = SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | SOUND_PLAY_PHYSICALLY
                            | (frontend      ? SOUND_FRONT_END : 0u)
                            | (lowVolumeBoost ? (SOUND_IS_DUCKABLE | SOUND_IS_COMPRESSABLE | SOUND_SMOOTH_DUCKING) : 0u),
        .FrequencyVariance = 0.0f,
        .PlayTime          = 0,
    });
}

// 0x4EC190
void CAEScriptAudioEntity::PreloadMissionAudio(uint8 slotId, int32 sampleId) {
    if (slotId >= MISSION_AUDIO_COUNT || !IsMissionAudioSampleFinished(slotId)) {
        return;
    }

    eSoundBankS32 bankId{};
    int32         soundId{};
    if (!CAEAudioUtility::GetBankAndSoundFromScriptSlotAudioEvent(static_cast<eAudioEvents>(sampleId), bankId, soundId, slotId)) {
        return;
    }

    auto& link = wavLinks[slotId];
    link.m_nBankId     = bankId;
    link.m_nBankSlotId = soundId;

    const auto slot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + slotId);
    if (soundId < 0) {
        AEAudioHardware.LoadSoundBank(static_cast<eSoundBank>(bankId), slot);
    } else {
        AEAudioHardware.LoadSound(static_cast<eSoundBank>(bankId), static_cast<eSoundID>(soundId), slot);
    }

    link.m_nAudioEvent = sampleId;
    link.m_pEntity     = nullptr;
    link.m_vPosition   = CVector(-1000.0f, -1000.0f, -1000.0f);
}

// 0x4ECCF0
void CAEScriptAudioEntity::ProcessMissionAudioEvent(eAudioEvents eventId, CVector& posn, CPhysical* physical, float volume, float speed) {
    // Plays the event with a sound of the bank that the mission has loaded
    const auto Bank = [&](int16 sfxId, uint8 linkId, uint8 a7 = 0, float vol = 0.0f, float maxDistance = 2.0f, float spd = 1.0f) {
        PlayMissionBankSound(eventId, posn, physical, sfxId, linkId, a7, vol, maxDistance, spd);
    };

    // Plays the event with a sound of a bank that is always loaded
    const auto Resident = [&](int32 slot, int32 bank, int32 sfxId, float vol = 0.0f, float spd = 1.0f, int16 playPosn = 0, float maxDistance = 1.0f) {
        PlayResidentSoundEvent(static_cast<eSoundBankSlot>(slot), static_cast<eSoundBank>(bank), static_cast<eSoundID>(sfxId), eventId, posn, physical, vol, spd, playPosn, maxDistance);
    };

    // Stops what the given event plays for the physical, or for everyone when there is none
    const auto CancelEvent = [&](eAudioEvents event) {
        if (physical) {
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntityAndPhysical(event, this, physical);
        } else {
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(event, this);
        }
    };

    // The doors of the garages are where the physical is, or where the script says
    const auto Garage = [&](eAudioEvents event, float vol, float spd) {
        m_GarageAudio.AddAudioEvent(event, physical ? physical->GetPosition() : posn, vol, spd);
    };

    switch (static_cast<uint16>(eventId)) {
    case AE_CRANE_WINCH_MOVE:
        if (field_7D) {
            if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(eventId, this, physical)) {
                Resident(40, 44, 1, volume, speed, 0, 2.5f);
            }
            m_Speed               = speed;
            m_Volume              = GetDefaultVolume(eventId) + volume;
            m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        }
        break;
    case AE_SCRIPT_DISABLE_HELI_AUDIO:
        CAEVehicleAudioEntity::DisableHelicoptors();
        break;
    case AE_SCRIPT_ENABLE_HELI_AUDIO:
        CAEVehicleAudioEntity::EnableHelicoptors();
        break;
    case AE_SCRIPT_CEILING_VENT_LAND:
        Resident(2, 39, 64, 0.0f, 0.79f, 35);
        break;
    case AE_SCRIPT_CLAXON_START:
    case AE_SCRIPT_KEYPAD_FAIL:
    case AE_SCRIPT_OGLOC_WINDOW_RATTLE_BANG:
    case AE_SCRIPT_STINGER_RELOAD:
    case AE_SCRIPT_DA_NANG_HEAVY_DOOR_OPEN:
    case AE_SCRIPT_MECHANIC_SLIDE_OUT:
    case AE_SCRIPT_CAT2_WOODEN_DOOR_BREACH:
    case AE_SCRIPT_VERTICAL_BIRD_ALARM_START:
        Bank(1, 2);
        break;
    case AE_SCRIPT_CLAXON_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CLAXON_START, this);
        break;
    case AE_SCRIPT_BLAST_DOOR_SLIDE_START:
    case AE_SCRIPT_OGLOC_DOORBELL:
    case AE_SCRIPT_DA_NANG_CONTAINER_OPEN:
    case AE_SCRIPT_STINGER_FIRE:
    case AE_SCRIPT_MECHANIC_ATTACH_CAR_BOMB:
    case AE_SCRIPT_CAT2_SECURITY_ALARM:
        Bank(0, 2);
        break;
    case AE_SCRIPT_BLAST_DOOR_SLIDE_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_BLAST_DOOR_SLIDE_START, this);
        break;
    case AE_SCRIPT_BONNET_DENT:
    case AE_SCRIPT_CAR_SMASH_CAR:
    case AE_SCRIPT_MAGNET_VEHICLE_COLLISION:
        Resident(2, 39, CAEAudioUtility::GetRandomNumberInRange<int32>(20, 28));
        break;
    case AE_SCRIPT_BASKETBALL_BOUNCE:
        Bank(CAEAudioUtility::GetRandomNumberInRange<int32>(0, 2), 3);
        break;
    case AE_SCRIPT_BASKETBALL_HIT_HOOP:
    case AE_SCRIPT_GOGO_EXPLOSION:
    case AE_SCRIPT_BANDIT_INSERT_COIN:
    case AE_SCRIPT_GYM_INCREASE_DIFFICULTY:
    case AE_SCRIPT_OTB_NO_CASH:
        Bank(3, 3);
        break;
    case AE_SCRIPT_BASKETBALL_SCORE:
        if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
            Bank(4, 3);
        }
        break;
    case AE_SCRIPT_POOL_HIT_WHITE:
        Bank(10, 3);
        break;
    case AE_SCRIPT_POOL_HIT_CUSHION:
    case AE_SCRIPT_DUAL_SHOOT:
        Bank(8, 3);
        break;
    case AE_SCRIPT_POOL_BALL_POT:
        Bank(CAEAudioUtility::GetRandomNumberInRange<int32>(3, 5), 3);
        break;
    case AE_SCRIPT_POOL_CHALK_CUE:
    case AE_SCRIPT_GYM_REST_WEIGHTS:
    case AE_SCRIPT_TEMPEST_SELECT:
        Bank(7, 3);
        break;
    case AE_SCRIPT_CRANE_ENTER:
        if (!AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(44), static_cast<eSoundBankSlot>(40))) {
            if (AESoundManager.AreSoundsPlayingInBankSlot(40)) {
                AESoundManager.CancelSoundsInBankSlot(40, false);
            }
            AEAudioHardware.LoadSoundBank(static_cast<eSoundBank>(44), static_cast<eSoundBankSlot>(40));
        }
        m_Physical = physical;
        field_7D   = 1;
        break;
    case AE_SCRIPT_CRANE_MOVE_START:
        if (field_7D) {
            if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(eventId, this, physical)) {
                Resident(40, 44, 1, 0.0f, 1.0f, 0, 2.5f);
            }
        }
        break;
    case AE_SCRIPT_CRANE_MOVE_STOP:
        if (field_7D) {
            CancelEvent(AE_SCRIPT_CRANE_MOVE_START);
            Resident(40, 44, 2, 0.0f, 1.0f, 0, 2.5f);
        }
        break;
    case AE_SCRIPT_CRANE_EXIT:
        if (field_7D) {
            AESoundManager.CancelSoundsInBankSlot(40, true);
            Resident(40, 44, 3, 0.0f, 1.0f, 0, 2.5f);
            m_Physical = nullptr;
            field_7D   = 0;
        }
        break;
    case AE_SCRIPT_VIDEO_POKER_PAYOUT:
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_DROP:
    case AE_SCRIPT_GOGO_DECLINE:
    case AE_SCRIPT_BEE_ACCEPT:
    case AE_SCRIPT_BANDIT_PAYOUT:
    case AE_SCRIPT_OTB_INCREASE_BET:
    case AE_SCRIPT_RESTAURANT_CJ_PUKE:
    case AE_SCRIPT_TEMPEST_ENEMY_SHOOT:
        Bank(1, 3);
        break;
    case AE_SCRIPT_VIDEO_POKER_BUTTON:
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_START:
    case AE_SCRIPT_GOGO_ACCEPT:
    case AE_SCRIPT_BANDIT_WHEEL_START:
    case AE_SCRIPT_OTB_BET_ZERO:
    case AE_SCRIPT_RESTAURANT_CJ_EAT:
        Bank(0, 3);
        break;
    case AE_SCRIPT_WHEEL_OF_FORTUNE_CLACKER:
        Resident(2, 39, 19, 0.0f, 1.0f, 70);
        break;
    case AE_SCRIPT_KEYPAD_BEEP:
        if (AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(242), static_cast<eSoundBankSlot>(28))) {
            Bank(0, 2);
        } else if (AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(345), static_cast<eSoundBankSlot>(28))) {
            Bank(3, 2);
        }
        break;
    case AE_SCRIPT_KEYPAD_PASS:
    case AE_SCRIPT_HEAVY_DOOR_START:
        Bank(2, 2);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_SHATTER:
        Bank(CAEAudioUtility::GetRandomNumberInRange<int32>(2, 6), 3);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_STOP:
        CancelEvent(AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_START);
        break;
    case AE_SCRIPT_SHUTTER_DOOR_START:
        Garage(AE_GARAGE_DOOR_OPENING, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_SHUTTER_DOOR_STOP:
        Garage(AE_GARAGE_DOOR_OPENED, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_PARACHUTE_OPEN:
        Resident(5, 143, 65);
        break;
    case AE_SCRIPT_DUAL_THRUST:
    case AE_SCRIPT_BEE_BUZZ:
    case AE_SCRIPT_TEMPEST_SHIELD_GLOW:
        Bank(0, 3, 1);
        m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        break;
    case AE_SCRIPT_DUAL_EXPLOSION_SHORT:
    case AE_SCRIPT_OTB_LOSE:
        Bank(2, 3, 1);
        break;
    case AE_SCRIPT_DUAL_EXPLOSION_LONG:
        Bank(1, 3, 1);
        break;
    case AE_SCRIPT_DUAL_MENU_SELECT:
    case AE_SCRIPT_GOGO_PLAYER_FIRE:
    case AE_SCRIPT_BEE_SELECT:
    case AE_SCRIPT_TEMPEST_PICKUP3:
        Bank(5, 3);
        break;
    case AE_SCRIPT_DUAL_MENU_DESELECT:
    case AE_SCRIPT_BEE_PICKUP:
    case AE_SCRIPT_OTB_PLACE_BET:
    case AE_SCRIPT_TEMPEST_HIGHLIGHT:
        Bank(4, 3);
        break;
    case AE_SCRIPT_DUAL_GAME_OVER:
    case AE_SCRIPT_BEE_GAME_OVER:
    case AE_SCRIPT_TEMPEST_GAME_OVER:
        Bank(3, 3, 1);
        break;
    case AE_SCRIPT_DUAL_PICKUP_LIGHT:
        Bank(7, 3, 1);
        break;
    case AE_SCRIPT_DUAL_PICKUP_DARK:
        Bank(6, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TOUCH_DARK:
        Bank(9, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TOUCH_LIGHT:
        Bank(10, 3, 1);
        break;
    case AE_SCRIPT_AMMUNATION_BUY_WEAPON:
    case AE_SCRIPT_SHOP_BUY:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PURCHASE_WEAPON);
        break;
    case AE_SCRIPT_AMMUNATION_BUY_WEAPON_DENIED:
    case AE_SCRIPT_SHOP_BUY_DENIED:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH);
        break;
    case AE_SCRIPT_RACE_321:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RACE_321);
        break;
    case AE_SCRIPT_RACE_GO:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RACE_GO);
        break;
    case AE_SCRIPT_PART_MISSION_COMPLETE:
    case AE_SCRIPT_CHECKPOINT_GREEN:
    case AE_SCRIPT_PROPERTY_PURCHASED:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE);
        break;
    case AE_SCRIPT_GOGO_ENEMY_FIRE:
    case AE_SCRIPT_BEE_DROP:
    case AE_SCRIPT_GYM_BOXING_BELL:
    case AE_SCRIPT_TEMPEST_EXPLOSION:
        Bank(2, 3);
        break;
    case AE_SCRIPT_GOGO_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_WEAPON_CHAINSAW_IDLE);
        break;
    case AE_SCRIPT_GOGO_TRACK_STOP:
    case AE_SCRIPT_DUAL_TRACK_STOP:
    case AE_SCRIPT_BEE_TRACK_STOP:
    case AE_SCRIPT_AWARD_TRACK_STOP:
    case AE_SCRIPT_OTB_TRACK_STOP:
    case AE_SCRIPT_TEMPEST_TRACK_STOP:
    case AE_SCRIPT_DRIVING_AWARD_TRACK_STOP:
    case AE_SCRIPT_BIKE_AWARD_TRACK_STOP:
    case AE_SCRIPT_PILOT_AWARD_TRACK_STOP:
        AEAmbienceTrackManager.StopSpecialMissionAmbienceTrack();
        break;
    case AE_SCRIPT_GOGO_SELECT:
    case AE_SCRIPT_BEE_ZAP:
    case AE_SCRIPT_TEMPEST_PLAYER_SHOOT:
        Bank(6, 3);
        break;
    case AE_SCRIPT_GOGO_GAME_OVER:
        Bank(4, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_WEAPON_FIRE_MINIGUN_AMMO);
        break;
    case AE_SCRIPT_BEE_DECLINE:
        Bank(1, 3, 0, 0.0f, 2.0f, 0.79f);
        break;
    case AE_SCRIPT_BEE_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_THUNDER);
        break;
    case AE_SCRIPT_FREEZER_OPEN:
        Bank(2, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_FREEZER_CLOSE:
        Bank(1, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_MEAT_TRACK_START:
        Bank(3, 2, 1, 0.0f, 3.0f);
        Bank(0, 2, 0, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_MEAT_TRACK_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_MEAT_TRACK_START, this);
        Bank(4, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_ROULETTE_ADD_CASH:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_START);
        break;
    case AE_SCRIPT_ROULETTE_REMOVE_CASH:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_BACK);
        break;
    case AE_SCRIPT_ROULETTE_NO_CASH:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_ERROR);
        break;
    case AE_SCRIPT_ROULETTE_SPIN:
        Bank(0, 3);
        m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        field_7C = 0;
        break;
    case AE_SCRIPT_BANDIT_WHEEL_STOP:
        CancelEvent(AE_SCRIPT_BANDIT_WHEEL_START);
        Bank(2, 3);
        break;
    case AE_SCRIPT_BIKE_PACKER_CLUNK:
        Resident(2, 39, 65, 0.0f, 1.0f, 16);
        break;
    case AE_SCRIPT_AWARD_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_WEAPON_FIRE);
        break;
    case AE_SCRIPT_MESH_GATE_OPEN_START:
        Resident(2, 39, field_7E ? 69 : 70, 0.0f, 1.0f, field_7E ? 63 : 62);
        field_7E = (field_7E + 1) % 2;
        break;
    case AE_SCRIPT_MESH_GATE_OPEN_STOP:
        Resident(2, 39, field_7E ? 69 : 70, 0.0f, field_7E ? 0.94f : 1.0f, field_7E ? 33 : 25);
        field_7E = (field_7E + 1) % 2;
        break;
    case AE_SCRIPT_SHOOT_CONTROLS:
        if (AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(167), static_cast<eSoundBankSlot>(28))) {
            Bank(3, 2);
        } else if (AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(345), static_cast<eSoundBankSlot>(28))) {
            Bank(4, 2);
        }
        break;
    case AE_SCRIPT_CARGO_PLANE_DOOR_START:
    case AE_SCRIPT_HEAVY_GATE_START:
        Bank(1, 2);
        Bank(0, 2);
        break;
    case AE_SCRIPT_CARGO_PLANE_DOOR_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CARGO_PLANE_DOOR_START, this);
        Bank(2, 2);
        break;
    case AE_SCRIPT_GYM_BIKE_START:
        Bank(0, 3, 1, -18.0f);
        field_8C = 1.0f;
        break;
    case AE_SCRIPT_GYM_BIKE_STOP:
    case AE_SCRIPT_GYM_RUNNING_MACHINE_STOP:
        field_8C = 2.0f;
        break;
    case AE_SCRIPT_GYM_RUNNING_MACHINE_START:
        Bank(1, 3, 1, -18.0f);
        field_8C = 1.0f;
        break;
    case AE_SCRIPT_OTB_WIN:
        Bank(5, 3, 1);
        break;
    case AE_SCRIPT_HEAVY_GATE_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_HEAVY_GATE_START, this);
        Bank(2, 2);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_LIFT_START:
        Bank(2, 2);
        Bank(0, 2);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_LIFT_STOP:
        CancelEvent(AE_SCRIPT_VERTICAL_BIRD_LIFT_START);
        Bank(3, 2);
        break;
    case AE_SCRIPT_PUNCH_PED:
        Resident(5, 143, 58);
        Resident(5, 143, 40);
        break;
    case AE_SCRIPT_AMMUNATION_GUN_COLLISION:
        Resident(2, 39, 33);
        Resident(2, 39, 50, 0.0f, 0.79f, 22);
        break;
    case AE_SCRIPT_CAMERA_SHOT:
        if (physical) {
            AudioEngine.ReportWeaponEvent(AE_WEAPON_FIRE, WEAPON_CAMERA, physical);
        } else {
            Resident(5, 143, 45);
        }
        break;
    case AE_SCRIPT_BUY_CAR_MOD:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_BUY_CAR_MOD);
        break;
    case AE_SCRIPT_BUY_CAR_RESPRAY:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_RESPRAY);
        break;
    case AE_SCRIPT_BASEBALL_BAT_HIT_PED:
        Resident(5, 143, 34, 5.0f);
        Resident(5, 143, 40);
        break;
    case AE_SCRIPT_STAMP_PED:
        Resident(5, 143, 82, -3.0f);
        break;
    case AE_SCRIPT_CHECKPOINT_AMBER:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.12f);
        break;
    case AE_SCRIPT_CHECKPOINT_RED:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.26f);
        break;
    case AE_SCRIPT_CAR_SMASH_GATE:
        Resident(2, 39, CAEAudioUtility::GetRandomNumberInRange<int32>(20, 28));
        Resident(2, 39, 65);
        break;
    case AE_SCRIPT_OTB_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_GARAGE_DOOR_OPENED);
        break;
    case AE_SCRIPT_PED_HIT_WATER_SPLASH:
        if (physical) {
            AudioEngine.ReportWaterSplash(physical, -6.0f, false);
        } else {
            AudioEngine.ReportWaterSplash(posn, -6.0f);
        }
        break;
    case AE_SCRIPT_RESTAURANT_TRAY_COLLISION:
        Resident(2, 39, 19, 0.0f, 1.0f, 65);
        break;
    case AE_SCRIPT_SWEETS_HORN:
        if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
            Resident(17, 74, 7);
            m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        }
        break;
    case AE_SCRIPT_PICKUP_STANDARD:
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_MONEY);
        break;
    case AE_SCRIPT_GARAGE_DOOR_START:
        Garage(AE_GARAGE_DOOR_OPENING, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_GARAGE_DOOR_STOP:
        Garage(AE_GARAGE_DOOR_OPENED, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_MINITANK_FIRE:
        if (physical) {
            AudioEngine.ReportWeaponEvent(AE_WEAPON_FIRE, WEAPON_M4, physical);
        }
        break;
    case AE_SCRIPT_EXPLOSION:
        m_ExplosionAudio.AddAudioEvent(AE_EXPLOSION, physical ? physical->GetPosition() : posn, 0.0f);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_ALARM_STOP:
        CancelEvent(AE_SCRIPT_VERTICAL_BIRD_ALARM_START);
        break;
    case AE_SCRIPT_PED_COLLAPSE:
        Resident(41, 0, 0);
        break;
    case AE_SCRIPT_AIR_HORN:
        {
            CVector left{ -1.0f, 0.0f, 0.0f };
            PlayMissionBankSound(eventId, left, nullptr, 0, 3);
            CVector right{ 1.0f, 0.0f, 0.0f };
            PlayMissionBankSound(eventId, right, nullptr, 1, 3);
        }
        break;
    case AE_SCRIPT_SHUTTER_DOOR_SLOW_START:
        Garage(AE_GARAGE_DOOR_OPENING, 0.0f, 0.79f);
        break;
    case AE_SCRIPT_SHUTTER_DOOR_SLOW_STOP:
        Garage(AE_GARAGE_DOOR_OPENED, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_TEMPEST_PICKUP1:
        Bank(5, 3, 0, 0.0f, 2.0f, 0.67f);
        break;
    case AE_SCRIPT_TEMPEST_PICKUP2:
        Bank(5, 3, 0, 0.0f, 2.0f, 0.79f);
        break;
    case AE_SCRIPT_TEMPEST_WARP:
        Bank(8, 3, 1, 0.0f, 2.0f, 0.38f);
        break;
    case AE_SCRIPT_TEMPEST_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack((eAudioEvents)172);
        break;
    case AE_SCRIPT_DRIVING_AWARD_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_WEAPON_FIRE_PLANE);
        break;
    case AE_SCRIPT_BIKE_AWARD_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_RAIN_COLLISION);
        break;
    case AE_SCRIPT_PILOT_AWARD_TRACK_START:
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack(AE_GARAGE_DOOR_CLOSING);
        break;
    case AE_SCRIPT_PED_DEATH_CRUNCH:
        if (CLocalisation::Blood() && physical && physical->GetIsTypePed()) {
            physical->AsPed()->GetAE().AddAudioEvent(AE_PED_CRUNCH, 0.0f, 1.0f, physical);
        }
        break;
    case (eAudioEvents)1190:
        Resident(5, 143, CAEAudioUtility::GetRandomNumberInRange<int32>(78, 80));
        break;
    default:
        break;
    }
}

// 0x4EE960
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CPhysical* physical, float volume, float speed) {
    CVector posn{ -1000.0f, -1000.0f, -1000.0f };
    ProcessMissionAudioEvent(eventId, posn, physical, volume, speed);
}

// 0x4EE940
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CVector& posn) {
    ProcessMissionAudioEvent(eventId, posn, nullptr);
}

// 0x4EC970
void CAEScriptAudioEntity::UpdateParameters(CAESound* sound, int16 playTime) {
    if (!sound) {
        return;
    }

    // Eases the volume of the sound towards the one of the event, or stops it when it is going down and got there
    const auto UpdateGymMachineVolume = [&](eAudioEvents event) {
        const auto eventVolume = GetDefaultVolume(event);
        if (field_8C == 1.0f) { // Going up
            const auto volume = sound->m_Volume;
            if (!(volume < eventVolume)) {
                return;
            }
            sound->m_Volume = volume + 0.1f < eventVolume ? volume + 0.1f : eventVolume;
        } else if (field_8C == 2.0f) { // Going down
            const auto targetVolume = eventVolume - 18.0f;
            const auto volume       = sound->m_Volume;
            if (volume <= targetVolume) {
                sound->StopSoundAndForget();
                return;
            }
            sound->m_Volume = volume - 0.1f > targetVolume ? volume - 0.1f : targetVolume;
        }
    };

    // The horn like sounds are stopped when they were going for too long
    const auto StopIfPlayedFor = [&](uint32 time) {
        if (CTimer::GetTimeInMS() > m_nLastTimeHornPlayed + time) {
            sound->StopSoundAndForget();
            m_nLastTimeHornPlayed = 0;
        }
    };

    for (auto& link : wavLinks) {
        if (link.m_Sound == sound) {
            if (playTime == -1) { // It has finished
                link.m_Sound = nullptr;
                return;
            }
            if (link.m_pEntity) {
                sound->SetPosition(link.m_pEntity->GetPosition());
            }
            continue;
        }

        switch (sound->m_Event) {
        case AE_CRANE_WINCH_MOVE:
            if (CTimer::GetTimeInMS() > m_nLastTimeHornPlayed + 300) {
                sound->StopSoundAndForget();
                m_nLastTimeHornPlayed = 0;
                CVector posn = sound->m_CurrPos;
                PlayResidentSoundEvent(SND_BANK_SLOT_PLAYER_ENGINE_P, SND_BANK_GENRL_CRANE_P, (eSoundID)2, AE_SCRIPT_CRANE_MOVE_STOP, posn, (CPhysical*)sound->m_PhysicalEntity.Get(), -12.0f, 1.0f, 0, 2.5f);
            } else {
                sound->m_Volume = m_Volume;
                sound->m_Speed  = m_Speed;
            }
            break;
        case AE_SCRIPT_ROULETTE_SPIN: {
            if (CTimer::GetTimeInMS() > m_nLastTimeHornPlayed + 4500) {
                // The spin slows down until it stops
                if (sound->m_Volume <= -40.0f) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                } else {
                    sound->m_Volume -= 0.1f;
                }
                if (sound->m_Speed > 0.0f) {
                    sound->m_Speed = std::max(sound->m_Speed - 0.001f, 0.0f);
                }
            }
            if (CTimer::GetTimeInMS() > m_nLastTimeHornPlayed + 4800 && !field_7C) { // The ball bounces
                CVector posn{ -1000.0f, -1000.0f, -1000.0f };
                PlayMissionBankSound(AE_SCRIPT_ROULETTE_BALL_BOUNCING, posn, nullptr, (int16)CAEAudioUtility::GetRandomNumberInRange<int32>(1, 3), 3, 0, 0.0f, 2.0f, 1.0f);
                field_7C = 1;
            }
            break;
        }
        case AE_SCRIPT_GYM_BIKE_START:
            UpdateGymMachineVolume(AE_SCRIPT_GYM_BIKE_START);
            break;
        case AE_SCRIPT_GYM_RUNNING_MACHINE_START:
            UpdateGymMachineVolume(AE_SCRIPT_GYM_RUNNING_MACHINE_START);
            break;
        case AE_SCRIPT_SWEETS_HORN:
            StopIfPlayedFor(500);
            break;
        case AE_SCRIPT_DUAL_THRUST:
        case AE_SCRIPT_BEE_BUZZ:
        case AE_SCRIPT_TEMPEST_SHIELD_GLOW:
            StopIfPlayedFor(300);
            break;
        default:
            break;
        }
    }
}

// 0x4EC900
void CAEScriptAudioEntity::Service() {
    CVector posn = {-1000.0f, -1000.0f, -1000.0f};
    if (!m_Physical)
        return;
    if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CRANE_ENTER, this) != 0)
        return;

    PlayResidentSoundEvent(SND_BANK_SLOT_PLAYER_ENGINE_P, SND_BANK_GENRL_CRANE_P, 0, AE_SCRIPT_CRANE_ENTER, posn, m_Physical, 0.0f, 1.0f, 0, 2.5f);
}

void CAEScriptAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEScriptAudioEntity, 0x862E58, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Constructor, 0x5074D0);
    RH_ScopedInstall(Initialise, 0x5B9B60);
    RH_ScopedInstall(Service, 0x4EC900);
    RH_ScopedInstall(Reset, 0x4EC150);
    RH_ScopedInstall(GetMissionAudioLoadingStatus, 0x4EBF60);
    RH_ScopedInstall(IsMissionAudioSampleFinished, 0x4EBFE0);
    RH_ScopedInstall(GetMissionAudioEvent, 0x4EC020);
    RH_ScopedInstall(ClearMissionAudio, 0x4EC040);
    RH_ScopedInstall(SetMissionAudioPosition, 0x4EC0C0);
    RH_ScopedInstall(AttachMissionAudioToPhysical, 0x4EC100);
    RH_ScopedInstall(PreloadMissionAudio, 0x4EC190);
    RH_ScopedInstall(PlayLoadedMissionAudio, 0x4EC270);
    RH_ScopedInstall(GetMissionAudioPosition, 0x4EC4D0);
    RH_ScopedInstall(PlayResidentSoundEvent, 0x4EC550);
    RH_ScopedInstall(PlayMissionBankSound, 0x4EC6D0);
    RH_ScopedInstall(ProcessMissionAudioEvent, 0x4ECCF0);
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "1", 0x4EE960, void (CAEScriptAudioEntity::*)(eAudioEvents, CPhysical*, float, float));
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "2", 0x4EE940, void (CAEScriptAudioEntity::*)(eAudioEvents, CVector&));
    RH_ScopedVMTInstall(UpdateParameters, 0x4EC970);
}

CAEScriptAudioEntity* CAEScriptAudioEntity::Constructor() {
    this->CAEScriptAudioEntity::CAEScriptAudioEntity();
    return this;
}
