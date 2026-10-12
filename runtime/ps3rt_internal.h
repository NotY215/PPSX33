#pragma once
// Shared internal surface between runtime modules.
// Modules:
//   ps3rt.cpp      - boot, log, exit, progress_hw orchestration
//   rsx_stub.cpp   - RSX FIFO + D3D present
//   spu_stub.cpp   - SPU tick helper
//   nid_table.h    - NID database
//   ps3rt_stage.h  - [stage] log markers
//   ppu_runtime_run.h - PPU interpreter loop
//
// Log stages (in order):
//   BOOT start -> MEM map end -> BOOT end -> GCM ready end ->
//   RSX ready end -> PPU run start -> PPU progress start ->
//   SPU progress end -> PPU run end -> EXIT requested
