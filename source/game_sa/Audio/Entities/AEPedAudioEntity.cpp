#include "StdInc.h"

#include "AEPedAudioEntity.h"
#include "AEAudioHardware.h"
#include "AEAudioUtility.h"

void CAEPedAudioEntity::InjectHooks() {
    RH_ScopedClass(CAEPedAudioEntity);
    RH_ScopedCategory("Audio/Entities");

    //RH_ScopedInstall(Constructor, 0x5DE8D0, { .Reversed = false });

    RH_ScopedInstall(Initialise, 0x4E0E80);
    RH_ScopedInstall(StaticInitialise, 0x5B98A0);
    RH_ScopedInstall(Terminate, 0x4E1360);
    RH_ScopedInstall(AddAudioEvent, 0x4E2BB0);
    RH_ScopedInstall(TurnOnJetPack, 0x4E28A0);
    RH_ScopedInstall(TurnOffJetPack, 0x4E2A70);
    RH_ScopedInstall(StopJetPackSound, 0x4E1120);
    RH_ScopedInstall(UpdateJetPack, 0x4E0EE0);
    RH_ScopedInstall(PlayWindRush, 0x4E1170);
    RH_ScopedInstall(UpdateParameters, 0x4E1180);
    RH_ScopedInstall(HandleFootstepEvent, 0x4E13A0);
    RH_ScopedInstall(HandleSkateEvent, 0x4E17E0);
    RH_ScopedInstall(HandleLandingEvent, 0x4E18E0);
    RH_ScopedInstall(HandlePedSwing, 0x4E1A40);
    RH_ScopedInstall(HandlePedHit, 0x4E1CC0);
    RH_ScopedInstall(HandlePedJacked, 0x4E2350);
    RH_ScopedInstall(HandleSwimSplash, 0x4E26A0);
    RH_ScopedInstall(HandleSwimWake, 0x4E2790);
    RH_ScopedInstall(PlayShirtFlap, 0x4E2A90);
    RH_ScopedInstall(Service, 0x4E2EE0);
}

// 0x5DE8D0
CAEPedAudioEntity::CAEPedAudioEntity() : CAEAudioEntity() {
    m_pPed = nullptr;
    m_bCanAddEvent = false;

    m_JetPackSound0 = nullptr;
    m_JetPackSound1 = nullptr;
    m_JetPackSound2 = nullptr;
}

// (CEntity* entity)
// 0x4E0E80
void CAEPedAudioEntity::Initialise(CPed* ped) {
    m_pPed = ped;
    m_nSfxId = 0;
    m_LastSwimWakeTriggerTimeMs = 0;

    m_bJetPackPlaying = false;
    m_JetPackSound0 = nullptr;
    m_JetPackSound1 = nullptr;
    m_fVolume1 = -100.0f;
    m_fVolume2 = -100.0f;

    field_150 = nullptr;
    field_154 = -100.0f;
    field_158 = -100.0f;
    m_bCanAddEvent = true;
}

// 0x5B98A0
void CAEPedAudioEntity::StaticInitialise() {
    AEAudioHardware.LoadSoundBank(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING);
}

// 0x4E1360
void CAEPedAudioEntity::Terminate() {
    m_bCanAddEvent = false;
    m_pPed   = nullptr;
    StopJetPackSound();
    AESoundManager.CancelSoundsOwnedByAudioEntity(this, true);
    if (m_sTwinLoopSoundEntity.IsActive()) {
        m_sTwinLoopSoundEntity.StopSoundAndForget();
    }
}

