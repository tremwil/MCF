#pragma once
#include "EventMan.h"

namespace MCF
{
	/// <summary>
	/// The component manager. Responsible for loading/unloading components provided by different DLLs.
	/// </summary>
	class ComponentMan : public SharedInterface<ComponentMan, "MCF_COMPONENT_MAN_001">
	{
	public:
		enum class LoadResult : int32_t
		{
			Success = 0,
			NameConflict = 1,
			DependencyNotFound = 2,
			CircularDependency = 3
		};

		/// <summary>
		/// Event raised when a component was just unloaded.
		/// </summary>
		struct ComponentUnloadEvent : public Event<"MCF_CM_UNLOAD_EVENT">
		{
			const CompInfo* info;
			IComponent* instance;
			LoadResult result;
		};

		/// <summary>
		/// Event raised when a batch component load has begun.
		/// </summary>
		struct LoadBeginEvent : public Event<"MCF_CM_LOAD_BEGIN_EVENT"> 
		{ 
			const CompInfo** to_load;
			size_t count;
		};

		/// <summary>
		/// Event raised when a batch component load has completed.
		/// </summary>
		struct LoadCompleteEvent : public Event<"MCF_CM_LOAD_COMPLETE_EVENT">
		{
			const CompInfo** batch;
			const LoadResult* results;
			IComponent** instances;
			size_t count;
		};

		/// <summary>
		/// Get the instance of a particular component by its unique version string.
		/// This does NOT increase the reference count of the component, so only use if
		/// you know that the component will not be able to be unloaded while you are 
		/// using it.
		/// </summary>
		/// <returns>The requested component, or a null pointer if the component could not be found.</returns>
		virtual IComponent* GetComponent(const char* version_string) = 0;

		/// <summary>
		/// Get the instance to a particular component by its unique version string.
		/// Increment the component's reference count, so that it cannot be freed while
		/// you are using it. This should only be used if you cannot make the component
		/// a dependency. 
		/// </summary>
		/// <param name="version_string"></param>
		/// <returns>The requested component, or a null pointer if the component could not be found.</returns>
		virtual IComponent* AcquireComponent(const char* version_string) = 0;

		/// <summary>
		/// Release a particular component, decrementing its reference count.
		/// </summary>
		/// <param name="version_string"></param>
		/// <returns></returns>
		virtual void ReleaseComponent(const char* version_string) = 0;

		/// <summary>
		/// Load a set of components.
		/// </summary>
		virtual void LoadComponents(const CompInfo* comps[], size_t count) = 0;

		/// <summary>
		/// Unload a set of components.
		/// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
		/// </summary>
		virtual void UnloadComponents(const CompInfo* comps[], size_t count, bool unload_deps = true) = 0;

		/// <summary>
		/// Load all the components exported by a set of DLLs.
		/// </summary>
		virtual void LoadDlls(const char* dll_names[], size_t count) = 0;

		/// <summary>
		/// Unload all the components exported by the given DLLs.
		/// If unload_deps is true, will unload components that depend on the ones unloaded instead of failing.
		/// </summary>
		virtual void UnloadDlls(const char* dll_names[], size_t count, bool unload_deps = true) = 0;
	};
}