#pragma once
#include "Include/ComponentMan.h"
#include "Include/Export.h"

namespace MCF
{
	class ComponentManImp final : public SharedInterfaceImp<ComponentMan, ComponentManImp, DepList<EventMan>, true>
	{
	public:
		ComponentManImp();

		EventCallback<ComponentMan::InterfaceLoadEvent> OnLoad = [this](ComponentMan::InterfaceLoadEvent* e)
		{
			printf("Loaded!");
		};

		virtual bool IsUnloadable() const override { return true; }

		virtual IComponent* GetComponent(const char* version_string) const override;


	};
	MCF_COMPONENT_EXPORT(ComponentManImp);
}