#pragma once
#include "SharedInterface.h"
#include "EventMan.h"
#include "SharedInterfaceMan.h"
#include <common.h>
#include <winnt.h>
#include "Utils.h"

namespace MCF
{
	typedef bool (*ModuleFilter)(HMODULE hMod, const char* name);
	typedef bool (*SectionFilter)(HMODULE hMod, const char* mod_name, IMAGE_SECTION_HEADER* section);
	typedef int AobHandle;

	union AobChar
	{
		uint16_t as_num;
		struct {
			uint8_t byte; // Byte to match
			uint8_t mask; // bits of the byte to consider in the scan
		};

		AobChar() : as_num(0) { };
		AobChar(uint16_t val) : as_num(val) { }
		AobChar(uint8_t byte, uint8_t mask) : byte(byte), mask(mask) { }

		operator uint16_t() const { return as_num; }

		inline bool Matches(uint8_t b)
		{
			return byte == (b & mask);
		}
	};

	/// <summary>
	/// Shared interface allowing the registration of AOBs for future scan, and then querying the results by name.
	/// </summary>
	class AobScanMan : public SharedInterface<AobScanMan, "MCF_AOB_SCAN_MAN_001", EventMan>
	{
	public:
		struct AobScanCompleteEvent : public Event<"MCF_AOB_SCAN_COMPLETE_EVENT"> { };

		/// <summary>
		/// Register an AOB to scan in the .text section of the main module with an object instance. 
		/// Will set out_result (if not null) to the result of the scan and dispatch an AobScanComplete event when
		/// all AOBs registered under the "obj" object have been found
		/// </summary>
		virtual AobHandle RegisterAob(const AobChar* aob, size_t length, const void* obj = nullptr, uintptr_t* out_result = nullptr, ModuleFilter module_filter = nullptr, SectionFilter section_filter = nullptr) = 0;

		/// <summary>
		/// Unregister an AOB by handle.
		/// </summary>
		/// <param name="handle"></param>
		virtual void UnregisterAob(AobHandle handle) = 0;

		/// <summary>
		/// Unregister all AOBs defined by a specific object.
		/// </summary>
		/// <param name="obj"></param>
		virtual void UnregisterAobsForObj(const void* obj) = 0;

		/// <summary>
		/// Query the result of an AOB scan by name. Returns the address, or 0 if not found.
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		virtual uintptr_t QueryAobResult(AobHandle) = 0;

		/// <summary>
		/// Convert a CE-style AOB string (ex. DE ? AD BE EF) to a AobChar vector.
		/// </summary>
		/// <param name="ce_aob_string"></param>
		/// <returns></returns>
		static std::vector<AobChar> ConvertAobString(const char* ce_aob_string)
		{
			std::vector<AobChar> aob;

			for (char c = *ce_aob_string; c != 0; c = *(++ce_aob_string))
			{
				if (isspace(c)) continue;
				if (c == '?') aob.push_back(0);
				
				uint8_t byte = Utils::ChrToHex(c);
				if (byte == 0xFF) continue;

				char n = Utils::ChrToHex(ce_aob_string[1]);
				if (n != 0xFF) byte = byte << 4 | n;

				aob.push_back({ byte, 0xFF });
			}

			return aob;
		}

		/// <summary>
		/// Register an AOB to scan in the .text section of the main module with an object instance. 
		/// Will set out_result (if not null) to the result of the scan and dispatch an AobScanComplete event when
		/// all AOBs registered under the "obj" object have been found
		/// </summary>
		AobHandle RegisterAob(const char* ce_aob_string, const void* obj = nullptr, uintptr_t* out_result = nullptr, ModuleFilter module_filter = nullptr, SectionFilter section_filter = nullptr)
		{
			std::vector<AobChar> aob = ConvertAobString(ce_aob_string);
			RegisterAob(aob.data(), aob.size(), obj, out_result, module_filter, section_filter);
		}
	};
}