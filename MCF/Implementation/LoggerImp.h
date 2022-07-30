#pragma once
#include "Include/Logger.h"
#include <regex>
#include <mutex>
#include <shared_mutex>

namespace MCF
{
	class LoggerImp final : public SharedInterfaceImp<Logger, LoggerImp, DepList<EventMan>>
	{
	private:
		std::regex re_src;
		std::regex re_sev;
		std::regex re_msg;
		uint32_t re_flags;

		std::shared_mutex filter_mutex;

		static constexpr uint32_t SRC_MASK = 1;
		static constexpr uint32_t SEV_MASK = 2;
		static constexpr uint32_t MSG_MASK = 4;

	public:
		virtual void Log(const char* source, const char* severity, const char* message) override;
		virtual void Log(const char* source, const char* severity, const wchar_t* message) override;

		virtual void SetFilter(const char* source_regex, const char* sev_regex, const char* msg_regex) override;
		virtual void SetFilter(const char* source_regex, const char* sev_regex, const wchar_t* msg_regex) override;
	};
}