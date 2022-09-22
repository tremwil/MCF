
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "RTTIScannerImp.h"
#include "include/core/Logger.h"
#include "include/core/Export.h"

namespace MCF
{
    MCF_COMPONENT_EXPORT(RTTIScannerImp);

    const RTTICompleteObjectLocator*
    RTTIScannerImp::GetObjectLocator(const MemRegion* search_region, const char *demangled_name)
    {
        if (search_region->size() == 0) return nullptr;

        std::lock_guard lock(mutex);
        ScanRegionIfRequired(search_region);

        auto& locators = cache[search_region->module_base].locators_by_name;
        return locators.contains(demangled_name) ? locators[demangled_name] : nullptr;
    }

    uintptr_t RTTIScannerImp::GetVmt(const MemRegion* search_region, const char *demangled_name)
    {
        if (search_region->size() == 0) return 0;

        std::lock_guard lock(mutex);
        ScanRegionIfRequired(search_region);

        auto& vtables = cache[search_region->module_base].vtables_by_name;
        return vtables.contains(demangled_name) ? vtables[demangled_name] : 0;
    }

    void RTTIScannerImp::ScanRegionIfRequired(const MemRegion* search_region)
    {
        MemRegion to_scan = *search_region;

        // If we already searched this module but in a smaller region, search again
        if (cache.contains(to_scan.module_base)) {
            auto& searched = cache[to_scan.module_base].searched_region;
            if (to_scan.contains(searched)) return;
            else to_scan = to_scan.combine(searched);
        }

        auto& module_cache = cache[to_scan.module_base];
        module_cache.locators_by_name.clear();
        module_cache.vtables_by_name.clear();

        // If the scan region is not aligned to the pointer size, align it
        to_scan.begin = to_scan.begin + sizeof(void*) - 1 & ~(sizeof(void*) - 1);

        // object locator ptr will be properly aligned, so we can safely skip by sizeof(void*)
        for (auto p = reinterpret_cast<RTTICompleteObjectLocator**>(to_scan.begin); to_scan.contains(p); p++)
        {
            auto locator = *p;
            if (!to_scan.contains(locator)) continue;
            if (!locator->IsSignatureValid()) continue;
#ifdef _WIN64
            if (to_scan.ibo2ptr_unchecked<RTTICompleteObjectLocator>(locator->pSelf) != locator) continue;
#endif
            auto type = locator->GetType(&to_scan);
            if (!type) continue;
            if (type->mangledName[0] != '.' || type->mangledName[1] != '?') continue;

            std::string name = type->name();
            if (name.empty()) continue;

            module_cache.locators_by_name[name] = locator;
            module_cache.vtables_by_name[name] = reinterpret_cast<uintptr_t>(p + 1);
        }
    }
} // MCF