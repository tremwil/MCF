#pragma once
#include "SharedInterface.h"
#include <common.h>
#include <winnt.h>

namespace MCF
{
	typedef bool (*ModuleFilter)(HMODULE hMod, const char* name);
	typedef bool (*SectionFilter)(HMODULE hMod, const char* mod_name, IMAGE_SECTION_HEADER* section);

	union AOBChar
	{
		uint16_t as_num;
		struct {
			uint8_t byte; // Byte to match
			uint8_t mask; // bits of the byte to consider in the scan
		};
	};

	/// <summary>
	/// Shared interface allowing the registration of AOBs for future scan, and then querying the results by name.
	/// </summary>
	class IAOBScanMgr : public SharedInterface<IAOBScanMgr>
	{
	public:
		SIMeta("Core_AOBScanMgr", 1);

		/// <summary>
		/// Register an AOB to scan to the .text section of the main module.
		/// </summary>
		virtual void RegisterAob(const char* name, const AOBChar* aob, int length) = 0;


		/// <summary>
		/// Register an AOB to scan which applies to the modules/sections given by the filter functions.
		/// </summary>
		virtual void RegisterAobEx(const char* name, const AOBChar* aob, int length, ModuleFilter module_filter, SectionFilter section_filter) = 0;


		/// <summary>
		/// Query the result of an AOB scan by name. Returns the address, or 0 if not found.
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		virtual uintptr_t QueryAobResult(const char* name) = 0;

		/// <summary>
		/// Convert a CE-style AOB string (ex. DE ?? AD ?? BE ?? EF) 
		/// </summary>
		/// <param name="ce_aob_string"></param>
		/// <returns></returns>
		std::vector<AOBChar> ConvertAobString(const char* ce_aob_string)
		{
			
		}

		/// <summary>
		/// Register an AOB to scan which applies to the modules/sections given by the filter functions (or only the main module/.text section
		/// of it if the filters are null). 
		/// </summary>
		/// <param name="ce_aob_string"></param>
		void RegisterAobString(const char* name, const char* ce_aob_string, ModuleFilter module_filter = nullptr, SectionFilter section_filter = nullptr)
		{

		}
	};
}