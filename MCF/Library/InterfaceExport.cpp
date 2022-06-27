#include "InterfaceExport.h"

std::vector<MCF::SIMeta*> MCF::AutoExportBase::metas;
extern "C" __declspec(dllexport) MCF::SIMeta** MCF_GetExportedInterfacesMeta(size_t* n)
{
	return MCF::AutoExportBase::Export(n);
}