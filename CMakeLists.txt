cmake_minimum_required(VERSION 3.23)
project(MCF VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 20)

set(OUT_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${OUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

add_subdirectory(launcher)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(MCF SHARED dllmain.cpp include/core/Component.h include/core/FixedString.h include/core/SharedInterface.h include/core/EventMan.h include/core/Logger.h include/core/ComponentMan.h include/cli/CommandMan.h include/cli/WindowsCLI.h include/memory/MemoryPatcher.h include/core/Export.h include/core/Export.cpp src/cli/WindowsCLIImp.cpp src/cli/WindowsCLIImp.h src/core/LoggerImp.cpp src/core/LoggerImp.h src/core/EventManImp.cpp src/core/EventManImp.h src/core/ComponentManImp.cpp src/core/ComponentManImp.h src/core/Bootstrap.cpp src/cli/CommandManImp.h src/cli/CommandManImp.cpp src/core/Bootstrap.h include/util/PathUtils.h src/core/CoreSettings.h src/core/CoreSettings.cpp include/memory/AobScan.h include/util/TemplateUtils.h include/hook/HookChainNode.h include/hook/HookMan.h include/hook/StaticThunk.h include/memory/RTTI.h include/memory/MemRegion.h src/memory/MemoryPatcherImp.cpp src/memory/MemoryPatcherImp.h src/memory/AobScannerImp.cpp src/memory/AobScannerImp.h src/memory/RTTIScannerImp.cpp src/memory/RTTIScannerImp.h include/windows_include.h src/memory/aob_scanners/AhoCorasickScanner.cpp src/memory/aob_scanners/AhoCorasickScanner.h include/util/HexDump.h src/steamstub/SteamStubHeader.hpp src/memory/VirtualAllocNear.hpp src/memory/aob_scanners/AobCache.cpp src/memory/aob_scanners/AobCache.h)

target_include_directories(MCF PUBLIC ./ ./include/ ./third-party/)
target_link_options(MCF PRIVATE -static-libstdc++)

target_compile_definitions(MCF PRIVATE NOMINMAX)

# Register Zydis dependency.
# Disable build of tools and examples.
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory(third-party/zydis)
target_link_libraries("MCF" PRIVATE "Zydis")

if (CMAKE_CL_64)
    set(CMAKE_CXX_FLAGS "/arch:AVX2")
elseif (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-O3 -Wall -Werror")
    target_compile_options(MCF PUBLIC "-mavx2")
endif()

#file(GLOB RESOURCES "resources/*.*")
#file(COPY ${RESOURCES} DESTINATION ${OUT_DIR})