// 0x4E2BB0
void CAEPedAudioEntity::AddAudioEvent(eAudioEvents event, float volume, float speed, CPhysical* ped, eSurfaceType surfaceId, int32 a7, uint32 maxVol) {
    if (!m_bCanAddEvent)
        return;

    if (!m_pPed)
        return;

    switch (event) {
    case AE_PED_FOOTSTEP_LEFT:
    case AE_PED_FOOTSTEP_RIGHT:
        HandleFootstepEvent(event, volume, speed, surfaceId);
        break;
    case AE_PED_SKATE_LEFT:
    case AE_PED_SKATE_RIGHT:
        HandleSkateEvent(event, volume, speed);
        break;
    case AE_PED_LAND_ON_FEET_AFTER_FALL:
    case AE_PED_COLLAPSE_AFTER_FALL:
        HandleLandingEvent(event);
        break;
    case AE_PED_SWING:
        HandlePedSwing(event, a7, maxVol);
        break;
    case AE_PED_HIT_HIGH:
    case AE_PED_HIT_LOW:
    case AE_PED_HIT_GROUND:
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
        HandlePedHit(event, ped, surfaceId, volume, maxVol);
        break;
    case AE_PED_JACKED_CAR_PUNCH:
    case AE_PED_JACKED_CAR_HEAD_BANG:
    case AE_PED_JACKED_CAR_KICK:
    case AE_PED_JACKED_BIKE:
    case AE_PED_JACKED_DOZER:
        HandlePedJacked(event);
        break;
    case AE_PED_SWIM_STROKE_SPLASH:
    case AE_PED_SWIM_DIVE_SPLASH:
        HandleSwimSplash(event);
        break;
    case AE_PED_SWIM_WAKE:
        HandleSwimWake(event);
        break;
    case AE_PED_CRUNCH: {
        if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_COLLISIONS, SND_BANK_SLOT_COLLISIONS)) {
            break;
        }

        volume += GetDefaultVolume(AE_PED_CRUNCH);

        if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_PED_CRUNCH, this)) {
            break;
        }
        AESoundManager.PlaySound({
            .BankSlotID        = SND_BANK_SLOT_COLLISIONS,
            .SoundID           = 29,
            .AudioEntity       = this,
            .Pos               = ped->GetPosition(),
            .Volume            = volume,
            .RollOffFactor     = 1.5f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = SOUND_DEFAULT,
            .FrequencyVariance = 0.06f,
            .PlayTime          = 0,
            .EventID           = AE_PED_CRUNCH
        });

        if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_PED_KNOCK_DOWN, this) != 0) {
            break;
        }
        AESoundManager.PlaySound({
            .BankSlotID        = SND_BANK_SLOT_COLLISIONS,
            .SoundID           = (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(47, 49)),
            .AudioEntity       = this,
            .Pos               = ped->GetPosition(),
            .Volume            = volume,
            .RollOffFactor     = 1.5f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = SOUND_DEFAULT,
            .FrequencyVariance = 0.06f,
            .PlayTime          = 0,
            .EventID           = AE_PED_KNOCK_DOWN
        });
        break;
    }
    default:
        break;
    }
}

// 0x4E28A0
void CAEPedAudioEntity::TurnOnJetPack() {
    if (!m_pPed)
        return;

    if (m_bJetPackPlaying || m_JetPackSound1 || m_JetPackSound0 || m_JetPackSound2)
        return;

    m_fVolume1 = -100.0f;
    m_fVolume2 = -100.0f;
    m_fVolume3 = +2.000f;
    m_JetPackSoundSpeedMult = 0.400f;

    m_bJetPackPlaying = true;

    const auto PlayJetPackSound = [&](eSoundBankSlot slot, eSoundID soundID) {
        return AESoundManager.PlaySound({
            .BankSlotID  = slot,
            .SoundID     = soundID,
            .AudioEntity = this,
            .Pos         = m_pPed->GetPosition(),
            .Volume      = -100.f,
            .Flags       = SOUND_REQUEST_UPDATES,
        });
    };
    m_JetPackSound0 = PlayJetPackSound(SND_BANK_SLOT_VEHICLE_GEN, 26);
    m_JetPackSound1 = PlayJetPackSound(SND_BANK_SLOT_WEAPON_GEN, 10);
    m_JetPackSound2 = PlayJetPackSound(SND_BANK_SLOT_FRONTEND_GAME, 0);
}

// 0x4E2A70
void CAEPedAudioEntity::TurnOffJetPack() {
    StopJetPackSound();
    m_bJetPackPlaying = false;
}

// 0x4E1120
void CAEPedAudioEntity::StopJetPackSound() {
    if (m_JetPackSound0) {
        m_JetPackSound0->StopSoundAndForget();
        m_JetPackSound0 = nullptr;
    }

    if (m_JetPackSound1) {
        m_JetPackSound1->StopSoundAndForget();
        m_JetPackSound1 = nullptr;
    }

    if (m_JetPackSound2) {
        m_JetPackSound2->StopSoundAndForget();
        m_JetPackSound2 = nullptr;
    }
}

// 0x4E0EE0
void CAEPedAudioEntity::UpdateJetPack(float thrustFwd, float thrustAngle) {
    if (!m_bJetPackPlaying || !m_JetPackSound1 || !m_JetPackSound0 || !m_JetPackSound2) {
        return;
    }

    if (thrustFwd <= 0.5f) { // flying
        m_fVolume1 = std::max(m_fVolume1 - 5.0f, -100.0f);
        m_fVolume2 = std::min(m_fVolume2 + 6.0f, -17.0f);
        m_fVolume3 = std::max(m_fVolume3 - 0.3f, 2.0f);
        m_JetPackSoundSpeedMult = std::max(m_JetPackSoundSpeedMult - 0.031f, 0.4f);
    } else { // idle
        m_fVolume1 = std::min(m_fVolume1 + 15.0f, -15.0f);
        m_fVolume2 = std::max(m_fVolume2 - 7.1f, -100.0f);
        m_fVolume3 = std::min(m_fVolume3 + 0.3f, 11.0f);
        m_JetPackSoundSpeedMult = std::min(m_JetPackSoundSpeedMult + 0.031f, 0.71f);
    }

    const auto angle = std::sin(thrustAngle);
    const float speed = angle < 0.0f ? -angle : angle; // maybe wrong

    // 0.0f == 0xB61384 (uninitialized)
    m_JetPackSound0->m_Volume = m_fVolume1 + 0.0f;
    m_JetPackSound0->m_Speed = speed * -0.07f + 1.0f;

    m_JetPackSound1->m_Volume = m_fVolume2 + 0.0f;
    m_JetPackSound1->m_Speed = 0.56f;

    m_JetPackSound2->m_Volume = m_fVolume3 + 0.0f;
    m_JetPackSound2->m_Speed = (speed / 5.0f + 1.0f) * m_JetPackSoundSpeedMult;
}

