#include "StdInc.h"
#include "Collision/Box.h"
#include <extensions/Shapes/AngledRect.hpp>
#include "Tasks/TaskTypes/TaskSimpleFinishBrain.h"

#include "RunningScript.h"
#include "TheScripts.h"
#include "CarGenerator.h"
#include "Hud.h"
#include "spdlog/sinks/stdout_color_sinks.h"

static notsa::log_ptr logger;

//static auto logger = NOTSA_MAKE_LOGGER("script");

//! Define it to dump out all commands that don't have a custom handler (that is, they aren't reversed)
//! Makes compilation slow, so don't enable unless necessary!
//#define DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE

#ifdef DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE
#include <fstream>
#endif

#include "CommandParser/Parser.hpp"
#include "CommandParser/LUTGenerator.hpp"
#include "reversiblehooks/ReversibleHook/ScriptCommand.h"

#include "Commands/Commands.hpp"
#ifdef NOTSA_WITH_CLEO_SCRIPT_COMMANDS // TODO: Add premake/cmake option for this define
#include "Commands/CLEO/Commands.hpp"
#include "Commands/CLEO/Extensions/Commands.hpp"
#endif

// https://library.sannybuilder.com/#/sa

//! Holds all custom command handlers (or null for commands with no custom handler)
static inline std::array<notsa::script::CommandHandlerFunction, (size_t)(COMMAND_HIGHEST_ID_TO_HOOK) + 1> s_CustomCommandHandlerTable{};

std::array<std::array<char, COMMANDS_CHAR_BUFFER_SIZE>, COMMANDS_CHAR_BUFFERS_COUNT> CRunningScript::ScriptArgCharBuffers        = {};
uint8                                                                                CRunningScript::ScriptArgCharNextFreeBuffer = 0;

void CRunningScript::InjectHooks() {
    logger = NOTSA_MAKE_LOGGER("script");

    InjectCustomCommandHooks();

    RH_ScopedClass(CRunningScript);
    RH_ScopedCategory("Scripts");

    RH_ScopedInstall(Init, 0x4648E0);
    RH_ScopedInstall(GetCorrectPedModelIndexForEmergencyServiceType, 0x464F50);

    RH_ScopedInstall(PlayAnimScriptCommand, 0x470150, { .reversed = false });
    RH_ScopedInstall(LocateCarCommand, 0x487A20);
    RH_ScopedInstall(LocateCharCommand, 0x486D80);
    RH_ScopedInstall(LocateObjectCommand, 0x487D10);
    RH_ScopedInstall(LocateCharCarCommand, 0x487420);
    RH_ScopedInstall(LocateCharCharCommand, 0x4870F0);
    RH_ScopedInstall(LocateCharObjectCommand, 0x487720);
    RH_ScopedInstall(CarInAreaCheckCommand, 0x488EC0);
    RH_ScopedInstall(CharInAreaCheckCommand, 0x488B50);
    RH_ScopedInstall(ObjectInAreaCheckCommand, 0x489150);
    RH_ScopedInstall(CharInAngledAreaCheckCommand, 0x487F60);
    RH_ScopedInstall(FlameInAngledAreaCheckCommand, 0x488780);
    RH_ScopedInstall(ObjectInAngledAreaCheckCommand, 0x4883F0);
    RH_ScopedInstall(CollectParameters, 0x464080, { .stackArguments = 1 });
    RH_ScopedInstall(CollectNextParameterWithoutIncreasingPC, 0x464250, { .stackArguments = 0 });
    RH_ScopedInstall(StoreParameters, 0x464370, { .stackArguments = 1 });
    RH_ScopedInstall(ReadArrayInformation, 0x463CF0, { .stackArguments = 3 });
    RH_ScopedInstall(ReadParametersForNewlyStartedScript, 0x464500, { .stackArguments = 1 });
    RH_ScopedInstall(ReadTextLabelFromScript, 0x463D50, { .stackArguments = 2 });
    RH_ScopedInstall(GetIndexOfGlobalVariable, 0x464700, { .stackArguments = 0 });
    RH_ScopedInstall(GetPadState, 0x485B10);
    RH_ScopedInstall(GetPointerToLocalVariable, 0x463CA0, { .stackArguments = 1 });
    RH_ScopedInstall(GetPointerToLocalArrayElement, 0x463CC0, { .stackArguments = 3 });
    RH_ScopedInstall(GetPointerToScriptVariable, 0x464790, { .stackArguments = 1 });
    RH_ScopedInstall(DoDeathArrestCheck, 0x485A50);
    RH_ScopedInstall(SetCharCoordinates, 0x464DC0);
    RH_ScopedInstall(AddScriptToList, 0x464C00, { .stackArguments = 1 });
    RH_ScopedInstall(RemoveScriptFromList, 0x464BD0, { .stackArguments = 1 });
    RH_ScopedInstall(ShutdownThisScript, 0x465AA0);
    RH_ScopedInstall(IsPedDead, 0x464D70);
    RH_ScopedInstall(ThisIsAValidRandomPed, 0x489490);
    RH_ScopedInstall(ScriptTaskPickUpObject, 0x46AF50, { .reversed = false });
    RH_ScopedInstall(UpdateCompareFlag, 0x4859D0, { .stackArguments = 1 });
    RH_ScopedInstall(UpdatePC, 0x464DA0, { .stackArguments = 1 });
    RH_ScopedInstall(ProcessOneCommand, 0x469EB0);
    RH_ScopedInstall(Process, 0x469F00);
    RH_ScopedOverloadedInstall(GivePedScriptedTask, "OG", 0x465C20, void(CRunningScript::*)(int32, CTask*, int32));
}

//! Register our custom script command handlers
void CRunningScript::InjectCustomCommandHooks() {
    // Uncommenting any call will prevent it from being hooked, so
    // feel free to do so when debugging (Just don't forget to undo the changes!)

    namespace c = ::notsa::script::commands;
    c::basic::RegisterHandlers();
    c::camera::RegisterHandlers();
    c::character::RegisterHandlers();
    c::clock::RegisterHandlers();
    c::comparasion::RegisterHandlers();
    c::game::RegisterHandlers();
    c::generic::RegisterHandlers();
    c::math::RegisterHandlers();
    c::mission::RegisterHandlers();
    c::object::RegisterHandlers();
    c::pad::RegisterHandlers();
    c::ped::RegisterHandlers();
    c::player::RegisterHandlers();
    c::script::RegisterHandlers();
    c::sequence::RegisterHandlers();
    c::text::RegisterHandlers();
    c::unused::RegisterHandlers();
    c::utility::RegisterHandlers();
    c::vehicle::RegisterHandlers();
    c::zone::RegisterHandlers();
    c::stat::RegisterHandlers();
    c::conversation::RegisterHandlers();

#ifdef NOTSA_WITH_CLEO_SCRIPT_COMMANDS
    cleo::audiostream::RegisterHandlers();
    cleo::character::RegisterHandlers();
    cleo::dynamiclibrary::RegisterHandlers();
    cleo::fs::RegisterHandlers();
    cleo::game::RegisterHandlers();
    cleo::generic::RegisterHandlers();
    cleo::memory::RegisterHandlers();
    cleo::pad::RegisterHandlers();
    cleo::script::RegisterHandlers();
    cleo::vehicle::RegisterHandlers();
    cleo::world::RegisterHandlers();

    cleo::extensions::cleoplus::RegisterHandlers();
    cleo::extensions::clipboard::RegisterHandlers();
    cleo::extensions::fs::RegisterHandlers();
    cleo::extensions::imgui::RegisterHandlers();
    cleo::extensions::intoperations::RegisterHandlers();
#endif

#ifdef DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE
    auto reversed{0}, total{0};
    std::ofstream ofsrev{ "reversed_script_command_handlers.txt" }, ofsnotrev{ "NOT_reversed_script_command_handlers.txt" };
    for (auto&& [idx, handler] : rngv::enumerate(s_CustomCommandHandlerTable)) {
        const auto id = (eScriptCommands)(idx);
        ++total;
        if (handler) ++reversed;
        (handler ? ofsrev : ofsnotrev) << ::notsa::script::GetScriptCommandName(id) << '\n';
    }
    NOTSA_LOG_DEBUG("Script cmds dumped! Find them in `<GTA Directory>/Scripts`!");
    NOTSA_LOG_DEBUG("Script cmds reverse progress: {}/{} ({:.2f}% done)", reversed, total, 100.0f * ((float)reversed / (float)total));
#endif
}

