
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "Bootstrap.h"
#include "include/util/PathUtils.h"
#include "CoreSettings.h"

namespace MCF
{
    void Bootstrap::Init(bool used_launcher)
    {
        launcher_dir = Utils::ModulePath();

        bool early_console = false;
        bool exit_on_fail = false;

        try {
            CoreSettings = toml::parse((launcher_dir / "MCFSettings.toml").string());
            early_console = toml::find<bool>(CoreSettings, "logging", "early_console");
            exit_on_fail = toml::find<bool>(CoreSettings, "loading", "terminate_on_load_failure");
        }
        catch(std::exception& e) {
            MessageBoxA(nullptr, e.what(), "MCF Settings Parse Error", MB_OK | MB_ICONERROR);
            exit(0);
        }

        if (early_console) {
            AllocConsole();
            FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
            freopen_s(&fpstdin, "CONIN$", "r", stdin);
            freopen_s(&fpstdout, "CONOUT$", "w", stdout);
            freopen_s(&fpstderr, "CONOUT$", "w", stderr);

            if (!used_launcher) {
                printf("WARNING: MCF was not loaded by its launcher. This may cause issues.\n");
            }
        }

        if (!comp_man->InitializeCore()) {
            MessageBoxA(nullptr, "MCF Core Library failed to initialize. Set early_console to true for more information", "MCF Error", MB_OK | MB_ICONERROR);
            exit(0);
        }

        // At this point we should be able to use core components
        // Load the MCF lib first, so that modules that rely on load events (AobScanMan) are loaded before external mods,
        // and so we have things like the Windows CLI and file logger components loaded
        const char* mcf[] = { "MCF.dll" };
        if (!comp_man->LoadDlls(mcf, 1) && exit_on_fail) {
            // TODO: Proper error message
            exit(0);
        }

        Dependency<Logger> logger;
        std::vector<std::string> dlls_to_load;

        if (std::filesystem::exists(launcher_dir / "mods")) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(launcher_dir / "mods")) {
                if (entry.is_regular_file() && entry.path().extension() == "dll") {
                    dlls_to_load.push_back(entry.path().string());
                }
            }
        }
        else {
            logger->Warn("Bootstrap", "WARNING: mods folder could not be found. Nothing will be loaded.");
        }

        std::vector<const char*> dll_cstrs;
        for (const auto& dll : dlls_to_load)
            dll_cstrs.push_back(dll.c_str());

        if (!comp_man->LoadDlls(dll_cstrs.data(), dll_cstrs.size()) && exit_on_fail) {
            // TODO: Proper error message
            exit(0);
        }

        // Load external DLLs into the process

        Test1::value += 1;
        logger->Info("TEST1", "{}", Test1::value);
        Test2::value += 1;
        logger->Info("TEST2", "{}", Test2::value);

        std::vector<std::string> external_dlls = toml::find_or<std::vector<std::string>>(
                CoreSettings, "loading", "external_dll_paths", std::vector<std::string>());

        for (const auto& dll : external_dlls) {
            if (!std::filesystem::exists(dll)) {
                logger->Error("Bootstrap", "External mod \"{}\" could not be found", dll);
            }
            else if (!LoadLibraryA(dll.c_str())) {
                logger->Error("Bootstrap", "Failed to load external mod \"{}\" (error code {})",
                             dll, GetLastError());
            }
        }
    }

    Bootstrap &Bootstrap::Get()
    {
        static Bootstrap ins;
        return ins;
    }

    Bootstrap::Bootstrap()
    {
        bool s = true;
        comp_man = std::make_unique<ComponentManImp>(&s);
    }
}

#ifdef MCF_EXPORTS
extern "C" MCF_C_API MCF::IComponent * MCF_GetComponent(const char* version_string)
{
    return MCF::Bootstrap::Get().comp_man->GetComponent(version_string);
}

extern "C" MCF_C_API MCF::IComponent * MCF_AcquireComponent(const char* version_string)
{
    return MCF::Bootstrap::Get().comp_man->AcquireComponent(version_string);
}

extern "C" MCF_C_API bool MCF_ReleaseComponent(MCF::IComponent* component)
{
    return MCF::Bootstrap::Get().comp_man->ReleaseComponent(component);
}

#endif