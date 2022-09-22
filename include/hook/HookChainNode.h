
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <type_traits>
#include "HookMan.h"
#include "StaticThunk.h"

namespace MCF
{
    /// \tparam UniqueType Type which must be unique on template instantiation, to ensure uniqueness of static thunks
    /// \tparam TSig Signature of the function to hook, as a function pointer.
    /// \tparam HookMan The hook manager class to use.
    /// \tparam HookParams The parameters required by the hook manager to register a hook.
    template<class UniqueType, class TSig, class HookMan, class... HookParams>
            requires std::is_pointer_v<TSig> && std::is_function_v<std::remove_pointer_t<TSig>>
    class GenericHook
    {
    public:
        using FSig = std::remove_pointer_t<TSig>;
    private:
        static std::function<FSig> shook;
        static HookNode<TSig> node;

        // Internal functors for providing the CallOriginal / CallNext functions

        template<class S>
        struct CallOriginalFunctor;
        template<class Ret, class... Args>
        struct CallOriginalFunctor<Ret(Args...)>
        {
            Ret operator()(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) node.orig_function(args...);
                else return node.orig_function(args...);
            }
        };
        template<class S>
        struct CallNextFunctor;
        template<class Ret, class... Args>
        struct CallNextFunctor<Ret(Args...)>
        {
            Ret operator()(Args... args)
            {
                if (node.next && node.next->thunk) {
                    if constexpr (std::is_same_v<Ret, void>) node.next->thunk(args...);
                    else return node.next->thunk(args...);
                }
                else {
                    if constexpr (std::is_same_v<Ret, void>) node.orig_function(args...);
                    else return node.orig_function(args...);
                }
            }
        };

