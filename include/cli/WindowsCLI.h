
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/core/SharedInterface.h"

namespace MCF
{
    class WindowsCLI : public SharedInterface<WindowsCLI, "MCF_WINDOWS_CLI_001">
    {
    public:
        virtual void Show() = 0;

        virtual void Hide() = 0;

        virtual void Clear() = 0;

        /// Set the foreground/background colors for a given log severity. The color format follows that of the Windows COLOR command.
        /// \param sev_name The name of the severity to set a color for.
        /// \param sev_color The consoke color of the severity name (this does not apply to the message).
        /// \param msg_color The console color to apply to the message.
        virtual void SetLogSeverityColor(const char* sev_name, uint8_t sev_color, uint8_t msg_color) = 0;
    };
}