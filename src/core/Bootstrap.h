
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include "third-party/toml11/toml.hpp"
#include "ComponentManImp.h"
#include <filesystem>
#include "hook/HookChainNode.h"

namespace MCF
{
    class Bootstrap
    {
    public:
        std::filesystem::path launcher_dir;
        std::unique_ptr<ComponentManImp> comp_man;

        void Init(bool used_launcher);

        static Bootstrap& Get();

        Bootstrap(const Bootstrap&) = delete;
        Bootstrap(const Bootstrap&&) = delete;
    private:
        Bootstrap();
    };
}