        /// Generates the static thunk function based on the target signature
        template<class S>
        struct ThunkGen;
        template<class Ret, class... Args>
        class ThunkGen<Ret (__fastcall*)(Args...)>
        {
            static Ret __fastcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) shook(args...);
                else return shook(args...);
            }
        public:
            using ThunkType = Ret(__fastcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
#ifndef _WIN64
        template<class Ret, class... Args>
        class ThunkGen<Ret (__cdecl*)(Args...)>
        {
            static Ret __cdecl Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) shook(args...);
                else return shook(args...);
            }
        public:
            using ThunkType = Ret(__cdecl*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
        template<class Ret, class... Args>
        class ThunkGen<Ret (__stdcall*)(Args...)>
        {
            static Ret __stdcall Thunk(Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) shook(args...);
                else return shook(args...);
            }
        public:
            using ThunkType = Ret(__stdcall*)(Args...);
            static constexpr ThunkType GetThunk() { return &Thunk; }
        };
        template<class Ret, class PObj, class... Args>
        class ThunkGen<Ret (__thiscall*)(PObj, Args...)>
        {
            Ret __thiscall Thunk(PObj obj, Args... args)
            {
                if constexpr (std::is_same_v<Ret, void>) shook((PObj)this, args...);
                else return shook((PObj)this, args...);
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
#endif

        Dependency<HookMan> hook_man;

    public:
        CallNextFunctor<FSig> CallNext;
        CallOriginalFunctor<FSig> CallOriginal;
    };
    template<class UniqueType, class TSig, class HookMan, class... HookParams>
        requires std::is_pointer_v<TSig> && std::is_function_v<std::remove_pointer_t<TSig>>
    std::function<typename GenericHook<UniqueType, TSig, HookMan, HookParams...>::FSig>
            GenericHook<UniqueType, TSig, HookMan, HookParams...>::shook;
    template<class UniqueType, class TSig, class HookMan, class... HookParams>
        requires std::is_pointer_v<TSig> && std::is_function_v<std::remove_pointer_t<TSig>>
    HookNode<TSig> GenericHook<UniqueType, TSig, HookMan, HookParams...>::node;


//    struct CallableStaticWrapper
//    {
//        ~CallableStaticWrapper() = default;
//        virtual void* GetWrapper() const = 0;
//    };
//
//    template<class Ret, class... Args>
//    struct HookChainNodeBase : CallableStaticWrapper
//    {
//        HookChainNodeBase() = default;
//        virtual Ret Call(Args... args) = 0;
//        virtual Ret CallNextHook(Args... args) = 0;
//        virtual Ret CallOriginal(Args... args) = 0;
//    };
//
//    template<class C, class S> requires std::is_function_v<S>
//    struct HookChainNode;
//
//    template<class C, class Ret, class... Args> requires std::is_invocable_r_v<Ret, C, Args...>
//    struct HookChainNode<C, Ret>
//    {
//        virtual Ret Call(Args... args)
//        {
//            if constexpr (std::is_same_v<Ret, void>)
//                hook(args...);
//            else return hook(args...);
//        }
//
//        virtual Ret CallNextHook(Args... args)
//        {
//            if (pNext) {
//                if constexpr (std::is_same_v<Ret, void>) {
//                    pNext->Call(args...);
//                }
//                else return pNext->Call(args...);
//            }
//                // Last hook in the chain
//            else {
//                if constexpr (std::is_same_v<Ret, void>) {
//                    CallOriginal(args...);
//                }
//                else return CallOriginal(args...);
//            }
//        }
//
//        virtual Ret CallOriginal(Args... args) = 0;
//
//        virtual void* GetStaticWrapper() const = 0;
//
//        void* orig;
//        Hook hook;
//        HookChainNodeBase* pNext;
//    };
//
//    template<class Hook, class FunSignature>
//    struct HookChainNode
//    {
//
//    };
//
//#ifndef _WIN64
//    template<class Hook, class Ret, class... Args>
//	struct HookChainNode<Hook, Ret __cdecl(Args...)> : HookChainNodeBase<Hook, Ret, Args...>
//	{
//		static constexpr const CallingConvention CallConv = CallingConvention::Cdecl;
//
//		virtual void* GetStaticWrapper() const override
//		{
//			return CecdlWrapper;
//		}
//
//	private:
//
//		static Ret __cdecl CecdlWrapper(Args... args)
//		{
//			if constexpr (std::is_same_v<Ret, void>)
//				first->hook(args...);
//			else return first->hook(args...);
//		}
//	};
//
//	template<class Hook, class Ret, class... Args>
//	struct HookChainNode<Hook, Ret __stdcall(Args...)> : HookChainNodeBase<Hook, Ret, Args...>
//	{
//		static constexpr const CallingConvention CallConv = CallingConvention::Stdcall;
//
//		virtual void* GetStaticWrapper() const override
//		{
//			return StdcallWrapper;
//		}
//
//	private:
//		static Ret __stdcall StdcallWrapper(Args... args)
//		{
//			if constexpr (std::is_same_v<Ret, void>)
//				first->hook(args...);
//			else return first->hook(args...);
//		}
//	};
//
//	template<class Hook, class Ret, class PThis, class... Args>
//	struct HookChainNode<Hook, Ret __thiscall(PThis, Args...)> : HookChainNodeBase<Hook, Ret, PThis, Args...>
//	{
//		static constexpr const CallingConvention CallConv = CallingConvention::Thiscall;
//
//		virtual void* GetStaticWrapper() const override
//		{
//			return (void***)this[0][4];
//		}
//
//	private:
//		virtual Ret ThiscallWrapper(Args... args)
//		{
//			if constexpr (std::is_same_v<Ret, void>)
//				first->hook((PThis)this, args...);
//			else return first->hook((PThis)this, args...);
//		}
//	};
//#endif
//
//
//
//    template<class Hook, class Ret, class... Args>
//    struct HookChainNode<Hook, Ret __fastcall(Args...)> : HookChainNodeBase<Hook, Ret, Args...>
//    {
//        static constexpr const CallingConvention CallConv = CallingConvention::Fastcall;
//
//        void* GetStaticWrapper() const override
//        {
//            return FastcallWrapper;
//        }
//
//        Ret CallOriginal(Args... args) override
//        {
//            auto o = (Ret (__fastcall*)(Args...))this->orig;
//            if constexpr(std::is_same_v<Ret, void>) o(args...);
//            else return o(args...);
//        }
//
//    private:
//        // By including the wrapper at the end, and having the calling convention and number of arguments,
//        // we can easily generate a thunk that pushes the address of the hook chain node to the stack or moves it
//        // into a register (depending on the calling convention)
//        static Ret __fastcall FastcallWrapper(Args... args, HookChainNode* first)
//        {
//            if constexpr (std::is_same_v<Ret, void>)
//                first->hook(args...);
//            else return first->hook(args...);
//        }
//    };

//    template<class Ret, class... Args>
//    struct StaticWrapperBase
//    {
//        using FunPtrType = Ret (*)(Args...);
//
//        StaticWrapperBase() = default;
//        virtual ~StaticWrapperBase() { }
//        virtual FunPtrType GetWrapper() const = 0;
//    };
//
//    template<class C, class S> requires std::is_function_v<S>
//    struct StaticThunk;
//
//    template<class C, class Ret, class... Args>
//    struct StaticThunk<C, Ret __fastcall(Args...)> : StaticWrapperBase<Ret, Args...>
//    {
//        using FunPtrType = Ret (*)(Args...);
//
//        explicit StaticThunk(const C& c)
//        {
//            if (!has_instance.exchange(true, std::memory_order_acquire))
//                callable = c;
//        }
//
//        explicit StaticThunk(C&& c)
//        {
//            if (!has_instance.exchange(true, std::memory_order_acquire))
//                callable = std::move(c);
//        }
//
//        ~StaticThunk() override
//        {
//            has_instance.store(false, std::memory_order_release);
//        }
//
//        FunPtrType GetWrapper() const override
//        {
//            return Wrapper;
//        }
//
//    private:
//        static C callable;
//        static std::atomic_bool has_instance;
//
//        Ret __fastcall Wrapper(Args... args)
//        {
//            if constexpr(std::is_same_v<Ret, void>) callable(args...);
//            else return callable(args...);
//        }
//    };


//
//    template <auto Func>
//    class StaticThunk
//    {
//    private:
//        static void* instance;
//
//        template<typename TFunc>
//        struct GenFun;
//
//        template <typename TObj, typename TRet, typename... TArgs>
//        struct GenFun<TRet(TObj::*)(TArgs...)>
//        {
//            static TRet fun(TArgs... args)
//            {
//                return ((TObj*)instance->*Func)(args...);
//            }
//
//            using TFunction = TRet(*)(TArgs...);
//        };
//
//        template <typename TRet, typename... TArgs>
//        struct GenFun<TRet(*)(TArgs...)>
//        {
//            static TRet fun(TArgs... args)
//            {
//                return Func(args...);
//            }
//
//            using TFunction = TRet(*)(TArgs...);
//        };
//        using TFunction = typename GenFun<decltype(Func)>::TFunction;
//
//    public:
//        static TFunction fun;
//
//        StaticThunk(void* instance) { this->instance = instance; };
//    };
//    template<auto Func>
//    typename StaticThunk<Func>::TFunction StaticThunk<Func>::fun = &StaticThunk::GenFun<decltype(Func)>::fun;
}