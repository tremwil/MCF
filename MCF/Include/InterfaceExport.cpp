#include "InterfaceExport.h"

std::vector<const MCF::ImpDetails*> MCF::AutoExportBase::metas;
extern "C" __declspec(dllexport) const MCF::ImpDetails** MCF_GetExportedInterfacesMeta(size_t* n)
{
	return MCF::AutoExportBase::Export(n);
}