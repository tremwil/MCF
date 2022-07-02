#include "Export.h"

std::vector<const MCF::ComponentInfo*> MCF::AutoExportBase::comp_infos;
extern "C" __declspec(dllexport) const MCF::ComponentInfo** MCF_GetExportedInterfaces(size_t* n)
{
	return MCF::AutoExportBase::Export(n);
}