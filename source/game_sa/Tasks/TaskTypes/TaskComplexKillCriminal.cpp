#include "StdInc.h"

#include <extensions/utility.hpp>

#include "TaskComplexKillCriminal.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskSimpleGangDriveBy.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskComplexEnterCarAsDriver.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleCarDrive.h"
#include "TaskComplexCarDriveMission.h"
#include "Entity/Ped/CopPed.h"
#include "Events/EventAcquaintancePedHate.h"
#include "LoadMonitor.h"
#include "InterestingEvents.h"


void CTaskComplexKillCriminal::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillCriminal, 0x870a00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x68BE70);
    RH_ScopedInstall(Destructor, 0x68BF30);

    RH_ScopedInstall(CreateSubTask, 0x68C050);
    RH_ScopedInstall(FindNextCriminalToKill, 0x68C3C0);
    RH_ScopedInstall(ChangeTarget, 0x68C6E0);

    RH_ScopedVMTInstall(Clone, 0x68CE50);
    RH_ScopedVMTInstall(GetTaskType, 0x68BF20);
    RH_ScopedVMTInstall(MakeAbortable, 0x68DAD0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x68E4F0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x68DC60);
    RH_ScopedVMTInstall(ControlSubTask, 0x68E950, {.Reversed = false});
}

bool NoPedOrNoHp(CPed* ped) {
    return !ped || ped->m_fHealth <= 0.f;
}

// 0x68BE70
CTaskComplexKillCriminal::CTaskComplexKillCriminal(CPed* criminal, bool randomize) :
    m_Randomize{randomize},
    m_Criminal{criminal}
{
    if (   !m_Criminal
        || m_Criminal->IsPlayer()
        || m_Criminal->IsCreatedByMission()
        || notsa::contains({ PED_TYPE_COP, PED_TYPE_MEDIC, PED_TYPE_FIREMAN, PED_TYPE_MISSION1 }, m_Criminal->m_nPedType)
    ) {
        m_Criminal = nullptr;
    }
}

// NOTSA (For 0x68CE50)
CTaskComplexKillCriminal::CTaskComplexKillCriminal(const CTaskComplexKillCriminal& o) :
    CTaskComplexKillCriminal{o.m_Criminal, false}
{
}

// 0x68BF30
CTaskComplexKillCriminal::~CTaskComplexKillCriminal() {
    if (m_Cop) {
        m_Cop->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS();
        m_Cop->bCullExtraFarAway = false;
        m_Cop->m_fRemovalDistMultiplier = 1.f;
        if (const auto veh = m_Cop->m_pVehicle) {
            veh->m_nExtendedRemovalRange = false;
            veh->vehicleFlags.bNeverUseSmallerRemovalRange = false;
            if (veh->IsDriver(m_Cop)) {
                const auto ap = &veh->m_autoPilot;
                ap->SetCarMission(MISSION_CRUISE);
                ap->SetDrivingStyle(DRIVING_STYLE_AVOID_CARS);
                ap->SetCruiseSpeed(10);
                if (veh->GetStatus() != STATUS_SIMPLE) {
                    CCarCtrl::JoinCarWithRoadSystem(veh);
                }
                veh->vehicleFlags.bSirenOrAlarm = false;
            }
            veh->vehicleFlags.bSirenOrAlarm = false;
        }
    }
}

