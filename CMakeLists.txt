# ASM_MASM optional — enable in MSVC builds when assembler present
# enable_language(ASM_MASM)
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
    ${CMAKE_SOURCE_DIR}/src/processor/PPU/translate
    ${CMAKE_SOURCE_DIR}/src/RSX/present
    ${CMAKE_SOURCE_DIR}/src/RSX/fifo
    ${CMAKE_SOURCE_DIR}/src/core/import
    ${CMAKE_SOURCE_DIR}/src/native/x64
  ${CMAKE_SOURCE_DIR}/src/core/loaders
  ${CMAKE_SOURCE_DIR}/src/core/fs
  ${CMAKE_SOURCE_DIR}/src/processor/PPU
)
target_compile_definitions(ps3core PRIVATE PS3CORE_BUILD=1)
if(WIN32)
  set_target_properties(ps3core PROPERTIES PREFIX "")
endif()


# ---- Modular graphics DLLs (rebuild independently via PPSX33.exe -render) ----
if(WIN32)
  add_library(ppsx33_resolution SHARED src/RSX/resolution/rsx_resolution.c)
  target_include_directories(ppsx33_resolution PUBLIC ${CMAKE_SOURCE_DIR}/src/RSX/resolution)
  target_compile_definitions(ppsx33_resolution PRIVATE PPSX33_RES_BUILD=1)
  set_target_properties(ppsx33_resolution PROPERTIES PREFIX "" OUTPUT_NAME "resolution")
  set_source_files_properties(src/RSX/resolution/rsx_resolution.c PROPERTIES LANGUAGE C)

endif()

# ---- ps3rt (game runtime) — sources under src/ only (no runtime/) ----
if(WIN32)
  add_library(ps3rt SHARED
    src/core/hle_kernel.cpp
    src/core/assets.cpp
    src/RSX/rsx_device.cpp
    src/RSX/Backends/Vulkan/rsx_vk_backend.cpp
    src/RSX/Backends/Vulkan/rsx_vk_memory.cpp
    src/RSX/Backends/Vulkan/rsx_vk_command.cpp
    src/RSX/Backends/Vulkan/rsx_vk_pipelines.cpp
    src/RSX/rsx_fifo.cpp
    src/RSX/rsx_framebuffer.cpp
    src/RSX/rsx_memory.cpp
    src/RSX/rsx_state.cpp
    src/RSX/rsx_texture.cpp
    src/RSX/rsx_shader_compiler.cpp
    src/RSX/rsx_shader_decoder.cpp
    src/native/x64/ps3rt_be.c
    src/core/import/import_stub_map.c
    src/RSX/fifo/rsx_method_table.c
    src/RSX/present/rsx_present_queue.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
    src/core/stats/host_stats.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/processor/SPU/spu_interpreter.cpp
    src/games-stuff/Audio/audio_device.cpp
  )
  target_include_directories(ps3rt PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/import
    ${CMAKE_SOURCE_DIR}/src/native/x64
    ${CMAKE_SOURCE_DIR}/src/processor/PPU
    ${CMAKE_SOURCE_DIR}/src/processor/PPU/translate
    ${CMAKE_SOURCE_DIR}/src/processor/SPU
    ${CMAKE_SOURCE_DIR}/src/RSX
    ${CMAKE_SOURCE_DIR}/src/RSX/fifo
    ${CMAKE_SOURCE_DIR}/src/RSX/present
    ${CMAKE_SOURCE_DIR}/src/RSX/draw
    ${CMAKE_SOURCE_DIR}/src/RSX/render
    ${CMAKE_SOURCE_DIR}/src/RSX/resolution
    ${CMAKE_SOURCE_DIR}/src/core/stats
    ${CMAKE_SOURCE_DIR}/src/RSX/Backends/Vulkan
    ${CMAKE_SOURCE_DIR}/src/games-stuff/Audio
  )
  set_source_files_properties(
    src/native/x64/ps3rt_be.c
    src/core/import/import_stub_map.c
    src/RSX/fifo/rsx_method_table.c
    src/RSX/present/rsx_present_queue.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
    src/core/stats/host_stats.c
    PROPERTIES LANGUAGE C)
  target_compile_definitions(ps3rt PRIVATE PS3RT_BUILD_DLL=1)
  if(MSVC)
    target_compile_options(ps3rt PRIVATE /bigobj)
  endif()
  target_link_libraries(ps3rt PRIVATE user32 gdi32 ole32 uuid windowscodecs winmm xaudio2 psapi
    $<TARGET_NAME_IF_EXISTS:ppsx33_resolution>)
  set_target_properties(ps3rt PROPERTIES PREFIX "" OUTPUT_NAME "ps3rt")
  # Shim targets for PPSX33.exe -render (full source split continues next milestones)
  add_custom_target(ppsx33_api DEPENDS ps3rt)
  add_custom_target(ppsx33_rsx DEPENDS ps3rt)
  add_custom_target(ppsx33_render DEPENDS ps3rt)
  # Single layout: ps3rt.dll / ps3rt.lib stay in dist/ next to PPSX33.exe
  # (do not also copy into dist/runtime/ — headers only live there)
  add_custom_command(TARGET ps3rt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
    COMMAND ${CMAKE_COMMAND}
      -DPS3_DIST=${CMAKE_BINARY_DIR}/dist
      -P ${CMAKE_SOURCE_DIR}/cmake/CopyVulkanRuntime.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_resolution> ${CMAKE_BINARY_DIR}/dist/resolution.dll
    COMMENT "Ensure dist/ has ps3rt + vulkan-1.dll (single location next to EXE)"
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
  COMMENT "Preparing dist/runtime headers only (no DLL/LIB duplicate)"
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
    COMMENT "Building GUI → dist/PPSX33.exe"
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
message(STATUS "Layout: dist/ps3rt.dll + dist/vulkan-1.dll next to EXE; dist/runtime/ = headers only")
