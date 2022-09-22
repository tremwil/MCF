
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/memory/AobScan.h"

namespace MCF
{

    class AobScannerImp : public SharedInterfaceImp<AobScanner, AobScannerImp>
    {
    public:
        AobScannerImp(bool* success) { }

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