// 0x4E1170
void CAEPedAudioEntity::PlayWindRush(float, float) {
    // NOP
}

// 0x4E1180
void CAEPedAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (sound == m_JetPackSound1) {
        if (curPlayPos == -1) {
            m_JetPackSound1 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    if (sound == m_JetPackSound0) {
        if (curPlayPos == -1) {
            m_JetPackSound0 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    if (sound == m_JetPackSound2) {
        if (curPlayPos == -1) {
            m_JetPackSound2 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    // shit
    if (sound == field_150) {
        if (curPlayPos == -1) {
            field_150 = nullptr;
        }
        return;
    }

    if (curPlayPos < 0)
        return;

    switch (sound->m_Event) {
    case AE_PED_SWING:
    case AE_PED_HIT_HIGH:
    case AE_PED_HIT_LOW:
    case AE_PED_HIT_GROUND:
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
    case AE_PED_JACKED_CAR_PUNCH:
    case AE_PED_JACKED_CAR_HEAD_BANG:
    case AE_PED_JACKED_CAR_KICK:
    case AE_PED_JACKED_BIKE:
    case AE_PED_JACKED_DOZER:
        if (CTimer::GetTimeInMS() < (uint32)sound->m_ClientVariable)
            return;
        sound->m_Speed = 1.0f;
        return;
    case AE_PED_SWIM_WAKE: {
        const auto volume = GetDefaultVolume(AE_PED_SWIM_WAKE);

        if (CTimer::GetTimeInMS() <= m_LastSwimWakeTriggerTimeMs + 100) {
            if (sound->m_Volume >= volume) {
                return;
            }

            if (sound->m_Volume + 0.6f >= volume) {
                sound->m_Volume = volume;
                return;
            }
            sound->m_Volume += 0.6f;
        } else {
            auto vol = volume - 20.0f;
            if (sound->m_Volume <= vol) {
                sound->StopSoundAndForget();
                m_LastSwimWakeTriggerTimeMs = 0;
                return;
            }
            sound->m_Volume = std::max(sound->m_Volume - 0.6f, vol); // TODO: Use TimeStep
        }

        return;
    }
    default:
        return;
    }
}

// 0x4E13A0
void CAEPedAudioEntity::HandleFootstepEvent(eAudioEvents event, float volume, float speed, eSurfaceType forcedSurfaceType) {
    volume += GetDefaultVolume(event);

    if (m_pPed->bIsInTheAir) {
        return;
    }
    volume += (float)(CAEAudioUtility::GetRandomNumberInRange(-3, 3));

    const auto PlayFootstepSound = [&] (eSoundBank bank, eSoundBankSlot slot, eSoundID sfx, int16 playTime = 0) {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(bank, slot)) {
            AESoundManager.PlaySound({
                .BankSlotID         = slot,
                .SoundID            = sfx,
                .AudioEntity        = this,
                .Pos                = m_pPed->GetPosition(),
                .Volume             = volume,
                .Speed              = speed,
                .Flags              = SOUND_START_PERCENTAGE,
                .FrequencyVariance  = 0.0588f,
                .PlayTime           = playTime,
                .RegisterWithEntity = m_pPed,
            });
        }
    };

    const auto PlayRandomGenericFootstepSound = [&] () {
        PlayFootstepSound(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(1, 5)));
    };

    if (FindPlayerPed(-1) != m_pPed) {
        volume -= 3.f;
        PlayRandomGenericFootstepSound();
    } else {
        const auto TryPlayPlayerFootstepSound = [&] (eSoundBank bank, eSoundBankSlot slot, eSoundID sfx, bool needCancelSoundsInSlot = false, int16 playTime = 0) {
            if (AEAudioHardware.IsSoundBankLoaded(bank, slot)) {
                PlayFootstepSound(bank, slot, sfx, playTime);
                return true;
            } else {
                if (needCancelSoundsInSlot) {
                    if (AESoundManager.AreSoundsPlayingInBankSlot(slot) - 1 <= 1) {
                        AESoundManager.CancelSoundsInBankSlot(slot, false);
                    }
                }
                AEAudioHardware.LoadSoundBank(bank, slot);
                PlayRandomGenericFootstepSound();
                return false;
            }
        };
        if (g_surfaceInfos.IsAudioWater(forcedSurfaceType) || g_surfaceInfos.IsAudioWater(m_pPed->m_nContactSurface)) { // 0x4E1703
            TryPlayPlayerFootstepSound(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), false, 50);
        } else if (g_surfaceInfos.IsAudioConcrete(m_pPed->m_nContactSurface)) { // 0x4E145E
            PlayRandomGenericFootstepSound();
        } else if (g_surfaceInfos.IsAudioGrass(m_pPed->m_nContactSurface) || g_surfaceInfos.IsAudioLongGrass(m_pPed->m_nContactSurface)) { // 0x4E16BA
            TryPlayPlayerFootstepSound(SND_BANK_FEET_GRASS, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioSand(m_pPed->m_nContactSurface)) { // 0x4E14B7
            TryPlayPlayerFootstepSound(SND_BANK_FEET_SAND, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 3)), true);
        } else if (g_surfaceInfos.IsAudioGravel(m_pPed->m_nContactSurface)) { // 0x4E1531
            TryPlayPlayerFootstepSound(SND_BANK_FEET_GRAVEL, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioWood(m_pPed->m_nContactSurface)) { // 0x4E157D
            TryPlayPlayerFootstepSound(SND_BANK_FEET_WOOD, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioTile(m_pPed->m_nContactSurface)) { // 0x4E15DB
            TryPlayPlayerFootstepSound(SND_BANK_FEET_TILE, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioMetal(m_pPed->m_nContactSurface)) { // 0x4E1636
            TryPlayPlayerFootstepSound(SND_BANK_FEET_METAL, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else { // 0x4E168F
            PlayRandomGenericFootstepSound();
        }
    }
}

// 0x4E17E0
void CAEPedAudioEntity::HandleSkateEvent(eAudioEvents event, float volume, float speed) {
    if (m_pPed->bIsInTheAir) {
        return;
    }
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
        return;
    }
    AESoundManager.PlaySound({
        .BankSlotID         = SND_BANK_SLOT_FOOTSTEPS_GENERIC,
        .SoundID            = (eSoundID)(event == AE_PED_SKATE_LEFT ? 7 : 8),
        .AudioEntity        = this,
        .Pos                = m_pPed->GetPosition(),
        .Volume             = GetDefaultVolume(event) + (float)(CAEAudioUtility::GetRandomNumberInRange(-3, 3)) + volume,
        .Speed              = speed,
        .FrequencyVariance  = 0.0588f,
        .RegisterWithEntity = m_pPed,
    });
}

// 0x4E18E0
void CAEPedAudioEntity::HandleLandingEvent(eAudioEvents event) {
    if (m_pPed->bIsInTheAir) {
        return;
    }

    const auto PlayLandingSound = [&](eSoundBankSlot slot, eSoundID soundID, float volume, int16 playPos) {
        AESoundManager.PlaySound({
            .BankSlotID = slot,
            .SoundID            = soundID,
            .AudioEntity        = this,
            .Pos                = m_pPed->GetPosition(),
            .Volume             = volume,
            .Flags              = SOUND_START_PERCENTAGE,
            .FrequencyVariance  = 0.0588f,
            .PlayTime           = playPos,
            .RegisterWithEntity = m_pPed
        });
    };
    const auto volume = GetDefaultVolume(event);
    if (g_surfaceInfos.IsAudioWater(m_pPed->m_nContactSurface)) {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING)) {
            PlayLandingSound(
                SND_BANK_SLOT_SWIMMING,
                CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_SWIMMING_SWIM1, SND_GENRL_SWIMMING_SWIM5),
                std::max(0.f, volume),
                50
            );
        }
    } else {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
            PlayLandingSound(
                SND_BANK_SLOT_FOOTSTEPS_GENERIC,
                event == AE_PED_LAND_ON_FEET_AFTER_FALL ? 6 : 0,
                volume,
                0
            );
        }
    }
}

// 0x4E1A40
void CAEPedAudioEntity::HandlePedSwing(eAudioEvents event, int32 a3, uint32 volume) {
    const auto& weapon = m_pPed->GetActiveWeapon();

    if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN)) {
        if (!AudioEngine.IsLoadingTuneActive()) {
            AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN);
        }
        return;
    }

    eSoundID soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
    float    pitch   = 0.84f;
    float    volOff  = -6.0f;
    switch (weapon.m_Type) {
    case WEAPON_GOLFCLUB:
    case WEAPON_CANE:
        soundId = SND_GENRL_WEAPONS_CANE_SWISH;
        pitch   = 1.0f;
        volOff  = 0.0f;
        break;
    case WEAPON_NIGHTSTICK:
    case WEAPON_POOL_CUE:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        pitch   = 0.84f;
        volOff  = 0.0f;
        break;
    case WEAPON_KNIFE:
    case WEAPON_KATANA:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE_METAL;
        pitch   = 1.0f;
        volOff  = -3.0f;
        break;
    case WEAPON_BASEBALLBAT:
    case WEAPON_SHOVEL:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        pitch   = 0.67f;
        volOff  = 0.0f;
        break;
    case WEAPON_DILDO1:
    case WEAPON_DILDO2:
    case WEAPON_VIBE1:
    case WEAPON_VIBE2:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        pitch   = 0.84f;
        volOff  = -6.0f;
        break;
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
    case WEAPON_FREEFALL_BOMB:
    case WEAPON_RLAUNCHER:
    case WEAPON_RLAUNCHER_HS:
        soundId = SND_GENRL_WEAPONS_UNK; // No dedicated swing sound
        break;
    default:
        break;
    }

    switch (a3) {
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
        pitch   = 0.84f;
        volOff  = -6.0f;
        break;
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
        pitch   = 0.84f;
        volOff  = -2.0f;
        break;
    default:
        if (soundId == SND_GENRL_WEAPONS_UNK) {
            return;
        }
        break;
    }

    m_tempSound.Initialise(
        SND_BANK_SLOT_WEAPON_GEN,
        soundId,
        this,
        m_pPed->GetPosition(),
        GetDefaultVolume(event) + volOff,
        1.0f, // rollOff
        pitch,
        1.0f, // doppler
        0,    // frameDelay
        SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
        0.04f, // speedVariance
        0      // playTime
    );
    m_tempSound.RegisterWithPhysicalEntity(m_pPed);

    if (volume > 0) {
        // Schedule playback: store the target time (with uint wrap-around correction) in `m_ClientVariable`
        m_tempSound.m_Speed         = 0.0f;
        m_tempSound.m_SpeedVariance = 0.0f;
        int32 targetTime            = (int32)CTimer::GetTimeInMS() + (int32)volume;
        float  clientVar            = (float)targetTime;
        if (targetTime < 0) {
            clientVar += 4294967296.0f;
        }
        m_tempSound.m_ClientVariable = clientVar;
        m_tempSound.m_Event          = event;
        m_tempSound.SetFlags(SOUND_REQUEST_UPDATES, 1);
    }

    AESoundManager.RequestNewSound(&m_tempSound);
}

