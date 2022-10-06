/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.

 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "include/windows_include.h"
#include <winternl.h>
#include "src/core/Bootstrap.h"
#include "src/steamstub/SteamStubHeader.hpp"
#include "src/memory/VirtualAllocNear.hpp"

typedef NTSTATUS (NTAPI *pNtGetNextThread)(
         HANDLE ProcessHandle,
         HANDLE ThreadHandle,
         ACCESS_MASK DesiredAccess,
         ULONG HandleAttributes,
         ULONG Flags,
         PHANDLE NewThreadHandle
);

typedef NTSTATUS (NTAPI *pNtQueryInformationThread)(
        IN HANDLE ThreadHandle,
        IN DWORD ThreadInformationClass,
        OUT PVOID ThreadInformation,
        IN ULONG ThreadInformationLength,
        OUT PULONG ReturnLength OPTIONAL
);

#define ThreadQuerySetWin32StartAddress 9

BOOL hijackSuspendedMainThread(void* hook, uintptr_t (**originalEntryPoint)())
{
    HMODULE exeBase = GetModuleHandle(nullptr);
    auto* dos = (IMAGE_DOS_HEADER*)exeBase;
    auto* nt = (IMAGE_NT_HEADERS64*)((uintptr_t)exeBase + dos->e_lfanew);
    uintptr_t procEntryPoint = (uintptr_t)exeBase + nt->OptionalHeader.AddressOfEntryPoint;

    if (originalEntryPoint)
        *originalEntryPoint = (uintptr_t(*)())procEntryPoint;

    auto ntdll = GetModuleHandleA("ntdll.dll");
    auto NtGetNextThread = (pNtGetNextThread)GetProcAddress(ntdll, "NtGetNextThread");
    auto NtQueryInformationThread = (pNtQueryInformationThread)GetProcAddress(ntdll, "NtQueryInformationThread");

    HANDLE hProc = GetCurrentProcess(), hThread = nullptr;
    while (NT_SUCCESS(NtGetNextThread(hProc, hThread, THREAD_ALL_ACCESS, 0, 0, &hThread))) {
        uintptr_t threadEntryPoint = 0;
        if (NT_SUCCESS(NtQueryInformationThread(hThread, ThreadQuerySetWin32StartAddress, &threadEntryPoint, sizeof(threadEntryPoint), nullptr)) &&
            threadEntryPoint == procEntryPoint) {

            CONTEXT ctx{};
            ctx.ContextFlags = CONTEXT_FULL;

            if (!GetThreadContext(hThread, &ctx))
                return FALSE;

#ifdef _X86_
            uintptr_t ins_ptr = ctx.Eip;
#else
            uintptr_t ins_ptr = ctx.Rip;
#endif
            // Make sure the process was created suspended (thread still on RtlUserThreadStart)
            if (ins_ptr != (DWORD64)GetProcAddress(ntdll, "RtlUserThreadStart"))
                return FALSE;

#ifdef _X86_
            // __stdcall convention
            *(uintptr_t*)(ctx.Esp + 4) = (uintptr_t)hook;
#else
            ctx.Rcx = (uintptr_t)hook;
#endif
            return SetThreadContext(hThread, &ctx);
        }
    }
    return FALSE;
}

static uintptr_t(*originalEntryPoint)();

//static uint8_t* steamStubEntryPointThunk;
//static void* steamStubThunkMem;
//
//uintptr_t __stdcall steamStubEntryPoint()
//{
//    MCF::Bootstrap::Get().Init(false);
//    return originalEntryPoint();
//}

uintptr_t __stdcall myEntryPoint()
{
//    // TODO: Proper "run after Steam sub" functionality
//    // Let SteamSub 3.1 run before our bootstrap code
//
//    auto header = (MCF::SteamStubHeader31*)((intptr_t)originalEntryPoint - 0xF0);
//    auto sig = header->xor_key ^ header->signature;
//
//    // Steam Stub header detected
//    if (sig == 0xC0DEC0DF) {
//        auto steamEntry = originalEntryPoint;
//        // Patch header in place
//        DWORD oldProtect;
//        VirtualProtectEx(GetCurrentProcess(), header, 0xF0, PAGE_EXECUTE_READWRITE, &oldProtect);
//
//        MCF::SteamXorDecrypt(header, header, 0xF0);
//
//        originalEntryPoint = (uintptr_t(*)())((intptr_t)MCF::MainModule().begin + header->original_entry_point);
//        // Steam detects this :(
//        //header->original_entry_point = (intptr_t)steamStubEntryPoint - (intptr_t)MCF::MainModule().begin;
//        MCF::SteamXorEncrypt(header, header, 0xF0);
//
//        VirtualProtectEx(GetCurrentProcess(), header, 0xF0, oldProtect, &oldProtect);
//
//        return steamEntry();
//    }

    MCF::Bootstrap::Get().Init(true);
    return originalEntryPoint();
}

DWORD __stdcall threadEntryPoint(LPVOID lParam)
{
    MCF::Bootstrap::Get().Init(false);
    return 0;
}

BOOL __stdcall DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        if (hijackSuspendedMainThread((void*)myEntryPoint, &originalEntryPoint)) {
            return TRUE;
        }
        else {
            return CreateThread(nullptr, 0, threadEntryPoint, nullptr, 0, nullptr) != INVALID_HANDLE_VALUE;
        }
    }
    return TRUE;
}