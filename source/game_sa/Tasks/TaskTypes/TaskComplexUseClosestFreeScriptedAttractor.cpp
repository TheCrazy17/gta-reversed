#include "StdInc.h"
#include "TaskComplexUseClosestFreeScriptedAttractor.h"

#include "TaskComplexUseEffect.h"
#include "TaskComplexUseEffectRunning.h"
#include "TaskComplexUseEffectSprinting.h"
#include "Scripts/Scripted2dEffects.h"
#include "PedAttractorManager.h"
#include "InterestingEvents.h"

void CTaskComplexUseClosestFreeScriptedAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexUseClosestFreeScriptedAttractor, 0x86e428, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6346F0);
    RH_ScopedInstall(Destructor, 0x634720);

    RH_ScopedGlobalInstall(ComputeClosestFreeScriptedEffect, 0x634740);

    RH_ScopedVMTInstall(Clone, 0x636F70);
    RH_ScopedVMTInstall(GetTaskType, 0x634710);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x634730);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x639530);
    RH_ScopedVMTInstall(ControlSubTask, 0x634890);
}

CTaskComplexUseClosestFreeScriptedAttractor::CTaskComplexUseClosestFreeScriptedAttractor(eMoveState ms) :
    m_MoveState{ms}
{
}

CTaskComplexUseClosestFreeScriptedAttractor::CTaskComplexUseClosestFreeScriptedAttractor(const CTaskComplexUseClosestFreeScriptedAttractor&) :
    CTaskComplexUseClosestFreeScriptedAttractor{}
{
}

// 0x634740
C2dEffect* CTaskComplexUseClosestFreeScriptedAttractor::ComputeClosestFreeScriptedEffect(CPed const& ped) {
    C2dEffect* closest      = nullptr;
    auto       closestDstSq = std::numeric_limits<float>::max();

    for (auto i = 0u; i < NUM_SCRIPTED_2D_EFFECTS; i++) {
        if (!CScripted2dEffects::ms_activated[i]) {
            continue;
        }

        const auto& userList = CScripted2dEffects::ms_userLists[i];
        auto* const effect   = &CScripted2dEffects::ms_effects[i];

        auto matches = !userList.m_bUseList;
        if (!matches) {
            matches = rng::any_of(userList.m_UserTypes, [&](int32 id) { return id == ped.m_nModelIndex; });
        }
        if (!matches) {
            for (auto j = 0u; j < userList.m_UserTypes.size(); j++) {
                if (userList.m_UserTypes[j] == -2 && userList.m_UserTypesByPedType[j] == ped.m_nPedType) {
                    matches = true;
                    break;
                }
            }
        }
        if (!matches) {
            continue;
        }

        const auto dstSq = (ped.GetPosition() - effect->m_Pos).SquaredMagnitude();
        if (dstSq < closestDstSq && GetPedAttractorManager()->HasEmptySlot(notsa::cast<C2dEffectPedAttractor>(effect), nullptr)) {
            closestDstSq = dstSq;
            closest      = effect;
        }
    }
    return closest;
}

// 0x639530
CTask* CTaskComplexUseClosestFreeScriptedAttractor::CreateFirstSubTask(CPed* ped) {
    auto* const fx = notsa::cast<C2dEffectPedAttractor>(ComputeClosestFreeScriptedEffect(*ped));
    if (!fx) {
        return nullptr;
    }

    g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_3, ped);

    switch (m_MoveState) {
    case PEDMOVE_RUN:
        return new CTaskComplexUseEffectRunning{ fx, nullptr };
    case PEDMOVE_SPRINT:
        return new CTaskComplexUseEffectSprinting{ fx, nullptr };
    default:
        return new CTaskComplexUseEffect{ fx, nullptr };
    }
}
