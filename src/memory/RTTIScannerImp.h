
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/memory/RTTI.h"

#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace MCF
{
    class RTTIScannerImp final : public SharedInterfaceImp<RTTIScanner, RTTIScannerImp>
    {
    private:
        struct ModuleRTTICache
        {
            MemRegion searched_region;
            std::unordered_map<std::string, const RTTICompleteObjectLocator*> locators_by_name;
            std::unordered_map<std::string, uintptr_t> vtables_by_name;
        };

        std::mutex mutex;
        std::unordered_map<uintptr_t, ModuleRTTICache> cache;

        void ScanRegionIfRequired(const MemRegion* search_region);

    public:
        explicit RTTIScannerImp(bool* success) { };

        /// Get a pointer to the RTTI complete object locator of the given class, or nullptr if not found.
        const RTTICompleteObjectLocator* GetObjectLocator(const MemRegion* search_region, const char* demangled_name) override;

        /// Get a pointer to the virtual method table of the given class, or nullptr if not found.
        uintptr_t GetVmt(const MemRegion* search_region, const char* demangled_name) override;
    };
} // MCF