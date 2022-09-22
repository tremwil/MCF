
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/EventMan.h"
#include "core/Logger.h"
#include "MemRegion.h"
#include "util/TemplateUtils.h"
#include <string>
#include <utility>

namespace MCF
{
    enum class AobType : uint8_t
    {
        /// Just returns the address of the AOB + the offset.
        Address,
        /// Assumes the AOB + offset points to a instruction pointer relative instruction and returns the address
        /// being read/written by said instruction.
        IpRelative
    };

    struct AOB
    {
        /// Region of memory to search the AOB string in.
        MemRegion search_region = MainModuleText();
        /// Sequence of bytes to scan for.
        std::vector<uint8_t> bytes;
        /// Bitmask for the above sequence of bytes.
        std::vector<uint8_t> mask;
        /// Constant offset added to the start address of the AOB.
        intptr_t offset = 0;
        /// Aob type.
        AobType type = AobType::Address;

        AOB() = default;

        /// Construct an AOB in the .text section of the main module, specified using a Cheat-Engine like syntax.
        AOB(const std::string& ce_aob, intptr_t offset = 0, AobType type = AobType::Address) : offset(offset), type(type)
        {
            ParseCEAob(ce_aob);
        }

        /// Construct an AOB in the .text section of the main module, specified using a Cheat-Engine like syntax.
        AOB(const std::string& ce_aob, AobType type) : type(type)
        {
            ParseCEAob(ce_aob);
        }

        /// Construct an AOB in the given search region, specified using a Cheat-Engine like syntax.
        AOB(const MemRegion& search_region, const std::string& ce_aob, intptr_t offset = 0, AobType type = AobType::Address) :
            search_region(search_region), offset(offset), type(type)
        {
            ParseCEAob(ce_aob);
        }

        /// Construct an AOB in the .text section of the main module, specified using a Cheat-Engine like syntax.
        AOB(const MemRegion& search_region, const std::string& ce_aob, AobType type) :
            search_region(search_region), type(type)
        {
            ParseCEAob(ce_aob);
        }

    private:
        static char hexit2num(char h) {
            if (h >= '0' && h <= '9') return h - '0';
            else if (h >= 'a' && h <= 'f') return 10 + h - 'a';
            else if (h >= 'A' && h <= 'F') return 10 + h - 'A';
            else return -1;
        }

        void ParseCEAob(const std::string& ce_aob)
        {
            bytes.clear();
            mask.clear();

            for (const char* c = ce_aob.c_str(); c[0]; c++) {
                if (c[0] == ' ') continue;
                else if (!c[1] || c[1] == ' ') {
                    char b = hexit2num(c[0]);
                    if (b < 0 && c[0] != '?')
                        // Logger should always be available
                        Logger::Get()->Warn("AOB", "Unrecognized character '{}' in AOB \"{}\"", c[0], ce_aob);
                    bytes.push_back(b);
                    mask.push_back(b == -1 ? 0 : 0xff);
                }
                else if (c[1]) {
                    char c1 = hexit2num(c[0]), c2 = hexit2num(c[1]);
                    if (c1 < 0 && c[0] != '?')
                        Logger::Get()->Warn("AOB", "Unrecognized character '{}' in AOB \"{}\"", c[0], ce_aob);
                    if (c2 < 0 && c[1] != '?')
                        Logger::Get()->Warn("AOB", "Unrecognized character '{}' in AOB \"{}\"", c[1], ce_aob);

                    bytes.push_back(std::max(c1, (char)0) << 4 | std::max(c2, (char)0));
                    mask.push_back((c1 < 0 ? 0 : 0xf0) | (c2 < 0 ? 0 : 0xf));
                    c++;
                }
            }
        }
    };

    /// AOB data free of STL types, to be passed to the interface
    struct CAob
    {
        MemRegion search_region;
        const uint8_t* bytes;
        const uint8_t* mask;
        size_t size;
        intptr_t offset;
        AobType type;

