#include "SharedInterfaceManImp.h"

namespace MCF
{
	SharedInterfaceBase* SharedInterfaceManImp::GetInterface(const char* version_string) const
	{
		return nullptr;
	} 

	SharedInterfaceManImp::SharedInterfaceManImp()
	{

	}
}

#ifdef MCF_EXPORTS
extern "C" __declspec(dllexport) MCF::SharedInterfaceBase * MCF_GetInterface(const char* version_string)
{
	return nullptr; //MCF::SharedInterfaceManImp::Ins().GetInterface(version_string);
}
#endif