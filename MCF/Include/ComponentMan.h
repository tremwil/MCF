#pragma once
#include "EventMan.h"

namespace MCF
{
	/// <summary>
	/// The shared interface manager. Responsible for loading/unloading interfaces provided by different DLLs.
	/// </summary>
	class ComponentMan : public SharedInterface<ComponentMan, "MCF_COMPONENT_MAN_001">
	{
	public:
		/// <summary>
		///Event raised when a shared interface was just loaded.
		// Provides the name and instance of the loaded interface.
		/// </summary>
		struct InterfaceLoadEvent : public Event<"MCF_CM_INTERFACE_LOAD_EVENT">
		{
			const char* version_string;
			IComponent* instance;
		};

		/// <summary>
		/// Event raised when a batch shared interface load has begun.
		/// </summary>
		struct LoadBeginEvent : public Event<"MCF_CM_LOAD_BEGIN_EVENT"> { };

		/// <summary>
		/// Event raised when a batch shared interface load has completed.
		/// </summary>
		struct LoadCompleteEvent : public Event<"MCF_CM_LOAD_COMPLETE_EVENT"> { };


		virtual IComponent* GetComponent(const char* version_string) const = 0;
	};
}