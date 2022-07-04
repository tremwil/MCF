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

		DepGraphNode* node = components[version_string];
		node->ref_count++;

		return node->instance;
	}

	void ComponentManImp::ReleaseComponent(const char* version_string)
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		if (!components.count(version_string)) return;

		DepGraphNode* node = components[version_string];
		node->ref_count--;
		if (node->ref_count < 0)
		{
			// TODO: C<Logger>()->Warn(...);
			node->ref_count = 0;
		};
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
		for (int i = 0; i < count; i++)
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

		
		while (!info_stack.empty())
		{
			const CompInfo* comp = info_stack.top();
			const char* vstr = comp->version_string;
			bool visited = visited_stack.top();
			visited_stack.pop();
			visited_stack.push(true);

			// Component was already intialized -- do nothing
			if (components.count(vstr))
			{
				info_stack.pop();
				visited_stack.pop();
				continue;
			}
			
			// First pass -- check for errors
			LoadResult res = LoadResult::Success;
			for (int j = 0; j < comp->num_dependencies; j++)
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

				info_stack.pop();
				visited_stack.pop();
			}
			// If succeeded but not visited, scan dependencies first (DFS)
			else if (!visited)
			{
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

				for (int j = 0; j < comp->num_dependencies; j++)
				{
					DepGraphNode* dep = components[comp->dependencies[j]];
					dep->inward.insert(node);
					node->outward.insert(dep);
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

	void ComponentManImp::UnloadComponents(const CompInfo* comps[], size_t count, bool unload_deps)
	{

	}

	void ComponentManImp::LoadDlls(const char* dll_names[], size_t count)
	{
		
	}

	void ComponentManImp::UnloadDlls(const char* dll_names[], size_t count, bool unload_deps)
	{

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