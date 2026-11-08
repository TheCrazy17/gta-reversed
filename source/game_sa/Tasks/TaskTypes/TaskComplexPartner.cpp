#include "StdInc.h"

#include "TaskComplexPartner.h"

void CTaskComplexPartner::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartner, 0x870664, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x681E70);
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x682310);
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370);
}

// 0x681E70
CTaskComplexPartner::CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point)
    : CTaskComplex()
{
    m_leadSpeaker              = leadSpeaker;
    m_makePedAlwaysFacePartner = makePedAlwaysFacePartner;
    m_distanceMultiplier       = distanceMultiplier;
    m_updateDirectionCount     = updateDirectionCount;
    m_point                    = point;
    m_partner                  = partner;
    m_partnerState             = PARTNER_STATE_UNK_1;
    m_taskCompleted            = false;
    m_firstToTargetFlag        = -1;
    m_requiredAnimsStreamedIn  = false;
    m_animBlockName[0]         = '\0';
    CEntity::SafeRegisterRef(m_partner);
}

/*!
 * @addr 0x683A40
 */
CTaskComplexPartner::~CTaskComplexPartner() {
    CEntity::SafeCleanUpRef(m_partner);

    if (m_requiredAnimsStreamedIn) {
        if (strcmp(m_animBlockName, "") != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
        }
        m_requiredAnimsStreamedIn = false;
    }
}

CTaskComplexPartner* CTaskComplexPartner::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point) {
    this->CTaskComplexPartner::CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, makePedAlwaysFacePartner, updateDirectionCount, point);
    return this;
}

CTask* CTaskComplexPartner::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x683AD0, CTaskComplexPartner*, CPed*>(this, ped);
}

CTask* CTaskComplexPartner::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x681F20, CTaskComplexPartner*, CPed*>(this, ped);
}

CTask* CTaskComplexPartner::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x6840D0, CTaskComplexPartner*, CPed*>(this, ped);
}

// 0x682310
void CTaskComplexPartner::StreamRequiredAnims() {
    if (m_requiredAnimsStreamedIn) {
        return;
    }

    // A task without an anim block has nothing to stream; any other waits until its block is loaded
    if (m_animBlockName[0]) {
        const auto blockIdx = CAnimManager::GetAnimationBlockIndex(m_animBlockName);
        if (!CAnimManager::GetAnimBlocks()[blockIdx].IsLoaded) {
            CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
            return;
        }
        CAnimManager::AddAnimBlockRef(blockIdx);
    }
    m_requiredAnimsStreamedIn = true;
}

// 0x682370
void CTaskComplexPartner::RemoveStreamedAnims() {
    if (!m_requiredAnimsStreamedIn) {
        return;
    }

    if (m_animBlockName[0]) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
    }
    m_requiredAnimsStreamedIn = false;
}
