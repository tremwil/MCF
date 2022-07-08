#include "Export.h"

std::vector<const MCF::CompInfo*> MCF::AutoExportBase::comp_infos;
extern "C" __declspec(dllexport) const MCF::CompInfo** MCF_GetExportedComponents(size_t* n)
{
	return MCF::AutoExportBase::Export(n);
}