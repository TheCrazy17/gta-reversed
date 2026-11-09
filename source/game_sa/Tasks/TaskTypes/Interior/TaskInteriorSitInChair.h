#pragma once

#include "TaskSimple.h"
#include "TaskTimer.h"

class CAnimBlendAssociation;
class CPed;
class CEvent;
struct InteriorInfo_t;

/// Ped sits down on a chair (the "lounge" animations) until the interior duration ran out
class NOTSA_EXPORT_VTABLE CTaskInteriorSitInChair : public CTaskSimple {
public:
    static void InjectHooks();

    static constexpr auto Type = eTaskType::TASK_INTERIOR_SIT_ON_CHAIR;

    CTaskInteriorSitInChair(int32 duration, InteriorInfo_t* intInfo, bool doInstantly);
    CTaskInteriorSitInChair(const CTaskInteriorSitInChair&);
    ~CTaskInteriorSitInChair();

    static void FinishAnimCB(CAnimBlendAssociation*, void*);

    CTask*    Clone() const override { return new CTaskInteriorSitInChair{*this}; } // 0x675CF0
    eTaskType GetTaskType() const override { return Type; }                         // 0x675C80
    bool      MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) override;
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x675C30
    CTaskInteriorSitInChair* Constructor(int32 duration, InteriorInfo_t* intInfo, bool doInstantly) {
        this->CTaskInteriorSitInChair::CTaskInteriorSitInChair(duration, intInfo, doInstantly);
        return this;
    }

    // 0x675C90
    CTaskInteriorSitInChair* Destructor() {
        this->CTaskInteriorSitInChair::~CTaskInteriorSitInChair();
        return this;
    }

private:
    int32                  m_SitDuration{};                   //< Time after which the ped gets up again (negative: never)
    InteriorInfo_t*        m_IntInfo{};                       //< The interior's info we're in
    bool                   m_bDoInstantly{};                  //< Whenever the ped should be set on the chair without the sitting down animation
    CAnimBlendAssociation* m_Anim{};                          //< The currently playing animation
    AnimationId            m_PrevAnimId{ ANIM_ID_UNDEFINED }; //< The animation that has finished last
    CTaskTimer             m_SitTimer{};                      //< Duration is `m_SitDuration`. After it expired the get up animation is played
    bool                   m_UpdatePedPos{};                  //< Whenever the ped's position should be updated (to reflect the animation)
    bool                   m_LastAnimFinished{};              //< Set when the get up animation has finished
    bool                   m_TaskAborting{};                  //< `MakeAbortable` was called, but not with IMMEDIATE mode, so the ped gets up first
};
VALIDATE_SIZE(CTaskInteriorSitInChair, 0x2C);
