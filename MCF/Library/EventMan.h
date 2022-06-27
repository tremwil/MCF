#pragma once
#include "SharedInterface.h"
#include <functional>

namespace MCF
{
	class EventCallbackBase
	{
	protected:
		friend class EventMan;
		virtual void Run(void* event_data) = 0;
		virtual const char* EventName() const = 0;
	};

	/// <summary>
	/// Class which manages and dispatches events. Events are structued similarly to Steam callbacks.
	/// Anything may listen and dispatch events.  
	/// </summary>
	class EventMan : public SharedInterface<EventMan, "MCF_EVENT_MAN_001">
	{
	public:
		virtual void RaiseEvent(const char* event_name, void* event_data) = 0;
 
		virtual void RegisterCallback(EventCallbackBase* callback) = 0;

		virtual void UnregisterCallback(EventCallbackBase* callback) = 0;
	};


	// Base class for all events. Event name should be unique. All event structs should be C "POD" structs,
	// as they are passed across DLLs.
	template<FixedString event_name>
	struct Event { static constexpr const char* name = event_name; };

	// Abstract template which uses an std::function to allow registering callbacks on any callable object.
	template<typename TEvent>
	class EventCallbackTemplate : public EventCallbackBase
	{
	protected:
		std::function<void(TEvent*)> fun;

		EventCallbackTemplate() { };

		bool Register()
		{
			EventMan* man = EventMan::Get();
			if (!man) return false;
			man->RegisterCallback(this);
			return true;
		}

		bool Unregister()
		{
			EventMan* man = EventMan::Get();
			if (!man) return false;
			man->UnregisterCallback(this);
			return true;
		}

		virtual void Run(void* event_data) override { fun((TEvent*)event_data); }
		virtual const char* EventName() const override { return TEvent::name; }
	};

	template<typename TEvent>
	class EventCallbackManual : public EventCallbackTemplate<TEvent>
	{
	public:
		EventCallbackManual() { }

		template<typename TObj>
		bool Bind(void(TObj::* cb)(TEvent*), TObj* instance)
		{
			this->fun = std::bind(cb, instance);
			return this->Register();
		}

		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
		bool Bind(TCallable cb)
		{
			this->fun = cb;
			return this->Register();
		}

		bool Unbind() { return this->Unregister(); }

		~EventCallbackManual() { this->Unregister(); }
	};

	template<typename TEvent>
	class EventCallback : public EventCallbackTemplate<TEvent>
	{
	public:
		template<typename TObj>
		EventCallback(void(TObj::* cb)(TEvent*), TObj* instance)
		{
			this->fun = std::bind(cb, instance);
			this->Register();
		}

		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, TEvent*>
		EventCallback(TCallable cb)
		{
			this->fun = cb;
			this->Register();
		}

		~EventCallback() { this->Unregister(); }
	};
}