
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "LoggerImp.h"
#include "CoreSettings.h"

namespace MCF
{
    LoggerImp::LoggerImp(const bool *load_success)
    {
        std::string sev_filter = toml::find<std::string>(CoreSettings, "logging", "log_severity_filter");
        if (!sev_filter.empty()) {
            SetFilter(nullptr, sev_filter.c_str(), nullptr);
        }
    }

    void LoggerImp::Log(const char* source, const char* severity, const char* message)
    {
        std::shared_lock<decltype(filter_mutex)> lock(filter_mutex);

        if ((re_flags & SRC_MASK) && !std::regex_search(source, re_src)) return;
        if ((re_flags & SEV_MASK) && !std::regex_search(severity, re_sev)) return;
        if ((re_flags & MSG_MASK) && !std::regex_search(message, re_msg)) return;

        lock.unlock(); // Avoid deadlock from using the logger inside an event callback
        Dep<EventMan>()->RaiseEvent(LogEvent{ .source = source, .sev = severity, .msg = message });
    }

    void LoggerImp::Log(const char* source, const char* severity, const wchar_t* message)
    {
        size_t new_cap = wcslen(message) + 1;
        char* buf = new char[new_cap];
        size_t new_len = 0;
        wcstombs_s(&new_len, buf, new_cap, message, _TRUNCATE);
        Log(source, severity, buf);
        delete[] buf;
    }

    void LoggerImp::SetFilter(const char* src_regex, const char* sev_regex, const char* msg_regex)
    {
        std::unique_lock<decltype(filter_mutex)> lock(filter_mutex);

        if (src_regex == MCF_FILTER_REMOVE) re_flags &= ~SRC_MASK;
        if (sev_regex == MCF_FILTER_REMOVE) re_flags &= ~SEV_MASK;
        if (msg_regex == MCF_FILTER_REMOVE) re_flags &= ~MSG_MASK;

        if (src_regex != nullptr) { re_src = std::regex(src_regex); re_flags |= SRC_MASK; }
        if (sev_regex != nullptr) { re_sev = std::regex(sev_regex); re_flags |= SEV_MASK; }
        if (msg_regex != nullptr) { re_msg = std::regex(msg_regex); re_flags |= MSG_MASK; }
    }
}
