# Runtime modules

| File | Role |
|------|------|
| ps3rt.cpp | Boot, map, LV2, PRX/GCM, progress |
| rsx_stub.cpp | RSX + D3D present |
| spu_stub.cpp | SPU tick |
| ppu_runtime_run.h | PPU loop |
| nid_table.h | NIDs |
| ps3rt_stage.h | Stage markers |

If log stops after a stage, next subsystem is the suspect.
