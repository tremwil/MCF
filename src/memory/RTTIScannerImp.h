
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
#include "memory/AobScan.h"

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

        AobScannedPtr<uintptr_t, "TEST1"> myaob1 = AOB("48 8B 05 ???????? 48 85 C0 ?? ?? 48 8b 40 ?? C3", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST2"> myaob2 = AOB("48 8B 1D ?? ?? ?? 04 48 8B F9 48 85 DB ?? ?? 8B 11 85 D2 ?? ?? 8D", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST3"> myaob3 = AOB("48 8B 05 ???????? 48 8B 80 60 0C 00 00", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST4"> myaob4 = AOB("48 8B 0D ???????? 48 85 C9 74 26 44 8B", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST5"> myaob5 = AOB("48 89 5C 24 48 8B FA 48 8B D9 C7 44 24 20 00000000 48", 18, AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST6"> myaob6 = AOB("48 8b 05 ???????? 40 0F B6 FF", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST7"> myaob7 = AOB("8B 41 28 48 8B D9 48 8B EA", 23, AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST8"> myaob8 = AOB("48 8B 05 ??????? 66 0F 7F 44 24 40 48 85 C0", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST9"> myaob9 = AOB("48 8B 0D ???????? E8 ???????? 48 8B D8 48 85 C0 0F 84 ? ? ? ? C7", AobType::IpRelative);
        AobScannedPtr<uintptr_t, "TEST10"> myaob10 = AOB("4C 8D 05 ???????? 48 8D 15 ???????? 48 8B CB E8 ???????? 48 83 3D ???????? 00", AobType::IpRelative);

    public:
        explicit RTTIScannerImp(bool* success)
        {

        };

        /// Get a pointer to the RTTI complete object locator of the given class, or nullptr if not found.
        const RTTICompleteObjectLocator* GetObjectLocator(const MemRegion* search_region, const char* demangled_name) override;

        /// Get a pointer to the virtual method table of the given class, or nullptr if not found.
        uintptr_t GetVmt(const MemRegion* search_region, const char* demangled_name) override;
    };
} // MCF