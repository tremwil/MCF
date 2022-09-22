
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "include/core/SharedInterface.h"
#include "include/core/EventMan.h"

namespace MCF
{
    class CommandBase
    {
    public:
        virtual void Run(const char* args[], size_t count) = 0;
        virtual const char* Name() const = 0;
        virtual const char* Description() const = 0;
    };

    class CommandMan : public SharedInterface<CommandMan, "MCF_COMMAND_MAN_001">
    {
    public:
        virtual bool Register(CommandBase* cmd) = 0;

        virtual bool Unregister(CommandBase* cmd) = 0;

        virtual void RunCommand(const char* command_string) = 0;
    };

    /// Command object using a std::function to allow registering callbacks on any callable object.
    /// May be registered on creation or manually. Unregistered automatically when destroyed.
    class Command : public CommandBase
    {
    protected:
        std::function<void(const char* args[], size_t count)> fun;
        std::string name;
        std::string description;
        bool is_bound = false;

        Dependency<CommandMan> cmdMan;

        bool TryRegister()
        {
            if (!cmdMan || is_bound) return false;
            is_bound = cmdMan->Register(this);
            return is_bound;
        }

        void Run(const char* args[], size_t count) override { fun(args, count); }
        const char* Name() const override { return name.c_str(); }
        const char* Description() const override { return description.c_str(); }

    public:

        /// Registers this command with a function to a member function pointer.
        template<typename TObj>
        bool Register(void(TObj::* cb)(const char*[], size_t), TObj* instance)
        {
            if (is_bound && !Unregister()) return false;
            fun = std::bind(cb, instance);
            return TryRegister();
        }

        /// Registers this command with a general callable object.
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, const char*[], size_t>
        bool Register(TCallable cb)
        {
            if (is_bound && !Unregister()) return false;
            fun = cb;
            return TryRegister();
        }

        /// Unregisters this command object.
        bool Unregister()
        {
            if (!is_bound || !cmdMan) return false;
            is_bound = !cmdMan->Unregister(this);
            return !is_bound;
        }

        Command() = default;

        /// Constructs a command without registering it.
        Command(const char* name, const char* description) : name(name), description(description) { }

        /// Constructs and registers this command with a function to a member function pointer.
        template<typename TObj>
        Command(const char* name, const char* description, void(TObj::* cb)(const char*[], size_t), TObj* instance) :
            name(name), description(description)
        {
            Register(cb, instance);
        }

        /// Constructs and registers this command with a general callable object.
        template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, const char*[], size_t>
        Command(const char* name, const char* description, TCallable cb) : name(name), description(description)
        {
            Register(cb);
        }

        ~Command() { Unregister(); }
    };
}