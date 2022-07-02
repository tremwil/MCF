#include "ComponentManImp.h"

namespace MCF
{
	IComponent* ComponentManImp::GetComponent(const char* version_string) const
	{
		return nullptr;
	} 

	ComponentManImp::ComponentManImp()
	{

	}
}

#ifdef MCF_EXPORTS
extern "C" __declspec(dllexport) MCF::IComponent * MCF_GetComponent(const char* version_string)
{
	return nullptr; //MCF::SharedInterfaceManImp::Ins().GetInterface(version_string);
}
#endif