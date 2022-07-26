#pragma once
#include "../Include/EventMan.h"
#include "../Include/Export.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>

namespace MCF
{
	class EventManImp final : public SharedInterfaceImp<EventMan, EventManImp>
	{
	private:
		std::unordered_map<std::string, std::unordered_set<EventCallbackBase*>> callbacks;
		std::unordered_map<HCallResult, CallResultBase*> cr_from_handle;
		std::unordered_map<CallResultBase*, std::unordered_set<HCallResult>> handle_from_cr;

		std::recursive_mutex cb_mutex;
		std::recursive_mutex cr_mutex;
		HCallResult handle_ctr = 0;

	public:
		virtual bool IsUnloadable() const override { return true; }

		virtual void RegisterCallback(EventCallbackBase* callback) override;

		virtual void UnregisterCallback(EventCallbackBase* callback) override;

		virtual void RaiseEvent(const char* event_name, void* event_data) override;

		virtual HCallResult BindCallResult(CallResultBase* call_result) override;

		virtual void UnbindCallResult(HCallResult handle) override;

		virtual void UnregisterCallResult(CallResultBase* call_result) override;

		virtual bool RaiseCallResult(HCallResult handle, void* result) override;
	};
}
