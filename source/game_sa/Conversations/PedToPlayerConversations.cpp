#include "PedToPlayerConversations.h"

#include "Ragdoll/IKChainManager.h"

#include "Conversations.h"
#include "Clock.h"
#include "Weather.h"
#include "Stats.h"
#include "Timer.h"
#include "PedClothesDesc.h"
#include "PlayerPedData.h"
#include "VehicleModelInfo.h"
#include "Pools/Pools.h"
#include "Audio/Entities/AEPedSpeechAudioEntity.h"
#include "Events/EventScriptCommand.h"
#include "Tasks/TaskComplexSequence.h"
#include "Tasks/TaskTypes/TaskSimpleStandStill.h"
#include "Tasks/TaskTypes/TaskComplexKillPedOnFoot.h"

void CPedToPlayerConversations::InjectHooks() {
    RH_ScopedClass(CPedToPlayerConversations);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Clear, 0x43AAE0);
    RH_ScopedInstall(Update, 0x43B0F0);
    RH_ScopedInstall(EndConversation, 0x43AB10);
}

// 0x43AAE0
void CPedToPlayerConversations::Clear() {
    ZoneScoped;

    if (m_State != eP2pState::INACTIVE) {
        CAEPedSpeechAudioEntity::ReleasePlayerConversation();
        m_State = eP2pState::INACTIVE;
    }

    m_pPed                         = nullptr;
    m_TimeOfLastPlayerConversation = 0;
}