// 0x4648E0
void CRunningScript::Init() {
    SetName("noname");
    rng::fill(m_IPStack, nullptr);
    rng::fill(m_LocalVars, tScriptParam{ 0 });
    m_BaseIP                          = nullptr;
    m_pPrev                           = nullptr;
    m_pNext                           = nullptr;
    m_IP                              = nullptr;
    m_StackDepth                      = 0;
    m_WakeTime                        = 0;
    m_IsActive                        = false;
    m_CondResult                      = false;
    m_UsesMissionCleanup              = false;
    m_IsExternal                      = false;
    m_IsTextBlockOverride             = false;
    m_ExternalType                    = -1;
    m_AndOrState                      = 0;
    m_NotFlag                         = false;
    m_DoneDeathArrest                 = false;
    m_SceneSkipIP                     = 0;
    m_ThisMustBeTheOnlyMissionRunning = false;
    m_IsDeathArrestCheckEnabled       = true;
}

/*!
 * Adds script to list
 * @addr 0x464C00
 */
void CRunningScript::AddScriptToList(CRunningScript** queueList) {
    m_pNext = *queueList;
    m_pPrev = nullptr;
    if (*queueList)
        (*queueList)->m_pPrev = this;
    *queueList = this;
}

/*!
 * Removes script from list
 * @addr 0x464BD0
 */
void CRunningScript::RemoveScriptFromList(CRunningScript** queueList) {
    if (m_pPrev)
        m_pPrev->m_pNext = m_pNext;
    else
        *queueList = m_pNext;

    if (m_pNext)
        m_pNext->m_pPrev = m_pPrev;
}

/*!
 * Terminates a script
 * @addr 0x465AA0
 */
void CRunningScript::ShutdownThisScript() {
    m_IsActive = false;

    if (m_IsExternal) {
        const auto idx = CTheScripts::StreamedScripts.GetStreamedScriptWithThisStartAddress(m_BaseIP);
        CTheScripts::StreamedScripts.m_aScripts[idx].m_NumberOfUsers--;
    }

    switch (m_ExternalType) {
    case 0:
    case 2:
    case 3:
    case 5: {
        const int32 pedRef = m_ThisMustBeTheOnlyMissionRunning
            ? CTheScripts::LocalVariablesForCurrentMission[0].iParam
            : m_LocalVars[0].iParam;
        if (CPed* ped = GetPedPool()->GetAtRef(pedRef)) {
            ped->bHasAScriptBrain = false;
            if (m_ExternalType == 5) {
                CScriptedBrainTaskStore::SetTask(ped, new CTaskSimpleFinishBrain{});
            }
        }
        break;
    }
    case 1:
    case 4: {
        const int32 objRef = m_ThisMustBeTheOnlyMissionRunning
            ? CTheScripts::LocalVariablesForCurrentMission[0].iParam
            : m_LocalVars[0].iParam;
        if (CObject* obj = GetObjectPool()->GetAtRef(objRef)) {
            obj->objectFlags.b0x100000_0x200000 = 1; // matches TheScripts.cpp's ProcessScriptsForBrains, which reacts to this state
        }
        break;
    }
    default:
        break;
    }
}

// 0x465C20
void CRunningScript::GivePedScriptedTask(int32 pedHandle, CTask* task, int32 opcode) {
    if (pedHandle == -1) {
        CTaskSequences::AddTaskToActiveSequence(task);
        return;
    }

    CPed* ped = GetPedPool()->GetAtRef(pedHandle);
    assert(ped);
    CPedGroup* pedGroup = CPedGroups::GetPedsGroup(ped);

    CPed* otherPed = nullptr;
    if (m_ExternalType == 5 || m_ExternalType == 2 || !m_ExternalType || m_ExternalType == 3) {
        auto* localVariable = reinterpret_cast<int32*>(GetPointerToLocalVariable(0));
        otherPed = GetPedPool()->GetAtRef(*localVariable);
    }

    if (ped->bHasAScriptBrain && otherPed != ped) {
        delete task;
    } else if (otherPed && m_ExternalType == 5) {
        if (CScriptedBrainTaskStore::SetTask(ped, task)) {
            const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
            CPedScriptedTaskRecord::ms_scriptedTasks[slot].SetAsAttractorScriptTask(ped, opcode, task);
        }
    } else if (!pedGroup || ped->IsPlayer()) {
        CEventScriptCommand eventScriptCommand(TASK_PRIMARY_PRIMARY, task, false);
        auto* event = static_cast<CEventScriptCommand*>(ped->GetEventGroup().Add(&eventScriptCommand, false));
        if (event) {
            const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
            CPedScriptedTaskRecord::ms_scriptedTasks[slot].Set(ped, opcode, event);
        }
    } else {
        pedGroup->GetIntelligence().SetScriptCommandTask(ped, *task);
        CTask* scriptedTask = pedGroup->GetIntelligence().GetTaskScriptCommand(ped);
        const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
        CPedScriptedTaskRecord::ms_scriptedTasks[slot].SetAsGroupTask(ped, opcode, scriptedTask);
        delete task;
    }
}

void CRunningScript::GivePedScriptedTask(CPed* ped, CTask* task, int32 opcode) {
    GivePedScriptedTask(GetPedPool()->GetRef(ped), task, opcode); // Must do it like this, otherwise unhooking of the original `GivePedScriptedTask` will do nothing
}

// 0x470150
void CRunningScript::PlayAnimScriptCommand(int32 commandId) {
    plugin::CallMethod<0x470150, CRunningScript*, int32>(this, commandId);
}

