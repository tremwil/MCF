
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "core/ComponentMan.h"
#include "core/Logger.h"
#include "include/third-party/fmt/core.h"

#include "include/windows_include.h"

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>

#include <mutex>
#include <condition_variable>

namespace MCF
{
    class ComponentManImp final : public SharedInterfaceImp<ComponentMan, ComponentManImp>
    {
    private:
        typedef const MCF::ComponentFactory** (*GetExportedComponents_t)(size_t* n);

        struct ComponentInfo
        {
            /// Components on which this one depends on (only those acquired during component construction)
            std::unordered_set<std::shared_ptr<ComponentInfo>> dependencies;
            /// Components which this one depends on (only when acquired during component construction)
            std::unordered_set<std::shared_ptr<ComponentInfo>> dependents;

            const ComponentFactory* factory = nullptr;
            HMODULE dll_handle = nullptr;
            IComponent* instance = nullptr;

            bool instantiation_initiated = false;
            bool load_success = false;
            LoadResult load_result = LoadResult::None;

            bool unload_visited = false;
            bool unload_check_done = false;

            int32_t ref_count = 0;
            std::condition_variable ref_cv;
        };

        std::unordered_map<HMODULE, std::unordered_set<std::shared_ptr<ComponentInfo>>> components_by_dll;
        std::unordered_map<std::string, std::shared_ptr<ComponentInfo>> components;

        std::queue<std::shared_ptr<ComponentInfo>> load_queue;
        std::stack<std::shared_ptr<ComponentInfo>> load_stack;

        std::recursive_mutex mutex; // Mutex for general access
        std::mutex release_mutex; // Mutex for releasing a component

        bool core_initialized = false;
        bool is_unloading = false;

        template<class Fmt, class... Args>
        inline void Log(const char* sev, Fmt format, Args&&... args) {
            if (core_initialized)
                Logger::Get()->Log(this, sev, format, args...);
            else {
                auto msg = fmt::vformat(format, fmt::make_format_args(args...));
                fmt::print("[{}] [{}] {}\n", version_string, sev, msg);
            }
        }

    public:
        ComponentManImp(bool* success) { };

        bool InitializeCore();

        bool RecursiveComponentLoad(std::shared_ptr<ComponentInfo>& cinfo);

        /// Get the instance of a particular component by its unique version string.
        /// This does NOT increase the reference count of the component, so only use if
        /// you know that the component will not be able to be unloaded while you are
        /// using it.
        /// \return The requested component, or a null pointer if the component could not be found
        IComponent* GetComponent(const char* version_string) override;

        /// Get the instance to a particular component by its unique version string.
        /// Increment the component's reference count, so that it cannot be freed while
        /// you are using it. This should only be used if you cannot make the component
        /// a dependency.
        /// \return The requested component, or a null pointer if the component could not be found.
        IComponent* AcquireComponent(const char* version_string) override;

        /// Release a particular component, decrementing its reference count.
        /// <param name="version_string"></param>
        /// <returns></returns>
        bool ReleaseComponent(MCF::IComponent* component) override;

        /// Load a set of components.
        bool LoadComponents(const ComponentFactory* comps[], size_t count) override;

        /// Unload a set of components by version string.
        /// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
        void UnloadComponents(const char* comps[], size_t count, bool unload_deps) override;

        /// Load all the components exported by a set of DLLs.
        bool LoadDlls(const char* dll_names[], size_t count) override;

        /// Unload all the components exported by the given DLLs.
        /// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
        void UnloadDlls(const char* dll_names[], size_t count, bool unload_deps) override;
    };
}