namespace {
constexpr auto HIT_SPEED_VARIANCE = 0.0588f;

struct WeaponHitSfx {
    int32 sfx;      // -1 when the weapon has no hit sound
    float variance; // random speed variance of the sound
};

// Sound that is played by the weapon of the attacker when it hits something
WeaponHitSfx GetWeaponHitSfx(eWeaponType type) {
    if ((uint32)type > WEAPON_INFRARED) {
        return { -1, HIT_SPEED_VARIANCE };
    }
    switch (type) {
    case WEAPON_UNARMED:
    case WEAPON_NIGHTVISION:
    case WEAPON_INFRARED:    return { 0x3A, HIT_SPEED_VARIANCE };
    case WEAPON_GOLFCLUB:    return { 0x30, HIT_SPEED_VARIANCE };
    case WEAPON_NIGHTSTICK:  return { 0x23, HIT_SPEED_VARIANCE };
    case WEAPON_KNIFE:       return { 0x39, 0.03f };
    case WEAPON_BASEBALLBAT: return { 0x22, HIT_SPEED_VARIANCE };
    case WEAPON_SHOVEL:      return { 0x4B, HIT_SPEED_VARIANCE };
    case WEAPON_POOL_CUE:    return { 0x43, HIT_SPEED_VARIANCE };
    case WEAPON_KATANA:      return { 0x38, HIT_SPEED_VARIANCE };
    case WEAPON_DILDO1:
    case WEAPON_DILDO2:
    case WEAPON_VIBE1:
    case WEAPON_VIBE2:       return { 0x32, HIT_SPEED_VARIANCE };
    case WEAPON_FLOWERS:     return { 0x36, 0.07f };
    case WEAPON_CANE:        return { 0x2E, HIT_SPEED_VARIANCE };
    case WEAPON_CHAINSAW:
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
    case WEAPON_FREEFALL_BOMB:
    case WEAPON_RLAUNCHER:
    case WEAPON_RLAUNCHER_HS: return { -1, HIT_SPEED_VARIANCE };
    default:                 return { 0x2C, HIT_SPEED_VARIANCE }; // Brass knuckles, thrown and fired weapons
    }
}

// Volume (in dB) of a hit on the surface, 0x4DA510 computes it from a table of percentages
float GetSurfaceHitVolume(uint8 surfaceId) {
    struct SurfaceHitInfo {
        int32 unk0;
        int32 volumePercent;
        int32 unk8;
        int32 unkC;
    };
    const auto& info = StaticRef<SurfaceHitInfo[256]>(0x8AD910)[surfaceId];
    return std::log10((float)info.volumePercent * 0.01f) * 20.0f;
}
} // namespace

