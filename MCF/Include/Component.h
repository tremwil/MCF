#pragma once
#include "TemplateUtils.h"

namespace MCF
{
	struct CompInfo;

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
		virtual const CompInfo* ComponentInfo() const = 0;

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
#define MCF_C_API __declspec(dllexport)
#else
#define MCF_C_API __declspec(dllimport)
#endif

extern "C" MCF_C_API MCF::IComponent * MCF_GetComponent(const char* version_string);
extern "C" MCF_C_API MCF::IComponent * MCF_AcquireComponent(const char* version_string);
extern "C" MCF_C_API void MCF_ReleaseComponent(const char* version_string);

namespace MCF
{
	///<summary>
	///Template class used to store dependencies to other components.
	///</summary>
	template<class... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
	struct DepList
	{
		static std::initializer_list<IComponent*> GetInstances()
		{
			return { MCF_GetComponent(Components::version_string)... };
		}

		static constexpr size_t count = sizeof...(Components);
		static const char* version_strings[count == 0 ? 1 : count];
	};
	template<typename... Components> requires (std::is_base_of_v<IComponent, Components> && ...)
		const char* DepList<Components...>::version_strings[] = { Components::version_string... };

	/// <summary>
	/// Get the index of D in the dependency list L at compile-time.
	/// </summary>
	template<class D, class L>
	struct DepListIndex
	{
		static constexpr int index = 0;
	};;

	template<class D, class Head, class... Tail>
	struct DepListIndex<D, DepList<Head, Tail...>>
	{
		static constexpr bool index = std::is_same_v<D, Head> ? 0 
			: 1 + DepListIndex<D, DepList<Tail...>>::index;
	};

	/// <summary>
	/// C struct containing information about the implementation of a particular component. 
	/// Passed to the module manager through MCF_GetExportedInterfaces. 
	/// </summary>
	struct CompInfo
	{
		typedef IComponent* (* const NewOp)(void);
		typedef void (* const DeleteOp)(IComponent*);

		const char*  version_string;
		const char** dependencies;
		const size_t num_dependencies;

		NewOp        new_fun;
		DeleteOp     delete_fun;
	};

	/// <summary>
	/// Represents an object automatically managed by MCF. Provides automatic
	/// load order resolution to ensure resourced required by objects initialized
	/// on construction (i.e. events, hooks, etc) are available.
	/// </summary>
	/// <param name="T">The type which will inherit from this template to implement functionality.</param>
	/// <param name="version_str">A unique version string for this component.</param>
	/// <param name="DependsOn">List of dependencies, i.e. other IComponents which must be constructed and loaded before this one.</param>
	/// <param name="Interface">The interface used as a base class for this component.</param>
	template<class T, FixedString version_str, class DependsOn, class Interface = IComponent>
		requires std::is_base_of_v<IComponent, Interface>
	class Component : public Interface
	{
	private:
		static IComponent* OpNew() { return new T; }
		static void OpDelete(IComponent* obj) { delete (T*)obj; }

		template<typename S>
		struct DepPtrArray { };

		template<typename... Deps> 
		struct DepPtrArray<DepList<Deps...>>
		{
			IComponent* deps[sizeof...(Deps) == 0 ? 1 : sizeof...(Deps)] = { MCF_GetComponent(Deps::version_string)... };
		};

		DepPtrArray<DependsOn> dep_list;

	protected:
		/// <summary>
		/// Efficient access to a dependency. Prefer this helper function to 
		/// Dep::Get(), as it avoids unecessary calls to MCF_GetComponent.
		/// </summary>
		template<class Dep> requires (DepListIndex<Dep, DependsOn>::index < DependsOn::count)
		Dep* C()
		{
			constexpr int i = DepListIndex<Dep, DependsOn>::index;
			return (Dep*)dep_list.deps[i];
		}

	public:
		Component() { };
		Component(Component&) = delete;
		Component(Component&&) = delete;

		static constexpr FixedString version_string = version_str;
		
		/// <summary>
		/// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet. 
		/// This does NOT increment the reference count of the underlying Component, and as such using 
		/// Get() to access a module which is not in the dependency list is unsafe. Consider using Acquire() 
		/// and Release() instead. 
		/// </summary>
		static inline T* Get()
		{
			return (T*)MCF_GetComponent(version_string);
		}

		/// <summary>
		/// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet. 
		/// This increments the reference count to this component, so it will not be able to be freed 
		/// while you are using it.
		/// </summary>
		static inline T* Acquire()
		{
			return (T*)MCF_AcquireComponent(version_string);
		}

		/// <summary>
		/// Release this component, decrementing its reference count.
		/// </summary>
		static inline void Release()
		{
			return MCF_ReleaseComponent(version_string);
		}

		static const CompInfo* ComponentInfoStatic()
		{
			static const MCF::CompInfo meta
			{
				.version_string = version_string,
				.dependencies = DependsOn::version_strings,
				.num_dependencies = DependsOn::count,
				.new_fun = &OpNew,
				.delete_fun = &OpDelete,
			};
			return &meta;
		}

		virtual const CompInfo* ComponentInfo() const override
		{
			return ComponentInfoStatic();
		}

		virtual const char* VersionString() const override
		{
			return version_string;
		}

		virtual bool IsUnloadable() const override
		{
			return false;
		}

		virtual const char** Dependencies(size_t* num) const override
		{
			*num = DependsOn::count;
			return DependsOn::version_strings;
		}
	};

	/// <summary>
	/// Wrapper around a given component that automatically 
	/// Acquires() on construction and Releases() when destroyed (i.e. goes out of scope).
	/// Prefer this to directly calling C::Acquire/Release or the C API functions.
	/// </summary>
	/// <typeparam name="C"></typeparam>
	template<class C> 
	struct SmartComp
	{
	private:
		C* instance;

	public:
		SmartComp(SmartComp&) = delete;
		SmartComp(SmartComp&&) = delete;

		SmartComp() : instance(C::Acquire()) { }
		~SmartComp() { C::Release(); }

		C* operator()() const { return instance; }
	};
}