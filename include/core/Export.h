
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "SharedInterface.h"
#include <vector>
#include <typeinfo>

namespace MCF
{
    class AutoExportBase
    {
    protected:
        static std::vector<const ComponentFactory*> factories;

    public:
        static const MCF::ComponentFactory** Export(size_t* n)
        {
            *n = AutoExportBase::factories.size();
            return AutoExportBase::factories.data();
        }
    };

    // Class template which can be used to automatically register a component
    // on boot (if you add Export.cpp to your project). If you prefer to
    // do things manually, you will have to define MCF_GetExportedComponents yourself.
    template<typename T>
    struct AutoExport : public AutoExportBase
    {
    private:
        AutoExport()
        {
            const ComponentFactory* factory = T::Factory();
            AutoExportBase::factories.push_back(factory);
        }
        static int AddOnce()
        {
            static AutoExport ex;
            return 0;
        }

        static int ex;
    };
}
#define MCF_COMPONENT_EXPORT(T) template<> int MCF::AutoExport<T>::ex = MCF::AutoExport<T>::AddOnce();

extern "C" __declspec(dllexport) const MCF::ComponentFactory** MCF_GetExportedComponents(size_t* n);