// 0x4E1CC0
void CAEPedAudioEntity::HandlePedHit(eAudioEvents event, CPhysical* physical, uint8 surfaceId, float volume, uint32 maxVol) {
    if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN)) {
        if (!AudioEngine.IsLoadingTuneActive()) {
            AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN);
        }
        return;
    }

    const auto baseVolume = GetDefaultVolume(event) + volume;
    const auto weaponType = m_pPed->GetActiveWeapon().m_Type;

    // The first sound comes from the weapon, the second one is the impact itself
    const auto weaponSfx     = GetWeaponHitSfx(weaponType);
    auto       sfxA          = weaponSfx.sfx;
    auto       volumeOffsetA = 0.0f;
    auto       sfxB          = -1;
    auto       slotB         = SND_BANK_SLOT_WEAPON_GEN;
    auto       volumeOffsetB = -3.0f;

    // The chainsaw only plays a sound from the bullet hits bank, that has to be loaded
    const auto UseChainsawHit = [&] {
        if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_BULLET_HITS, SND_BANK_SLOT_BULLET_HITS)) {
            return false;
        }
        sfxA  = -1;
        slotB = SND_BANK_SLOT_BULLET_HITS;
        sfxB  = CAEAudioUtility::GetRandomNumberInRange(7, 9);
        return true;
    };
    // Blades only play the cutting sound
    const auto UseBladeHit = [&] {
        sfxA          = -1;
        sfxB          = 0x51;
        volumeOffsetB = -9.0f;
    };

    if (physical && physical->GetIsTypePed()) {
        switch (event) {
        case AE_PED_HIT_HIGH:
        case AE_PED_HIT_LOW: {
            const auto high = event == AE_PED_HIT_HIGH;
            const auto Hit  = [&](float offset) {
                volumeOffsetA = offset;
                sfxB          = high
                    ? CAEAudioUtility::GetRandomNumberInRange(0x28, 0x2B)
                    : CAEAudioUtility::GetRandomNumberInRange(0x24, 0x27);
            };
            switch (weaponType) {
            case WEAPON_BRASSKNUCKLE: Hit(1.0f); break;
            case WEAPON_BASEBALLBAT:  Hit(-10.0f); break;
            case WEAPON_KNIFE:
            case WEAPON_KATANA:       UseBladeHit(); break;
            case WEAPON_CHAINSAW:
                if (!UseChainsawHit()) {
                    return;
                }
                break;
            case WEAPON_FLOWERS:      break;
            default:                  Hit(-15.0f); break;
            }
            break;
        }
        case AE_PED_HIT_GROUND:
            switch (weaponType) {
            case WEAPON_KNIFE:
            case WEAPON_KATANA:
                UseBladeHit();
                break;
            case WEAPON_BASEBALLBAT:
                volumeOffsetA = -10.0f;
                sfxB          = 0x52;
                volumeOffsetB = -6.0f;
                break;
            case WEAPON_CHAINSAW:
                if (!UseChainsawHit()) {
                    return;
                }
                break;
            case WEAPON_FLOWERS:
                break;
            default:
                volumeOffsetA = -15.0f;
                sfxB          = 0x52;
                volumeOffsetB = -6.0f;
                break;
            }
            break;
        case AE_PED_HIT_GROUND_KICK:
            sfxA          = -1;
            sfxB          = 0x52;
            volumeOffsetB = -6.0f;
            break;
        case AE_PED_HIT_HIGH_UNARMED:
        case AE_PED_HIT_MARTIAL_PUNCH:
            sfxA = 0x3A;
            sfxB = CAEAudioUtility::GetRandomNumberInRange(0x28, 0x2B);
            break;
        case AE_PED_HIT_LOW_UNARMED:
            sfxA = 0x3A;
            sfxB = CAEAudioUtility::GetRandomNumberInRange(0x24, 0x27);
            break;
        case AE_PED_HIT_MARTIAL_KICK:
            sfxA = -1;
            sfxB = CAEAudioUtility::GetRandomNumberInRange(0x3B, 0x3E);
            break;
        default:
            break;
        }
    } else {
        // Hit on something that is not a ped, the collision audio plays the impact
        const auto ReportCollision = [&] {
            AudioEngine.ReportCollision(m_pPed, physical, SURFACE_PED, (eSurfaceType)surfaceId, m_pPed->GetPosition(), nullptr, 1.0f, 1.0f, true, false);
        };
        switch (event) {
        case AE_PED_HIT_HIGH:
        case AE_PED_HIT_LOW:
        case AE_PED_HIT_GROUND:
            volumeOffsetA = GetSurfaceHitVolume(surfaceId);
            ReportCollision();
            break;
        case AE_PED_HIT_HIGH_UNARMED:
        case AE_PED_HIT_LOW_UNARMED:
        case AE_PED_HIT_MARTIAL_PUNCH:
            sfxA          = 0x3A;
            volumeOffsetA = GetSurfaceHitVolume(surfaceId);
            ReportCollision();
            break;
        case AE_PED_HIT_GROUND_KICK:
        case AE_PED_HIT_MARTIAL_KICK:
            ReportCollision();
            return;
        default:
            break;
        }
    }

    const auto PlayHit = [&](eSoundBankSlot slot, int32 sfx, float volumeOffset, float variance) {
        m_tempSound.Initialise(
            slot,
            (eSoundID)sfx,
            this,
            m_pPed->GetPosition(),
            baseVolume + volumeOffset,
            1.0f, // rollOff
            1.0f, // speed
            1.0f, // doppler
            0,    // frameDelay
            SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
            variance,
            0     // playTime
        );
        m_tempSound.RegisterWithPhysicalEntity(m_pPed);

        if (maxVol > 0) {
            // Schedule playback: store the target time (with uint wrap-around correction) in `m_ClientVariable`
            m_tempSound.m_Speed         = 0.0f;
            m_tempSound.m_SpeedVariance = 0.0f;
            const auto targetTime       = (int32)CTimer::GetTimeInMS() + (int32)maxVol;
            auto       clientVar        = (float)targetTime;
            if (targetTime < 0) {
                clientVar += 4294967296.0f;
            }
            m_tempSound.m_ClientVariable = clientVar;
            m_tempSound.m_Event          = event;
            m_tempSound.SetFlags(SOUND_REQUEST_UPDATES, 1);
        }

        AESoundManager.RequestNewSound(&m_tempSound);
    };

    if ((int16)sfxA >= 0) {
        PlayHit(SND_BANK_SLOT_WEAPON_GEN, sfxA, volumeOffsetA, weaponSfx.variance);
    }
    if ((int16)sfxB >= 0) {
        PlayHit(slotB, sfxB, volumeOffsetB, HIT_SPEED_VARIANCE);
    }
}

