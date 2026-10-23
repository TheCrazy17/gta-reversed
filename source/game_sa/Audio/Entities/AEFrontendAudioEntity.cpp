#include "StdInc.h"

#include "AEFrontendAudioEntity.h"

#include "AETwinLoopSoundEntity.h"
#include "AEAudioHardware.h"
#include "AESoundManager.h"
#include "AEAudioUtility.h"
#include "AESound.h"

// 0x5B9AB0
void CAEFrontendAudioEntity::Initialise() {
    m_bAmplifierWakeUp = false;
    m_pAmplifierWakeUp = nullptr;

    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_FRONTEND_MENU, SND_BANK_SLOT_FRONTEND_MENU);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_BULLET_PASS_1, SND_BANK_SLOT_BULLET_PASS);

    m_nbLoadingTuneSeed = CAEAudioUtility::GetRandomNumberInRange(0, 3);
    AEAudioHardware.LoadSound(SND_BANK_GENRL_LOADING, 2 * m_nbLoadingTuneSeed + 0, SND_BANK_SLOT_COLLISIONS);
    AEAudioHardware.LoadSound(SND_BANK_GENRL_LOADING, 2 * m_nbLoadingTuneSeed + 1, SND_BANK_SLOT_WEAPON_GEN);
}

// 0x4DD440
void CAEFrontendAudioEntity::Reset() {
    m_nLastFrameGeneral_or_nFrameCount = 0;
    m_nLastFrameMissionComplete        = 0;
    m_nLastFrameBulletPass             = 0;
    m_BulletPassCount                  = 0;
    m_f7E                              = -1;
    AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this);
}

