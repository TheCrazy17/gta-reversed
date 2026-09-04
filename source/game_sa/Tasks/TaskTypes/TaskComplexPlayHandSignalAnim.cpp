#include "StdInc.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexSequence.h"
#include "TaskTypes/TaskSimpleStandStill.h"
#include "TaskTypes/TaskSimplePlayHandSignalAnim.h"
#include "Streaming.h"
#include "Animation/AnimManager.h"
#include "Models/ModelInfo.h"

void CTaskComplexPlayHandSignalAnim::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPlayHandSignalAnim, 0x86d5dc, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61B2B0);
    RH_ScopedInstall(Destructor, 0x61BDF0);

    RH_ScopedInstall(GetAnimIdForPed, 0x61B460);
    RH_ScopedInstall(CreateSubTask, 0x61B2F0);

    RH_ScopedVMTInstall(Clone, 0x61BA00);
    RH_ScopedVMTInstall(GetTaskType, 0x61B2E0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x61B570);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x61B4F0);
    RH_ScopedVMTInstall(ControlSubTask, 0x61B580);
}

// 0x61B2B0
CTaskComplexPlayHandSignalAnim::CTaskComplexPlayHandSignalAnim(AnimationId animationId, float blendFactor) :
    m_animationId{animationId},
    m_AnimBlenDelta{blendFactor}
{
}

// 0x61BDF0
CTaskComplexPlayHandSignalAnim::~CTaskComplexPlayHandSignalAnim() {
    enum {
        RIGHT,
        LEFT,
    };
    enum {
        NONFAT,
        FAT
    };
    const eModelID handModels[2][2]{
        // nonfat           fat
        { MODEL_SHANDL, MODEL_FHANDL }, // left
        { MODEL_SHANDR, MODEL_FHANDR }  // right
    };

    // Remove hand model refs
    for (const auto i : { LEFT, RIGHT }) {
        CModelInfo::GetModelInfo(handModels[i][m_DoUseFatHands ? FAT : NONFAT])->RemoveRef();
    }

    // Deal with anim
    if (m_bAnimationLoaded) { // Remove anim ref
        CAnimManager::RemoveAnimBlockRef(ms_animBlock);
    } else if (ms_animBlock != -1 && !CAnimManager::GetAnimBlocks()[ms_animBlock].RefCnt) { 
        if (!rng::all_of(std::array{ LEFT, RIGHT }, [&, this](auto i) { // Unload anim block if not all of the models has refs
            return CModelInfo::GetModelInfo(handModels[i][m_DoUseFatHands ? FAT : NONFAT])->m_nRefCount != 0;
        })) {
            CStreaming::RemoveModel(IFPToModelId(ms_animBlock));        
        }
    }
}

// 0x61B460
AnimationId CTaskComplexPlayHandSignalAnim::GetAnimIdForPed(CPed* ped) {
    switch (ped->m_nPedType) {
    case PED_TYPE_GANG1: return ANIM_ID_GSIGN1; // Ballas
    case PED_TYPE_GANG2: return ANIM_ID_GSIGN2; // Grove Street Families
    case PED_TYPE_GANG3: return ANIM_ID_GSIGN3; // Los Santos Vagos
    case PED_TYPE_GANG5: return ANIM_ID_GSIGN5; // Da Nang Boys
    case PED_TYPE_GANG8: return ANIM_ID_GSIGN4; // Varrio Los Aztecas
    default:             return ANIM_ID_UNDEFINED;
    }
}

// 0x61B2F0
CTask* CTaskComplexPlayHandSignalAnim::CreateSubTask(eTaskType taskType) {
    if (taskType == CTaskSimpleStandStill::Type) {
        return new CTaskSimpleStandStill(0, true, false, 8.0f);
    }

    if (taskType == CTaskSimplePlayHandSignalAnim::Type) {
        auto* sequence = new CTaskComplexSequence();
        sequence->AddTask(new CTaskSimpleStandStill(CGeneral::GetRandomNumberInRange(0, 1500), false, false, 8.0f));
        sequence->AddTask(new CTaskSimplePlayHandSignalAnim(m_animationId, m_AnimBlenDelta, m_DoUseFatHands, false));
        return sequence;
    }

    return nullptr;
}

// 0x61BA00
CTask* CTaskComplexPlayHandSignalAnim::Clone() const {
    auto* clone = new CTaskComplexPlayHandSignalAnim();
    clone->m_animationId = m_animationId;
    clone->m_AnimBlenDelta = m_AnimBlenDelta;
    return clone;
}

// 0x61B570
CTask* CTaskComplexPlayHandSignalAnim::CreateNextSubTask(CPed* ped) {
    return nullptr;
}

// 0x61B4F0
CTask* CTaskComplexPlayHandSignalAnim::CreateFirstSubTask(CPed* ped) {
    if (IKChainManager_c::IsArmPointing(IK_ARM_LEFT, ped)) {
        return nullptr;
    }

    if (m_animationId == ANIM_ID_UNDEFINED) {
        m_animationId = GetAnimIdForPed(ped);
        if (m_animationId == ANIM_ID_UNDEFINED) {
            return nullptr;
        }
    }

    m_DoUseFatHands = ped->GetModelId() == MODEL_BALLAS2 || ped->GetModelId() == MODEL_FAM1 || ped->GetModelId() == MODEL_FAM3;
    return CreateSubTask(TASK_SIMPLE_STAND_STILL);
}

// 0x61B580
CTask* CTaskComplexPlayHandSignalAnim::ControlSubTask(CPed* ped) {
    const auto RequestHand = [](eModelID model, bool alreadyLoaded) {
        if (CStreaming::IsModelLoaded(model)) {
            if (!alreadyLoaded) {
                CModelInfo::GetModelInfo(model)->AddRef();
            }
            return true;
        }
        CStreaming::RequestModel(model, STREAMING_KEEP_IN_MEMORY);
        return alreadyLoaded;
    };

    if (!m_DoUseFatHands) {
        m_bLeftHandLoaded  = RequestHand(MODEL_SHANDL, m_bLeftHandLoaded);
        m_bRightHandLoaded = RequestHand(MODEL_SHANDR, m_bRightHandLoaded);
    } else {
        m_bLeftHandLoaded  = RequestHand(MODEL_FHANDL, m_bLeftHandLoaded);
        m_bRightHandLoaded = RequestHand(MODEL_FHANDR, m_bRightHandLoaded);
    }

    if (m_bLeftHandLoaded && m_bRightHandLoaded) {
        if (ms_animBlock == ~0u) {
            ms_animBlock = (uint32)CAnimManager::GetAnimationBlockIndex("ghands");
        }
        if (CStreaming::IsModelLoaded(IFPToModelId(ms_animBlock))) {
            if (!m_bAnimationLoaded) {
                CAnimManager::AddAnimBlockRef(ms_animBlock);
                m_bAnimationLoaded = true;
            }
        } else {
            CStreaming::RequestModel(IFPToModelId(ms_animBlock), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (m_bAnimationLoaded && m_pSubTask->GetTaskType() == CTaskSimpleStandStill::Type) {
        return CreateSubTask(CTaskSimplePlayHandSignalAnim::Type);
    }
    return m_pSubTask;
}
