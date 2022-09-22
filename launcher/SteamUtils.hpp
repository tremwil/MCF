
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <include/third-party/windows_include.h>
#include <filesystem>
#include <iostream>
#include "ValveFileVDF/vdf_parser.hpp"

using namespace std::filesystem;

bool GetSteamInstallDirForGame(int game_appid, path& out_path) {
    char steamInstallPath[MAX_PATH];
    DWORD buff_size = sizeof(steamInstallPath);
    LONG error = RegGetValueA(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Wow6432Node\Valve\Steam)", "InstallPath", RRF_RT_REG_SZ, nullptr,
                              (LPDWORD)steamInstallPath, &buff_size);
    if (error != ERROR_SUCCESS) {
        printf("Failure querying the Steam installation path from registry (error code %08lx)\n", error);
        return false;
    }

    path steam_path = steamInstallPath;
    auto lib_folders_vdf = steam_path / "steamapps/libraryfolders.vdf";

    if (!(exists(lib_folders_vdf) && is_regular_file(lib_folders_vdf))) {
        printf("Failed to find libraryfolders.vdf file\n");
        return false;
    }
    std::ifstream file(lib_folders_vdf);

    try {
        const auto vdf = tyti::vdf::read(file);
        for (const auto& [index, lib_folder] : vdf.childs) {
            if (!lib_folder->attribs.contains("path") || !lib_folder->childs.contains("apps"))
                continue;

            for (const auto& [app_id, idk] : lib_folder->childs["apps"]->attribs) {
                if (atoi(app_id.c_str()) == game_appid) {
                    out_path = lib_folder->attribs["path"];
                    return true;
                }
            }
        }
        printf("Could not find game install directory in libraryfolders.vdf. Make sure that the game is installed and that the right app id was provided\n");
        return false;
    }
    catch (std::runtime_error& e) {
        printf("libraryfolders.vdf parsing failed with the following error:\n%s\n", e.what());
        return false;
    }
}

bool GetSteamGameInstallDir(int game_appid, path& out_path)
{
    path steam_install_dir;
    if (!GetSteamInstallDirForGame(game_appid, steam_install_dir)) {
        return false;
    }
    auto manifest = steam_install_dir / "steamapps" / ("appmanifest_" + std::to_string(game_appid) + ".acf");
    if (!(exists(manifest) && is_regular_file(manifest))) {
        printf("Could not find Steam app manifest file for app ID %d in the following path:\n    %s\n", game_appid, manifest.string().c_str());
        return false;
    }

    try {
        std::ifstream file(manifest);
        const auto vdf = tyti::vdf::read(file);
        if (!vdf.attribs.contains("installdir")) {
            printf("Install directory is not present in Steam app manifest\n");
            return false;
        }
        out_path = steam_install_dir / "steamapps" / "common" / vdf.attribs.at("installdir");
        return exists(out_path) && is_directory(out_path);
    }
    catch (std::runtime_error& e) {
        printf("appmanifest VDF parsing failed with the following error:\n%s\n", e.what());
        return false;
    }
}