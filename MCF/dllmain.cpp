// dllmain.cpp : Defines the entry point for the DLL application.
#include "common.h"
#include <iostream>
//#include "ThirdParty/Kiero/kiero.h"
//#include "imgui.h"
//#include "backends/imgui_impl_dx12.h"
#include "Implementation/ComponentManImp.h"

DWORD testThread(LPVOID lParam)
{
    //AllocConsole();
    //SetConsoleTitleW(L"PartyMemberInfo layout VEH based patcher");
    //FILE* fDummy;
    //freopen_s(&fDummy, "CONOUT$", "w", stdout);
    //freopen_s(&fDummy, "CONOUT$", "w", stderr);
    //freopen_s(&fDummy, "CONIN$", "r", stdin);
    //std::cout.clear();
    //std::clog.clear();
    //std::cerr.clear();
    //std::cin.clear();

    //if (kiero::init(kiero::RenderType::Auto) != kiero::Status::Success)
    //    return;

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        printf("Test thread start\n");
        CreateThread(NULL, 0, testThread, hModule, 0, NULL);
    }
    return TRUE;
}

