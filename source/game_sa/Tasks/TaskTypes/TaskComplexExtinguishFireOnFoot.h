#pragma once

#include "TaskComplex.h"
#include "Vector.h"

class CPed;
class CFire;

/// Ped runs to the closest (non script) fire and puts it out with a gun control task
class NOTSA_EXPORT_VTABLE CTaskComplexExtinguishFireOnFoot : public CTaskComplex {
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_EXTINGUISH_FIRE_ON_FOOT;

    static void InjectHooks();

    explicit CTaskComplexExtinguishFireOnFoot(const CVector& firePos);
    ~CTaskComplexExtinguishFireOnFoot() override = default;

    //! @return The closest fire if it is closer than 10 units to the ped
    CFire* FindNearbyFire(CPed* ped);

    CTask*    Clone() const override { return new CTaskComplexExtinguishFireOnFoot{m_FirePos}; } // 0x659D70
    eTaskType GetTaskType() const override { return Type; }                                        // 0x6598A0
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override { return m_pSubTask; } // 0x659980

private: // Wrappers for hooks
    // 0x659870
    CTaskComplexExtinguishFireOnFoot* Constructor(const CVector& firePos) {
        this->CTaskComplexExtinguishFireOnFoot::CTaskComplexExtinguishFireOnFoot(firePos);
        return this;
    }

private:
    CVector m_FirePos{}; //< Position of the fire to extinguish; overwritten by `CreateFirstSubTask` with the fire it found
};
VALIDATE_SIZE(CTaskComplexExtinguishFireOnFoot, 0x18);
