#pragma once
#include "TemplateUtils.h"

namespace MCF
{
	struct ImpDetails;

	class SharedInterfaceBase
	{
	public:
		/// <summary>
		/// Return a structure containing details about the implementation of this shared interface. 
		/// </summary>
		/// <returns></returns>
		virtual const ImpDetails* ImpDetails() const = 0;

		/// <summary>
		/// Return the unique version string of this shared interface.
		/// </summary>
		virtual const char* VersionString() const = 0;

		/// <summary>
		/// Return a pointer to a list of version strings of shared interfaces on which this one depends. 
		/// While this can be overriden directly, it is preferable to inherit from the DependsOn template.
		/// </summary>
		/// <param name="num">Pointer to an integer variable which will receive the numbers of dependencies.</param>
		/// <returns>A pointer to an array of version strings for each dependency.</returns>
		virtual const char** Dependencies(size_t* num) const = 0;

		/// <summary>
		/// Returns true if this shared interface can be unloaded. False by default.
		/// Override this if you wish to add support for disabling your mod at runtime.
		/// </summary>
		virtual bool IsUnloadable() const = 0;

	protected:
		friend class SharedInterfaceManImp;

		/// <summary>
		/// Virtual destructor. Override to free any dynamically allocated resources.
		/// </summary>
		virtual ~SharedInterfaceBase() = default;
	};
}

#ifdef MCF_EXPORTS
extern "C" __declspec(dllexport) MCF::SharedInterfaceBase * MCF_GetInterface(const char* version_string);
#else
extern "C" __declspec(dllimport) MCF::SharedInterfaceBase * MCF_GetInterface(const char* version_string);
#endif

namespace MCF
{
	/// <summary>
	/// Template class used to define singleton-like virtual method interfaces that are designed
	/// to be used by different mods. Similar in design to Steam API interfaces.
	/// This class should be inherited by the interface class for TInterface, whose
	/// header should be made available for other mods to use. The actual implementation should
	/// inherit from TInterface and implement the virtual methods.
	/// </summary>
	template<class TInt, FixedString version_str>
	class SharedInterface : public SharedInterfaceBase
	{
	public:
		static constexpr const char* version_string = version_str;

		virtual const char* VersionString() const override
		{
			return version_string; 
		}

		virtual bool IsUnloadable() const override
		{
			return false;
		}

		/// <summary>
		/// Get a pointer to an instance of this interface, or NULL if it has not been instantiated yet. 
		/// </summary>
		/// <returns></returns>
		static inline TInt* Get()
		{
			return (TInt*)MCF_GetInterface(version_string);
		}
	};

	///<summary>
	///Template class used to store dependencies.
	///</summary>
	template<class... TInterfaces> requires (std::is_base_of_v<SharedInterfaceBase, TInterfaces> && ...)
	struct DepList
	{
		static constexpr size_t count = sizeof...(TInterfaces);
		static const char* version_strings[count == 0 ? 1 : count];
	};
	template<typename... TInterfaces> requires (std::is_base_of_v<SharedInterfaceBase, TInterfaces> && ...)
		const char* DepList<TInterfaces...>::version_strings[] = { TInterfaces::version_string... };
	
	/// <summary>
	/// C struct containing information about the implementation of a particular shared interface. 
	/// This is passed to the module manager. 
	/// </summary>
	struct ImpDetails
	{
		SharedInterfaceBase* (*const new_fcn)();
		void (*const delete_fun)(SharedInterfaceBase*);
		const char* version_string;
		const char** dependencies;
		const size_t num_dependencies;
	};

	/// <summary>
	/// Metadata wrapper template for shared interfaces implementations. The shared interface
	/// implementation should inherit from this.
	/// </summary>
	/// <typeparam name="TInterface">Type of abstract interface.</typeparam>
	/// <typeparam name="DependsOn">List of dependencies.</typeparam>
	/// <typeparam name="is_core_interface">True if this a "core" interface, i.e. an interface that
	/// is loaded manually before the interface manager. Constructor will NOT be called.</typeparam>
	template<class TInt, class TImp, class DependsOn = DepList<>, bool is_core_interface = false> 
		requires (std::is_base_of_v<SharedInterfaceBase, TInt>)
	class SharedInterfaceImp : public TInt
	{
	private:
		static SharedInterfaceBase* OpNew() { return new TImp; }
		static void OpDelete(SharedInterfaceBase* obj) { delete (TImp*)obj; }

	public:
		static const ImpDetails* ImpDetailsStatic()
		{
			static const MCF::ImpDetails meta
			{
				&OpNew,
				&OpDelete,
				TInt::version_string,
				DependsOn::version_strings,
				DependsOn::count
			};
			return &meta;
		}

		virtual const ImpDetails* ImpDetails() const override
		{
			return ImpDetailsStatic();
		}

		virtual const char** Dependencies(size_t* num) const override
		{
			*num = DependsOn::count;
			return DependsOn::version_strings;
		}
	};
}