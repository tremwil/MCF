
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <type_traits>

namespace MCF
{
    template<class T>
    struct AlwaysFalse
    {
        static constexpr bool value = false;
    };

    /// Object which cannot be copied. Can be inherited to prevent implicit copy ctor / assignment generation.
    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };

    /// Object which cannot be assigned. Can be inherited to prevent implicit copy/move ctor / assignment generation.
    struct NonAssignable
    {
        NonAssignable() = default;
        NonAssignable(const NonAssignable&) = delete;
        NonAssignable(const NonAssignable&&) = delete;
        NonAssignable& operator=(const NonAssignable&) = delete;
    };

    template<class T>
    struct IsFunctionPointer
    {
        static constexpr bool value = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;
    };
    template<class T>
    constexpr bool IsFunctionPointer_V = IsFunctionPointer<T>::value;

    template<class T>
    struct PMTraits
    {
        using class_type = void;
        using member_type = std::remove_pointer_t<T>;
    };

    template<class T, class U>
    struct PMTraits<U T::*>
    {
        using class_type = T;
        using member_type = U;
    };

    /// Windows calling conventions
    enum class CallConv : uint32_t
    {   /// Automatically determine calling convention of wrapper from calling function.
        Auto,
        Fastcall,
#ifndef _WIN64
        Stdcall,
        Thiscall,
        Cdecl,
#else
        Stdcall = Fastcall,
        Thiscall = Fastcall,
        Cdecl = Fastcall
#endif
    };

    /// Get the microsoft calling convention given a function or member function pointer.
    /// NOTE: Does not support functions with specifiers like const, noexcept, volatile
    template<class F> requires std::is_member_function_pointer_v<F> || IsFunctionPointer_V<F>
    struct CCTraits;

    template<class Ret, class... Args>
    struct CCTraits<Ret(__fastcall*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Fastcall;
        using base_type = Ret(Args...);
    };
    template<class Ret, class Obj, class... Args>
    struct CCTraits<Ret(__fastcall Obj::*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Fastcall;
        using base_type = Ret(Args...);
    };

#ifndef _WIN64
    template<class Ret, class... Args>
    struct CCTraits<Ret(__cdecl*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Cdecl;
        using base_type = Ret(Args...);
    };
    template<class Ret, class Obj, class... Args>
    struct CCTraits<Ret(__cdecl Obj::*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Fastcall;
        using base_type = Ret(Args...);
    };

    template<class Ret, class... Args>
    struct CCTraits<Ret(__stdcall*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Stdcall;
        using base_type = Ret(Args...);
    };
    template<class Ret, class Obj, class... Args>
    struct CCTraits<Ret(__stdcall Obj::*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Fastcall;
        using base_type = Ret(Args...);
    };

    template<class Ret, class... Args>
    struct CCTraits<Ret(__thiscall*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Thiscall;
        using base_type = Ret(Args...);
    };
    template<class Ret, class Obj, class... Args>
    struct CCTraits<Ret(__thiscall Obj::*)(Args...)>
    {
        static constexpr CallConv call_conv = CallConv::Fastcall;
        using base_type = Ret(Args...);
    };
#endif
}