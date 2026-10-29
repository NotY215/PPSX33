#pragma once
// Pipeline stage markers for log diagnosis.
// If the log stops after a stage, the next subsystem is the suspect.
// Order: BOOT -> MEM -> PPU entry -> GCM -> RSX -> PPU run -> SPU -> EXIT
#ifndef PS3RT_STAGE_H
#define PS3RT_STAGE_H
PS3RT_API void ps3rt_log(const char* fmt, ...);
#define PS3RT_STAGE(msg) ps3rt_log("[stage] " msg "\n")
#endif
