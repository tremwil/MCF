#include "LoggerImp.h"

namespace MCF
{
	void LoggerImp::Log(const char* source, const char* severity, const char* message)
	{
		size_t new_cap = strlen(message) + 1;
		wchar_t* wbuf = new wchar_t[new_cap];
		size_t new_len = 0;
		mbstowcs_s(&new_len, wbuf, new_cap, message, _TRUNCATE);
		Log(source, severity, wbuf);
		delete[] wbuf;
	}

	void LoggerImp::Log(const char* source, const char* severity, const wchar_t* message)
	{
		std::shared_lock<decltype(filter_mutex)> lock(filter_mutex);

		if ((re_flags & SRC_MASK) && !std::regex_search(source, re_src)) return;
		if ((re_flags & SEV_MASK) && !std::regex_search(severity, re_sev)) return;
		if ((re_flags & MSG_MASK) && !std::regex_search(message, re_msg)) return;

		C<EventMan>()->RaiseEvent(LogEvent{ .source = source, .sev = severity, .w_msg = message });
	}

	void LoggerImp::SetFilter(const char* src_regex, const char* sev_regex, const char* msg_regex)
	{
		if (msg_regex != nullptr && msg_regex != FilterRemove)
		{
			size_t new_cap = strlen(msg_regex) + 1;
			wchar_t* wbuf = new wchar_t[new_cap];
			size_t new_len = 0;
			mbstowcs_s(&new_len, wbuf, new_cap, msg_regex, _TRUNCATE);
			SetFilter(src_regex, sev_regex, wbuf);
			delete[] wbuf;
		}
		else SetFilter(src_regex, sev_regex, (const wchar_t*)msg_regex);
	}

	void LoggerImp::SetFilter(const char* src_regex, const char* sev_regex, const wchar_t* msg_regex)
	{
		std::unique_lock<decltype(filter_mutex)> lock(filter_mutex);

		if (src_regex == FilterRemove) re_flags &= ~SRC_MASK;
		if (sev_regex == FilterRemove) re_flags &= ~SEV_MASK;
		if (msg_regex == (const wchar_t*)FilterRemove) re_flags &= ~MSG_MASK;

		if (src_regex != nullptr) { re_src = std::regex(src_regex); re_flags |= SRC_MASK; }
		if (sev_regex != nullptr) { re_sev = std::regex(sev_regex); re_flags |= SEV_MASK; }
		if (msg_regex != nullptr) { re_msg = std::wregex(msg_regex); re_flags |= MSG_MASK; }
	}
}
