#pragma once
#include "Component.h"

namespace MCF
{
	/// <summary>
	/// Template class used to define singleton-like virtual method interfaces that are designed
	/// to be used by different mods. Similar in design to Steam API interfaces. This class should 
	/// be inherited by the interface class for TInterface, whose header should be made available for 
	/// other mods to use. The actual implementation should inherit from the SharedInterfaceImp template. 
	/// </summary>
	template<class TInterface, FixedString version_str>
	class SharedInterface : public IComponent
	{
	public:
		static constexpr FixedString version_string = version_str;

		/// <summary>
		/// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet. 
		/// This does NOT increment the reference count of the underlying Component, and as such using 
		/// Get() to access a module which is not in the dependency list is unsafe. Consider using Acquire() 
		/// and Release() instead. 
		/// </summary>
		static inline TInterface* Get()
		{
			return (TInterface*)MCF_GetComponent(version_string);
		}

		/// <summary>
		/// Get a pointer to an instance of this component, or NULL if it has not been instantiated yet. 
		/// This increments the reference count to this component, so it will not be able to be freed 
		/// while you are using it.
		/// </summary>
		static inline TInterface* Acquire()
		{
			return (TInterface*)MCF_AcquireComponent(version_string);
		}

		/// <summary>
		/// Release this component, decrementing its reference count.
		/// </summary>
		static inline void Release()
		{
			return MCF_ReleaseComponent(version_string);
		}
	};

	/// <summary>
	/// Implementation of a SharedInterface.
	/// </summary>
	/// <param name="Interface">The abstract interface we are implementing.</param>
	/// <param name="Implementation">The implementation type. Should be the class that inherits this.</param>
	/// <param name="DependsOn">List of dependencies, i.e. other IComponents which must be constructed and loaded before this one.</param>
	/// <param name="is_core_interface">Signifies to the ComponentMan that this interface is a core interface and loaded early. Internal, 
	/// do not sure on your own components.</param>
	template<class Interface, class Implementation, class DependsOn = DepList<>, bool is_core_interface = false>
	class SharedInterfaceImp : public Component<Implementation, Interface::version_string, DependsOn, is_core_interface, Interface> { };
}