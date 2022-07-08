#pragma once
#include "EventMan.h"
#include <format>
#include <string>
#include <string_view>

namespace MCF
{
	class Logger : public SharedInterface<Logger, "MCF_LOGGER_001">
	{
	public:
		struct LogEvent : Event<"MCF_LOG_EVENT">
		{
			const char* source;
			const char* sev;
			const wchar_t* w_msg;
		};

		static constexpr const char* SevDebug = "debug";
		static constexpr const char* SevInfo = "info";
		static constexpr const char* SevWarn = "warn";
		static constexpr const char* SevError = "error";

		static constexpr const char* FilterRemove = (const char*)1;

		virtual void Log(const char* source, const char* severity, const char* message) = 0;
		virtual void Log(const char* source, const char* severity, const wchar_t* message) = 0;

		/// <summary>
		/// Set a global filter on log messages, based on either the source, severity or message contents.
		/// Passing NULL for a filter will leave it untouched, while passing FILTER_REMOVE (1) will remove it.
		/// </summary>
		virtual void SetFilter(const char* source_regex, const char* sev_regex, const char* msg_regex) = 0;

		/// <summary>
		/// Set a global filter on log messages, based on either the source, severity or message contents.
		/// Passing NULL for a filter will leave it untouched, while passing FILTER_REMOVE (1) will remove it.
		/// </summary>
		virtual void SetFilter(const char* source_regex, const char* sev_regex, const wchar_t* msg_regex) = 0;

		template<class Fmt, class... Args>
		inline void Log(const char* source, const char* severity, Fmt fmt, Args&&... args)
		{
			auto message = std::format(fmt, args...);
			Log(source, severity, message.c_str());
		}

		template<class Fmt, class... Args>
		inline void Log(IComponent* source, const char* severity, Fmt fmt, Args&&... args)
		{
			Log(source->VersionString(), severity, fmt, args...);
		}

		template<class Source, class Fmt, class... Args>
		inline void Debug(Source source, Fmt fmt, Args&&... args) { Log(source, SevDebug, fmt, args...); }
		template<class Source, class Fmt, class... Args>
		inline void Info(Source source, Fmt fmt, Args&&... args) { Log(source, SevInfo, fmt, args...); }
		template<class Source, class Fmt, class... Args>
		inline void Warn(Source source, Fmt fmt, Args&&... args) { Log(source, SevWarn, fmt, args...); }
		template<class Source, class Fmt, class... Args>
		inline void Error(Source source, Fmt fmt, Args&&... args) { Log(source, SevError, fmt, args...); }
	};
}
