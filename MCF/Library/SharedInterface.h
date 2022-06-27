#pragma once
#include "TemplateUtils.h"

namespace MCF
{
	class SharedInterfaceBase
	{
	public:
		virtual const char* VersionString() const = 0;

		/// <summary>
		/// Return a pointer to a list of other SharedInterfaces on which this one depends. 
		/// While this can be overriden directly, it is preferable to use the SIDependencies template.
		/// </summary>
		/// <param name="num">Pointer to an integer variable which will receive the numbers of dependencies.</param>
		/// <returns>A pointer to a static array for SIMetadata structs identifying each dependency.</returns>
		virtual const char** Dependencies(size_t* num) const = 0;

		/// <summary>
		/// Returns true if this shared interface can be unloaded. False by default.
		/// Override this if you wish to add support for disabling your mod at runtime.
		/// </summary>
		/// <returns></returns>
		virtual bool IsUnloadable() const = 0;

	protected:
		friend class SharedInterfaceMan;

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
	template<class TInterface, FixedString version_str, typename... Deps>
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

		virtual const char** Dependencies(size_t* num) const override
		{
			return StaticDependencies(num);
		}

		static const char** StaticDependencies(size_t* num)
		{
			if constexpr ((sizeof...(Deps)) > 0)
			{
				static const char* deps[] = { Deps::version_string... };
				*num = sizeof(deps) / sizeof(char*);
				return deps;
			}
			else
			{
				*num = 0;
				return nullptr;
			}
		}

		/// <summary>
		/// Get a pointer to an instance of this interface, or NULL if it has not been instantiated yet. 
		/// </summary>
		/// <returns></returns>
		static inline TInterface* Get()
		{
			return (TInterface*)MCF_GetInterface(version_string);
		}
	};
}