// 0x487A20
void CRunningScript::LocateCarCommand(int32 commandId) {
    // Handles LOCATE_(STOPPED_)CAR_2D/3D [0x1AD..0x1B0]. Box is centered on (x,y[,z]) with a radius per axis.
    const bool is3D = commandId == COMMAND_LOCATE_CAR_3D || commandId == COMMAND_LOCATE_STOPPED_CAR_3D;
    CollectParameters(is3D ? 8 : 6);

    auto* vehicle = GetVehiclePool()->GetAtRef(ScriptParams[0].iParam);

    const auto x = ScriptParams[1].fParam;
    const auto y = ScriptParams[2].fParam;
    const auto z = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto radiusX = ScriptParams[is3D ? 4 : 3].fParam;
    const auto radiusY = ScriptParams[is3D ? 5 : 4].fParam;
    const auto radiusZ = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    const bool isStoppedVariant = commandId == COMMAND_LOCATE_STOPPED_CAR_2D || commandId == COMMAND_LOCATE_STOPPED_CAR_3D;

    bool result = false;
    if (!isStoppedVariant || CTheScripts::IsVehicleStopped(vehicle)) { // NOTSA: matches original - vehicle deref'd even if the handle was invalid
        result = is3D
            ? vehicle->IsWithinArea(x - radiusX, y - radiusY, z - radiusZ, x + radiusX, y + radiusY, z + radiusZ)
            : vehicle->IsWithinArea(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    }
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x - radiusX, y - radiusY, z}, CVector{x + radiusX, y + radiusY, z});
        } else {
            HighlightImportantArea(CVector2D{x - radiusX, y - radiusY}, CVector2D{x + radiusX, y + radiusY});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    }
}

// 0x486D80
void CRunningScript::LocateCharCommand(int32 commandId) {
    // Handles LOCATE_(STOPPED_)CHAR_ANY_MEANS/ON_FOOT/IN_CAR_2D/3D [0xEC..0xF1, 0xFE..0x103]. Box is centered on (x,y[,z]) with a radius per axis.
    const bool is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_3D && commandId <= COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D;
    CollectParameters(is3D ? 8 : 6);

    auto* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);

    const auto x = ScriptParams[1].fParam;
    const auto y = ScriptParams[2].fParam;
    const auto z = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto radiusX = ScriptParams[is3D ? 4 : 3].fParam;
    const auto radiusY = ScriptParams[is3D ? 5 : 4].fParam;
    const auto radiusZ = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    bool isStoppedVariant = false;
    switch (commandId) {
    case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_3D:
    case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_3D:
    case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D:
        isStoppedVariant = true;
        break;
    default:
        break;
    }

    bool result = false;
    if (!isStoppedVariant || CTheScripts::IsPedStopped(ped)) { // NOTSA: matches original - ped deref'd even if the handle was invalid
        const auto testPos = ped->GetRealPosition(); // uses the vehicle's position instead, if ped is inside one
        result = testPos.x >= x - radiusX && testPos.x <= x + radiusX && testPos.y >= y - radiusY && testPos.y <= y + radiusY;
        if (result && is3D) {
            result = testPos.z >= z - radiusZ && testPos.z <= z + radiusZ;
        }

        if (result) {
            switch (commandId) {
            case COMMAND_LOCATE_CHAR_ON_FOOT_2D:
            case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_2D:
            case COMMAND_LOCATE_CHAR_ON_FOOT_3D:
            case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_3D:
                result = !ped->IsInVehicle();
                break;
            case COMMAND_LOCATE_CHAR_IN_CAR_2D:
            case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_2D:
            case COMMAND_LOCATE_CHAR_IN_CAR_3D:
            case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D:
                result = ped->IsInVehicle();
                break;
            default:
                break; // LOCATE_(STOPPED_)CHAR_ANY_MEANS_2D/3D - no extra condition
            }
        }
    }
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x - radiusX, y - radiusY, z}, CVector{x + radiusX, y + radiusY, z});
        } else {
            HighlightImportantArea(CVector2D{x - radiusX, y - radiusY}, CVector2D{x + radiusX, y + radiusY});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    }
}

// 0x487D10
void CRunningScript::LocateObjectCommand(int32 commandId) {
    // Handles LOCATE_OBJECT_2D/3D [0x4E5/0x4E6] - no stopped/on-foot/in-car variants exist for objects.
    const bool is3D = commandId == COMMAND_LOCATE_OBJECT_3D;
    CollectParameters(is3D ? 8 : 6);

    auto* object = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    const auto x = ScriptParams[1].fParam;
    const auto y = ScriptParams[2].fParam;
    const auto z = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto radiusX = ScriptParams[is3D ? 4 : 3].fParam;
    const auto radiusY = ScriptParams[is3D ? 5 : 4].fParam;
    const auto radiusZ = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    const bool result = is3D
        ? object->IsWithinArea(x - radiusX, y - radiusY, z - radiusZ, x + radiusX, y + radiusY, z + radiusZ)
        : object->IsWithinArea(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x - radiusX, y - radiusY, z}, CVector{x + radiusX, y + radiusY, z});
        } else {
            HighlightImportantArea(CVector2D{x - radiusX, y - radiusY}, CVector2D{x + radiusX, y + radiusY});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    }
}

// 0x487420
void CRunningScript::LocateCharCarCommand(int32 commandId) {
    const auto is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_3D; // Only ever called with [0x202,0x207]
    CollectParameters(is3D ? 6 : 5);

    auto* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    auto* car = GetVehiclePool()->GetAtRef(ScriptParams[1].iParam);
    assert(ped);
    assert(car);

    const auto radiusX = ScriptParams[2].fParam;
    const auto radiusY = ScriptParams[3].fParam;

    const auto subjectPos = ped->GetRealPosition(); // uses the vehicle's position instead, if ped is inside one
    const auto targetPos  = car->GetPosition();

    bool  inArea;
    CBox  box{};  // only valid/used if is3D
    CRect rect{}; // only valid/used if !is3D
    if (is3D) {
        const auto radiusZ = ScriptParams[4].fParam;
        const CVector radius{ radiusX, radiusY, radiusZ };
        box    = CBox{ targetPos - radius, targetPos + radius };
        inArea = box.IsPointInside(subjectPos);
    } else {
        const CVector2D targetPos2D{ targetPos };
        const CVector2D radius{ radiusX, radiusY };
        rect   = CRect{ targetPos2D - radius, targetPos2D + radius };
        inArea = rect.IsPointInside(CVector2D{ subjectPos });
    }

    auto result = false;
    if (inArea) {
        switch ((eScriptCommands)commandId) {
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_3D:
            result = true;
            break;
        case COMMAND_LOCATE_CHAR_ON_FOOT_CAR_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_CAR_3D:
            result = !ped->bInVehicle;
            break;
        case COMMAND_LOCATE_CHAR_IN_CAR_CAR_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_CAR_3D:
            result = ped->bInVehicle;
            break;
        default:
            break;
        }
    }
    UpdateCompareFlag(result);

    // Highlight/debug-draw run unconditionally of `result` - only gated on the draw-area param.
    if (ScriptParams[is3D ? 5 : 4].bParam) {
        if (is3D) {
            HighlightImportantArea(box.m_vecMin, box.m_vecMax);
        } else {
            HighlightImportantArea(rect.GetTopLeft(), rect.GetBottomRight());
        }
    }
    if (!is3D && CTheScripts::DbgFlag) {
        CTheScripts::DrawDebugSquare(rect);
    }
}

