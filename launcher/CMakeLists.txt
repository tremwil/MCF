cmake_minimum_required(VERSION 3.23)
project(MCFLauncher VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)

include_directories("../third-party")
add_executable(MCFLauncher Launcher.cpp SteamUtils.hpp)

target_link_options(MCFLauncher PRIVATE -static-libstdc++)
