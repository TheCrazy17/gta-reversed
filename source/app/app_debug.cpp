#include "StdInc.h"

#include "app_debug.h"
#include <windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>
#include <csignal>
#include <cstdlib>
#include <assert.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <extensions/debug.hpp>

#define FINAL 0

#ifndef FINAL
RtCharset* debugCharset;
bool g_bDebugRenderGroups;
#endif

#if !defined(FINAL)
static bool charsetOpen;
void OpenCharsetSafe() {
    if (!charsetOpen)
        RtCharsetOpen();
    charsetOpen = true;
}
#endif

// 0x734610
void CreateDebugFont() {
#ifndef FINAL
    RwRGBA color = { 255, 255, 128, 255 };
    RwRGBA colorbg = { 0, 0, 0, 0 };
    OpenCharsetSafe();
    debugCharset = RtCharsetCreate(&color, &colorbg);
#endif
}

// 0x734620
void DestroyDebugFont() {
#ifndef FINAL
    RtCharsetDestroy(debugCharset);
    RtCharsetClose();
    charsetOpen = false;
#endif
}

// unused
// 0x734630
void ObrsPrintfString(const char* str, int16 x, int16 y) {
#ifndef FINAL
    RtCharsetPrintBuffered(debugCharset, str, x * 8, y * 16, true);
#endif
}

// 0x734640
void FlushObrsPrintfs() {
#ifndef FINAL
    RtCharsetBufferFlush();
#endif
}

// Takes the context by reference because StackWalk modifies it while walking
static void DumpCallStack(CONTEXT& context) {
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread  = GetCurrentThread();

    STACKFRAME stackFrame = {};
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrFrame.Offset = context.Ebp;

    while (StackWalk(
        IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame, &context, NULL,
        SymFunctionTableAccess, SymGetModuleBase, NULL))
    {
        NOTSA_LOG_INFO("\t{}", notsa::debug::GetFunctionInfoAtAddress(stackFrame.AddrPC.Offset));
    }
}

// Logs every C++ exception at the moment it is thrown.
// Our hooks run inside the exe's frames, which have no unwind data; an exception that leaves a hook therefore ends in
// std::terminate and the process quits with exit code 3 without ever reaching the unhandled exception filter.
// The throw site is the only place where the cause can still be seen.
static bool s_LogCxxExceptions = false;

static LONG CALLBACK CxxExceptionLogger(PEXCEPTION_POINTERS pExceptionInfo) {
    constexpr DWORD MSVC_CXX_EXCEPTION = 0xE06D7363;
    constexpr int   MAX_LOGGED         = 24; // Some code throws and catches on purpose, so don't flood the log

    static int s_NumLogged = 0;
    if (!s_LogCxxExceptions || pExceptionInfo->ExceptionRecord->ExceptionCode != MSVC_CXX_EXCEPTION || s_NumLogged >= MAX_LOGGED) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    s_NumLogged++;

    NOTSA_LOG_INFO("C++ exception thrown (#{}); call stack:", s_NumLogged);
    CONTEXT context = *pExceptionInfo->ContextRecord;
    DumpCallStack(context);

    spdlog::apply_all([](auto&& logger) {
        logger->flush();
    });
    return EXCEPTION_CONTINUE_SEARCH;
}

#ifdef _DEBUG
// The debug CRT reports failed asserts and invalid arguments (for example fgets on a null FILE*) with a message box.
// The box does not say who called the CRT, so log the message together with the call stack before it is shown.
static int CrtReportLogger(int reportType, char* message, int* /*returnValue*/) {
    if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR) {
        NOTSA_LOG_ERR("CRT {} report: {}", reportType == _CRT_ASSERT ? "assert" : "error", message);
        CONTEXT context{};
        RtlCaptureContext(&context);
        DumpCallStack(context);
        spdlog::apply_all([](auto&& logger) {
            logger->flush();
        });
    }
    return FALSE; // Not handled; the CRT still shows its dialog
}

// assert() does not go through the report hook above; it calls _wassert, which shows its own box.
// That box is the only trace of a failed assert, with no call stack, so the stack is logged before the box is shown.
// The box itself is kept as the CRT shows it (abort, retry, ignore).
static void __cdecl WAssertLogger(const wchar_t* expression, const wchar_t* file, unsigned line) {
    const auto ToUtf8 = [](const wchar_t* str) {
        std::string out((size_t)std::max(0, WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr)), '\0');
        WideCharToMultiByte(CP_UTF8, 0, str, -1, out.data(), (int)out.size(), nullptr, nullptr);
        if (!out.empty()) {
            out.pop_back(); // The terminating null
        }
        return out;
    };

    NOTSA_LOG_ERR("Assertion failed: {} ({}:{})", ToUtf8(expression), ToUtf8(file), line);
    CONTEXT context{};
    RtlCaptureContext(&context);
    DumpCallStack(context);
    spdlog::apply_all([](auto&& logger) {
        logger->flush();
    });

    wchar_t program[MAX_PATH]{};
    GetModuleFileNameW(nullptr, program, MAX_PATH);

    wchar_t text[4096]{};
    _snwprintf_s(
        text, _TRUNCATE,
        L"Assertion failed!\n\nProgram: %ls\nFile: %ls\nLine: %u\n\nExpression: %ls\n\n"
        L"For information on how your program can cause an assertion failure, see the Visual C++ documentation on asserts\n\n"
        L"(Press Retry to debug the application - JIT must be enabled)",
        program, file, line, expression
    );
    switch (MessageBoxW(nullptr, text, L"Microsoft Visual C++ Runtime Library", MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL)) {
    case IDABORT:
        raise(SIGABRT);
        _exit(3);
    case IDRETRY:
        __debugbreak();
        break;
    default: // Ignore
        break;
    }
}

