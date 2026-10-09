cmake_minimum_required(VERSION 3.20)
project(PS3Recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Everything lands in build/dist so native tools and the GUI share one folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)

# ------------------------------------------------------------------
# ps3core.dll  – ELF loader, PPU lifter, project manager, build driver
# ------------------------------------------------------------------
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

# ------------------------------------------------------------------
# ps3_cli.exe – command-line front-end (same API as the GUI)
# ------------------------------------------------------------------
add_executable(ps3_cli src/cli/ps3_cli.cpp)
target_link_libraries(ps3_cli PRIVATE ps3core)

# ------------------------------------------------------------------
# Copy runtime sources + optional Compilers-files into dist/
# ------------------------------------------------------------------
add_custom_target(copy_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/runtime
            ${CMAKE_BINARY_DIR}/dist/runtime
    COMMENT "Copying runtime/ sources"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/Compilers-files")
    add_custom_command(TARGET copy_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/Compilers-files
                ${CMAKE_BINARY_DIR}/dist/Compilers-files
        COMMENT "Copying Compilers-files"
    )
endif()

# ------------------------------------------------------------------
# GUI (C# WinForms) – built as part of the same "Rebuild All"
# Requires a system-wide .NET 8 SDK (dotnet on PATH).
# ------------------------------------------------------------------
find_program(DOTNET_EXE dotnet)

if(DOTNET_EXE)
    add_custom_target(PS3Recomp ALL
        COMMAND ${DOTNET_EXE} publish
                ${CMAKE_SOURCE_DIR}/src/gui/PS3Recomp.Gui.csproj
                -c Release
                -o ${CMAKE_BINARY_DIR}/dist
                --self-contained false
                /p:AppendTargetFrameworkToOutputPath=false
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ps3core copy_runtime
        COMMENT "Building GUI → build/dist/PS3Recomp.exe"
        VERBATIM
    )

    # Make the common "all" target also guarantee the GUI is up to date
    add_dependencies(PS3Recomp ps3core copy_runtime)
else()
    message(WARNING "dotnet not found on PATH – GUI target (PS3Recomp) will be skipped. Install .NET 8 SDK.")
endif()

message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/dist")
message(STATUS "After build, run:  ${CMAKE_BINARY_DIR}/dist/PS3Recomp.exe")
