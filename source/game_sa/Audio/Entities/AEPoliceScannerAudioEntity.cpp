#include "StdInc.h"

#include "AEPoliceScannerAudioEntity.h"

#include "AEAudioHardware.h"
#include "AEAudioUtility.h"
#include "TheZones.h"
#include "Wanted.h"

// 0x4E6E00
CAEPoliceScannerAudioEntity::~CAEPoliceScannerAudioEntity() {
    if (s_pPSControlling == this && s_nScannerPlaybackState != STATE_INITIAL) {
        s_bStoppingScanner = true;
        if (s_pSound) {
            s_pSound->StopSoundAndForget();
            s_pSound = nullptr;
        }
        FinishedPlayingScannerDialogue();
    }
}

// 0x5B9C30
void CAEPoliceScannerAudioEntity::StaticInitialise() {
    s_NextNewScannerDialogueTime = 0;
    s_bScannerDisabled           = false;
    s_nScannerPlaybackState      = STATE_INITIAL;
    s_pPSControlling             = nullptr;
    s_pCurrentSlots              = nullptr;
    s_fVolumeOffset              = 0.0f;
}

// 0x4E6E90
void CAEPoliceScannerAudioEntity::Reset() {
    StopScanner(true);
    FinishedPlayingScannerDialogue();
}