// 0x4870F0
void CRunningScript::LocateCharCharCommand(int32 commandId) {
    const auto is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_3D; // Valid IDs: {0xF2..0xF4} u {0x104..0x106} (non-contiguous)
    CollectParameters(is3D ? 6 : 5);

    auto* ped      = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    auto* otherPed = GetPedPool()->GetAtRef(ScriptParams[1].iParam);
    assert(ped);
    assert(otherPed);

    const auto radiusX = ScriptParams[2].fParam;
    const auto radiusY = ScriptParams[3].fParam;

    const auto subjectPos = ped->GetRealPosition();
    const auto targetPos  = otherPed->GetRealPosition(); // also vehicle-substituted, unlike the Car/Object variants

    bool  inArea;
    CBox  box{};
    CRect rect{};
    if (is3D) {
        const auto radiusZ = ScriptParams[4].fParam;
        const CVector radius{ radiusX, radiusY, radiusZ };
        box    = CBox{ targetPos - radius, targetPos + radius };
        inArea = box.IsPointInside(subjectPos);
    } else {
        const CVector2D targetPos2D{ targetPos };
        const CVector2D radius{ radiusX, radiusY };
        rect   = CRect{ targetPos2D - radius, targetPos2D + radius };
        inArea = rect.IsPointInside(CVector2D{ subjectPos });
    }

    auto result = false;
    if (inArea) {
        switch ((eScriptCommands)commandId) {
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_3D:
            result = true;
            break;
        case COMMAND_LOCATE_CHAR_ON_FOOT_CHAR_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_CHAR_3D:
            result = !ped->bInVehicle;
            break;
        case COMMAND_LOCATE_CHAR_IN_CAR_CHAR_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_CHAR_3D:
            result = ped->bInVehicle;
            break;
        default:
            break;
        }
    }
    UpdateCompareFlag(result);

    if (ScriptParams[is3D ? 5 : 4].bParam) {
        if (is3D) {
            HighlightImportantArea(box.m_vecMin, box.m_vecMax);
        } else {
            HighlightImportantArea(rect.GetTopLeft(), rect.GetBottomRight());
        }
    }
    if (!is3D && CTheScripts::DbgFlag) {
        CTheScripts::DrawDebugSquare(rect);
    }
}

// 0x487720
void CRunningScript::LocateCharObjectCommand(int32 commandId) {
    const auto is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_3D; // Only ever called with [0x471,0x476]
    CollectParameters(is3D ? 6 : 5);

    auto* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    auto* obj = GetObjectPool()->GetAtRef(ScriptParams[1].iParam);
    assert(ped);
    assert(obj);

    const auto radiusX = ScriptParams[2].fParam;
    const auto radiusY = ScriptParams[3].fParam;

    const auto subjectPos = ped->GetRealPosition();
    const auto targetPos  = obj->GetPosition(); // objects can't be "in a car" - no substitution needed

    bool  inArea;
    CBox  box{};
    CRect rect{};
    if (is3D) {
        const auto radiusZ = ScriptParams[4].fParam;
        const CVector radius{ radiusX, radiusY, radiusZ };
        box    = CBox{ targetPos - radius, targetPos + radius };
        inArea = box.IsPointInside(subjectPos);
    } else {
        const CVector2D targetPos2D{ targetPos };
        const CVector2D radius{ radiusX, radiusY };
        rect   = CRect{ targetPos2D - radius, targetPos2D + radius };
        inArea = rect.IsPointInside(CVector2D{ subjectPos });
    }

    auto result = false;
    if (inArea) {
        switch ((eScriptCommands)commandId) {
        case COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_3D:
            result = true;
            break;
        case COMMAND_LOCATE_CHAR_ON_FOOT_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_OBJECT_3D:
            result = !ped->bInVehicle;
            break;
        case COMMAND_LOCATE_CHAR_IN_CAR_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_OBJECT_3D:
            result = ped->bInVehicle;
            break;
        default:
            break;
        }
    }
    UpdateCompareFlag(result);

    if (ScriptParams[is3D ? 5 : 4].bParam) {
        if (is3D) {
            HighlightImportantArea(box.m_vecMin, box.m_vecMax);
        } else {
            HighlightImportantArea(rect.GetTopLeft(), rect.GetBottomRight());
        }
    }
    if (!is3D && CTheScripts::DbgFlag) {
        CTheScripts::DrawDebugSquare(rect);
    }
}

