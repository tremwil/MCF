
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "Component.h"

namespace MCF
{
    /// Template class used to define singleton-like virtual method interfaces that are designed
    /// to be used by different mods. Similar in design to Steam API interfaces. This class should
    /// be inherited by the interface class for TInterface, whose header should be made available for
    /// other mods to use. The actual implementation should inherit from the SharedInterfaceImp template.
    template<class TInterface, FixedString version_str>
    class SharedInterface : public IComponent
    {
    public:
        static constexpr FixedString version_string = version_str;

        /// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet.
        /// Consider using Dependency objects instead.
        static inline TInterface* Get()
        {
            return (TInterface*)MCF_GetComponent(version_string);
        }

        /// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet.
        /// This increments the reference count to this component, so it will not be able to be freed
        /// while you are using it. Consider using Dependency objects instead.
        static inline TInterface* Acquire()
        {
            return (TInterface*)MCF_AcquireComponent(version_string);
        }

        /// Release this component, decrementing its reference count. Consider using Dependency objects instead.
        static inline void Release()
        {
            return MCF_ReleaseComponent(version_string);
        }
    };

    /// Implementation of a SharedInterface.
    /// \tparam Interface The abstract interface we are implementing.
    /// \tparam Implementation The implementation type. Should be the class that inherits this.
    /// \tparam DependsOn List of dependencies, i.e. other IComponents which must be constructed and loaded before this one.
    template<class Interface, class Implementation, class DependsOn = DependencyList<>>
    using SharedInterfaceImp = Component<Implementation, Interface::version_string, DependsOn, Interface>;
}