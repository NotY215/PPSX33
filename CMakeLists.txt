cmake_minimum_required(VERSION 3.20)
project(PS3Recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Everything lands in build/dist so native tools and the GUI share one folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER ${cfg} cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} ${CMAKE_BINARY_DIR}/dist)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} ${CMAKE_BINARY_DIR}/dist)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} ${CMAKE_BINARY_DIR}/dist)
endforeach()

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
# ps3rt.dll — prebuilt game runtime (headers stay separate for lift includes)
# ------------------------------------------------------------------
if(WIN32)
    add_library(ps3rt SHARED
        ${CMAKE_SOURCE_DIR}/runtime/ps3rt.cpp
        ${CMAKE_SOURCE_DIR}/runtime/rsx_stub.cpp
        ${CMAKE_SOURCE_DIR}/runtime/spu_stub.cpp
        ${CMAKE_SOURCE_DIR}/runtime/audio_save.cpp
        ${CMAKE_SOURCE_DIR}/runtime/assets.cpp
    )
    target_include_directories(ps3rt PRIVATE ${CMAKE_SOURCE_DIR}/runtime)
    target_compile_definitions(ps3rt PRIVATE PS3RT_BUILD_DLL=1)
    if(MSVC)
        target_compile_options(ps3rt PRIVATE /bigobj)
    endif()
    target_link_libraries(ps3rt PRIVATE d3d11 dxgi d3dcompiler user32 gdi32 ole32 winmm)
    set_target_properties(ps3rt PROPERTIES
        PREFIX ""
        OUTPUT_NAME "ps3rt"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    )
    add_custom_command(TARGET ps3rt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/runtime
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3rt> ${CMAKE_BINARY_DIR}/dist/runtime/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:ps3rt> ${CMAKE_BINARY_DIR}/dist/runtime/
        COMMENT "Install ps3rt.dll/lib into dist/runtime"
        VERBATIM)
endif()

set(PS3RT_DIST_HEADERS
    ppu_runtime.h ppu_runtime_core.h ppu_runtime_core_a.h ppu_runtime_core_b.h
    ppu_runtime_run.h ps3rt_internal.h ps3rt_stage.h ps3rt_b.inl
    nid_table.h hle_registry.h lv2_syscalls.inl
)
add_custom_target(copy_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/runtime
    COMMENT "Preparing dist/runtime (headers only)"
)
foreach(_h ${PS3RT_DIST_HEADERS})
    if(EXISTS "${CMAKE_SOURCE_DIR}/runtime/${_h}")
        add_custom_command(TARGET copy_runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/runtime/${_h}"
                    "${CMAKE_BINARY_DIR}/dist/runtime/${_h}"
            VERBATIM)
    endif()
endforeach()

# Ship redist/ next to the app (README + VS Community install notes).
# Compilers-files is intentionally NOT copied — toolchain is system VS.
if(EXISTS "${CMAKE_SOURCE_DIR}/redist")
    add_custom_command(TARGET copy_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/redist"
                "${CMAKE_BINARY_DIR}/dist/redist"
        COMMENT "Copying redist/ (VS setup guide)"
        VERBATIM
    )
else()
    message(STATUS "redist/ not found — dist will ship without redist docs")
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
    if(TARGET ps3rt)
        add_dependencies(PS3Recomp ps3rt)
        add_dependencies(copy_runtime ps3rt)
    endif()
else()
    message(WARNING "dotnet not found on PATH – GUI target (PS3Recomp) will be skipped. Install .NET 8 SDK.")
endif()

message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/dist")
message(STATUS "Toolchain: system Visual Studio (see dist/redist/README.txt) — no Compilers-files")
message(STATUS "After build, run:  ${CMAKE_BINARY_DIR}/dist/PS3Recomp.exe")


# ------------------------------------------------------------------
# Modular layout (Processor / GPU / Game-Stuff)
# See STRUCTURE.md — monoliths in runtime/ remain linked until absorbed.
# ------------------------------------------------------------------
option(PPSX33_BUILD_MODULE_STUBS "Compile Processor/GPU/Game-Stuff skeleton units into ps3rt" OFF)

if(PPSX33_BUILD_MODULE_STUBS AND TARGET ps3rt)
    file(GLOB_RECURSE PPSX33_MODULE_SOURCES
        ${CMAKE_SOURCE_DIR}/Processor/*.cpp
        ${CMAKE_SOURCE_DIR}/GPU/*.cpp
        ${CMAKE_SOURCE_DIR}/Game-Stuff/*.cpp
    )
    if(PPSX33_MODULE_SOURCES)
        target_sources(ps3rt PRIVATE ${PPSX33_MODULE_SOURCES})
        target_include_directories(ps3rt PRIVATE
            ${CMAKE_SOURCE_DIR}/Processor/PPU
            ${CMAKE_SOURCE_DIR}/Processor/SPU
            ${CMAKE_SOURCE_DIR}/GPU
            ${CMAKE_SOURCE_DIR}/GPU/dx11
            ${CMAKE_SOURCE_DIR}/GPU/gl
            ${CMAKE_SOURCE_DIR}/GPU/vk
            ${CMAKE_SOURCE_DIR}/Game-Stuff/Audio
            ${CMAKE_SOURCE_DIR}/Game-Stuff/Video
        )
        message(STATUS "PPSX33_BUILD_MODULE_STUBS=ON — module skeletons linked into ps3rt")
    endif()
endif()

# Always surface the layout doc next to the app when present
if(EXISTS "${CMAKE_SOURCE_DIR}/STRUCTURE.md")
    add_custom_command(TARGET copy_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/STRUCTURE.md"
                "${CMAKE_BINARY_DIR}/dist/STRUCTURE.md"
        COMMENT "Copy STRUCTURE.md to dist"
        VERBATIM)
endif()
