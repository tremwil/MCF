#pragma once
#include "Include/SharedInterfaceMan.h"
#include "Include/InterfaceExport.h"

namespace MCF
{
	class SharedInterfaceManImp final : public SharedInterfaceImp<SharedInterfaceMan, SharedInterfaceManImp, DepList<EventMan>, true>
	{
	public:
		SharedInterfaceManImp();

		EventCallback<SharedInterfaceMan::InterfaceLoadEvent> OnLoad = [this](SharedInterfaceMan::InterfaceLoadEvent* e)
		{
			printf("Loaded!");
		};

		virtual bool IsUnloadable() const override { return true; }

		virtual SharedInterfaceBase* GetInterface(const char* version_string) const override;
	};
	MCF_INTERFACE_EXPORT(SharedInterfaceManImp);
}