namespace {
struct tScannerZone {
    char  Name[8];
    int16 AreaSoundID;  // Sound in the areas bank, negative if the zone has no recording
    bool  HasDirection; // Whether the side of the zone can be announced
};

constexpr tScannerZone SCANNER_ZONES[] = {
    { "ALDEA", 0, true },
    { "ANGPI", 1, true },
    { "ARCO", 2, true },
    { "CUNTC", 3, false },
    { "BACKO", 4, true },
    { "BATTP", 5, true },
    { "SUNNN", 6, true },
    { "SUNMA", 7, false },
    { "BYTUN", 8, true },
    { "BEACO", 9, true },
    { "BFLD", 10, true },
    { "BFC", 11, true },
    { "BINT", 12, true },
    { "BLUAC", 13, true },
    { "BLUEB", 14, true },
    { "BONE", 15, true },
    { "CALI", 16, false },
    { "CALT", 17, true },
    { "CALT", 18, true },
    { "CITYS", 19, false },
    { "LOT", 20, false },
    { "COM", 21, true },
    { "CONF", 22, false },
    { "CRANB", 23, false },
    { "DILLI", 24, true },
    { "DOH", 25, true },
    { "SFDWT", 26, true },
    { "LDT", 27, true },
    { "ELS", 28, false },
    { "EBE", 29, false },
    { "EASB", 30, true },
    { "SFAIR", 31, false },
    { "EBAY", 32, false },
    { "ETUNN", 33, false },
    { "ELCA", 34, true },
    { "ELCO", 35, true },
    { "ELQUE", 36, true },
    { "ESPE", 37, false },
    { "ESPN", 38, false },
    { "HAUL", 39, true },
    { "FALLO", 40, false },
    { "FERN", 41, true },
    { "FINA", 42, true },
    { "FISH", 43, true },
    { "FLINTC", 44, true },
    { "FLINTI", 45, true },
    { "FLINTR", 46, true },
    { "FLINW", 47, true },
    { "CARSO", 48, false },
    { "SILLY", 49, true },
    { "FRED", 50, false },
    { "GAN", 51, true },
    { "GANTB", 52, false },
    { "GARC", 53, true },
    { "GARV", 54, false },
    { "GLN", 55, true },
    { "GGC", 56, false },
    { "PALMS", 57, true },
    { "HBARNS", 58, true },
    { "HANKY", 59, true },
    { "HGP", 60, true },
    { "HASH", 61, true },
    { "TOPFA", 62, false },
    { "QUARY", 63, false },
    { "IWD", 64, true },
    { "JTE", 65, false },
    { "JTN", 66, false },
    { "JTS", 67, false },
    { "JTW", 68, false },
    { "JUNIHI", 69, true },
    { "JUNIHO", 70, true },
    { "KACC", 71, false },
    { "KINC", 72, false },
    { "THEA", 73, true },
    { "BARRA", 74, true },
    { "BRUJA", 75, true },
    { "PAYAS", 76, true },
    { "LDM", 77, false },
    { "LEAFY", 79, true },
    { "PROBE", 80, false },
    { "LDS", 81, true },
    { "LINDEN", 82, false },
    { "LMEX", 83, true },
    { "CHC", 84, true },
    { "LFL", 85, true },
    { "LSINL", 87, false },
    { "LAIR", 88, false },
    { "???", 89, true },
    { "VAIR", 90, false },
    { "LVA", 91, false },
    { "MAR", 92, false },
    { "MKT", 93, false },
    { "MARKST", 94, false },
    { "MART", 95, false },
    { "HILLP", 96, true },
    { "MONT", 97, true },
    { "MONINT", 98, true },
    { "MTCHI", 99, true },
    { "MUL", 100, true },
    { "MULINT", 101, true },
    { "NROCK", 102, true },
    { "LDOC", 103, true },
    { "OCEAF", 104, false },
    { "OCTAN", 105, false },
    { "OVS", 106, true },
    { "OPEN", 107, false },
    { "BAYV", 108, true },
    { "PALO", 109, true },
    { "PARA", 110, true },
    { "PILG", 111, true },
    { "PINT", 112, true },
    { "PLS", 113, true },
    { "PRP", 114, true },
    { "????", 115, true },
    { "RIE", 116, false },
    { "RED", 117, true },
    { "REDE", 118, false },
    { "REDW", 119, false },
    { "TOM", 120, false },
    { "RIH", 121, true },
    { "ROCE", 122, true },
    { "RSE", 123, false },
    { "RSW", 124, false },
    { "ROD", 125, true },
    { "ROY", 126, false },
    { "SASO", 127, false },
    { "SANB", 129, true },
    { "CIVI", 130, true },
    { "SMB", 131, true },
    { "CREEK", 132, true },
    { "SHERR", 133, false },
    { "SRY", 134, false },
    { "SPIN", 135, true },
    { "STAR", 136, false },
    { "SUN", 137, true },
    { "SUN", 138, true },
    { "BIGE", 139, false },
    { "CAM", 140, false },
    { "RING", 141, false },
    { "ISLE", 142, false },
    { "FARM", 143, false },
    { "DRAG", 144, false },
    { "HIGH", 145, false },
    { "MAKO", 146, false },
    { "PANOP", 147, false },
    { "PINK", 148, false },
    { "PIRA", 149, false },
    { "DAM", 150, false },
    { "VISA", 151, false },
    { "ROBAD", 152, true },
    { "UNITY", 153, false },
    { "VALLE", 154, true },
    { "BLUF", 155, false },
    { "MEAD", 156, true },
    { "VERO", 157, true },
    { "VIN", 158, true },
    { "WHET", 159, true },
    { "WWE", 160, true },
    { "LIND", 161, true },
    { "YBELL", 162, false },
    { "YELLOW", 163, false },
    { "VE", 78, true },
    { "LA", 86, true },
    { "SF", 128, true },
    { "SAN_AND", -1, true },
    { "JEF", -1, true },
    { "PER1", 86, false },
    { "YBELL1", 162, false },
    { "YBELL2", 162, false },
    { "STRIP", 78, false },
    { "WESTP", -1, true },
    { "CUNTC1", 3, false },
    { "CUNTC2", 3, false },
    { "CUNTC3", 3, false },
    { "KINC1", 72, false },
    { "KINC2", 72, false },
    { "GARV1", 54, false },
    { "GARV2", 54, false },
    { "MONT1", 97, false },
    { "MTCHI1", 99, false },
    { "MTCHI2", 99, false },
    { "MTCHI3", 99, false },
    { "MTCHI4", 99, false },
    { "SHACA", -1, true },
    { "REST", -1, true },
    { "SANB2", 129, false },
    { "SFBAG1", 31, false },
    { "SFBAG2", 31, false },
    { "SFBAG3", 31, false },
    { "LVBAG", 90, false },
    { "LBAG1", 88, false },
    { "LBAG2", 88, false },
    { "LBAG3", 88, false },
    { "CONST1", 78, false },
};

constexpr eSoundID CRIME_INTRO_SOUNDS[4] = { 0, 5, 8, 0 }; // 0x8C8160
constexpr eSoundID CRIME_SOUNDS[23] = {                    // 0x8C8168
    -1, -1, 0, 7, 5, 8, 2, 1, 1, 1, 0, 8, 10, 3, 8, 3, 10, 4, 5, 8, 9, 6, 6
};

constexpr eSoundID VEHICLE_SOUNDS[46] = { // 0x8C89F8
    34, 0, 47, 44, 49, -1, 56, 17, 20, 30, 31, 57, 2, 25, 52, 36, 27, 7, 39, 6, 11, 51, 14, 24,
    56, 32, 55, 22, 37, 33, 40, 15, 8, 18, 53, 12, 23, 29, 38, 54, 26, 5, 43, 41, 10, 1
};
constexpr bool VEHICLE_HAS_COLOUR[46] = { // 0x8C8A54
    1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0,
    1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1
};
constexpr eSoundID COLOUR_SOUNDS[127] = { // 0x8C8A88, indexed by the primary colour
    0, 14, 1, 12, 1, 6, -1, 1, 1, 10, 1, 9, 1, 9, 10, 9, 8, 12, 12, 10, 1, 12, 12, 9,
    9, 6, 9, 9, 1, 9, 6, 12, 1, 9, 10, 9, 6, 8, 10, 1, 6, 6, 12, 12, 12, 12, 10, 8,
    10, 9, 9, 8, 8, 1, 1, 6, 1, 10, 12, 1, 10, 10, 12, 9, 9, 10, 6, 1, 10, 10, 12, 1,
    9, 10, 12, 6, 9, 10, 12, 1, 12, 10, 12, 8, 6, 12, 10, 1, 12, 10, 13, 1, 9, 10, 1, 1,
    13, 1, 1, 10, 1, 6, 10, 1, 10, 9, 1, 10, 10, 9, 6, 9, 1, 6, 8, 12, 1, 12, 10, 6,
    10, 12, 9, 6, 12, 1, 11
};
}

