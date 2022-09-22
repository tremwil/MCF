
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/EventMan.h"
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
        EventManImp(const bool* success);

        bool IsUnloadable() const override { return true; }

        void RegisterCallback(EventCallbackBase* callback) override;

        void UnregisterCallback(EventCallbackBase* callback) override;

        void RaiseEvent(const char* event_name, void* event_data) override;

        HCallResult BindCallResult(CallResultBase* call_result) override;

        void UnbindCallResult(HCallResult handle) override;

        void UnregisterCallResult(CallResultBase* call_result) override;

        bool RaiseCallResult(HCallResult handle, void* result) override;
    };
}