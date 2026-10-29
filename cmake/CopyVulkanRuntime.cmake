# Copy vulkan-1.dll next to PS3Recomp.exe / ps3rt.dll in dist/
# Does not require Vulkan SDK at build time — searches common install locations.

if(NOT PS3_DIST)
  message(FATAL_ERROR "PS3_DIST not set")
endif()

file(MAKE_DIRECTORY "${PS3_DIST}")

set(_vk_candidates
  "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll"
  "$ENV{VULKAN_SDK}/Bin32/vulkan-1.dll"
  "C:/Windows/System32/vulkan-1.dll"
  "C:/Windows/SysWOW64/vulkan-1.dll"
  "$ENV{ProgramFiles}/VulkanSDK/*/Bin/vulkan-1.dll"
)

set(_vk_src "")
foreach(c ${_vk_candidates})
  file(GLOB _g ${c})
  foreach(f ${_g})
    if(EXISTS "${f}")
      set(_vk_src "${f}")
      break()
    endif()
  endforeach()
  if(_vk_src)
    break()
  endif()
  if(EXISTS "${c}")
    set(_vk_src "${c}")
    break()
  endif()
endforeach()

if(_vk_src)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_vk_src}" "${PS3_DIST}/vulkan-1.dll")
  message(STATUS "Copied vulkan-1.dll -> ${PS3_DIST} (from ${_vk_src})")
else()
  message(WARNING "vulkan-1.dll not found on build machine — place it next to the EXE for Vulkan present")
endif()