// 0x4E2350
void CAEPedAudioEntity::HandlePedJacked(eAudioEvents event) {
    if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN)) {
        if (!AudioEngine.IsLoadingTuneActive()) {
            AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN);
        }
        return;
    }

    const auto volume = GetDefaultVolume(event);
    const auto now    = CTimer::GetTimeInMS();

    // The sounds start silent, `UpdateParameters` starts them once their time (stored in the client variable) came
    const auto PlayAt = [&](eSoundBankSlot bankSlot, eSoundID sfxId, uint32 startTime) {
        m_tempSound.Initialise(
            bankSlot,
            sfxId,
            this,
            m_pPed->GetPosition(),
            volume,
            2.0f, // rollOff
            0.0f, // speed
            1.0f, // doppler
            0,    // frameDelay
            SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY | SOUND_REQUEST_UPDATES,
            0.0f, // speedVariance
            0     // playTime
        );
        m_tempSound.RegisterWithPhysicalEntity(m_pPed);
        m_tempSound.m_Event          = event;
        m_tempSound.m_ClientVariable = (float)startTime;
        AESoundManager.RequestNewSound(&m_tempSound);
    };

    auto lastSoundTime = now;
    {
        // Every kind of jacking has its own set of sounds and timing
        int32  firstSfx{ -1 };
        int32  secondSfx{ -1 };
        uint32 firstTime{};
        switch (event) {
        case AE_PED_JACKED_CAR_PUNCH:
            firstSfx     = 0x3A;
            secondSfx    = CAEAudioUtility::GetRandomNumberInRange(0x28, 0x2B);
            firstTime    = now + 500;
            lastSoundTime = now + 0x855;
            break;
        case AE_PED_JACKED_CAR_HEAD_BANG:
            firstSfx     = 0x32;
            secondSfx    = CAEAudioUtility::GetRandomNumberInRange(0x24, 0x27);
            firstTime    = now + 0x3A5;
            lastSoundTime = now + 0xA49;
            break;
        case AE_PED_JACKED_CAR_KICK:
            secondSfx    = CAEAudioUtility::GetRandomNumberInRange(0x28, 0x2B);
            firstTime    = now + 0x384;
            lastSoundTime = now + 0xAF0;
            break;
        case AE_PED_JACKED_BIKE:
            firstSfx     = 0x3A;
            secondSfx    = CAEAudioUtility::GetRandomNumberInRange(0x28, 0x2B);
            firstTime    = now + 0x21;
            lastSoundTime = now + 0x16E;
            break;
        case AE_PED_JACKED_DOZER:
            secondSfx    = CAEAudioUtility::GetRandomNumberInRange(0x24, 0x27);
            firstTime    = now + 0x10A;
            lastSoundTime = now + 0x341;
            break;
        default:
            break;
        }

        if (firstSfx != -1) {
            PlayAt(SND_BANK_SLOT_WEAPON_GEN, (eSoundID)firstSfx, firstTime);
        }
        if (secondSfx != -1) {
            PlayAt(SND_BANK_SLOT_WEAPON_GEN, (eSoundID)secondSfx, firstTime);
        }
    }

    if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
        AEAudioHardware.LoadSoundBank(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC);
        return;
    }
    PlayAt(SND_BANK_SLOT_FOOTSTEPS_GENERIC, (eSoundID)0, lastSoundTime);
}