// 0x4E71E0
void CAEPoliceScannerAudioEntity::AddAudioEvent(eAudioEvents event, eCrimeType crimeType, const CVector& point) {
    if (event != AE_CRIME_COMMITTED || (int32)crimeType < CRIME_DAMAGED_PED || (int32)crimeType > CRIME_AIM_GUN) {
        return;
    }

    tScannerSlot first[NUM_POLICE_SCANNER_SLOTS]{};
    tScannerSlot second[NUM_POLICE_SCANNER_SLOTS]{};

    first[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, CRIME_INTRO_SOUNDS[CAEAudioUtility::GetRandomNumberInRange<int32>(0, 3)] };
    first[1] = { SND_BANK_SCRIPT_SCANNER_NUMBERS, CRIME_SOUNDS[crimeType] };

    const auto* const zone = CTheZones::FindSmallestZoneForPosition(point, true);
    if (!zone) {
        return;
    }

    const tScannerZone* scannerZone{};
    for (const auto& z : SCANNER_ZONES) {
        if (!memcmp(z.Name, zone->m_TextLabel, sizeof(z.Name)) && z.AreaSoundID >= 0) {
            scannerZone = &z;
            break;
        }
    }
    if (!scannerZone) {
        return;
    }

    if (scannerZone->HasDirection) {
        const auto width   = (float)(zone->m_fX2 - zone->m_fX1);
        const auto height  = (float)(zone->m_fY2 - zone->m_fY1);
        const auto centerX = width * 0.5f + (float)zone->m_fX1;
        const auto centerY = height * 0.5f + (float)zone->m_fY1;
        const auto quarterX = width * 0.25f;
        const auto quarterY = height * 0.25f;

        bool hasNorthSouth = false;
        if (point.y > centerY + quarterY) {
            first[2]      = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 2 };
            hasNorthSouth = true;
        } else if (point.y < centerY - quarterY) {
            first[2]      = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 3 };
            hasNorthSouth = true;
        }

        if (point.x > centerX + quarterX) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 1 };
        } else if (point.x < centerX - quarterX) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 4 };
        } else if (!hasNorthSouth) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 0 };
        }
    }
    first[4] = { SND_BANK_SCRIPT_SCANNER_AREAS, scannerZone->AreaSoundID };

    CPed* ped{};
    for (auto i = 0; i < 2; i++) {
        const auto* const wanted = FindPlayerWanted(i);
        if (wanted && &wanted->m_PoliceScannerAudioEntity == this) {
            ped = FindPlayerPed(i);
        }
    }

    if (ped) {
        if (ped->bInVehicle) {
            if (auto* const veh = ped->m_pVehicle) {
                const auto type = (int32)veh->m_vehicleAudio.m_AuSettings.VehicleAudioType;
                if (type >= 0 && type < (int32)std::size(VEHICLE_SOUNDS) && VEHICLE_SOUNDS[type] >= 0) {
                    second[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, 7 };
                    second[1] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, 1 };
                    if (type == 0x19 || type == 0x1D || type == 0x1E || type == 0x25 || type == 0x29 || type == 0x22) {
                        second[1].SoundID = 3;
                    }

                    if (VEHICLE_HAS_COLOUR[type]) {
                        if (veh->GetRemapIndex() != -1) {
                            second[3] = { SND_BANK_SCRIPT_SCANNER_COLOURS, 4 };
                        } else if (const auto colour = veh->m_nPrimaryColor; colour != 0 && colour < (uint8)std::size(COLOUR_SOUNDS) && COLOUR_SOUNDS[colour] >= 0) {
                            second[3] = { SND_BANK_SCRIPT_SCANNER_COLOURS, COLOUR_SOUNDS[colour] };
                        }
                    }
                    second[4] = { SND_BANK_SCRIPT_SCANNER_VEHICLES, VEHICLE_SOUNDS[type] };
                }
            }
        } else {
            auto* const intel = ped->GetIntelligence();
            bool        speak = true;
            if (intel->GetTaskSwim()) {
                second[4].SoundID = 2;
            } else if (!intel->GetTaskJetPack()) {
                second[4].SoundID = 4;
            } else {
                speak = false;
            }
            if (speak) {
                second[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, 7 };
                second[4].Bank = SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS;
            }
        }
    }

    PlayPoliceScannerDialogue(first, second);
}

