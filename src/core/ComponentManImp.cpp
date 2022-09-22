
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "ComponentManImp.h"
#include "EventManImp.h"
#include "LoggerImp.h"

using namespace std::chrono_literals;

namespace MCF
{
    bool ComponentManImp::InitializeCore()
    {
        auto self_cinfo = std::make_shared<ComponentInfo>();
        self_cinfo->factory = Factory();
        self_cinfo->instance = this;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(self_cinfo->factory), &self_cinfo->dll_handle);
        self_cinfo->instantiation_initiated = true;
        self_cinfo->load_result = LoadResult::Success;

        components[version_string.buf] = self_cinfo;
        components_by_dll[self_cinfo->dll_handle].insert(self_cinfo);

        const ComponentFactory* core[] = {
                EventManImp::Factory(),
                LoggerImp::Factory()
        };

        core_initialized = LoadComponents(core, sizeof(core) / sizeof(void*));
        return core_initialized;
    }

    bool ComponentManImp::RecursiveComponentLoad(std::shared_ptr<ComponentInfo>& cinfo)
    {
        Log(Logger::SevDebug, R"(Recursive load {})", cinfo->factory->version_string);

        load_stack.push(cinfo);
        cinfo->load_success = true;
        cinfo->instantiation_initiated = true;

        Log(Logger::SevDebug, "Attempting to instantiate {}", cinfo->factory->version_string);
        cinfo->instance = cinfo->factory->new_fun(&cinfo->load_success);

        if (cinfo->load_success) {
            cinfo->load_result = LoadResult::Success;
            Log(Logger::SevDebug, "Successfully instantiated {}", cinfo->factory->version_string);
        }
        else {
            if (cinfo->load_result == LoadResult::CircularDependency)
                Log(Logger::SevError, "Component \"{}\" depends or is part of a cycle of dependencies",
                    cinfo->factory->version_string);

            else if (cinfo->load_result == LoadResult::DependencyNotFound)
                Log(Logger::SevError, "Component \"{}\" depends on a non-existing component",
                    cinfo->factory->version_string);

            cinfo->factory->delete_fun(cinfo->instance);
            cinfo->instance = nullptr;
        }

        load_stack.pop();
        return cinfo->load_success;
    }

    IComponent* ComponentManImp::GetComponent(const char* version_string)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (!components.contains(version_string))
            return nullptr;

        return components[version_string]->instance;
    }

    IComponent* ComponentManImp::AcquireComponent(const char* version_string)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        Log(Logger::SevDebug, R"(Acquire {})", version_string);

        if (!components.contains(version_string)) {
            if (!load_queue.empty() && !load_stack.empty()) {
                Log(Logger::SevError, R"(Tried to load non-existing component "{}")", version_string);

                auto& dependent = load_stack.top();
                dependent->load_success = false;
                if (dependent->load_result == LoadResult::None)
                    dependent->load_result = LoadResult::DependencyNotFound;
            }
            return nullptr;
        }
        auto& cinfo = components[version_string];
        if (!cinfo->instance) {
            Log(Logger::SevDebug, R"(Instantiate {})", version_string);

            // Second call when constructing the same component -> circular dependency
            if (cinfo->load_result != LoadResult::None) {
                Log(Logger::SevError, "Dependency on component \"{}\" which failed to load",version_string);
                components[version_string]->load_result = LoadResult::DependencyFailedToLoad;
                load_stack.top()->load_success = false;
                load_stack.top()->load_result = LoadResult::DependencyFailedToLoad;
                return nullptr;
            }
            else if (cinfo->instantiation_initiated) {
                Log(Logger::SevError, "Cycle in dependencies for component \"{}\"",version_string);
                components[version_string]->load_result = LoadResult::CircularDependency;
                load_stack.top()->load_success = false;
                load_stack.top()->load_result = LoadResult::CircularDependency;
                return nullptr;
            }
            if (!RecursiveComponentLoad(cinfo))
                return nullptr;
        }

        if (!load_stack.empty()) {
            load_stack.top()->dependencies.insert(cinfo);
            cinfo->dependents.insert(load_stack.top());
        }

        if (load_stack.empty()) release_mutex.lock();
        cinfo->ref_count++;
        if (load_stack.empty()) release_mutex.unlock();

        return cinfo->instance;
    }

    bool ComponentManImp::ReleaseComponent(IComponent* component)
    {
        if (component == nullptr)
            return false;

        std::lock_guard<decltype(release_mutex)> release_lock(release_mutex);

        auto vstr = component->VersionString();
        Log(Logger::SevDebug, "Release component {}", vstr);

        if (!components.contains(vstr)) {
            if (load_stack.empty()) Log(Logger::SevWarn, "Tried to release non-existing component \"{}\"",vstr);
            return false;
        }

        auto& cinfo = components[vstr];
        if (!cinfo->load_success) return false;

        if (cinfo->ref_count == 0) {
            Log(Logger::SevWarn, "Tried to release component \"{}\" which has zero reference count",vstr);
            return false;
        }

        if (--(cinfo->ref_count) == 0)
            cinfo->ref_cv.notify_one();

        return true;
    }

    bool ComponentManImp::LoadComponents(const ComponentFactory* comps[], size_t count)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        std::lock_guard<decltype(release_mutex)> release_lock(release_mutex);

        if (!load_queue.empty() || is_unloading)
            return false;

        if (core_initialized)
            EventMan::Get()->RaiseEvent(LoadBeginEvent{ .to_load = comps, .count = count });

        std::vector<LoadResult> out_load_results(count);

        // Setup component graph
        for (int i = 0; i < count; i++) {
            if (components.contains(comps[i]->version_string)) {
                Log(Logger::SevError, "Component with unique version string \"{}\" already exists", comps[i]->version_string);
                out_load_results[i] = LoadResult::NameConflict;
                continue;
            }

            auto cinfo = std::make_shared<ComponentInfo>();
            cinfo->factory = comps[i];
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(comps[i]), &cinfo->dll_handle);

            components[comps[i]->version_string] = cinfo;
            components_by_dll[cinfo->dll_handle].insert(cinfo);
            load_queue.push(cinfo);
        }

        // Load components
        for (; !load_queue.empty(); load_queue.pop()) {
            auto cinfo = load_queue.front();
            if (cinfo->load_result != LoadResult::None)
                continue;

            RecursiveComponentLoad(cinfo);
        }

        // Build dependent map and load complete event, cleaning up unused ComponentInfos in the process
        for (int i = 0; i < count; i++) {
            if (components.contains(comps[i]->version_string)) {
                auto& cinfo = components[comps[i]->version_string];
                out_load_results[i] = cinfo->load_result;
                if (!cinfo->instance) {
                    for (const auto& dep : cinfo->dependencies)
                        dep->dependents.erase(cinfo);

                    components_by_dll.erase(cinfo->dll_handle);
                    components.erase(comps[i]->version_string);
                }
            }
        }

        bool all_success = std::all_of(
                out_load_results.begin(),
                out_load_results.end(),
                [](auto x) { return x == LoadResult::Success; }
        );

        if (core_initialized) {
            EventMan::Get()->RaiseEvent(LoadCompleteEvent{
                    .batch = comps,
                    .results = out_load_results.data(),
                    .count = count
            });
        }

        return all_success;
    }

    void ComponentManImp::UnloadComponents(const char* comps[], size_t count, bool unload_deps)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (!load_queue.empty() || is_unloading) return;
        std::unique_lock<decltype(release_mutex)> release_lock(release_mutex);

        is_unloading = true;

        EventMan::Get()->RaiseEvent(UnloadBeginEvent{
            .version_strings = comps,
            .count = count,
            .unload_deps = unload_deps });

        std::unordered_set<std::shared_ptr<ComponentInfo>> existing;
        std::unordered_map<std::string, UnloadResult> res_map;

        // Build hashset of existing unloadable components
        for (int i = 0; i < count; i++) {
            if (!components.contains(comps[i])) {
                Log(Logger::SevError, "Cannot unload component \"{}\"; it is not currently loaded", comps[i]);
                res_map[comps[i]] = UnloadResult::NameNotFound;
            }
            else existing.insert(components[comps[i]]);
        }

        // Clear visited flag for DFS
        for (const auto& [vstr, cinfo] : components)
            cinfo->unload_visited = false;

        std::stack<std::shared_ptr<ComponentInfo>> unload_stack;
        while (!unload_stack.empty()) {
            auto& cinfo = unload_stack.top();
            if (cinfo->unload_visited && cinfo->unload_check_done) {
                unload_stack.pop();
            }
            else if (!unload_deps && !existing.contains(cinfo)) {
                cinfo->unload_visited = true;
                Log(Logger::SevError, "Cannot unload component \"{}\" as unload_deps is false",
                    cinfo->factory->version_string);
                cinfo->unload_check_done = true;
                unload_stack.pop();
            }
            else if (!cinfo->instance->IsUnloadable()) {
                cinfo->unload_visited = true;
                Log(Logger::SevError, "Cannot unload component \"{}\"; it is marked as non-unloadable",
                    cinfo->factory->version_string);
                res_map[cinfo->factory->version_string] = UnloadResult::IsNotUnloadable;
                cinfo->unload_check_done = true;
                unload_stack.pop();
            }
            // Component visited but dependents are still loaded, so something failed to unload
            else if (cinfo->unload_visited && !cinfo->dependents.empty()) {
                Log(Logger::SevError, "Cannot unload component \"{}\"; it is required by a non-unloadable component",
                    cinfo->factory->version_string);
                res_map[cinfo->factory->version_string] = UnloadResult::HasDependentComponent;
                cinfo->unload_check_done = true;
                unload_stack.pop();
            }
            // Component ready to unload, wait for cv and unload
            else if (cinfo->unload_visited) {
                // TODO: Get timeout from core settings
                if (cinfo->ref_count > 0 && cinfo->ref_cv.wait_for(release_lock, 100ms) == std::cv_status::timeout) {
                    Log(Logger::SevError, "Cannot unload component \"{}\"; Timeout while waiting for references to expire",
                        cinfo->factory->version_string);
                    res_map[cinfo->factory->version_string] = UnloadResult::ReferenceStillHeld;
                    cinfo->unload_check_done = true;
                    unload_stack.pop();
                    continue;
                }
                // Component is not used by anything, we can safely unload it
                cinfo->factory->delete_fun(cinfo->instance);
                cinfo->instance = nullptr;
                res_map[cinfo->factory->version_string] = UnloadResult::Success;

                Log(Logger::SevDebug, "Unloaded component \"{}\"", cinfo->factory->version_string);

                for (const auto& dep: cinfo->dependencies)
                    dep->dependents.erase(cinfo);

                components_by_dll[cinfo->dll_handle].erase(cinfo);
                components.erase(cinfo->factory->version_string);
                cinfo->unload_check_done = true;
                unload_stack.pop();
            }
            else {
                // First time visiting this component, add its dependents to the unload stack
                cinfo->unload_visited = true;
                cinfo->unload_check_done = false;
                for (const auto& dep : cinfo->dependents) {
                    unload_stack.push(dep);
                }
            }
        }

        existing.clear();

        // Pack unload results for sending as event
        std::vector<const char*> out_vstrs;
        std::vector<UnloadResult> out_results;

        for (const auto& [vstr, res] : res_map) {
            out_vstrs.push_back(vstr.c_str());
            out_results.push_back(res);
        }

        is_unloading = false;

        EventMan::Get()->RaiseEvent(UnloadCompleteEvent{
            .version_strings = out_vstrs.data(),
            .results = out_results.data(),
            .count = out_vstrs.size() });
    }

    bool ComponentManImp::LoadDlls(const char* dll_names[], size_t count)
    {
        std::vector<const ComponentFactory*> to_load;
        for (size_t i = 0; i < count; i++)
        {
            HMODULE hmod = GetModuleHandleA(dll_names[i]);
            if (hmod == nullptr) hmod = LoadLibraryA(dll_names[i]);
            if (hmod == nullptr)
            {
                Log(Logger::SevError, "DLL with name \"{}\" could not be found", dll_names[i]);
                continue;
            }
            auto cinfo_getter = (GetExportedComponents_t)GetProcAddress(hmod, "MCF_GetExportedComponents");
            if (cinfo_getter == nullptr)
            {
                Log(Logger::SevError, "DLL with name {} does not export MCF_GetExportedComponents", dll_names[i]);
                FreeLibrary(hmod);
                continue;
            }

            size_t comp_count = 0;
            auto comp_arr = cinfo_getter(&comp_count);

            for (size_t j = 0; j < comp_count; j++)
                to_load.push_back(comp_arr[j]);
        }

        return LoadComponents(to_load.data(), to_load.size());
    }

    void ComponentManImp::UnloadDlls(const char* dll_names[], size_t count, bool unload_deps)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        std::vector<HMODULE> dlls_to_unload;
        std::vector<const char*> comps_to_unload;
        for (size_t i = 0; i < count; i++)
        {
            HMODULE hmod = GetModuleHandleA(dll_names[i]);
            if (hmod == nullptr)
            {
                Log(Logger::SevError, "DLL with name \"{}\" could not be found", dll_names[i]);
                continue;
            }
            if (components_by_dll.contains(hmod) && !components_by_dll[hmod].empty()) {
                for (const auto& cinfo : components_by_dll[hmod])
                    comps_to_unload.push_back(cinfo->factory->version_string);

                dlls_to_unload.push_back(hmod);
            }
        }

        UnloadComponents(comps_to_unload.data(), comps_to_unload.size(), unload_deps);
        for (const auto& hmod : dlls_to_unload)
            if (components_by_dll[hmod].empty()) FreeLibrary(hmod);
    }
}