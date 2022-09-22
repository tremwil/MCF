
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "include/core/Logger.h"
#include <regex>
#include <mutex>
#include <shared_mutex>

namespace MCF
{
    class LoggerImp final : public SharedInterfaceImp<Logger, LoggerImp, DependencyList<EventMan>>
    {
        private:
        std::regex re_src;
        std::regex re_sev;
        std::regex re_msg;
        uint32_t re_flags = 0;

        std::shared_mutex filter_mutex;

        static constexpr uint32_t SRC_MASK = 1;
        static constexpr uint32_t SEV_MASK = 2;
        static constexpr uint32_t MSG_MASK = 4;

        public:
        LoggerImp(const bool* load_success);

        void Log(const char* source, const char* severity, const char* message) override;
        void Log(const char* source, const char* severity, const wchar_t* message) override;

        void SetFilter(const char* source_regex, const char* sev_regex, const char* msg_regex) override;
    };
}