#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"
#include "Vector.h"
#include <extensions/EntityRef.hpp>

class CColSphere;
class CEntity;

/// Walks round an entity (a parked car for example) to a point on the other side of it, looking the way it goes
class NOTSA_EXPORT_VTABLE CTaskComplexAvoidEntity : public CTaskComplex {
public:
    using EntitiesToAvoidArray = std::array<CEntity*, 16>;

    static constexpr auto Type = TASK_COMPLEX_AVOID_ENTITY;

    static void InjectHooks();

    CTaskComplexAvoidEntity(eMoveState moveState, CEntity* entityToAvoid, const CVector& targetPoint);
    ~CTaskComplexAvoidEntity() override = default;

    CTask*    Clone() const override { return new CTaskComplexAvoidEntity{ m_MoveState, m_EntityToAvoid, m_TargetPt }; } // 0x66D0C0
    eTaskType GetTaskType() const override { return Type; }                                                                // 0x66AAD0
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

    void QuitIK(CPed* ped);
    void SetUpIK(CPed* ped);

    /// Bounding sphere (round the XY centre of the entities, z stays 0) that covers the entities in the array
    void ComputeSphere(CColSphere& outSphere, EntitiesToAvoidArray& entities);
    bool ComputeRouteRoundSphere(CPed* ped);

private: // Wrappers for hooks
    // 0x66AA20
    CTaskComplexAvoidEntity* Constructor(eMoveState moveState, CEntity* entityToAvoid, const CVector& targetPoint) {
        this->CTaskComplexAvoidEntity::CTaskComplexAvoidEntity(moveState, entityToAvoid, targetPoint);
        return this;
    }

    // 0x66AAE0
    CTaskComplexAvoidEntity* Destructor() {
        this->CTaskComplexAvoidEntity::~CTaskComplexAvoidEntity();
        return this;
    }

private:
    notsa::EntityRef<CEntity> m_EntityToAvoid{};
    eMoveState                m_MoveState{};
    CVector                   m_StartPt{};
    CVector                   m_TargetPt{};
    CVector                   m_DetourTargetPt{}; //< Where to walk to; the target itself or a point round the entity
    CVector                   m_NewTargetPt{};
    CTaskTimer                m_Timer{};          //< Never started by this task
    bool                      m_IsDoingIK : 1{};
};
VALIDATE_SIZE(CTaskComplexAvoidEntity, 0x54);
