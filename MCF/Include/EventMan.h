#pragma once
#include "SharedInterface.h"
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
		virtual const char* EventName() const = 0;
	};
	
	/// <summary>
	/// Base abstract class for call results. A call result is essentially a single-time callback that is 
	/// automatically unregistered once destroyed, making it safer than passing a callback (which may 
	/// depend on objects with a limited lifetime) to a function directly.
	/// </summary>
	class CallResultBase 
	{ 
	protected:
		friend class EventManImp;
		virtual void Run(void* result) = 0;
	};

	/// <summary>
	/// Handle to a specific bound call result (bound meaning associated with a particular function call).
	/// </summary>
	typedef uint32_t HCallResult;

	/// <summary>
	/// Base class for all events. Event name should be unique. All event structs should be C "POD" structs,
	/// as they are passed across DLLs. Note that call result structs do not need a name, and as such 
	/// inheriting from this class is not required.
	/// </summary>
	template<FixedString event_name>
	struct Event { static constexpr const char* name = event_name; };

	/// <summary>
	/// Class which manages and dispatches events. Events are structued similarly to Steam callbacks
	/// and call results. Anything may listen for and dispatch events.  
	/// </summary>
	class EventMan : public SharedInterface<EventMan, "MCF_EVENT_MAN_001">
	{
	public:
		/// <summary>
		/// Registers a callback to a particular event, defined by the EventName() virtual function of the callback object.
		/// When a call to RaiseEvent with the given event name is made, this callback will be fired.
		/// </summary>
		/// <param name="callback">The callback object.</param>
		virtual void RegisterCallback(EventCallbackBase* callback) = 0;

		/// <summary>
		/// Unregisters a callback.
		/// </summary>
		/// <param name="callback">The callback object.</param>
		virtual void UnregisterCallback(EventCallbackBase* callback) = 0;

		/// <summary>
		/// Raise an event by name. All currently registered event callbacks with this event name will be fired.
		/// Note that no particular firing order is guaranteed.
		/// </summary>
		/// <param name="event_name">The name of the event to be raised.</param>
		/// <param name="event_data">The data associated with this event.</param>
		virtual void RaiseEvent(const char* event_name, void* event_data) = 0;

		/// <summary>
		/// Raise an event by type. All currently registered event callbacks with this event name will be fired.
		/// Note that no particular firing order is guaranteed.
		/// </summary>
		/// <param name="event_data">The data associated with this event.</param>
		template<typename TEvent> void RaiseEvent(TEvent* event_data)
		{
			RaiseEvent(TEvent::name, event_data);
		}

		/// <summary>
		/// Raise an event by type. All currently registered event callbacks with this event name will be fired.
		/// Note that no particular firing order is guaranteed.
		/// </summary>
		/// <param name="event_data">The data associated with this event.</param>
		template<typename TEvent> void RaiseEvent(TEvent event_data)
		{
			RaiseEvent(TEvent::name, &event_data);
		}

		/// <summary>
		/// "Binds" a call result, returning a handle which can be used to call it once the task to be performed 
		/// has been completed. If implementing call results, you MUST call this at the beginning of your function.
		/// </summary>
		/// <param name="call_result">The call result to bind.</param>
		/// <returns>The call result handle. Zero if the operation failed (i.e. the call result was not registered).</returns>
		virtual HCallResult BindCallResult(CallResultBase* call_result) = 0;

		/// <summary>
		/// "Unbinds" a call result handle, making it so the callback will not be fired when RaiseCallResult is 
		/// called with this handle in the future.
		/// </summary>
		/// <param name="handle">The call result handle.</param>
		virtual void UnbindCallResult(HCallResult handle) = 0;

		/// <summary>
		/// Unregisters a call result object. This also unbinds it from all calls.
		/// </summary>
		/// <param name="call_result"></param>
		virtual void UnregisterCallResult(CallResultBase* call_result) = 0;
		
		/// <summary>
		/// Raises a call result by handle, firing the callback which was registered using BindCallResult. 
		/// This unbinds the handle, so it can only be called once.
		/// </summary>
		/// <returns>A boolean indicating success/failure.</returns>
		virtual bool RaiseCallResult(HCallResult handle, void* result) = 0;
	};

	/// <summary>
	/// Event callback which uses an std::function to allow registering callbacks on any callable object.
	/// May be registered on creation or manually. Unregistered automatically when destroyed.
	/// </summary>
	/// <typeparam name="TEvent">The associated event data type.</typeparam>
	template<typename TEvent>
	class EventCallback : public EventCallbackBase
	{
	protected:
		std::function<void(TEvent*)> fun;

		bool TryRegister()
		{
			EventMan* man = EventMan::Get();
			if (!man) return false;
			man->RegisterCallback(this);
			return true;
		}

		virtual void Run(void* event_data) override { fun((TEvent*)event_data); }
		virtual const char* EventName() const override { return TEvent::name; }

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
			EventMan* man = EventMan::Get();
			if (!man) return false;
			man->UnregisterCallback(this);
			return true;
		}

		EventCallback() { }

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

	template<typename TEvent>
	class CallResult : public CallResultBase
	{
	protected:
		std::function<void(TEvent*)> fun;
		virtual void Run(void* event_data) override { fun((TEvent*)event_data); }

	public:

		/// <summary>
		///  Sets the callback of this call result to a function to a member function pointer.
		/// </summary>
		template<typename TObj>
		void SetCallback(void(TObj::* cb)(TEvent*), TObj* instance)
		{
			fun = std::bind(cb, instance);
		}

		/// <summary>
		/// Sets the callback of this call result to a general callable object.
		/// </summary>
		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
		void SetCallback(TCallable cb)
		{
			fun = cb;
		}
		
		/// <summary>
		/// Attempts to unregister this call result.
		/// </summary>
		bool Unregister()
		{
			EventMan* man = EventMan::Get();
			if (!man) return false;
			man->UnregisterCallResult(this);
			return true;
		}

		CallResult() { }

		/// <summary>
		/// Constructs and registers this call result with a function to a member function pointer.
		/// </summary>
		template<typename TObj>
		CallResult(void(TObj::* cb)(TEvent*), TObj* instance) { SetCallback(cb, instance); }

		/// <summary>
		/// Constructs and registers this call result with a general callable object.
		/// </summary>
		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
		CallResult(TCallable cb) { SetCallback(cb); }

		~CallResult() { Unregister(); }
	};
}