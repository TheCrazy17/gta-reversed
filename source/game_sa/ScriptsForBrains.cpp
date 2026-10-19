#include "StdInc.h"

#include "ScriptsForBrains.h"
#include "Entity/Object/Object.h"

void CScriptsForBrains::InjectHooks() {
    RH_ScopedClass(CScriptsForBrains);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Init, 0x46A8C0);
    RH_ScopedInstall(SwitchAllObjectBrainsWithThisID, 0x46A900);
    RH_ScopedInstall(AddNewScriptBrain, 0x46A930);
    RH_ScopedInstall(AddNewStreamedScriptBrainForCodeUse, 0x46A9C0);
    RH_ScopedInstall(GetIndexOfScriptBrainWithThisName, 0x46AA30);
    RH_ScopedInstall(HasAttractorScriptBrainWithThisNameLoaded, 0x46AB20);
    RH_ScopedInstall(StartNewStreamedScriptBrain, 0x46B270);
    RH_ScopedInstall(StartAttractorScriptBrainWithThisName, 0x46B390);
    RH_ScopedInstall(StartOrRequestNewStreamedScriptBrain, 0x46CD80);
    //RH_ScopedInstall(StartOrRequestNewStreamedScriptBrainWithThisName, 0x46CED0, {.Reversed = false});
    RH_ScopedInstall(IsObjectWithinBrainActivationRange, 0x46B3D0);
    RH_ScopedInstall(CheckIfNewEntityNeedsScript, 0x46FF20);
    RH_ScopedInstall(MarkAttractorScriptBrainWithThisNameAsNoLongerNeeded, 0x46AAE0);
    RH_ScopedInstall(RequestAttractorScriptBrainWithThisName, 0x46AA80);
}


// 0x46A8C0
void CScriptsForBrains::Init() {
    for (auto& script : m_aScriptForBrains) {
        script = tScriptForBrains();
    }
}

// 0x46A900
void CScriptsForBrains::SwitchAllObjectBrainsWithThisID(int8 ID, bool bStatus) {
    if (ID < 0) {
        return;
    }
    for (auto& scriptBrain : m_aScriptForBrains) {
        if (scriptBrain.m_ObjectGroupingId == ID) {
            scriptBrain.m_bBrainActive = bStatus;
        }
    }
}

// 0x46A930
void CScriptsForBrains::AddNewScriptBrain(int16 ImgIndex, int16 Model, uint16 priority, int8 attachType, int8 Type, float Radius) {
    for (auto& scriptBrain : m_aScriptForBrains) {
        if (scriptBrain.m_StreamedScriptIndex != -1) {
            continue;
        }
        scriptBrain.m_StreamedScriptIndex           = ImgIndex;
        scriptBrain.m_PedModelOrPedGeneratorIndex   = Model;
        scriptBrain.m_PercentageChance              = priority;
        scriptBrain.m_TypeOfBrain                   = attachType;
        scriptBrain.m_ObjectGroupingId              = Type;
        scriptBrain.m_bBrainActive                  = true;
        scriptBrain.m_ObjectBrainActivationRadius   = Radius < 0.0f ? 5.0f : Radius; // A radius of exactly 0 is kept
        return;
    }
}

// 0x46A9C0
void CScriptsForBrains::AddNewStreamedScriptBrainForCodeUse(int16 ImgIndex, char* name, int8 attachType) {
    for (auto& scriptBrain : m_aScriptForBrains) {
        if (scriptBrain.m_StreamedScriptIndex != -1) {
            continue;
        }
        scriptBrain.m_StreamedScriptIndex         = ImgIndex;
        scriptBrain.m_TypeOfBrain                 = attachType;
        scriptBrain.m_ObjectGroupingId            = -1;
        scriptBrain.m_bBrainActive                = true;
        scriptBrain.m_ObjectBrainActivationRadius = 5.0f;
        strcpy(scriptBrain.m_ScriptName, name);
        return;
    }
}

void CScriptsForBrains::CheckIfNewEntityNeedsScript(CEntity* entity, int8 attachType, void* unused) {
    // Entities that already have a brain, or are waiting for it to load, are left alone
    if (attachType == 0) {
        auto* const ped = entity->AsPed();
        if (ped->bHasAScriptBrain || ped->bWaitingForScriptBrainToLoad) {
            return;
        }
    } else if (entity->AsObject()->objectFlags.b0x100000_0x200000) {
        return;
    }

    for (uint8 i = 0; i < m_aScriptForBrains.size(); i++) {
        const auto& script = m_aScriptForBrains[i];
        if (script.m_TypeOfBrain != attachType || script.m_PedModelOrPedGeneratorIndex != entity->m_nModelIndex) {
            continue;
        }

        // The chance is a percentage
        const auto roll = static_cast<int32>(static_cast<float>(CGeneral::GetRandomNumber() & 0xFFFF) * (1.0f / 32768.0f) * 100.0f);
        if (roll < script.m_PercentageChance) {
            StartOrRequestNewStreamedScriptBrain(i, entity, attachType, true);
            return;
        }
    }
}

void CScriptsForBrains::MarkAttractorScriptBrainWithThisNameAsNoLongerNeeded(const char* name) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
        auto& script = CTheScripts::StreamedScripts.m_aScripts[m_aScriptForBrains[idx].m_StreamedScriptIndex];
        if (script.m_NumberOfUsers != 0) {
            script.m_NumberOfUsers--;
        }
    }
}

