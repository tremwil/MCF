
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include <sstream>
#include <iomanip>
#include "CommandManImp.h"
#include "core/Export.h"

namespace MCF
{
    MCF_COMPONENT_EXPORT(CommandManImp);

    CommandManImp::CommandManImp(const bool* success) : load_success(*success)
    {
        if (!load_success) return;

        Register(&helpCommand);
    }

    void CommandManImp::HelpCommand(const char **args, size_t count)
    {
        std::stringstream ss;

        ss << "There are " << commands.size() << " registered commands:" << std::endl;
        int padding = std::max_element(commands.begin(), commands.end(),
                                       [](const auto& p1, const auto& p2) {
            return p1.first.length() < p2.first.length();
        })->first.length() + 4;

        for (const auto& [name, cmd] : commands) {
            ss << std::setfill(' ') << std::setw(padding) << std::left << name << std::setw(0) << cmd->Description() << std::endl;
        }

        Dep<Logger>()->Info(this, ss.str());
    }

    bool CommandManImp::Register(CommandBase *cmd)
    {
        if (!load_success) return false;

        std::lock_guard<decltype(mutex)> lock(mutex);

        if (commands.contains(cmd->Name())) {
            Dep<Logger>()->Error(this, "Command with name '{}' already exists", cmd->Name());
            return false;
        }

        commands[cmd->Name()] = cmd;
        return true;
    }

    bool CommandManImp::Unregister(CommandBase *cmd)
    {
        if (!load_success) return false;

        std::lock_guard<decltype(mutex)> lock(mutex);

        if (!commands.contains(cmd->Name())) {
            Dep<Logger>()->Error(this, "Command '{}' does not exist; cannot unregister", cmd->Name());
            return false;
        }

        commands.erase(cmd->Name());
        return true;
    }

    void CommandManImp::RunCommand(const char *command_string)
    {
        if (!load_success) return;

        std::lock_guard<decltype(mutex)> lock(mutex);

        std::stringstream ss;
        std::vector<std::string> args;

        bool in_quotes = false;
        for (int i = 0; command_string[i]; i++) {
            if (command_string[i] == '"') {
                in_quotes = !in_quotes;
                continue;
            }
            if (in_quotes) {
                char escaped = command_string[i];
                if (command_string[i] == '\\') {
                    switch (command_string[i+1]) {
                        case 'n':
                            escaped = '\n';
                            break;
                        case 't':
                            escaped = '\t';
                            break;
                        case '\\':
                            escaped = '\\';
                            break;
                        case '"':
                            escaped = '"';
                            break;
                        default:
                            Dep<Logger>()->Error(this, "Unknown escape sequence '\\{}' in command '{}'",
                                                 command_string[i+1], command_string);
                            return;
                    }
                    i++;
                }
                ss << escaped;
            }
            else if (std::isspace(command_string[i]) && !ss.str().empty()) {
                args.push_back(ss.str());
                ss.clear();
            }
            else ss << command_string[i];
        }
        if (!ss.str().empty())
            args.push_back(ss.str());

        if (args.empty()) {
            Dep<Logger>()->Error(this, "Empty command");
            return;
        }
        else if (!commands.contains(args[0])) {
            Dep<Logger>()->Error(this, "Command '{}' not found", args[0]);
            return;
        }

        std::vector<const char*> args_cstr;
        for (int i = 1; i < args.size(); i++)
            args_cstr.push_back(args[i].c_str());

        commands[args[0]]->Run(args_cstr.data(), args_cstr.size());
    }
}