// 0x4E6BC0
void CAEPoliceScannerAudioEntity::PrepSlots() {
    if (!s_pCurrentSlots) {
        return;
    }
    for (auto i = 0; i < NUM_POLICE_SCANNER_SLOTS; i++) {
        s_SlotState[i] = s_pCurrentSlots[i].IsActive();
    }
}

// 0x4E6CD0
void CAEPoliceScannerAudioEntity::LoadSlots() {
    if (!s_pCurrentSlots) {
        return;
    }

    bool canPlay = true;
    for (auto i = 0; i < NUM_POLICE_SCANNER_SLOTS; i++) {
        const auto slot = (eSoundBankSlot)(SND_BANK_SLOT_SCANNER_FIRST + i);
        auto& currentSlot = s_pCurrentSlots[i];

        if (s_SlotState[i]) {
            if (s_SlotState[i] == 2) {
                bool loaded = AEAudioHardware.IsSoundLoaded(currentSlot.Bank, currentSlot.SoundID, slot);
                if (loaded) {
                    s_SlotState[i] = 3;
                } else {
                    canPlay = false;
                }
            }
        } else if (currentSlot.IsActive()) {
            s_SlotState[i] = 1;
        } else {
            if (!CStreaming::IsVeryBusy()) {
                AEAudioHardware.LoadSound(currentSlot.Bank, currentSlot.SoundID, slot);
                s_SlotState[i] = 2;
            }
            canPlay = false;
        }
    }
    if (canPlay) {
        s_nScannerPlaybackState = FOUR;
    }
}

// 0x4E6DB0
void CAEPoliceScannerAudioEntity::EnableScanner() {
    s_bScannerDisabled = false;
}

// 0x4E71B0
void CAEPoliceScannerAudioEntity::DisableScanner(bool a1, bool bStopSound) {
    s_bScannerDisabled = true;
    if (a1 && s_nScannerPlaybackState != STATE_INITIAL) {
        if (s_pPSControlling) {
            StopScanner(bStopSound);
        }
    }
}

// 0x4E6DC0
void CAEPoliceScannerAudioEntity::StopScanner(bool bStopSound) {
    if (s_nScannerPlaybackState == STATE_INITIAL)
        return;

    s_bStoppingScanner = true;
    if (bStopSound) {
        if (s_pSound) {
            s_pSound->StopSoundAndForget();
            s_pSound = nullptr;
        }
        FinishedPlayingScannerDialogue();
    }
}

// 0x4E6C30
void CAEPoliceScannerAudioEntity::FinishedPlayingScannerDialogue() {
    constexpr uint32 NEW_DIALOGUE_DELAY_MS = 10'000; // 0x8C8140

    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_STOP);
    s_nScannerPlaybackState      = STATE_INITIAL;
    s_pPSControlling             = nullptr;
    s_pCurrentSlots              = nullptr;
    s_bStoppingScanner           = false;
    s_NextNewScannerDialogueTime = CTimer::GetTimeInMS() + NEW_DIALOGUE_DELAY_MS;
    s_fVolumeOffset              = 0.0f;

    rng::fill(s_SlotState, 1);

    rng::fill(s_ScannerSlotFirst, tScannerSlot{});
    rng::fill(s_ScannerSlotSecond, tScannerSlot{});
}

