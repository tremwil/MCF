
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "core/Logger.h"
#include "core/EventMan.h"
#include "include/cli/CommandMan.h"
#include "core/Export.h"

#include "include/cli/WindowsCLI.h"

#include "include/windows_include.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>

#include "conio.h"

namespace MCF
{
    class WindowsCLIImp final : public SharedInterfaceImp<WindowsCLI, WindowsCLIImp, DependencyList<CommandMan, Logger>>
    {
    private:
        bool load_success = false;

        std::unordered_map<std::string, uint16_t> colors;
        std::mutex mutex;

        std::thread input_thread;
        bool input_in_progress = false;
        bool close_input_thread = false;
        bool console_loaded = false;

        struct QueuedLogMessage
        {
            std::string source;
            std::string severity;
            std::string message;

            QueuedLogMessage(const Logger::LogEvent* evt) :
                source(evt->source),
                severity(evt->sev),
                message(evt->msg) { };
        };

        std::queue<QueuedLogMessage> msg_queue;

        EventCallback<Logger::LogEvent> log_cb = [this](const Logger::LogEvent *evt) {
            if (!load_success) return;

            std::lock_guard<decltype(mutex)> lock(mutex);

            if (input_in_progress)
                msg_queue.push(evt);
            else
                LogInternal(evt->source, evt->sev, evt->msg);
        };

        Command clearCommand = { "clear", "Clears the console window", [this](const char* args[], size_t count) {
             Clear();
        }};

        Command hideCommand = { "hide", "Hides the console window", [this](const char* args[], size_t count) {
            Hide();
        }};

        void LogInternal(const char* src, const char* sev, const char* msg)
        {
            // Log style: [HH:MM:SS.mmm] [SEV] [SOURCE] MESSAGE
            uint16_t color = colors.count(sev) ? colors.at(sev) : 0x0707u;

            auto now = std::chrono::system_clock::now();
            auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto tm = std::localtime(&time);

            HANDLE STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);

            SetConsoleTextAttribute(STDOUT, color & 0xff);
            std::cout << std::put_time(tm, "[%T.") << std::setfill('0') << std::setw(3) << milliseconds << "] [";
            SetConsoleTextAttribute(STDOUT, color >> 8);
            std::cout << sev;
            SetConsoleTextAttribute(STDOUT, color & 0xff);
            std::cout << "] [" << src << "] ";
            std::cout << msg << std::endl;
            SetConsoleTextAttribute(STDOUT, 0x07);
        }

        void InputThread()
        {
            while (!close_input_thread) {
                _getch(); // Block until user presses a key
                if (close_input_thread) return;

                // Temporarily lock mutex to set input mode
                mutex.lock();
                input_in_progress = true;
                printf(">>> ");
                mutex.unlock();

                // Read user input and handle cancellation from destructor
                std::string command;
                try {
                    std::getline(std::cin, command);
                }
                catch (std::ifstream::failure& e) {
                    if (close_input_thread) return;
                    else throw e;
                }

                // Print queued log messages
                mutex.lock();
                input_in_progress = false;
                while (!msg_queue.empty()) {
                    QueuedLogMessage& msg = msg_queue.front();
                    LogInternal(msg.source.c_str(), msg.severity.c_str(), msg.message.c_str());
                    msg_queue.pop();
                }
                mutex.unlock();

                // Run command if it is not empty
                if (!command.empty()) Dep<CommandMan>()->RunCommand(command.c_str());
            }
        }

    public:
        WindowsCLIImp(const bool* success) : load_success(*success)
        {
            if (!load_success)
                return;

            // TODO: Fetch from core TOML config
            colors[Logger::SevDebug] = 0x0808;
            colors[Logger::SevInfo] = 0x0907;
            colors[Logger::SevWarn] = 0x0E07;
            colors[Logger::SevError] = 0x0C07;

            if (GetConsoleWindow() || AllocConsole()) {
                HWND hConsole = GetConsoleWindow();

                FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;

                freopen_s(&fpstdin, "CONIN$", "r", stdin);
                freopen_s(&fpstdout, "CONOUT$", "w", stdout);
                freopen_s(&fpstderr, "CONOUT$", "w", stderr);

                std::cout.clear();
                std::clog.clear();
                std::cerr.clear();
                std::cin.clear();

                // Remove hConsole close button to prevent accidentally closing the game process
                HMENU hMenu = GetSystemMenu(hConsole, FALSE);
                if (hMenu != nullptr) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

                input_thread = std::thread([this]() { InputThread(); });
                console_loaded = true;
            }
            else
                Dep<Logger>()->Error(this, "AllocConsole failed! error = {}", GetLastError());
        }

        ~WindowsCLIImp() override {
            if (console_loaded) {
                close_input_thread = true;
                // Cancel read IO operation
                auto hIn = GetStdHandle(STD_INPUT_HANDLE);
                if (!CancelIoEx(hIn, nullptr)) {
                    // Failed to cancel IO operation, just terminate thread instead
                    TerminateThread((HANDLE)input_thread.native_handle(), 0);
                }
                input_thread.join();

                fclose(stdin);
                fclose(stdout);
                fclose(stderr);
                FreeConsole();
            }
        }

        bool IsUnloadable() const override
        { return true; }

        void Show() override
        {
            HWND hConsole = GetConsoleWindow();
            if (hConsole) ShowWindow(hConsole, SW_SHOW);
        }

        void Hide() override
        {
            HWND hConsole = GetConsoleWindow();
            if (hConsole) ShowWindow(hConsole, SW_HIDE);
        };

        void Clear() override
        {
            std::lock_guard<decltype(mutex)> lock(mutex);

            COORD tl = {0, 0};
            CONSOLE_SCREEN_BUFFER_INFO s;
            HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
            GetConsoleScreenBufferInfo(console, &s);
            DWORD written, cells = s.dwSize.X * s.dwSize.Y;
            FillConsoleOutputCharacterA(console, ' ', cells, tl, &written);
            FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
            SetConsoleCursorPosition(console, tl);
        };

        void SetLogSeverityColor(const char *sev_name, uint8_t sev_color, uint8_t msg_color) override
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            colors[sev_name] = ((uint16_t) sev_color << 8) | (uint16_t) msg_color;
        };
    };
}