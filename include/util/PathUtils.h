
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/windows_include.h"
#include <filesystem>

namespace MCF::Utils
{
    /// Return the path to the executable (EXE or DLL) that compiled this function.
    /// This can be used to parse one's config file or other resources present with the mod.
    /// If this fails, will default to the current working directory.
    std::filesystem::path ModulePath() {
        HMODULE hModule = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, "", &hModule)) {
            return std::filesystem::current_path();
        }

        char module_path[MAX_PATH];
        auto n_written = GetModuleFileNameA(hModule, module_path, sizeof(module_path));
        if (n_written == 0 || n_written >= sizeof(module_path)) {
            return std::filesystem::current_path();
        }
        else {
            return std::filesystem::path(module_path).parent_path();
        }
    }
}