// 0x488EC0
void CRunningScript::CarInAreaCheckCommand(int32 commandId) {
    // Handles: IS_CAR_IN_AREA_2D/3D, IS_CAR_STOPPED_IN_AREA_2D/3D
    const auto is3D = commandId == COMMAND_IS_CAR_IN_AREA_3D || commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    auto* vehicle = GetVehiclePool()->GetAtRef(ScriptParams[0].iParam);

    const auto x1 = ScriptParams[1].fParam;
    const auto y1 = ScriptParams[2].fParam;
    const auto z1 = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto x2 = ScriptParams[is3D ? 4 : 3].fParam;
    const auto y2 = ScriptParams[is3D ? 5 : 4].fParam;
    const auto z2 = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    const bool isStoppedVariant = commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_2D || commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_3D;

    bool result = false;
    if (!isStoppedVariant || CTheScripts::IsVehicleStopped(vehicle)) { // NOTSA: matches original - vehicle deref'd even if the handle was invalid
        result = is3D
            ? vehicle->IsWithinArea(x1, y1, z1, x2, y2, z2)
            : vehicle->IsWithinArea(x1, y1, x2, y2);
    }
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x1, y1, z1}, CVector{x2, y2, z2});
        } else {
            HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

// 0x488B50
void CRunningScript::CharInAreaCheckCommand(int32 commandId) {
    // Handles the full IS_CHAR_(STOPPED_)IN_AREA(_ON_FOOT|_IN_CAR)_2D/3D family.
    const bool is3D = commandId == COMMAND_IS_CHAR_IN_AREA_3D
        || (commandId >= COMMAND_IS_CHAR_IN_AREA_ON_FOOT_3D && commandId <= COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D);
    CollectParameters(is3D ? 8 : 6);

    auto* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);

    const auto x1 = ScriptParams[1].fParam;
    const auto y1 = ScriptParams[2].fParam;
    const auto z1 = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto x2 = ScriptParams[is3D ? 4 : 3].fParam;
    const auto y2 = ScriptParams[is3D ? 5 : 4].fParam;
    const auto z2 = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    bool isStoppedVariant = false;
    switch (commandId) {
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D:
        isStoppedVariant = true;
        break;
    default:
        break;
    }

    bool result = false;
    if (!isStoppedVariant || CTheScripts::IsPedStopped(ped)) { // NOTSA: matches original - ped deref'd even if the handle was invalid
        const auto testPos = ped->GetRealPosition(); // NOTSA: unlike Car/ObjectInAreaCheckCommand, this uses the vehicle's position instead of the ped's own if the ped is inside one (confirmed via raw disasm)
        const auto [xMin, xMax] = std::minmax(x1, x2);
        const auto [yMin, yMax] = std::minmax(y1, y2);
        result = testPos.x >= xMin && testPos.x <= xMax && testPos.y >= yMin && testPos.y <= yMax;
        if (result && is3D) {
            const auto [zMin, zMax] = std::minmax(z1, z2);
            result = testPos.z >= zMin && testPos.z <= zMax;
        }

        if (result) {
            switch (commandId) {
            case COMMAND_IS_CHAR_IN_AREA_ON_FOOT_2D:
            case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_2D:
            case COMMAND_IS_CHAR_IN_AREA_ON_FOOT_3D:
            case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_3D:
                result = !ped->IsInVehicle();
                break;
            case COMMAND_IS_CHAR_IN_AREA_IN_CAR_2D:
            case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_2D:
            case COMMAND_IS_CHAR_IN_AREA_IN_CAR_3D:
            case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D:
                result = ped->IsInVehicle();
                break;
            default:
                break; // IS_CHAR_(STOPPED_)IN_AREA_2D/3D - no extra condition
            }
        }
    }
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x1, y1, z1}, CVector{x2, y2, z2});
        } else {
            HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

// 0x489150
void CRunningScript::ObjectInAreaCheckCommand(int32 commandId) {
    // Handles: IS_OBJECT_IN_AREA_2D/3D (no stopped/on-foot/in-car variants exist for objects).
    const bool is3D = commandId == COMMAND_IS_OBJECT_IN_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    auto* object = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    const auto x1 = ScriptParams[1].fParam;
    const auto y1 = ScriptParams[2].fParam;
    const auto z1 = is3D ? ScriptParams[3].fParam : 0.0f;
    const auto x2 = ScriptParams[is3D ? 4 : 3].fParam;
    const auto y2 = ScriptParams[is3D ? 5 : 4].fParam;
    const auto z2 = is3D ? ScriptParams[6].fParam : 0.0f;
    const bool highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    const bool result = is3D
        ? object->IsWithinArea(x1, y1, z1, x2, y2, z2)
        : object->IsWithinArea(x1, y1, x2, y2);
    UpdateCompareFlag(result);

    if (highlightArea) {
        if (is3D) {
            HighlightImportantArea(CVector{x1, y1, z1}, CVector{x2, y2, z2});
        } else {
            HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2});
        }
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

// 0x487F60
void CRunningScript::CharInAngledAreaCheckCommand(int32 commandId) {
    // Handles the full IS_CHAR_(STOPPED_)IN_ANGLED_AREA(_ON_FOOT|_IN_CAR)_2D/3D family [0x5F6..0x601].
    const bool is3D = commandId >= COMMAND_IS_CHAR_IN_ANGLED_AREA_3D && commandId <= COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D;
    CollectParameters(is3D ? 9 : 7);

    auto* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);

    const CVector2D a{ ScriptParams[1].fParam, ScriptParams[2].fParam };
    const auto      az = is3D ? ScriptParams[3].fParam : 0.0f;
    const CVector2D b{ ScriptParams[is3D ? 4 : 3].fParam, ScriptParams[is3D ? 5 : 4].fParam };
    const auto      bz = is3D ? ScriptParams[6].fParam : 0.0f;
    const auto      widthAndDir  = ScriptParams[is3D ? 7 : 5].fParam; // sign picks which side of A-B corners C/D fall on
    const bool      highlightArea = ScriptParams[is3D ? 8 : 6].iParam != 0;

    bool isStoppedVariant = false;
    switch (commandId) {
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D:
        isStoppedVariant = true;
        break;
    default:
        break;
    }

    const notsa::shapes::AngledRect rect{ a, b, widthAndDir };
    const auto [minZ, maxZ] = std::minmax(az, bz);

    bool result = false;
    if (!isStoppedVariant || CTheScripts::IsPedStopped(ped)) { // NOTSA: matches original - ped deref'd even if the handle was invalid
        const auto testPos = ped->GetRealPosition(); // uses the vehicle's position instead, if ped is inside one
        result = rect.IsPointWithin(CVector2D{ testPos }) && (!is3D || (testPos.z >= minZ && testPos.z < maxZ));
        if (result) {
            switch (commandId) {
            case COMMAND_IS_CHAR_IN_ANGLED_AREA_ON_FOOT_2D:
            case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_2D:
            case COMMAND_IS_CHAR_IN_ANGLED_AREA_ON_FOOT_3D:
            case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_3D:
                result = !ped->IsInVehicle();
                break;
            case COMMAND_IS_CHAR_IN_ANGLED_AREA_IN_CAR_2D:
            case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_2D:
            case COMMAND_IS_CHAR_IN_ANGLED_AREA_IN_CAR_3D:
            case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D:
                result = ped->IsInVehicle();
                break;
            default:
                break; // IS_CHAR_(STOPPED_)IN_ANGLED_AREA_2D/3D - no extra condition
            }
        }
    }
    UpdateCompareFlag(result);

    // NOTSA: `CRunningScript::HighlightImportantAngledArea` (the member wrapper) is currently a dead
    // `NOTSA_UNREACHABLE()` stub and is missing a `z` param besides - calling the fully-implemented
    // `CTheScripts::HighlightImportantAngledArea` directly instead (same pattern as `HighlightImportantArea`).
    if (highlightArea) {
        const auto c = rect.GetCornerC();
        const auto d = rect.GetCornerD();
        const auto z = is3D ? (minZ + maxZ) / 2.f : -100.f; // -100.f = "use ground Z" sentinel, same as HighlightImportantArea's default
        CTheScripts::HighlightImportantAngledArea(
            reinterpret_cast<int32>(this) + reinterpret_cast<int32>(m_IP),
            a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y, z
        );
    }
    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugAngledSquare(a, b, rect.GetCornerC(), rect.GetCornerD());
    }
}

// 0x488780
void CRunningScript::FlameInAngledAreaCheckCommand(int32 commandId) {
    // Handles IS_FLAME_IN_ANGLED_AREA_2D/3D [0x72D/0x72E].
    // NOTSA: Unlike Char/Object, there's no "self" handle param - this scans every currently active
    // flamethrower shot (CShotInfo, NOT CFireManager) for one whose origin falls inside the area.
    const bool is3D = commandId == COMMAND_IS_FLAME_IN_ANGLED_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    const CVector2D a{ ScriptParams[0].fParam, ScriptParams[1].fParam };
    const auto      az = is3D ? ScriptParams[2].fParam : 0.0f;
    const CVector2D b{ ScriptParams[is3D ? 3 : 2].fParam, ScriptParams[is3D ? 4 : 3].fParam };
    const auto      bz = is3D ? ScriptParams[5].fParam : 0.0f;
    const auto      widthAndDir   = ScriptParams[is3D ? 6 : 4].fParam;
    const bool      highlightArea = ScriptParams[is3D ? 7 : 5].iParam != 0;

    const notsa::shapes::AngledRect rect{ a, b, widthAndDir };
    const auto [minZ, maxZ] = std::minmax(az, bz);

    bool result = false;
    for (uint8 shotId = 0; shotId < 100; shotId++) { // 100 == CShotInfo::MAX_SHOT_INFOS
        CVector shotPos;
        if (!CShotInfo::GetFlameThrowerShotPosn(shotId, shotPos)) {
            continue;
        }
        if (rect.IsPointWithin(CVector2D{ shotPos }) && (!is3D || (shotPos.z >= minZ && shotPos.z < maxZ))) {
            result = true;
            break;
        }
    }
    UpdateCompareFlag(result);

    if (highlightArea) {
        const auto c = rect.GetCornerC();
        const auto d = rect.GetCornerD();
        const auto z = is3D ? (minZ + maxZ) / 2.f : -100.f;
        CTheScripts::HighlightImportantAngledArea(
            reinterpret_cast<int32>(this) + reinterpret_cast<int32>(m_IP),
            a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y, z
        );
    }
    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugAngledSquare(a, b, rect.GetCornerC(), rect.GetCornerD());
    }
}

