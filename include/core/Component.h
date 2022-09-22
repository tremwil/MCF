
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "FixedString.h"
#include <cstdint>
#include <type_traits>

namespace MCF
{
    struct ComponentFactory;

    /// Interface for components, the base objects managed automatically by MCF.
    class IComponent
    {
    protected:
        virtual ~IComponent() = default;

    public:
        /// Get the unique version string of this component.
        virtual const char* VersionString() const = 0;

        /// true if this component can be unloaded. True by default.
        /// Override this if you wish to specify your component cannot be unloaded at runtime.
        virtual bool IsUnloadable() const = 0;
    };
}

#ifdef MCF_EXPORTS
#define MCF_C_API __declspec(dllexport)
#else
#define MCF_C_API __declspec(dllimport)
#endif

/// Get a component by version string without incrementing its reference count.
/// Only safe to use with "unloadable" components.
extern "C" MCF_C_API MCF::IComponent * MCF_GetComponent(const char* version_string);

/// Acquire a component (or attempt to load it if currently loading modules), incrementing its reference count.
/// WARNING: If the component cannot be acquired, this method will throw an exception.
extern "C" MCF_C_API MCF::IComponent * MCF_AcquireComponent(const char* version_string);

/// Call when done using a component acquired via MCF_AcquireComponent to lower the reference count.
extern "C" MCF_C_API bool MCF_ReleaseComponent(MCF::IComponent * component);

namespace MCF
{
    /// RAII template for acquiring a component one depends on. Meant to be used in other RAII-style objects, not
    /// components. Components should use the DependsOn template parameter and pass a DependencyList<...>.
    template<typename T> requires std::is_base_of_v<IComponent, T>
    class Dependency
    {
    public:
        T* operator->() {
            return instance;
        }

        operator bool() const {
            return instance != nullptr;
        }

        Dependency() {
            instance = (T*)MCF_AcquireComponent(T::version_string);
        }

        ~Dependency() {
            MCF_ReleaseComponent(instance);
            instance = nullptr;
        }

    private:
        T* instance;
    };

    /// Template class used to store dependencies to multiple other components.
    template<class... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
    struct DependencyList
    {
        static constexpr size_t count = sizeof...(Components);
        static const char* version_strings[count == 0 ? 1 : count];
    };
    template<typename... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
    const char* DependencyList<Components...>::version_strings[] = { Components::version_string... };

    /// Get the index of D in the dependency list L at compile-time.
    template<class D, class L>
    struct DepListIndex
    {
        static constexpr int index = 0;
    };

    template<class D, class Head, class... Tail>
    struct DepListIndex<D, DependencyList<Head, Tail...>>
    {
        static constexpr bool index = std::is_same_v<D, Head> ? 0
        : 1 + DepListIndex<D, DependencyList<Tail...>>::index;
    };

    /// C struct providing functions for constructing and destroying a component.
    /// Passed to the component manager through MCF_GetExportedInterfaces.
    struct ComponentFactory
    {
        typedef IComponent* (* const NewOp)(bool* all_dependencies_loaded);
        typedef void (* const DeleteOp)(IComponent*);

        const char*  version_string = nullptr;

        ComponentFactory(const char* version_string, NewOp new_fun, DeleteOp delete_fun) :
                version_string(version_string),
                new_fun(new_fun),
                delete_fun(delete_fun) { }


    private:
        friend class ComponentManImp;

        /// Allocator + constructor of the component. A pointer to a boolean is passed as an argument.
        /// This boolean is false if a dependency failed to load, in which case the component
        /// will be immediately unloaded after the constructor returns.
        NewOp new_fun = nullptr;

        /// Destructor + deallocator of the component.
        DeleteOp delete_fun = nullptr;
    };

    /// Represents an object automatically managed by MCF. Provides automatic
    /// load order resolution to ensure resourced required by objects initialized
    /// on construction (i.e. events, hooks, etc) are available.
    /// \tparam T The type which will inherit from this template to implement functionality.
    /// \tparam version_str A unique version string for this component.
    /// \tparam DependsOn RAII list of dependencies. You can also define Dependency instances as fields directly.
    /// \tparam Interface Base class of this Component. Should not be changed directly.
    template<class T, FixedString version_str, class DependsOn = DependencyList<>, class Interface = IComponent>
    requires std::is_base_of_v<IComponent, Interface>
    class Component : public Interface
    {
    private:
        static IComponent* OpNew(bool* all_dependencies_loaded) { return new T(all_dependencies_loaded); }
        static void OpDelete(IComponent* obj) { delete (T*)obj; }

        template<typename S>
        struct DepArray { };

        template<typename... Deps>
        struct DepArray<DependencyList<Deps...>>
        {
            IComponent* deps[sizeof...(Deps) == 0 ? 1 : sizeof...(Deps)] = { MCF_AcquireComponent(Deps::version_string)... };

            ~DepArray() {
                for (int i = 0; i < sizeof...(Deps); i++)
                    MCF_ReleaseComponent(deps[i]);
            }
        };

        DepArray<DependsOn> dep_list{};

    protected:
        /// Efficient access to a dependency. Prefer this helper function to
        /// D::Get(), as it avoids unnecessary calls to MCF_GetComponent.
        template<class D> requires (DepListIndex<D, DependsOn>::index < DependsOn::count)
        D* Dep()
        {
            constexpr int i = DepListIndex<D, DependsOn>::index;
            return (D*)dep_list.deps[i];
        }

        Component() = default;

    public:
        Component(Component&) = delete;
        Component(Component&&) = delete;

        static constexpr FixedString version_string = version_str;

        /// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet.
        /// Consider using Dependency objects instead.
        static inline T* Get()
        {
            return (T*)MCF_GetComponent(version_string);
        }

        /// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet.
        /// This increments the reference count to this component, so it will not be able to be freed
        /// while you are using it. Consider using Dependency objects instead.
        static inline T* Acquire()
        {
            return (T*)MCF_AcquireComponent(version_string);
        }

        static const ComponentFactory* Factory()
        {
            static const MCF::ComponentFactory f(version_str, &OpNew, &OpDelete);
            return &f;
        }

        const char* VersionString() const override
        {
            return version_string;
        }

        bool IsUnloadable() const override
        {
            return true;
        }
    };
}