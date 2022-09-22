/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.

 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "include/windows_include.h"
#include <winternl.h>
#include "src/core/Bootstrap.h"

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
uintptr_t __stdcall myEntryPoint()
{
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