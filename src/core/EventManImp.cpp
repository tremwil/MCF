
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "EventManImp.h"
#include "core/Export.h"

namespace MCF
{
    EventManImp::EventManImp(const bool *success)
    {
        if (!*success) return;
        deferred_event_thread = std::thread([this]() {
            std::tuple<std::string, HCallResult, EventData*> evt;
            while (true) {
                deferred_events.wait_dequeue(evt);

                // Use HCallResult value -1 to break the loop on destruction
                if (std::get<1>(evt) == -1) break;

                // First case: No call result handle set -> Event callback
                else if (std::get<1>(evt) == 0)
                    RunCallbacks(std::get<0>(evt), std::get<2>(evt));

                // Second case -> Call Result
                else RunCallResult(std::get<1>(evt), std::get<2>(evt));
            }
        });
    }

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
        UnbindCallResultInternal(handle);
    }

    void EventManImp::UnregisterCallResult(CallResultBase* call_result)
    {
        std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

        if (handle_from_cr.count(call_result) == 0) return;

        for (const auto handle : handle_from_cr[call_result])
            cr_from_handle.erase(handle);

        handle_from_cr.erase(call_result);
    }

    void EventManImp::UnbindCallResultInternal(HCallResult handle)
    {
        if (cr_from_handle.count(handle) == 0) return;

        CallResultBase* cr = cr_from_handle[handle];
        handle_from_cr[cr].erase(handle);
        if (handle_from_cr[cr].empty())
            handle_from_cr.erase(cr);

        cr_from_handle.erase(handle);
    }

    void EventManImp::RaiseEvent(const char* event_name, EventData* data, bool deferred)
    {
        if (deferred) {
            deferred_events.enqueue(std::make_tuple(event_name, 0, data));
        }
        else RunCallbacks(event_name, data);
    }

    bool EventManImp::RaiseCallResult(HCallResult handle, EventData* result, bool deferred)
    {
        if (deferred) {
            deferred_events.enqueue(std::make_tuple("", handle, result));
            return true;
        }
        else return RunCallResult(handle, result);
    }

    void EventManImp::RunCallbacks(const std::string &event_name, EventData* data)
    {
        std::lock_guard<decltype(cb_mutex)> lock(cb_mutex);
        if (callbacks.count(event_name) == 0) return;

        for (const auto cb : callbacks[event_name])
            cb->Run(data);

        data->Free();
    }

    bool EventManImp::RunCallResult(HCallResult handle, EventData* result)
    {
        std::lock_guard<decltype(cr_mutex)> lock(cr_mutex);

        if (cr_from_handle.count(handle) == 0) return false;
        cr_from_handle[handle]->Run(result);
        result->Free();
        UnbindCallResultInternal(handle);
        return true;
    }

    EventManImp::~EventManImp()
    {
        deferred_events.enqueue(std::make_tuple("", -1, nullptr));
        deferred_event_thread.join();
    }
}