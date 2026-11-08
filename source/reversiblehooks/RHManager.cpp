#include "StdInc.h"

#ifdef NOTSA_STANDALONE_DUMP_HOOKS_ONLY
#include "ReversibleHook/NullHook.h"
#else
#include "ReversibleHook/VirtualHook.h"
#endif
#include "ReversibleHook/ScriptCommandHook.h"
#include "RHManager.h"

static constexpr auto HOOKS_CHECK_INTERVAL = std::chrono::milliseconds{ 500 };

namespace ReversibleHooks {
void RHManager::CheckAll() {
    if (const auto now = HooksCheckClock::now(); now - m_LastHooksCheckTime > HOOKS_CHECK_INTERVAL) {
        m_LastHooksCheckTime = now;
        m_RootHookCategory->ForEachItem([](const auto item) {
            item->GetHook()->Check();
        });
    }
}

void RHManager::WriteHooksToFile(const std::filesystem::path& file) {
    const auto path = std::filesystem::weakly_canonical(file);
    if (std::ofstream of{ file }) {
        json::array_t arr;
        [&](this auto&& Self, const HookCategory& c) -> void {
            for (const auto item : c.Items()) {
                json j{};
                to_json(j, *item);
                arr.emplace_back(std::move(j));
            }
            for (const auto subcat : c.SubCategories()) {
                Self(*subcat);
            }
        }(*GetRootCategory());
        of << std::setw(4) << arr;
        NOTSA_LOG_INFO("Hooks written to `{}`", path.string());
    } else {
        NOTSA_LOG_ERR("Failed to open file `{}` for writing hooks!", path.string());
    }
}

void RHManager::InstallVirtual(
    std::string_view   category,
    std::string        fnName,
    Utility::VMTInfo   vmtInfoOur,
    void*              fnAddressOur,
    Utility::VMTInfo   vmtInfoGTA,
    void*              fnAddressGTA,
    HookInstallOptions opt
) {
#ifdef NOTSA_STANDALONE_DUMP_HOOKS_ONLY
    AddHookToCategory(category, opt, std::make_shared<ReversibleHook::NullHook>(
        std::move(fnName),
        fnAddressOur,
        fnAddressGTA
    ));
#else
    const auto idx = vmtInfoGTA.FindIndexOf(fnAddressGTA);
    // We can't do `vmtInfoOur.FindIndexOf(fnAddressOur)` because `fnAddressOur` points to the thunk, while the address in the VMT is pointing to the actual function

    if (opt.Overrides) {
        AddHookToCategory(category, opt, std::make_shared<ReversibleHook::VirtualHook>(
            std::move(fnName),
            vmtInfoOur.GetEntryAddressAt(idx),
            vmtInfoGTA.GetEntryAddressAt(idx)
        ));
    } else {
        AddHookToCategory(category, opt, std::make_shared<ReversibleHook::VMTRedirectHook>(
            std::move(fnName),
            vmtInfoOur.GetEntryAddressAt(idx),
            vmtInfoGTA.GetEntryAddressAt(idx)
        ));
    }
#endif
}

void RHManager::AddHookToCategory(std::string_view path, HookInstallOptions opt, std::shared_ptr<ReversibleHook::TwoWayHook> hook) {
    // Debug aid: GTAR_UNHOOK=name1,name2 starts every hook whose `category/class/function` path contains one of the names
    // unhooked, so the exe's own code runs instead. Comparing a run with and without it shows which reversed code is at fault.
    static const auto s_UnhookNames = [] {
        std::vector<std::string> names;
        if (const char* env = std::getenv("GTAR_UNHOOK")) {
            for (auto&& part : std::string_view{ env } | rng::views::split(',')) {
                if (!part.empty()) {
                    names.emplace_back(std::string_view{ part });
                }
            }
        }
        return names;
    }();
    if (!s_UnhookNames.empty() && opt.Reversed) {
        const auto fullPath = std::format("{}/{}", path, hook->Name());
        if (rng::any_of(s_UnhookNames, [&](const std::string& name) { return fullPath.contains(name); })) {
            opt.State = HookInstallOptions::HS::Unhooked;
        }
    }

    GetRootCategory()->FindCategoryByPath(path, true)->AddItem(std::make_shared<HookCategoryItem>(
        std::move(hook),
        opt.Locked,
        opt.Reversed,
        opt.State,
        opt.InstallSrcLoc
    ));
}

#ifdef NOTSA_WITH_SCRIPT_COMMAND_HOOKS
void RHManager::InstallScriptCommand(std::string_view path, eScriptCommands cmd, HookInstallOptions opt) {
    AddHookToCategory(
        path,
        opt,
        std::make_shared<ReversibleHook::ScriptCommandHook>(cmd) // This can stay a regular hook even in standalone, because it doesn't access the game in any way
    );
}
#endif
}; // namespace ReversibleHooks