// 0x4883F0
void CRunningScript::ObjectInAngledAreaCheckCommand(int32 commandId) {
    // Handles IS_OBJECT_IN_ANGLED_AREA_2D/3D [0x8E3/0x8E4] - no on-foot/in-car/stopped variants exist for objects.
    const bool is3D = commandId == COMMAND_IS_OBJECT_IN_ANGLED_AREA_3D;
    CollectParameters(is3D ? 9 : 7);

    auto* obj = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    const CVector2D a{ ScriptParams[1].fParam, ScriptParams[2].fParam };
    const auto      az = is3D ? ScriptParams[3].fParam : 0.0f;
    const CVector2D b{ ScriptParams[is3D ? 4 : 3].fParam, ScriptParams[is3D ? 5 : 4].fParam };
    const auto      bz = is3D ? ScriptParams[6].fParam : 0.0f;
    const auto      widthAndDir   = ScriptParams[is3D ? 7 : 5].fParam;
    const bool      highlightArea = ScriptParams[is3D ? 8 : 6].iParam != 0;

    const notsa::shapes::AngledRect rect{ a, b, widthAndDir };
    const auto [minZ, maxZ] = std::minmax(az, bz);

    const auto testPos = obj->GetPosition(); // NOTSA: matches original - obj deref'd even if the handle was invalid; objects can't be "in a car"
    const bool result = rect.IsPointWithin(CVector2D{ testPos }) && (!is3D || (testPos.z >= minZ && testPos.z < maxZ));
    UpdateCompareFlag(result);

    if (highlightArea) {
        const auto c = rect.GetCornerC();
        const auto d = rect.GetCornerD();
        const auto z = is3D ? (minZ + maxZ) / 2.f : -100.f;
        CTheScripts::HighlightImportantAngledArea(
            reinterpret_cast<int32>(this) + reinterpret_cast<int32>(m_IP),
            a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y, z
        );
    }
    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugAngledSquare(a, b, rect.GetCornerC(), rect.GetCornerD());
    }
}

// 0x464D70
bool CRunningScript::IsPedDead(CPed* ped) const {
    ePedState pedState = ped->m_nPedState;
    return pedState == PEDSTATE_DEAD || pedState == PEDSTATE_DIE || pedState == PEDSTATE_DIE_BY_STEALTH;
}

// 0x489490
bool CRunningScript::ThisIsAValidRandomPed(ePedType pedType, bool civilian, bool gang, bool criminal) {
    switch (pedType) {
    case PED_TYPE_CIVMALE:
    case PED_TYPE_CIVFEMALE:
        return civilian;
    case PED_TYPE_GANG1:
    case PED_TYPE_GANG2:
    case PED_TYPE_GANG3:
    case PED_TYPE_GANG4:
    case PED_TYPE_GANG5:
    case PED_TYPE_GANG6:
    case PED_TYPE_GANG7:
    case PED_TYPE_GANG8:
    case PED_TYPE_GANG9:
        return gang;
    case PED_TYPE_CRIMINAL:
    case PED_TYPE_PROSTITUTE:
        return criminal;
    default:
        return false;
    }
}

// 0x485A50
void CRunningScript::DoDeathArrestCheck() {
    if (!m_IsDeathArrestCheckEnabled) {
        return;
    }

    if (!CTheScripts::IsPlayerOnAMission()) {
        return;
    }

    if (const auto& pi = FindPlayerInfo(); !pi.IsRestartingAfterDeath() && !pi.IsRestartingAfterArrest()) {
        return;
    }

    CMessages::ClearSmallMessagesOnly();
    memset(&CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag], 0, sizeof(uint32));
    ResetIP();

    m_DoneDeathArrest = true;
    m_WakeTime        = 0;
}

// 0x464F50
void CRunningScript::GetCorrectPedModelIndexForEmergencyServiceType(ePedType pedType, uint32* typeSpecificModelId) {
    switch (*typeSpecificModelId) {
    case MODEL_LAPD1:
    case MODEL_SFPD1:
    case MODEL_LVPD1:
    case MODEL_LAPDM1:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_CITYCOP;
        }
        break;
    case MODEL_CSHER:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_CSHER;
        }
        break;
    case MODEL_SWAT:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_SWAT1;
        }
        break;
    case MODEL_FBI:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_FBI;
        }
        break;
    case MODEL_ARMY:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_ARMY;
        }
        break;
    default:
        return;
    }
}

// Returns state of pad button
// 0x485B10
int16 CRunningScript::GetPadState(uint16 playerIndex, eButtonId buttonId) {
    const auto* pad = CPad::GetPad(playerIndex);
    switch (buttonId) {
    case BUTTON_LEFT_STICK_X:    return pad->NewState.LeftStickX;
    case BUTTON_LEFT_STICK_Y:    return pad->NewState.LeftStickY;
    case BUTTON_RIGHT_STICK_X:   return pad->NewState.RightStickX;
    case BUTTON_RIGHT_STICK_Y:   return pad->NewState.RightStickY;
    case BUTTON_LEFT_SHOULDER1:  return pad->NewState.LeftShoulder1;
    case BUTTON_LEFT_SHOULDER2:  return pad->NewState.LeftShoulder2;
    case BUTTON_RIGHT_SHOULDER1: return pad->NewState.RightShoulder1;
    case BUTTON_RIGHT_SHOULDER2: return pad->NewState.RightShoulder2;
    case BUTTON_DPAD_UP:         return pad->NewState.DPadUp;
    case BUTTON_DPAD_DOWN:       return pad->NewState.DPadDown;
    case BUTTON_DPAD_LEFT:       return pad->NewState.DPadLeft;
    case BUTTON_DPAD_RIGHT:      return pad->NewState.DPadRight;
    case BUTTON_START:           return pad->NewState.Start;
    case BUTTON_SELECT:          return pad->NewState.Select;
    case BUTTON_SQUARE:          return pad->NewState.ButtonSquare;
    case BUTTON_TRIANGLE:        return pad->NewState.ButtonTriangle;
    case BUTTON_CROSS:           return pad->NewState.ButtonCross;
    case BUTTON_CIRCLE:          return pad->NewState.ButtonCircle;
    case BUTTON_LEFTSHOCK:       return pad->NewState.ShockButtonL;
    case BUTTON_RIGHTSHOCK:      return pad->NewState.ShockButtonR;
    default:                     return OR_CONTINUE;
    }
}

// 0x46AF50
void CRunningScript::ScriptTaskPickUpObject(int32 commandId) {
    plugin::CallMethod<0x46AF50, CRunningScript*, int32>(this, commandId);
}

