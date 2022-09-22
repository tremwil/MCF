
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/cli/CommandMan.h"
#include "core/Logger.h"

#include <unordered_map>
#include <mutex>

namespace MCF
{
    class CommandManImp final : public SharedInterfaceImp<CommandMan, CommandManImp, DependencyList<Logger>>
    {
    private:
        bool load_success = false;

        std::unordered_map<std::string, CommandBase*> commands;
        std::recursive_mutex mutex;

        // This is stupid but we can't use the RAII Command class here (CommandMan wouldn't be constructed yet)
        struct HelpCommand : public CommandBase
        {
            CommandManImp* cmdMan;
            HelpCommand(CommandManImp* cmdMan) : cmdMan(cmdMan) { }

            void Run(const char* args[], size_t count) override {
                cmdMan->HelpCommand(args, count);
            }

            const char* Name() const override {
                return "help";
            }

            const char* Description() const override {
                return "Prints the list of all available commands";
            }
        };

        HelpCommand helpCommand = { this };
        void HelpCommand(const char* args[], size_t count);

    public:
        CommandManImp(const bool* success);

        bool Register(CommandBase *cmd) override;

        bool Unregister(CommandBase *cmd) override;

        void RunCommand(const char *command_string) override;
    };
}