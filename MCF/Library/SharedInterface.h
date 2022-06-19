#pragma once
#include <type_traits>
#include "TemplateUtils.h"

namespace MCF
{
	/// <summary>
	/// Metadata for SharedInterfaces. If two mods share such a structure, the shared interface manager
	/// will issue a warning but assume both are the same and only load one (to account for mod makers 
	/// copying implementations in their project). 
	/// </summary>
	struct SIMetadata
	{
		/// <summary>
		/// Name of the shared interface. Should be unique to avoid conflicts with other mods. 
		/// Prepending your mod name to the interface name is a good idea to avoid conflicts.
		/// </summary>
		const char* name = nullptr;

		/// <summary>
		/// Version of the interface. Should only be updated when the layout of the virtual
		/// functions in the interface changes. 
		/// </summary>
		const uint32_t version = 0;

		bool operator==(const SIMetadata& lhs, const SIMetadata& rhs)
		{
			return lhs.name == rhs.name && lhs.version == rhs.version;
		}
	};

	/// <summary>
	/// Template class used to define singleton-like virtual method interfaces that are designed
	/// to be used by different mods. Similar in design to Steam API interfaces.
	/// This class should be inherited by the interface class for TInterface, whose
	/// header should be made available for other mods to use. The actual implementation should
	/// inherit from TInterface and implement the virtual methods.
	/// </summary>
	template<class TInterface>
	class SharedInterface
	{
	public:
		// Simple shortcut macro to specify shared interface metadata
		#define SIMeta(name, version) static constexpr const SIMetadata si_metadata = { (name), (version) }
		SIMeta(nullptr, 0);

		virtual const char* Name() const 
		{
			static_assert(TInterface::si_metadata.name != nullptr && TInterface::si_metadata.version != 0, "Invalid/unspecified shared interface metadata");
			return TInterface::si_metadata.name; 
		}
		
		virtual int Version() const 
		{
			return TInterface::si_metadata.version; 
		}

		/// <summary>
		/// Return a pointer to a list of other SharedInterfaces on which this one depends. 
		/// While this can be overriden directly, it is preferable to use the SIDependencies template.
		/// </summary>
		/// <param name="num">Pointer to an integer variable which will receive the numbers of dependencies.</param>
		/// <returns>A pointer to a static array for SIMetadata structs identifying each dependency.</returns>
		virtual const SIMetadata* Dependencies(size_t* num)
		{
			*num = 0;
			return nullptr;
		}

		/// <summary>
		/// Called when the shared interface is first loaded.
		/// Guaranteed to be called after all dependencies. 
		/// May not have to do anything depending on the implementation.
		/// </summary>
		/// <returns>True if initiation was sucessful, false otherwise.</returns>
		virtual bool Init() { };

		/// <summary>
		/// Virtual destructor, will be called when the shared interface is unloaded.
		/// </summary>
		virtual ~SharedInterface() { };
	};

	/// <summary>
	/// Helper template to overwrite the virtual Dependencies() function with less boilerplate
	/// </summary>
	template<typename... Deps>
	class SIDependencies
	{
	public:
		virtual const SIMetadata* Dependencies(size_t& num)
		{
			static const SIMetadata deps[] = { Deps::si_metadata... };
			*num = sizeof(deps) / sizeof(SIMetadata);
			return deps;
		}
	};
}

// Make SIMetadata a hashable type that can be used as a hashmap key
MAKE_HASHABLE(MCF::SIMetadata, t.name, t.version);