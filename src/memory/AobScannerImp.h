
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/core/ComponentMan.h"
#include "include/memory/AobScan.h"
#include "aob_scanners/AhoCorasickScanner.h"
#include <set>
#include <unordered_set>
#include <mutex>

namespace MCF
{
    class AhoSubstring;

    struct RegisteredAob
    {
        AOB aob;
        std::string unmasked;
        int unmasked_len = 0;
        int unmasked_offset = 0;
        HCallResult cr;

        std::vector<uintptr_t> results;

        RegisteredAob() = default;
        RegisteredAob(const CAob* caob, HCallResult cr);
    };

    struct ScanRegion
    {
        uintptr_t begin;
        uintptr_t end;

        std::vector<RegisteredAob*> aho_aobs;
        std::vector<RegisteredAob*> simd_aobs;

        inline explicit ScanRegion(RegisteredAob* aob)
        {
            begin = aob->aob.search_region.begin;
            end = aob->aob.search_region.end;
            if (aob->unmasked_len <= 4) simd_aobs.push_back(aob);
            else aho_aobs.push_back(aob);
        }

        inline bool Intersects(const ScanRegion& other) const
        {
            return end > other.begin && other.end > begin;
        }

        // Merge two intersecting regions
        inline void Merge(const ScanRegion& other)
        {
            begin = std::min(begin, other.begin);
            end = std::max(end, other.end);
            aho_aobs.insert(aho_aobs.end(), other.aho_aobs.begin(), other.aho_aobs.end());
            simd_aobs.insert(simd_aobs.end(), other.simd_aobs.begin(), other.simd_aobs.end());
        }

        inline auto operator<=>(const ScanRegion& other) const { return end <=> other.end; }
    };

    class AobScannerImp : public SharedInterfaceImp<AobScanner, AobScannerImp>
    {
    private:
        Dependency<EventMan> event_man;

        std::set<ScanRegion> scan_regions;
        std::unordered_map<HCallResult, RegisteredAob> registered_aobs;

        std::mutex mutex;

        // Extract a static memory/code address from an instruction.
        static uintptr_t GetInstructionStaticAddress(uintptr_t instr_addr);

        static void AddIfValid(std::vector<uintptr_t>& res, uintptr_t scan_addr, AobType type, intptr_t offset);

        void OnLoadComplete(const ComponentMan::LoadCompleteEvent* evt);
        EventCallback<ComponentMan::LoadCompleteEvent> load_complete_event = { &AobScannerImp::OnLoadComplete, this };

    public:
        AobScannerImp(bool* success);

        /// Register an AOB to be scanned once all components are loaded or after a set delay (100ms). This allows the
        /// scanner to use an efficient DFA based algorithm (Aho-Corasick) to scan all AOBs simultaneously.
        /// \param aob The AOB to scan.
        /// \param call_result The call result whose callback will be executed once the aob is found (or not). Argument is a AobScanResult.
        /// \return true if the AOB was successfully registered; false otherwise.
        bool RegisterAob(const CAob* aob, CallResultBase* call_result) override;

        /// Scan for an AOB immediately and return the results.
        /// NOTE: You *MUST* call the FreeAddresses() method of the scan_result after use to avoid leaking memory.
        /// \param aob the AOB to scan.
        /// \param scan_result Structure containing the scan results.
        void AobScan(const CAob* aob, AobScanResult* scan_result) override;

        /// Scan for an AOB immediately and return the first result, or 0 if it could not be found.
        /// \param aob The AOB to scan.
        /// \return The first address matching the AOB, if found. 0 otherwise.
        uintptr_t AobScanUnique(const CAob* aob) override;
    };

} // MCF