// 0x464DC0
void CRunningScript::SetCharCoordinates(CPed& ped, CVector posn, bool warpGang, bool offset) {
    CWorld::PutToGroundIfTooLow(posn);

    CVehicle* vehicle = ped.GetVehicleIfInOne();
    if (vehicle) {
        posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
        vehicle->Teleport(posn, false);
        CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);
    } else {
        posn.z += offset ? ped.GetDistanceFromCentreOfMassToBaseOfModel() : 0.0f;
        CTheScripts::ClearSpaceForMissionEntity(posn, &ped);
        auto* group = CPedGroups::GetPedsGroup(&ped);
        if (group && group->GetMembership().IsLeader(&ped) && warpGang) {
            group->Teleport(posn);
        } else {
            ped.Teleport(posn, false);
        }
    }
}

// 0x463CA0
tScriptParam* CRunningScript::GetPointerToLocalVariable(int32 loc) {
    return &GetLocal<tScriptParam>(loc);
}

/*!
 * @addr 0x463CC0
 * @brief Returns pointer to a local script variable.
 *
 * @param arrayBaseOffset          The offset of the array
 * @param index                    Index of the variable inside the array
 * @param arrayEntriesSizeAsParams Size of 1 variable in the array (In terms of `tScriptParam`'s - So for a regular `int` (or float, etc) variable this will be `1`, for long strings it's `4` and for short one's it's `2`)
 */
tScriptParam* CRunningScript::GetPointerToLocalArrayElement(int32 arrayBaseOffset, uint16 index, uint8 arrayEntriesSizeAsParams) {
    return &GetArrayLocal<tScriptParam>(arrayBaseOffset, index, arrayEntriesSizeAsParams);
}

/*!
 * Returns pointer to script variable of any type.
 * @addr 0x464790
 */
tScriptParam* CRunningScript::GetPointerToScriptVariable(eScriptVariableType) {
    uint8  arrElemSize;
    uint16 arrVarOffset;
    int32  arrElemIdx;

    int8 type = CTheScripts::Read1ByteFromScript(m_IP);
    switch (type) {
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[index]);
    }
    case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
    case SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        return GetPointerToLocalVariable(index);
    }

    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY)
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[LONG_STRING_SIZE * arrElemIdx + arrVarOffset]);
        else if (type == SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY)
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[SHORT_STRING_SIZE * arrElemIdx + arrVarOffset]);
        else // SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[4 * arrElemIdx + arrVarOffset]);

    case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
    case SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY)
            arrElemSize = 4;
        else if (type == SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY)
            arrElemSize = 2;
        else // SCRIPT_PARAM_LOCAL_NUMBER_ARRAY
            arrElemSize = 1;
        return GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, arrElemSize);

    default:
        NOTSA_UNREACHABLE();
    }
}

/*!
 * Returns offset of global variable
 * @addr 0x464700
 */
uint16 CRunningScript::GetIndexOfGlobalVariable() {
    switch (const auto t = GetAtIPAs<uint8>()) {
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        return GetAtIPAs<uint16>();
    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY: {
        uint16 base;
        int32  idx;
        ReadArrayInformation(true, &base, &idx);
        return base + sizeof(tScriptParam) * idx;
    }
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x464080
void CRunningScript::CollectParameters(int16 count) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    for (auto i = 0; i < count; i++) {
        switch (CTheScripts::Read1ByteFromScript(m_IP)) {
        case SCRIPT_PARAM_STATIC_INT_32BITS:
            ScriptParams[i].iParam = CTheScripts::Read4BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            ScriptParams[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            ScriptParams[i] = *GetPointerToLocalVariable(index);
            break;
        }
        case SCRIPT_PARAM_STATIC_INT_8BITS:
            ScriptParams[i].iParam = CTheScripts::Read1ByteFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_INT_16BITS:
            ScriptParams[i].iParam = CTheScripts::Read2BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_FLOAT:
            ScriptParams[i].fParam = CTheScripts::ReadFloatFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            ScriptParams[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            ScriptParams[i] = *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1);
            break;
        }
    }
}

/*!
 * Collects parameter and returns it.
 * @addr 0x464250
 */
int32 CRunningScript::CollectNextParameterWithoutIncreasingPC() {
    uint16 arrVarOffset;
    int32  arrElemIdx;
    uint8* ip = m_IP;
    int32  result = -1;

    switch (CTheScripts::Read1ByteFromScript(m_IP)) {
    case SCRIPT_PARAM_STATIC_INT_32BITS:
    case SCRIPT_PARAM_STATIC_FLOAT:
        result = CTheScripts::Read4BytesFromScript(m_IP);
        break;
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        result = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
        break;
    }
    case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        result = GetPointerToLocalVariable(index)->iParam;
        break;
    }
    case SCRIPT_PARAM_STATIC_INT_8BITS:
        result = CTheScripts::Read1ByteFromScript(m_IP);
        break;
    case SCRIPT_PARAM_STATIC_INT_16BITS:
        result = CTheScripts::Read2BytesFromScript(m_IP);
        break;
    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
        ReadArrayInformation(false, &arrVarOffset, &arrElemIdx);
        result = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
        break;
    case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
        ReadArrayInformation(false, &arrVarOffset, &arrElemIdx);
        result = GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1)->iParam;
        break;
    }

    m_IP = ip;
    return result;
}

/*!
 * @addr 0x464370
 */
void CRunningScript::StoreParameters(int16 count) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    for (auto i = 0; i < count; i++) {
        switch (CTheScripts::Read1ByteFromScript(m_IP)) {
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]) = ScriptParams[i].iParam;
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            *GetPointerToLocalVariable(index) = ScriptParams[i];
            break;
        }
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]) = ScriptParams[i].iParam;
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1) = ScriptParams[i];
            break;
        }
    }
}

// Reads array var base offset and element index from index variable.
// 0x463CF0
void CRunningScript::ReadArrayInformation(int32 updateIP, uint16* outArrayBase, int32* outArrayIndex) {
    const auto op = GetAtIPAs<scm::ArrayAccess>(updateIP);
    *outArrayIndex = op.IdxVarIsGlobal
        ? GetGlobal<int32>(op.IdxVarLoc)
        : GetLocal<int32>(op.IdxVarLoc);
    *outArrayBase = op.ArrayBase;
}

// Collects parameters and puts them to local variables of new script
// 0x464500
void CRunningScript::ReadParametersForNewlyStartedScript(CRunningScript* newScript) {
    uint16 arrVarOffset;
    int32  arrElemIdx;
    int8   type = CTheScripts::Read1ByteFromScript(m_IP);

    for (int i = 0; type != SCRIPT_PARAM_END_OF_ARGUMENTS; type = CTheScripts::Read1ByteFromScript(m_IP), i++) {
        switch (type) {
        case SCRIPT_PARAM_STATIC_INT_32BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read4BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            newScript->m_LocalVars[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            newScript->m_LocalVars[i] = *GetPointerToLocalVariable(index);
            break;
        }
        case SCRIPT_PARAM_STATIC_INT_8BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read1ByteFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_INT_16BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read2BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_FLOAT:
            newScript->m_LocalVars[i].fParam = CTheScripts::ReadFloatFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            newScript->m_LocalVars[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            newScript->m_LocalVars[i] = *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1);
            break;
        default:
            break;
        }
    }
}

