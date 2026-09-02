#pragma once

#include "TaskComplexGoToPointAiming.h"

class CEntity;

class NOTSA_EXPORT_VTABLE CTaskComplexGoToPointShooting : public CTaskComplexGoToPointAiming {
public:
    static constexpr auto Type = TASK_COMPLEX_GO_TO_POINT_SHOOTING;

    CTaskComplexGoToPointShooting(
        eMoveState moveState,
        const CVector& movePos,
        CEntity* aimAtEntity,
        CVector aimPos,
        const float targetRadius,
        const float slowDownDist
    );
    CTaskComplexGoToPointShooting(const CTaskComplexGoToPointShooting&);
    ~CTaskComplexGoToPointShooting() override = default;

    eTaskType GetTaskType() const override { return Type; } // 0x668CC0
    CTask*    Clone() const override { return new CTaskComplexGoToPointShooting(m_moveState, m_movePos, m_aimAtEntity, m_aimPos, m_moveTargetRadius, m_slowDownDistance); } // 0x66CE10

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    // 0x668C70
    CTaskComplexGoToPointShooting* Constructor(eMoveState moveState, CVector const& movePos, CEntity* aimAtEntity, CVector aimPos, float targetRadius, float slowDownDist) {
        this->CTaskComplexGoToPointShooting::CTaskComplexGoToPointShooting(moveState, movePos, aimAtEntity, aimPos, targetRadius, slowDownDist);
        return this;
    }
};