        explicit CAob(const AOB& aob) {
            search_region = aob.search_region;
            bytes = aob.bytes.data();
            mask = aob.mask.data();
            size = aob.bytes.size();
            offset = aob.offset;
            type = aob.type;
        }
    };

    struct AobScanResult {
        CAob* aob;
        size_t num_results;
        uintptr_t* addresses;

        void FreeAddresses()
        {
            if (deallocator && addresses) {
                deallocator(addresses);
                addresses = nullptr;
            }
        }

    private:
        friend class AobScanManImp;
        void (*deallocator)(void*); // Deallocator will be nullptr when not manually allocated
    };

    /// Cached, efficient AOB scanner. Can scan for memory immediately or batch AOBs to be efficiently scanned
    /// in one step once component loading is complete.
    class AobScanner : public SharedInterface<AobScanner, "MCF_AOB_SCANNER_001">
    {
    public:
        // TODO: Provide more useful information here (though the main use of this event would just be waiting for all AOBs to be initialized
        struct BatchScanCompleteEvent : Event<"MCF_AOB_SCAN_EVT_001">
        {
            /// Number of AOBs that were registered for scanning.
            uintptr_t num_registered;
        };

        /// Register an AOB to be scanned once all components are loaded or after a set delay (100ms). This allows the
        /// scanner to use an efficient DFA based algorithm (Aho-Corasick) to scan all AOBs simultaneously.
        /// \param aob The AOB to scan.
        /// \param call_result The call result whose callback will be executed once the aob is found (or not). Argument is a AobScanResult.
        /// \return true if the AOB was successfully registered; false otherwise.
        virtual bool RegisterAob(const CAob* aob, CallResultBase* call_result) = 0;

        /// Scan for an AOB immediately and return the results.
        /// NOTE: You *MUST* call the FreeAddresses() method of the scan_result after use to avoid leaking memory.
        /// \param aob the AOB to scan.
        /// \param scan_result Structure containing the scan results.
        virtual void AobScan(const CAob* aob, AobScanResult* scan_result) = 0;

        /// Scan for an AOB immediately and return the first result, or 0 if it could not be found.
        /// \param aob The AOB to scan.
        /// \return The first address matching the AOB, if found. 0 otherwise.
        virtual uintptr_t AobScanUnique(const CAob* aob) = 0;

        /// Register an AOB to be scanned once all components are loaded or after a set delay (100ms). This allows the
        /// scanner to use an efficient DFA based algorithm (Aho-Corasick) to scan all AOBs simultaneously.
        /// \param aob The AOB to scan.
        /// \param call_result The call result whose callback will be executed once the aob is found (or not). Argument is a AobScanResult.
        /// \return true if the AOB was successfully registered; false otherwise.
        inline bool RegisterAob(const AOB& aob, CallResultBase* call_result)
        {
            CAob caob(aob);
            return RegisterAob(&caob, call_result);
        }

        /// Scan for an AOB immediately and return the results.
        /// NOTE: You *MUST* call the FreeAddresses() method of the scan_result after use to avoid leaking memory.
        /// \param aob the AOB to scan.
        /// \param scan_result Structure containing the scan results.
        inline void AobScan(const AOB& aob, AobScanResult* scan_result)
        {
            CAob caob(aob);
            return AobScan(&caob, scan_result);
        }

        /// Scan for an AOB immediately and return the first result, or 0 if it could not be found.
        /// \param aob The AOB to scan.
        /// \return The first address matching the AOB, if found. 0 otherwise.
        virtual uintptr_t AobScanUnique(const AOB& aob)
        {
            CAob caob(aob);
            return AobScanUnique(&caob);
        }
    };

    /// RAII object representing a pointer to an object in memory found through AOB scanning.
    /// \tparam TAddress The underlying address type. Can be an integral or pointer type.
    /// \tparam DebugName Optional compile-time debug name which will be used to identify the AOB in warnings when not found / duplicated.
    template<class TAddress, FixedString DebugName = "Unnamed"> requires (sizeof(TAddress) == sizeof(void*)) ||
        (std::is_pointer_v<TAddress> || std::is_integral_v<TAddress>)
    class AobScannedPtr : private NonAssignable
    {
    private:
        Dependency<AobScanner> aobScanMan;

