
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "EventMan.h"
#include "../third-party/fmt/format.h"

namespace MCF
{
    class Logger : public SharedInterface<Logger, "MCF_LOGGER_001">
    {
    public:
        struct LogEvent : Event<LogEvent, "MCF_LOG_EVENT">
        {
        private:
            friend class LoggerImp; // To avoid needless virtual indirection
            std::string source;
            std::string sev;
            std::string msg;
        public:
            virtual const char* Source() const { return source.c_str(); }
            virtual const char* Sev() const { return sev.c_str(); }
            virtual const char* Msg() const { return msg.c_str(); }
        };

        static constexpr const char* SevDebug = "debug";
        static constexpr const char* SevInfo = "info";
        static constexpr const char* SevWarn = "warn";
        static constexpr const char* SevError = "error";

        virtual void Log(const char* source, const char* severity, const char* message) = 0;
        virtual void Log(const char* source, const char* severity, const wchar_t* message) = 0;

        /// Set a global filter on log messages, based on either the source, severity or message contents.
        /// Passing NULL for a filter will leave it untouched, while passing MCF_FILTER_REMOVE (1) will remove it.
        virtual void SetFilter(const char* source_regex, const char* sev_regex, const char* msg_regex) = 0;

        template<class Fmt, class... Args>
        inline void Log(const char* source, const char* severity, Fmt format, Args&&... args)
        {
            try {
                auto message = fmt::vformat(format, fmt::make_format_args(args...));
                Log(source, severity, message.c_str());
            }
            catch (fmt::format_error& err) {
                Log(version_string, SevError, err.what());
            }
        }

        template<class Fmt, class... Args>
        inline void Log(IComponent* source, const char* severity, Fmt format, Args&&... args)
        {
            Log(source->VersionString(), severity, format, args...);
        }

        template<class Source, class Fmt, class... Args>
        inline void Debug(Source source, Fmt format, Args&&... args) { Log(source, SevDebug, format, args...); }
        template<class Source, class Fmt, class... Args>
        inline void Info(Source source, Fmt format, Args&&... args) { Log(source, SevInfo, format, args...); }
        template<class Source, class Fmt, class... Args>
        inline void Warn(Source source, Fmt format, Args&&... args) { Log(source, SevWarn, format, args...); }
        template<class Source, class Fmt, class... Args>
        inline void Error(Source source, Fmt format, Args&&... args) { Log(source, SevError, format, args...); }
    };
}

#define MCF_FILTER_REMOVE ((const char*)1)
