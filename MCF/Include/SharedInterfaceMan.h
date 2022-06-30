#pragma once
#include "EventMan.h"

namespace MCF
{
	/// <summary>
	/// The shared interface manager. Responsible for loading/unloading interfaces provided by different DLLs.
	/// </summary>
	class SharedInterfaceMan : public SharedInterface<SharedInterfaceMan, "MCF_SHARED_INTERFACE_MAN_001">
	{
	public:
		/// <summary>
		///Event raised when a shared interface was just loaded.
		// Provides the name and instance of the loaded interface.
		/// </summary>
		struct InterfaceLoadEvent : public Event<"MCF_SIM_INTERFACE_LOAD_EVENT">
		{
			const char* version_string;
			SharedInterfaceBase* instance;
		};

		/// <summary>
		/// Event raised when a batch shared interface load has begun.
		/// </summary>
		struct LoadBeginEvent : public Event<"MCF_SIM_LOAD_BEGIN_EVENT"> { };

		/// <summary>
		/// Event raised when a batch shared interface load has completed.
		/// </summary>
		struct LoadCompleteEvent : public Event<"MCF_SIM_LOAD_COMPLETE_EVENT"> { };

		virtual SharedInterfaceBase* GetInterface(const char* version_string) const = 0;
	};
}