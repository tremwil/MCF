#include "EventManImp.h"

namespace MCF
{

	void EventManImp::RegisterCallback(EventCallbackBase* callback)
	{
		std::lock_guard<decltype(cb_mutex)> lock(cb_mutex);
		callbacks[callback->EventName()].insert(callback);
	}

	void EventManImp::UnregisterCallback(EventCallbackBase* callback)
	{
		std::lock_guard<decltype(cb_mutex)> lock(cb_mutex);
		callbacks[callback->EventName()].erase(callback);
	}

	void EventManImp::RaiseEvent(const char* event_name, void* event_data)
	{
		std::lock_guard<decltype(cb_mutex)> lock(cb_mutex);
		if (callbacks.count(event_name) == 0) return;

		for (const auto cb : callbacks[event_name])
			cb->Run(event_data);
	}

	HCallResult EventManImp::BindCallResult(CallResultBase* call_result)
	{
		std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

		cr_from_handle[++handle_ctr] = call_result;
		handle_from_cr[call_result].insert(handle_ctr);
		return handle_ctr;
	}

	void EventManImp::UnbindCallResult(HCallResult handle)
	{
		std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

		if (cr_from_handle.count(handle) == 0) return;

		CallResultBase* cr = cr_from_handle[handle];
		handle_from_cr[cr].erase(handle);
		if (handle_from_cr[cr].empty())
			handle_from_cr.erase(cr);

		cr_from_handle.erase(handle);
	}

	void EventManImp::UnregisterCallResult(CallResultBase* call_result)
	{
		std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

		if (handle_from_cr.count(call_result) == 0) return;

		for (const auto handle : handle_from_cr[call_result])
			cr_from_handle.erase(handle);

		handle_from_cr.erase(call_result);
	}

	bool EventManImp::RaiseCallResult(HCallResult handle, void* result)
	{
		std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

		if (cr_from_handle.count(handle) == 0) return false;
		cr_from_handle[handle]->Run(result);
		UnbindCallResult(handle);
		return true;
	}
}
