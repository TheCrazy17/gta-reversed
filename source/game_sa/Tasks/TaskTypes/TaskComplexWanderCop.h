#pragma once

#include "TaskComplexWander.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskTimer.h"
#include "eMoveState.h"

class CPed;

class NOTSA_EXPORT_VTABLE CTaskComplexWanderCop : public CTaskComplexWander {
public:
    CTaskComplexGoToPointAndStandStill* m_pGoToPointAndStandStillTask;
    CTaskTimer                          m_nScanForStuffTimer;
    uint32                              m_nTimePassedSinceLastLookedForCriminals;
    uint32                              m_nTimePassedSinceLastLookedForCarAlarmsAndStolenCopCars;
    CPed*                               m_pLastCriminalPedLookedFor;
    CTaskTimer                          m_nSubTaskCreatedTimer;

public:
    CTaskComplexWanderCop(eMoveState moveState, uint8 dir);
    ~CTaskComplexWanderCop() override;

    eWanderType GetWanderType() override { return WANDER_TYPE_COP; } // 0x460D50
    CTask* Clone() const override { return new CTaskComplexWanderCop(m_nMoveState, m_nDir); } // 0x460CE0
    CTask* CreateNextSubTask(CPed* ped) override; // 0x674860
    CTask* CreateFirstSubTask(CPed* ped) override; // 0x674750
    CTask* ControlSubTask(CPed* ped) override; // 0x674D80
    void ScanForStuff(CPed* ped) override; // 0x6702B0

    void LookForCarAlarms(CPed* ped); // 0x66B1B0
    void LookForStolenCopCars(CPed* ped); // 0x66B290
    void LookForCriminals(CPed* ped); // 0x66B300
    bool ShouldPursuePlayer(CPed* ped); // 0x66B160

private:
    CPed* FindFirstCriminalInSight(CPed* ped) const;

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskComplexWanderCop* Constructor(eMoveState moveState, uint8 dir);

    // 0x460D80
    CTaskComplexWanderCop* Destructor() {
        this->CTaskComplexWanderCop::~CTaskComplexWanderCop();
        return this;
    }
};

VALIDATE_SIZE(CTaskComplexWanderCop, 0x50);