// The CRT is linked statically, so _wassert is a function of this module and can be redirected with a jump
static void RedirectWAssertToLogger() {
    auto* const target = reinterpret_cast<uint8*>(&_wassert);
    DWORD       oldProtect{};
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return;
    }
    target[0] = 0xE9; // jmp rel32
    *reinterpret_cast<int32*>(target + 1) = (int32)(reinterpret_cast<uint8*>(&WAssertLogger) - (target + 5));
    VirtualProtect(target, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
}
#endif

// This probably should be in winps :D
LONG WINAPI WindowsExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    // If this function itself crashes it's invoked again
    // So let's prevent the recusion with this simple hack
    static bool s_HasHandled = false;
    if (s_HasHandled) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    s_HasHandled = true;

    spdlog::apply_all([](auto&& logger) {
        logger->dump_backtrace();
    });
    
    const auto Section = [](const char* name) {
        SPDLOG_INFO("*********{}**********", name);
    };

    Section("UNHANDLED EXCEPTION");

    SPDLOG_INFO("Exception Code: {:#010x}", pExceptionInfo->ExceptionRecord->ExceptionCode);
    SPDLOG_INFO("Exception Flags: {:#010x}", pExceptionInfo->ExceptionRecord->ExceptionFlags);
    SPDLOG_INFO("Exception Address: {:#010x}", (uintptr_t)pExceptionInfo->ExceptionRecord->ExceptionAddress);

    // Dump exception parameters
    Section("PARAMETERS");
    {
        SPDLOG_INFO("Parameters[{}]:", pExceptionInfo->ExceptionRecord->NumberParameters);
        for (DWORD i = 0; i < pExceptionInfo->ExceptionRecord->NumberParameters; i++) {
            SPDLOG_INFO("{:>8}: {:#010x}", i, pExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
        }
    }

    CONTEXT& context = *pExceptionInfo->ContextRecord;

    Section("REGISTERS");
    {
        const auto DumpRegister = [](auto name, auto value) {
            SPDLOG_INFO("\t{}: {:#010x}", name, value);
        };
        DumpRegister("EAX", context.Eax);
        DumpRegister("EBX", context.Ebx);
        DumpRegister("ECX", context.Ecx);
        DumpRegister("EDX", context.Edx);
        DumpRegister("ESI", context.Esi);
        DumpRegister("EDI", context.Edi);
        DumpRegister("EBP", context.Ebp);
        DumpRegister("ESP", context.Esp);
        DumpRegister("EIP", context.Eip);
        DumpRegister("EFLAGS", context.EFlags);
    }

#if 0
    Section("LOADED MODULES");
    {
        HANDLE hProcess = GetCurrentProcess();

        HMODULE hModules[1024];
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            const DWORD numModules = cbNeeded / sizeof(HMODULE);
            for (DWORD i = 0; i < numModules; i++) {
                MODULEINFO moduleInfo;
                if (GetModuleInformation(hProcess, hModules[i], &moduleInfo, sizeof(moduleInfo))) {
                    char moduleName[MAX_PATH];
                    GetModuleBaseName (hProcess, hModules[i], moduleName, sizeof(moduleName));

                    SPDLOG_INFO("\t{:#010x}: {}", LOG_PTR(moduleInfo.lpBaseOfDll), moduleName);
                }
            }
        }
    }
#endif

    Section("CALL STACK");
    DumpCallStack(context);

    Section("END OF UNHANDLED EXCEPTION");

    spdlog::apply_all([](auto&& logger) {
        logger->flush();
    });

    return EXCEPTION_EXECUTE_HANDLER;
}

notsa::Logging::Logging() {
    using namespace std::chrono_literals;
#if 0
    while (!IsDebuggerPresent()) {
        Sleep(1);
    }
#endif
    spdlog::init_thread_pool(1 << 16, 4);

    // See https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    spdlog::set_pattern("%^[%l][%H:%M:%S.%e][%s:%#]: %v%$");
    spdlog::enable_backtrace(128);
    spdlog::set_level(spdlog::level::debug);

    m_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.log"));
    m_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    spdlog::set_default_logger(Create("default"));
    spdlog::flush_every(100ms);

    SetUnhandledExceptionFilter(WindowsExceptionHandler);
    AddVectoredExceptionHandler(1, CxxExceptionLogger);
#ifdef _DEBUG
    _CrtSetReportHook(CrtReportLogger);
    RedirectWAssertToLogger();
#endif
}

void notsa::Logging::EnableCxxExceptionLogging() {
    s_LogCxxExceptions = true;
}

notsa::Logging::~Logging() {
    spdlog::shutdown();
}

auto notsa::Logging::Create(std::string name, std::optional<spdlog::level::level_enum> level) -> notsa::log_ptr {
    auto logger = std::make_shared<spdlog::logger>(name, m_sinks.begin(), m_sinks.end());
    spdlog::initialize_logger(logger);
    if (level.has_value()) {
        logger->set_level(*level);
    }
    return logger;
}
