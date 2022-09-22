
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "Export.h"

std::vector<const MCF::ComponentFactory*> MCF::AutoExportBase::factories;
extern "C" __declspec(dllexport) const MCF::ComponentFactory** MCF_GetExportedComponents(size_t* n)
{
    return MCF::AutoExportBase::Export(n);
}