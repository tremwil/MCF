
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <string>

#include "include/windows_include.h"
#include <dbghelp.h>

#include "core/SharedInterface.h"
#include "MemRegion.h"

#pragma comment(lib, "dbghelp.lib")

namespace MCF
{
    struct PMD
    {
        int32_t mdisp;
        int32_t pdisp;
        int32_t vdisp;
    };

    struct RTTITypeDescriptor
    {
        void* typeInfoVtable;
        void* spare;
        const char mangledName[1];

        inline std::string name() const
        {
            char name_buff[2048];
            DWORD sz = UnDecorateSymbolName(mangledName + 1, name_buff, sizeof(name_buff),
                                            UNDNAME_32_BIT_DECODE |
                                            UNDNAME_NAME_ONLY     |
                                            UNDNAME_NO_ARGUMENTS  |
                                            UNDNAME_NO_MS_KEYWORDS);

            if (sz) return name_buff;
            else return "";
        }
    };

    struct RTTIBaseClassDescriptor
    {
        ibo32 pTypeDescriptor;
        uint32_t numContainedBases; // Same as numBasesClasses in hierarchy descriptor
        PMD where;
        uint32_t attributes;

        inline RTTITypeDescriptor* GetType(const MemRegion* module) const
        {
            return module->ibo2ptr<RTTITypeDescriptor>(pTypeDescriptor);
        }
    };

    struct RTTIBaseClassArray
    {
        ibo32 arrayOfBaseClassDescriptors[1];

        inline RTTIBaseClassDescriptor* GetBaseClass(const MemRegion* module, int index) const
        {
            return module->ibo2ptr<RTTIBaseClassDescriptor>(arrayOfBaseClassDescriptors[index]);
        }
    };

#define MCF_RTTI_MULTIPLE_INHERITANCE 1
#define MCF_RTTI_VIRTUAL_INHERITANCE 2

    struct RTTIClassHierarchyDescriptor
    {
        uint32_t signature;  // 0 = x86, 1 = x64
        uint32_t attributes; // bit 0 = multiple inheritance, bit 1 = virtual inheritance
        uint32_t numBaseClasses;
        ibo32 pBaseClassArray;

        inline RTTIBaseClassArray* GetBaseClassArray(const MemRegion* module) const
        {
            return module->ibo2ptr<RTTIBaseClassArray>(pBaseClassArray);
        }

        inline RTTIBaseClassDescriptor* GetBaseClass(const MemRegion* module, int index) const
        {
            auto class_array = GetBaseClassArray(module);
            return class_array ? class_array->GetBaseClass(module, index) : nullptr;
        }

        bool IsSignatureValid() const
        {
            if constexpr (sizeof(void*) == 4) return signature == 0;
            else return signature == 1;
        }

        bool HasBaseClass(const MemRegion* module, const RTTITypeDescriptor* type) const
        {
            auto class_array = GetBaseClassArray(module);
            if (!class_array) return false;

            for (int i = 0; i < numBaseClasses; i++)
            {
                auto base_class = class_array->GetBaseClass(module, i);
                if (base_class->GetType(module) == type) return true;
            }
            return false;
        }
    };

    struct RTTICompleteObjectLocator
    {
        uint32_t signature; // 0 = x86, 1 = x64
        uint32_t offset;    // offset of vtable within class
        uint32_t cdOffset;  // constructor displacement offset
        ibo32 pTypeDescriptor;
        ibo32 pClassDescriptor;

#ifdef _WIN64
        ibo32 pSelf;
#endif

        inline const RTTITypeDescriptor* GetType(const MemRegion* module) const
        {
            return module->ibo2ptr<RTTITypeDescriptor>(pTypeDescriptor);
        }

        inline const RTTIClassHierarchyDescriptor* GetClass(const MemRegion* module) const
        {
            return module->ibo2ptr<RTTIClassHierarchyDescriptor>(pClassDescriptor);
        }

        inline bool IsSignatureValid() const
        {
            if constexpr (sizeof(void*) == 4) return signature == 0;
            else return signature == 1;
        }
    };

    /// Scans RTTI for given modules and caches results, allowing to efficiently query virtual method tables and RTTI objects
    /// based on the demangled class name.
    class RTTIScanner : public SharedInterface<RTTIScanner, "MCF_RTTI_SCANNER_001">
    {
    public:
        /// Get a pointer to the RTTI complete object locator of the given class, or nullptr if not found.
        virtual const RTTICompleteObjectLocator* GetObjectLocator(const MemRegion* search_region, const char* demangled_name) = 0;

        /// Get a pointer to the virtual method table of the given class, or nullptr if not found.
        virtual uintptr_t GetVmt(const MemRegion* search_region, const char* demangled_name) = 0;

        /// Get a pointer to the RTTI complete object locator of the given class, or nullptr if not found.
        inline const RTTICompleteObjectLocator* GetObjectLocator(const MemRegion& search_region, const char* demangled_name)
        {
            return GetObjectLocator(&search_region, demangled_name);
        }

        /// Get a pointer to the virtual method table of the given class, or nullptr if not found.
        inline uintptr_t GetVmt(const MemRegion& search_region, const char* demangled_name)
        {
            return GetVmt(&search_region, demangled_name);
        }

        /// Get a pointer to the RTTI complete object locator of the given class in the main module, or nullptr if not found.
        inline const RTTICompleteObjectLocator* GetObjectLocator(const char* demangled_name)
        {
            return GetObjectLocator(MainModuleRdata(), demangled_name);
        }

        /// Get a pointer to the virtual method table of the given class in the main module, or nullptr if not found.
        inline uintptr_t GetVmt(const char* demangled_name)
        {
            return GetVmt(MainModuleRdata(), demangled_name);
        }
    };
}