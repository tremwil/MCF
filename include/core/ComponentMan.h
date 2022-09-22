
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "EventMan.h"

namespace MCF
{
    /// The component manager. Responsible for loading/unloading components provided by different DLLs.
    class ComponentMan : public SharedInterface<ComponentMan, "MCF_COMPONENT_MAN_001">
    {
    public:
        enum class LoadResult : int32_t
        {
            /// No attempt was made to load the component yet. For internal use.
            None = 0,
            /// Component was loaded successfully.
            Success = 1,
            /// There is a component with the same name already loaded.
            NameConflict = 2,
            /// The component has a dependency which could not be found.
            DependencyNotFound = 3,
            /// The component was part of a circular dependency.
            CircularDependency = 4,
            /// The component has a dependency which failed to load.
            DependencyFailedToLoad = 5
        };

        /// Event raised when a batch component load has begun.
        struct LoadBeginEvent : public Event<"MCF_CM_LOAD_BEGIN_EVENT">
        {
            const ComponentFactory** to_load;
            size_t count;
        };

        /// Event raised when a batch component load has completed.
        struct LoadCompleteEvent : public Event<"MCF_CM_LOAD_COMPLETE_EVENT">
        {
            const ComponentFactory** batch;
            const LoadResult* results;
            size_t count;
        };

        enum class UnloadResult : int32_t
        {
            /// Component was unloaded successfully.
            Success = 0,
            /// Provided version string could not be found.
            NameNotFound = 1,
            /// Component has a dependent component which cannot be unloaded, and as such cannot be freed.
            HasDependentComponent = 2,
            /// A non-component resource was still a holding a reference to this component after the timeout period.
            ReferenceStillHeld = 3,
            /// Component was marked as not unloadable.
            IsNotUnloadable = 4
        };

        /// Event raised when an unload operation has begun.
        struct UnloadBeginEvent : public Event<"MCF_CM_UNLOAD_BEGIN_EVENT">
        {
            const char** version_strings;
            size_t count;
            bool unload_deps;
        };

        /// Event raised when an unload operation has completed.
        struct UnloadCompleteEvent : public Event<"MCF_CM_UNLOAD_BEGIN_EVENT">
        {
            const char** version_strings;
            UnloadResult* results;
            size_t count;
        };

        /// Get the instance of a particular component by its unique version string.
        /// This does NOT increase the reference count of the component, so only use if
        /// you know that the component will not be able to be unloaded while you are 
        /// using it.
        /// \return The requested component, or a null pointer if the component could not be found.
        virtual IComponent* GetComponent(const char* version_string) = 0;

        /// Get the instance to a particular component by its unique version string.
        /// Increment the component's reference count, so that it cannot be freed while
        /// you are using it. This should only be used if you cannot make the component
        /// a dependency.
        virtual IComponent* AcquireComponent(const char* version_string) = 0;

        /// Release a particular component, decrementing its reference count.
        virtual bool ReleaseComponent(IComponent* component) = 0;

        /// Load a set of components. NOTE: Cannot be called recursively. Will return false and not
        /// load anything if you try to do so.
        virtual bool LoadComponents(const ComponentFactory* comps[], size_t count) = 0;

        /// Unload a set of components by version strings.
        /// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
        virtual void UnloadComponents(const char* comps[], size_t count, bool unload_deps) = 0;

        /// Load all the components exported by a set of DLLs. NOTE: Cannot be called recursively.
        /// Will return false and not load anything if you try to do so.
        virtual bool LoadDlls(const char* dll_names[], size_t count) = 0;

        /// Unload all the components exported by the given DLLs.
        /// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
        virtual void UnloadDlls(const char* dll_names[], size_t count, bool unload_deps) = 0;
    };
}