// 0x68C050
CTask* CTaskComplexKillCriminal::CreateSubTask(eTaskType tt, CPed* ped, bool force) {
    if (!force && m_pSubTask && m_pSubTask->GetTaskType() == tt) {
        return m_pSubTask;        
    }

    switch (tt) {
    case TASK_COMPLEX_KILL_PED_ON_FOOT: {
        ped->SetCurrentWeapon(WEAPON_PISTOL);
        return new CTaskComplexKillPedOnFoot{ m_Criminal };
    }
    case TASK_FINISHED: {
        if (m_Criminal) {
            m_Criminal->SetPedDefaultDecisionMaker();
        }
        return nullptr;
    }
    case TASK_SIMPLE_GANG_DRIVEBY:
        return new CTaskSimpleGangDriveBy{
            m_Criminal,
            nullptr,
            70.f,
            70,
            eDrivebyStyle::AI_ALL_DIRN,
            false
        };
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger{ ped->m_pVehicle };
    case TASK_COMPLEX_ENTER_CAR_AS_DRIVER:
        return new CTaskComplexEnterCarAsDriver{ ped->m_pVehicle };
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    case TASK_SIMPLE_CAR_DRIVE:
        return new CTaskSimpleCarDrive{ ped->m_pVehicle };
    case TASK_COMPLEX_CAR_DRIVE_MISSION: {
        const auto oveh = ped->m_pVehicle; // (o)ur (veh)icle
        if (!oveh) {
            return nullptr;
        }

        const auto CreateDriveMission = [&, this](eCarMission mission, float cruiseSpeed, CEntity* traget) {
            return new CTaskComplexCarDriveMission{
                ped->m_pVehicle,
                traget,
                mission,
                DRIVING_STYLE_AVOID_CARS,
                cruiseSpeed
            };
        };

        if (const auto cveh = m_Criminal->GetVehicleIfInOne()) {
            return CreateDriveMission(
                oveh->IsBike()
                    ? MISSION_FOLLOWCAR_CLOSE
                    : MISSION_BLOCKCAR_CLOSE,
                (float)(m_Criminal->m_pVehicle->m_autoPilot.m_nCruiseSpeed) + 10.f,
                cveh
            );
        } else {
            return CreateDriveMission(MISSION_KILLPED_CLOSE, 20.f, m_Criminal);
        }
    }
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x68C3C0
CPed* CTaskComplexKillCriminal::FindNextCriminalToKill(CPed* ped, bool any) {
    const auto [closest, distSq] = notsa::SpatialQuery(
        m_Cop->m_apCriminalsToKill | rng::views::filter(notsa::Not(NoPedOrNoHp)),
        m_Cop->GetPosition(),
        m_Criminal.Get(),
        !any && NoPedOrNoHp(m_Criminal)
            ? m_Criminal.Get()
            : nullptr
    );
    return closest;
}

// 0x68C6E0
bool CTaskComplexKillCriminal::ChangeTarget(CPed* newTarget) { // TODO: Figure out if `newTarget` is actually the new target, or it's just he ped that is the owner of this task
    if (newTarget == m_Criminal) {
        return true;
    }

    if (NoPedOrNoHp(newTarget)) {
        return false;
    }

    if (m_Criminal && m_Criminal->bInVehicle) {
        return false;
    }

    if (notsa::isa_and_nonnull<CTaskComplexKillPedOnFoot>(m_pSubTask) && !m_pSubTask->MakeAbortable(newTarget, ABORT_PRIORITY_URGENT, nullptr)) {
        return false;
    }

    if (!notsa::contains(m_Cop->m_apCriminalsToKill, newTarget)) { // 0x68c760
        return false;
    }

    m_Criminal = newTarget;

    // Propagate change to partner
    if (const auto partner = m_Cop->m_pCopPartner) {
        if (partner->bInVehicle) {
            if (const auto partnersTask = partner->GetTaskManager().Find<CTaskComplexKillCriminal>(false)) {
                notsa::cast<CTaskComplexKillCriminal>(partnersTask)->ChangeTarget(newTarget);
            }
        }
    }

    m_HasFinished = false;

    return true;
}

// 0x68DAD0
bool CTaskComplexKillCriminal::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if ([&, this]{
        if (!event) {
            return true;
        }

        // Code @ 0x68DB32 has been inlined into the stuff below

        switch (const auto evType = event->GetEventType()) {
        case EVENT_ACQUAINTANCE_PED_HATE:
        case EVENT_VEHICLE_DAMAGE_COLLISION: // Ignore these
            return false;

        case EVENT_DAMAGE:
        case EVENT_VEHICLE_DAMAGE_WEAPON:
        case EVENT_GUN_AIMED_AT:
        case EVENT_SHOT_FIRED: {
            const auto evSrc  = event->GetSourceEntity();
            if (m_Criminal && evSrc == m_Criminal) {
                return false; // As per 0x68DB32
            }
            if (!evSrc || !evSrc->GetIsTypePed() || evSrc->AsPed()->IsPlayer()) {
                return false;
            }
            const auto evSrcPed = evSrc->AsPed();
            if (!m_Cop || m_Cop->AddCriminalToKill(evSrcPed) == (notsa::IsFixBugs() ? -1 : 0)) {
                return false;
            }
            if (notsa::contains({ EVENT_DAMAGE, EVENT_VEHICLE_DAMAGE_WEAPON }, evType)) { // Change target immediately
                if (!m_Criminal || (m_Criminal->GetPosition() - ped->GetPosition()).SquaredMagnitude() <= sq(25.f)) {
                    ChangeTarget(evSrcPed);
                }
            }
            return false;
        }
        }
        return true;
    }()) {
        return m_pSubTask->MakeAbortable(ped, priority, event);
    } else {
        const_cast<CEvent*>(event)->m_nTimeActive++; // ???????
        return false;
    }
}

// 0x68E4F0
CTask* CTaskComplexKillCriminal::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_GANG_DRIVEBY:
        return CreateSubTask(
            m_Cop->m_isTheDriver
                ? TASK_COMPLEX_CAR_DRIVE_MISSION
                : TASK_SIMPLE_CAR_DRIVE,
            ped
        );
    case TASK_COMPLEX_KILL_PED_ON_FOOT: { // Try finding the next criminal, if none, set `m_finished` and get into *the* car (if possible)
        CPed* const nextCriminal = NoPedOrNoHp(m_Criminal)
            ? FindNextCriminalToKill(ped, true)
            : m_Criminal.Get();
        if (nextCriminal && ChangeTarget(nextCriminal)) {
            return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped, true);
        }

        // No criminal, or can't target it, so just bail, so try getting back into the car
        m_HasFinished = true;
        if (m_CantGetInCar || !ped->m_pVehicle) {
            return CreateSubTask(TASK_FINISHED, ped);
        }

        if (!m_Cop->m_isTheDriver) {
            if (NoPedOrNoHp(m_Cop->m_pCopPartner)) { // Partner is dead, we get in as the driver
                m_Cop->m_isTheDriver = true;
                m_Cop->SetPartner(nullptr);
            } else {
                return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);
            }
        }

        return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_DRIVER, ped);
    }
    case TASK_COMPLEX_ENTER_CAR_AS_DRIVER: { // 0x68E533
        if (!ped->bInVehicle) {
            return CreateSubTask(
                m_HasFinished || NoPedOrNoHp(m_Criminal)
                    ? TASK_FINISHED
                    : TASK_COMPLEX_KILL_PED_ON_FOOT,
                ped
            );
        }
        const auto copPartnerNoneOrInVeh = NoPedOrNoHp(m_Cop->m_pCopPartner) || m_Cop->m_pCopPartner->bInVehicle;
        if (!m_HasFinished && !NoPedOrNoHp(m_Criminal) && !m_Criminal->IsInVehicle()) {
            return CreateSubTask(
                !ped->m_pVehicle || m_Criminal->IsEntityInRange(ped->m_pVehicle, 25.f) // 0x68E5D8
                    ? TASK_COMPLEX_KILL_PED_ON_FOOT
                    : copPartnerNoneOrInVeh // otherwise if criminal is too far chase them with the car
                        ? TASK_COMPLEX_CAR_DRIVE_MISSION
                        : TASK_SIMPLE_CAR_DRIVE,
                ped
            );
        }
        if (const auto next = FindNextCriminalToKill(ped, true); next && ChangeTarget(next)) { // Try finding another criminal to kill
            return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped, true);
        }
        // No criminal to kill, so get into *the* vehicle and fuck off
        if (ped->IsInVehicle()) {
            ped->m_pVehicle->vehicleFlags.bSirenOrAlarm = false;
        }
        return CreateSubTask(
            copPartnerNoneOrInVeh
                ? TASK_FINISHED
                : TASK_SIMPLE_CAR_DRIVE,
            ped
        );
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: { // 0x68E6FA
        if (ped->bInVehicle) { // (Inverted)
            return CreateSubTask(TASK_SIMPLE_CAR_DRIVE, ped);
        }
        m_CantGetInCar = true;
        if (m_HasFinished || NoPedOrNoHp(m_Criminal)) {
            return CreateSubTask(TASK_FINISHED, ped);
        } 
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: // 0x68E77F
        return CreateSubTask(
            !ped->bInVehicle || m_CantGetInCar || (!m_HasFinished && !NoPedOrNoHp(m_Criminal) && !m_Criminal->IsInVehicle() && m_Criminal->IsEntityInRange(ped, 25.f))
                ? TASK_COMPLEX_KILL_PED_ON_FOOT     // Criminal can be killed on foot
                : TASK_COMPLEX_ENTER_CAR_AS_DRIVER, // We have to chase the criminal with a vehicle
            ped
        );
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x68DC60
CTask* CTaskComplexKillCriminal::CreateFirstSubTask(CPed* ped) {
    ZoneScoped;

    if (!m_Criminal || m_Criminal->IsPlayer()) {
        return nullptr;
    }
    if (FindPlayerWanted()->m_WantedLevel != 0) { // The cops have the player to deal with
        return nullptr;
    }
    if (!g_LoadMonitor.IsAmbientCrimeEnabled()) {
        return nullptr;
    }
    if (ped->m_nPedType != PED_TYPE_COP) {
        return nullptr;
    }

    // The player is riding with the criminal
    if (const auto criminalVeh = m_Criminal->m_pVehicle) {
        if (criminalVeh->m_pDriver && criminalVeh->m_pDriver->IsPlayer()) {
            return nullptr;
        }
        for (auto i = 0u; i < criminalVeh->m_nMaxPassengers; i++) {
            if (const auto passenger = criminalVeh->m_apPassengers[i]; passenger && passenger->IsPlayer()) {
                return nullptr;
            }
        }
    }

    m_Cop = static_cast<CCopPed*>(ped);

    if (m_Randomize && !m_Criminal->bWantedByPolice) {
        const auto criminalVeh = m_Criminal->m_pVehicle;
        if (!m_Criminal->bInVehicle || !criminalVeh || !criminalVeh->vehicleFlags.bMadDriver || CGeneral::GetRandomNumberInRange(0, 3) != 0) {
            return nullptr;
        }
    }

    m_Cop->AddCriminalToKill(m_Criminal);
    if (m_Criminal->bInVehicle) {
        m_Criminal->GetIntelligence()->SetPedDecisionMakerType(6);
    }

    // Tells the cop, that is not us, to go after the criminal as well
    const auto AddHateEvent = [](CPed* receiver, CPed* hated, eTaskType taskId) {
        CEventAcquaintancePedHate event{ hated };
        event.m_TaskId = taskId;
        receiver->GetIntelligence()->GetEventGroup().Add(&event, false);
    };

    // The task for the cop, the cop being in a vehicle with a partner changes what it should do
    CTask* task{};
    auto   hasTask = false;
    if (m_Cop->bInVehicle && m_Cop->m_pVehicle) {
        const auto copVeh = m_Cop->m_pVehicle;
        if (copVeh->m_pDriver == ped) {
            m_Cop->m_isTheDriver = true;
            if (!m_Cop->m_pCopPartner) {
                for (auto i = 0u; i < copVeh->m_nMaxPassengers; i++) {
                    if (const auto passenger = copVeh->m_apPassengers[i]; passenger && passenger->m_nPedType == PED_TYPE_COP) {
                        m_Cop->SetPartner(static_cast<CCopPed*>(passenger));
                        break;
                    }
                }
            }
            if (m_Cop->m_pCopPartner) {
                AddHateEvent(m_Cop->m_pCopPartner, m_Criminal, TASK_COMPLEX_KILL_CRIMINAL);
            }
        } else {
            if (const auto driver = copVeh->m_pDriver; driver && driver->m_nPedType == PED_TYPE_COP) {
                m_Cop->SetPartner(static_cast<CCopPed*>(driver));
                static_cast<CCopPed*>(m_Cop->m_pVehicle->m_pDriver)->m_isTheDriver = true;
            }
            if (const auto partner = m_Cop->m_pCopPartner) {
                if (partner->m_isTheDriver) {
                    // The driver is already after the same criminal, if not we tell it to be
                    const auto partnerTask = static_cast<CTaskComplexKillCriminal*>(partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_KILL_CRIMINAL));
                    if (partnerTask && partnerTask->m_Criminal == m_Criminal) {
                        m_Cop->m_isTheDriver         = false;
                        m_Cop->bDontDragMeOutCar     = true;
                    } else {
                        AddHateEvent(partner, m_Criminal, TASK_COMPLEX_KILL_CRIMINAL);
                        return nullptr;
                    }
                }
            } else {
                m_Cop->m_isTheDriver = true;
                task                 = CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
                hasTask              = task != nullptr;
            }
        }
    } else {
        if (!m_Cop->m_pCopPartner || m_Cop->m_pCopPartner->m_fHealth <= 0.0f) {
            m_Cop->m_isTheDriver = true;
        }
        task    = CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
        hasTask = true;
    }

    if (!hasTask) {
        if (m_Criminal->bInVehicle && m_Criminal->m_pVehicle) {
            task = CreateSubTask(m_Cop->m_isTheDriver ? TASK_COMPLEX_CAR_DRIVE_MISSION : TASK_SIMPLE_CAR_DRIVE, ped);
        } else {
            task = CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
        }
    }

    // The way the vehicle was driving is brought back when the task is over
    if (ped->m_pVehicle && m_Cop->m_isTheDriver) {
        const auto& autoPilot = ped->m_pVehicle->m_autoPilot;
        m_OrigDrivingMode     = autoPilot.m_nCarDrivingStyle;
        m_OrigMission         = autoPilot.m_nCarMission;
        m_OrigCruiseSpeed     = autoPilot.m_nCruiseSpeed;
        m_IsSetUp             = true;
    }

    // The criminal, and the ones with them, are armed
    const auto GiveCriminalGun = [](CPed* criminal) {
        criminal->GiveWeapon(WEAPON_PISTOL, 1000, true);
        criminal->SetCurrentWeapon(WEAPON_PISTOL);
    };
    if (const auto criminalVeh = m_Criminal->m_pVehicle) {
        if (criminalVeh->vehicleFlags.bMadDriver) {
            if (criminalVeh->m_nNumPassengers > 0) {
                GiveCriminalGun(m_Criminal);
                for (auto i = 0u; i < m_Criminal->m_pVehicle->m_nMaxPassengers; i++) {
                    if (const auto passenger = m_Criminal->m_pVehicle->m_apPassengers[i]) {
                        GiveCriminalGun(passenger);
                        m_Cop->AddCriminalToKill(passenger);
                    }
                }
            } else if (CGeneral::GetRandomNumber() & 1) {
                GiveCriminalGun(m_Criminal);
            }
        }
    } else if (CGeneral::GetRandomNumber() & 1) {
        GiveCriminalGun(m_Criminal);
        AddHateEvent(m_Criminal, ped, TASK_COMPLEX_KILL_PED_ON_FOOT);
    }

    ped->m_nTimeTillWeNeedThisPed   = CTimer::GetTimeInMS() + 5 * 60 * 1000;
    ped->bCullExtraFarAway          = true;
    ped->m_fRemovalDistMultiplier   = 0.3f;
    if (const auto veh = ped->m_pVehicle) {
        veh->m_nExtendedRemovalRange                = 255;
        veh->vehicleFlags.bNeverUseSmallerRemovalRange = true;
    }
    if (const auto veh = ped->m_pVehicle; veh && ped->bInVehicle) {
        veh->vehicleFlags.bSirenOrAlarm = true;
    }

    g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_25, ped);

    return task;
}

// 0x68E950
CTask* CTaskComplexKillCriminal::ControlSubTask(CPed* ped) {
    /*
    if (m_criminal && !m_criminal->CanBeCriminal()) {
        return nullptr;
    }

    if (const auto wanted = FindPlayerWanted(); wanted->m_nWantedLevel) {
        if (wanted->CanCopJoinPursuit(static_cast<CCopPed*>(ped)) && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return nullptr;
        }
    }

    if (!g_LoadMonitor.m_bEnableAmbientCrime) {
        return nullptr;
    }

    auto taskToCreate = TASK_NONE;

    const auto cpartner = m_cop->m_pCopPartner;
    if (!m_cop->m_isTheDriver && IsPedNullOrLowHP(cpartner)) {
        m_cop->m_isTheDriver = true;
        m_cop->SetPartner(nullptr);
        if (m_cop->IsInVehicle()) {
            taskToCreate = TASK_COMPLEX_LEAVE_CAR;
        }
    }
    */
    return plugin::CallMethodAndReturn<CTask*, 0x68E950, CTaskComplexKillCriminal*, CPed*>(this, ped); // Good luck!
}