// 0x4DD4A0
void CAEFrontendAudioEntity::AddAudioEvent(eAudioEvents event, float fVolumeBoost, float fSpeed) {
    ZoneScoped;

    constexpr CVector XVECM{ -1.0f, 0.0f, 0.0f };
    constexpr CVector XVECP{ +1.0f, 0.0f, 0.0f };
    constexpr CVector YVECP{ +0.0f, 1.0f, 0.0f };

    // The flags that the sounds use
    constexpr uint32 FLAGS_ROLLED_OFF   = SOUND_FORCED_FRONT | SOUND_ROLLED_OFF | SOUND_IS_DUCKABLE | SOUND_IS_PAUSABLE | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END; // 0x151B
    constexpr uint32 FLAGS_PAUSABLE     = SOUND_FORCED_FRONT | SOUND_IS_DUCKABLE | SOUND_IS_PAUSABLE | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END;                   // 0x111B
    constexpr uint32 FLAGS_UPDATES      = SOUND_FORCED_FRONT | SOUND_IS_DUCKABLE | SOUND_PLAY_PHYSICALLY | SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | SOUND_FRONT_END;               // 0x110F
    constexpr uint32 FLAGS_NOT_FORCED   = SOUND_IS_DUCKABLE | SOUND_IS_PAUSABLE | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END;                                       // 0x11B

    const float volume = GetDefaultVolume(event) + fVolumeBoost;

    const auto IsLoaded = [](eSoundBank bank, eSoundBankSlot slot) {
        return AEAudioHardware.IsSoundBankLoaded(bank, slot);
    };
    const auto Play = [&](eSoundBankSlot slot, eSoundID sfx, const CVector& pos, uint32 flags, float speed, int32 eventId = AE_UNDEFINED, float speedVariance = 0.0f) {
        return AESoundManager.PlaySound({
            .BankSlotID        = slot,
            .SoundID           = sfx,
            .AudioEntity       = this,
            .Pos               = pos,
            .Volume            = volume,
            .RollOffFactor     = 1.0f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = flags,
            .FrequencyVariance = speedVariance,
            .PlayTime          = 0,
            .EventID           = eventId,
        });
    };
    // One sound in each of the sides
    const auto PlayPair = [&](eSoundBankSlot slot, eSoundID sfxLeft, eSoundID sfxRight, uint32 flags, float speed, int32 eventId = AE_UNDEFINED) {
        Play(slot, sfxLeft, XVECM, flags, speed, eventId);
        Play(slot, sfxRight, XVECP, flags, speed, eventId);
    };
    const auto PlayInGameBank = [&](eSoundID sfxLeft, eSoundID sfxRight, uint32 flags, float speed) { // Does nothing if the bank is not there
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME)) {
            PlayPair(SND_BANK_SLOT_FRONTEND_GAME, sfxLeft, sfxRight, flags, speed);
        }
    };
    const auto PlayInMenuBank = [&](eSoundID sfxLeft, eSoundID sfxRight, uint32 flags) {
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_MENU, SND_BANK_SLOT_FRONTEND_MENU)) {
            PlayPair(SND_BANK_SLOT_FRONTEND_MENU, sfxLeft, sfxRight, flags, fSpeed);
        }
    };
    // Only one sound, in the middle
    const auto PlayInGameBankMiddle = [&](eSoundID sfx, uint32 flags, float speed, int32 eventId = AE_UNDEFINED, float speedVariance = 0.0f) {
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, sfx, YVECP, flags, speed, eventId, speedVariance);
        }
    };
    // Not too often, there has to be some frames between them
    const auto PlayInGameBankEveryFewFrames = [&](uint32& lastFrame, eSoundID sfxLeft, eSoundID sfxRight, uint32 flags, float speed) {
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME) && CTimer::GetFrameCounter() >= lastFrame + 5) {
            lastFrame = CTimer::GetFrameCounter();
            PlayPair(SND_BANK_SLOT_FRONTEND_GAME, sfxLeft, sfxRight, flags, speed);
        }
    };

    // The bullets that fly by, from the bank that is being used for them
    const auto BulletPass = [&](const CVector& pos, eAudioEvents bulletEvent) {
        if (!IsLoaded(m_BulletPassBank, SND_BANK_SLOT_BULLET_PASS) || CTimer::GetFrameCounter() < m_nLastFrameBulletPass + 5) {
            return;
        }
        if (m_BulletPassCount > 10) { // The next bank is loaded when nothing is using this one
            if (AESoundManager.AreSoundsPlayingInBankSlot(SND_BANK_SLOT_BULLET_PASS)) {
                return;
            }
            m_BulletPassBank = (eSoundBank)(m_BulletPassBank + 1);
            if (m_BulletPassBank > SND_BANK_GENRL_BULLET_PASS_LAST) {
                m_BulletPassBank = SND_BANK_GENRL_BULLET_PASS_FIRST;
            }
            AEAudioHardware.LoadSoundBank(m_BulletPassBank, SND_BANK_SLOT_BULLET_PASS);
            m_BulletPassCount = 0;
            return;
        }
        m_nLastFrameBulletPass = CTimer::GetFrameCounter();
        Play(SND_BANK_SLOT_BULLET_PASS, (eSoundID)CAEAudioUtility::GetRandomNumberInRange<int32>(0, 2), pos, SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | SOUND_FRONT_END, 1.0f, bulletEvent, 0.03125f);
        m_BulletPassCount++;
    };

    switch (event) {
    case AE_FRONTEND_START:
        PlayInGameBank((eSoundID)25, (eSoundID)26, FLAGS_ROLLED_OFF, fSpeed);
        break;
    case AE_FRONTEND_SELECT:
        PlayInMenuBank((eSoundID)6, (eSoundID)7, FLAGS_ROLLED_OFF);
        break;
    case AE_FRONTEND_BACK:
        PlayInMenuBank((eSoundID)0, (eSoundID)1, FLAGS_ROLLED_OFF);
        break;
    case AE_FRONTEND_HIGHLIGHT:
        PlayInMenuBank((eSoundID)4, (eSoundID)5, FLAGS_PAUSABLE);
        break;
    case AE_FRONTEND_ERROR:
        PlayInMenuBank((eSoundID)2, (eSoundID)3, FLAGS_ROLLED_OFF);
        break;
    case AE_FRONTEND_NOISE_TEST:
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_MENU, SND_BANK_SLOT_FRONTEND_MENU)) {
            Play(SND_BANK_SLOT_FRONTEND_MENU, (eSoundID)8, YVECP, FLAGS_NOT_FORCED, fSpeed);
        }
        break;
    case AE_FRONTEND_PICKUP_WEAPON:
    case AE_FRONTEND_CAR_FIT_BOMB_TIMED:
    case AE_FRONTEND_CAR_FIT_BOMB_BOOBY_TRAPPED:
    case AE_FRONTEND_CAR_FIT_BOMB_REMOTE_CONTROLLED:
    case AE_FRONTEND_PURCHASE_WEAPON:
        PlayInGameBankEveryFewFrames(m_nLastFrameGeneral_or_nFrameCount, (eSoundID)27, (eSoundID)28, FLAGS_PAUSABLE, fSpeed);
        break;
    case AE_FRONTEND_PICKUP_MONEY:
    case AE_FRONTEND_PICKUP_HEALTH:
    case AE_FRONTEND_PICKUP_ADRENALINE:
    case AE_FRONTEND_PICKUP_BODY_ARMOUR:
        PlayInGameBankEveryFewFrames(m_nLastFrameGeneral_or_nFrameCount, (eSoundID)16, (eSoundID)17, FLAGS_PAUSABLE, fSpeed);
        break;
    case AE_FRONTEND_PICKUP_INFO:
    case AE_FRONTEND_DISPLAY_INFO:
        PlayInGameBank((eSoundID)14, (eSoundID)15, FLAGS_ROLLED_OFF, fSpeed);
        break;
    case AE_FRONTEND_PICKUP_DRUGS:
    case AE_FRONTEND_PICKUP_COLLECTABLE1:
    case AE_FRONTEND_PART_MISSION_COMPLETE:
        PlayInGameBankEveryFewFrames(m_nLastFrameMissionComplete, (eSoundID)18, (eSoundID)19, FLAGS_ROLLED_OFF, fSpeed);
        break;
    case AE_FRONTEND_CAR_NO_CASH:
    case AE_FRONTEND_CAR_IS_HOT:
    case AE_FRONTEND_CAR_ALREADY_RIGGED:
        PlayInGameBankEveryFewFrames(m_nLastFrameGeneral_or_nFrameCount, (eSoundID)27, (eSoundID)28, FLAGS_PAUSABLE, 0.841f);
        break;
    case AE_FRONTEND_CAR_RESPRAY:
        if (IsLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN)) {
            // The sound of each side has its own pitch
            const auto leftIsHigher = CAEAudioUtility::ResolveProbability(0.5f);
            Play(SND_BANK_SLOT_WEAPON_GEN, SND_GENRL_WEAPONS_SPRAY_PAINT, XVECM, FLAGS_UPDATES, leftIsHigher ? 1.1892101f : 1.0f, AE_FRONTEND_CAR_RESPRAY);
            Play(SND_BANK_SLOT_WEAPON_GEN, SND_GENRL_WEAPONS_SPRAY_PAINT, XVECP, FLAGS_UPDATES, leftIsHigher ? 1.0f : 1.1892101f, AE_FRONTEND_CAR_RESPRAY);
            m_nLastTimeCarRespray = CTimer::GetTimeInMS();
        }
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_REAR:
        BulletPass({ -0.1f, -1.0f, 0.0f }, AE_FRONTEND_BULLET_PASS_LEFT_REAR);
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_FRONT:
        BulletPass({ -0.1f, +1.0f, 0.0f }, AE_FRONTEND_BULLET_PASS_LEFT_FRONT);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_REAR:
        BulletPass({ +0.1f, -1.0f, 0.0f }, AE_FRONTEND_BULLET_PASS_RIGHT_REAR);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_FRONT:
        BulletPass({ +0.1f, +1.0f, 0.0f }, AE_FRONTEND_BULLET_PASS_RIGHT_FRONT);
        break;
    case AE_FRONTEND_WAKEUP_AMPLIFIER:
        if (!m_bAmplifierWakeUp && IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME)) {
            m_pAmplifierWakeUp = Play(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)8, YVECP, FLAGS_NOT_FORCED | SOUND_REQUEST_UPDATES, fSpeed);
            m_bAmplifierWakeUp = m_pAmplifierWakeUp != nullptr;
        }
        break;
    case AE_FRONTEND_TIMER_COUNT:
        if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_TIMER_COUNT, this)) {
            if (!IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME)) {
                break;
            }
            PlayPair(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)4, (eSoundID)5, FLAGS_UPDATES, fSpeed, AE_FRONTEND_TIMER_COUNT);
        }
        m_nLatestTimerCount = CTimer::GetTimeInMS();
        break;
    case AE_FRONTEND_RADIO_RETUNE_START:
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME)) {
            // The one for when the game is paused is the one that keeps going
            auto& tune = CTimer::GetIsPaused() ? m_objRetunePaused : m_objRetune;
            if (!tune.IsActive()) {
                tune.Initialise(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)2, (eSoundID)1, this, 200, 650, -1, -1);
                tune.PlayTwinLoopSound(YVECP, volume, 1.0f, 1.0f, 1.0f, (eSoundEnvironment)(CTimer::GetIsPaused() ? 0x15B : 0x14B));
            }
        }
        break;
    case AE_FRONTEND_RADIO_RETUNE_STOP:
        if (!CTimer::GetIsPaused()) {
            if (m_objRetune.IsActive()) {
                m_objRetune.StopSoundAndForget();
            }
            break;
        }
        [[fallthrough]];
    case AE_FRONTEND_RADIO_RETUNE_STOP_PAUSED:
        if (m_objRetunePaused.IsActive()) {
            m_objRetunePaused.StopSoundAndForget();
        }
        break;
    case AE_FRONTEND_RADIO_CLICK_ON:
        PlayInGameBankMiddle((eSoundID)23, FLAGS_NOT_FORCED, 1.0f);
        break;
    case AE_FRONTEND_RADIO_CLICK_OFF:
        PlayInGameBankMiddle((eSoundID)23, FLAGS_NOT_FORCED, 0.8909f);
        break;
    case AE_FRONTEND_FIRE_FAIL_SNIPERRIFFLE:
        PlayInGameBankMiddle((eSoundID)10, SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed);
        break;
    case AE_FRONTEND_FIRE_FAIL_ROCKET:
        PlayInGameBankMiddle((eSoundID)11, SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed);
        break;
    case AE_FRONTEND_RACE_321:
        PlayInGameBank((eSoundID)6, (eSoundID)7, FLAGS_ROLLED_OFF, fSpeed);
        break;
    case AE_FRONTEND_RACE_GO:
        PlayInGameBank((eSoundID)12, (eSoundID)13, FLAGS_ROLLED_OFF, fSpeed);
        break;
    case AE_FRONTEND_BUY_CAR_MOD:
        PlayInGameBankMiddle((eSoundID)9, FLAGS_NOT_FORCED, 1.0f, AE_UNDEFINED, 0.0588f);
        break;
    case AE_FRONTEND_SCANNER_NOISE_START:
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME) && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)3, YVECP, SOUND_IS_DUCKABLE | SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed, AE_FRONTEND_SCANNER_NOISE_START);
        }
        break;
    case AE_FRONTEND_SCANNER_NOISE_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this);
        break;
    case AE_FRONTEND_SCANNER_CLICK:
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME) && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_CLICK, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)24, YVECP, SOUND_IS_DUCKABLE | SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed, AE_FRONTEND_SCANNER_CLICK);
        }
        break;
    case AE_FRONTEND_LOADING_TUNE_START:
        if (   AEAudioHardware.IsSoundLoaded(SND_BANK_GENRL_LOADING, (eSoundID)(2 * m_nbLoadingTuneSeed), SND_BANK_SLOT_COLLISIONS)
            && AEAudioHardware.IsSoundLoaded(SND_BANK_GENRL_LOADING, (eSoundID)(2 * m_nbLoadingTuneSeed + 1), SND_BANK_SLOT_WEAPON_GEN)
            && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this)
        ) {
            Play(SND_BANK_SLOT_COLLISIONS, (eSoundID)(2 * m_nbLoadingTuneSeed), XVECM, FLAGS_PAUSABLE, fSpeed, AE_FRONTEND_LOADING_TUNE_START);
            Play(SND_BANK_SLOT_WEAPON_GEN, (eSoundID)(2 * m_nbLoadingTuneSeed + 1), XVECP, FLAGS_PAUSABLE, fSpeed, AE_FRONTEND_LOADING_TUNE_START);
        }
        break;
    case AE_FRONTEND_LOADING_TUNE_STOP:
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this);
        break;
    case AE_MISSILE_LOCK:
        if (IsLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME) && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_MISSILE_LOCK, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, (eSoundID)13, YVECP, SOUND_IS_DUCKABLE | SOUND_FRONT_END, fSpeed, AE_MISSILE_LOCK);
        }
        break;
    default:
        break;
    }
}

