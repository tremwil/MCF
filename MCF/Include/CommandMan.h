#pragma once
#include "SharedInterface.h"
#include "EventMan.h"

namespace MCF
{
	class CommandBase
	{
	public:
		virtual void Run(const char* args[], size_t count) = 0;
		virtual const char* Name() const = 0;
		virtual const char* HelpMessage() const = 0;
	};

	class CommandMan : public SharedInterface<CommandMan, "MCF_COMMAND_MAN_001">
	{
	public:
		virtual bool Register(CommandBase* cmd) = 0;

		virtual bool Unregister(CommandBase* cmd) = 0;

		virtual void RunCommand(const char* command_string) = 0;
	};

	/// <summary>
	/// Command object using a std::function to allow registering callbacks on any callable object.
	/// May be registered on creation or manually. Unregistered automatically when destroyed.
	/// </summary>
	class Command : public CommandBase
	{
	protected:
		std::function<void(const char* args[], size_t count)> fun;
		const char* name;
		const char* help_message;

		bool TryRegister()
		{
			auto man = CommandMan::Get();
			if (!man) return false;
			man->Register(this);
			return true;
		}

		virtual void Run(const char* args[], size_t count) override { fun(args, count); }
		virtual const char* Name() const override { return name; }
		virtual const char* HelpMessage() const override { return help_message; }

	public:

		/// <summary>
		/// Registers this command with a function to a member function pointer.
		/// </summary>
		template<typename TObj>
		bool Register(void(TObj::* cb)(const char*[], size_t), TObj* instance)
		{
			fun = std::bind(cb, instance);
			return TryRegister();
		}

		/// <summary>
		/// Registers this command with a general callable object.
		/// </summary>
		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, const char*[], size_t>
		bool Register(TCallable cb)
		{
			fun = cb;
			return TryRegister();
		}

		/// <summary>
		/// Unregisters this command object. 
		/// </summary>
		bool Unregister()
		{
			auto man = CommandMan::Get();
			if (!man) return false;
			man->Unregister(this);
			return true;
		}

		Command() { }

		/// <summary>
		/// Constructs and registers this command with a function to a member function pointer.
		/// </summary>
		template<typename TObj>
		Command(void(TObj::* cb)(const char*[], size_t), TObj* instance)
		{
			Register(cb, instance);
		}

		/// <summary>
		/// Constructs and registers this command with a general callable object.
		/// </summary>
		template<typename TCallable> requires std::is_invocable_r_v<void, TCallable, const char*[], size_t>
		Command(TCallable cb)
		{
			Register(cb);
		}

		~Command() { Unregister(); }
	};
}