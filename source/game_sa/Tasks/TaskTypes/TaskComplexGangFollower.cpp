#include "StdInc.h"

#include "TaskComplexGangFollower.h"
#include "TaskSimpleCarDrive.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleStandStill.h"
#include "SeekEntity/TaskComplexSeekEntity.h"

void CTaskComplexGangFollower::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGangFollower, 0x86F938, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65EAA0);
    RH_ScopedInstall(Destructor, 0x65EBB0);
    //RH_ScopedInstall(CalculateOffsetPosition, 0x65ED40, { .Reversed = false }); // not hooked because i want to keep CVector return, but original function took a CVector&
    RH_ScopedInstall(Clone, 0x65ECB0);
    RH_ScopedInstall(MakeAbortable, 0x65EC30);
    RH_ScopedInstall(CreateNextSubTask, 0x665E00, { .Reversed = false });
    RH_ScopedInstall(CreateFirstSubTask, 0x666160);
    RH_ScopedInstall(ControlSubTask, 0x662A10, { .Reversed = false });
}

// 0x65EAA0
CTaskComplexGangFollower::CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6) : CTaskComplex() {
    m_PedGroup = pedGroup;
    m_Leader   = ped;
    byte3C     = a4;
    dword2C    = pos;
    dword20    = pos;
    dword38    = a6;

    dword40 = 0;
    dword44 = 0;
    byte48  = 0;
    byte49  = 0;

    m_Flags |= 0x14;

    if (m_Leader) {
        CEntity::RegisterReference(m_Leader);
        m_PedPosn = m_Leader->GetPosition();
    }

    m_Flags &= ~3;
    if (m_Leader == FindPlayerPed(0)) {
        m_Flags |= 8;
    } else {
        m_Flags &= ~8;
    }
}


// 0x65EBB0
CTaskComplexGangFollower::~CTaskComplexGangFollower() {
    CEntity::SafeCleanUpRef(m_Leader);

    if (m_Flags & 1) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_Flags &= ~1;
    }
}

// 0x65ED40
CVector CTaskComplexGangFollower::CalculateOffsetPosition() {
    CVector ret;
    plugin::CallMethod<0x65ED40, CTaskComplexGangFollower*>(this, &ret);
    return ret;
}

// 0x65ECB0
CTask* CTaskComplexGangFollower::Clone() const {
    auto* clone = new CTaskComplexGangFollower(m_PedGroup, m_Leader, byte3C, dword20, dword38);
    clone->m_Flags = (clone->m_Flags & ~4) | (m_Flags & 4);
    return clone;
}

// 0x65EC30
bool CTaskComplexGangFollower::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_pSubTask && !m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }

    ped->bDontAcceptIKLookAts = false;
    ped->bMoveAnimSpeedHasBeenSetByTask = false;
    return true;
}

// 0x665E00
CTask* CTaskComplexGangFollower::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x665E00, CTaskComplexGangFollower*, CPed*>(this, ped);
}

// 0x666160
CTask* CTaskComplexGangFollower::CreateFirstSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    // Ride along when the leader is in the same vehicle, otherwise get out of the one the ped is in
    if (ped->bInVehicle && ped->m_pVehicle) {
        if (m_Leader->m_pVehicle == ped->m_pVehicle) {
            return new CTaskSimpleCarDrive{ ped->m_pVehicle, nullptr, false };
        }
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    }

    static auto& s_MaxStaticCounter = StaticRef<int32>(0x86C938);
    if ((m_Flags & 4) && !(ped->GetIntelligence()->m_AnotherStaticCounter > s_MaxStaticCounter)) {
        auto* const seek = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
            m_Leader,
            50'000,
            1'000,
            0.5f,
            6.0f,
            2.0f,
            false,
            true,
            CEntitySeekPosCalculatorXYOffset{ dword20 }
        };
        seek->SetMoveState(PEDMOVE_SPRINT);
        seek->SetIsTrackingEntity(true);
        return seek;
    }
    return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
}

// 0x662A10
CTask* CTaskComplexGangFollower::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x662A10, CTaskComplexGangFollower*, CPed*>(this, ped);
}
