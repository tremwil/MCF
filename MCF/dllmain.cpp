// dllmain.cpp : Defines the entry point for the DLL application.
#include "common.h"
#include <iostream>
//#include "ThirdParty/Kiero/kiero.h"
//#include "imgui.h"
//#include "backends/imgui_impl_dx12.h"
#include "Implementation/SharedInterfaceManImp.h"

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

    //MCF::Test test1;
    //MCF::Test test2;

    //int res_1 = test1.t1.Call<int>(1, 2);
    //int res_2 = test1.t2.Call<int>(1, 2);
    //int res_3 = test2.t1.Call<int>(2, 4);
    //int res_4 = test2.t2.Call<int>(2, 4);
    //printf("%d %d %d %d\n", res_1, res_2, res_3, res_4);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        //auto f = MCF::SharedInterfaceManImp::force_init;
        //MCF::AutoExport<MCF::SharedInterfaceManImp>::ForceInit f;

        printf("Test thread start\n");
        //CreateThread(NULL, 0, testThread, hModule, 0, NULL);
    }
    return TRUE;
}