// 0x4E6F60
void CAEPoliceScannerAudioEntity::PlayLoadedDialogue() {
    static constexpr float clickVolume = +0.0f; // 0xB61D54, same constant Service() uses under that name

    int16 i;
    for (i = 0; i < NUM_POLICE_SCANNER_SLOTS; ++i) {
        if (s_SlotState[i] == FIVE) {
            break;
        }
    }

    if (i == NUM_POLICE_SCANNER_SLOTS) {
        i = 0;
        goto LABEL_7;
    }

    if (i >= NUM_POLICE_SCANNER_SLOTS) {
    LABEL_9:
        auto volumeChange = s_fVolumeOffset + clickVolume;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_CLICK, volumeChange);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_STOP);
        if (s_nSectionPlaying) {
            FinishedPlayingScannerDialogue();
        } else {
            auto v2 = (int16*)s_ScannerSlotSecond;
            s_pCurrentSlots = s_ScannerSlotSecond;
            s_nSectionPlaying = 1;
            auto v3 = s_SlotState;
            auto v4 = 5;
            do {
                *v3 = *v2 < 0 || v2[1] < 0;
                v2 += 2;
                ++v3;
                --v4;
            } while (v4);
            s_nPlaybackStartTime = 0;
            s_nAbortPlaybackTime = CTimer::GetTimeInMS() + 5000; // matches the constant PlayPoliceScannerDialogue() already uses here
            s_nScannerPlaybackState = TWO;
        }
    } else {
    LABEL_7:
        while (s_SlotState[i] != 3) {
            if (++i >= 5)
                goto LABEL_9;
        }

        auto volume = GetDefaultVolume(AE_CRIME_COMMITTED) + s_fVolumeOffset;
        CAESound sound;
        sound.Initialise((eSoundBankSlot)(SND_BANK_SLOT_SCANNER_FIRST + i), s_pCurrentSlots[i].SoundID, this, { 0.0, 1.0f, 0.0f }, volume, 1.0f, 1.0f, 1.0f, 0, SOUND_DEFAULT, 0.0f, 0);
        sound.m_ClientVariable = (float)i;
        sound.m_Flags = SOUND_FRONT_END | SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | SOUND_IS_DUCKABLE;
        sound.m_Event = AE_CRIME_COMMITTED;

        s_pSound = AESoundManager.RequestNewSound(&sound);
        if (s_pSound) {
            s_SlotState[i] = 5;
            s_nScannerPlaybackState = SEVEN;
        }
    }
}

// 0x4E6B60
void CAEPoliceScannerAudioEntity::PopulateScannerDialogueLists(const tScannerSlot* first, const tScannerSlot* second) {
    assert(first && second);
    if (s_nScannerPlaybackState == STATE_INITIAL) {
        for (auto slotIndex = 0; slotIndex < NUM_POLICE_SCANNER_SLOTS; slotIndex++) {
            s_ScannerSlotFirst[slotIndex] = first[slotIndex];
            s_ScannerSlotSecond[slotIndex] = second[slotIndex];
        }
    }
}

// inlined
// 0x4E6C00
bool CAEPoliceScannerAudioEntity::CanWePlayNewScannerDialogue() {
    if (s_nScannerPlaybackState != STATE_INITIAL)
        return false;

    if (CTimer::GetTimeInMS() < s_NextNewScannerDialogueTime)
        return false;

    if (TheCamera.m_bWideScreenOn)
        return false;

    if (s_bScannerDisabled)
        return false;

    return true;
}

// 0x4E6ED0
void CAEPoliceScannerAudioEntity::PlayPoliceScannerDialogue(tScannerSlot* first, tScannerSlot* second) {
    assert(first && second);
    if (CanWePlayNewScannerDialogue()) { // todo: maybe little bit wrong
        PopulateScannerDialogueLists(first, second);
        s_pPSControlling = this;
        s_nSectionPlaying = 0;
        s_pCurrentSlots = s_ScannerSlotFirst;
        PrepSlots();
        s_nScannerPlaybackState = TWO;
        s_nPlaybackStartTime = CTimer::GetTimeInMS() + 2000; // todo: gSpeechContextLookup[365][6]
        s_nAbortPlaybackTime = CTimer::GetTimeInMS() + 5000; // todo: gSpeechContextLookup[366][0]
    }
}