// 0x4E26A0
void CAEPedAudioEntity::HandleSwimSplash(eAudioEvents event) {
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING)) {
        return;
    }
    if (++m_nSfxId > 4) {
        m_nSfxId = 0;
    }
    AESoundManager.PlaySound({
        .BankSlotID         = SND_BANK_SLOT_SWIMMING,
        .SoundID            = (eSoundID)(m_nSfxId),
        .AudioEntity        = this,
        .Pos                = m_pPed->GetPosition(),
        .Volume             = GetDefaultVolume(event),
        .FrequencyVariance  = 0.0588f,
        .RegisterWithEntity = m_pPed,
    });
}

// 0x4E2790
void CAEPedAudioEntity::HandleSwimWake(eAudioEvents event) {
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_COLLISIONS, SND_BANK_SLOT_COLLISIONS, true)) {
        return;
    }
    if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(event, this, m_pPed)) {
        AESoundManager.PlaySound({
            .BankSlotID         = SND_BANK_SLOT_COLLISIONS,
            .SoundID            = 3,
            .AudioEntity        = this,
            .Pos                = m_pPed->GetPosition(),
            .Volume             = GetDefaultVolume(event) - 20.0f,
            .RollOffFactor      = 1.f,
            .Speed              = 0.75f,
            .Doppler            = 1.f,
            .FrameDelay         = 0,
            .Flags              = SOUND_REQUEST_UPDATES | SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
            .RegisterWithEntity = m_pPed,
            .EventID            = event,
        });
    }
    m_LastSwimWakeTriggerTimeMs = CTimer::GetTimeInMS();
}

