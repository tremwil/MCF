
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/SharedInterface.h"
#include "MemRegion.h"

namespace MCF
{
    typedef uintptr_t HMemPatch;

    template<typename T>
    struct CList
    {
        T* items;
        size_t count;
    };

    struct MemoryPatch
    {
        HMemPatch handle;
        uintptr_t addr;
        size_t size;
        uint8_t* orig_mem;
    };

    struct OriginalMemSegment
    {
        uintptr_t addr;
        uint8_t* memory;
        size_t size;
    };

    // Interface for registering patches to program memory.
    // Use whenever possible when editing code so that AOB scans can always be carried out on original memory.
    class MemoryPatcher : public SharedInterface<MemoryPatcher, "MCF_MEMORY_PATCHER_001">
    {
    public:
        virtual HMemPatch ApplyPatch(const void* addr, const uint8_t* patch, size_t patch_size) = 0;

        virtual HMemPatch RegisterPatch(const void* addr, const uint8_t* orig_mem, size_t patch_size) = 0;

        virtual bool RestoreMemory(HMemPatch patch) = 0;

        virtual CList<MemoryPatch>* GetPatchList(const MemRegion* in_region) = 0;

        virtual void FreePatchList(CList<MemoryPatch>* list) = 0;

        virtual CList<OriginalMemSegment>* GetOriginalSegments(const MemRegion* in_region) = 0;

        virtual void FreeOriginalSegments(CList<OriginalMemSegment>* list) = 0;
    };
}