        CallResult<AobScanResult> cr_scan_result = [this](AobScanResult* scan_result) {
            Dependency<Logger> logger;
            if (scan_result->num_results > 1) {
                if (logger) logger->Warn("AutoUniqueAob", "Duplicate results for AOB \"{}\"", DebugName.buf);
            }
            if (scan_result->num_results == 0) {
                if (logger) logger->Warn("AutoUniqueAob", "AOB \"{}\" not found", DebugName.buf);
            }
            else {
                found = true;
                unique = scan_result->num_results == 1;
                addr = (TAddress)scan_result->addresses[0];
            }
        };

    public:
        AOB aob;
        TAddress addr = (TAddress)0;
        bool found = false;
        bool unique = false;

        explicit AobScannedPtr(const AOB& aob) : aob(aob)
        {
            if (aobScanMan) aobScanMan->RegisterAob(aob, &cr_scan_result);
        }

        AobScannedPtr(AobScanResult const&) = delete;

        TAddress operator->() {
            return addr;
        }

        operator TAddress() const { return addr; }
    };

    /// Wrapper around AobScannedPtr for a function pointer that can be called directly.
    /// \tparam TFun The function type. Must be a function pointer. Pass void(__stdcall *)(int), not void __stdcall(int)
    /// \tparam DebugName Optional compile-time debug name which will be used to identify the AOB in warnings when not found / duplicated.
    template<class TFun, FixedString DebugName = "Unnamed"> requires std::is_function_v<TFun>
    class AobScannedFunction;

#ifndef _WIN64
    template<FixedString DebugName, class Ret, class... Args>
    class AobScannedFunction<Ret (__stdcall*)(Args...), DebugName> : public AobScannedPtr<Ret(__stdcall*)(Args...), DebugName>
    {
    public:
        AobScannedFunction(const AOB& aob) : AobScannedPtr<__stdcall Ret (*)(Args...), DebugName>(aob) { }

        Ret operator()(Args... args)
        {
            if constexpr (std::is_same_v<Ret, void>) this->addr(args...);
            else return this->addr(args...);
        }
    };
    template<FixedString DebugName, class Ret, class... Args>
    class AobScannedFunction<Ret (__cdecl*)(Args...), DebugName> : public AobScannedPtr<Ret(__cdecl*)(Args...), DebugName>
    {
    public:
        AobScannedFunction(const AOB& aob) : AobScannedPtr<__cdecl Ret (*)(Args...), DebugName>(aob) { }

        Ret operator()(Args... args)
        {
            if constexpr (std::is_same_v<Ret, void>) this->addr(args...);
            else return this->addr(args...);
        }
    };
    template<FixedString DebugName, class Ret, class... Args>
    class AobScannedFunction<Ret (__thiscall*)(Args...), DebugName> : public AobScannedPtr<Ret(__thiscall*)(Args...), DebugName>
    {
    public:
        AobScannedFunction(const AOB& aob) : AobScannedPtr<__thiscall Ret (*)(Args...), DebugName>(aob) { }

        Ret operator()(Args... args)
        {
            if constexpr (std::is_same_v<Ret, void>) this->addr(args...);
            else return this->addr(args...);
        }
    };
#endif

    template<FixedString DebugName, class Ret, class... Args>
    class AobScannedFunction<Ret (__fastcall*)(Args...), DebugName> : public AobScannedPtr<Ret(__fastcall*)(Args...), DebugName>
    {
    public:
        explicit AobScannedFunction(const AOB& aob) : AobScannedPtr<__fastcall Ret (*)(Args...), DebugName>(aob) { }

        Ret operator()(Args... args)
        {
            if constexpr (std::is_same_v<Ret, void>) this->addr(args...);
            else return this->addr(args...);
        }
    };
};