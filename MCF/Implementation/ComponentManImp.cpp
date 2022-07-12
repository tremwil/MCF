#include "ComponentManImp.h"
#include <stack>

namespace MCF
{
	IComponent* ComponentManImp::GetComponent(const char* version_string)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);

		if (!components.count(version_string))
			return nullptr;

		return components[version_string]->instance;
	}

	IComponent* ComponentManImp::AcquireComponent(const char* version_string)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);

		if (!components.count(version_string))
			return nullptr;

		ref_counts++;
		return components[version_string]->instance;
	}

	void ComponentManImp::ReleaseComponent(const char* version_string)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		if (!components.count(version_string)) return;

		DepGraphNode* node = components[version_string];
		if (--ref_counts < 0)
		{
			C<Logger>()->Warn(this, "Got negative ref count while attempting to release component \"{}\"", node->comp_info->version_string);
			ref_counts = 0;
		};
		ref_cv.notify_one();
	}

	void ComponentManImp::LoadComponents(const CompInfo* comps[], size_t count)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);

		C<EventMan>()->RaiseEvent(LoadBeginEvent{ .to_load = comps, .count = count });

		std::unordered_map<std::string, const CompInfo*> curr_modules;
		std::stack<const CompInfo*> info_stack;
		std::stack<bool> visited_stack;

		std::vector<const CompInfo*> infos(count);
		std::vector<IComponent*> instances(count);
		std::vector<LoadResult> results(count);

		// First pass - Check for name conflicts
		for (size_t i = 0; i < count; i++)
		{
			const char* vstr = comps[i]->version_string;
			if (components.count(vstr) > 0 || curr_modules.count(vstr))
			{
				infos.push_back(comps[i]);
				instances.push_back(nullptr);
				results.push_back(LoadResult::NameConflict);
			}
			else
			{
				info_stack.push(comps[i]);
				visited_stack.push(false);
				curr_modules[vstr] = comps[i];
			}
		}

		// Second pass - DFS over component list to load modules in the correct order
		while (!info_stack.empty())
		{
			const CompInfo* comp = info_stack.top();
			const char* vstr = comp->version_string;
			bool visited = visited_stack.top();

			info_stack.pop();
			visited_stack.pop();

			// Component was already intialized -- do nothing
			if (components.count(vstr))
			{
				info_stack.pop();
				visited_stack.pop();
				continue;
			}
			
			// First pass -- check for errors
			LoadResult res = LoadResult::Success;
			for (size_t j = 0; j < comp->num_dependencies; j++)
			{
				const char* dep = comp->dependencies[j];
				if (!components.count(dep) || !curr_modules.count(dep))
				{
					res = LoadResult::DependencyNotFound;
					break;
				}
				else if (visited && !components.count(dep))
				{
					res = LoadResult::CircularDependency;
					break;
				}
			}
			// If failed, add failure to results and remove from stack
			if (res != LoadResult::Success)
			{
				infos.push_back(comp);
				instances.push_back(nullptr);
				results.push_back(res);
				curr_modules.erase(vstr);
			}
			// If succeeded but not visited, scan dependencies first (DFS)
			else if (!visited)
			{
				info_stack.push(comp);
				visited_stack.push(true);

				for (int j = 0; j < comp->num_dependencies; j++)
				{
					info_stack.push(curr_modules[comp->dependencies[j]]);
					visited_stack.push(false);
				}
			}
			// Else create graph node, init component and append to the main list
			else
			{
				auto node = new DepGraphNode;
				node->comp_info = comp;
				node->instance = comp->new_fun();

				GetModuleHandleExA(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					(LPCSTR)comp, &node->dll_handle
				);
				dll_ref_counts[node->dll_handle]++;

				for (size_t j = 0; j < comp->num_dependencies; j++)
				{
					DepGraphNode* dep = components[comp->dependencies[j]];
					dep->dependents.insert(node);
					node->dependencies.insert(dep);
				}

				components[vstr] = node;
				infos.push_back(comp);
				instances.push_back(node->instance);
				results.push_back(LoadResult::Success);
			}
		}

		C<EventMan>()->RaiseEvent(LoadCompleteEvent{ 
			.batch = infos.data(), 
			.results = results.data(), 
			.instances = instances.data(), 
			.count = count 
		});
	}

	void ComponentManImp::UnloadComponents(const char* comps[], size_t count, bool unload_deps)
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		ref_cv.wait(lock, [this]{ return ref_counts == 0; });

		C<EventMan>()->RaiseEvent(UnloadBeginEvent{
			.version_strings = comps,
			.count = count
		});

		std::vector<const char*> out_vstrs(count);
		std::vector<UnloadResult> out_results(count);

		std::unordered_set<DepGraphNode*> freed;
		std::stack<DepGraphNode*> stack;

		for (size_t i = 0; i < count; i++)
		{
			if (components.count(comps[i]) == 0)
			{
				out_vstrs.push_back(comps[i]);
				out_results.push_back(UnloadResult::NameNotFound);
			}
			stack.push(components[comps[i]]);
		}

		while (!stack.empty())
		{
			DepGraphNode* node = stack.top();
			stack.pop();

			if (freed.count(node) > 0) continue;
			else if (node->dependents.empty())
			{
				node->comp_info->delete_fun(node->instance);
				for (auto& dep : node->dependencies)
					dep->dependents.erase(node);

				if (--dll_ref_counts[node->dll_handle] < 0)
				{
					C<Logger>()->Warn(this, "Negative ref count for DLL {} upon unloading component {}",
						(void*)node->dll_handle, node->comp_info->version_string);

					dll_ref_counts[node->dll_handle] = 0;
				}

				components.erase(node->comp_info->version_string);
				out_vstrs.push_back(node->comp_info->version_string);
				out_results.push_back(UnloadResult::Success);

				freed.insert(node);
				delete node;
			}
			else if (!unload_deps)
			{
				out_vstrs.push_back(node->comp_info->version_string);
				out_results.push_back(UnloadResult::HasDependent);
			}
			else if (!node->instance->IsUnloadable())
			{
				out_vstrs.push_back(node->comp_info->version_string);
				out_results.push_back(UnloadResult::IsNotUnloadable);
			}
			else
			{
				stack.push(node);
				for (auto& dep : node->dependents)
					stack.push(dep);
			}
		}

		C<EventMan>()->RaiseEvent(UnloadCompleteEvent{
			.version_strings = out_vstrs.data(),
			.results = out_results.data(),
			.count = count
		});
	}

	void ComponentManImp::LoadDlls(const char* dll_names[], size_t count)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		
		std::vector<const CompInfo*> to_load;
		for (size_t i = 0; i < count; i++)
		{
			HMODULE hmod = GetModuleHandleA(dll_names[i]);
			if (hmod == NULL) hmod = LoadLibraryA(dll_names[i]);
			if (hmod == NULL)
			{
				C<Logger>()->Warn(this, "DLL with name \"{}\" could not be found", dll_names[i]);
				continue;
			}
			auto cinfo_getter = (GetExportedComponents_t)GetProcAddress(hmod, "MCF_GetExportedComponents");
			if (cinfo_getter == NULL)
			{
				C<Logger>()->Warn(this, "DLL with name {} does not export MCF_GetExportedComponents", dll_names[i]);
				continue;
			}

			size_t count = 0;
			auto comp_arr = cinfo_getter(&count);

			for (size_t i = 0; i < count; i++)
				to_load.push_back(comp_arr[i]);
		}

		LoadComponents(to_load.data(), to_load.size());
	}

	void ComponentManImp::UnloadDlls(const char* dll_names[], size_t count, bool unload_deps)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);

		std::vector<HMODULE> dlls_to_unload;
		std::vector<const char*> comps_to_unload;
		for (size_t i = 0; i < count; i++)
		{
			HMODULE hmod = GetModuleHandleA(dll_names[i]);
			if (hmod == NULL)
			{
				C<Logger>()->Warn(this, "DLL with name \"{}\" could not be found", dll_names[i]);
				continue;
			}
			auto cinfo_getter = (GetExportedComponents_t)GetProcAddress(hmod, "MCF_GetExportedComponents");
			if (cinfo_getter == NULL)
			{
				C<Logger>()->Warn(this, "DLL with name {} does not export MCF_GetExportedComponents", dll_names[i]);
				continue;
			}

			size_t count = 0;
			auto comp_arr = cinfo_getter(&count);

			for (size_t i = 0; i < count; i++)
				comps_to_unload.push_back(comp_arr[i]->version_string);

			dlls_to_unload.push_back(hmod);
		}

		UnloadComponents(comps_to_unload.data(), comps_to_unload.size(), unload_deps);

		for (const auto& hmod : dlls_to_unload)
			if (dll_ref_counts[hmod] == 0) FreeLibrary(hmod);
	}
}

static MCF::ComponentManImp comp_man;

#ifdef MCF_EXPORTS
extern "C" MCF_C_API MCF::IComponent * MCF_GetComponent(const char* version_string)
{
	return comp_man.GetComponent(version_string);
}

extern "C" MCF_C_API MCF::IComponent * MCF_AcquireComponent(const char* version_string)
{
	return comp_man.AcquireComponent(version_string);
}

extern "C" MCF_C_API void MCF_ReleaseComponent(const char* version_string)
{
	comp_man.ReleaseComponent(version_string);
}

#endif