void CScriptsForBrains::RequestAttractorScriptBrainWithThisName(const char* name) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
        const auto scriptIdx = m_aScriptForBrains[idx].m_StreamedScriptIndex;
        if (!CStreaming::IsModelLoaded(SCMToModelId(scriptIdx))) {
            CStreaming::RequestModel(SCMToModelId(scriptIdx), STREAMING_PRIORITY_REQUEST);
        }
        CTheScripts::StreamedScripts.m_aScripts[scriptIdx].m_NumberOfUsers++;
    }
}

void CScriptsForBrains::StartNewStreamedScriptBrain(uint8 index, CEntity* entity, bool bHasAScriptBrain) {
    const auto& brain  = m_aScriptForBrains[index];
    auto* const script = CTheScripts::StreamedScripts.StartNewStreamedScript(brain.m_StreamedScriptIndex);
    script->m_ExternalType = brain.m_TypeOfBrain;

    switch (brain.m_TypeOfBrain) {
    case 0:
    case 3:
    case 5: {
        auto* const ped = entity->AsPed();
        script->m_LocalVars[0].iParam = GetPedPool()->GetRef(ped);
        ped->bHasAScriptBrain = true;
        if (brain.m_TypeOfBrain == 5) {
            script->m_LocalVars[1].iParam = bHasAScriptBrain;
        }
        ped->bWaitingForScriptBrainToLoad = false;
        CTheScripts::RemoveFromWaitingForScriptBrainArray(ped, ped->m_StreamedScriptBrainToLoad);
        ped->m_StreamedScriptBrainToLoad = -1;
        break;
    }
    case 1:
    case 4: {
        auto* const object = entity->AsObject();
        script->m_LocalVars[0].iParam = GetObjectPool()->GetRef(object);
        object->objectFlags.b0x100000_0x200000 = 3;
        break;
    }
    default:
        break;
    }
}

void CScriptsForBrains::StartOrRequestNewStreamedScriptBrain(uint8 index, CEntity* entity, int8 attachType, bool bAddToWaitingArray) {
    const auto& brain = m_aScriptForBrains[index];

    if (bAddToWaitingArray) {
        if (!brain.m_bBrainActive) {
            return;
        }
        switch (attachType) {
        case 0:
        case 3: {
            const auto* const ped = entity->AsPed();
            if (ped->bHasAScriptBrain || ped->bWaitingForScriptBrainToLoad) {
                return;
            }
            break;
        }
        case 1:
        case 4:
            if (entity->AsObject()->objectFlags.b0x100000_0x200000) {
                return;
            }
            break;
        default:
            break;
        }
    }

    if (attachType == 1) {
        auto* const object = entity->AsObject();
        if (!bAddToWaitingArray) {
            object->objectFlags.b0x100000_0x200000 = 2;
        } else {
            object->m_nStreamedScriptBrainToLoad = index;
            object->objectFlags.b0x100000_0x200000           = 1;
            CTheScripts::AddToWaitingForScriptBrainArray(object, index);
        }
    }

    if (!CStreaming::IsModelLoaded(SCMToModelId(brain.m_StreamedScriptIndex))) {
        CStreaming::RequestModel(SCMToModelId(brain.m_StreamedScriptIndex), STREAMING_MISSION_REQUIRED);

        const bool isPed = attachType == 0 || (attachType > 2 && attachType < 5);
        if (isPed && bAddToWaitingArray) {
            auto* const ped = entity->AsPed();
            ped->m_StreamedScriptBrainToLoad = index;
            ped->bWaitingForScriptBrainToLoad = true;
            CTheScripts::AddToWaitingForScriptBrainArray(ped, index);
        }
        return;
    }
    StartNewStreamedScriptBrain(index, entity, false);
}

// 0x46CED0
void CScriptsForBrains::StartOrRequestNewStreamedScriptBrainWithThisName(const char* name, CEntity* entity, int8 attachType) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, attachType); idx >= 0) {
        StartOrRequestNewStreamedScriptBrain(static_cast<uint8>(idx), entity, attachType, true);
    }
}

bool CScriptsForBrains::HasAttractorScriptBrainWithThisNameLoaded(const char* name) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
        return CStreaming::IsModelLoaded(SCMToModelId(m_aScriptForBrains[idx].m_StreamedScriptIndex));
    }
    return false;
}

bool CScriptsForBrains::IsObjectWithinBrainActivationRange(CObject* entity, const CVector& point) {
    const auto& script = m_aScriptForBrains[entity->m_nStreamedScriptBrainToLoad];
    if (script.m_TypeOfBrain != 1) {
        return false;
    }
    return CVector::Dist(point, entity->GetPosition()) < script.m_ObjectBrainActivationRadius;
}

int16 CScriptsForBrains::GetIndexOfScriptBrainWithThisName(const char* name, int8 type) {
    const auto it = rng::find_if(m_aScriptForBrains, [=](tScriptForBrains& script) {
        return script.m_TypeOfBrain == type && !_stricmp(script.m_ScriptName, name);
    });
    return it != m_aScriptForBrains.end()
        ? rng::distance(m_aScriptForBrains.begin(), it)
        : -1;
}

void CScriptsForBrains::StartAttractorScriptBrainWithThisName(const char* name, CPed* ped, bool bHasAScriptBrain) {
    if (!ped->bWaitingForScriptBrainToLoad && !ped->bHasAScriptBrain) {
        if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
            StartNewStreamedScriptBrain(static_cast<uint8>(idx), ped, bHasAScriptBrain);
        }
    }
}
