#pragma once
#include "SharedInterface.h"

namespace MCF
{
	class EventCallbackBase
	{
	public:
		virtual void Run(void* event_data) = 0;
	};

	/// <summary>
	/// Class which manages and dispatches events. Events are structued similarly to Steam callbacks.
	/// Any SharedInterface may register and raise events. Events can be dispatched to class or static functions.
	/// </summary>
	class EventMgr : public SharedInterface<EventMgr>
	{
	public:
		SIMeta("Core::EventMgr", 1);

		virtual bool RegisterEvent(const SIMetadata source, int event_code) = 0;
		template<typename SI>
		inline bool RegisterEvent(int event_code)
		{
			RegisterEvent(&SI::si_metadata, event_code);
		}

		virtual bool UnegisterEvent(const SIMetadata source, int event_code) = 0;
		template<typename SI, typename EC>
		inline bool UnregisterEvent(EC event_code)
		{
			UnregisterEvent(&SI::si_metadata, static_cast<int>(event_code));
		}

		virtual void RaiseEvent(const SIMetadata source, int event_code, void* event_data) = 0;
		template<typename SI, typename EC, typename ED>
		inline bool RaiseEvent(EC event_code, ED* event_data)
		{
			RaiseEvent(&SI::si_metadata, static_cast<int>(event_code), event_data);
		}
 
		virtual void RegisterCallback(const SIMetadata source, int event_code, EventCallbackBase* callback) = 0;

		virtual void UnregisterCallback(EventCallbackBase* callback) = 0;
	};
}