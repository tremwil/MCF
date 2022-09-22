
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/SharedInterface.h"

namespace MCF
{
    struct GenericHookNode
    {
        GenericHookNode* prev = nullptr;
        GenericHookNode* next = nullptr;
        void* thunk = nullptr;
        void* orig_function = nullptr;
    };

    template<class S>
    struct HookNode;
    template<class Ret, class... Args>
    struct HookNode<Ret(__fastcall *)(Args...)>
    {
        HookNode* prev = nullptr;
        HookNode* next = nullptr;
        Ret (__fastcall *thunk)(Args...) = nullptr;
        Ret (__fastcall *orig_function)(Args...) = nullptr;
    };
#ifndef _WIN64
    template<class Ret, class... Args>
    struct HookNode<Ret(__cdecl *)(Args...)>
    {
        HookNode* prev = nullptr;
        HookNode* next = nullptr;
        Ret (__cdecl *thunk)(Args...) = nullptr;
        Ret (__cdecl *orig_function)(Args...) = nullptr;
    };
    template<class Ret, class... Args>
    struct HookNode<Ret(__stdcall *)(Args...)>
    {
        HookNode* prev = nullptr;
        HookNode* next = nullptr;
        Ret (__stdcall *thunk)(Args...) = nullptr;
        Ret (__stdcall *orig_function)(Args...) = nullptr;
    };
    template<class Ret, class... Args>
    struct HookNode<Ret(__thiscall *)(Args...)>
    {
        HookNode* next = nullptr;
        Ret (__thiscall *thunk)(Args...) = nullptr;
        Ret (__thiscall *orig_function)(Args...) = nullptr;
    };
#endif

    template<class M, FixedString VersionString, class... HookParams>
    class GenericHookMan : public SharedInterface<M, VersionString>
    {
    public:
        virtual bool SetHook(HookParams... params, GenericHookNode* hook) = 0;
        virtual bool RemoveHook(GenericHookNode* hook) = 0;
        virtual void* GetOriginal(HookParams... params) = 0;
    };
}