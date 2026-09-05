#include "StdInc.h"

#include "TaskGangHassleVehicle.h"

void CTaskGangHassleVehicle::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHassleVehicle, 0x86F9D4, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FAC0);
    RH_ScopedInstall(Destructor, 0x65FB60);
    RH_ScopedInstall(GetTargetHeading, 0x65FDD0);
    RH_ScopedInstall(CalcTargetOffset, 0x6641A0);
    RH_ScopedInstall(Clone, 0x65FC00);
    RH_ScopedInstall(CreateNextSubTask, 0x65FC80, { .reversed = false });
    RH_ScopedInstall(CreateFirstSubTask, 0x664BA0, { .reversed = false });
    RH_ScopedInstall(ControlSubTask, 0x6637C0, { .reversed = false });
}

// 0x65FAC0
CTaskGangHassleVehicle::CTaskGangHassleVehicle(CVehicle* vehicle, int32 a3, uint8 a4, float a5, float a6) : CTaskComplex() {
    m_nTime = 0;
    dword3C = 0;
    byte40 = 0;
    byte41 = 0;
    byte18 = a4;
    dword1C = a5;
    m_Vehicle = vehicle;
    m_nHasslePosId = -1;
    m_fOffsetX = a6;
    m_bRemoveAnim = 0;
    m_pEntity = nullptr;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x65FB60
CTaskGangHassleVehicle::~CTaskGangHassleVehicle() {
    if (m_Vehicle) {
        if (m_nHasslePosId > -1) {
            m_Vehicle->SetHasslePosId(m_nHasslePosId, false);
        }
        CEntity::SafeCleanUpRef(m_Vehicle);
    }

    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bRemoveAnim) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }
}

// 0x65FDD0
float CTaskGangHassleVehicle::GetTargetHeading(CPed* ped) {
    const auto& matrix = m_Vehicle->GetMatrix();

    float dx, dy;
    switch (m_nHasslePosId) {
    case 1:
    case 3:
        dx = -matrix.GetRight().x;
        dy = -matrix.GetRight().y;
        break;
    case 4:
        dx = matrix.GetUp().x;
        dy = matrix.GetUp().y;
        break;
    case 5:
        dx = -matrix.GetUp().x;
        dy = -matrix.GetUp().y;
        break;
    default: // 0, 2
        dx = matrix.GetRight().x;
        dy = matrix.GetRight().y;
        break;
    }

    return CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(dx, dy, 0.0f, 0.0f));
}

// 0x6641A0
void CTaskGangHassleVehicle::CalcTargetOffset() {
    m_vecPosn = CVector{};

    const auto& box = m_Vehicle->GetModelInfo()->GetColModel()->GetBoundingBox();
    switch (m_nHasslePosId) {
    case 0:
        m_vecPosn.x = box.m_vecMin.x - m_fOffsetX;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 1:
        m_vecPosn.x = box.m_vecMax.x + m_fOffsetX;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 2:
        m_vecPosn.x = box.m_vecMin.x - m_fOffsetX;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 3:
        m_vecPosn.x = box.m_vecMax.x + m_fOffsetX;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 4:
        m_vecPosn.y = box.m_vecMin.y - m_fOffsetX;
        break;
    case 5:
        m_vecPosn.y = box.m_vecMax.y + m_fOffsetX;
        break;
    }
}

// 0x65FC80
CTask* CTaskGangHassleVehicle::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x65FC80, CTaskGangHassleVehicle*, CPed*>(this, ped);
}

// 0x664BA0
CTask* CTaskGangHassleVehicle::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x664BA0, CTaskGangHassleVehicle*, CPed*>(this, ped);
}

// 0x6637C0
CTask* CTaskGangHassleVehicle::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x6637C0, CTaskGangHassleVehicle*, CPed*>(this, ped);
}
