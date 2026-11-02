// WORK: PPU run support (ppu_runtime.h).

// ppu_runtime.h - Phase F (null-table skip + TOC restore)
#pragma once
#include "ppu_runtime_core.h"
#include "ppu_runtime_run.h"

#ifndef PS3RT_API
#  define PS3RT_API
#endif
PS3RT_API void ps3rt_rsx_set_color_surface(uint32_t offset, uint32_t w, uint32_t h);
