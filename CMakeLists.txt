cmake_minimum_required(VERSION 3.20)
project(PS3Recomp LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  string(TOUPPER ${cfg} U)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${U} ${CMAKE_BINARY_DIR}/dist)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${U} ${CMAKE_BINARY_DIR}/dist)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${U} ${CMAKE_BINARY_DIR}/dist)
endforeach()

# ---- ps3core (host tools) ----
add_library(ps3core SHARED
  src/core/ps3_util.c
  src/core/loaders/elf_loader.cpp
  src/processor/PPU/ppu_lifter.cpp
  src/core/project.cpp
  src/core/ps3core_api.cpp
  src/core/memory.cpp
  src/core/fs/vfs.cpp
  src/core/fs/psarc_parser.cpp
  src/core/fs/sfo_parser.cpp
)
target_include_directories(ps3core PRIVATE
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/core/loaders
  ${CMAKE_SOURCE_DIR}/src/core/fs
  ${CMAKE_SOURCE_DIR}/src/processor/PPU
)
target_compile_definitions(ps3core PRIVATE PS3CORE_BUILD=1)
if(WIN32)
  set_target_properties(ps3core PROPERTIES PREFIX "")
endif()

# ---- ps3rt (game runtime) — sources under src/ only (no runtime/) ----
if(WIN32)
  add_library(ps3rt SHARED
    src/core/hle_kernel.cpp
    src/core/assets.cpp
    src/RSX/rsx_device.cpp
    src/RSX/rsx_fifo.cpp
    src/processor/SPU/spu_interpreter.cpp
    src/games-stuff/Audio/audio_device.cpp
  )
  target_include_directories(ps3rt PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/processor/PPU
    ${CMAKE_SOURCE_DIR}/src/processor/SPU
    ${CMAKE_SOURCE_DIR}/src/RSX
    ${CMAKE_SOURCE_DIR}/src/RSX/Backends/DX11
    ${CMAKE_SOURCE_DIR}/src/games-stuff/Audio
  )
  target_compile_definitions(ps3rt PRIVATE PS3RT_BUILD_DLL=1)
  if(MSVC)
    target_compile_options(ps3rt PRIVATE /bigobj)
  endif()
  target_link_libraries(ps3rt PRIVATE d3d11 dxgi d3dcompiler user32 gdi32 ole32 winmm xaudio2)
  set_target_properties(ps3rt PROPERTIES PREFIX "" OUTPUT_NAME "ps3rt")
  add_custom_command(TARGET ps3rt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/runtime
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3rt> ${CMAKE_BINARY_DIR}/dist/runtime/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:ps3rt> ${CMAKE_BINARY_DIR}/dist/runtime/
    COMMENT "Install ps3rt into dist/runtime"
  )
endif()

# Headers needed by generated game sources (ship next to prebuilt DLL)
set(PS3RT_DIST_HEADERS
  ppu_runtime.h ppu_runtime_core.h ppu_runtime_core_a.h ppu_runtime_core_b.h
  ppu_runtime_run.h hle_kernel_internal.h hle_stage.h hle_kernel_syscalls.inl
  nid_table.h hle_registry.h lv2_syscalls.inl hle_status.h
)
add_custom_target(copy_runtime ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/runtime
  COMMENT "Preparing dist/runtime headers"
)
# Map header names from new locations
set(_HDR_MAP
  "src/processor/PPU/ppu_runtime.h:ppu_runtime.h"
  "src/processor/PPU/ppu_runtime_core.h:ppu_runtime_core.h"
  "src/processor/PPU/ppu_runtime_core_a.h:ppu_runtime_core_a.h"
  "src/processor/PPU/ppu_runtime_core_b.h:ppu_runtime_core_b.h"
  "src/processor/PPU/ppu_runtime_run.h:ppu_runtime_run.h"
  "src/core/hle_kernel_internal.h:hle_kernel_internal.h"
  "src/core/hle_stage.h:hle_stage.h"
  "src/core/hle_kernel_syscalls.inl:hle_kernel_syscalls.inl"
  "src/core/nid_table.h:nid_table.h"
  "src/core/hle_registry.h:hle_registry.h"
  "src/core/lv2_syscalls.inl:lv2_syscalls.inl"
  "src/core/hle_status.h:hle_status.h"
)
foreach(pair ${_HDR_MAP})
  string(REPLACE ":" ";" kv ${pair})
  list(GET kv 0 srcp)
  list(GET kv 1 name)
  if(EXISTS "${CMAKE_SOURCE_DIR}/${srcp}")
    add_custom_command(TARGET copy_runtime POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/${srcp}"
        "${CMAKE_BINARY_DIR}/dist/runtime/${name}"
      VERBATIM)
  endif()
endforeach()

if(EXISTS "${CMAKE_SOURCE_DIR}/redist")
  add_custom_command(TARGET copy_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/redist" "${CMAKE_BINARY_DIR}/dist/redist"
    COMMENT "Copy redist/"
    VERBATIM)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/STRUCTURE.md")
  add_custom_command(TARGET copy_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/STRUCTURE.md" "${CMAKE_BINARY_DIR}/dist/STRUCTURE.md"
    VERBATIM)
endif()

find_program(DOTNET_EXE dotnet)
if(DOTNET_EXE)
  add_custom_target(PS3Recomp ALL
    COMMAND ${DOTNET_EXE} publish
      ${CMAKE_SOURCE_DIR}/src/UI/PS3Recomp.Gui.csproj
      -c Release -o ${CMAKE_BINARY_DIR}/dist
      --self-contained false
      /p:AppendTargetFrameworkToOutputPath=false
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ps3core copy_runtime
    COMMENT "Building GUI → dist/PS3Recomp.exe"
    VERBATIM)
  add_dependencies(PS3Recomp ps3core copy_runtime)
  if(TARGET ps3rt)
    add_dependencies(PS3Recomp ps3rt)
    add_dependencies(copy_runtime ps3rt)
  endif()
else()
  message(WARNING "dotnet not found — GUI skipped")
endif()

message(STATUS "Layout: src/core, src/processor, src/RSX, src/games-stuff, src/UI")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/dist")
