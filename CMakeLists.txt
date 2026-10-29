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
    src/core/hle_log.cpp
    src/core/hle_hw_progress.cpp
    src/core/assets.cpp
    src/RSX/rsx_device.cpp
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
