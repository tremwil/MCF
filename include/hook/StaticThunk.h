
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/Logger.h"
#include "util/TemplateUtils.h"
#include <atomic>

namespace MCF
{
    /// Provides a compile-time generated static thunk function to call a member function.
    /// Particularly useful when trying to write hooks as singleton methods.
    /// NOTE: Does not support functions with specifiers like const, noexcept, volatile.
    template<auto Func, CallConv DesiredCConv = CallConv::Auto> requires
    std::is_member_function_pointer_v<decltype(Func)> || IsFunctionPointer_V<decltype(Func)>
    class StaticThunk : NonAssignable
    {
        using ObjType = typename PMTraits<decltype(Func)>::class_type;

        static std::atomic<ObjType*> instance; // Store instance for member function ptr, not needed for static fun.
        static std::atomic_int32_t ref_count; // Store number of references to instance for wrapper (may happen when

        template<class F, CallConv CC>
        struct ThunkCC {
            static constexpr CallConv value = CC;
        };
        template<class F>
        struct ThunkCC<F, CallConv::Auto> {
            static constexpr CallConv value = CCTraits<F>::call_conv;
        };

    public:
        static constexpr CallConv ThunkCConv =
                ThunkCC<decltype(Func), DesiredCConv>::value;
    private:
        template<class Ret, class... Args>
        static inline Ret SafePMFCall(Args... args)
        {
            ObjType* ins = instance.load(std::memory_order_relaxed);
            if constexpr (std::is_same_v<Ret, void>) {
                if (ins) (ins->*Func)(args...);
            }
            else return ins ? (ins->*Func)(args...) : Ret();
        }

        template<class S, CallConv C>
        class ThunkGen;

        template<class Ret, class... Args>
        class ThunkGen<Ret(*)(Args...), CallConv::Fastcall>
        {
            static Ret __fastcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) Func(args...);
                else return Func(args...);
            }
        public:
            using ThunkType = Ret(__fastcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
        template<class Ret, class Obj, class... Args>
        class ThunkGen<Ret(Obj::*)(Args...), CallConv::Fastcall>
        {
            static Ret __fastcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) SafePMFCall(args...);
                else return SafePMFCall(args...);
            }
        public:
            using ThunkType = Ret(__fastcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };

#ifndef _WIN64
        template<class Ret, class... Args>
        class ThunkGen<Ret(Args...), CallConv::Stdcall>
        {
            static Ret __stdcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) Func(args...);
                else return Func(args...);
            }
        public:
            using ThunkType = Ret(__stdcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
        template<class Ret, class Obj, class... Args>
        class ThunkGen<Ret(Obj::*)(Args...), CallConv::Stdcall>
        {
            static Ret __stdcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) SafePMFCall(args...);
                else return SafePMFCall(args...);
            }
        public:
            using ThunkType = Ret(__stdcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };

        template<class Ret, class... Args>
        class ThunkGen<Ret(Args...), CallConv::Cdecl>
        {
            static Ret __cdecl Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) Func(args...);
                else return Func(args...);
            }
        public:
            using ThunkType = Ret(__cdecl*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
        template<class Ret, class Obj, class... Args>
        class ThunkGen<Ret(Obj::*)(Args...), CallConv::Cdecl>
        {
            static Ret __cdecl Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) SafePMFCall(args...);
                else return SafePMFCall(args...);
            }
        public:
            using ThunkType = Ret(__cdecl*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };

        template<class Ret, class PObj, class... Args>
        class ThunkGen<Ret(PObj, Args...), CallConv::Thiscall>
        {
            Ret __thiscall Thunk(PObj obj, Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) Func((PObj)this, args...);
                else return Func((PObj)this, args...);
            }
        public:
            using ThunkType = Ret(__thiscall*)(PObj, Args...);
            static ThunkType GetThunk()
            {
                // Couldn't find a better way to get a static function pointer from Thunk
                union {
                    Ret (ThunkGen::*member_fptr)(PObj, Args...) = &Thunk;
                    ThunkType static_fptr;
                };
                return static_fptr;
            }
        };
        template<class Ret, class Obj, class PObj, class... Args>
        class ThunkGen<Ret(Obj::*)(PObj, Args...), CallConv::Thiscall>
        {
            Ret __thiscall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) SafePMFCall((PObj)this, args...);
                else return SafePMFCall((PObj)this, args...);
            }
        public:
            using ThunkType = Ret(__thiscall*)(PObj, Args...);
            static ThunkType GetThunk()
            {
                union {
                    Ret (ThunkGen::*member_fptr)(PObj, Args...) = &Thunk;
                    ThunkType static_fptr;
                };
                return static_fptr;
            }
        };
#endif
    public:
        StaticThunk() = delete;

        using T = ThunkGen<decltype(Func), ThunkCConv>;

        static bool SetInstance(ObjType* ins)
        {
            if constexpr (!std::is_same_v<ObjType, void>)
            {
                if (ins == nullptr && !instance.exchange(nullptr, std::memory_order_release))
                {
                    Dependency<Logger> logger;
                    if (logger) logger->Warn(typeid(StaticThunk).name(), "Tried to unset static wrapper instance twice, ignoring");
                }
                else
                {
                    ObjType* expected = nullptr;
                    if (!instance.compare_exchange_strong(&expected, ins))
                    {
                        Dependency<Logger> logger;
                        if (logger) logger->Warn(typeid(StaticThunk).name(), "Tried to set static wrapper instance twice, ignoring");
                    }
                }
            }
        }
    };
}

