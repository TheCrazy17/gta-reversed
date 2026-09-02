#include "StdInc.h"

#include "ScriptResourceManager.h"

void CScriptResourceManager::InjectHooks() {
    RH_ScopedClass(CScriptResourceManager);
    RH_ScopedCategory("Scripts");

    RH_ScopedInstall(Initialise, 0x470480);
    RH_ScopedInstall(AddToResourceManager, 0x4704B0);
    RH_ScopedInstall(RemoveFromResourceManager, 0x470510);
    RH_ScopedInstall(HasResourceBeenRequested, 0x470620);
    //RH_ScopedInstall(Load, 0x0, { .reversed = false });
    //RH_ScopedInstall(Save, 0x0, { .reversed = false });
}

// 0x470480
void CScriptResourceManager::Initialise() {
    m_aScriptResources.fill(tScriptResource());
}

// 0x4704B0
void CScriptResourceManager::AddToResourceManager(int32 modelId, eScriptResourceType type, CRunningScript* script) {
    int32 freeSlot = -1;
    for (auto i = 0u; i < m_aScriptResources.size(); i++) {
        auto& res = m_aScriptResources[i];
        if (res.m_nModelId == modelId && res.m_nType == type && res.m_pThread == script) {
            return; // Already registered
        }
        if (res.m_nType == RESOURCE_TYPE_DEFAULT) {
            freeSlot = (int32)i;
        }
    }
    if (freeSlot != -1) {
        auto& res = m_aScriptResources[freeSlot];
        res.m_nModelId = modelId;
        res.m_nType    = type;
        res.m_pThread  = script;
    }
}

// 0x470510
bool CScriptResourceManager::RemoveFromResourceManager(int32 modelId, eScriptResourceType type, CRunningScript* script) {
    int32 matchedSlot     = -1;
    int32 numOtherHolders = 0;
    for (auto i = 0u; i < m_aScriptResources.size(); i++) {
        auto& res = m_aScriptResources[i];
        if (res.m_nModelId == modelId && res.m_nType == type) {
            if (res.m_pThread == script) {
                matchedSlot = (int32)i;
            } else {
                numOtherHolders++;
            }
        }
    }
    if (matchedSlot != -1) {
        auto& res = m_aScriptResources[matchedSlot];
        res.m_nModelId = -1;
        res.m_nType    = RESOURCE_TYPE_DEFAULT;
        // NOTSA: original doesn't clear m_pThread here either, leaving a stale pointer in the
        // freed slot - harmless since m_nType == RESOURCE_TYPE_DEFAULT marks it reusable, and
        // AddToResourceManager overwrites all 3 fields on reuse.
    }
    return numOtherHolders == 0;
}

// 0x470620
bool CScriptResourceManager::HasResourceBeenRequested(int32 modelId, eScriptResourceType type) {
    return rng::any_of(m_aScriptResources, [=](const auto& res) {
        return res.m_nModelId == modelId && res.m_nType == type;
    });
}

// 0x0
bool CScriptResourceManager::Load() {
    assert(false);
    return true;
}

// 0x0
bool CScriptResourceManager::Save() {
    assert(false);
    return true;
}
