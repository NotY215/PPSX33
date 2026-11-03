# ASM_MASM optional — enable in MSVC builds when assembler present
# enable_language(ASM_MASM)
cmake_minimum_required(VERSION 3.20)
project(PPSX33 VERSION 1.1.0 LANGUAGES C CXX)
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
  ${CMAKE_SOURCE_DIR}/src/RSX/dispatch
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


# ---- Unified runtime: single ps3rt.dll (no api/rsx/render/resolution split, no -render) ----
set(PPSX33_COMMON_INCLUDES
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/core/import
  ${CMAKE_SOURCE_DIR}/src/core/hle
  ${CMAKE_SOURCE_DIR}/src/core/hle/gcm
  ${CMAKE_SOURCE_DIR}/src/native/x64
  ${CMAKE_SOURCE_DIR}/src/processor/PPU
  ${CMAKE_SOURCE_DIR}/src/processor/PPU/translate
  ${CMAKE_SOURCE_DIR}/src/processor/SPU
  ${CMAKE_SOURCE_DIR}/src/RSX
  ${CMAKE_SOURCE_DIR}/src/RSX/fifo
  ${CMAKE_SOURCE_DIR}/src/RSX/dispatch
  ${CMAKE_SOURCE_DIR}/src/RSX/present
  ${CMAKE_SOURCE_DIR}/src/RSX/draw
  ${CMAKE_SOURCE_DIR}/src/RSX/render
  ${CMAKE_SOURCE_DIR}/src/RSX/resolution
  ${CMAKE_SOURCE_DIR}/src/core/stats
  ${CMAKE_SOURCE_DIR}/src/RSX/Backends/Vulkan
  ${CMAKE_SOURCE_DIR}/src/RSX/Backends/OpenGL
  ${CMAKE_SOURCE_DIR}/src/games-stuff/Audio
)