// 0x43B0F0
void CPedToPlayerConversations::Update() {
    ZoneScoped;

    static_assert(offsetof(CVehicleModelInfo, m_nVehicleClass) == 0x4D);

    auto* const player = FindPlayerPed(-1);

    // The vehicle of the player is what the peds comment on
    if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
        m_pPlayerVehicle = vehicle;
        vehicle->RegisterReference(reinterpret_cast<CEntity**>(&m_pPlayerVehicle));
    }

    // The ped goes after the player, and it is the ped that has to be sure about wanting to
    const auto MakePedAttackPlayer = [&] {
        auto* const sequence = new CTaskComplexSequence{};
        sequence->AddTask(new CTaskSimpleStandStill{ 2000, false, false, 8.0f });
        sequence->AddTask(new CTaskComplexKillPedOnFoot{ player, -1, 0, 0, 0, 0 });

        CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, sequence, false };
        m_pPed->GetEventGroup().Add(&event, false);
    };

    // What the player says back, depends on the sex of the ped
    const auto SayDismiss = [&](eGlobalSpeechContext female, eGlobalSpeechContext male) {
        player->Say(IsPedTypeFemale(m_pPed->m_nPedType) ? female : male, 0, 1.0f, true, false, false);
    };

    switch (m_State) {
    case eP2pState::INACTIVE: {
        if (CTimer::GetTimeInMS() <= m_TimeOfLastPlayerConversation + 30'000) {
            return;
        }
        if (FindPlayerVehicle(-1, false)) {
            return;
        }
        if (FindPlayerPed(-1)->GetWantedLevel() != eWantedLevel::WANTED_CLEAN) {
            return;
        }
        if (CConversations::IsConversationGoingOn()) {
            return;
        }
        if (!player->PedIsReadyForConversation(true)) {
            return;
        }

        // A few of the peds are checked each frame, one after the other
        auto& pool = *GetPedPool();
        for (auto attempt = 0; attempt < 8; attempt++) {
            m_PedPoolIndex++;
            if (m_PedPoolIndex >= (int32)pool.GetSize()) {
                m_PedPoolIndex = 0;
            }

            auto* const ped = pool.GetAt(m_PedPoolIndex);
            if (!ped || !ped->IsCreatedBy(PED_GAME)) {
                continue;
            }
            if (!ped->PedIsReadyForConversation(true) || ped->bHasAScriptBrain || !ped->GetIsOnScreen()) {
                continue;
            }

            // They have to be looking at each other, and be close enough
            if (!(DotProduct(ped->GetForward(), player->GetPosition() - ped->GetPosition()) > 0.0f)) {
                continue;
            }
            if (!(DotProduct(player->GetForward(), ped->GetPosition() - player->GetPosition()) > 0.0f)) {
                continue;
            }
            if (!((ped->GetPosition() - FindPlayerCoors(-1)).Magnitude() < 7.0f)) {
                continue;
            }

            // Gangsters only ask where the player is from
            const auto type = ped->m_nPedType;
            m_Topic = (type == PED_TYPE_GANG1 || (type > PED_TYPE_GANG2 && type <= PED_TYPE_GANG10))
                ? CGeneral::GetRandomNumberInRange(8, 10)
                : CGeneral::GetRandomNumberInRange(0, 7);
            if (!ped->GetSpeechAE().WillPedChatAboutTopic((int16)m_Topic)) {
                continue;
            }

            // Says what the topic is, when nobody else is in a conversation with the player
            const auto Open = [&](eGlobalSpeechContext ctx, bool isPositive) {
                if (!CAEPedSpeechAudioEntity::RequestPlayerConversation(ped)) {
                    return false;
                }
                m_bPositiveOpening = isPositive;
                ped->Say(ctx, 0, 1.0f, true, false, false);
                return true;
            };

            // How much the ped likes something about the player, the seed of the ped makes it different for each of them
            const auto Opinion = [&](eStats stat, uint32 seedMask, float offset) {
                return (int32)((double)CStats::GetStatValue(stat) + (double)(ped->m_nRandomSeed & seedMask) - (double)offset);
            };

            const auto isOpen = [&] {
                switch (m_Topic) {
                case 0: { // The vehicle of the player
                    if (!m_pPlayerVehicle) {
                        return false;
                    }
                    if (!((m_pPlayerVehicle->GetPosition() - ped->GetPosition()).Magnitude2D() < 20.0f)) {
                        return false;
                    }
                    switch (CModelInfo::GetModelInfo(m_pPlayerVehicle->m_nModelIndex)->AsVehicleModelInfoPtr()->m_nVehicleClass) {
                    case VEHICLE_CLASS_POORFAMILY:
                    case VEHICLE_CLASS_WORKER:
                    case VEHICLE_CLASS_BIG:
                    case VEHICLE_CLASS_TAXI:
                        return Open(CTX_GLOBAL_CONV_DISL_CAR, false);
                    case VEHICLE_CLASS_RICHFAMILY:
                    case VEHICLE_CLASS_EXECUTIVE:
                    case VEHICLE_CLASS_LEISUREBOAT:
                        return m_pPlayerVehicle->m_fHealth > 800.0f && Open(CTX_GLOBAL_CONV_LIKE_CAR, true);
                    default:
                        return false;
                    }
                }
                case 1: { // The clothes
                    const auto opinion = Opinion(STAT_CLOTHES_RESPECT, 0x1FF, 256.0f);
                    if (opinion > 500) {
                        return Open(CTX_GLOBAL_CONV_LIKE_CLOTHES, true);
                    }
                    return opinion < 150 && Open(CTX_GLOBAL_CONV_DISL_CLOTHES, false);
                }
                case 2: { // The hair
                    const auto opinion = Opinion(STAT_CLOTHES_RESPECT, 0x1FF, 256.0f);
                    if (!FindPlayerPed(-1)->GetPlayerData()->m_pPedClothesDesc->HasVisibleNewHairCut(0)) {
                        return false;
                    }
                    if (opinion > 500) {
                        return Open(CTX_GLOBAL_CONV_LIKE_HAIR, true);
                    }
                    if (opinion >= 150 && !(CStats::GetStatValue(STAT_HAIRDRESSING_BUDGET) < 5.0f)) {
                        return false;
                    }
                    return Open(CTX_GLOBAL_CONV_DISL_HAIR, false);
                }
                case 3: { // The body
                    const auto muscle = Opinion(STAT_MUSCLE, 0xFF, 128.0f);
                    const auto fat    = Opinion(STAT_FAT, 0xFF, 128.0f);
                    if (muscle > 400 && fat < 250) {
                        return Open(CTX_GLOBAL_CONV_LIKE_PHYS, true);
                    }
                    return muscle - fat < 200 && fat > 500 && Open(CTX_GLOBAL_CONV_DISL_PHYS, false);
                }
                case 4: { // The shoes
                    const auto opinion = Opinion(STAT_CLOTHES_RESPECT, 0x3FF, 512.0f);
                    if (opinion > 700) {
                        return Open(CTX_GLOBAL_CONV_LIKE_SHOES, true);
                    }
                    return opinion < 300 && Open(CTX_GLOBAL_CONV_DISL_SHOES, false);
                }
                case 5: { // The smell
                    const auto opinion = Opinion(STAT_SEX_APPEAL, 0x1FF, 256.0f);
                    if (opinion > 700) {
                        return Open(CTX_GLOBAL_CONV_LIKE_SMELL, true);
                    }
                    return opinion < 300 && Open(CTX_GLOBAL_CONV_DISL_SMELL, false);
                }
                case 6: { // The tattoos
                    if (!FindPlayerPed(-1)->GetPlayerData()->m_pPedClothesDesc->HasVisibleTattoo()) {
                        return false;
                    }
                    const auto opinion = CGeneral::GetRandomNumberInRange(0, 1000);
                    if (opinion > 700) {
                        return Open(CTX_GLOBAL_CONV_LIKE_TATTOO, true);
                    }
                    return opinion < 300 && Open(CTX_GLOBAL_CONV_DISL_TATTOO, false);
                }
                case 7: { // The weather
                    if (CWeather::Wind < 0.1f) {
                        const auto hour = CClock::GetGameClockHours();
                        return hour < 20 && hour > 6 && Open(CTX_GLOBAL_CONV_LIKE_WEATHER, true);
                    }
                    return CWeather::Wind > 0.5f && Open(CTX_GLOBAL_CONV_DISL_WEATHER, false);
                }
                case 8: // Where the player is from
                    return Open(CTX_GLOBAL_WHERE_YOU_FROM, true);
                case 9: // If the player is a gangster
                    return Open(CTX_GLOBAL_GANGBANG, true);
                default:
                    return false;
                }
            }();

            if (isOpen) {
                m_State = eP2pState::PEDHASOPENED;
                m_pPed  = ped;
                ped->RegisterReference(reinterpret_cast<CEntity**>(&m_pPed));
                m_TimeOfLastPlayerConversation = m_StartTime = CTimer::GetTimeInMS();

                ped->DisablePedSpeech(false);
                g_ikChainMan.LookAt("Ped2Pl_Conversation", ped, player, 100'000, BONE_HEAD, nullptr, false, 0.25f, 500, 8, false);
                g_ikChainMan.LookAt("Ped2Pl_ConversationP", player, ped, 100'000, BONE_HEAD, nullptr, false, 0.25f, 500, 8, false);
                return;
            }

            // Sometimes the ped looks at the player without a reason
            if ((CGeneral::GetRandomNumber() & 0xFFF) == 3) {
                ped->Say(CTX_GLOBAL_EYEING_PLAYER, 0, 1.0f, false, false, false);
                return;
            }
        }
        break;
    }
    case eP2pState::PEDHASOPENED: {
        if (!m_pPed) {
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            return;
        }

        // The player did not reply
        if (CTimer::GetTimeInMS() > m_StartTime + 4000) {
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            m_pPed->EnablePedSpeech();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            if (g_ikChainMan.IsLooking(m_pPed)) {
                g_ikChainMan.AbortLookAt(m_pPed, 250);
                if (m_bPositiveOpening) {
                    m_pPed->Say(CTX_GLOBAL_CONV_IGNORED, 0, 1.0f, false, false, false);
                }
            }
            return;
        }

        const auto Finish = [&](eP2pState state) {
            m_State     = state;
            m_StartTime = CTimer::GetTimeInMS();
        };

        auto* const pad = CPad::GetPad(0);
        if (pad->ConversationYesJustDown()) {
            if (!m_bPositiveOpening) { // The player said yes to something that was not nice
                SayDismiss(CTX_GLOBAL_LIKE_DISMISS_FEMALE, CTX_GLOBAL_LIKE_DISMISS_MALE);
                Finish(eP2pState::WAITINGTOFINISH);
                return;
            }

            const auto reply = [&]() -> std::optional<eGlobalSpeechContext> {
                switch (m_Topic) {
                case 0: return CTX_GLOBAL_LIKE_CAR_REPLY;
                case 1: return CTX_GLOBAL_LIKE_CLOTHES_REPLY;
                case 2: return CTX_GLOBAL_LIKE_HAIR_REPLY;
                case 3: return CTX_GLOBAL_LIKE_PHYS_REPLY;
                case 4: return CTX_GLOBAL_LIKE_SHOES_REPLY;
                case 5: return CTX_GLOBAL_LIKE_SMELL_REPLY;
                case 6: return CTX_GLOBAL_LIKE_TATTOO_REPLY;
                case 7: return CTX_GLOBAL_WEATHER_LIKE_REPLY;
                default: return std::nullopt;
                }
            }();
            if (reply) {
                player->Say(*reply, 0, 1.0f, true, false, false);
            } else if (m_Topic == 8 || m_Topic == 9) { // Gangsters can get upset, and then it ends
                player->Say(m_Topic == 8 ? CTX_GLOBAL_WHERE_YOU_FROM_POS : CTX_GLOBAL_GANGBANG_YES, 0, 1.0f, true, false, false);
                if (CGeneral::GetRandomNumberInRange(0, 100) < 75) {
                    MakePedAttackPlayer();
                }
                EndConversation();
                return;
            }
            Finish(eP2pState::WAITINGTOFINISH);
        } else if (pad->ConversationNoJustDown()) {
            if (m_bPositiveOpening) {
                switch (m_Topic) {
                case 8:
                    player->Say(CTX_GLOBAL_WHERE_YOU_FROM_NEG, 0, 1.0f, true, false, false);
                    EndConversation();
                    return;
                case 9:
                    player->Say(CTX_GLOBAL_GANGBANG_NO, 0, 1.0f, true, false, false);
                    EndConversation();
                    return;
                default:
                    SayDismiss(CTX_GLOBAL_LIKE_DISMISS_FEMALE, CTX_GLOBAL_LIKE_DISMISS_MALE);
                    Finish(eP2pState::WAITINGFORFINALWORD);
                    return;
                }
            }

            // The player said no to something that was not nice
            SayDismiss(CTX_GLOBAL_LIKE_NEGATIVE_FEMALE, CTX_GLOBAL_LIKE_NEGATIVE_MALE);
            if (m_pPed->m_nPedType != PED_TYPE_GANG2 && CGeneral::GetRandomNumberInRange(0, 100) < 40) {
                MakePedAttackPlayer();
            }
            Finish(eP2pState::WAITINGTOFINISH);
        }
        break;
    }
    case eP2pState::WAITINGFORFINALWORD: {
        if (!m_pPed) {
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            return;
        }
        if (CTimer::GetTimeInMS() > m_StartTime + 3000 && !m_pPed->GetPedTalking()) {
            m_pPed->Say(CTX_GLOBAL_LIKE_DISMISS_REPLY, 0, 1.0f, true, false, false);
            m_State     = eP2pState::WAITINGTOFINISH;
            m_StartTime = CTimer::GetTimeInMS();
        }
        break;
    }
    case eP2pState::WAITINGTOFINISH: {
        if (CTimer::GetTimeInMS() > m_StartTime + 2500 || !m_pPed) {
            EndConversation();
        }
        break;
    }
    default:
        break;
    }
}

// 0x43AB10
void CPedToPlayerConversations::EndConversation() {
    m_State           = eP2pState::INACTIVE;

    CAEPedSpeechAudioEntity::ReleasePlayerConversation();

    if (m_pPed) {
        m_pPed->EnablePedSpeech();
    }

    const auto player = FindPlayerPed(-1);
    if (g_ikChainMan.IsLooking(player)) {
        g_ikChainMan.AbortLookAt(player, 250);
    }

    if (m_pPed && g_ikChainMan.IsLooking(m_pPed)) {
        return g_ikChainMan.AbortLookAt(m_pPed, 250);
    }
}
