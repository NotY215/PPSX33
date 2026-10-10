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

# Compilers-files is often locked (AV, Explorer, cl.exe, previous game.exe).
# NEVER delete the destination (rm -rf fails and kills the whole rebuild).
# Just ensure the folder exists and robocopy merge; always exit 0 so ninja continues.
if(EXISTS "${CMAKE_SOURCE_DIR}/Compilers-files")
    if(WIN32)
        # Use a small .bat so nested quoting does not break under ninja/cmd.
        file(WRITE "${CMAKE_BINARY_DIR}/copy_compilers.bat"
"@echo off
set SRC=%~1
set DST=%~2
if not exist \"%DST%\" mkdir \"%DST%\"
robocopy \"%SRC%\" \"%DST%\" /E /XO /NFL /NDL /NJH /NJS /nc /ns /np >nul 2>&1
rem robocopy: 0-7 = success / partial; >=8 = failure — we still continue
exit /b 0
")
        add_custom_command(TARGET copy_runtime POST_BUILD
            COMMAND cmd /C "\"${CMAKE_BINARY_DIR}/copy_compilers.bat\" \"${CMAKE_SOURCE_DIR}/Compilers-files\" \"${CMAKE_BINARY_DIR}/dist/Compilers-files\""
            COMMENT "Copying Compilers-files (best-effort, never fails build)"
            VERBATIM
        )
    else()
        add_custom_command(TARGET copy_runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/Compilers-files"
                    "${CMAKE_BINARY_DIR}/dist/Compilers-files"
            COMMENT "Copying Compilers-files"
        )
    endif()
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

    add_dependencies(PS3Recomp ps3core copy_runtime)
else()
    message(WARNING "dotnet not found on PATH – GUI target (PS3Recomp) will be skipped. Install .NET 8 SDK.")
endif()

message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/dist")
message(STATUS "After build, run:  ${CMAKE_BINARY_DIR}/dist/PS3Recomp.exe")
