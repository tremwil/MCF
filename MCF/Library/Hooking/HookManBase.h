#pragma once
#include "../SharedInterface.h"
#include "../TemplateUtils.h"

namespace MCF
{
	template <typename T, FixedString version_str, typename... THookParams>
	class HookManBase : public SharedInterface<T, version_str>
	{
	public:
		virtual bool Hook(uintptr_t address, THookParams... params, void* hook_fun) = 0;

		virtual bool Unhook(void* hook_fun) = 0;

		virtual void* GetNextHookAddr(void* current_hook) = 0;
	};
}