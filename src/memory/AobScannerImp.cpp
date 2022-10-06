
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "AobScannerImp.h"
#include "util/HexDump.h"
#include "core/Export.h"
#include "aob_scanners/AhoCorasickScanner.h"
#include "mem/mem.h"
#include "mem/pattern.h"
#include "Zydis/Zydis.h"

#include <chrono>
#include <future>
#include <algorithm>
#include <execution>

using namespace MCF::Utils;
using namespace std::chrono;

namespace MCF
{
    MCF_COMPONENT_EXPORT(AobScannerImp);

    void AobScannerImp::AddIfValid(std::vector<uintptr_t>& res, uintptr_t scan_addr, AobType type, intptr_t offset)
    {
        if (!scan_addr) return;
        auto addr = scan_addr + offset;
        if (type == AobType::IpRelative) addr = GetInstructionStaticAddress(addr);
        if (addr != 0) res.push_back(addr);
    }

    void AobScannerImp::OnLoadComplete(const ComponentMan::LoadCompleteEvent* evt)
    {
        std::lock_guard lock(mutex);

        auto t1 = steady_clock::now();
//
//        std::for_each(std::execution::par, scan_regions.begin(), scan_regions.end(), [this, &logger](const ScanRegion& sr) {
//            logger->Info(this, "SIMD: {}, AHO: {}", sr.simd_aobs.size(), sr.aho_aobs.size());
//
//            if (sr.aho_aobs.size() < 20) { // Less than 20 aobs => parallel SIMD is definitely faster
//                std::vector<RegisteredAob*> aobs = sr.simd_aobs;
//                aobs.insert(aobs.end(), sr.aho_aobs.begin(), sr.aho_aobs.end());
//                std::for_each(std::execution::par, aobs.begin(), aobs.end(), [sr](RegisteredAob* a) {
//                    mem::pattern pat(a->aob.bytes.data(), a->aob.mask.data(), a->aob.bytes.size());
//                    mem::region region(a->aob.search_region.begin, a->aob.search_region.size());
//                    for (const auto& result : mem::scan_all(pat, region)) {
//                        AddIfValid(a->results, result.as<uintptr_t>(), a->aob.type, a->aob.offset);
//                    }
//                });
//            }
//            else {
//                auto f = std::async(std::launch::async, [sr]() {
//                    std::vector<std::string> aho_kws;
//                    for (auto aob : sr.aho_aobs)
//                        aho_kws.push_back(aob->unmasked);
//
//                    AhoCorasickScanner scanner;
//                    scanner.BuildStateMachine(aho_kws);
//                    scanner.Search((uint8_t*)sr.begin, sr.end - sr.begin, [sr](int i, uintptr_t addr) {
//                        auto aob = sr.aho_aobs[i];
//                        auto true_addr = addr - aob->unmasked_offset;
//                        if (!aob->aob.search_region.contains(MemRegion {
//                            true_addr, true_addr + aob->aob.bytes.size() })) return false;
//
//                        const uint8_t* to_scan = (uint8_t*)true_addr;
//                        for (int j = 0; j < aob->aob.bytes.size(); j++) {
//                            if ((aob->aob.bytes[j] ^ to_scan[i]) & aob->aob.mask[i]) return false;
//                        }
//                        AddIfValid(aob->results, true_addr, aob->aob.type, aob->aob.offset);
//                        return false;
//                    });
//                });
//
//                std::for_each(std::execution::par, sr.simd_aobs.begin(), sr.simd_aobs.end(), [sr](RegisteredAob* a) {
//                    mem::pattern pat(a->aob.bytes.data(), a->aob.mask.data(), a->aob.bytes.size());
//                    mem::region region(a->aob.search_region.begin, a->aob.search_region.size());
//                    for (const auto& result : mem::scan_all(pat, region)) {
//                        AddIfValid(a->results, result.as<uintptr_t>(), a->aob.type, a->aob.offset);
//                    }
//                });
//
//                f.get();
//            }
//        });

//        std::for_each(std::execution::par, registered_aobs.begin(), registered_aobs.end(), [](std::pair<const HCallResult, RegisteredAob>& p) {
//            auto a = p.second;
//            mem::pattern pat(a.aob.bytes.data(), a.aob.mask.data(), a.aob.bytes.size());
//            mem::region region(a.aob.search_region.begin, a.aob.search_region.size());
//            for (const auto& result : mem::scan_all(pat, region)) {
//                AddIfValid(a.results, result.as<uintptr_t>(), a.aob.type, a.aob.offset);
//            }
//        });

        for (auto& [cr, a] : registered_aobs) {
            mem::pattern pat(a.aob.bytes.data(), a.aob.mask.data(), a.aob.bytes.size());
            mem::region region(a.aob.search_region.begin, a.aob.search_region.size());
            for (const auto& result : mem::scan_all(pat, region)) {
                AddIfValid(a.results, result.as<uintptr_t>(), a.aob.type, a.aob.offset);
            }

//            AobScanResult result{};
//            result.addresses = const_cast<uintptr_t*>(a.results.data()),
//            result.num_results = a.results.size();
//            event_man->RaiseCallResult(cr, &result);
        }

        auto t2 = steady_clock::now();

        Dependency<Logger> logger;
        Logger::Get()->Info(this, "Total AOB scan time: {} ms", duration_cast<milliseconds>(t2 - t1).count());

        scan_regions.clear();
        registered_aobs.clear();
    }

