
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include <include/third-party/windows_include.h>
#include <string>
#include <cstdio>
#include <psapi.h>
#include "toml11/toml.hpp"
#include "SteamUtils.hpp"

bool injectDll(HANDLE hProc, const char* dllPath)
{
    char dllFullPath[MAX_PATH + 1];
    SIZE_T dllFullPathSz = (SIZE_T)GetFullPathNameA(dllPath, MAX_PATH + 1, dllFullPath, nullptr) + 1;
    if (dllFullPathSz == 1 || dllFullPathSz > sizeof(dllFullPath)) {
        printf("[DLL INJECT] GetFullPathNameA failed! Size = %I64d, Error = %lu\n", dllFullPathSz, GetLastError());
        return false;
    }
    LPVOID lib = VirtualAllocEx(hProc, nullptr, dllFullPathSz, MEM_COMMIT, PAGE_READWRITE);
    if (lib == nullptr) {
        printf("[DLL INJECT] VirtualAllocEx failed! Error = %lu\n", GetLastError());
        return false;
    }
    SIZE_T nWritten = 0;
    if (!WriteProcessMemory(hProc, lib, dllFullPath, dllFullPathSz, &nWritten) || nWritten != dllFullPathSz) {
        printf("[DLL INJECT] WriteProcessMemory failed! Error = %lu, nWritten = %I64d (expected %I64d)\n", GetLastError(), nWritten, dllFullPathSz);
        VirtualFreeEx(hProc, lib, 0, MEM_RELEASE);
        return false;
    }
    auto loadLibCall = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibCall, lib, 0, nullptr);
    if (hThread == nullptr) {
        printf("[DLL INJECT] CreateRemoteThread failed! Error = %lu\n", GetLastError());
        VirtualFreeEx(hProc, lib, 0, MEM_RELEASE);
        return false;
    }
    DWORD waitResult = WaitForSingleObject(hThread, 10000);

    CloseHandle(hThread);
    VirtualFreeEx(hProc, lib, 0, MEM_RELEASE);

    if (waitResult != WAIT_OBJECT_0) {
        printf("[DLL INJECT] WaitForSingleObject failed! Return value = %lu, Error = %lu\n", waitResult, GetLastError());
        return false;
    }

    // Check if the DLL was successfully loaded.
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char modName[MAX_PATH];

            if (GetModuleFileNameExA(hProc, hMods[i], modName, sizeof(modName) / sizeof(char)))
            {
                if (!strcmp(modName, dllFullPath)) return true;
            }
        }
    }

    printf("[DLL INJECT] DLL could not be found in the target process. Load failed.\n");
    return false;
}

int main(int argc, char* argv[])
{
    try {
        const auto settings = toml::parse("LauncherSettings.toml");
        const auto game_relative_path = toml::find<std::string>(settings, "launcher", "game_exe_relative_path");

        // Compute game exe path and make sure it is correct

        path game_exe_path;
        int target_appid = toml::find<int>(settings, "launcher", "target_appid");
        if (target_appid == 0) {
            game_exe_path = game_relative_path;
        }
        else if (GetSteamGameInstallDir(target_appid, game_exe_path)) {
            game_exe_path = game_exe_path / game_relative_path;
        }
        else {
            system("pause");
            return 1;
        };
        if (!(exists(game_exe_path) && is_regular_file(game_exe_path))) {
            printf("Game executable path (%s) does not point to an existing file\n", game_exe_path.string().c_str());
            system("pause");
            return 1;
        }

        // Set SteamAppId env var for the Steam API

        if (!SetEnvironmentVariableA("SteamAppId", std::to_string(target_appid).c_str())) {
            printf("SetEnvironmentVariable failed! Error = %lu\n", GetLastError());
            system("pause");
            return 1;
        }

        // Create game process as suspended

        printf("Creating game process in suspended state...\n");
        STARTUPINFOW si;
        ZeroMemory(&si, sizeof(si));
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(game_exe_path.c_str(), nullptr, nullptr, nullptr, FALSE, DETACHED_PROCESS | CREATE_SUSPENDED, nullptr, game_exe_path.parent_path().c_str(), &si, &pi)) {
            printf("Game process creation failed! Game Path = %s, Error = %lu\n", game_exe_path.string().c_str(), GetLastError());
            system("pause");
            return 1;
        }

        // Inject MCF library
        if (toml::find_or<bool>(settings, "launcher", "pause_before_injection", false)) system("pause");
        if (!injectDll(pi.hProcess, "MCF.dll")) {
            printf("DLL injection failed! Killing game process.\n");
            TerminateProcess(pi.hProcess, 0);
            system("pause");
            return 1;
        }

        printf("Injection successful, resuming game process...\n");
        ResumeThread(pi.hThread);

        // Inject external mods

        int initial_delay = toml::find<int>(settings, "external_mods", "initial_delay");
        int delay_between_dlls = toml::find<int>(settings, "external_mods", "delay_between_dlls");
        std::vector<std::string> dlls = toml::find<std::vector<std::string>>(settings, "external_mods", "paths");

        Sleep(initial_delay);

        for (const auto& dll : dlls) {
            if (!injectDll(pi.hProcess, dll.c_str())) {
                printf("Injection of DLL %s failed\n", dll.c_str());
            }
            Sleep(delay_between_dlls);
        }
    }
    catch (std::exception& e) {
        printf("Error parsing launcher settings:\n%s\n", e.what());
        system("pause");
        return 1;
    }

    return 0;
}