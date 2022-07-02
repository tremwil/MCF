#pragma once
#include "TemplateUtils.h"

namespace MCF
{
	struct ComponentInfo;

	/// <summary>
	/// Interface for components, the base objects managed automatically by MCF.
	/// </summary>
	class IComponent
	{
	protected:
		/// <summary>
		/// Virtual destructor. Override to free any dynamically allocated resources.
		/// </summary>
		virtual ~IComponent() = default;

	public:
		/// <summary>
		/// Return a structure containing details about the implementation of this component. 
		/// </summary>
		/// <returns></returns>
		virtual const ComponentInfo* ComponentInfo() const = 0;

		/// <summary>
		/// Return the unique version string of this component.
		/// </summary>
		virtual const char* VersionString() const = 0;

		/// <summary>
		/// Return a pointer to a list of version strings of components on which this one depends. 
		/// This should not be overriden directly. Pass a DependsOn list in the Component template instead.
		/// </summary>
		/// <param name="num">Pointer to an integer variable which will receive the numbers of dependencies.</param>
		/// <returns>A pointer to an array of version strings for each dependency.</returns>
		virtual const char** Dependencies(size_t* num) const = 0;

		/// <summary>
		/// Returns true if this component can be unloaded. False by default.
		/// Override this if you wish to specify your component cannot be unloaded at runtime.
		/// </summary>
		virtual bool IsUnloadable() const = 0;
	};
}

#ifdef MCF_EXPORTS
extern "C" __declspec(dllexport) MCF::IComponent * MCF_GetComponent(const char* version_string);
#else
extern "C" __declspec(dllimport) MCF::IComponent * MCF_GetComponent(const char* version_string);
#endif

namespace MCF
{
	///<summary>
	///Template class used to store dependencies to other components.
	///</summary>
	template<class... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
	struct DepList
	{
		static constexpr size_t count = sizeof...(Components);
		static const char* version_strings[count == 0 ? 1 : count];
	};
	template<typename... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
		const char* DepList<Components...>::version_strings[] = { Components::version_string... };
	
	/// <summary>
	/// C struct containing information about the implementation of a particular component. 
	/// Passed to the module manager through MCF_GetExportedInterfaces. 
	/// </summary>
	struct ComponentInfo
	{
		typedef IComponent* (* const NewOp)(void);
		typedef void (* const DeleteOp)(IComponent*);

		NewOp        new_fun;
		DeleteOp     delete_fun;
		const char*  version_string;
		const char** dependencies;
		const size_t num_dependencies;
		const bool   is_core_component;
	};

	/// <summary>
	/// Represents an object automatically managed by MCF. Provides automatic
	/// load order resolution to ensure resourced required by objects initialized
	/// on construction (i.e. events, hooks, etc) are available.
	/// </summary>
	/// <param name="T">The type which will inherit from this template to implement functionality.</param>
	/// <param name="version_str">A unique version string for this component.</param>
	/// <param name="DependsOn">List of dependencies, i.e. other IComponents which must be constructed and loaded before this one.</param>
	/// <param name="is_core_interface">Signifies to the ComponentMan that this component is a core interface and loaded early. Internal, 
	/// do not sure on your own components.</param>
	/// <param name="Interface">The interface used as a base class for this component.</param>
	template<class T, FixedString version_str, class DependsOn = DepList<>, bool is_core_component = false, class Interface = IComponent>
		requires std::is_base_of_v<IComponent, Interface>
	class Component : public Interface
	{
	private:
		static IComponent* OpNew() { return new T; }
		static void OpDelete(IComponent* obj) { delete (T*)obj; }

	public:
		static constexpr FixedString version_string = version_str;
		
		/// <summary>
		/// Get a pointer to an instance of this interface, or NULL if it has not been instantiated yet. 
		/// </summary>
		static inline T* Get()
		{
			return (T*)MCF_GetComponent(version_string);
		}

		static const ComponentInfo* ComponentInfoStatic()
		{
			static const MCF::ComponentInfo meta
			{
				.new_fun = &OpNew,
				.delete_fun = &OpDelete,
				.version_string = version_string,
				.dependencies = DependsOn::version_strings,
				.num_dependencies = DependsOn::count,
				.is_core_component = is_core_component
			};
			return &meta;
		}

		virtual const ComponentInfo* ComponentInfo() const override
		{
			return ComponentInfoStatic();
		}

		virtual const char* VersionString() const override
		{
			return version_string;
		}

		virtual bool IsUnloadable() const override
		{
			return !is_core_component;
		}

		virtual const char** Dependencies(size_t* num) const override
		{
			*num = DependsOn::count;
			return DependsOn::version_strings;
		}
	};
}