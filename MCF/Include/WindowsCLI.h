#pragma once
#include "SharedInterface.h"

namespace MCF
{
	class WindowsCLI : public SharedInterface<WindowsCLI, "MCF_WINDOWS_CLI_001">
	{
	public:
		virtual void Show() = 0;

		virtual void Hide() = 0;

		virtual void Clear() = 0;

		/// <summary>
		/// Set the foreground/background colors for a given log severity. The color format follows that of the Windows COLOR command.
		/// </summary>
		/// <param name="sev_name">The name of the severity to set a color for.</param>
		/// <param name="sev_color">The consoke color of the severity name (this does not apply to the message).</param>
		/// <param name="msg_color">The console color to apply to the message.</param>
		virtual void SetLogSeverityColor(const char* sev_name, uint8_t sev_color, uint8_t msg_color) = 0;
	};
}