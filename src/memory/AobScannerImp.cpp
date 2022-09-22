
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "AobScannerImp.h"

namespace MCF
{
    bool AobScannerImp::RegisterAob(const CAob* aob, CallResultBase* call_result)
    {
        return false;
    }

    void AobScannerImp::AobScan(const CAob* aob, AobScanResult* scan_result)
    {

    }

    uintptr_t AobScannerImp::AobScanUnique(const CAob* aob)
    {
        return 0;
    }
} // MCF