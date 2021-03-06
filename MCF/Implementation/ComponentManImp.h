#pragma once
#include "Include/ComponentMan.h"
#include "Include/Logger.h"
#include "Include/Export.h"
#include "common.h"

namespace MCF
{
	class ComponentManImp final : public SharedInterfaceImp<ComponentMan, ComponentManImp, DepList<EventMan, Logger>>
	{
	private:
		typedef const MCF::CompInfo** (*GetExportedComponents_t)(size_t* n);

		struct DepGraphNode
		{
			std::unordered_set<DepGraphNode*> dependents; // Nodes which depend on this one
			std::unordered_set<DepGraphNode*> dependencies; // Nodes which this one depends on

			const CompInfo* comp_info;
			HMODULE dll_handle;
			IComponent* instance;
		};

		int32_t ref_counts = 0;
		std::condition_variable ref_cv;

		std::unordered_map<HMODULE, int32_t> dll_ref_counts;
		std::unordered_map<std::string, DepGraphNode*> components;
		std::mutex mutex;

	public:
		/// <summary>
		/// Get the instance of a particular component by its unique version string.
		/// This does NOT increase the reference count of the component, so only use if
		/// you know that the component will not be able to be unloaded while you are 
		/// using it.
		/// </summary>
		/// <returns>The requested component, or a null pointer if the component could not be found.</returns>
		virtual IComponent* GetComponent(const char* version_string) override;

		/// <summary>
		/// Get the instance to a particular component by its unique version string.
		/// Increment the component's reference count, so that it cannot be freed while
		/// you are using it. This should only be used if you cannot make the component
		/// a dependency. 
		/// </summary>
		/// <param name="version_string"></param>
		/// <returns>The requested component, or a null pointer if the component could not be found.</returns>
		virtual IComponent* AcquireComponent(const char* version_string) override;

		/// <summary>
		/// Release a particular component, decrementing its reference count.
		/// </summary>
		/// <param name="version_string"></param>
		/// <returns></returns>
		virtual void ReleaseComponent(const char* version_string) override;

		/// <summary>
		/// Load a set of components.
		/// </summary>
		virtual void LoadComponents(const CompInfo* comps[], size_t count) override;

		/// <summary>
		/// Unload a set of components by version string.
		/// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
		/// </summary>
		virtual void UnloadComponents(const char* comps[], size_t count, bool unload_deps = true) override;

		/// <summary>
		/// Load all the components exported by a set of DLLs.
		/// </summary>
		virtual void LoadDlls(const char* dll_names[], size_t count) override;

		/// <summary>
		/// Unload all the components exported by the given DLLs.
		/// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
		/// </summary>
		virtual void UnloadDlls(const char* dll_names[], size_t count, bool unload_deps = true) override;
	};
}