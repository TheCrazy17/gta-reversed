#pragma once

#include "TaskComplex.h"

class CTaskComplexSequence;

// Both peds run the same task and read each other's state (see GetPartnerState) to advance together.
enum ePartnerState : int8 {
    PARTNER_STATE_NONE             = 0, // Not a state of the task, GetPartnerState returns it when the partner has no matching task
    PARTNER_STATE_INIT             = 1, // Standing still for the first sub task; the lead speaker then works out the meeting points
    PARTNER_STATE_WAIT_FOR_POINT   = 2, // Standing still until `m_point` holds the meeting point (the lead speaker writes it for the other ped)
    PARTNER_STATE_GO_TO_POINT      = 3, // Walking to `m_point`, then turning towards the partner
    PARTNER_STATE_WAIT_FOR_PARTNER = 4, // Arrived and facing the partner, waiting for the partner to arrive too
    PARTNER_STATE_ALIGN            = 5, // Both arrived; waiting until the partner is ready (or closing in on it)
    PARTNER_STATE_PERFORM          = 6, // Running the sequences from GetPartnerSequence
};

class NOTSA_EXPORT_VTABLE CTaskComplexPartner : public CTaskComplex {
public:
    int32         field_C;
    int32         field_10;
    char          m_commandName[32];
    eTaskType     m_taskId;                    // Task type of the derived class, used to find the partner's task
    CPed*         m_partner;
    float         m_distanceMultiplier;        // Where between the peds the meeting point lies (0.5 is the middle)
    CVector       m_point;                     // Where this ped goes; for the ped that is not the lead speaker it is filled in by the lead speaker
    CVector       m_targetPoint{};             // Where the partner goes; the exe leaves it uninitialised
    bool          m_leadSpeaker;
    ePartnerState m_partnerState;
    int8          m_firstToTargetFlag;         // 0xFF not decided yet, 1 arrived first, 0 arrived second
    int8          m_updateDirectionCount;      // How many more times GetPartnerSequence is run
    bool          m_taskCompleted;
    bool          m_makePedAlwaysFacePartner;
    char          m_animBlockName[16];
    bool          m_requiredAnimsStreamedIn;
    int16         m_goToPointTicks;            // ControlSubTask calls spent in the go to point sub task; this was the derived classes' first field
    int16         field_72;                    // Padding, never touched by the exe; keeps the derived classes' members at 0x74

public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER;

    static void InjectHooks();

    CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point);
    ~CTaskComplexPartner() override;

    eTaskType GetTaskType() const override { return Type; } // 0x681F10
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;
    virtual void StreamRequiredAnims();
    virtual void RemoveStreamedAnims();
    virtual CTaskComplexSequence* GetPartnerSequence() = 0;

    //! @returns The state of the partner's task (of the same type as this one), or 0 if it has none
    int32 GetPartnerState();

    //! Works out where this ped (`outPoint`) and its partner (`outPartnerPoint`) should stand; sets `m_taskCompleted` if no free spot was found
    void CalculateMeetingPoints(CPed* ped, CVector& outPoint, CVector& outPartnerPoint);

private:
    // 0x681E70
    CTaskComplexPartner* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point) {
        this->CTaskComplexPartner::CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, makePedAlwaysFacePartner, updateDirectionCount, point);
        return this;
    }

    // 0x683A40
    CTaskComplexPartner* Destructor() {
        this->CTaskComplexPartner::~CTaskComplexPartner();
        return this;
    }
};

VALIDATE_SIZE(CTaskComplexPartner, 0x74);
