cmake_minimum_required(VERSION 3.20)
project(PS3Recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)

# ps3core: ELF loader, PPU lifter, project manager, build driver (C++ + C). Used by GUI and CLI.
add_library(ps3core SHARED
    src/core/ps3_util.c
    src/core/elf_loader.cpp
    src/core/ppu_lifter.cpp
    src/core/project.cpp
    src/core/ps3core_api.cpp)
target_compile_definitions(ps3core PRIVATE PS3CORE_BUILD=1)
if(NOT MSVC)
    target_compile_options(ps3core PRIVATE -Wall -Wextra)
endif()
if(WIN32)
    set_target_properties(ps3core PROPERTIES PREFIX "")
endif()

# ps3_cli: command line front end (no GUI), used by tests/run_smoke_test.*
add_executable(ps3_cli src/cli/ps3_cli.cpp)
target_link_libraries(ps3_cli PRIVATE ps3core)

# Runtime sources are NOT compiled here: they are copied next to the app and compiled per game
# by the build driver (Phase 6), so they must ship as source in <app>/runtime.
add_custom_target(copy_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/runtime ${CMAKE_BINARY_DIR}/dist/runtime
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Compilers-files ${CMAKE_BINARY_DIR}/dist/Compilers-files)

# The C# GUI is built with:  dotnet publish src/gui -c Release -o build/dist
