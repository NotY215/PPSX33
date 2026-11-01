# ASM_MASM optional — enable in MSVC builds when assembler present
# enable_language(ASM_MASM)
cmake_minimum_required(VERSION 3.20)
project(PS3Recomp LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist)
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


# ---- Modular graphics DLLs (PPSX33.exe -render targets) ----
set(PPSX33_COMMON_INCLUDES
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
    ${CMAKE_SOURCE_DIR}/src/core/hle
  ${CMAKE_SOURCE_DIR}/src/RSX/Backends/Vulkan
  ${CMAKE_SOURCE_DIR}/src/games-stuff/Audio
)

if(WIN32)
  add_library(ppsx33_resolution SHARED src/RSX/resolution/rsx_resolution.c)
  target_include_directories(ppsx33_resolution PUBLIC ${CMAKE_SOURCE_DIR}/src/RSX/resolution)
  target_compile_definitions(ppsx33_resolution PRIVATE PPSX33_RES_BUILD=1)
  set_target_properties(ppsx33_resolution PROPERTIES
    PREFIX ""
    OUTPUT_NAME "resolution"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set_source_files_properties(src/RSX/resolution/rsx_resolution.c PROPERTIES LANGUAGE C)

  add_library(ppsx33_api SHARED
    src/RSX/Backends/Vulkan/rsx_vk_backend.cpp
    src/RSX/Backends/Vulkan/rsx_vk_memory.cpp
    src/RSX/Backends/Vulkan/rsx_vk_command.cpp
    src/RSX/Backends/Vulkan/rsx_vk_pipelines.cpp
  )
  target_include_directories(ppsx33_api PRIVATE ${PPSX33_COMMON_INCLUDES})
  target_compile_definitions(ppsx33_api PRIVATE PPSX33_API_BUILD=1)
  target_link_libraries(ppsx33_api PRIVATE user32)
  set_target_properties(ppsx33_api PROPERTIES
    PREFIX ""
    OUTPUT_NAME "api"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

  add_library(ppsx33_render SHARED
    src/RSX/present/rsx_present_queue.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
  )
  target_include_directories(ppsx33_render PRIVATE ${PPSX33_COMMON_INCLUDES})
  target_compile_definitions(ppsx33_render PRIVATE PPSX33_RENDER_BUILD=1)
  set_source_files_properties(
    src/RSX/present/rsx_present_queue.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
    PROPERTIES LANGUAGE C)
  target_link_libraries(ppsx33_render PRIVATE ppsx33_resolution)
  set_target_properties(ppsx33_render PROPERTIES
    PREFIX ""
    OUTPUT_NAME "render"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

  # rsx.dll: FIFO/state helpers (device stays in ps3rt to use HLE log/mem)
  add_library(ppsx33_rsx SHARED
    src/RSX/rsx_fifo.cpp
    src/RSX/rsx_framebuffer.cpp
    src/RSX/rsx_memory.cpp
    src/RSX/rsx_state.cpp
    src/RSX/rsx_texture.cpp
    src/RSX/rsx_shader_compiler.cpp
    src/RSX/rsx_shader_decoder.cpp
    src/RSX/fifo/rsx_method_table.c
    src/RSX/fifo/rsx_fifo_drain.c
    src/RSX/methods/rsx_nv4097_surface.c
    src/native/x64/ps3rt_be.c
    src/RSX/rsx_exports.c
  )
  target_include_directories(ppsx33_rsx PRIVATE ${PPSX33_COMMON_INCLUDES})
  target_compile_definitions(ppsx33_rsx PRIVATE PPSX33_RSX_BUILD=1)
  set_source_files_properties(src/RSX/fifo/rsx_method_table.c src/RSX/fifo/rsx_fifo_drain.c src/RSX/methods/rsx_nv4097_surface.c src/native/x64/ps3rt_be.c src/RSX/rsx_exports.c PROPERTIES LANGUAGE C)
  target_link_libraries(ppsx33_rsx PRIVATE ppsx33_render ppsx33_resolution)
  set_target_properties(ppsx33_rsx PROPERTIES
    PREFIX ""
    OUTPUT_NAME "rsx"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# ---- ps3rt (HLE + RSX device + SPU + audio); links modular DLLs ----
if(WIN32)
  add_library(ps3rt SHARED
    src/core/hle_kernel.cpp
    src/core/assets.cpp
    src/RSX/rsx_device.cpp
    src/core/import/import_stub_map.c
    src/core/stats/host_stats.c
    src/core/hle/sysmodule_progress.c
    src/core/hle/gcm/gcm_control_bridge.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/processor/SPU/spu_interpreter.cpp
    src/games-stuff/Audio/audio_device.cpp
  )
  target_include_directories(ps3rt PRIVATE
    ${PPSX33_COMMON_INCLUDES}
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
    src/core/import/import_stub_map.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/core/stats/host_stats.c
    src/core/hle/sysmodule_progress.c
    src/core/hle/gcm/gcm_control_bridge.c
    PROPERTIES LANGUAGE C)
  target_compile_definitions(ps3rt PRIVATE PS3RT_BUILD_DLL=1)
  if(MSVC)
    target_compile_options(ps3rt PRIVATE /bigobj)
  endif()
  target_link_libraries(ps3rt PRIVATE
    user32 gdi32 ole32 uuid windowscodecs winmm xaudio2 psapi
    ppsx33_api ppsx33_rsx ppsx33_render ppsx33_resolution)
  # Import libs must exist: WINDOWS_EXPORT_ALL_SYMBOLS + explicit rsx export
  set_target_properties(ps3rt PROPERTIES PREFIX "" OUTPUT_NAME "ps3rt"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_dependencies(ps3rt ppsx33_api ppsx33_render ppsx33_rsx ppsx33_resolution)
  add_custom_command(TARGET ps3rt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
    COMMAND ${CMAKE_COMMAND}
      -DPS3_DIST=${CMAKE_BINARY_DIR}/dist
      -P ${CMAKE_SOURCE_DIR}/cmake/CopyVulkanRuntime.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_resolution> ${CMAKE_BINARY_DIR}/dist/resolution.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_api> ${CMAKE_BINARY_DIR}/dist/api.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_render> ${CMAKE_BINARY_DIR}/dist/render.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_rsx> ${CMAKE_BINARY_DIR}/dist/rsx.dll
    COMMENT "dist/: ps3rt + resolution + api + render + rsx + vulkan-1.dll"
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

# Default host build MUST produce runtime DLLs (game prebuilt path).
add_custom_target(ppsx33_runtime_modules ALL
  DEPENDS ps3rt ppsx33_api ppsx33_rsx ppsx33_render ppsx33_resolution
  COMMENT "Runtime DLLs: ps3rt api rsx render resolution")

# C# / dotnet GUI removed — host UI is Qt6 target PPSX33

message(STATUS "Layout: src/core, src/processor, src/RSX, src/games-stuff, src/ui_qt")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/dist")
message(STATUS "Layout: dist/ps3rt.dll + dist/vulkan-1.dll next to EXE; dist/runtime/ = headers only")

# ---- PPSX33 Qt6 desktop UI (replaces former C# WinForms UI) ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
# Vulkan used via dynamic load in api.dll — headers not required at configure time.
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders ON)
set(PPSX33_ICON_ICO "${CMAKE_SOURCE_DIR}/assets/ppsx33-logo.ico")
set(PPSX33_ICON_PNG "${CMAKE_SOURCE_DIR}/assets/ppsx33-logo.png")
set(PPSX33_RC "${CMAKE_BINARY_DIR}/ppsx33_app_icon.rc")
if(WIN32 AND EXISTS "${PPSX33_ICON_ICO}")
  file(WRITE "${PPSX33_RC}" "IDI_ICON1 ICON \"${PPSX33_ICON_ICO}\"\n")
  set(PPSX33_RC_SRC "${PPSX33_RC}")
else()
  set(PPSX33_RC_SRC "")
endif()
add_executable(PPSX33
  src/ui_qt/main.cpp
  src/ui_qt/MainWindow.cpp
  src/ui_qt/MainWindow.h
  src/ui_qt/RenderWorkbench.cpp
  src/ui_qt/RenderWorkbench.h
  src/ui_qt/NativeBridge.cpp
  src/ui_qt/NativeBridge.h
  src/ui_qt/Settings.cpp
  src/ui_qt/Settings.h
  src/ui_qt/TermsDialog.cpp
  src/ui_qt/TermsDialog.h
  src/ui_qt/AppVersion.h
  src/ui_qt/UiLogoAnim.h
  src/ui_qt/UiLogoAnim.cpp
  src/ui_qt/EmbeddedTerms.h
  src/ui_qt/EmbeddedTerms.cpp
  src/ui_qt/EmbeddedLegal.h
  src/ui_qt/EmbeddedLegal.cpp
  src/ui_qt/resources.qrc
  ${PPSX33_RC_SRC}
)
target_link_libraries(PPSX33 PRIVATE Qt6::Widgets)
target_include_directories(PPSX33 PRIVATE ${CMAKE_SOURCE_DIR}/src/ui_qt ${CMAKE_SOURCE_DIR}/src/core)
set_target_properties(PPSX33 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
  WIN32_EXECUTABLE TRUE
)
if(TARGET ps3rt)
  add_dependencies(PPSX33 ps3core ps3rt ppsx33_api ppsx33_rsx ppsx33_render ppsx33_resolution ppsx33_runtime_modules)
else()
endif()
if(WIN32)
  add_custom_command(TARGET PPSX33 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3core> $<TARGET_FILE_DIR:PPSX33>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3rt> $<TARGET_FILE_DIR:PPSX33>/ps3rt.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_api> $<TARGET_FILE_DIR:PPSX33>/api.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_rsx> $<TARGET_FILE_DIR:PPSX33>/rsx.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_render> $<TARGET_FILE_DIR:PPSX33>/render.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ppsx33_resolution> $<TARGET_FILE_DIR:PPSX33>/resolution.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_SOURCE_DIR}/assets/ppsx33-logo.png $<TARGET_FILE_DIR:PPSX33>/ppsx33-logo.png
    COMMENT "dist/: EXE + ps3core + ps3rt + modular DLLs + logo")
endif()
