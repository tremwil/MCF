
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "SharedInterface.h"
#include "util/TemplateUtils.h"
#include <functional>

namespace MCF
{
    /// <summary>
    /// Base abstract class for all event callbacks (listeners).
    /// </summary>
    class EventCallbackBase
    {
    protected:
        friend class EventManImp;

        virtual void Run(void* event_data) = 0;
        [[nodiscard]] virtual const char* EventName() const = 0;
    };

    /// Base abstract class for call results. A call result is essentially a single-time callback that is
    /// automatically unregistered once destroyed, making it safer than passing a callback (which may
    /// depend on objects with a limited lifetime) to a function directly.
    class CallResultBase
    {
    protected:
        friend class EventManImp;
        virtual void Run(void* result) = 0;
    };

    /// Handle to a specific bound call result (bound meaning associated with a particular function call).
    typedef uint32_t HCallResult;

    /// Base class for all events. Event name should be unique. All event structs should be C "POD" structs,
    /// as they are passed across DLLs. Note that call result structs do not need a name, and as such
    /// inheriting from this class is not required.
    template<FixedString event_name>
    struct Event { static constexpr const char* name = event_name; };

    /// Class which manages and dispatches events. Events are structured similarly to Steam callbacks
    /// and call results. Anything may listen for and dispatch events.
    class EventMan : public SharedInterface<EventMan, "MCF_EVENT_MAN_001">
    {
    public:
        /// Registers a callback to a particular event, defined by the EventName() virtual function of the callback object.
        /// When a call to RaiseEvent with the given event name is made, this callback will be fired.
        /// \param callback The callback object.
        virtual void RegisterCallback(EventCallbackBase* callback) = 0;

        /// Unregisters a callback.
        /// \param callback The callback object.
        virtual void UnregisterCallback(EventCallbackBase* callback) = 0;

        /// Raise an event by name. All currently registered event callbacks with this event name will be fired.
        /// Note that no particular firing order is guaranteed.
        virtual void RaiseEvent(const char* event_name, void* event_data) = 0;

        /// Raise an event by type. All currently registered event callbacks with this event name will be fired.
        /// Note that no particular firing order is guaranteed.
        template<typename TEvent> void RaiseEvent(TEvent* event_data)
        {
            RaiseEvent(TEvent::name, event_data);
        }

        /// Raise an event by type. All currently registered event callbacks with this event name will be fired.
        /// Note that no particular firing order is guaranteed.
        template<typename TEvent> void RaiseEvent(TEvent event_data)
        {
            RaiseEvent(TEvent::name, &event_data);
        }

        /// "Binds" a call result, returning a handle which can be used to call it once the task to be performed
        /// has been completed. If implementing call results, you MUST call this at the beginning of your function.
        /// \param call_result The call result to bind
        /// \return The call result handle. Zero if the operation failed (i.e. the call result was not registered)
        [[nodiscard]] virtual HCallResult BindCallResult(CallResultBase* call_result) = 0;

        /// "Unbinds" a call result handle, making it so the callback will not be fired when RaiseCallResult is
        /// called with this handle in the future.
        virtual void UnbindCallResult(HCallResult handle) = 0;

        /// Unregisters a call result object. This also unbinds it from all calls.
        virtual void UnregisterCallResult(CallResultBase* call_result) = 0;

        /// Raises a call result by handle, firing the callback which was registered using BindCallResult.
        /// This unbinds the handle, so it can only be called once.
        /// \return A boolean indicating success/failure.
        virtual bool RaiseCallResult(HCallResult handle, void* result) = 0;

        /// Raises a call result by handle, firing the callback which was registered using BindCallResult.
        /// This unbinds the handle, so it can only be called once.
        /// \return A boolean indicating success/failure.
        template<typename TResult> bool RaiseEvent(HCallResult handle, TResult result)
        {
            return RaiseCallResult(handle, &result);
        }
    };

    /// Event callback which uses an std::function to allow registering callbacks on any callable object.
    /// May be registered on creation or manually. Unregistered automatically when destroyed.
    /// \tparam TEvent The associated event type
    template<typename TEvent>
    class EventCallback : public EventCallbackBase, private NonAssignable
    {
    protected:
        std::function<void(TEvent*)> fun;
        Dependency<EventMan> eventMan;

        bool TryRegister()
        {
            if (!eventMan) return false;
            eventMan->RegisterCallback(this);
            return true;
        }

        void Run(void* event_data) override { fun((TEvent*)event_data); }
        [[nodiscard]] const char* EventName() const override { return TEvent::name; }

    public:

        /// <summary>
        /// Registers this callback with a function to a member function pointer.
        /// </summary>
        template<typename TObj>
        bool Register(void(TObj::* cb)(TEvent*), TObj* instance)
        {
            fun = std::bind(cb, instance);
            return TryRegister();
        }

        /// <summary>
        /// Registers this callback with a general callable object.
        /// </summary>
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
        bool Register(TCallable cb)
        {
            fun = cb;
            return TryRegister();
        }

        /// <summary>
        /// Unregisters this callback object.
        /// </summary>
        bool Unregister()
        {
            if (!eventMan) return false;
            eventMan->UnregisterCallback(this);
            return true;
        }

        EventCallback() = default;

        /// <summary>
        /// Constructs and registers this callback with a function to a member function pointer.
        /// </summary>
        template<typename TObj>
        EventCallback(void(TObj::* cb)(TEvent*), TObj* instance)
        {
            Register(cb, instance);
        }

        /// <summary>
        /// Constructs and registers this callback with a general callable object.
        /// </summary>
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
        EventCallback(TCallable cb)
        {
            Register(cb);
        }

        ~EventCallback() { Unregister(); }
    };

    template<typename TResult>
class CallResult : public CallResultBase, private NonAssignable
    {
    protected:
        Dependency<EventMan> eventMan;

        std::function<void(TResult*)> fun;
        void Run(void* result) override { fun((TResult*)result); }

    public:

        /// <summary>
        ///  Sets the callback of this call result to a function to a member function pointer.
        /// WARNING: Changing this after the call result has been bound is not thread safe.
        /// </summary>
        template<typename TObj>
        void SetCallback(void(TObj::* cb)(TResult*), TObj* instance)
        {
            fun = std::bind(cb, instance);
        }

        /// <summary>
        /// Sets the callback of this call result to a general callable object.
        /// WARNING: Changing this after the call result has been bound is not thread safe.
        /// </summary>
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TResult*>
        void SetCallback(TCallable cb)
        {
            fun = cb;
        }

        /// <summary>
        /// Attempts to unregister this call result.
        /// </summary>
        void Unregister()
        {
            if (eventMan) eventMan->UnregisterCallResult(this);
        }

        CallResult() = default;

        /// <summary>
        /// Constructs and registers this call result with a function to a member function pointer.
        /// </summary>
        template<typename TObj>
        CallResult(void(TObj::* cb)(TResult*), TObj* instance) { SetCallback(cb, instance); }

        /// <summary>
        /// Constructs and registers this call result with a general callable object.
        /// </summary>
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TResult*>
        CallResult(TCallable cb) { SetCallback(cb); }

        ~CallResult() { Unregister(); }
    };
}