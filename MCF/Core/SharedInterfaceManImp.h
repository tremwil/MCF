#pragma once
#include "Library/SharedInterfaceMan.h"
#include "Library/InterfaceExport.h"

namespace MCF
{
	class SharedInterfaceManImp final : public SharedInterfaceMan
	{
	public:
		SharedInterfaceManImp();

		EventCallback<SharedInterfaceMan::InterfaceLoadEvent> OnLoad = [this](SharedInterfaceMan::InterfaceLoadEvent* e)
		{
			printf("Loaded!");
		};

		virtual SharedInterfaceBase* GetInterface(const char* version_string) const override;
	};
	MCF_INTERFACE_EXPORT(SharedInterfaceManImp);
}