// 0x4E2A90
void CAEPedAudioEntity::PlayShirtFlap(float volume, float speed) {
    if (m_sTwinLoopSoundEntity.IsActive()) {
        m_sTwinLoopSoundEntity.UpdateTwinLoopSound(m_pPed->GetPosition(), volume, speed);
    } else {
        m_sTwinLoopSoundEntity.Initialise(SND_BANK_SLOT_WEAPON_GEN, 19, 20, this, 200, 1000, -1, -1);
        m_sTwinLoopSoundEntity.PlayTwinLoopSound(m_pPed->GetPosition(), volume, speed, 2.0f, 1.0f, SOUND_DEFAULT);
    }
}

// 0x4E2EE0
void CAEPedAudioEntity::Service() {
    if (!m_pPed->IsPlayer()) {
        return;
    }

    auto volume    = -100.0f;
    auto speed     = 1.0f;
    auto fallBlend = 0.0f;
    auto paraBlend = 0.0f;

    // Wind only blows when the player is not inside of a vehicle
    const auto attachedTo = m_pPed->m_pAttachedTo;
    const auto windBlows  = !((attachedTo && attachedTo->GetIsTypeVehicle()) || m_pPed->bInVehicle);

    // Driving a bike or an open top car makes the wind depend on the speed of the vehicle
    if (m_pPed->m_nPedState == PEDSTATE_DRIVING && m_pPed->m_pVehicle) {
        const auto vehicle = m_pPed->m_pVehicle;
        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE
            || (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && vehicle->IsOpenTopCar())
        ) {
            const auto vehicleSpeed = std::abs(vehicle->GetMoveSpeed().Magnitude());
            if (vehicleSpeed > 0.0f) {
                const auto baseVolume = vehicle->m_vehicleAudio.m_AuSettings.VehicleAudioType == AE_BMX
                    ? -17.0f
                    : -10.0f;
                const auto t = std::clamp(vehicleSpeed, 0.0f, 1.0f);
                volume       = CAEAudioUtility::AudioLog10(t) * 20.0f + baseVolume;
                speed        = (1.2f - 0.8f) * t + 0.8f;
            }
        }
    }

    const auto usingParachute = m_pPed->GetIntelligence() && m_pPed->GetIntelligence()->GetUsingParachute();

    if (windBlows) {
        // Speed of the ped, on foot only the part above the half of it is heard
        auto  t = m_pPed->GetMoveSpeed().Magnitude() / 0.7f;
        float baseVolume;
        if (!usingParachute) {
            baseVolume = -16.0f;
            t          = t < 0.5f
                ? 0.0f
                : std::clamp((t - 0.5f) / (1.0f - 0.5f), 0.0f, 1.0f);
        } else {
            baseVolume = -7.0f;
            if (const auto assoc = RpAnimBlendClumpGetAssociation(m_pPed->GetRpClump(), "FALL")) {
                fallBlend = std::clamp(assoc->m_BlendAmount, 0.0f, 1.0f);
            }
            if (const auto assoc = RpAnimBlendClumpGetAssociation(m_pPed->GetRpClump(), "Para")) {
                paraBlend = std::clamp(assoc->m_BlendAmount, 0.0f, 1.0f);
            }
            t = std::clamp(t, 0.0f, 1.0f);
        }
        volume = CAEAudioUtility::AudioLog10(t) * 20.0f + baseVolume;
        speed  = (1.6f - 0.8f) * t + 0.8f;
    }

    speed *= 0.5f * fallBlend + 1.0f;
    const auto targetVolume = 4.0f * fallBlend + volume + -6.0f * paraBlend;

    // The volume of the flapping follows the target slowly
    if (targetVolume > field_158) {
        field_158 = std::min(field_158 + 5.0f, targetVolume);
    } else if (targetVolume <= field_158) {
        field_158 = std::max(field_158 - 2.0f, targetVolume);
    }
    if (targetVolume < -20.0f && field_158 < -20.0f) {
        field_158 = targetVolume;
    }

    if (field_158 > -100.0f) {
        PlayShirtFlap(field_158, speed);
    } else {
        m_sTwinLoopSoundEntity.StopSoundAndForget();
    }
}
