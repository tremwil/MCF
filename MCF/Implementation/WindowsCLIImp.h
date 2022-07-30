#pragma once
#include "Include/Logger.h"
#include "Include/WindowsCLI.h"
#include "Include/EventMan.h"
#include "Include/CommandMan.h"
#include "Include/Export.h"

#include <Windows.h>
#include <concurrent_unordered_map.h>
#include <iomanip>
#include <iostream>

namespace MCF
{
	class WindowsCLIImp : public SharedInterfaceImp<WindowsCLI, WindowsCLIImp, DepList<EventMan, CommandMan>>
	{
	private:
		concurrency::concurrent_unordered_map<std::string, uint16_t> colors;

		EventCallback<Logger::LogEvent> log_cb = [this](Logger::LogEvent* evt) {
			// Log style: [TIME] [SEV] [SOURCE] MESSAGE

			uint16_t color = colors.count(evt->sev) ? colors.at(evt->sev) : 0x0707u;

			auto time = std::time(nullptr);
			auto tm = std::localtime(&time);
			
			HANDLE STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);
			
			SetConsoleTextAttribute(STDOUT, color & 0xff);
			std::cout << std::put_time(tm, "[%T] ") << "[";
			SetConsoleTextAttribute(STDOUT, color >> 8);
			std::cout << evt->sev;
			SetConsoleTextAttribute(STDOUT, color & 0xff);
			std::cout << "] [" << evt->source << "] ";
			std::cout << evt->msg << std::endl;
			SetConsoleTextAttribute(STDOUT, 0x07);
		};

	public:
		WindowsCLIImp()
		{
			colors[Logger::SevDebug] = 0x0A0A;
			colors[Logger::SevInfo] = 0x0907;
			colors[Logger::SevWarn] = 0x0E07;
			colors[Logger::SevError] = 0x0C07;

			Show();
		}

		virtual bool IsUnloadable() const override { return true; }

		virtual void Show() override 
		{
			if (GetConsoleWindow() || AllocConsole()) {
				FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;

				freopen_s(&fpstdin, "CONIN$", "r", stdin);
				freopen_s(&fpstdout, "CONOUT$", "w", stdout);
				freopen_s(&fpstderr, "CONOUT$", "w", stderr);

				std::cout.clear();
				std::clog.clear();
				std::cerr.clear();
				std::cin.clear();

				// Remove console close button to prevent accidentally closing the game process
				HWND hwnd = GetConsoleWindow();
				if (hwnd != NULL)
				{
					HMENU hMenu = GetSystemMenu(hwnd, FALSE);
					if (hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
				}
			}
		};

		virtual void Hide() override
		{
			fclose(stdin);
			fclose(stdout);
			fclose(stderr);
			FreeConsole();
		};

		virtual void Clear() override
		{
			COORD tl = { 0,0 };
			CONSOLE_SCREEN_BUFFER_INFO s;
			HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleScreenBufferInfo(console, &s);
			DWORD written, cells = s.dwSize.X * s.dwSize.Y;
			FillConsoleOutputCharacterA(console, ' ', cells, tl, &written);
			FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
			SetConsoleCursorPosition(console, tl);
		};

		virtual void SetLogSeverityColor(const char* sev_name, uint8_t sev_color, uint8_t msg_color) override
		{
			colors[sev_name] = ((uint16_t)sev_color << 8) | (uint16_t)msg_color;
		};
	};

	MCF_COMPONENT_EXPORT(WindowsCLIImp);
}