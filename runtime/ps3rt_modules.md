# Runtime module map

| File | Responsibility | Stage tags |
|------|----------------|------------|
| ps3rt.cpp | Boot, memory map, LV2, PRX/GCM, progress_hw, SPU HLE | BOOT, MEM, GCM, PPU progress, SPU, EXIT |
| rsx_stub.cpp | RSX methods, D3D11 window, Present | RSX ready |
| spu_stub.cpp | SPU tick link stub | - |
| ppu_runtime_run.h | PPU interpreter loop | PPU run start/end |
| nid_table.h | NID name table | - |
| ps3rt_stage.h | PS3RT_STAGE() macro | - |
| ps3rt_log.cpp | Optional log helpers | - |
| ps3rt_hw.cpp | Optional HW helpers | - |

If a log stops after a stage tag, the **next** row is the likely fault.