// 0x4DD480
bool CAEFrontendAudioEntity::IsRadioTuneSoundActive() {
    return CTimer::GetIsPaused()
        ? m_objRetunePaused.IsActive()
        : m_objRetune.IsActive();
}

// 0x4DD470
bool CAEFrontendAudioEntity::IsLoadingTuneActive() {
    return AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this);
}

// 0x4DEDA0
void CAEFrontendAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (!sound)
        return;

    const float curPos = (float)curPlayPos / 350.f + (float)curPlayPos / 350.f;
    const float rear = curPos - 1.0f;
    const float front = 1.0f - curPos;

    const auto BulletSound = [=](const float x, const float y) {
        if (curPlayPos >= 0 && curPlayPos <= 350) {
            sound->SetPosition({ x, y, 0.0f });
        }
    };

    switch (sound->m_Event) {
    case AE_FRONTEND_CAR_RESPRAY:
        if (curPlayPos > 0 && CTimer::GetTimeInMS() > m_nLastTimeCarRespray + 1900) {
            sound->StopSoundAndForget();
        }
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_REAR:
        BulletSound(-0.1f, rear);
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_FRONT:
        BulletSound(-0.1f, front);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_REAR:
        BulletSound(+0.1f, rear);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_FRONT:
        BulletSound(+0.1f, front);
        break;
    case AE_FRONTEND_TIMER_COUNT:
        if (curPlayPos > 0 && CTimer::GetTimeInMS() > m_nLatestTimerCount + 100) {
            sound->StopSoundAndForget();
        }
        break;
    default:
        break;
    }

    if (sound == m_pAmplifierWakeUp && curPlayPos == -1) {
        m_pAmplifierWakeUp = nullptr;
        m_bAmplifierWakeUp = false;
    }
}

void CAEFrontendAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEFrontendAudioEntity, 0x862E54, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Initialise, 0x5B9AB0);
    RH_ScopedInstall(Reset, 0x4DD440);
    RH_ScopedInstall(AddAudioEvent, 0x4DD4A0);
    RH_ScopedInstall(IsRadioTuneSoundActive, 0x4DD480);
    RH_ScopedInstall(IsLoadingTuneActive, 0x4DD470);
    RH_ScopedVMTInstall(UpdateParameters, 0x4DEDA0);
}