// 0x4E7590
void CAEPoliceScannerAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (curPlayPos == -1) {
        s_pSound = nullptr;
        if (s_bStoppingScanner) {
            if (s_nScannerPlaybackState != STATE_INITIAL) {
                s_bStoppingScanner = true; // V1048 [CWE-1164] The 's_bStoppingScanner' variable was assigned the same value
                FinishedPlayingScannerDialogue();
            }
            return;
        }
        PlayLoadedDialogue();
        return;
    }

    if (s_bStoppingScanner) {
        sound->m_Volume = sound->m_Volume - 6.0f; // todo: *(float*)&gSpeechContextLookup[366][4]
        return;
    }

    if (sound->m_Length > 0 && curPlayPos > sound->m_Length - 40 && sound->m_BankSlot != 37) { // todo: -40 should be replaced with by gSpeechContextLookup[366][2]
        sound->SetFlags(eSoundEnvironment::SOUND_REQUEST_UPDATES, false);
        s_pSound = nullptr;
        PlayLoadedDialogue();
        return;
    }
}

// 0x4E7630
void CAEPoliceScannerAudioEntity::Service() {
    static constexpr uint32 startDelay = 300;   // 0x8C8154
    static constexpr float noiseVolume = -6.0f; // 0x8C8158
    static constexpr float clickVolume = +0.0f; // 0xB61D54

    bool finishPlaying;
    if (TheCamera.m_bWideScreenOn && s_nScannerPlaybackState != STATE_INITIAL) {
        finishPlaying = true;
        s_bStoppingScanner = true;
    } else {
        finishPlaying = s_bStoppingScanner;
    }

    switch (s_nScannerPlaybackState) {
    case TWO:
        if (CTimer::GetTimeInMS() > s_nAbortPlaybackTime || finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        LoadSlots();
        break;
    case FOUR:
        if (finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        if (CTimer::GetTimeInMS() >= s_nPlaybackStartTime) {
            s_fVolumeOffset = CAEVehicleAudioEntity::s_pVehicleAudioSettingsForRadio ? 0.0f : -8.0f; // todo: -8 is gSpeechContextLookup[367][2]
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_CLICK,       s_fVolumeOffset + clickVolume, 1.0f);
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_START, s_fVolumeOffset + noiseVolume, 1.0f); // todo: noiseVolume is gSpeechContextLookup[367][0]
            s_nPlaybackStartTime = startDelay + CTimer::GetTimeInMS(); // todo: startDelay is gSpeechContextLookup[366][6]
            s_nScannerPlaybackState = FIVE;
        }
        break;
    case FIVE:
        if (finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        if (CTimer::GetTimeInMS() >= s_nPlaybackStartTime && s_pPSControlling) {
            s_nScannerPlaybackState = SEVEN;
            s_pPSControlling->PlayLoadedDialogue();
        }
        break;
    default:
        return;
    }
}

void CAEPoliceScannerAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEPoliceScannerAudioEntity, 0x85F368, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Constructor, 0x56DA00);
    RH_ScopedInstall(Destructor, 0x4E6E00);
    RH_ScopedInstall(StaticInitialise, 0x5B9C30);
    RH_ScopedInstall(Reset, 0x4E6E90);
    RH_ScopedInstall(AddAudioEvent, 0x4E71E0);
    RH_ScopedInstall(PrepSlots, 0x4E6BC0);
    RH_ScopedInstall(LoadSlots, 0x4E6CD0);
    RH_ScopedInstall(EnableScanner, 0x4E6DB0);
    RH_ScopedInstall(DisableScanner, 0x4E71B0);
    RH_ScopedInstall(StopScanner, 0x4E6DC0);
    RH_ScopedInstall(FinishedPlayingScannerDialogue, 0x4E6C30);
    RH_ScopedInstall(PlayLoadedDialogue, 0x4E6F60);
    RH_ScopedInstall(PopulateScannerDialogueLists, 0x4E6B60);
    RH_ScopedInstall(CanWePlayNewScannerDialogue, 0x4E6C00);
    RH_ScopedInstall(PlayPoliceScannerDialogue, 0x4E6ED0);
    RH_ScopedVMTInstall(UpdateParameters, 0x4E7590);
    RH_ScopedInstall(Service, 0x4E7630);
}

CAEPoliceScannerAudioEntity* CAEPoliceScannerAudioEntity::Constructor() {
    this->CAEPoliceScannerAudioEntity::CAEPoliceScannerAudioEntity();
    return this;
}

CAEPoliceScannerAudioEntity* CAEPoliceScannerAudioEntity::Destructor() {
    this->CAEPoliceScannerAudioEntity::~CAEPoliceScannerAudioEntity();
    return this;
}