if(WIN32)
  add_library(ps3rt SHARED
    src/core/hle_kernel.cpp
    src/core/assets.cpp
    src/RSX/rsx_device.cpp
    src/core/import/import_stub_map.c
    src/core/stats/host_stats.c
    src/core/hle/sysmodule_progress.c
    src/core/hle/gcm/gcm_control_bridge.c
    src/core/hle/gcm/gcm_flip_label.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/processor/SPU/spu_interpreter.cpp
    src/games-stuff/Audio/audio_device.cpp
    # former modular sources (single DLL)
    src/RSX/resolution/rsx_resolution.c
    src/RSX/Backends/Vulkan/rsx_vk_backend.cpp
    src/RSX/Backends/Vulkan/rsx_vk_spirv_blit.cpp
    src/RSX/Backends/OpenGL/rsx_gl_backend.cpp
    src/RSX/Backends/Vulkan/rsx_vk_memory.cpp
    src/RSX/Backends/Vulkan/rsx_vk_command.cpp
    src/RSX/Backends/Vulkan/rsx_vk_pipelines.cpp
    src/RSX/present/rsx_present_queue.c
    src/RSX/present/rsx_present_policy.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
    src/RSX/rsx_fifo.cpp
    src/RSX/rsx_state.cpp
    src/RSX/rsx_memory.cpp
    src/RSX/rsx_framebuffer.cpp
    src/RSX/rsx_texture.cpp
    src/RSX/rsx_shader_compiler.cpp
    src/RSX/rsx_shader_decoder.cpp
    src/RSX/rsx_shader_bind.c
    src/RSX/fifo/rsx_method_table.c
    src/RSX/fifo/rsx_fifo_drain.c
    src/RSX/methods/rsx_nv4097_surface.c
    src/RSX/dispatch/rsx_dispatch_sink.c
    src/native/x64/ps3rt_be.c
    src/RSX/rsx_exports.c
  )
  target_include_directories(ps3rt PRIVATE ${PPSX33_COMMON_INCLUDES}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/import
    ${CMAKE_SOURCE_DIR}/src/core/hle
    ${CMAKE_SOURCE_DIR}/src/core/hle/gcm
  )
  set_source_files_properties(
    src/core/import/import_stub_map.c
    src/processor/PPU/translate/ppu_import_bridge.c
    src/core/stats/host_stats.c
    src/core/hle/sysmodule_progress.c
    src/core/hle/gcm/gcm_control_bridge.c
    src/core/hle/gcm/gcm_flip_label.c
    src/RSX/resolution/rsx_resolution.c
    src/RSX/present/rsx_present_queue.c
    src/RSX/present/rsx_present_policy.c
    src/RSX/draw/rsx_draw_batch.c
    src/RSX/draw/rsx_surface_fmt.c
    src/RSX/render/rsx_render_frame.c
    src/RSX/render/rsx_light_present.c
    src/RSX/fifo/rsx_method_table.c
    src/RSX/fifo/rsx_fifo_drain.c
    src/RSX/methods/rsx_nv4097_surface.c
    src/RSX/dispatch/rsx_dispatch_sink.c
    src/native/x64/ps3rt_be.c
    src/RSX/rsx_exports.c
    src/RSX/rsx_shader_bind.c
    PROPERTIES LANGUAGE C)
  target_compile_definitions(ps3rt PRIVATE
    PS3RT_BUILD_DLL=1
    PPSX33_API_BUILD=1
    PPSX33_RSX_BUILD=1
    PPSX33_RENDER_BUILD=1
    PPSX33_RES_BUILD=1)
  if(MSVC)
    target_compile_options(ps3rt PRIVATE /bigobj)
  endif()
  target_link_libraries(ps3rt PRIVATE
    user32 gdi32 ole32 uuid windowscodecs winmm xaudio2 psapi
  opengl32
)
  set_target_properties(ps3rt PROPERTIES PREFIX "" OUTPUT_NAME "ps3rt"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_custom_command(TARGET ps3rt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
    COMMAND ${CMAKE_COMMAND}
      -DPS3_DIST=${CMAKE_BINARY_DIR}/dist
      -P ${CMAKE_SOURCE_DIR}/cmake/CopyVulkanRuntime.cmake
    COMMENT "dist/: ps3rt.dll + vulkan-1.dll"
  )
endif()

# Headers for generated game sources
set(PS3RT_DIST_HEADERS
  ppu_runtime.h ppu_runtime_core.h ppu_runtime_core_a.h ppu_runtime_core_b.h
  ppu_runtime_run.h hle_kernel_internal.h hle_stage.h hle_kernel_syscalls.inl
  nid_table.h hle_registry.h lv2_syscalls.inl hle_status.h
)
add_custom_target(copy_runtime ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/runtime
  COMMENT "Preparing dist/runtime headers only"
)
set(_HDR_MAP
  "src/processor/PPU/ppu_runtime.h:ppu_runtime.h"
  "src/processor/PPU/ppu_runtime_core.h:ppu_runtime_core.h"
  "src/processor/PPU/ppu_runtime_core_a.h:ppu_runtime_core_a.h"
  "src/processor/PPU/ppu_runtime_core_b.h:ppu_runtime_core_b.h"
  "src/processor/PPU/ppu_runtime_run.h:ppu_runtime_run.h"
  "src/processor/PPU/ppu_island_table.h:ppu_island_table.h"
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
    file(WRITE "${PPSX33_RC}"
"#include <winver.h>
IDI_ICON1 ICON \"${PPSX33_ICON_ICO}\"

VS_VERSION_INFO VERSIONINFO
FILEVERSION 1,1,0,0
PRODUCTVERSION 1,1,0,0
FILEFLAGSMASK 0x3fL
FILEFLAGS 0x0L
FILEOS 0x40004L
FILETYPE 0x1L
FILESUBTYPE 0x0L
BEGIN
  BLOCK \"StringFileInfo\"
  BEGIN
    BLOCK \"040904B0\"
    BEGIN
      VALUE \"CompanyName\", \"PPSX33\"
      VALUE \"FileDescription\", \"PPSX33 Host\"
      VALUE \"FileVersion\", \"1.1.0-Alpha\"
      VALUE \"InternalName\", \"PPSX33\"
      VALUE \"OriginalFilename\", \"PPSX33.exe\"
      VALUE \"ProductName\", \"PPSX33\"
      VALUE \"ProductVersion\", \"1.1.0-Alpha\"
    END
  END
  BLOCK \"VarFileInfo\"
  BEGIN
    VALUE \"Translation\", 0x409, 1200
  END
END
")
  set(PPSX33_RC_SRC "${PPSX33_RC}")
else()
  set(PPSX33_RC_SRC "")
endif()
add_executable(PPSX33
  src/ui_qt/main.cpp
  src/ui_qt/MainWindow.cpp
  src/ui_qt/MainWindow.h
  src/ui_qt/NativeBridge.cpp
  src/ui_qt/NativeBridge.h
  src/ui_qt/Settings.cpp
  src/ui_qt/Settings.h
  src/ui_qt/TermsDialog.cpp
  src/ui_qt/TermsDialog.h
  src/ui_qt/AppVersion.h
  src/ui_qt/UiLogoAnim.h
  src/ui_qt/UiLogoAnim.cpp
  src/ui_qt/JobWorker.cpp
  src/ui_qt/JobWorker.h
  src/ui_qt/EmbeddedTerms.h
  src/ui_qt/EmbeddedTerms.cpp
  src/ui_qt/EmbeddedLegal.h
  src/ui_qt/EmbeddedLegal.cpp
  src/ui_qt/resources.qrc
  ${PPSX33_RC_SRC}
)
target_link_libraries(PPSX33 PRIVATE Qt6::Widgets)
target_compile_definitions(PPSX33 PRIVATE PPSX33_VERSION_STRING=\"1.1.0-Alpha\")
target_include_directories(PPSX33 PRIVATE ${CMAKE_SOURCE_DIR}/src/ui_qt ${CMAKE_SOURCE_DIR}/src/core)
set_target_properties(PPSX33 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/dist
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/dist
  WIN32_EXECUTABLE TRUE
)
if(TARGET ps3rt)
  add_dependencies(PPSX33 ps3core ps3rt)
else()
endif()
if(WIN32)
  add_custom_command(TARGET PPSX33 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3core> $<TARGET_FILE_DIR:PPSX33>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ps3rt> $<TARGET_FILE_DIR:PPSX33>/ps3rt.dll
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_SOURCE_DIR}/assets/ppsx33-logo.png $<TARGET_FILE_DIR:PPSX33>/ppsx33-logo.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_SOURCE_DIR}/assets/ppsx33-logo.ico $<TARGET_FILE_DIR:PPSX33>/ppsx33-logo.ico
    COMMENT "dist/: EXE + ps3core + ps3rt + modular DLLs + logo")
endif()

# Per-game launcher (copied next to game EXE as Modify.exe)
add_executable(PPSX33Modify WIN32 src/tools/modify/modify_main.cpp)
target_compile_definitions(PPSX33Modify PRIVATE UNICODE _UNICODE)

set_target_properties(PPSX33Modify PROPERTIES OUTPUT_NAME "Modify" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
target_link_libraries(PPSX33Modify PRIVATE user32 gdi32 comctl32 shell32)

# Modify.exe must sit next to PPSX33.exe so Port can ship it with each game
add_dependencies(PPSX33 PPSX33Modify)
add_custom_command(TARGET PPSX33 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:PPSX33Modify>
    $<TARGET_FILE_DIR:PPSX33>/Modify.exe
  COMMENT "Port source: Modify.exe next to PPSX33.exe")