// Collects string parameter
// 0x463D50
void CRunningScript::ReadTextLabelFromScript(char* buffer, uint8 nBufferLength) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    int8 type = CTheScripts::Read1ByteFromScript(m_IP);
    switch (type) {
    case SCRIPT_PARAM_STATIC_SHORT_STRING:
        for (auto i = 0; i < SHORT_STRING_SIZE; i++)
            buffer[i] = CTheScripts::Read1ByteFromScript(m_IP);
        break;

    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, SHORT_STRING_SIZE, (char*)&CTheScripts::ScriptSpace[index], SHORT_STRING_SIZE);
        break;
    }

    case SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, SHORT_STRING_SIZE, (char*) GetPointerToLocalVariable(index), SHORT_STRING_SIZE);
        break;
    }

    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY)
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[SHORT_STRING_SIZE * arrElemIdx + arrVarOffset], SHORT_STRING_SIZE);
        else
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[LONG_STRING_SIZE * arrElemIdx + arrVarOffset], std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;

    case SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY)
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 2), SHORT_STRING_SIZE);
        else {
            const auto bufferLength = std::min<uint8>(nBufferLength, LONG_STRING_SIZE);
            strncpy_s(buffer, bufferLength, (char*)GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 4), bufferLength);
        }
        break;

    case SCRIPT_PARAM_STATIC_PASCAL_STRING:
    {
        int16 nStringLen = CTheScripts::Read1ByteFromScript(m_IP); // sign extension. max size = 127, not 255
        for (auto i = 0; i < nStringLen; i++)
            buffer[i] = CTheScripts::Read1ByteFromScript(m_IP);

        if (nStringLen < nBufferLength)
            memset(&buffer[(uint8)nStringLen], 0, (uint8)(nBufferLength - nStringLen));
        break;
    }

    case SCRIPT_PARAM_STATIC_LONG_STRING:
        // slightly changed code: original code is a bit messy and calls Read1ByteFromScript
        // in a loop and does some additional checks to ensure that buffer can hold the data
        strncpy_s(buffer, LONG_STRING_SIZE, (char*)m_IP, std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        m_IP += LONG_STRING_SIZE;
        break;

    case SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, LONG_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[index], std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;
    }

    case SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, LONG_STRING_SIZE, (char*) GetPointerToLocalVariable(index), std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;
    }

    default:
        break;
    }
}

// Updates comparement flag, used in conditional commands
// 0x4859D0
void CRunningScript::UpdateCompareFlag(bool state) {
    if (m_NotFlag)
        state = !state;

    if (m_AndOrState == ANDOR_NONE) {
        m_CondResult = state;
        return;
    }

    if (m_AndOrState >= ANDS_1 && m_AndOrState <= ANDS_8) {
        m_CondResult &= state;
        if (m_AndOrState == ANDS_1)
            m_AndOrState = ANDOR_NONE;
        else
            m_AndOrState--;

        return;
    }

    if (m_AndOrState >= ORS_1 && m_AndOrState <= ORS_8) {
        m_CondResult |= state;
        if (m_AndOrState == ORS_1)
            m_AndOrState = ANDOR_NONE;
        else
            m_AndOrState--;

        return;
    }
}

// Sets instruction pointer, used in GOTO-like commands
// 0x464DA0
void CRunningScript::UpdatePC(int32 newIP) {
    m_IP = newIP >= 0
        ? &CTheScripts::ScriptSpace[newIP]
        : m_BaseIP + std::abs(newIP);
}

// 0x469EB0, inlined
OpcodeResult CRunningScript::ProcessOneCommand() {
    ++CTheScripts::CommandsExecuted;

    const auto op = GetAtIPAs<scm::Instruction>();

    // Check if IP is valid pre-return
    notsa::ScopeGuard guardIP{[this]() {
        const auto next{ GetAtIPAs<scm::Instruction>(false) };
        VERIFY(next.Command <= COMMAND_HIGHEST_VANILLA_ID);
    }};

#ifdef NOTSA_SCRIPT_TRACING
    // snprintf is faster (in debug at least) - Gotta stick to it for now
    char msg[4096];
    sprintf_s(msg, "[%s][IP: 0x%X + 0x%X]: %s [0x%X]", m_szName, LOG_PTR(m_pBaseIP), LOG_PTR(m_IP - m_pBaseIP), notsa::script::GetScriptCommandName((eScriptCommands)op.Command).data(), (size_t)op.Command);
    SPDLOG_LOGGER_TRACE(logger, msg);
    //SPDLOG_LOGGER_TRACE(logger, "[{}][IP: {:#x} + {:#x}]: {} [{:#x}]", BaseFilename, LOG_PTR(m_pBaseIP), LOG_PTR(m_IP - m_pBaseIP), notsa::script::GetScriptCommandName((eScriptCommands)op.command), (size_t)op.command);
#endif
    
    m_NotFlag = op.NotFlag;

    if (const auto handler = CustomCommandHandlerOf((eScriptCommands)(op.Command))) {
        return std::invoke(handler, this);
    } else {
        return std::invoke(s_OriginalCommandHandlerTable[(size_t)op.Command / 100], this, (eScriptCommands)(op.Command));
    }
}

// 0x469F00
OpcodeResult CRunningScript::Process() {
    if (m_SceneSkipIP && CCutsceneMgr::IsCutsceneSkipButtonBeingPressed()) {
        CHud::m_BigMessage[1][0] = 0;
        UpdatePC(std::exchange(m_SceneSkipIP, 0));
        m_WakeTime = 0;
    }

    if (m_UsesMissionCleanup) {
        DoDeathArrestCheck();
    }

    if (m_ThisMustBeTheOnlyMissionRunning && CTheScripts::FailCurrentMission == 1) {
        if (m_StackDepth > 0) {
            ResetIP();
        }
    }

    CTheScripts::ReinitialiseSwitchStatementData();

    if (CTimer::GetTimeInMS() >= (uint32)m_WakeTime) {
        while (ProcessOneCommand() == OR_CONTINUE)
            ; // Process commands
    }

    return OR_CONTINUE;
}

void CRunningScript::HighlightImportantArea(CVector2D from, CVector2D to, float z) {
    CTheScripts::HighlightImportantArea(reinterpret_cast<int32>(this) + reinterpret_cast<int32>(m_IP), from.x, from.y, to.x, to.y, z);
}

void CRunningScript::HighlightImportantArea(CRect area, float z) {
    HighlightImportantArea(area.GetTopLeft(), area.GetBottomRight(), z);
}

void CRunningScript::HighlightImportantArea(CVector from, CVector to) {
    HighlightImportantArea(CVector2D{ from }, CVector2D{ to }, (from.z + to.z) / 2.f);
}

void CRunningScript::HighlightImportantAngledArea(uint32 id, CVector2D a, CVector2D b, CVector2D c, CVector2D d) {
    NOTSA_UNREACHABLE(); // Fuck this, we dont need it!
}

notsa::script::CommandHandlerFunction& CRunningScript::CustomCommandHandlerOf(eScriptCommands command) {
    return s_CustomCommandHandlerTable[(size_t)(command)];
}

void CRunningScript::ResetIP() {
    assert(m_StackDepth > 0); // Original bug...
    do {
        m_IP = std::exchange(m_IPStack[m_StackDepth--], nullptr); // NOTSA: Also clear the stack, we don't need it anymore
    } while (m_StackDepth);
}
