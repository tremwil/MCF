// dllmain.cpp : Defines the entry point for the DLL application.
#include "common.h"
#include <iostream>
//#include "ThirdParty/Kiero/kiero.h"
//#include "imgui.h"
//#include "backends/imgui_impl_dx12.h"
#include "Implementation/ComponentManImp.h"
#include <Zydis/Zydis.h>

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

    ZyanU8 data[] =
    {
        0x51, 0x8D, 0x45, 0xFF, 0x50, 0xFF, 0x75, 0x0C, 0xFF, 0x75,
        0x08, 0xFF, 0x15, 0xA0, 0xA5, 0x48, 0x76, 0x85, 0xC0, 0x0F,
        0x88, 0xFC, 0xDA, 0x02, 0x00
    };

    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);

    ZydisDecodedInstruction instr;
    ZydisDecoderDecodeBuffer(& decoder, data, sizeof(data), & instr);

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

