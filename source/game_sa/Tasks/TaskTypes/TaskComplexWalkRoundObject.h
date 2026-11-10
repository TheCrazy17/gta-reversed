#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"
#include "Vector.h"

class CPointRoute;

class NOTSA_EXPORT_VTABLE CTaskComplexWalkRoundObject : public CTaskComplex {
public:
    int32        m_moveState;
    CVector      m_targetPoint;
    CEntity*     m_object;
    CPointRoute* m_pointRoute;
    CTaskTimer   m_timer;         // Limits how long the ped may follow the route
    CVector      m_objectPos;     // State of the object when the route was made; the route is dropped when the object moves or turns
    CVector      m_objectForward;
    CVector      m_objectRight;

public:
    static constexpr auto Type = TASK_COMPLEX_WALK_ROUND_OBJECT;

    CTaskComplexWalkRoundObject(int32 moveState, const CVector& targetPoint, CEntity* object);
    ~CTaskComplexWalkRoundObject() override;

    eTaskType GetTaskType() const override { return Type; }                                                                      // 0x6550C0
    CTask* Clone() const override { return new CTaskComplexWalkRoundObject(m_moveState, m_targetPoint, m_object); }              // 0x655C10
    CTask* CreateNextSubTask(CPed* ped) override;
    CTask* CreateFirstSubTask(CPed* ped) override;
    CTask* ControlSubTask(CPed* ped) override;

    CTask* CreateRouteTask(CPed* ped);
    CTask* CreateSubTask(eTaskType taskType, CPed* ped);

    //! Fills `m_pointRoute` with the way round the object to `m_targetPoint`.
    //! @return Signed distance of the ped to the side of the bounding box it is closest to (positive outside the box)
    float ComputeRoute(CPed* ped);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    // 0x655020
    CTaskComplexWalkRoundObject* Constructor(int32 moveState, const CVector& targetPoint, CEntity* object) {
        this->CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(moveState, targetPoint, object);
        return this;
    }

    // 0x6550D0
    CTaskComplexWalkRoundObject* Destructor() {
        this->CTaskComplexWalkRoundObject::~CTaskComplexWalkRoundObject();
        return this;
    }
};
VALIDATE_SIZE(CTaskComplexWalkRoundObject, 0x54);
