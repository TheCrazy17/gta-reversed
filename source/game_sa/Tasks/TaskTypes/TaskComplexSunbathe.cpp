#include "StdInc.h"
#include "TaskComplexSunbathe.h"

#include "Streaming.h"
#include "Animation/AnimManager.h"
#include "Animation/AnimBlock.h"

bool CanSunbathe();               // 0x632140
bool ShouldLoadSunbatheAnims();   // 0x632190

void CTaskComplexSunbathe::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSunbathe, 0x86e0ac, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x631F80);
    RH_ScopedInstall(Destructor, 0x632050);

    RH_ScopedGlobalInstall(CanSunbathe, 0x632140);
    RH_ScopedGlobalInstall(ShouldLoadSunbatheAnims, 0x632190);

    //RH_ScopedInstall(CreateSubTask, 0x638290, { .Reversed = false });

    RH_ScopedVMTInstall(Clone, 0x6366A0);
    RH_ScopedVMTInstall(GetTaskType, 0x632040);
    RH_ScopedVMTInstall(MakeAbortable, 0x6320F0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6399F0, { .Reversed = false });
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x639CB0, { .Reversed = false });
    RH_ScopedVMTInstall(ControlSubTask, 0x6381A0);

}

// 0x631f80
CTaskComplexSunbathe::CTaskComplexSunbathe(CObject* towel, bool bStartStanding) :
    m_pTowel{towel},
    m_bStartStanding{bStartStanding}
{
    if (m_pTowel) {
        CEntity::SafeRegisterRef(m_pTowel);
        m_pTowel->m_nObjectType = OBJECT_TYPE_DECORATION;
    }

    const auto LoadAnim = [](int32& idx, CAnimBlock*& blk, const char* name) {
        idx = CAnimManager::GetAnimationBlockIndex(name);
        blk = CAnimManager::GetAnimationBlock(name);
    };
    LoadAnim(m_BeachAnimBlockIndex, m_pBeachAnimBlock, "beach");
    LoadAnim(m_SunbatheAnimBlockIndex, m_pSunbatheAnimBlock, "sunbathe");

    // Refs added later in another function
}

CTaskComplexSunbathe::CTaskComplexSunbathe(const CTaskComplexSunbathe& o) :
    CTaskComplexSunbathe{ o.m_pTowel, o.m_bStartStanding }
{
}

// 0x632050
CTaskComplexSunbathe::~CTaskComplexSunbathe() {
    if (m_pTowel) {
        m_pTowel->m_nObjectType = OBJECT_TEMPORARY;
        CEntity::SafeCleanUpRef(m_pTowel);
    }

    const auto UnrefAnimBlock = [](bool& refed, int32 blkIdx) {
        if (refed) {
            CAnimManager::RemoveAnimBlockRef(blkIdx);
            refed = false;
        }
    };
    UnrefAnimBlock(m_bBeachAnimsReferenced, m_BeachAnimBlockIndex);
    UnrefAnimBlock(m_bSunbatheAnimsReferenced, m_SunbatheAnimBlockIndex);
}

// 0x6320F0
bool CTaskComplexSunbathe::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (event && (event->GetEventType() == EVENT_PED_COLLISION_WITH_PED || event->GetEventType() == EVENT_PED_COLLISION_WITH_PLAYER)) {
        return false;
    }
    if (!CTaskComplex::MakeAbortable(ped, priority, event)) {
        return false;
    }
    m_bAborted = true;
    return true;
}

// 0x62F2D0
static bool IsExtraSunnyWeather(eWeatherType weather) {
    switch (weather) {
    case WEATHER_EXTRASUNNY_LA:
    case WEATHER_EXTRASUNNY_SMOG_LA:
    case WEATHER_EXTRASUNNY_SF:
    case WEATHER_EXTRASUNNY_VEGAS:
    case WEATHER_EXTRASUNNY_COUNTRYSIDE:
    case WEATHER_EXTRASUNNY_DESERT:
        return true;
    default:
        return false;
    }
}

// 0x632140
bool CanSunbathe() {
    const auto hour = CClock::GetGameClockHours();
    if (hour < 10 || hour > 17) {
        return false;
    }

    const auto weather = CWeather::NewWeatherType;
    switch (weather) {
    case WEATHER_SUNNY_LA:
    case WEATHER_SUNNY_SMOG_LA:
    case WEATHER_SUNNY_SF:
    case WEATHER_SUNNY_VEGAS:
    case WEATHER_SUNNY_COUNTRYSIDE:
    case WEATHER_SUNNY_DESERT:
        return true;
    default:
        return IsExtraSunnyWeather(weather);
    }
}

// 0x632190
bool ShouldLoadSunbatheAnims() {
    const auto* ped = CWorld::Players[CWorld::PlayerInFocus].m_pPed;

    // Don't load the animations while the player is speeding around in a vehicle
    if (ped->bInVehicle && ped->m_pVehicle) {
        if (ped->m_pVehicle->m_vecMoveSpeed.SquaredMagnitude() > 0.04f) { // 0x863244
            return false;
        }
    }
    return true;
}

// 0x6399F0
CTask* CTaskComplexSunbathe::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x6399F0, CTaskComplexSunbathe*, CPed*>(this, ped);
}

// 0x639CB0
CTask* CTaskComplexSunbathe::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x639CB0, CTaskComplexSunbathe*, CPed*>(this, ped);
}

// 0x6381A0
CTask* CTaskComplexSunbathe::ControlSubTask(CPed* ped) {
    if (m_bAborted) {
        return nullptr;
    }

    const auto ReferenceBlock = [](CAnimBlock* block, int32& blockIndex, bool& referenced) {
        if (referenced) {
            return;
        }
        if (block->IsLoaded) {
            CAnimManager::AddAnimBlockRef(blockIndex);
            referenced = true;
        } else {
            // Anim blocks are streamed as models, with the IFP resource ids right after the DAT ones
            CStreaming::RequestModel(blockIndex + RESOURCE_ID_IFP, 8);
        }
    };

    ReferenceBlock(m_pBeachAnimBlock, m_BeachAnimBlockIndex, m_bBeachAnimsReferenced);
    if (!m_bSunbatheAnimsReferenced && ShouldLoadSunbatheAnims()) {
        ReferenceBlock(m_pSunbatheAnimBlock, m_SunbatheAnimBlockIndex, m_bSunbatheAnimsReferenced);
    }

    if (m_bBathing) {
        // NOTSA: two CPed-level bits still without name (ped+0x528 bit 3, ped+0x474 bit 20)
        *(uint32*)((char*)ped + 0x528) |= 8u;
        *(uint32*)((char*)ped + 0x474) |= 0x100000u;
    }

    if (!CanSunbathe()) {
        if (m_BathingTimer.m_bStarted) {
            m_BathingTimer.m_bStopped = true;
            m_BathingTimer.m_nInterval -= CTimer::GetTimeInMS() - m_BathingTimer.m_nStartTime;
        }
        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_SUNBATHE) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
        }
    }

    return m_pSubTask;
}