    RegisteredAob::RegisteredAob(const CAob* caob, HCallResult cr) : cr(cr)
    {
        // Copy AOB
        aob.type = caob->type;
        aob.offset = caob->offset;
        aob.search_region = caob->search_region;

        aob.bytes.resize(caob->size);
        memcpy(aob.bytes.data(), caob->bytes, caob->size);
        aob.mask.resize(caob->size);
        memcpy(aob.mask.data(), caob->mask, caob->size);

        // Compute the longest unmasked substring for Aho-Corasick
        int ci = -1;
        int clen = 0;
        int prob_best = 0;
        int prob_score = 0;

        for (int i = 0; i < aob.mask.size(); i++) {
            if (aob.mask[i] == 0xFF) {
                clen += 1;
                // In case of a tie, we keep the longest substring which is less likely to be found
                // in the executable according to a very crude heuristic
                prob_score += mem::simd_scanner::default_frequencies()[aob.bytes[i]];
                if (ci < 0) ci = i;
            } else {
                ci = -1;
                clen = 0;
                prob_score = 0;
            }
            if (clen > unmasked_len || (clen == unmasked_len && prob_score < prob_best)) {
                unmasked_len = clen;
                unmasked_offset = ci;
                prob_best = prob_score;
            }
        }

        unmasked = std::string(aob.bytes.data() + unmasked_offset, aob.bytes.data() + unmasked_offset + unmasked_len);
    }

    uintptr_t AobScannerImp::GetInstructionStaticAddress(uintptr_t instr_addr)
    {
        ZydisDecoder decoder;
#ifdef _WIN64
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
#endif
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(
                &decoder, (void*)instr_addr, ZYDIS_MAX_INSTRUCTION_LENGTH, &instruction,operands,
                ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY))) {
        }

        for (int i = 0; i < instruction.operand_count; i++) {
            if (operands[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                return instr_addr + instruction.length + operands[i].imm.value.u;
            }
            // Instruction which accesses a static memory address for reading/writing/jumping
            if (operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[i].mem.type == ZYDIS_MEMOP_TYPE_MEM) {
                return instr_addr + instruction.length + operands[i].mem.disp.value;
            }
        }
        return 0;
    }

    AobScannerImp::AobScannerImp(bool* success)
    {

    }

    bool AobScannerImp::RegisterAob(const CAob* aob, CallResultBase* call_result)
    {
        std::lock_guard lock(mutex);
        if (!aob->size || !aob->search_region.size()) return false;

        auto cr = event_man->BindCallResult(call_result);
        if (cr == 0) return false;

        registered_aobs[cr] = RegisteredAob(aob, cr);

        ScanRegion sr(&registered_aobs[cr]);
        // If there are no regions, we can simply add it
        if (scan_regions.empty()) {
            scan_regions.insert(sr);
            return true;
        }
        // Find element with the largest end either on top or intersecting the new region
        auto upper = scan_regions.upper_bound(sr);
        if (upper != scan_regions.end() && upper->Intersects(sr)) upper++;

        // Find the earliest region which intersects
        auto lower = upper;
        if (lower == scan_regions.end()) lower--;
        while (lower != scan_regions.begin() && lower->Intersects(sr)) lower--;
        // If no element intersects, just add it
        if (lower == upper && !lower->Intersects(sr)) {
            scan_regions.insert(sr);
            return true;
        }
        if (!lower->Intersects(sr)) lower++;

        // Merge everything from lower to upper in the scan region, then delete them
        for (auto i = lower; i != upper; i++) sr.Merge(*i);
        scan_regions.erase(lower, upper);
        scan_regions.insert(sr);
        return true;
    }

    void AobScannerImp::AobScan(const CAob* aob, AobScanResult* scan_result)
    {
        mem::pattern pat(aob->bytes, aob->mask, aob->size);
        auto results = mem::scan_all(pat, mem::region(aob->search_region.begin, aob->search_region.size()));

        std::vector<uintptr_t> filtered_results;
        for (auto res : results) {
            AddIfValid(filtered_results, res.as<uintptr_t>(), aob->type, aob->offset);
        }

        scan_result->num_results = filtered_results.size();
        scan_result->addresses = (uintptr_t*)calloc(filtered_results.size(), sizeof(uintptr_t));
        scan_result->deallocator = &free;

        memcpy(scan_result->addresses, filtered_results.data(), filtered_results.size() * sizeof(uintptr_t));
    }

    uintptr_t AobScannerImp::AobScanUnique(const CAob* aob)
    {
        mem::pattern pat(aob->bytes, aob->mask, aob->size);
        auto result = mem::scan(pat, mem::region(aob->search_region.begin, aob->search_region.size())).as<uintptr_t>();
        if (result == 0) return 0;
        result += aob->offset;
        if (aob->type == AobType::IpRelative)
            result = GetInstructionStaticAddress(result);

        return result;
    }

} // MCF