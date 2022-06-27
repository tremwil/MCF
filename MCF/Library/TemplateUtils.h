#pragma once
#include <stdint.h>
#include <type_traits>

namespace MCF
{
	/// <summary>
	/// Generates a static function from any function pointer (static or member function of singleton class)  
	/// </summary>
	template <auto Func>
	class StaticWrapper
	{
	private:
		static void* instance;

		template<typename TFunc>
		struct GenFun;

		template <typename TObj, typename TRet, typename... TArgs>
		struct GenFun<TRet(TObj::*)(TArgs...)>
		{
			static TRet fun(TArgs... args)
			{
				return ((TObj*)instance->*Func)(args...);
			}

			using TFunction = TRet(*)(TArgs...);
		};

		template <typename TRet, typename... TArgs>
		struct GenFun<TRet(*)(TArgs...)>
		{
			static TRet fun(TArgs... args)
			{
				return Func(args...);
			}

			using TFunction = TRet(*)(TArgs...);
		};
		using TFunction = GenFun<decltype(Func)>::TFunction;

	public:
		static TFunction fun;

		StaticWrapper(void* instance) { this->instance = instance; };
	};
	template<auto Func>
	StaticWrapper<Func>::TFunction StaticWrapper<Func>::fun = &StaticWrapper::GenFun<decltype(Func)>::fun;

	template<unsigned N>
	struct FixedString
	{
		char buf[N + 1]{};
		constexpr FixedString(char const* s)
		{
			for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
		}
		constexpr operator char const* () const { return buf; }
	};
	template<unsigned N> FixedString(char const (&)[N])->FixedString<N - 1>;
}