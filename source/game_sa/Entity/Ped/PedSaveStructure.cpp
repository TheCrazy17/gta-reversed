#include "StdInc.h"
#include "PedSaveStructure.h"

#include "EntryExitManager.h"
#include "EntryExit.h"
#include "Streaming.h"
#include "WeaponInfo.h"

void CPedSaveStructure::InjectHooks() {
    RH_ScopedClass(CPedSaveStructure);
    RH_ScopedCategory("Entity/Ped");

    RH_ScopedInstall(Extract, 0x5D44B0);
    RH_ScopedInstall(Construct, 0x5D43D0);
}

// Writes the state saved in this structure back into the ped
// 0x5D44B0
void CPedSaveStructure::Extract(CPed* ped) {
    ped->GetPosition() = m_pos;
    ped->m_fHealth     = m_health;
    ped->m_fArmour     = m_armor;
    ped->m_nActiveWeaponSlot = m_activeWeaponSlot;
    ped->SetCharCreatedBy(m_createdBy);
    ped->m_nFightingStyle      = (eFightingStyle)m_nFightingStyle;
    ped->m_nAllowedAttackMoves = m_nAllowedAttackMoves;

    for (auto i = 0u; i < std::size(m_weapons); i++) {
        const auto& saved = m_weapons[i];
        if (saved.m_Type == WEAPON_UNARMED) {
            continue;
        }

        // The models of the weapon have to be in memory before it can be given to the ped
        const auto& info = *CWeaponInfo::GetWeaponInfo(saved.m_Type, eWeaponSkill::STD);
        for (const auto modelId : info.GetModels()) {
            if (modelId != -1) {
                CStreaming::RequestModel(modelId, STREAMING_KEEP_IN_MEMORY);
                CStreaming::LoadAllRequestedModels(false);
            }
        }

        ped->GiveWeapon(saved.m_Type, saved.m_TotalAmmo, false);
        ped->m_aWeapons[i].m_AmmoInClip = saved.m_AmmoInClip;
    }

    ped->SetCurrentWeapon((int32)m_activeWeaponSlot);
    ped->SetAreaCode((eAreaCodes)m_areaCode);

    // The entry exit is stored as its index in the pool; one that has been freed since is dropped
    ped->m_pEnex = m_nExitIndex == -1 ? nullptr : CEntryExitManager::GetPool()->GetAt(m_nExitIndex);
}

// Saves the state of the ped into this structure
// 0x5D43D0
void CPedSaveStructure::Construct(CPed* ped) {
    m_pos              = ped->GetPosition();
    m_health           = ped->m_fHealth;
    m_armor            = ped->m_fArmour;
    m_createdBy        = (ePedCreatedBy)ped->GetCreatedBy();
    m_activeWeaponSlot = ped->m_nActiveWeaponSlot;
    m_areaCode         = (int8)ped->GetAreaCode();
    m_nFightingStyle      = (uint8)ped->m_nFightingStyle;
    m_nAllowedAttackMoves = (uint8)ped->m_nAllowedAttackMoves;

    // Only an entry exit (or the one it is linked to) that belongs to an interior is worth remembering
    m_nExitIndex = -1;
    if (auto* const enex = ped->m_pEnex) {
        if (enex->GetLinkedOrThis()->m_nArea != 0) {
            m_nExitIndex = CEntryExitManager::GetPool()->GetIndex(enex);
        }
    }

    static_assert(std::size(m_weapons) == NUM_WEAPON_SLOTS);
    std::ranges::copy(ped->m_aWeapons, m_weapons);
}
